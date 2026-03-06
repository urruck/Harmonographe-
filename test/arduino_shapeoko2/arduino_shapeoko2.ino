/**
 * ╔══════════════════════════════════════════════════════════════════════╗
 * ║         HARMONOGRAPHE SHAPEOKO 2 — MODE TEST SANS CAPTEURS          ║
 * ║                                                                      ║
 * ║  Test des axes X/Y de la Shapeoko 2 via interface Processing (PC).  ║
 * ║  Les capteurs sont simulés par des sliders dans l'interface.         ║
 * ║                                                                      ║
 * ║  Carte : Arduino Mega 2560 + Shield GRBL Shapeoko 2                 ║
 * ╚══════════════════════════════════════════════════════════════════════╝
 *
 * Câblage minimal requis :
 *   - Arduino Mega USB → PC  (Serial0 : debug + Processing)
 *   - Pin 18 (TX1) → Shield GRBL RX  (Serial1 : G-code)
 *   - Pin 19 (RX1) ← Shield GRBL TX
 *   - Alimentation Shapeoko 2 allumée
 *   - Aucun capteur nécessaire
 *
 * Protocole série USB (depuis Processing) :
 *   SET temp 0.50   → température simulée [0.0 - 1.0]
 *   SET hum  0.30   → humidité simulée    [0.0 - 1.0]
 *   SET dist 0.80   → distance simulée    [0.0 - 1.0]
 *   SET snd  0.10   → son simulé          [0.0 - 1.0]
 *   SET feed 800    → vitesse mm/min (0 = auto)
 *   CMD home        → homing GRBL ($H)
 *   CMD pause       → arrêt d'urgence (Ctrl+X)
 *   CMD resume      → reprise après pause
 *   CMD circle      → test cercle 50mm de diamètre
 *   CMD reset       → retour au centre
 *
 * Réponses vers Processing :
 *   STATUS x=12.3 y=-45.1 mode=RUNNING pts=1234 cycle=2
 *   LOG <message>
 */

// ─── Modules du firmware principal (copies locales, sans dépendances LCD) ─
#include "curves.h"
#include "gcode_sender.h"

// ─── États de la machine ─────────────────────────────────────────────────
enum MachineMode {
    MODE_BOOT,
    MODE_HOMING,
    MODE_RUNNING,
    MODE_PAUSED,
    MODE_ERROR
};

// ─── Variables globales ───────────────────────────────────────────────────
static MachineMode       g_mode         = MODE_BOOT;
static CurveState        g_curveState;
static NormalizedSensors g_sensors      = {0.5f, 0.5f, 0.5f, 0.0f};
static uint32_t          g_pointCount   = 0;
static uint32_t          g_cycleCount   = 0;
static bool              g_feedOverride = false;  // true si vitesse imposée par PC
static float             g_feedManual   = 800.0f;

// ─── Constantes ───────────────────────────────────────────────────────────
#define STATUS_INTERVAL_MS   500    // Envoi status vers Processing (500ms)
#define POINTS_PER_CYCLE    8000
#define RETURN_RADIUS       160.0f  // Seuil de retour centre (mm)

static uint32_t g_lastStatus = 0;

// ─── Helpers ──────────────────────────────────────────────────────────────

void log_to_pc(const char *msg) {
    Serial.print(F("LOG "));
    Serial.println(msg);
}

const char* mode_name() {
    switch (g_mode) {
        case MODE_BOOT:    return "BOOT";
        case MODE_HOMING:  return "HOMING";
        case MODE_RUNNING: return "RUNNING";
        case MODE_PAUSED:  return "PAUSED";
        case MODE_ERROR:   return "ERROR";
    }
    return "UNKNOWN";
}

void send_status(float x, float y) {
    Serial.print(F("STATUS x="));
    Serial.print(x, 1);
    Serial.print(F(" y="));
    Serial.print(y, 1);
    Serial.print(F(" mode="));
    Serial.print(mode_name());
    Serial.print(F(" pts="));
    Serial.print(g_pointCount);
    Serial.print(F(" cycle="));
    Serial.println(g_cycleCount);
}

// ─── Analyse des commandes reçues depuis Processing ───────────────────────

/**
 * Extrait un token (mot séparé par espace) à la position donnée.
 * Retourne la longueur du token.
 */
int next_token(const char *buf, int start, char *out, int outSize) {
    while (buf[start] == ' ') start++;  // skip spaces
    int i = 0;
    while (buf[start + i] && buf[start + i] != ' ' && i < outSize - 1) {
        out[i] = buf[start + i];
        i++;
    }
    out[i] = '\0';
    return start + i;
}

/**
 * Parse et exécute une commande reçue depuis le PC.
 * Format : "SET param val\n" ou "CMD action\n"
 */
