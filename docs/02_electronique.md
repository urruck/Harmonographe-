# Électronique — Schémas et Composants

## 1. Architecture électronique globale

Le système est divisé en **deux Arduino** communiquant par liaison série :

```
┌─────────────────────────────────────────────────────────────────┐
│  ARDUINO MEGA #1 — "GRBL Controller" (Shapeoko standard)        │
│                                                                   │
│  • Firmware GRBL v1.1                                            │
│  • Contrôle des 3 moteurs pas-à-pas (X, Y1, Y2)                │
│  • Reçoit G-code par Serial1 (RX1/TX1)                          │
│  • Pilote les drivers DRV8825 (ou TB6600) de la Shapeoko        │
└───────────────────────┬─────────────────────────────────────────┘
                        │ Serial (G-code ASCII) 115200 baud
                        │ Câble: TX1→RX, RX1←TX (croisé)
┌───────────────────────┴─────────────────────────────────────────┐
│  ARDUINO MEGA #2 — "Sensor & Curve Controller"                   │
│                                                                   │
│  Entrées :                    Sorties :                          │
│  • DHT22      → Pin 2         • Serial1 → GRBL (G-code)         │
│  • HC-SR04    → Pin 3,4       • LCD I2C → Pin SDA/SCL           │
│  • MAX9814    → Pin A0        • LED WS2812B → Pin 6             │
│  • Bouton mode → Pin 7        • Buzzer → Pin 8                  │
│  • Potent. vitesse → A1       • LED statut → Pin 13             │
└─────────────────────────────────────────────────────────────────┘
```

---

## 2. Liste des composants électroniques

### 2.1 Microcontrôleurs

| Composant | Référence | Quantité | Rôle |
|---|---|---|---|
| Arduino Mega 2560 | A000047 | 2 | Contrôleur GRBL + Capteurs |
| (Alternative) Arduino Uno | A000066 | 1+1 | Si Mega non disponible |

> **Pourquoi Mega ?** Le Mega a 4 ports série hardware (Serial0-3), ce qui permet de déboguer sur Serial0 (USB) tout en parlant à GRBL sur Serial1, sans conflit.

### 2.2 Capteurs

| Capteur | Référence | Grandeur mesurée | Plage |
|---|---|---|---|
| **DHT22** | AM2302 | Température | -40 à +80°C ±0.5°C |
| **DHT22** | AM2302 | Humidité relative | 0-100% ±2% |
| **HC-SR04** | HCSR04 | Distance ultrasonique | 2-400 cm ±3mm |
| **MAX9814** | Adafruit 1713 | Niveau sonore (micro) | 40-110 dB |

### 2.3 Interface utilisateur

| Composant | Référence | Quantité |
|---|---|---|
| LCD 20×4 I2C | HD44780 + PCF8574 | 1 |
| Encodeur rotatif | KY-040 | 1 |
| Bouton-poussoir | 12mm momentané | 3 |
| LED RGB 5mm | Commune cathode | 3 |
| Buzzer passif | 5V 2kHz | 1 |

### 2.4 Gestion de puissance

| Composant | Référence | Quantité |
|---|---|---|
| Alimentation 24V 10A | Meanwell LRS-240-24 | 1 |
| Alimentation 5V 3A | Meanwell RS-15-5 | 1 |
| Régulateur 5V 1A | LM7805 + radiateur | 1 |
| Condensateur 100µF 35V | Électrolytique | 4 |
| Condensateur 100nF | Céramique | 10 |

### 2.5 Optionnel — Éclairage LED

| Composant | Quantité |
|---|---|
| Bande LED WS2812B 60LED/m | 2m |
| Condensateur 1000µF 10V | 1 |
| Résistance 470Ω | 1 |

---

## 3. Schémas de câblage des capteurs

### 3.1 Capteur DHT22

```
          DHT22
    ┌─────────────┐
    │ VCC DATA NC GND │
    └──┬──┬────┬───┘
       │  │    │
      3.3V │   GND
           │
        Pin 2 (Arduino)
           │
        [10kΩ]            ← Résistance pull-up OBLIGATOIRE
           │
         3.3V

Note : Utiliser 3.3V pour DHT22 (supporte 3.3-6V)
       La résistance pull-up est parfois intégrée sur les modules.
```

**Code de connexion :**
```
DHT22 VCC  → Arduino 3.3V
DHT22 DATA → Arduino Pin 2 (+ résistance 10kΩ vers 3.3V)
DHT22 GND  → Arduino GND
```

### 3.2 Capteur ultrasonique HC-SR04

```
         HC-SR04
    ┌──────────────┐
    │ VCC TRIG ECHO GND │
    └──┬───┬───┬───┘
       │   │   │
      5V  P3  P4   GND

                 ┌─[1kΩ]─→ Pin 4 (ECHO)    ← Diviseur de tension
    ECHO ────────┤
                 └─[2kΩ]─→ GND             ← 5V→3.3V pour Arduino 3.3V

Note : Si Arduino 5V (Mega), ECHO peut être connecté directement.
       Si vous utilisez un niveau 3.3V, utilisez un diviseur.
```

**Code de connexion :**
```
HC-SR04 VCC  → Arduino 5V
HC-SR04 TRIG → Arduino Pin 3
HC-SR04 ECHO → Arduino Pin 4
HC-SR04 GND  → Arduino GND
```

### 3.3 Microphone MAX9814

```
         MAX9814
    ┌──────────────┐
    │ VDD OUT GND AR│
    └──┬───┬───┬──┘
       │   │   │
      3.3V A0  GND   (AR = non connecté par défaut)

Gain par défaut : 40dB (AR flottant)
Gain 50dB : AR→GND
Gain 60dB : AR→VDD
```

