# Algorithme Harmonographe — Théorie et Implémentation

## 1. Qu'est-ce qu'un harmonographe ?

Un harmonographe physique est un dispositif à pendules couplés qui trace des courbes sur du papier. L'interaction entre plusieurs oscillateurs sinusoïdaux crée des **figures de Lissajous généralisées**, d'une beauté mathématique remarquable.

```
Harmonographe classique à 3 pendules :

    Pendule 1 (axe X, fréq. f₁)     Pendule 2 (axe X, fréq. f₂)
         │                                    │
         │ sin(f₁t + φ₁)                     │ sin(f₂t + φ₂)
         │                                    │
         └──────────────┬─────────────────────┘
                        │ somme pondérée
                        ▼
                  Position X(t)

    Pendule 3 (axe Y, fréq. f₃)
         │
         │ sin(f₃t + φ₃)
         ▼
    Position Y(t)
```

---

## 2. Équations mathématiques

### 2.1 Harmonographe classique (avec amortissement)

```
X(t) = A₁·sin(f₁·t + φ₁)·e^(-d₁·t) + A₂·sin(f₂·t + φ₂)·e^(-d₂·t)
Y(t) = A₃·sin(f₃·t + φ₃)·e^(-d₃·t)
```

L'amortissement `e^(-d·t)` fait mourir l'oscillation → la courbe spirale vers le centre.

### 2.2 Harmonographe "Sisyphus" (sans amortissement, continu)

Pour une machine de sable, on veut un mouvement **perpétuel et non-répétitif** :

```
X(t) = R · [ A₁·sin(ω₁·t + φ₁) + A₂·sin(ω₂·t + φ₂) ]
Y(t) = R · [ A₃·sin(ω₃·t + φ₃) ]
```

Où :
- `R` = rayon du bac (200mm ici)
- `A₁ + A₂ = 1` (amplitudes normalisées)
- `ω₁, ω₂, ω₃` = fréquences angulaires des 3 pendules virtuels
- `φ₁, φ₂, φ₃` = phases initiales

### 2.3 Condition de non-répétition (clé de la beauté organique)

La courbe ne se répète **jamais exactement** si les ratios de fréquences sont **irrationnels** :

```
ω₁/ω₂ ∉ ℚ  (ratio irrationnel)
ω₁/ω₃ ∉ ℚ
```

**Exemple de ratios beaux :**
| Ratio | Valeur | Type de courbe |
|---|---|---|
| ω₁/ω₂ = √2 | 1.41421... | Courbe quasi-périodique dense |
| ω₁/ω₂ = φ | 1.61803... | Spirale dorée (Fibonacci) |
| ω₁/ω₂ = π/2 | 1.57079... | Presque carré, remplissage lent |
| ω₁/ω₂ = 3/2 + ε | 1.50023... | Lissajous légèrement détuné |

**La magie :** Un ratio presque rationnel (ex: 3/2 + 0.002) crée une figure de Lissajous qui **tourne lentement** → motif vivant et organique.

---

## 3. Le modèle à 3 pendules virtuels

### 3.1 Choix des fréquences

```
Pendule 1 : ω₁ = ω_base × n₁   (modulé par la température)
Pendule 2 : ω₂ = ω_base × n₂   (modulé par l'humidité)
Pendule 3 : ω₃ = ω_base × n₃   (couplé à n₁ et n₂)

ω_base = 2π / T_base   avec T_base = 10 secondes (période de référence)
```

### 3.2 Tableau des fréquences et figures

```
n₁ = 1.0, n₂ = 1.5 :   Figure de Lissajous 2:3  → 8 couché
n₁ = 1.0, n₂ = 2.0 :   Figure 1:2               → Parabole
n₁ = 2.0, n₂ = 3.0 :   Figure 2:3               → Nœud de trèfle
n₁ = φ,   n₂ = φ²  :   Spirale dorée             → Nautile
n₁ = √2,  n₂ = √3  :   Irrationnel total         → Remplissage dense
```

### 3.3 Modulation par les capteurs

```
TEMPÉRATURE (15-35°C) → n₁ mapping :
  15°C → n₁ = 1.0  (figure simple, Lissajous géométrique)
  25°C → n₁ = φ    (spirale dorée, organique)
  35°C → n₁ = √2   (quasi-chaotique, très dense)

HUMIDITÉ (30-90%) → n₂ mapping :
  30% → n₂ = 1.5   (figure compacte)
  60% → n₂ = φ²    (spirale élargie)
  90% → n₂ = √3    (remplissage vertical)

SON (0-100 dB) → perturbation δ :
  Silence → δ = 0         (courbe pure)
  Voix    → δ = 0.05      (légère déformation organique)
  Musique → δ = 0.15      (ondes perturbées)

DISTANCE (5-50 cm) → phase φ₁ :
  Loin (50cm) → φ₁ = 0         (orientation standard)
  Proche (5cm) → φ₁ = 2π/3     (figure pivotée de 120°)
```

