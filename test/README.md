# Test Shapeoko 2 — Harmonographe sans capteurs

Permet de tester les dessins harmonographe sur la Shapeoko 2 **avant d'avoir le montage complet** (sans DHT22, HC-SR04, encodeur, LCD). Les paramètres sont simulés depuis le PC via une interface graphique Processing.

## Architecture

```
[PC — Processing]  ←—USB série—→  [Arduino Mega 2560]  ←—Serial1—→  [Shield GRBL Shapeoko 2]
   Sliders capteurs                  Calcul courbes                      Moteurs X/Y
   Visualisation trajectoire         Envoi G-code
```

## Câblage minimal requis

| Arduino Mega | Shapeoko (shield GRBL) |
|---|---|
| Pin 18 (TX1) | RX (entrée G-code) |
| Pin 19 (RX1) | TX (sortie GRBL) |
| GND | GND commun |
| USB → PC | — |

- Alimentation Shapeoko 2 allumée (pas besoin de mettre un outil)
- Aucun capteur (DHT22, HC-SR04, encodeur, LCD) nécessaire

## Installation

### Arduino
1. Ouvrir `test/arduino_shapeoko2/arduino_shapeoko2.ino` dans l'Arduino IDE
2. Carte : **Arduino Mega 2560**
3. Téléverser sur la carte (le shield GRBL peut rester branché)

### Processing
1. Télécharger Processing 4.x : https://processing.org/download
2. Ouvrir `test/processing_interface/HarmoControl.pde`
3. Lancer le sketch (bouton Play)

## Utilisation

1. Lancer le sketch Processing
2. Sélectionner le port COM/ttyUSB de l'Arduino Mega (flèches `<` `>`)
3. Cliquer **CONNECT**
4. Cliquer **HOME** pour lancer le homing (fins de course requis) — ou ignorer si homing désactivé dans GRBL
5. Ajuster les sliders :

| Slider | Paramètre simulé | Effet sur la courbe |
|---|---|---|
| TEMP | Température [0-1] | Fréquence principale (simple → complexe) |
| HUM | Humidité [0-1] | Fréquence secondaire (compact → ouvert) |
| DIST | Distance [0-1] | Phase (orientation de la figure) |
| SON | Son [0-1] | Amplitude asymétrique + perturbation |
| VITESSE | Feedrate [200-3000 mm/min] | Vitesse de déplacement |

6. La trajectoire s'affiche en temps réel dans le canvas de droite

## Commandes disponibles

| Bouton | Action |
|---|---|
| HOME | Homing GRBL (`$H`) |
| PAUSE / RESUME | Arrêt d'urgence / reprise |
| CERCLE | Test cercle Ø50mm (validation calibration) |
| CENTRE | Retour au centre (0, 0) |

## Protocole série (référence)

Depuis Processing vers Arduino :
```
SET temp 0.50
SET hum  0.30
SET dist 0.80
SET snd  0.10
SET feed 800
CMD home
CMD pause
CMD resume
CMD circle
CMD reset
```

De l'Arduino vers Processing :
```
STATUS x=12.3 y=-45.1 mode=RUNNING pts=1234 cycle=2
LOG Message de debug
```
