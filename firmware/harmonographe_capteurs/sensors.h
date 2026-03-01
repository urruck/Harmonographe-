/**
 * sensors.h — Gestion des capteurs ambiants
 *
 * Lit et filtre les données de :
 *   - DHT22 : température + humidité
 *   - HC-SR04 : distance ultrasonique
 *   - MAX9814 : niveau sonore ambiant
 *
 * Fournit des valeurs normalisées 0.0-1.0 pour l'algorithme de courbes.
 */

#ifndef SENSORS_H
#define SENSORS_H

#include <DHT.h>
#include <NewPing.h>
#include "curves.h"   // Pour NormalizedSensors

// ─── Pins ─────────────────────────────────────────────────────────────────
#define DHT_PIN         2
#define DHT_TYPE        DHT22
#define TRIG_PIN        3
#define ECHO_PIN        4
#define MIC_PIN         A0
#define POT_SPEED_PIN   A1
#define BTN_MODE_PIN    7
#define LED_STATUS_PIN  13

// ─── Paramètres HC-SR04 ───────────────────────────────────────────────────
#define MAX_DISTANCE_CM 450     // Distance max ultrason (cm)
#define MIN_DISTANCE_CM 4       // Distance min fiable (cm)
#define MAX_DETECT_CM   55      // Distance max considérée comme "proche"

// ─── Paramètres DHT22 ────────────────────────────────────────────────────
#define TEMP_MIN        12.0f   // Température min (°C) pour normalisation
#define TEMP_MAX        38.0f   // Température max (°C)
#define HUM_MIN         25.0f   // Humidité min (%) pour normalisation
#define HUM_MAX         92.0f   // Humidité max (%)

// ─── Paramètres microphone ───────────────────────────────────────────────
#define MIC_SAMPLES         64      // Nombre de lectures par mesure
#define MIC_SAMPLE_MS       1       // Délai entre lectures (ms)
#define SOUND_SMOOTH_TAU    0.08f   // Constante filtre passe-bas
#define SOUND_SPIKE_THRESH  0.65f   // Seuil pour détecter un son fort

// ─── Intervalles de lecture ───────────────────────────────────────────────
#define DHT_READ_INTERVAL_MS     2500   // DHT22 max 0.5Hz
#define SONIC_READ_INTERVAL_MS   80     // HC-SR04 (toutes les 80ms)
#define MIC_READ_INTERVAL_MS     50     // Microphone (toutes les 50ms)

// ─── Structure données capteurs brutes ────────────────────────────────────
struct RawSensorData {
    float temperature;   // °C
    float humidity;      // %
    float distanceCm;    // cm (0 = pas de détection)
    float soundRms;      // RMS normalisé 0.0-1.0
    bool  dhtValid;      // true si la lecture DHT est valide
    bool  sonicValid;    // true si la lecture ultrason est valide
    unsigned long timestamp;  // millis() de la dernière lecture
};

// ─── Objets bibliothèques ─────────────────────────────────────────────────
DHT    dht(DHT_PIN, DHT_TYPE);
NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE_CM);

// ─── Variables état interne ───────────────────────────────────────────────
static RawSensorData   g_rawData;
static NormalizedSensors g_normalized;
static unsigned long   g_lastDhtRead   = 0;
static unsigned long   g_lastSonicRead = 0;
static unsigned long   g_lastMicRead   = 0;
static float           g_soundFiltered = 0.0f;

// ─── Prototypes ──────────────────────────────────────────────────────────
void sensors_init();
void sensors_update();
const NormalizedSensors& sensors_get_normalized();
const RawSensorData& sensors_get_raw();
float sensors_read_sound_rms();
void sensors_print_debug(Stream &serial);

// ─── Implémentation ──────────────────────────────────────────────────────

/**
 * Initialise tous les capteurs et les valeurs par défaut.
 */
void sensors_init() {
    dht.begin();
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    pinMode(BTN_MODE_PIN, INPUT_PULLUP);
    pinMode(LED_STATUS_PIN, OUTPUT);

    // Valeurs par défaut (conditions ambiantes normales)
    g_rawData.temperature = 20.0f;
    g_rawData.humidity    = 50.0f;
    g_rawData.distanceCm  = MAX_DETECT_CM;
    g_rawData.soundRms    = 0.0f;
    g_rawData.dhtValid    = false;
    g_rawData.sonicValid  = false;

    g_normalized.temperature = 0.38f;  // ≈ 20°C
    g_normalized.humidity    = 0.42f;  // ≈ 50%
    g_normalized.distance    = 1.0f;   // Personne loin
    g_normalized.sound       = 0.0f;   // Silence

    // Première lecture DHT (délai nécessaire après begin)
    delay(2000);
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
        g_rawData.temperature = t;
        g_rawData.humidity    = h;
        g_rawData.dhtValid    = true;
    }
}

/**
 * Lit le niveau sonore RMS depuis le MAX9814.
 *
 * Méthode : prendre N échantillons, calculer la valeur RMS
 * (Root Mean Square) autour de la moyenne DC du signal.
 *
 * Le MAX9814 centre son signal sur VCC/2 ≈ 512 en ADC 10-bit.
 */
float sensors_read_sound_rms() {
    long sumDC  = 0;
    int samples[MIC_SAMPLES];

    // 1. Lire les échantillons et calculer la moyenne DC
    for (int i = 0; i < MIC_SAMPLES; i++) {
        samples[i] = analogRead(MIC_PIN);
        sumDC += samples[i];
        delay(MIC_SAMPLE_MS);
    }
    float dc = (float)sumDC / MIC_SAMPLES;

    // 2. Calculer la variance (puissance AC)
    float sumSq = 0.0f;
    for (int i = 0; i < MIC_SAMPLES; i++) {
        float diff = (float)samples[i] - dc;
        sumSq += diff * diff;
    }

    // 3. RMS = sqrt(variance)
    float rms = sqrtf(sumSq / MIC_SAMPLES);

    // 4. Normaliser [0, 1] → valeur max RMS attendue ≈ 150 ADC counts
    return constrain(rms / 150.0f, 0.0f, 1.0f);
}

