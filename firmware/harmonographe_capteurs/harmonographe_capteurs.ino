/**
 * ╔══════════════════════════════════════════════════════════════════════╗
 * ║          HARMONOGRAPHE DE SABLE INTERACTIF — Style Sisyphus         ║
 * ║                                                                      ║
 * ║  Machine artistique : bille d'acier sur sable guidée par aimant     ║
 * ║  Base : Shapeoko CNC | Contrôle : Arduino Mega 2560                 ║
 * ║  Algorithme : Harmonographe à 3 pendules virtuels                   ║
 * ║  Modulation : Température, Humidité, Distance, Son                  ║
 * ╚══════════════════════════════════════════════════════════════════════╝
 *
 * Bibliothèques requises (gestionnaire de bibliothèques Arduino IDE) :
 *   - DHT sensor library     (Adafruit)      → capteur DHT22
 *   - Adafruit Unified Sensor (Adafruit)     → dépendance DHT
 *   - NewPing                (Tim Eckel)     → capteur HC-SR04
 *   - LiquidCrystal I2C      (Frank de Brabander) → écran LCD
 *   - FastLED                (FastLED)       → LEDs WS2812B (optionnel)
 *
 * Architecture :
 *   Cet Arduino (Mega #2) lit les capteurs, calcule les points de courbe
 *   harmonographe, et envoie les coordonnées en G-code à l'Arduino #1
 *   (Shapeoko, firmware GRBL) via Serial1.
 *
 * Pins utilisées :
 *   Pin 2  : DHT22 (données)
 *   Pin 3  : HC-SR04 TRIG
 *   Pin 4  : HC-SR04 ECHO
 *   Pin 6  : LED WS2812B données
 *   Pin 7  : Bouton mode (INPUT_PULLUP)
 *   Pin 8  : Buzzer passif
 *   Pin 9  : Encodeur CLK
 *   Pin 10 : Encodeur DT
 *   Pin 11 : Encodeur SW
 *   Pin 13 : LED statut
 *   Pin 18 : TX1 → GRBL RX
 *   Pin 19 : RX1 ← GRBL TX
 *   Pin 20 : SDA (LCD I2C)
 *   Pin 21 : SCL (LCD I2C)
 *   A0     : MAX9814 (microphone)
 *   A1     : Potentiomètre vitesse
 */

// ─── Bibliothèques ────────────────────────────────────────────────────────
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Activer FastLED uniquement si la bande LED est installée
#define USE_FASTLED 1
#if USE_FASTLED
  #include <FastLED.h>
  #define LED_PIN        6
  #define NUM_LEDS       64      // Adapter au nombre de LEDs installées
  #define LED_BRIGHTNESS 80      // 0-255
  CRGB leds[NUM_LEDS];
#endif

// ─── Modules du projet ───────────────────────────────────────────────────
#include "curves.h"        // Algorithme harmonographe
#include "sensors.h"       // Gestion des capteurs
#include "gcode_sender.h"  // Communication GRBL

// ─── Configuration LCD ────────────────────────────────────────────────────
#define LCD_I2C_ADDR    0x27    // Adresse I2C (0x27 ou 0x3F selon modèle)
#define LCD_COLS        20
#define LCD_ROWS        4
LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);

// ─── Pins interface utilisateur ───────────────────────────────────────────
#define BTN_MODE_PIN    7
#define BUZZER_PIN      8
#define ENC_CLK_PIN     9
#define ENC_DT_PIN      10
#define ENC_SW_PIN      11
#define POT_SPEED_PIN   A1

// ─── États de la machine ─────────────────────────────────────────────────
enum MachineMode {
    MODE_BOOT,          // Démarrage et initialisation
    MODE_HOMING,        // Recherche des origines
    MODE_CALIBRATE,     // Test de calibration (cercle)
    MODE_RUNNING,       // Mode dessin harmonographe (principal)
    MODE_PAUSED,        // Pause (attente bouton)
    MODE_ERROR          // Erreur (GRBL non connecté, etc.)
};

