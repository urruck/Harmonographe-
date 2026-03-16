# CLAUDE.md — Harmonographe de Sable

Guide pour assistants IA travaillant sur ce dépôt.

## Présentation du projet

Machine artistique de type Sisyphus : une bille d'acier est guidée sur du sable par un aimant N52 fixé sous le chariot Z d'une Shapeoko 2 CNC. L'algorithme harmonographe à 3 pendules virtuels calcule la trajectoire en temps réel, modulée par des capteurs ambiants (température, humidité, distance, son).

## Structure du dépôt

```
Harmonographe-/
├── firmware/harmonographe_capteurs/   ← Firmware principal (Arduino Mega)
│   ├── harmonographe_capteurs.ino     ← Fichier principal : setup/loop, LCD, LEDs
│   ├── curves.h                       ← Algorithme harmonographe (tout y est)
│   ├── sensors.h                      ← Lecture + normalisation des capteurs
│   └── gcode_sender.h                 ← Communication GRBL (Serial1)
│
├── test/
│   ├── arduino_shapeoko2/             ← Test hardware sans capteurs physiques
│   │   ├── arduino_shapeoko2.ino      ← Reçoit les paramètres depuis Processing
│   │   ├── curves.h                   ← Copie locale (autonome, sans LCD/FastLED)
│   │   └── gcode_sender.h             ← Copie locale
│   ├── processing_interface/
│   │   └── HarmoControl.pde           ← GUI Processing → envoie paramètres via série
│   └── test-sanscarte-mega/
│       ├── HarmoGcode/
│       │   └── HarmoGcode.pde         ← Génère un .nc G-code (aucun hardware)
│       └── harmo_sim.py               ← Simulateur Python en ligne de commande
│
├── docs/                              ← Documentation technique complète
│   ├── 01_conception_mecanique.md
│   ├── 02_electronique.md
│   ├── 03_cablage_electrique.md
│   ├── 04_algorithme_harmonographe.md
│   └── 05_bom_liste_materiaux.md
│
├── assets/README.md                   ← Specs des pièces 3D à imprimer
├── geogebra/                          ← Visualisation mathématique GeoGebra
│   ├── generer_ggb.py
│   └── saisons_harmonographe.xml/.ggb
└── schematics/                        ← Schémas de câblage textuels
```

## Architecture matérielle

```
[Capteurs]         [Arduino Mega #2]      Serial1      [Arduino Mega #1 + gShield]
DHT22        ───►  curves.h               ──────────►  GRBL firmware
HC-SR04      ───►  sensors.h                           Shapeoko 2 moteurs X/Y
MAX9814      ───►  gcode_sender.h
Encodeur     ───►  harmonographe_capteurs.ino
LCD 20×4
WS2812B LEDs
```

- **Serial0** (USB) : debug + éventuelle connexion PC
- **Serial1** (pin 18 TX / pin 19 RX) : G-code vers GRBL à 115200 baud
- **Zone de travail** : 300×300mm, centre à (150, 150) en coordonnées machine
- **Rayon de dessin** : ±120mm depuis le centre (`MACHINE_RADIUS_MM`)

## Fichiers clés et leur rôle

### `firmware/harmonographe_capteurs/curves.h`
C'est le cœur artistique. Contient :
- `CurveParams` : les 13 paramètres d'une courbe (fréquences, phases, amplitudes, feedrate)
- `CurveState` : état courant + cible + interpolation
- `curves_from_sensors()` : convertit les 4 valeurs normalisées [0-1] en paramètres
- `curves_get_point()` : calcule (x, y) en mm depuis le centre pour un temps t
- `curves_advance()` : avance le temps, gère les transitions douces (150 pas)
- Tables de fréquences `FREQ_TABLE_1/2/Y` : ratios irrationnels (φ, √2, √3) pour que les figures ne se ferment jamais

### `firmware/harmonographe_capteurs/gcode_sender.h`
- Toutes les fonctions `grbl_*` travaillent en **coordonnées de travail (WCS)** : centre = (0,0)
- **Ne jamais ajouter WORK_OFFSET manuellement** : `grbl_home()` configure le WCS via `G10 L20` pour que le centre soit work (0,0). `grbl_move_to(x, y)` envoie `G1 X{x} Y{y}` directement.
- Clamp de sécurité : `[-145, +145]` mm (pas `[5, 295]`)
- `grbl_send_circle_test(radius)` : G2 avec endpoint `(radius, 0)` et `I=-radius J=0`

### `firmware/harmonographe_capteurs/sensors.h`
- DHT22 → [0-1] : temp 12-38°C, humidité 25-92%
- HC-SR04 → [0-1] : distance 5-55cm
- MAX9814 → [0-1] : niveau RMS avec attaque rapide, décroissance lente
- Tout retourné dans `NormalizedSensors` défini dans `curves.h`

### `test/arduino_shapeoko2/`
- Copie **autonome** de `curves.h` et `gcode_sender.h` (pas de dépendance vers `firmware/`)
- Si on corrige un bug dans `firmware/.../gcode_sender.h`, il faut **aussi** corriger `test/arduino_shapeoko2/gcode_sender.h`

## Conventions de code