void handle_command(const char *line) {
    char verb[8], param[8], val[16];
    int pos = 0;
    pos = next_token(line, pos, verb, sizeof(verb));
    pos = next_token(line, pos, param, sizeof(param));
    next_token(line, pos, val, sizeof(val));

    if (strcmp(verb, "SET") == 0) {
        float v = atof(val);
        v = constrain(v, 0.0f, 1.0f);

        if (strcmp(param, "temp") == 0) {
            g_sensors.temperature = v;
        } else if (strcmp(param, "hum") == 0) {
            g_sensors.humidity = v;
        } else if (strcmp(param, "dist") == 0) {
            g_sensors.distance = v;
        } else if (strcmp(param, "snd") == 0) {
            g_sensors.sound = v;
        } else if (strcmp(param, "feed") == 0) {
            float fv = atof(val);  // feedrate n'est pas contraint à [0,1]
            if (fv > 0.0f) {
                g_feedManual   = constrain(fv, 100.0f, 3000.0f);
                g_feedOverride = true;
            } else {
                g_feedOverride = false;
            }
        }

    } else if (strcmp(verb, "CMD") == 0) {

        if (strcmp(param, "home") == 0) {
            log_to_pc("Homing...");
            g_mode = MODE_HOMING;
            if (grbl_home()) {
                log_to_pc("Homing OK");
                g_mode = MODE_RUNNING;
            } else {
                log_to_pc("Homing echoue - position manuelle G92 X0 Y0");
                grbl_send_line("G92 X0 Y0");
                g_mode = MODE_RUNNING;
            }

        } else if (strcmp(param, "pause") == 0) {
            if (g_mode == MODE_RUNNING) {
                grbl_emergency_stop();
                g_mode = MODE_PAUSED;
                log_to_pc("PAUSE");
            }

        } else if (strcmp(param, "resume") == 0) {
            if (g_mode == MODE_PAUSED) {
                grbl_init();
                g_mode = MODE_RUNNING;
                log_to_pc("REPRISE");
            }

        } else if (strcmp(param, "circle") == 0) {
            log_to_pc("Test cercle 25mm...");
            grbl_move_to(25.0f, 0.0f, 500.0f);
            delay(500);
            grbl_send_circle_test(25.0f, 300.0f);
            grbl_go_to_center(800.0f);
            log_to_pc("Test cercle OK");

        } else if (strcmp(param, "reset") == 0) {
            log_to_pc("Retour centre...");
            grbl_go_to_center(1000.0f);
            log_to_pc("Centre atteint");
        }
    }
}

// ─── Lecture des commandes série ──────────────────────────────────────────

static char g_serialBuf[64];
static int  g_serialIdx = 0;

void read_serial_commands() {
    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\n' || c == '\r') {
            if (g_serialIdx > 0) {
                g_serialBuf[g_serialIdx] = '\0';
                handle_command(g_serialBuf);
                g_serialIdx = 0;
            }
        } else if (g_serialIdx < (int)sizeof(g_serialBuf) - 1) {
            g_serialBuf[g_serialIdx++] = c;
        }
    }
}

// ─── Gestion des cycles ───────────────────────────────────────────────────

void start_new_cycle() {
    g_cycleCount++;
    g_pointCount = 0;

    float fromX = g_grblState.currentX;
    float fromY = g_grblState.currentY;

    log_to_pc("Nouveau cycle - retour centre");

    int steps = 40;
    for (int i = 0; i <= steps; i++) {
        float alpha = smoothstep3((float)i / steps);
        float rx = fromX * (1.0f - alpha);
        float ry = fromY * (1.0f - alpha);
        grbl_move_to(rx, ry, 800.0f);
    }

    delay(300);
    g_curveState.t = 0.0f;
    g_curveState.isInterpolating = false;
}

// ─── setup() ─────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    Serial.println(F("LOG Harmonographe Shapeoko2 - Mode test"));
    Serial.println(F("LOG En attente de connexion Processing..."));

    curves_init(g_curveState);

    if (!grbl_init()) {
        Serial.println(F("LOG ERREUR: GRBL non connecte! Verifier le cable Serial1."));
        g_mode = MODE_ERROR;
        return;
    }

    log_to_pc("GRBL connecte OK");
    log_to_pc("Envoyer CMD home pour demarrer le homing");
    g_mode = MODE_RUNNING;

    send_status(0.0f, 0.0f);
}

// ─── loop() ──────────────────────────────────────────────────────────────

void loop() {
    // Lire les commandes depuis Processing
    read_serial_commands();

    // Modes bloquants
    if (g_mode == MODE_ERROR || g_mode == MODE_PAUSED || g_mode == MODE_HOMING || g_mode == MODE_BOOT) {
        if ((millis() - g_lastStatus) >= STATUS_INTERVAL_MS) {
            g_lastStatus = millis();
            send_status(g_grblState.currentX, g_grblState.currentY);
        }
        delay(20);
        return;
    }

    // Mode dessin actif
    curves_advance(g_curveState, g_sensors);

    float x, y;
    curves_get_point(g_curveState, x, y);

    float feedrate = g_feedOverride ? g_feedManual : g_curveState.current.feedrate;
    feedrate = constrain(feedrate, 150.0f, 3000.0f);

    bool ok = grbl_move_to(x, y, feedrate);

    if (ok) {
        g_pointCount++;
        g_grblState.errorCount = 0;
    } else {
        g_grblState.errorCount++;
        if (g_grblState.errorCount > 5) {
            g_mode = MODE_ERROR;
            log_to_pc("ERREUR: trop d'erreurs GRBL, arret!");
            return;
        }
    }

    if (g_pointCount >= POINTS_PER_CYCLE) {
        start_new_cycle();
    }

    // Envoi périodique du status vers Processing
    if ((millis() - g_lastStatus) >= STATUS_INTERVAL_MS) {
        g_lastStatus = millis();
        send_status(x, y);
    }
}