// ─── Variables globales ───────────────────────────────────────────────────
static MachineMode g_mode          = MODE_BOOT;
static CurveState  g_curveState;
static uint32_t    g_pointCount    = 0;       // Nb de points tracés
static uint32_t    g_cycleCount    = 0;       // Nb de cycles complets
static uint32_t    g_lastLcdUpdate = 0;       // Timestamp LCD
static uint32_t    g_lastDebugPrint = 0;
static bool        g_btnModePressed = false;
static int         g_encoderPos    = 0;       // Position encodeur
static bool        g_usePotSpeed   = false;   // Vitesse manuelle par pot.

// ─── Constantes ───────────────────────────────────────────────────────────
#define LCD_UPDATE_INTERVAL_MS   500    // Rafraîchissement LCD toutes les 500ms
#define DEBUG_PRINT_INTERVAL_MS  3000   // Debug toutes les 3s
#define POINTS_PER_CYCLE         8000   // Points avant de redémarrer un cycle
#define RETURN_RADIUS_THRESHOLD  160.0f // Distance du centre pour déclencher retour

// ─── Fonctions ───────────────────────────────────────────────────────────

/**
 * Émet un bip court sur le buzzer.
 */
void beep(int freq = 1000, int durationMs = 50) {
    tone(BUZZER_PIN, freq, durationMs);
}

/**
 * Émet une séquence sonore de démarrage.
 */
void play_startup_melody() {
    int notes[] = {523, 659, 784, 1047};  // Do Mi Sol Do (octave)
    for (int i = 0; i < 4; i++) {
        tone(BUZZER_PIN, notes[i], 120);
        delay(150);
    }
}

/**
 * Lit l'encodeur rotatif (interruption simulée par polling).
 * Retourne +1, -1, ou 0.
 */
int read_encoder_delta() {
    static int lastClk = HIGH;
    int clk = digitalRead(ENC_CLK_PIN);
    if (clk != lastClk && clk == LOW) {
        lastClk = clk;
        return (digitalRead(ENC_DT_PIN) == LOW) ? +1 : -1;
    }
    lastClk = clk;
    return 0;
}

/**
 * Lit le bouton mode (avec anti-rebond logiciel).
 * Retourne true sur front montant (appui).
 */
bool read_button_mode() {
    static bool lastState = HIGH;
    static uint32_t lastDebounce = 0;
    bool state = digitalRead(BTN_MODE_PIN);
    if (state != lastState) {
        lastDebounce = millis();
        lastState = state;
    }
    if ((millis() - lastDebounce) > 50 && state == LOW && lastState == LOW) {
        lastState = HIGH;  // Réarmer
        return true;
    }
    return false;
}

// ─── Affichage LCD ────────────────────────────────────────────────────────

/**
 * Met à jour l'écran LCD avec les informations courantes.
 *
 * Layout 20×4 :
 * ┌────────────────────┐
 * │ HARMONOGRAPHE SAND │  Ligne 0 : Titre
 * │ T:22.3C  H:58%     │  Ligne 1 : Capteurs T & H
 * │ D:32cm  S:0.12     │  Ligne 2 : Distance & Son
 * │ Pts:12345 C:7      │  Ligne 3 : Stats
 * └────────────────────┘
 */