### Arduino / C++
- Nommage : `snake_case` pour tout (fonctions, variables, macros)
- Guards `#ifndef / #define / #endif` sur tous les `.h`
- Pas de `delay()` dans la loop principale (le sync naturel vient de `grbl_wait_ok`)
- Variables globales préfixées `g_` : `g_curveState`, `g_mode`, `g_pointCount`
- Constantes en `#define` ou `static const float`
- Commentaires en français

### G-code
- Mode absolu G90, millimètres G21, plan XY G17
- Toujours en coordonnées de travail (centre = 0,0)
- Format : `G1 X%.3f Y%.3f F%.0f`
- Ne jamais utiliser G53 (coordonnées machine absolues)

### Processing (Java)
- Sliders verticaux : valeur 0.0 en bas, 1.0 en haut
- Protocole série avec l'Arduino : lignes texte terminées `\n`
- Layout : panneau de contrôle gauche / canvas de visualisation droite

### Python
- L'algorithme harmonographe doit rester **identique** au firmware Arduino
- `OMEGA_BASE = 2π / 10.0` (période de base 10 secondes)
- Tables de fréquences identiques à `FREQ_TABLE_1/2/Y` du firmware

## Algorithme harmonographe — formule

```
x(t) = MACHINE_RADIUS × [ A1·sin(ω1·t + φ1) + A2·sin(ω2·t + φ2) + δx(t) ]
y(t) = MACHINE_RADIUS × [ A3·sin(ω3·t + φ3) + δy(t) ]
```

Mapping capteurs → paramètres :

| Capteur | Plage normalisée | Paramètre | Effet |
|---------|-----------------|-----------|-------|
| Température | 0=12°C, 1=38°C | ω1 via FREQ_TABLE_1 | Froid=géométrique, Chaud=organique dense |
| Humidité | 0=25%, 1=92% | ω2 via FREQ_TABLE_2 | Sec=compact, Humide=ouvert |
| Distance | 0=5cm, 1=55cm | φ1, φ2, φ3 | Proche=figure pivotée |
| Son | 0=silence, 1=fort | A1, A2, A3, δx, δy | Fort=asymétrique + perturbation organique |

## Workflows de développement

### Modifier l'algorithme de courbes
1. Éditer `firmware/harmonographe_capteurs/curves.h`
2. Si les mêmes fonctions existent dans `test/arduino_shapeoko2/curves.h` → **même correction**
3. Vérifier cohérence avec `harmo_sim.py` (l'algorithme Python doit rester identique)

### Modifier la communication GRBL
1. Éditer `firmware/harmonographe_capteurs/gcode_sender.h`
2. Reporter **aussi** dans `test/arduino_shapeoko2/gcode_sender.h`
3. **Règle critique** : ne jamais ajouter `WORK_OFFSET_X/Y` dans `grbl_move_to` — le WCS est géré par GRBL après `grbl_home()`

### Ajouter un nouveau test Processing
- Créer un sous-dossier dans `test/`
- Dossier = nom du sketch Processing (ex: `test/MonTest/MonTest.pde`)
- Rendre le sketch autonome : pas de `#include` relatif vers `firmware/`
- Aucune bibliothèque externe pour les sketches Processing (utiliser uniquement `processing.serial.*` si besoin)

### Ajouter un capteur
1. Ajouter la pin dans `sensors.h` et le commentaire dans `harmonographe_capteurs.ino`
2. Ajouter le champ dans `NormalizedSensors` (dans `curves.h`)
3. Mettre à jour `curves_from_sensors()` pour utiliser la nouvelle valeur
4. Mettre à jour l'affichage LCD dans `lcd_update()`

## Git

- Branche de travail active : `claude/fix-harmonograph-shapeoko-9PCFJ`
- Commits en français, message court + description si nécessaire
- Toujours `git push -u origin <branche>`

## Pièges connus

| Piège | Explication |
|-------|-------------|
| Double décalage WORK_OFFSET | `grbl_home()` configure le WCS via G10 → `grbl_move_to()` envoie les coords de travail telles quelles, sans ajouter 150mm |
| `#include` relatifs dans les sketches test | L'IDE Arduino compile tous les fichiers du dossier firmware → utiliser des copies locales dans `test/arduino_shapeoko2/` |
| `LiquidCrystal_I2C` absente | Nécessaire uniquement pour le firmware principal, pas pour les sketches de test |
| Synchronisation des copies | `curves.h` et `gcode_sender.h` existent en deux endroits (`firmware/` et `test/arduino_shapeoko2/`) — toujours garder les deux à jour |
| `grbl_send_circle_test` coords G2 | Endpoint = `(radius, 0)` en work coords, pas `(WORK_OFFSET_X + radius, WORK_OFFSET_Y)` |

## Bibliothèques Arduino requises (firmware principal uniquement)

| Bibliothèque | Version | Gestionnaire |
|---|---|---|
| DHT sensor library | Adafruit | Arduino Library Manager |
| Adafruit Unified Sensor | Adafruit | Arduino Library Manager |
| NewPing | Tim Eckel | Arduino Library Manager |
| LiquidCrystal I2C | Frank de Brabander | Arduino Library Manager |
| FastLED | FastLED | Arduino Library Manager |

Les sketches dans `test/` n'ont **aucune** bibliothèque externe requise.
