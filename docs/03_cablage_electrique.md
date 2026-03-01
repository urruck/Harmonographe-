# Câblage Électrique

## 1. Architecture d'alimentation

```
220V AC (secteur)
      │
      ├─────────────────────────────────────────────────────┐
      │                                                       │
  ┌───▼──────────────────┐                     ┌─────────────▼──────┐
  │  Meanwell LRS-240-24 │                     │  Meanwell RS-15-5  │
  │    24V DC — 10A      │                     │    5V DC — 3A      │
  └───┬──────────────────┘                     └─────┬──────────────┘
      │                                               │
      │ 24V                                           │ 5V
      │                                               │
  ┌───▼─────────────────────┐              ┌──────────▼────────────┐
  │  Shapeoko CNC Board     │              │  Arduino Mega #2      │
  │  (DRV8825 / TB6600)     │              │  + Capteurs           │
  │  Moteurs X, Y1, Y2      │              │  + LCD + LED WS2812B  │
  └─────────────────────────┘              └───────────────────────┘
                                                    │ USB
                                           ┌────────▼──────┐
                                           │ Arduino Mega#1│
                                           │ (GRBL)        │
                                           └───────────────┘
```

> **Note** : L'Arduino GRBL (#1) peut être alimenté via USB depuis un hub ou un chargeur 5V USB séparé, ou depuis la carte Shapeoko si elle dispose d'une sortie 5V.

---

## 2. Câblage des alimentations

### 2.1 Alimentation 24V (Meanwell LRS-240-24)

```
BORNIER SECTEUR (LRS-240-24) :
┌───────────────────────────────────┐
│  L   N   PE   +V   -V   ADJ      │
└──┬───┬───┬────┬────┬──────────────┘
   │   │   │    │    │
 Phase Neutre Terre 24V  GND
   │   │   │    │    │
   └───┴───┴────┴────┴─── (câble 3×1.5mm² sectionné avec fusible 10A)
```

**Fusibles recommandés :**
- Côté 220V AC : Fusible 10A temporisé (slo-blo) dans le cordon secteur
- Côté 24V DC : Fusible 15A sur le +24V vers la Shapeoko

### 2.2 Alimentation 5V pour l'électronique

```
Option A — Alimentation dédiée Meanwell RS-15-5 :
  +5V ──→ VCC Arduino Mega #2 (pin Vin avec régulateur 7805)
  GND ──→ GND Arduino

Option B — Régulateur LM7805 depuis 12V :
  +12V ─→ [LM7805] ─→ +5V ─→ Arduino Mega #2
  GND ────────────────→ GND

  ⚠️ LM7805 chauffe ! Ajouter radiateur aluminium + pâte thermique
     I_max = 1A. Au-delà, utiliser un LM338 ou une alimentation dédiée.
```

---

## 3. Câblage des moteurs Shapeoko

La Shapeoko utilise des moteurs NEMA 23 (ou NEMA 17 selon version) câblés sur le contrôleur d'origine. **Ne pas modifier ce câblage**.

```
Shapeoko Board
┌────────────────────────────────┐
│  X_STEP  X_DIR  X_EN          │──→ Moteur X (1 moteur)
│  Y_STEP  Y_DIR  Y_EN          │──→ Moteur Y1 + Y2 (en parallèle)
│  Z_STEP  Z_DIR  Z_EN          │──→ (non utilisé — Z fixe)
│  +24V  GND                    │──→ Alimentation moteurs
│  RX    TX                     │──→ Arduino #2 (croisé)
└────────────────────────────────┘
```

**Courant moteur NEMA 23 :** Régler les DRV8825 à 2A/phase (vis de réglage Vref = 0.4V pour 2A).

**Formule :** `Vref = I_moteur × 0.1 × 8 = I_moteur × 0.8`
- Pour 2A : Vref = 1.6V
- Pour 1.5A : Vref = 1.2V

---

## 4. Câblage des capteurs — Tableau récapitulatif

### 4.1 DHT22 — Température & Humidité

```
DHT22                          Arduino Mega #2
Pin 1 (VCC) ──────────────────→ 3.3V
Pin 2 (DATA)─────[10kΩ]──────→ 3.3V   (pull-up)
Pin 2 (DATA)──────────────────→ Pin 2
Pin 3 (NC)  ─── Non connecté
Pin 4 (GND) ──────────────────→ GND

Câble recommandé : 4 fils, 0.25mm², longueur max 2m
```

### 4.2 HC-SR04 — Distance

```
HC-SR04                        Arduino Mega #2
VCC ───────────────────────────→ 5V
TRIG ──────────────────────────→ Pin 3
ECHO ──────────────────────────→ Pin 4
GND ───────────────────────────→ GND

Câble recommandé : 4 fils, 0.25mm², longueur max 1m
Position : Monté en façade du bac à sable, à ~30cm de la surface
```

### 4.3 MAX9814 — Microphone

```
MAX9814 (module Adafruit)       Arduino Mega #2
VDD ───────────────────────────→ 3.3V
GND ───────────────────────────→ GND
OUT ───────────────────────────→ A0
AR ──── Non connecté (gain 40dB)

Câble BLINDÉ recommandé : coaxial fin, longueur max 50cm
Le blindage (tresse) → GND côté Arduino
Position : En dessous du bac, face vers les utilisateurs
```