void lcd_update() {
    const RawSensorData &raw = sensors_get_raw();
    char buf[21];

    // Ligne 0 : mode courant
    lcd.setCursor(0, 0);
    switch (g_mode) {
        case MODE_BOOT:      lcd.print(F("  HARMONOGRAPHE     ")); break;
        case MODE_HOMING:    lcd.print(F("   -- HOMING --     ")); break;
        case MODE_CALIBRATE: lcd.print(F("  -- CALIBRATION -- ")); break;
        case MODE_RUNNING:   lcd.print(F("   DESSIN ACTIF     ")); break;
        case MODE_PAUSED:    lcd.print(F("  *** PAUSE ***     ")); break;
        case MODE_ERROR:     lcd.print(F("  !!! ERREUR !!!    ")); break;
    }

    // Ligne 1 : température et humidité
    lcd.setCursor(0, 1);
    snprintf(buf, sizeof(buf), "T:%-5.1fC  H:%-3.0f%%   ",
             raw.temperature, raw.humidity);
    lcd.print(buf);

    // Ligne 2 : distance et son
    lcd.setCursor(0, 2);
    snprintf(buf, sizeof(buf), "D:%-4.0fcm  S:%-5.2f  ",
             raw.distanceCm, raw.soundRms);
    lcd.print(buf);

    // Ligne 3 : statistiques
    lcd.setCursor(0, 3);
    snprintf(buf, sizeof(buf), "Pts:%-6lu C:%-5lu  ",
             g_pointCount, g_cycleCount);
    lcd.print(buf);
}

// ─── Mise à jour des LEDs WS2812B ────────────────────────────────────────

#if USE_FASTLED
/**
 * Met à jour les LEDs RGB en fonction des capteurs.
 * - Température → teinte (bleu froid → rouge chaud)
 * - Son → éclat / pulsation
 * - Distance → intensité (quelqu'un proche → plus lumineux)
 */
void leds_update(const NormalizedSensors &sensors) {
    static float ledPhase = 0.0f;
    ledPhase += 0.03f;

    for (int i = 0; i < NUM_LEDS; i++) {
        float pos = (float)i / NUM_LEDS;

        // Teinte de base : bleu (froid) → rouge (chaud)
        // sensors.temperature : 0=bleu, 0.5=vert/jaune, 1=rouge
        float hue = (1.0f - sensors.temperature) * 180.0f;  // 180=bleu, 0=rouge

        // Saturation : humidité élevée → couleurs plus saturées
        float sat = 180.0f + sensors.humidity * 75.0f;

        // Luminosité : pulsation avec le son + distance
        float pulse = (sinf(ledPhase + pos * TWO_PI * 2.0f) + 1.0f) * 0.5f;
        float brightness = 40.0f
                         + sensors.distance * 80.0f     // Proche → plus lumineux
                         + sensors.sound    * 100.0f    // Son → flashs
                         + pulse * 30.0f;               // Pulsation douce
        brightness = constrain(brightness, 0.0f, 255.0f);

        leds[i] = CHSV((uint8_t)hue, (uint8_t)sat, (uint8_t)brightness);
    }
    FastLED.show();
}
#endif

// ─── Gestion des cycles de dessin ────────────────────────────────────────

/**
 * Initialise un nouveau cycle de dessin.
 * Retour au centre + reset du temps de la courbe.
 */
void start_new_cycle() {
    g_cycleCount++;
    g_pointCount = 0;

    // Retour au centre en spirale douce
    float fromX = g_grblState.currentX;
    float fromY = g_grblState.currentY;
    float pts[2][2];  // Buffer minimal (utilisé point par point)

    Serial.print(F("Nouveau cycle #"));
    Serial.println(g_cycleCount);

    // Spirale de retour au centre
    int steps = 40;
    for (int i = 0; i <= steps; i++) {
        float alpha = smoothstep3((float)i / steps);
        float retX = fromX * (1.0f - alpha);
        float retY = fromY * (1.0f - alpha);
        grbl_move_to(retX, retY, 800.0f);
    }

    // Pause au centre
    delay(300);

    // Réinitialiser le temps de la courbe (repart de t=0)
    // Garder les paramètres courants pour une transition douce
    g_curveState.t = 0.0f;
    g_curveState.isInterpolating = false;

    beep(880, 80);
}

// ─── setup() ─────────────────────────────────────────────────────────────

