/**
 * curves.h — Algorithme Harmonographe à 3 Pendules Virtuels
 *
 * Génère des courbes organiques continues en superposant 3 oscillateurs
 * sinusoïdaux dont les paramètres sont modulés par les capteurs ambiants.
 *
 * Théorie : https://en.wikipedia.org/wiki/Harmonograph
 */

#ifndef CURVES_H
#define CURVES_H

#include <math.h>

// ─── Dimensions de la machine ─────────────────────────────────────────────
#define MACHINE_RADIUS_MM   185.0f   // Rayon de travail (bac 400×400, marge 15mm)
#define CENTER_X_MM         200.0f   // Centre X (homing + offset)
#define CENTER_Y_MM         200.0f   // Centre Y

// ─── Paramètres temporels ─────────────────────────────────────────────────
#define DT_DEFAULT          0.06f    // Pas de temps en secondes (≈17 pts/s)
#define OMEGA_BASE          (TWO_PI / 10.0f)  // Fréquence de base (période 10s)

// ─── Constantes mathématiques ────────────────────────────────────────────
#define PHI         1.6180339887f    // Nombre d'or φ
#define SQRT2       1.4142135623f    // √2
#define SQRT3       1.7320508075f    // √3
#define TWO_PI      6.2831853071f

// ─── Types de patterns de dessin ─────────────────────────────────────────
#define PATTERN_HARMONOGRAPHE  0    // Harmonographe 3 pendules (défaut)
#define PATTERN_ROSE           1    // Rose de Rhodonée (activée par le son)
#define PATTERN_SPIRALE        2    // Spirale d'Archimède (sélection manuelle)
#define PATTERN_COUNT          3    // Nombre total de patterns

// ─── Paramètres spirale d'Archimède ──────────────────────────────────────
// 4 tours complets en 480 unités de temps mathématique
// (= POINTS_PER_CYCLE × DT_DEFAULT = 8000 × 0.06)
#define SPIRAL_TURNS       4.0f
#define SPIRAL_OMEGA_RAD   (SPIRAL_TURNS * TWO_PI / 480.0f)

// ─── Seuil de transition douce ───────────────────────────────────────────
#define PARAM_CHANGE_THRESHOLD  0.08f  // Si changement > 8%, interpoler
#define INTERPOLATION_STEPS     150    // Points de transition

/**
 * Paramètres complets d'une courbe harmonographe.
 * Ces valeurs définissent entièrement la trajectoire à l'instant t.
 */
struct CurveParams {
    // Fréquences des 3 pendules virtuels (en multiples de OMEGA_BASE)
    float omega1;       // Fréquence pendule 1 (axe X principal)
    float omega2;       // Fréquence pendule 2 (axe X secondaire)
    float omega3;       // Fréquence pendule 3 (axe Y)

    // Phases initiales (rad)
    float phi1;         // Phase pendule 1
    float phi2;         // Phase pendule 2
    float phi3;         // Phase pendule 3

    // Amplitudes (normalisées, somme ≤ 1)
    float amp1;         // Amplitude pendule 1 (0.3 - 0.9)
    float amp2;         // Amplitude pendule 2 = 1 - amp1
    float amp3;         // Amplitude pendule Y (0.7 - 1.0)

    // Perturbation organique liée au son
    float soundPerturb; // 0.0 (silence) → 0.25 (bruit fort)
    float perturbFreq1; // Fréquence perturbation X (irrationnelle)
    float perturbFreq2; // Fréquence perturbation Y

    // Vitesse de déplacement (mm/min)
    float feedrate;
};

/**
 * Données brutes normalisées des capteurs (0.0 - 1.0).
 */
struct NormalizedSensors {
    float temperature;  // 0=15°C, 1=35°C
    float humidity;     // 0=30%, 1=90%
    float distance;     // 0=5cm, 1=50cm
    float sound;        // 0=silence, 1=son fort
};

/**
 * État interne du générateur de courbes.
 */
