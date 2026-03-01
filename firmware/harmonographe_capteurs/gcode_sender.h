/**
 * gcode_sender.h — Envoi de G-code vers le contrôleur GRBL
 *
 * Gère la communication série avec le firmware GRBL de la Shapeoko.
 * Implémente le protocole de contrôle de flux GRBL (attente du "ok").
 *
 * Protocole GRBL :
 *   → Envoyer une ligne G-code terminée par '\n'
 *   ← Attendre "ok\n" (succès) ou "error:N\n" (erreur)
 *   → Envoyer la ligne suivante
 *
 * Buffer GRBL : 127 caractères. On envoie ligne par ligne.
 */

#ifndef GCODE_SENDER_H
#define GCODE_SENDER_H

#include "curves.h"  // Pour CENTER_X_MM, CENTER_Y_MM

// ─── Configuration ────────────────────────────────────────────────────────
#define GRBL_SERIAL         Serial1         // Port série vers GRBL
#define GRBL_BAUD           115200          // Baud rate GRBL standard
#define GRBL_TIMEOUT_MS     5000            // Timeout attente "ok" (ms)
#define GRBL_RETRY_MAX      3               // Nombre max de relances

// ─── Paramètres machine ───────────────────────────────────────────────────
#define WORK_OFFSET_X       200.0f          // Offset X (homing → centre bac)
#define WORK_OFFSET_Y       200.0f          // Offset Y

// ─── Structure état GRBL ─────────────────────────────────────────────────
struct GrblState {
    bool connected;         // GRBL répond
    bool homed;             // Homing effectué
    bool isMoving;          // Mouvement en cours
    float currentX;         // Position courante X (mm, depuis centre)
    float currentY;         // Position courante Y (mm, depuis centre)
    int errorCount;         // Compteur d'erreurs
};

static GrblState g_grblState = {false, false, false, 0.0f, 0.0f, 0};
static char      g_grblLineBuffer[64];   // Buffer ligne G-code

// ─── Prototypes ──────────────────────────────────────────────────────────
bool grbl_init();
bool grbl_home();
bool grbl_send_line(const char *line);
bool grbl_wait_ok(unsigned long timeoutMs = GRBL_TIMEOUT_MS);
bool grbl_move_to(float x, float y, float feedrate);
bool grbl_set_feedrate(float feedrate);
bool grbl_go_to_center(float feedrate = 500.0f);
bool grbl_send_circle_test(float radius, float feedrate = 300.0f);
void grbl_emergency_stop();
bool grbl_is_ready();
const GrblState& grbl_get_state();
void grbl_flush_incoming();

// ─── Implémentation ──────────────────────────────────────────────────────

/**
 * Vide le buffer série entrant (ignore les données parasites).
 */
void grbl_flush_incoming() {
    while (GRBL_SERIAL.available()) {
        GRBL_SERIAL.read();
    }
}

/**
 * Attend la réponse "ok" de GRBL.
 * Retourne true si "ok" reçu, false si timeout ou erreur.
 */
bool grbl_wait_ok(unsigned long timeoutMs) {
    unsigned long start = millis();
    char resp[32];
    int idx = 0;

    while ((millis() - start) < timeoutMs) {
        if (GRBL_SERIAL.available()) {
            char c = GRBL_SERIAL.read();
            if (c == '\n' || idx >= 30) {
                resp[idx] = '\0';

                if (strncmp(resp, "ok", 2) == 0) {
                    return true;
                }
                if (strncmp(resp, "error", 5) == 0) {
                    // Afficher l'erreur sur debug
                    Serial.print(F("GRBL error: "));
                    Serial.println(resp);
                    g_grblState.errorCount++;
                    return false;
                }
                // Ignorer les autres lignes (ex: "Grbl 1.1h...")
                idx = 0;
            } else if (c != '\r') {
                resp[idx++] = c;
            }
        }
        yield();  // Évite le watchdog sur certaines cartes
    }

    Serial.println(F("GRBL timeout!"));
    return false;
}

/**
 * Envoie une ligne G-code et attend la confirmation.
 */
bool grbl_send_line(const char *line) {
    // Vider d'abord les données parasites
    // (uniquement si rien n'est attendu)

    GRBL_SERIAL.print(line);
    GRBL_SERIAL.print('\n');

    return grbl_wait_ok();
}

/**
 * Initialise la connexion GRBL et vérifie la communication.
 * Appelée aussi après un arrêt d'urgence pour reprendre le dessin.
 */
bool grbl_init() {
    GRBL_SERIAL.begin(GRBL_BAUD);
    delay(500);  // GRBL prend ~500ms pour démarrer

    grbl_flush_incoming();

    // Débloquer l'alarme si GRBL est en état d'alarme (ex: après Ctrl+X)
    // Sans danger si GRBL n'est pas en alarme (réponse ignorée)
    GRBL_SERIAL.print("$X\n");
    delay(200);
    grbl_flush_incoming();

    // Envoyer '\n' pour vider le buffer GRBL et obtenir une réponse propre
    GRBL_SERIAL.print('\n');
    delay(100);
    grbl_flush_incoming();

    // Envoyer '$' (demande de statut) — GRBL doit répondre "ok"
    GRBL_SERIAL.print("$\n");
    if (!grbl_wait_ok(3000)) {
        // Deuxième tentative
        delay(1000);
        GRBL_SERIAL.print("\n$\n");
        if (!grbl_wait_ok(3000)) {
            Serial.println(F("GRBL: Pas de réponse!"));
            g_grblState.connected = false;
            return false;
        }
    }

    g_grblState.connected = true;

    // Configurer le mode de travail
    // G90 = mode absolu, G21 = millimètres, G17 = plan XY
    grbl_send_line("G90 G21 G17");

    // Désactiver la broche (pin spindle) qui n'est pas utilisée
    grbl_send_line("M5");

    Serial.println(F("GRBL: Connecté OK"));
    return true;
}