---

## 4. Algorithme de génération de points

### 4.1 Pseudo-code

```
INITIALISATION :
  t ← 0
  ω_base ← 2π / 10.0   // une période = 10 secondes
  dt ← 0.05             // pas de temps (en secondes)
  R ← 190.0             // rayon max en mm (bac 400×400, marge)

BOUCLE PRINCIPALE :
  RÉPÉTER :
    // 1. Lire les capteurs
    temp     ← lire_DHT22_temperature()
    humidity ← lire_DHT22_humidite()
    distance ← lire_HC_SR04()
    sound    ← lire_MAX9814()

    // 2. Calculer les paramètres de courbe
    params ← calculer_params(temp, humidity, distance, sound)

    // 3. Générer le prochain point
    x ← R × (params.A1 × sin(params.ω1 × t + params.φ1)
            + params.A2 × sin(params.ω2 × t + params.φ2))
    y ← R × (params.A3 × sin(params.ω3 × t + params.φ3))

    // 4. Envoyer G-code au GRBL
    envoyer_gcode("G1 X%.3f Y%.3f F%.0f\n", x, y, params.vitesse)

    // 5. Avancer le temps
    t ← t + dt

    // 6. Transition douce si paramètres changent beaucoup
    SI |params_nouveaux - params_anciens| > SEUIL :
      interpoler_parametres(params_anciens, params_nouveaux, 200 pas)
```

### 4.2 Interpolation douce des paramètres

Pour éviter les mouvements brusques quand les capteurs changent soudainement :

```
FONCTION interpoler(P_ancien, P_nouveau, N_pas) :
  POUR i DE 1 À N_pas :
    α ← smoothstep(i / N_pas)     // 0 → 1 avec accélération douce
    P_interp ← P_ancien × (1-α) + P_nouveau × α
    t_interp ← t_courant + i × dt
    générer_point(t_interp, P_interp)

FONCTION smoothstep(x) :
  RETOURNER x × x × (3 - 2×x)   // Courbe en S douce
```

### 4.3 Gestion des bords du bac

Si la bille atteint le bord (|x| > R ou |y| > R), ramener doucement au centre :

```
FONCTION verifier_bords(x, y, R) :
  r ← sqrt(x×x + y×y)    // Distance au centre
  SI r > R × 0.95 :        // 5% de marge
    // Spirale de retour vers le centre
    POUR i DE 0 À 50 :
      x_retour ← x × (1 - i/50) × 0.9
      y_retour ← y × (1 - i/50) × 0.9
      envoyer_gcode(x_retour, y_retour, vitesse_lente)
```

---

## 5. Patterns spéciaux

### 5.1 Rose de Rhodonée (quand son fort détecté)

```
// Courbe rose : r = cos(k·θ)
// En paramétrique :
x(θ) = R × cos(k·θ) × cos(θ)
y(θ) = R × cos(k·θ) × sin(θ)

k = 3 → trèfle à 3 pétales
k = 5 → étoile à 5 pétales
k = φ → rose spirale infinie
```

### 5.2 Spirale d'Archimède (transition entre patterns)

```
// Spirale de l'intérieur vers l'extérieur :
r(θ) = a × θ
x(θ) = r(θ) × cos(θ)
y(θ) = r(θ) × sin(θ)

θ : 0 → 4π (2 tours)
a = R / (4π)  // pour atteindre le bord en 2 tours
```

### 5.3 Pattern "son ambiant" (perturbation Perlin)

Quand le son dépasse un seuil, ajouter une perturbation organique :

```
δx(t) = sound_norm × A_perturb × sin(7.3 × t + φ_son)
δy(t) = sound_norm × A_perturb × sin(5.7 × t + φ_son + π/3)

x_final = x_harmonographe + δx
y_final = y_harmonographe + δy
```

---

## 6. Paramètres de qualité des courbes

### 6.1 Critères d'une belle courbe de sable

| Critère | Valeur idéale | Explication |
|---|---|---|
| Vitesse min. | 200 mm/min | En dessous : la bille hésite |
| Vitesse max. | 3000 mm/min | Au-delà : la bille se décroche |
| Accélération max. | 300 mm/s² | Changement de direction doux |
| Rayon min. courbe | 5mm | Virage serré possible |
| Pas de temps dt | 0.05-0.1s | Résolution de la courbe |
| Nombre de points | 5000-50000 | Pour un motif complet |