struct CurveState {
    float t;                    // Temps courant (s)
    float dt;                   // Pas de temps
    CurveParams current;        // Paramètres actifs
    CurveParams target;         // Paramètres cibles (depuis capteurs)
    int interpolStep;           // Compteur d'interpolation
    bool isInterpolating;       // En cours de transition
    uint8_t pattern;            // Pattern actuel (0=harmonographe, 1=rose, 2=spirale)
    float patternPhase;         // Phase interne du pattern courant
};

// ─── Prototypes ──────────────────────────────────────────────────────────
void curves_init(CurveState &state);
CurveParams curves_from_sensors(const NormalizedSensors &sensors);
void curves_get_point(const CurveState &state, float &x, float &y);
void curves_advance(CurveState &state, const NormalizedSensors &sensors);
bool curves_needs_transition(const CurveParams &a, const CurveParams &b);
CurveParams curves_interpolate(const CurveParams &a, const CurveParams &b, float alpha);
uint8_t curves_auto_select_pattern(const NormalizedSensors &sensors);
float smoothstep(float x);
float smoothstep3(float x);

// ─── Sélection des fréquences belles ─────────────────────────────────────

/**
 * Table des fréquences harmoniques pour le pendule 1.
 * Choisies pour créer des figures de Lissajous organiques.
 *
 * Ratios "détuned" : légèrement écartés des ratios rationnels purs
 * pour que la figure tourne lentement plutôt que de se fermer.
 */
static const float FREQ_TABLE_1[] = {
    1.000f,              // 1:1 → cercle/ellipse
    1.002f,              // 1:1 légèrement détuné → ellipse qui tourne
    PHI,                 // φ:1 → spirale dorée
    1.502f,              // 3:2 détuné → lemniscate qui tourne
    SQRT2,               // √2 → courbe dense quasi-aléatoire
    2.004f,              // 2:1 détuné → parabole oscillante
    PHI * PHI,           // φ² → double spirale dorée
    SQRT3,               // √3 → très organique
};

/**
 * Table des fréquences pour le pendule 2 (axe X secondaire).
 */
static const float FREQ_TABLE_2[] = {
    1.5f,                // Complémentaire de base
    PHI,                 // Spirale dorée
    2.0f,                // Double
    SQRT2 + 0.003f,      // √2 détuné
    2.503f,              // 5:2 détuné
    PHI * PHI,           // φ²
    3.001f,              // 3:1 détuné → trèfle tournant
    SQRT3 + 0.005f,      // √3 détuné
};

/**
 * Table des fréquences pour le pendule Y (pendule 3).
 */
static const float FREQ_TABLE_Y[] = {
    0.999f,              // Quasi-synchrone → ellipse presque verticale
    1.333f,              // 4:3 → courbe de Lissajous classique
    PHI - 0.001f,        // φ légèrement en dessous
    SQRT2 - 0.002f,      // √2 légèrement en dessous
    2.0f - 0.003f,       // Quasi-2:1
    2.5f + 0.001f,       // Quasi-5:2
    SQRT3 - 0.001f,      // √3 légèrement en dessous
    1.618f + 0.004f,     // φ légèrement au dessus
};

#define FREQ_TABLE_SIZE 8

// ─── Implémentation ──────────────────────────────────────────────────────

/**
 * Initialise l'état du générateur avec des paramètres par défaut.
 */
void curves_init(CurveState &state) {
    state.t = 0.0f;
    state.dt = DT_DEFAULT;
    state.interpolStep = 0;
    state.isInterpolating = false;
    state.pattern = 0;
    state.patternPhase = 0.0f;

    // Paramètres initiaux : figure de Lissajous φ:φ² (spirale dorée)
    state.current.omega1 = PHI * OMEGA_BASE;
    state.current.omega2 = PHI * PHI * OMEGA_BASE;
    state.current.omega3 = 1.333f * OMEGA_BASE;
    state.current.phi1 = 0.0f;
    state.current.phi2 = TWO_PI / 4.0f;  // 90°
    state.current.phi3 = TWO_PI / 6.0f;  // 60°
    state.current.amp1 = 0.65f;
    state.current.amp2 = 0.35f;
    state.current.amp3 = 0.95f;
    state.current.soundPerturb = 0.0f;
    state.current.perturbFreq1 = 7.391f * OMEGA_BASE;  // Irrationnel
    state.current.perturbFreq2 = 5.742f * OMEGA_BASE;  // Irrationnel
    state.current.feedrate = 800.0f;

    state.target = state.current;
}

