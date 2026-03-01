# CLAUDE.md — Guide pour assistants IA

## Vue d'ensemble

Projet Arduino : machine artistique interactive de type Sisyphus (bille sur sable guidée par aimant). Une Shapeoko CNC déplace un aimant sous un bac à sable ; les trajectoires sont calculées par un algorithme d'harmonographe à 3 pendules virtuels modulé par des capteurs ambiants.

**Matériel cible :** Arduino Mega 2560 (×2), firmware GRBL v1.1
**Langage :** C++ Arduino (`.ino` + `.h`)
**Aucun build system** — compilation via Arduino IDE uniquement

---

## Structure du dépôt

```
Harmonographe-/
├── firmware/harmonographe_capteurs/
│   ├── harmonographe_capteurs.ino   ← Point d'entrée (setup/loop)
│   ├── curves.h                     ← Algorithme harmonographe (maths)
│   ├── sensors.h                    ← Lecture et normalisation des capteurs
│   └── gcode_sender.h               ← Protocole GRBL (envoi G-code)
├── docs/                            ← Documentation technique (Markdown)
├── geogebra/                        ← Simulations et visualisations
└── schematics/                      ← Schémas de câblage ASCII
```

---

## Architecture du firmware

### Flux de données

```
sensors.h          curves.h                gcode_sender.h
─────────        ───────────────         ──────────────────
DHT22     →      curves_from_sensors()  →  grbl_move_to()
HC-SR04   →  →  curves_advance()            ↓
MAX9814   →      curves_get_point()      Serial1 (GRBL)
               (x, y en mm, centré)          ↓
                                         Shapeoko CNC
```

### Système de coordonnées — CRITIQUE

**Le centre du bac à sable = (0, 0) dans le repère de travail GRBL (G54).**

- Après homing (`grbl_home()`), la commande `G10 L20 P1 X-200 Y-200` configure l'offset de coordonnées de travail : machine `(200, 200)` = travail `(0, 0)`.
- `grbl_move_to(x, y, f)` envoie `x` et `y` **directement** comme coordonnées de travail GRBL. **Ne pas ajouter `WORK_OFFSET` ici** — ce serait un double décalage.
- Plage valide : `x, y ∈ [-195, +195]` mm (limites de sécurité internes).
- Sans homing : `G92 X0 Y0` fixe la position courante comme origine — la machine doit être positionnée manuellement au centre.

### Modules (`curves.h`)

- `curves_init(state)` — initialise l'état avec une figure de Lissajous φ:φ² par défaut.
- `curves_from_sensors(sensors)` — mappe les 4 capteurs normalisés `[0, 1]` vers les paramètres de courbe (fréquences, phases, amplitudes).
- `curves_get_point(state, x, y)` — calcule `(x, y)` en mm depuis le centre. Inclut un clamp circulaire à 97% de `MACHINE_RADIUS_MM`.
- `curves_advance(state, sensors)` — avance le temps et gère l'interpolation douce entre jeux de paramètres.

### Modules (`sensors.h`)

Lectures non-bloquantes avec intervalles configurés :
- DHT22 : toutes les 2500ms (contrainte hardware)
- HC-SR04 : toutes les 80ms
- MAX9814 (microphone) : toutes les 50ms (64 échantillons RMS)

Toutes les valeurs sont normalisées `[0.0, 1.0]` avant d'être transmises à `curves.h`.

### Modules (`gcode_sender.h`)

Protocole GRBL : envoyer une ligne G-code + attendre `"ok\n"` (ou `"error:N"`). Timeout 5s par défaut.

Fonctions clés :
- `grbl_init()` — initialise Serial1, débloque l'alarme (`$X`), configure `G90 G21 G17`.
- `grbl_home()` — lance `$H`, configure le repère de travail, va au centre.
- `grbl_move_to(x, y, f)` — déplacement en coordonnées de travail centrées.
- `grbl_emergency_stop()` — envoie Ctrl+X (0x18), met GRBL en alarme.

