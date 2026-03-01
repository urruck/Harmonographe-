# Conception Mécanique

## 1. Principe de fonctionnement Sisyphus

La machine Sisyphus originale fait rouler une bille d'acier sur du sable en utilisant un **aimant permanent se déplaçant sous un plateau fin**. L'attraction magnétique à travers le plateau attire la bille qui suit l'aimant, traçant des motifs dans le sable.

```
Vue de côté :

  ████████████████████████████   ← Bac à sable (acrylique 3mm)
  ~  ~  ●  ~  ~  ~  ~  ~  ~  ~  ← Sable (3mm profondeur) + Bille ●
  ────────────────────────────   ← Fond du bac
           ↑ 5mm d'air
        [N  S]                   ← Aimant néodyme N52 Ø50mm
           │
    ┌──────┴──────┐
    │  Support    │              ← Monté sur le chariot Shapeoko
    │  aimant     │
    └─────────────┘
           │
    ══════════════               ← Rails Shapeoko X/Y
```

---

## 2. Modification de la Shapeoko

### 2.1 Démontage de la broche

Retirer la broche Makita/Dewalt et le support d'outil. Conserver le **chariot Z** et sa platine de montage.

### 2.2 Support d'aimant

Fabriquer un support aimant en **PETG ou ABS** (pas de PLA — trop fragile à la chaleur) :

```
Support aimant — Vue isométrique :

    ┌──────────────────┐    ← Plaque de fixation 80×80mm
    │  ●  ●        ●  ●│    ← Trous M6 pour fixation Shapeoko
    │                  │
    │    ┌────────┐    │    ← Cage aimant Ø55mm × 25mm profond
    │    │        │    │
    │    │ AIMANT │    │    ← Aimant N52 Ø50mm × 20mm
    │    │        │    │
    │    └────────┘    │
    └──────────────────┘
         Hauteur totale : 60mm sous la platine Z
```

**Fichiers STL** : Voir `assets/README.md` pour les cotes de conception. Les STL sont à modéliser (FreeCAD, Fusion 360, Tinkercad) puis imprimer en PETG, 40% remplissage, 3 périmètres.

### 2.3 Hauteur critique (Z)

La hauteur Z doit être réglée précisément :

| Paramètre | Valeur |
|---|---|
| Épaisseur bac acrylique | 3 mm |
| Profondeur de sable | 3 mm |
| Gap air aimant-bac | 4-6 mm |
| **Distance totale aimant-bille** | **10-12 mm** |

> **⚠️ Important** : Trop proche = la bille reste collée et griffe le sable. Trop loin = la bille ne suit plus. Tester par incrément de 1mm. La distance idéale dépend de la force de l'aimant choisi.

---

## 3. Le bac à sable

### 3.1 Dimensions

Pour une Shapeoko 2 (zone de travail 300×300mm) :

```
Bac à sable — Vue de dessus :

┌──────────────────────┐
│  ┌────────────────┐  │  ← Bac extérieur : 330×330mm
│  │                │  │
│  │  Zone active   │  │  ← Zone active : 250×250mm
│  │  250 × 250 mm  │  │
│  │                │  │
│  └────────────────┘  │
└──────────────────────┘

Coupe :
┌─┬─────────────────────────┬─┐
│ │  Sable fin (3mm)        │ │  ← Hauteur bac : 15mm
│ │_________________________│ │
│ │  Acrylique 3mm          │ │  ← Fond transparent
└─┴─────────────────────────┴─┘
  ← Cadre aluminium 15×15×2mm →
```

### 3.2 Matériaux du bac

| Pièce | Matière | Dimensions | Quantité |
|---|---|---|---|
| Fond | Acrylique transparent 3mm | 250×250mm | 1 |
| Côtés | Aluminium profilé 15×15mm | 250mm | 4 |
| Vis d'assemblage | M3×10 inox | — | 16 |
| Joint d'étanchéité | Silicone noir | 4×250mm | 4 |

### 3.3 Type de sable