/**
 * Fonction smoothstep cubique (accélération/décélération douce).
 * x ∈ [0, 1] → résultat ∈ [0, 1]
 */
float smoothstep(float x) {
    if (x <= 0.0f) return 0.0f;
    if (x >= 1.0f) return 1.0f;
    return x * x * (3.0f - 2.0f * x);
}

/**
 * Smoothstep ordre 5 (encore plus doux, dérivée nulle aux extrémités).
 */
float smoothstep3(float x) {
    if (x <= 0.0f) return 0.0f;
    if (x >= 1.0f) return 1.0f;
    return x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f);
}

/**
 * Convertit les capteurs normalisés en paramètres de courbe.
 *
 * C'est le cœur artistique de la machine :
 * chaque capteur influence une dimension distincte de la courbe.
 */
CurveParams curves_from_sensors(const NormalizedSensors &sensors) {
    CurveParams p;

    // ── Fréquence pendule 1 : pilotée par la TEMPÉRATURE ──────────────
    // Température froide → ratios simples (Lissajous géométrique)
    // Température chaude → ratios irrationnels (organique, dense)
    float tempIdx = sensors.temperature * (FREQ_TABLE_SIZE - 1);
    int t0 = (int)tempIdx;
    int t1 = min(t0 + 1, FREQ_TABLE_SIZE - 1);
    float tFrac = tempIdx - t0;
    p.omega1 = (FREQ_TABLE_1[t0] * (1.0f - tFrac) + FREQ_TABLE_1[t1] * tFrac) * OMEGA_BASE;

    // ── Fréquence pendule 2 : pilotée par l'HUMIDITÉ ──────────────────
    // Humidité faible → figure compacte, fermée
    // Humidité forte → figure ouverte, remplissage dense
    float humIdx = sensors.humidity * (FREQ_TABLE_SIZE - 1);
    int h0 = (int)humIdx;
    int h1 = min(h0 + 1, FREQ_TABLE_SIZE - 1);
    float hFrac = humIdx - h0;
    p.omega2 = (FREQ_TABLE_2[h0] * (1.0f - hFrac) + FREQ_TABLE_2[h1] * hFrac) * OMEGA_BASE;

    // ── Fréquence pendule Y : couplage temp+humidité ───────────────────
    float yIdx = ((sensors.temperature + sensors.humidity) / 2.0f) * (FREQ_TABLE_SIZE - 1);
    int y0 = (int)yIdx;
    int y1 = min(y0 + 1, FREQ_TABLE_SIZE - 1);
    float yFrac = yIdx - y0;
    p.omega3 = (FREQ_TABLE_Y[y0] * (1.0f - yFrac) + FREQ_TABLE_Y[y1] * yFrac) * OMEGA_BASE;

    // ── Phases : pilotées par la DISTANCE ─────────────────────────────
    // Quelqu'un s'approche → la figure pivote et change d'orientation
    // Distance max (loin) → phase 0 (orientation neutre)
    // Distance min (proche) → phase 2π/3 (rotation 120°)
    float distNorm = 1.0f - sensors.distance;  // Inverse : proche = valeur forte
    p.phi1 = distNorm * TWO_PI * 0.333f;         // 0 → 120°
    p.phi2 = distNorm * TWO_PI * 0.5f + 0.78f;  // Déphasage fixe + variable
    p.phi3 = distNorm * TWO_PI * 0.167f;         // 0 → 60°

    // ── Amplitudes : pilotées par le SON ──────────────────────────────
    // Silence → courbe symétrique (amp1 ≈ amp2)
    // Son fort → courbe asymétrique, plus dynamique
    float soundNorm = sensors.sound;
    p.amp1 = 0.55f + soundNorm * 0.30f;   // 0.55 → 0.85
    p.amp2 = 1.0f - p.amp1;               // Complémentaire
    // Assurer amp2 ≥ 0.10 pour garder deux composantes
    if (p.amp2 < 0.10f) {
        p.amp2 = 0.10f;
        p.amp1 = 0.90f;
    }
    p.amp3 = 0.80f + soundNorm * 0.18f;  // 0.80 → 0.98

    // ── Perturbation organique (son) ──────────────────────────────────
    // Quand quelqu'un parle ou fait du bruit, la courbe se déforme
    // légèrement de façon organique, comme une respiration
    p.soundPerturb = soundNorm * 0.20f;   // Max 20% de perturbation
    p.perturbFreq1 = 7.391f * OMEGA_BASE; // Fréquences irrationnelles fixes
    p.perturbFreq2 = 5.742f * OMEGA_BASE; // → perturbation jamais périodique

    // ── Vitesse de déplacement ────────────────────────────────────────
    // Modulée par la distance (quelqu'un regarde → plus lent = plus visible)
    // et par le son (ambiance = vitesse)
    float baseSpeed = 600.0f + sensors.temperature * 800.0f;  // 600→1400 mm/min
    float distFactor = 0.5f + sensors.distance * 0.8f;        // 0.5→1.3×
    p.feedrate = constrain(baseSpeed * distFactor, 200.0f, 2500.0f);

    return p;
}