### 4.4 LCD I2C 20×4

```
LCD I2C                         Arduino Mega #2
GND ───────────────────────────→ GND
VCC ───────────────────────────→ 5V
SDA ───────────────────────────→ Pin 20 (SDA)
SCL ───────────────────────────→ Pin 21 (SCL)

Câble : 4 fils, 0.25mm², longueur max 50cm
```

### 4.5 LED WS2812B (optionnel)

```
WS2812B (entrée bande)          Arduino Mega #2
+5V ───[1000µF 10V]────────────→ 5V (condensateur sur bornier)
DIN ───[470Ω]──────────────────→ Pin 6
GND ───────────────────────────→ GND

⚠️ La résistance 470Ω sur le signal DIN est OBLIGATOIRE
   pour protéger la première LED des oscillations.
⚠️ Le condensateur 1000µF absorbe les pics de courant au démarrage.
⚠️ Courant max : 60mA/LED × nombre de LEDs = calculer séparément
```

---

## 5. Liaison série Arduino #2 ↔ Arduino #1 (GRBL)

```
Arduino Mega #2                 Arduino Mega #1 (GRBL)
(Capteurs)                      (Shapeoko)

Pin 18 (TX1) ─────────────────→ RX (Pin 0)
Pin 19 (RX1) ←───────────────── TX (Pin 1)
GND ──────────────────────────── GND   ← IMPORTANT : masse commune

Configuration série :
- Baud rate : 115200
- Format : 8N1
- Niveau : 5V TTL (compatible direct entre Mega)
```

> **⚠️ IMPORTANT** : Ne jamais connecter TX→TX ou RX→RX. La liaison est croisée : TX→RX et RX→TX.

> **⚠️ Masse commune** : Les deux Arduino doivent partager la même masse (GND), sinon la communication ne fonctionne pas.

---

## 6. Schéma de câblage complet

```
                    BOÎTIER ÉLECTRONIQUE
    ╔═══════════════════════════════════════════════════════╗
    ║  ┌──────────────────┐    ┌──────────────────────────┐ ║
    ║  │  Alimentation    │    │  Alimentation 5V 3A      │ ║
    ║  │  24V 10A         │    │  (capteurs + LCD + LED)  │ ║
    ║  │  Meanwell        │    │  Meanwell RS-15-5        │ ║
    ║  └────────┬─────────┘    └────────────┬─────────────┘ ║
    ║           │ 24V                        │ 5V           ║
    ║  ┌────────▼─────────┐    ┌────────────▼─────────────┐ ║
    ║  │  Shapeoko Board  │    │  Arduino Mega #2         │ ║
    ║  │  + Arduino #1    │    │  Capteurs + Courbes       │ ║
    ║  │  GRBL v1.1       │◄───┤  (Serial1 G-code)        │ ║
    ║  └──────────────────┘    └──────────────────────────┘ ║
    ║           │                           │               ║
    ║        Moteurs                    Capteurs           ║
    ╚═══════════════════════════════════════════════════════╝
               │                           │
         ┌─────▼──────┐         ┌──────────▼────────────┐
         │  Shapeoko  │         │  DHT22                │
         │  X + Y     │         │  HC-SR04              │
         │  (CNC)     │         │  MAX9814              │
         └─────┬──────┘         │  LCD 20×4             │
               │                │  LED WS2812B          │
         ┌─────▼──────┐         └───────────────────────┘
         │  AIMANT    │
         │  N52 Ø50mm │
         └─────┬──────┘
               │ (champ magnétique)
         ┌─────▼──────┐
         │ ~~SABLE~~  │
         │     ●      │ ← Bille d'acier
         └────────────┘
```

---

## 7. Sections de câbles recommandées

| Câble | Section | Type | Couleur convention |
|---|---|---|---|
| 220V AC secteur | 3×1.5mm² | H05VV-F | Marron/Bleu/Vert-Jaune |
| 24V moteurs | 1mm² | Rigide | Rouge/Noir |
| 5V alimentation | 0.5mm² | Souple | Rouge/Noir |
| Signal capteurs | 0.25mm² | Souple | Couleurs variées |
| Signal micro | Coaxial | Blindé | — |
| Serial entre Arduino | 0.25mm² | Souple | Blanc/Gris |

---

## 8. Sécurités électriques

### 8.1 Protections obligatoires

| Protection | Valeur | Emplacement |
|---|---|---|
| Disjoncteur différentiel 30mA | 16A | Tableau électrique |
| Fusible AC | 10A T | Dans le cordon secteur |
| Fusible DC 24V | 15A | Sur le +24V vers moteurs |
| Fusible DC 5V | 5A | Sur le +5V vers Arduino |

### 8.2 Mise à la terre

**Obligatoire** : Relier la carcasse métallique de la Shapeoko à la terre (PE) du secteur. Utiliser un câble vert-jaune de section minimale 1.5mm².

### 8.3 Bouton d'arrêt d'urgence

Installer un **bouton d'arrêt d'urgence rouge** (NC — normalement fermé) en série sur le +24V des moteurs :

```
+24V ──→ [BOUTON URGENCE NC] ──→ Shapeoko Board +24V
```

En appuyant, les moteurs perdent leur alimentation et s'arrêtent immédiatement.
