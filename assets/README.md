# Assets — Pièces imprimées 3D

Ce dossier est destiné à recevoir les fichiers STL des pièces à imprimer en 3D.
Les fichiers STL ne sont pas encore générés ; cette page décrit les **cotes et contraintes de conception** pour les créer avec FreeCAD, Fusion 360, OpenSCAD ou tout autre logiciel de CAO.

---

## Paramètres d'impression recommandés

| Paramètre | Valeur |
|---|---|
| Matière | **PETG** (résistant à la chaleur et aux contraintes) |
| Épaisseur de couche | 0.2 mm |
| Remplissage | 40 % (gyroïde ou nid d'abeille) |
| Périmètres | 3 |
| Supports | Selon pièce (indiqué ci-dessous) |

> ⚠️ Ne pas imprimer en PLA : la chaleur dégagée par les moteurs et les drivers peut le déformer.

---

## 1. Support aimant — `support_aimant.stl`

### Rôle
Fixe l'aimant N52 Ø50mm × 20mm sous le chariot Z de la **Shapeoko 2**, à la place de la broche.

### Contraintes Shapeoko 2
Le chariot Z de la Shapeoko 2 expose une platine de montage avec **4 trous M5 espacés de 50×50mm** (entraxe). Vérifier sur votre machine avant d'imprimer.

### Cotes à respecter

```
Vue de face :

    ┌──────────────────────────┐   ← Plaque de fixation : 80×80mm
    │  ●         ●             │   ← Trous M5 Ø5.5mm — entraxe 50×50mm
    │                          │
    │      ┌──────────┐        │   ← Cage aimant :
    │      │  Ø 52mm  │        │     Ø intérieur 52mm (aimant Ø50 + jeu 1mm)
    │      │  P 22mm  │        │     Profondeur 22mm (aimant 20mm + fond 2mm)
    │      └──────────┘        │
    │  ●         ●             │
    └──────────────────────────┘
         Épaisseur totale : 8mm

Vue de côté (montée sous le chariot) :

    ══════════ Chariot Z Shapeoko 2 ══════════
          │  │  │  │   ← Vis M5×16 (4×)
    ┌─────┴──┴──┴──┴──────┐  ← Plaque fixation (8mm)
    │                      │
    │    ╔════════╗         │  ← Cage aimant (22mm de profondeur)
    │    ║ AIMANT ║         │
    │    ║  N52   ║         │  ← L'aimant est côté BAS (vers le bac)
    │    ╚════════╝         │
    └──────────────────────┘
    Hauteur totale sous chariot : 30mm
```

### Dimensions récapitulatives

| Élément | Cote |
|---|---|
| Plaque de base | 80 × 80 × 8 mm |
| Trous de fixation | Ø5.5 mm, entraxe 50×50 mm (M5) |
| Logement aimant (Ø int.) | Ø52 mm |
| Profondeur logement | 22 mm |
| Trous de blocage latéraux | 2× M3, à 90° l'un de l'autre |

### Supports d'impression
Supports nécessaires à l'intérieur du logement aimant (fond de cage).

---

## 2. Pattes de fixation du bac — `patte_fixation_bac.stl` (× 4)

### Rôle
Maintient le bac à sable 250×250mm centré sur le plateau de la Shapeoko 2, sans le percer.

### Principe

```
Vue de dessus — Bac + 4 pattes :

    ┌──────────────────────────┐
    │ [P]                  [P] │   ← Pattes aux 4 coins
    │                          │
    │     Bac à sable          │
    │      250 × 250mm         │
    │                          │
    │ [P]                  [P] │
    └──────────────────────────┘

Vue d'une patte (coupe) :

    ╔══════╗          ← Plateau Shapeoko 2
    ║ Slot ║ ← Fente pour vis M5 dans le profilé du plateau (ou collage)
    ╠══════╣
    ║      ║ ← Corps de la patte (20mm de hauteur)
    ║  L   ║
    ╚══════╬══╗  ← Crochet retenant le bord du bac (5mm de saillie)
           ║  ║
           ╚══╝
```

### Cotes

| Élément | Cote |
|---|---|
| Base de fixation (plateau) | 30 × 20 × 5 mm |
| Corps vertical | 20 × 15 × 20 mm |
| Crochet de retenue | 5 mm de saillie, 10 mm de large |
| Trou de fixation | Ø5.5 mm (M5) ou fente 5×10 mm |

> Adapter la hauteur du corps (20mm par défaut) selon la hauteur réelle du bac fabriqué.

### Supports d'impression
Aucun si imprimé à la verticale (crochet en haut).

---

## 3. Boîtier capteurs — `boitier_capteurs.stl`

### Rôle
Regroupe les capteurs DHT22 et HC-SR04 dans un seul boîtier fixable sur le bord du bac ou du cadre.

### Principe

```
Vue de face :

    ┌──────────────────────────┐
    │  ┌──────┐   ┌──────────┐ │
    │  │DHT22 │   │ HC-SR04  │ │   ← Fenêtres capteurs (côte avant)
    │  └──────┘   └──────────┘ │
    │                          │
    │  Sortie câbles           │
    │     ↓                    │
    └──────────┬───────────────┘
               │ Patte de fixation intégrée (M3 ou clip)

Dimensions extérieures : 80 × 50 × 35 mm
```

### Cotes des ouvertures

| Capteur | Ouverture dans le boîtier |
|---|---|
| DHT22 | 15 × 20 mm (à l'air libre, pas sous verre) |
| HC-SR04 | 2× trous Ø18 mm espacés de 26 mm (capteur + émetteur) |
| Sortie câbles | Presse-étoupe PG7 (Ø12 mm côté arrière) |

### Dimensions extérieures

| Élément | Cote |
|---|---|
| Largeur | 80 mm |
| Hauteur | 50 mm |
| Profondeur | 35 mm |
| Épaisseur paroi | 2.5 mm |

### Supports d'impression
Aucun si orienté face avant vers le bas.

---

## Logiciels suggérés pour concevoir les STL

| Logiciel | Niveau | Lien |
|---|---|---|
| **FreeCAD** | Intermédiaire — paramétrique | freecad.org (gratuit) |
| **OpenSCAD** | Avancé — code paramétrique | openscad.org (gratuit) |
| **Fusion 360** | Professionnel | autodesk.com (gratuit usage personnel) |
| **Tinkercad** | Débutant — en ligne | tinkercad.com (gratuit) |

> Pour les débutants, **Tinkercad** permet de créer ces pièces simples en moins d'une heure directement dans le navigateur.