// ─── Fonctions internes de calcul de points ───────────────────────────────

/**
 * Applique le clamp circulaire de sécurité à (x, y).
 * Ne dépasse pas 97% du rayon de travail.
 */
static void curves_clamp_circle(float &x, float &y) {
    float r = sqrtf(x * x + y * y);
    if (r > MACHINE_RADIUS_MM * 0.97f) {
        float scale = (MACHINE_RADIUS_MM * 0.97f) / r;
        x *= scale;
        y *= scale;
    }
}

/**
 * Pattern HARMONOGRAPHE — 3 pendules virtuels superposés.
 *
 * Formule :
 *   x(t) = R × [ A₁·sin(ω₁·t + φ₁) + A₂·sin(ω₂·t + φ₂) + δx(t) ]
 *   y(t) = R × [ A₃·sin(ω₃·t + φ₃) + δy(t) ]
 *
 * Où δx, δy sont les perturbations organiques liées au son.
 */
static void curves_get_point_harmonographe(const CurveState &state, float &x, float &y) {
    const CurveParams &p = state.current;
    float t = state.t;

    float rawX = p.amp1 * sinf(p.omega1 * t + p.phi1)
               + p.amp2 * sinf(p.omega2 * t + p.phi2);
    float rawY = p.amp3 * sinf(p.omega3 * t + p.phi3);

    // Perturbation organique liée au son (fréquences irrationnelles → jamais périodique)
    if (p.soundPerturb > 0.001f) {
        float dx = p.soundPerturb * sinf(p.perturbFreq1 * t + 1.234f)
                                  * sinf(p.perturbFreq2 * t * 0.7f);
        float dy = p.soundPerturb * sinf(p.perturbFreq2 * t + 2.718f)
                                  * sinf(p.perturbFreq1 * t * 0.6f);
        rawX += dx;
        rawY += dy;
    }

    x = rawX * MACHINE_RADIUS_MM;
    y = rawY * MACHINE_RADIUS_MM;
    curves_clamp_circle(x, y);
}

/**
 * Pattern ROSE — Rose de Rhodonée (rhodonea curve).
 *
 * Équation polaire : r = cos(k · θ)
 *   k entier impair → k pétales
 *   k entier pair   → 2k pétales
 *   k irrationnel   → spirale infinie qui ne se ferme jamais
 *
 * k est calculé depuis amp1 (modulé par le son) :
 *   k ∈ [3.0, 5.0] → 3 à 5 pétales / rose ouverte
 *
 * La perturbation sonore déforme organiquement les pétales,
 * comme si la rose "respirait" au rythme du son ambiant.
 */