| Type | Avantage | Inconvénient |
|---|---|---|
| **Sable de plage fin (recommandé)** | Motifs beaux, bonne viscosité | Rincer et sécher |
| Sable de quartz Ø0.1mm | Très fin, motifs précis | Plus cher |
| Sable de Loire | Naturel, bon rendu | Qualité variable |
| Sable coloré (déco) | Effets visuels | Moins fluide |

> **Préparation** : Tamiser au 0.5mm, rincer, sécher 24h à 80°C au four.

---

## 4. La bille

| Paramètre | Valeur recommandée |
|---|---|
| Diamètre | 19mm (3/4") — bon compromis |
| Matière | Acier chromé (roulement) |
| Masse | ~28g |
| Référence | SKF ou NSK — bille de roulement 3/4" |

> La bille doit être **magnétique** (acier, pas inox A4). Les billes en inox 316L ne sont pas magnétiques !

---

## 5. Montage sur la Shapeoko

### 5.1 Procédure d'assemblage

```
Étape 1 : Retirer la broche et son support
Étape 2 : Imprimer ou usiner le support aimant
Étape 3 : Fixer le support sur la platine Z avec 4× M6×20
Étape 4 : Insérer l'aimant N52 dans le logement
           (⚠️ Attention aux doigts — force ~5kg)
Étape 5 : Bloquer l'aimant avec 2× vis M3 latérales
Étape 6 : Placer le bac à sable sur le plateau Shapeoko
           (centré, fixé avec 4 pattes d'arrêt imprimées)
Étape 7 : Régler le Z : descendre lentement jusqu'à ce que
           la bille commence à suivre l'aimant
Étape 8 : Fixer la hauteur Z et régler le home machine
```

### 5.2 Réglage de la distance aimant-plateau

```
Test de calibration :
1. Placer la bille au centre du bac
2. Descendre Z par incrément de 0.5mm
3. Tester si la bille suit un mouvement X lent (F200 mm/min)
4. La distance optimale est trouvée quand :
   - La bille suit sans saccade
   - Elle peut tourner en cercle Ø50mm sans se décrocher
```

---

## 6. Éclairage (optionnel mais très esthétique)

Placer des **LED RGB** sous le bac pour un effet lumineux :

```
Vue de côté avec éclairage :

  ~~~~~~●~~~~~~   ← Bac à sable (vue de dessus, éclairé)
  ─────────────   ← Fond acrylique TRANSPARENT
  ═ LED RGB ═     ← Bande LED WS2812B sur pourtour
      ↓
  [diffusion lumière vers le haut]
```

- **Bande LED** : WS2812B 60 LED/m, 1.0m total (pourtour 250mm × 4)
- **Contrôle** : Pin numérique Arduino (bibliothèque FastLED)
- **Effets** : Heatmap liée aux capteurs (bleu = froid, rouge = chaud)
- **Alimentation** : 5V 3A séparé

---

## 7. Structure principale (cadre)

Pour surélever le bac et laisser la Shapeoko dessous :

```
Vue de face — Structure finale :

    ┌────────────────────────────────┐
    │      BAC À SABLE               │  ← H = 800mm du sol
    │   (avec éclairage LED)         │
    └─────────────┬──────────────────┘
                  │ Pieds 30×30mm aluminium
    ┌─────────────┴──────────────────┐
    │         SHAPEOKO CNC           │  ← H = 500mm du sol
    │  (moteurs + aimant en dessous) │
    └────────────────────────────────┘
                  │
              [Boîtier
              électronique]           ← H = 100mm du sol
```

**Profilés aluminium 30×30 fente 6mm (style 2020)** :
- 4× pieds de 300mm
- 4× traverses de 500mm
- Assemblage avec équerre d'angle et vis M6

---

## 8. Vitesse de déplacement recommandée

| Phase | Vitesse (mm/min) | Effet sur la bille |
|---|---|---|
| Dessin fin | 200-500 | Sillons nets et précis |
| Courbes organiques | 500-1500 | Fluidité naturelle |
| Transitions | 1500-3000 | Rapidité entre motifs |
| Maximum mécanique | 5000 | (risque de perdre la bille) |

> **Note GRBL** : Toujours utiliser des accélérations douces (GRBL `$120` et `$121` = 100-200 mm/s²) pour éviter de décrocher la bille.