void setup() {
    // Port série USB pour debug
    Serial.begin(115200);
    Serial.println(F("\n╔═══════════════════════════════════╗"));
    Serial.println(F("║  HARMONOGRAPHE DE SABLE v1.0      ║"));
    Serial.println(F("║  Style Sisyphus — Base Shapeoko   ║"));
    Serial.println(F("╚═══════════════════════════════════╝"));

    // Pins interface
    pinMode(BTN_MODE_PIN, INPUT_PULLUP);
    pinMode(BUZZER_PIN,   OUTPUT);
    pinMode(ENC_CLK_PIN,  INPUT_PULLUP);
    pinMode(ENC_DT_PIN,   INPUT_PULLUP);
    pinMode(ENC_SW_PIN,   INPUT_PULLUP);

    // LCD
    Wire.begin();
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print(F("HARMONOGRAPHE"));
    lcd.setCursor(4, 1);
    lcd.print(F("DE SABLE"));
    lcd.setCursor(2, 2);
    lcd.print(F("Style Sisyphus"));
    lcd.setCursor(1, 3);
    lcd.print(F("Initialisation..."));

    // LEDs (optionnel)
#if USE_FASTLED
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(LED_BRIGHTNESS);
    FastLED.clear();
    FastLED.show();
#endif

    // Mélodie de démarrage
    play_startup_melody();
    delay(1000);

    // Initialiser les capteurs
    Serial.println(F("Init capteurs..."));
    lcd.setCursor(0, 3);
    lcd.print(F("Capteurs...         "));
    sensors_init();
    Serial.println(F("Capteurs OK"));

    // Initialiser l'algorithme de courbes
    curves_init(g_curveState);
    Serial.println(F("Algorithme courbes OK"));

    // Connexion GRBL
    Serial.println(F("Connexion GRBL..."));
    lcd.setCursor(0, 3);
    lcd.print(F("Connexion GRBL...   "));

    if (!grbl_init()) {
        Serial.println(F("ERREUR: GRBL non connecté!"));
        lcd.setCursor(0, 3);
        lcd.print(F("!ERREUR GRBL!       "));
        g_mode = MODE_ERROR;
        return;
    }
    Serial.println(F("GRBL connecté OK"));

    // Homing
    g_mode = MODE_HOMING;
    lcd.setCursor(0, 3);
    lcd.print(F("Homing...           "));

    if (!grbl_home()) {
        Serial.println(F("ATTENTION: Homing échoué, mode sans homing"));
        // Continuer sans homing (moins précis mais fonctionnel)
        // Envoyer simplement au centre présumé
        grbl_send_line("G92 X0 Y0");  // Définir position courante comme origine
    }

    // Test de calibration rapide
    g_mode = MODE_CALIBRATE;
    lcd.setCursor(0, 3);
    lcd.print(F("Test cercle...      "));
    Serial.println(F("Test cercle calibration..."));

    // Petit cercle de test Ø50mm à vitesse lente
    grbl_move_to(25.0f, 0.0f, 500.0f);  // Aller au départ du cercle
    delay(500);
    grbl_send_circle_test(25.0f, 300.0f);
    grbl_go_to_center(800.0f);

    beep(1047, 100);
    delay(500);

    // Prêt !
    g_mode = MODE_RUNNING;
    lcd.clear();
    Serial.println(F("\n>>> DESSIN EN COURS <<<"));
    Serial.println(F("Appuyer sur MODE pour pause/reprendre"));

    // Première mise à jour LCD
    lcd_update();
}

// ─── loop() ──────────────────────────────────────────────────────────────