static void curves_get_point_rose(const CurveState &state, float &x, float &y) {
    const CurveParams &p = state.current;
    float theta = state.t * p.omega3;   // Rotation à vitesse omega3

    // Nombre de pétales (pilotés par amp1, lui-même modulé par le son)
    // amp1 ∈ [0.55, 0.90] → k ∈ [3.0, 4.75]
    float k = 3.0f + p.amp1 * 2.5f;

    float r = cosf(k * theta);          // r peut être négatif → pétales complets

    // Perturbation organique : gonfle/dégonfle les pétales avec le son
    if (p.soundPerturb > 0.001f) {
        float pulse = 1.0f + p.soundPerturb * sinf(p.perturbFreq1 * state.t + 0.5f)
                                            * sinf(p.perturbFreq2 * state.t * 0.8f + 1.1f);
        r *= pulse;
    }

    x = r * cosf(theta) * MACHINE_RADIUS_MM;
    y = r * sinf(theta) * MACHINE_RADIUS_MM;
    curves_clamp_circle(x, y);
}

/**
 * Pattern SPIRALE — Spirale d'Archimède sortante.
 *
 * La bille part du centre et s'éloigne progressivement
 * en faisant SPIRAL_TURNS tours jusqu'au bord du bac.
 * La fonction start_new_cycle() ramène au centre à la fin de chaque cycle.
 *
 * r(t) = (SPIRAL_OMEGA_RAD · t) / (SPIRAL_TURNS · 2π) × R
 * θ(t) = SPIRAL_OMEGA_RAD · t + φ₁
 *
 * La légère perturbation sonore rend chaque spire unique.
 */
static void curves_get_point_spiral(const CurveState &state, float &x, float &y) {
    const CurveParams &p = state.current;
    float theta = state.t * SPIRAL_OMEGA_RAD;   // Angle total parcouru

    // Rayon croissant de 0 (centre) à MACHINE_RADIUS_MM (bord)
    float r = (theta / (SPIRAL_TURNS * TWO_PI)) * MACHINE_RADIUS_MM * 0.95f;

    // Légère ondulation pour que chaque spire soit différente
    if (p.soundPerturb > 0.001f) {
        r *= (1.0f + p.soundPerturb * 0.4f * sinf(p.perturbFreq1 * state.t));
    }

    // Rotation de l'axe de la spirale selon les phases capteurs
    float axisAngle = p.phi1 * 0.5f;

    x = r * cosf(theta + axisAngle);
    y = r * sinf(theta + axisAngle);
    curves_clamp_circle(x, y);
}

/**
 * Calcule la position (x, y) en mm depuis le centre pour le temps t.
 * Dispatch vers le pattern actif (harmonographe, rose ou spirale).
 */
void curves_get_point(const CurveState &state, float &x, float &y) {
    switch (state.pattern) {
        case PATTERN_ROSE:
            curves_get_point_rose(state, x, y);
            break;
        case PATTERN_SPIRALE:
            curves_get_point_spiral(state, x, y);
            break;
        case PATTERN_HARMONOGRAPHE:
        default:
            curves_get_point_harmonographe(state, x, y);
            break;
    }
}

/**
 * Sélection automatique du pattern selon les capteurs.
 *
 * Logique avec hystérésis pour éviter les oscillations rapides :
 *   - Son > 0.70 → Rose (les pétales vibrent avec le son)
 *   - Son < 0.40 → Retour à l'Harmonographe
 *   - Spirale : uniquement par sélection manuelle
 *
 * Note : le pattern SPIRALE n'est jamais sélectionné automatiquement
 * pour ne pas interrompre un motif manuel en cours.
 */
uint8_t curves_auto_select_pattern(const NormalizedSensors &sensors) {
    static uint8_t lastAuto = PATTERN_HARMONOGRAPHE;

    if (lastAuto != PATTERN_ROSE && sensors.sound > 0.70f) {
        lastAuto = PATTERN_ROSE;
    } else if (lastAuto == PATTERN_ROSE && sensors.sound < 0.40f) {
        lastAuto = PATTERN_HARMONOGRAPHE;
    }
    return lastAuto;
}

/**
 * Vérifie si un changement de paramètres nécessite une interpolation douce.
 */
