# BOM — Liste des Matériaux (Bill of Materials)

## Résumé des coûts

| Catégorie | Coût estimé (€) |
|---|---|
| Mécanique | 100-140 € |
| Électronique | 70-110 € |
| Électrique | 60-90 € |
| Visserie & consommables | 20-30 € |
| **TOTAL** | **250-370 €** |

> Note : La **Shapeoko 2** (base CNC) et son contrôleur Arduino Uno + gShield sont considérés comme déjà disponibles.
> Prix indicatifs 2024, hors frais de port.

---

## 1. Mécanique

| # | Désignation | Référence / Fournisseur | Qté | Prix unit. | Total |
|---|---|---|---|---|---|
| M01 | Aimant néodyme N52 disque Ø50mm × 20mm | Supermagnete S-50-20-N / Amazon | 1 | 18 € | 18 € |
| M02 | Bille d'acier chromé Ø19mm (3/4") | SKF / RS Components / Amazon | 5 | 1.5 € | 7.5 € |
| M03 | Plaque acrylique transparente 250×250mm ép.3mm | Plastics-shop / Leroy Merlin | 1 | 8 € | 8 € |
| M04 | Profilé aluminium 30×30 fente 6mm (série 30) | MakerBeam / Motedis / Amazon | 3m | 8 €/m | 24 € |
| M05 | Équerre d'angle 30×30mm pour profilé | Motedis / Amazon | 16 | 0.80 € | 13 € |
| M06 | Vis M6×10 à tête cylindrique (profilés) | Quincaillerie | 32 | 0.05 € | 1.6 € |
| M07 | Écrou M6 marteau (fente 6mm) | Motedis / Amazon | 32 | 0.30 € | 9.6 € |
| M08 | Sable de quartz fin Ø0.1-0.3mm | Jardinerie / Aquarium | 3 kg | 4 €/kg | 12 € |
| M09 | Joint silicone noir 4mm (rouleau) | Brico-dépôt / Amazon | 1.2m | 5 €/m | 6 € |
| M10 | Visserie inox M3 assortiment | Quincaillerie | 1 boîte | 8 € | 8 € |
| M11 | Filament PETG 1kg (support aimant + pattes) | Colorfabb / Prusament | 0.5 kg | 25 €/kg | 12.5 € |
| **Total Mécanique** | | | | | **~120 €** |

### Notes mécanique

- **M01** : L'aimant N52 Ø50×20mm développe une force d'attraction d'environ 18kg sur une plaque d'acier. À travers 3mm d'acrylique + 3mm de sable ≈ 12mm, la force est d'environ 2-3kg. Suffisant pour entraîner une bille de 30g.
- **M02** : Commander 5 billes pour en tester différentes. La taille Ø19mm est le meilleur compromis. Les billes Ø25mm nécessitent un aimant plus puissant.
- **M11** : Imprimer les pièces suivantes (fichiers STL dans `assets/`) :
  - Support aimant (1×)
  - Pattes de fixation bac (4×)
  - Boîtier capteurs (1×)

---

## 2. Électronique

| # | Désignation | Référence / Fournisseur | Qté | Prix unit. | Total |
|---|---|---|---|---|---|
| E01 | Arduino Uno R3 (GRBL — déjà dans la Shapeoko 2) | Arduino A000066 | 1 | 0 € | 0 € |
| E01b | Arduino Mega 2560 (Capteurs) | Arduino A000047 / Gotronic | 1 | 25 € | 25 € |
| E02 | Capteur DHT22 (AM2302) | Adafruit 385 / Amazon | 1 | 10 € | 10 € |
| E03 | Capteur ultrasonique HC-SR04 | HiLetgo / Amazon | 1 | 3 € | 3 € |
| E04 | Amplificateur micro MAX9814 | Adafruit 1713 | 1 | 8.5 € | 8.5 € |
| E05 | LCD 20×4 avec module I2C | Keyestudio / Amazon | 1 | 12 € | 12 € |
| E06 | Encodeur rotatif KY-040 | Amazon / Gotronic | 1 | 2 € | 2 € |
| E07 | Bande LED WS2812B 60LED/m | Adafruit / Amazon | 1.2m | 12 €/m | 14.4 € |
| E08 | Condensateur électrolytique 100µF 35V | Composants électroniques | 5 | 0.50 € | 2.5 € |
| E09 | Condensateur électrolytique 1000µF 10V | Composants électroniques | 2 | 0.80 € | 1.6 € |
| E10 | Condensateur céramique 100nF | Composants électroniques | 20 | 0.10 € | 2 € |
| E11 | Résistance 10kΩ 1/4W | Composants électroniques | 5 | 0.05 € | 0.25 € |
| E12 | Résistance 470Ω 1/4W | Composants électroniques | 5 | 0.05 € | 0.25 € |
| E13 | Résistance 220Ω 1/4W | Composants électroniques | 5 | 0.05 € | 0.25 € |
| E14 | Bouton poussoir 12mm (momentané) | Amazon / Gotronic | 5 | 0.50 € | 2.5 € |
| E15 | Buzzer passif 5V | HiLetgo / Amazon | 1 | 1.5 € | 1.5 € |
| E16 | LED RGB 5mm commune cathode | Composants | 5 | 0.30 € | 1.5 € |
| E17 | Câble Dupont F-F 20cm (assortiment) | Amazon | 1 kit | 6 € | 6 € |
| E18 | Câble coaxial blindé (micro) | Conrad / RS | 1m | 3 € | 3 € |
| E19 | Ferrite sur câble Ø5mm | RS Components | 2 | 2 € | 4 € |
| **Total Électronique** | | | | | **~110 €** |