**Code de connexion :**
```
MAX9814 VDD  → Arduino 3.3V
MAX9814 OUT  → Arduino A0
MAX9814 GND  → Arduino GND
MAX9814 AR   → Non connecté (gain 40dB par défaut)
```

### 3.4 Écran LCD I2C 20×4

```
         LCD I2C
    ┌──────────────┐
    │ GND VCC SDA SCL │
    └──┬───┬───┬───┘
       │   │   │
      GND  5V SDA  SCL   (Arduino Mega: SDA=Pin20, SCL=Pin21)

Adresse I2C par défaut : 0x27 (ou 0x3F selon modèle)
Tester avec le sketch I2C Scanner si écran non détecté.
```

### 3.5 Encodeur rotatif KY-040

```
         KY-040
    ┌──────────────┐
    │ CLK DT SW VCC GND │
    └──┬──┬──┬───┬──┘
       │  │  │
      P9 P10 P11    (avec pull-up internes Arduino activés)
```

**Code de connexion :**
```
KY-040 CLK → Arduino Pin 9
KY-040 DT  → Arduino Pin 10
KY-040 SW  → Arduino Pin 11
KY-040 VCC → Arduino 5V
KY-040 GND → Arduino GND
```

---

## 4. Schéma global ASCII — Arduino Capteurs

```
                    ARDUINO MEGA 2560 (#2)
                   ┌─────────────────────────────┐
              3.3V─┤                             ├─ ...
               GND─┤                             │
                   │                             │
    DHT22 DATA ────┤ Pin 2                       │
    HC-SR04 TRIG───┤ Pin 3             Pin 20 SDA├──── LCD SDA
    HC-SR04 ECHO───┤ Pin 4             Pin 21 SCL├──── LCD SCL
    BUTTON MODE ───┤ Pin 7                       │
    LED_WS2812B ───┤ Pin 6                       │
    BUZZER ────────┤ Pin 8                       │
    ENC_CLK ───────┤ Pin 9                       │
    ENC_DT ────────┤ Pin 10                      │
    ENC_SW ────────┤ Pin 11                      │
    LED_STATUS ────┤ Pin 13                      │
                   │                             │
    MAX9814 OUT ───┤ A0                          │
    POT_VITESSE ───┤ A1                          │
                   │                             │
                   │      Serial1 (GRBL) ←→ MEGA│
                   │      TX1 Pin 18 ────────────┤──── TX → GRBL RX
                   │      RX1 Pin 19 ────────────┤──── RX ← GRBL TX
                   └─────────────────────────────┘
```

---

## 5. Communication Arduino → GRBL

### Protocole

La communication se fait en **G-code ASCII** à 115200 bauds via Serial1 :

```
Arduino Capteurs (TX1) ──────────────────→ Shapeoko GRBL (RX)
Arduino Capteurs (RX1) ←────────────────── Shapeoko GRBL (TX)

Exemple d'échange :
→ "G90 G21\n"                 # Mode absolu, millimètres
← "ok\n"
→ "F500\n"                    # Vitesse 500 mm/min
← "ok\n"
→ "G1 X123.456 Y-89.123\n"  # Aller au point calculé
← "ok\n"
```

### Gestion du flux (flow control)

GRBL a un buffer de 127 caractères. L'Arduino capteurs doit :
1. Envoyer une ligne G-code
2. Attendre le `ok\n` de réponse
3. Envoyer la ligne suivante

---

## 6. Pinout complet de référence

### Arduino Mega #2 (Capteurs)

| Pin | Nom | Composant | Détails |
|---|---|---|---|
| 2 | DHT_DATA | DHT22 | Pull-up 10kΩ vers 3.3V |
| 3 | TRIG | HC-SR04 | Trigger ultrason |
| 4 | ECHO | HC-SR04 | Echo ultrason |
| 6 | LED_DATA | WS2812B | Données LED RGB |
| 7 | BTN_MODE | Bouton | Pull-up interne |
| 8 | BUZZER | Buzzer passif | Via transistor NPN |
| 9 | ENC_CLK | KY-040 | Pull-up interne |
| 10 | ENC_DT | KY-040 | Pull-up interne |
| 11 | ENC_SW | KY-040 | Pull-up interne |
| 13 | LED_STATUS | LED | Via résistance 220Ω |
| 18 (TX1) | GRBL_TX | GRBL Mega | Envoi G-code |
| 19 (RX1) | GRBL_RX | GRBL Mega | Réception GRBL |
| 20 (SDA) | I2C_SDA | LCD | I2C données |
| 21 (SCL) | I2C_SCL | LCD | I2C horloge |
| A0 | MIC_OUT | MAX9814 | Niveau sonore |
| A1 | POT_VIT | Potentiomètre | Vitesse manuelle |

---

## 7. Condensateurs de découplage

**Règle d'or** : Placer un condensateur 100nF près de chaque VCC de capteur.

```
Pour chaque capteur :

    VCC ──────┬──────→ VCC capteur
              │
           [100nF]     ← Condensateur céramique
              │
    GND ──────┴──────→ GND capteur
```

Sur l'alimentation principale 5V, ajouter un électrolytique 100µF.

---

## 8. Protection contre les interférences sonores

Le microphone MAX9814 est sensible aux perturbations électromagnétiques des moteurs pas-à-pas. Mesures de protection :

1. **Câble blindé** pour le signal micro (coaxial ou torsadé + blindage)
2. **Condensateur 10µF** en parallèle sur VCC du MAX9814
3. **Ferrite** sur le câble micro à l'entrée du boîtier
4. **Séparation physique** : max 30cm entre le câble micro et les câbles moteur
5. **Moyenne mobile** sur les lectures ADC (filtrage logiciel — implémenté dans le firmware)