bool curves_needs_transition(const CurveParams &a, const CurveParams &b) {
    float dOmega = fabsf(a.omega1 - b.omega1) / a.omega1;
    float dAmp   = fabsf(a.amp1 - b.amp1);
    float dPhi   = fabsf(a.phi1 - b.phi1) / TWO_PI;
    return (dOmega > PARAM_CHANGE_THRESHOLD ||
            dAmp   > PARAM_CHANGE_THRESHOLD ||
            dPhi   > PARAM_CHANGE_THRESHOLD);
}

/**
 * Interpole linéairement entre deux jeux de paramètres.
 * alpha : 0.0 = paramètres a, 1.0 = paramètres b
 */
CurveParams curves_interpolate(const CurveParams &a, const CurveParams &b, float alpha) {
    float s = smoothstep3(alpha);  // Courbe en S lisse
    CurveParams r;
    r.omega1       = a.omega1       + s * (b.omega1       - a.omega1);
    r.omega2       = a.omega2       + s * (b.omega2       - a.omega2);
    r.omega3       = a.omega3       + s * (b.omega3       - a.omega3);
    r.phi1         = a.phi1         + s * (b.phi1         - a.phi1);
    r.phi2         = a.phi2         + s * (b.phi2         - a.phi2);
    r.phi3         = a.phi3         + s * (b.phi3         - a.phi3);
    r.amp1         = a.amp1         + s * (b.amp1         - a.amp1);
    r.amp2         = a.amp2         + s * (b.amp2         - a.amp2);
    r.amp3         = a.amp3         + s * (b.amp3         - a.amp3);
    r.soundPerturb = a.soundPerturb + s * (b.soundPerturb - a.soundPerturb);
    r.perturbFreq1 = b.perturbFreq1;  // Fréquences de perturbation : pas d'interpolation
    r.perturbFreq2 = b.perturbFreq2;
    r.feedrate     = a.feedrate     + s * (b.feedrate     - a.feedrate);
    return r;
}

/**
 * Met à jour l'état et fait avancer le temps.
 * Gère l'interpolation douce si les paramètres changent.
 */
void curves_advance(CurveState &state, const NormalizedSensors &sensors) {
    // Calculer les paramètres cibles depuis les capteurs
    CurveParams newTarget = curves_from_sensors(sensors);

    // Vérifier si une transition est nécessaire
    if (!state.isInterpolating && curves_needs_transition(state.current, newTarget)) {
        state.target = newTarget;
        state.isInterpolating = true;
        state.interpolStep = 0;
    }

    // Mise à jour des paramètres (interpolation ou direct)
    if (state.isInterpolating) {
        state.interpolStep++;
        float alpha = (float)state.interpolStep / (float)INTERPOLATION_STEPS;
        state.current = curves_interpolate(state.current, state.target, alpha);
        if (state.interpolStep >= INTERPOLATION_STEPS) {
            state.current = state.target;
            state.isInterpolating = false;
        }
    } else {
        // Mise à jour douce directe (pas de saut brutal)
        // Filtre passe-bas du 1er ordre sur les paramètres lents
        float tau = 0.02f;  // Constante de temps = 2%
        state.current.soundPerturb =
            state.current.soundPerturb * (1.0f - tau) + newTarget.soundPerturb * tau;
        state.current.feedrate =
            state.current.feedrate * (1.0f - tau) + newTarget.feedrate * tau;
    }

    // Avancer le temps
    state.t += state.dt;
}

/**
 * Génère une spirale de retour vers le centre.
 * Utilisée quand la bille s'éloigne trop du centre entre les patterns.
 *
 * Paramètres de sortie : tableau de (x, y) positions en mm
 * Retourne le nombre de points générés.
 */
int curves_return_to_center(float fromX, float fromY, float pts[][2], int maxPts) {
    int n = 0;
    int steps = min(maxPts, 60);
    for (int i = 0; i <= steps && n < maxPts; i++) {
        float alpha = smoothstep3((float)i / steps);
        pts[n][0] = fromX * (1.0f - alpha);
        pts[n][1] = fromY * (1.0f - alpha);
        n++;
    }
    return n;
}

#endif // CURVES_H