### 6.2 Vitesse adaptative

La vitesse peut être calculée en fonction du rayon de courbure local :

```
// Rayon de courbure en un point (t) :
κ(t) = |x'·y'' - y'·x''| / (x'² + y'²)^(3/2)
ρ(t) = 1 / κ(t)  // Rayon de courbure

// Vitesse en fonction du rayon :
vitesse = clamp(ρ × vitesse_angulaire_max, V_min, V_max)
```

---

## 7. Visualisation des patterns attendus

```
Température basse (15°C) + Humidité faible (30%) :

     ┌─────────────────────────┐
     │         / \             │  Figure de Lissajous 2:3
     │        /   \            │  Géométrique, symétrique
     │   ____/     \____       │
     │  (              )       │
     │   ────\     /────       │
     │        \   /            │
     │         \ /             │
     └─────────────────────────┘

Température haute (30°C) + Humidité haute (80%) :

     ┌─────────────────────────┐
     │  ·°·°·°·°·°·°·°·°·°·° │  Courbe dense et organique
     │ °·°·°·°·°·°·°·°·°·°·° │  Presque fractale
     │·°·°·°·°·°·°·°·°·°·°·°·│  Spirale de Fibonacci visible
     │ °·°·°·°·°·°·°·°·°·°·° │
     │  ·°·°·°·°·°·°·°·°·°·° │
     └─────────────────────────┘

Avec perturbation sonore forte :

     ┌─────────────────────────┐
     │   ~∿∿~∿~∿∿∿~∿~∿∿~∿~  │  Courbe ondulée
     │  ∿~∿~∿∿~∿~∿∿~∿~∿~∿∿~  │  Perturbations organiques
     │ ~∿∿~∿~∿∿∿~∿~∿∿~∿~∿∿~  │  Comme une respiration
     └─────────────────────────┘
```

---

## 8. Calibration de l'algorithme

### 8.1 Séquence de calibration initiale

```
ÉTAPE 1 — Test de connectivité GRBL :
  Envoyer "$\n" → doit répondre les paramètres GRBL
  Envoyer "?\n" → doit répondre la position

ÉTAPE 2 — Homing :
  Envoyer "$H\n" → cherche les fins de course
  Attendre "ok\n"

ÉTAPE 3 — Aller au centre :
  Envoyer "G90 G21 G1 X200 Y200 F1000\n" → centre du bac 400×400
  Vérifier que la bille suit

ÉTAPE 4 — Test cercle :
  Envoyer cercle G2 R50 → vérifier la fluidité
  Ajuster la hauteur Z si la bille ne suit pas

ÉTAPE 5 — Test harmonographe minimal :
  Générer 100 points, vitesse 500 mm/min
  Vérifier les courbes dans le sable
```

### 8.2 Paramètres GRBL recommandés

```
$0=10      ; Step pulse, 10 μs
$1=255     ; Step idle delay, toujours actif
$2=0       ; Step port invert
$3=0       ; Direction port invert (ajuster selon câblage)
$4=0       ; Step enable invert
$5=0       ; Limit pins invert
$6=0       ; Probe pin invert
$10=1      ; Status report, position machine
$11=0.010  ; Junction deviation, 0.01mm
$12=0.002  ; Arc tolerance, 0.002mm
$13=0      ; Report inches, non
$20=0      ; Soft limits, désactivé
$21=0      ; Hard limits, désactivé (pas de fins de course)
$22=1      ; Homing cycle, activé
$23=0      ; Homing direction invert
$24=25     ; Homing feed, 25 mm/min
$25=500    ; Homing seek, 500 mm/min
$26=250    ; Homing debounce, 250 ms
$27=1.0    ; Homing pull-off, 1mm
$30=1000   ; Max spindle speed (non utilisé)
$31=0      ; Min spindle speed
$32=0      ; Laser mode, désactivé
$100=40    ; X steps/mm (à calibrer selon Shapeoko)
$101=40    ; Y steps/mm
$102=400   ; Z steps/mm (non utilisé)
$110=5000  ; X max rate, 5000 mm/min
$111=5000  ; Y max rate
$112=500   ; Z max rate
$120=200   ; X acceleration, 200 mm/s² (doux pour la bille)
$121=200   ; Y acceleration
$122=10    ; Z acceleration
$130=400   ; X max travel, 400mm
$131=400   ; Y max travel
$132=50    ; Z max travel
```