### Notes électronique

- **E01** : L'Arduino Uno + gShield de la Shapeoko 2 est déjà présent dans la machine — pas de coût supplémentaire.
- **E01b** : L'Arduino Mega est utilisé pour les capteurs (4 ports série hardware, pratique pour déboguer).
- **E04** : Le MAX9814 est nettement meilleur que le KY-038 pour capturer les sons ambiants (préampli intégré, AGC optionnel).
- **E07** : Commander légèrement plus (2.5m) pour les connexions et les découpes.

---

## 3. Électrique et alimentation

| # | Désignation | Référence / Fournisseur | Qté | Prix unit. | Total |
|---|---|---|---|---|---|
| P01 | Alimentation 24V DC 10A (240W) | Meanwell LRS-240-24 / RS | 1 | 35 € | 35 € |
| P02 | Alimentation 5V DC 3A (15W) | Meanwell RS-15-5 / RS | 1 | 18 € | 18 € |
| P03 | Câble secteur H05VV-F 3×1.5mm² | Castorama / Brico | 2m | 3 €/m | 6 € |
| P04 | Câble souple 0.5mm² bifilaire | Brico | 5m | 0.50 €/m | 2.5 € |
| P05 | Câble souple 0.25mm² multifilaire | Conrad / RS | 5m | 0.30 €/m | 1.5 € |
| P06 | Bouton d'arrêt urgence rouge 40mm | Schneider XB4 / RS | 1 | 15 € | 15 € |
| P07 | Fusible verre 10A temporisé (avec porte-fusible) | Brico / Conrad | 1 | 4 € | 4 € |
| P08 | Fusible verre 15A temporisé | Brico / Conrad | 2 | 1.5 € | 3 € |
| P09 | Bornier à vis 5A (assortiment) | Legrand / Wago | 1 boîte | 8 € | 8 € |
| P10 | Boîtier ABS 200×150×80mm (électronique) | Hammond / RS | 1 | 18 € | 18 € |
| P11 | Presse-étoupes PG7 + PG9 | Conrad / Amazon | 10 | 1 € | 10 € |
| P12 | Gaine thermorétractable assortiment | Amazon | 1 kit | 6 € | 6 € |
| P13 | Cosses à sertir assortiment | Conrad | 1 boîte | 8 € | 8 € |
| **Total Électrique** | | | | | **~135 €** |

### Notes électrique

- **P01** : Le Meanwell LRS-240-24 est une alimentation industrielle fiable. Alternative : alimentation 24V pour imprimante 3D (moins cher, moins fiable).
- **P06** : L'arrêt d'urgence est **obligatoire** pour la sécurité. Ne pas l'omettre.
- **P10** : Placer tout l'électronique dans ce boîtier (les deux Arduino, les alimentations 5V, les condensateurs, les borniers).

---

## 4. Outillage nécessaire (si non disponible)

| # | Outil | Utilité | Coût approx. |
|---|---|---|---|
| O01 | Imprimante 3D (FDM, PETG) | Support aimant, pattes | Déjà disponible / 200-400€ |
| O02 | Fer à souder + étain | Soudure composants | 30-80€ |
| O03 | Multimètre | Tests continuité, tensions | 20-50€ |
| O04 | Pince à sertir | Cosses électriques | 25-50€ |
| O05 | Coupe-boulons inox | Aimant manipulation | 10€ |
| O06 | Tournevis cruciforme PH0/PH1/PH2 | Assemblage | 10€ |

> **⚠️ Sécurité aimants** : Les aimants N52 Ø50mm sont extrêmement puissants (~18kg de force). Porter des gants épais lors de la manipulation. Ne jamais approcher deux aimants rapidement l'un de l'autre (risque de se pincer les doigts très fort).

---

## 5. Sources d'approvisionnement recommandées

| Fournisseur | Spécialité | Site |
|---|---|---|
| **Gotronic** | Électronique, Arduino (FR) | gotronic.fr |
| **Mouser** | Composants électroniques pro | mouser.fr |
| **RS Components** | Composants industriels | fr.rs-online.com |
| **Motedis** | Profilés aluminium (DE/EU) | motedis.fr |
| **Supermagnete** | Aimants néodyme (EU) | supermagnete.fr |
| **Conrad** | Électronique générale (FR) | conrad.fr |
| **Adafruit** | Modules capteurs US (via Mouser) | adafruit.com |
| **Amazon FR** | Composants génériques | amazon.fr |
| **Castorama/Leroy Merlin** | Visserie, câbles, boîtiers | — |

---

## 6. Planning de fabrication estimé

| Phase | Durée estimée | Tâches |
|---|---|---|
| Préparation | 1 week-end | Commander les composants, préparer l'espace |
| Impression 3D | 1 journée | Support aimant, pattes, boîtier capteurs |
| Mécanique | 1 week-end | Bac à sable, structure, montage aimant |
| Électronique | 1 week-end | Câblage capteurs, test individuel |
| Électrique | 1 journée | Boîtier, alimentation, câblage final |
| Firmware | 1-2 soirées | Flash GRBL, upload sketch, calibration |
| Calibration | 1 journée | Réglage hauteur aimant, paramètres GRBL, courbes |
| **Total** | **~3 week-ends** | |