void loop() {
    // ── 1. Gestion du bouton MODE (pause/reprendre) ───────────────────
    if (read_button_mode()) {
        if (g_mode == MODE_RUNNING) {
            g_mode = MODE_PAUSED;
            grbl_emergency_stop();  // Stop immédiat propre
            beep(440, 200);
            Serial.println(F("PAUSE"));
        } else if (g_mode == MODE_PAUSED) {
            g_mode = MODE_RUNNING;
            grbl_init();  // Réinitialiser GRBL après arrêt d'urgence
            beep(880, 100);
            Serial.println(F("REPRISE"));
        }
        delay(200);  // Anti-rebond supplémentaire
    }

    // ── 2. Mode pause : ne rien faire ────────────────────────────────
    if (g_mode != MODE_RUNNING) {
        sensors_update();  // Continuer à lire les capteurs (LCD update)

        // Mise à jour LCD même en pause
        if ((millis() - g_lastLcdUpdate) >= LCD_UPDATE_INTERVAL_MS) {
            g_lastLcdUpdate = millis();
            lcd_update();
        }
        delay(50);
        return;
    }

    // ── 3. Mode dessin actif ─────────────────────────────────────────

    // 3a. Lire les capteurs (mise à jour non-bloquante)
    sensors_update();
    const NormalizedSensors &sensors = sensors_get_normalized();

    // 3b. Lire l'encodeur (ajustement manuel de vitesse)
    int enc = read_encoder_delta();
    if (enc != 0) {
        g_encoderPos += enc;
        g_usePotSpeed = false;
        // L'encodeur permet d'ajuster ±20% la vitesse de base
        // (modulé dans curves_from_sensors via encoderPos)
    }

    // 3c. Avancer l'état de la courbe (mise à jour des paramètres)
    curves_advance(g_curveState, sensors);

    // 3d. Calculer le prochain point de la courbe
    float x, y;
    curves_get_point(g_curveState, x, y);

    // 3e. Lire la vitesse (pot. manuel ou calculée)
    float feedrate = g_curveState.current.feedrate;
    if (g_usePotSpeed) {
        // Potentiomètre : 200 - 2500 mm/min
        int potVal = analogRead(POT_SPEED_PIN);
        feedrate = map(potVal, 0, 1023, 200, 2500);
    }
    // Ajustement encodeur ±20% (g_encoderPos ∈ [-10, +10] idéalement)
    feedrate *= (1.0f + g_encoderPos * 0.02f);
    feedrate = constrain(feedrate, 150.0f, 3000.0f);

    // 3f. Envoyer le point au GRBL
    bool ok = grbl_move_to(x, y, feedrate);

    if (!ok) {
        g_grblState.errorCount++;
        if (g_grblState.errorCount > 5) {
            g_mode = MODE_ERROR;
            Serial.println(F("ERREUR: Trop d'erreurs GRBL, arrêt!"));
            return;
        }
    } else {
        g_grblState.errorCount = 0;  // Reset compteur si OK
        g_pointCount++;
    }

    // 3g. Vérifier si un nouveau cycle est nécessaire
    if (g_pointCount >= POINTS_PER_CYCLE) {
        start_new_cycle();
    }

    // 3h. Mise à jour LCD (non-bloquante)
    if ((millis() - g_lastLcdUpdate) >= LCD_UPDATE_INTERVAL_MS) {
        g_lastLcdUpdate = millis();
        lcd_update();
    }

    // 3i. Mise à jour LEDs RGB (optionnel)
#if USE_FASTLED
    leds_update(sensors);
#endif

    // 3j. Debug série (toutes les N secondes)
    if ((millis() - g_lastDebugPrint) >= DEBUG_PRINT_INTERVAL_MS) {
        g_lastDebugPrint = millis();
        Serial.print(F("Pt #")); Serial.print(g_pointCount);
        Serial.print(F(" Cycle #")); Serial.print(g_cycleCount);
        Serial.print(F(" X=")); Serial.print(x, 1);
        Serial.print(F(" Y=")); Serial.print(y, 1);
        Serial.print(F(" F=")); Serial.print(feedrate, 0);
        Serial.print(F(" t=")); Serial.println(g_curveState.t, 1);
        sensors_print_debug(Serial);
        Serial.println(F("---"));
    }

    // Note : pas de delay() ici — la boucle est cadencée par
    // le temps de réponse GRBL (attente du "ok" = sync naturel)
}
