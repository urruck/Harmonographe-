# Harmonographe de Sable Interactif — Style Sisyphus

> Machine artistique interactive à bille de sable contrôlée par aimant, montée sur base Shapeoko, pilotée par Arduino et modulée par capteurs environnementaux.

---

## Vue d'ensemble

Ce projet transforme une fraiseuse CNC Shapeoko en une machine artistique interactive de type **Sisyphus** : une bille d'acier roule sur du sable, guidée par un aimant néodyme se déplaçant sous le plateau. Les trajectoires sont générées par un **algorithme d'harmonographe à 3 pendules virtuels**, dont les paramètres sont modulés en temps réel par des capteurs :

| Capteur | Paramètre modulé |
|---|---|
| 🌡️ Température (DHT22) | Ratio de fréquences f₁/f₂ → type de figure |
| 💧 Humidité (DHT22) | Ratio f₃/f₂ → étirement vertical |
| 📡 Distance (HC-SR04) | Décalage de phase → rotation du motif |
| 🔊 Son (MAX9814) | Amplitude / perturbation organique |

---

## Architecture du système

```
┌─────────────────────────────────────────────────────────┐
│                  SHAPEOKO CNC (modifiée)                  │
│  ┌──────────┐   ┌──────────┐                             │
│  │ Moteur X │   │ Moteur Y │  × 2 (axe Y bilatéral)     │
│  └──────────┘   └──────────┘                             │
│         │              │                                  │
│         └──────┬───────┘                                  │
│                │                                          │
│         ┌──────▼──────┐                                   │
│         │  Aimant N52 │ ← monté sur le chariot           │
│         │  (sous bac) │                                   │
│         └─────────────┘                                   │
│  ════════════════════════ Bac à sable ════════════════   │
│                    ●  Bille d'acier                       │
└─────────────────────────────────────────────────────────┘
           ↑
    ┌──────┴──────────────────────────────┐
    │     Arduino Mega 2560 (GRBL)        │
    │         Contrôleur CNC              │
    └──────┬──────────────────────────────┘
           │ Serial (G-code)
    ┌──────┴──────────────────────────────┐
    │     Arduino Uno/Mega (Capteurs)     │
    │  DHT22 | HC-SR04 | MAX9814 | LCD   │
    │      Génération des trajectoires    │
    └─────────────────────────────────────┘
```

---

## Structure du projet

```
Harmonographe-/
├── README.md                          ← Ce fichier
├── docs/
│   ├── 01_conception_mecanique.md     ← Mécanique, adaptation Shapeoko
│   ├── 02_electronique.md             ← Schémas et composants
│   ├── 03_cablage_electrique.md       ← Câblage et alimentation
│   ├── 04_algorithme_harmonographe.md ← Théorie et implémentation
│   └── 05_bom_liste_materiaux.md      ← BOM complet avec prix
├── firmware/
│   └── harmonographe_capteurs/
│       ├── harmonographe_capteurs.ino ← Programme principal
│       ├── curves.h                   ← Algorithme harmonographe
│       ├── sensors.h                  ← Gestion capteurs
│       └── gcode_sender.h             ← Envoi G-code au GRBL
└── schematics/
    ├── schema_global.txt              ← Schéma ASCII global
    └── pinout_arduino.txt             ← Correspondance des broches
```

---

## Démarrage rapide

### 1. Matériel requis (résumé)
- Shapeoko 3 (ou 4) — base CNC
- Arduino Mega 2560 × 2 (un pour GRBL, un pour les capteurs)
- Aimant néodyme N52 Ø50mm × 20mm
- Capteurs : DHT22, HC-SR04, MAX9814
- Bille d'acier Ø19mm (3/4")
- Bac à sable 400×400mm en acrylique 3mm

### 2. Installation firmware
```bash
# Arduino 1 (Shapeoko) — flasher GRBL v1.1
# (via Arduino IDE avec la bibliothèque GRBL)

# Arduino 2 (Capteurs) — flasher le firmware harmonographe
# Ouvrir firmware/harmonographe_capteurs/harmonographe_capteurs.ino
# Installer les bibliothèques : DHT, NewPing, AccelStepper
```

### 3. Connexion
- Relier RX/TX de l'Arduino Capteurs au TX/RX du Shapeoko (via niveau logique 5V→5V)
- Alimenter séparément : 24V pour les moteurs, 5V USB pour l'Arduino capteurs

---

## Principe artistique

La machine crée des **courbes harmonographe organiques** en superposant 3 oscillateurs sinusoïdaux virtuels. Contrairement à un harmonographe physique à pendules (qui s'amortit), cette machine maintient un mouvement continu et **évolue lentement** sous l'influence des capteurs :

- En été (chaud + humide) → motifs denses et tourbillonnants
- En hiver (froid + sec) → figures de Lissajous géométriques pures
- Quand quelqu'un parle → perturbations organiques, inflexions
- Quand quelqu'un approche → changement de phase, rotation du motif

---

## Références

- [Sisyphus Industries](https://sisyphus-industries.com) — inspiration principale
- [Harmonographe Wikipedia](https://fr.wikipedia.org/wiki/Harmonographe)
- [GRBL v1.1 Documentation](https://github.com/gnea/grbl)
- [Shapeoko Documentation](https://docs.carbide3d.com)