/**
 * Lance la procédure de homing (recherche des origines).
 * Nécessite que les fins de course soient câblés sur la Shapeoko.
 */
bool grbl_home() {
    Serial.println(F("GRBL: Homing..."));

    GRBL_SERIAL.print("$H\n");

    // Le homing peut prendre jusqu'à 30 secondes
    if (!grbl_wait_ok(30000)) {
        Serial.println(F("GRBL: Homing échoué!"));
        return false;
    }

    g_grblState.homed = true;
    g_grblState.currentX = 0.0f;
    g_grblState.currentY = 0.0f;

    // Définir le zéro de travail au centre du bac
    // G10 L20 = définir le système de coordonnées de travail
    snprintf(g_grblLineBuffer, sizeof(g_grblLineBuffer),
             "G10 L20 P1 X%.3f Y%.3f",
             -WORK_OFFSET_X, -WORK_OFFSET_Y);
    grbl_send_line(g_grblLineBuffer);

    // Se repositionner au centre (x=0, y=0 dans le repère de travail)
    grbl_go_to_center(1000.0f);

    Serial.println(F("GRBL: Homing OK, position centre"));
    return true;
}

/**
 * Déplace la machine à la position (x, y) en mm depuis le centre du bac.
 * x ∈ [-MACHINE_RADIUS, +MACHINE_RADIUS]
 * y ∈ [-MACHINE_RADIUS, +MACHINE_RADIUS]
 *
 * Note : x et y sont des coordonnées de travail GRBL (G54).
 * Le système de coordonnées est configuré par grbl_home() via G10 L20
 * pour que (0,0) corresponde au centre du bac. Ne pas ajouter WORK_OFFSET
 * ici — ce serait un double décalage.
 */
bool grbl_move_to(float x, float y, float feedrate) {
    // Sécurité : limiter aux limites du bac en coordonnées centrées
    // Machine [5, 395] - WORK_OFFSET 200 = [-195, +195]
    x = constrain(x, -195.0f, 195.0f);
    y = constrain(y, -195.0f, 195.0f);

    // Formater la ligne G-code en coordonnées de travail
    // Format : "G1 X123.456 Y-89.123 F1234\n"
    snprintf(g_grblLineBuffer, sizeof(g_grblLineBuffer),
             "G1 X%.3f Y%.3f F%.0f",
             x, y, feedrate);

    bool ok = grbl_send_line(g_grblLineBuffer);

    if (ok) {
        g_grblState.currentX = x;
        g_grblState.currentY = y;
    }

    return ok;
}

/**
 * Change la vitesse d'avance sans mouvement.
 */
bool grbl_set_feedrate(float feedrate) {
    snprintf(g_grblLineBuffer, sizeof(g_grblLineBuffer),
             "F%.0f", feedrate);
    return grbl_send_line(g_grblLineBuffer);
}

/**
 * Envoie la machine au centre du bac à sable (position 0,0).
 */
bool grbl_go_to_center(float feedrate) {
    return grbl_move_to(0.0f, 0.0f, feedrate);
}

/**
 * Test : trace un cercle de rayon donné autour du centre.
 * Utile pour valider la calibration de la distance aimant-plateau.
 */
bool grbl_send_circle_test(float radius, float feedrate) {
    // Aller au point de départ (droite du cercle)
    grbl_move_to(radius, 0.0f, feedrate * 2.0f);
    delay(500);

    // Tracer le cercle avec des arcs G2 (sens horaire)
    // Point final = point de départ = (radius, 0) en coordonnées de travail
    // I/J = vecteur du point courant vers le centre = (-radius, 0)
    snprintf(g_grblLineBuffer, sizeof(g_grblLineBuffer),
             "G2 X%.3f Y%.3f I%.3f J%.3f F%.0f",
             radius,   // Point final X (coords de travail, même que départ)
             0.0f,     // Point final Y
             -radius,  // I : décalage X vers le centre
             0.0f,     // J : décalage Y vers le centre
             feedrate);
    return grbl_send_line(g_grblLineBuffer);
}

/**
 * Arrêt d'urgence immédiat (envoie Ctrl+X au GRBL).
 * Le GRBL s'arrête immédiatement et passe en mode alarme.
 */
void grbl_emergency_stop() {
    GRBL_SERIAL.write(0x18);  // Ctrl+X = soft reset GRBL
    delay(100);
    grbl_flush_incoming();
    g_grblState.isMoving = false;
    Serial.println(F("GRBL: ARRÊT D'URGENCE!"));
}

/**
 * Vérifie si GRBL est prêt à recevoir de nouvelles commandes.
 */
bool grbl_is_ready() {
    return g_grblState.connected;
}

/**
 * Retourne l'état courant du GRBL.
 */
const GrblState& grbl_get_state() {
    return g_grblState;
}

#endif // GCODE_SENDER_H