---

## Conventions de code

- **Noms de fonctions :** `module_action()` snake_case (ex: `curves_get_point`, `grbl_move_to`).
- **Variables globales :** préfixe `g_` (ex: `g_curveState`, `g_grblState`).
- **Constantes :** `#define` en MAJUSCULES (ex: `MACHINE_RADIUS_MM`, `DT_DEFAULT`).
- **Structures :** PascalCase (ex: `CurveParams`, `NormalizedSensors`).
- **Commentaires :** en français, alignés avec `//`.
- **Pas de classes C++** — style C avec structs, fonctions libres.
- **Pas d'allocation dynamique** — tout statique ou sur la pile.

### Defines — attention aux doublons

`BTN_MODE_PIN (7)` et `POT_SPEED_PIN (A1)` sont définis dans `sensors.h`. Ne pas les redéfinir dans le `.ino`.

---

## Paramètres importants

| Constante | Fichier | Valeur | Description |
|---|---|---|---|
| `MACHINE_RADIUS_MM` | curves.h | 185.0 | Rayon de travail (mm) |
| `WORK_OFFSET_X/Y` | gcode_sender.h | 200.0 | Centre bac en coords machine |
| `DT_DEFAULT` | curves.h | 0.06 | Pas de temps harmonographe (s) |
| `POINTS_PER_CYCLE` | .ino | 8000 | Points avant retour au centre |
| `INTERPOLATION_STEPS` | curves.h | 150 | Points de transition douce |
| `GRBL_TIMEOUT_MS` | gcode_sender.h | 5000 | Timeout attente "ok" (ms) |

---

## Pièges courants

1. **Double offset de coordonnées** : `grbl_move_to` n'ajoute PAS `WORK_OFFSET` — le repère de travail GRBL est déjà configuré. Ajouter l'offset ici décalerait toutes les positions de 200mm hors du bac.

2. **Reprise après pause** : `grbl_emergency_stop()` met GRBL en état d'alarme. `grbl_init()` envoie `$X` pour déverouiller avant de reconfigurer — ne pas appeler autre chose directement.

3. **Coordonnées du cercle de test** : `grbl_send_circle_test(radius)` utilise des coordonnées de travail `(radius, 0)` comme point de départ et de fin du cercle, pas des coordonnées machine absolues.

4. **DHT22** : délai minimum de 2s entre lectures. `sensors_update()` gère cela automatiquement.

5. **Buffer G-code** : `g_grblLineBuffer[64]` — vérifier que les commandes formatées ne dépassent pas 63 caractères.

---

## Workflow de développement

Il n'y a pas de tests automatisés ni de Makefile. Le projet se compile et se flashe entièrement via **Arduino IDE**.

### Modifier l'algorithme de courbes
Éditer `curves.h` — uniquement. Pas d'impact sur le matériel.

### Modifier les capteurs
Éditer `sensors.h` — vérifier les intervalles de lecture et les formules de normalisation.

### Modifier la communication GRBL
Éditer `gcode_sender.h` — tester avec un terminal série avant de flasher.

### Ajouter un pattern de dessin
Dans `curves.h`, le champ `CurveState.pattern` (actuellement non utilisé en logique de sélection) est prévu pour étendre les types de motifs.

---

## Bibliothèques Arduino requises

À installer via le gestionnaire de bibliothèques Arduino IDE :

| Bibliothèque | Auteur | Usage |
|---|---|---|
| DHT sensor library | Adafruit | Capteur DHT22 |
| Adafruit Unified Sensor | Adafruit | Dépendance DHT |
| NewPing | Tim Eckel | Capteur HC-SR04 |
| LiquidCrystal I2C | Frank de Brabander | Écran LCD 20×4 |
| FastLED | FastLED | LEDs WS2812B (optionnel, `USE_FASTLED 1`) |