/**
 * Utility : normalise une valeur dans une plage vers [0, 1].
 */
static float normalize(float value, float minVal, float maxVal) {
    return constrain((value - minVal) / (maxVal - minVal), 0.0f, 1.0f);
}

/**
 * Met à jour toutes les lectures capteurs selon les intervalles configurés.
 * Doit être appelée régulièrement dans la boucle principale.
 */
void sensors_update() {
    unsigned long now = millis();

    // ── DHT22 : température et humidité ──────────────────────────────
    if ((now - g_lastDhtRead) >= DHT_READ_INTERVAL_MS) {
        g_lastDhtRead = now;

        float t = dht.readTemperature();
        float h = dht.readHumidity();

        if (!isnan(t) && !isnan(h)) {
            // Filtre passe-bas du 1er ordre (lissage)
            // τ = 0.2 → les changements lents de T/H sont suivis
            const float tau = 0.2f;
            g_rawData.temperature = g_rawData.temperature * (1.0f - tau) + t * tau;
            g_rawData.humidity    = g_rawData.humidity    * (1.0f - tau) + h * tau;
            g_rawData.dhtValid    = true;

            // Normalisation
            g_normalized.temperature = normalize(g_rawData.temperature, TEMP_MIN, TEMP_MAX);
            g_normalized.humidity    = normalize(g_rawData.humidity,    HUM_MIN,  HUM_MAX);

            // LED de statut : clignote si T > 30°C (environnement chaud)
            if (g_rawData.temperature > 30.0f) {
                digitalWrite(LED_STATUS_PIN, (now / 500) % 2);
            } else {
                digitalWrite(LED_STATUS_PIN, HIGH);
            }
        } else {
            // Lecture invalide : garder les valeurs précédentes
            g_rawData.dhtValid = false;
        }
    }

    // ── HC-SR04 : distance ────────────────────────────────────────────
    if ((now - g_lastSonicRead) >= SONIC_READ_INTERVAL_MS) {
        g_lastSonicRead = now;

        // NewPing : renvoie 0 si hors portée
        unsigned int duration = sonar.ping();
        if (duration > 0) {
            float distCm = sonar.convert_cm(duration);
            if (distCm >= MIN_DISTANCE_CM && distCm <= MAX_DISTANCE_CM) {
                // Filtre passe-bas rapide (réaction en ~0.5s)
                const float tau = 0.25f;
                g_rawData.distanceCm = g_rawData.distanceCm * (1.0f - tau) + distCm * tau;
                g_rawData.sonicValid = true;
            } else {
                // Hors portée → personne n'est là
                g_rawData.distanceCm = MAX_DETECT_CM;
            }
        } else {
            // Pas d'écho → personne
            const float tau = 0.05f;
            g_rawData.distanceCm = g_rawData.distanceCm * (1.0f - tau) +
                                    MAX_DETECT_CM * tau;
        }

        // Normalisation : proche (5cm)=1.0, loin (55cm)=0.0
        g_normalized.distance = 1.0f - normalize(g_rawData.distanceCm,
                                                   MIN_DISTANCE_CM, MAX_DETECT_CM);
    }

    // ── MAX9814 : niveau sonore ───────────────────────────────────────
    if ((now - g_lastMicRead) >= MIC_READ_INTERVAL_MS) {
        g_lastMicRead = now;

        float rawRms = sensors_read_sound_rms();

        // Filtre passe-bas exponentiel
        // Montée rapide (son soudain) / descente lente (ambiance)
        if (rawRms > g_soundFiltered) {
            // Attaque rapide
            g_soundFiltered = g_soundFiltered * (1.0f - SOUND_SMOOTH_TAU * 3.0f)
                            + rawRms * (SOUND_SMOOTH_TAU * 3.0f);
        } else {
            // Déclin lent
            g_soundFiltered = g_soundFiltered * (1.0f - SOUND_SMOOTH_TAU)
                            + rawRms * SOUND_SMOOTH_TAU;
        }

        g_rawData.soundRms   = g_soundFiltered;
        g_normalized.sound   = g_soundFiltered;
    }
}

/**
 * Retourne les capteurs normalisés pour l'algorithme de courbes.
 */
const NormalizedSensors& sensors_get_normalized() {
    return g_normalized;
}

/**
 * Retourne les données brutes pour l'affichage LCD.
 */
const RawSensorData& sensors_get_raw() {
    return g_rawData;
}

/**
 * Affiche un résumé debug sur le port série.
 */
void sensors_print_debug(Stream &serial) {
    const RawSensorData &r = g_rawData;
    const NormalizedSensors &n = g_normalized;

    serial.print(F("T="));   serial.print(r.temperature, 1); serial.print(F("°C "));
    serial.print(F("H="));   serial.print(r.humidity, 0);    serial.print(F("% "));
    serial.print(F("D="));   serial.print(r.distanceCm, 0);  serial.print(F("cm "));
    serial.print(F("S="));   serial.print(r.soundRms, 2);    serial.println();
    serial.print(F("Norm: T="));  serial.print(n.temperature, 2);
    serial.print(F(" H="));      serial.print(n.humidity, 2);
    serial.print(F(" D="));      serial.print(n.distance, 2);
    serial.print(F(" S="));      serial.println(n.sound, 2);
}

#endif // SENSORS_H
