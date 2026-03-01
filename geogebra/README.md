# Harmonographe des 4 Saisons — GeoGebra

Visualisation des courbes harmonographe correspondant aux **conditions environnementales typiques de chaque saison**, telles que notre machine les génèrerait via ses capteurs.

---

## Correspondance capteurs → saisons

| Saison | Temp. | Humidité | Distance | Son | Caractère |
|---|---|---|---|---|---|
| ❄️ **Hiver** | ~3°C | 70% | 50cm (vide) | très faible | Lissajous cristallin, géométrique |
| 🌸 **Printemps** | ~15°C | 60% | 30cm (quelqu'un passe) | modéré | Spirale dorée φ, en transition |
| ☀️ **Été** | ~30°C | 80% | 20cm (interactif) | fort | Dense, tourbillonnant, organique |
| 🍂 **Automne** | ~12°C | 75% | 35cm | léger | Mélancolique, quasi-ellipse tournante |

---

## Comment utiliser dans GeoGebra

### Option A — Coller les commandes (le plus simple)

1. Ouvrir [GeoGebra Classic](https://www.geogebra.org/classic) (navigateur ou application)
2. Cliquer sur le champ de saisie en bas
3. Coller **chaque ligne** de `commandes.txt` une par une et valider avec **Entrée**
4. Afficher une seule saison à la fois via la liste des objets (œil à côté du nom)

### Option B — Ouvrir le fichier .ggb directement

1. Exécuter le script Python :
   ```bash
   python3 generer_ggb.py
   ```
   → génère `saisons_harmonographe.ggb`
2. Ouvrir GeoGebra → **Fichier → Ouvrir** → sélectionner le `.ggb`

### Option C — Utiliser l'URL GeoGebra (partage en ligne)

Les commandes de `commandes.txt` peuvent être collées dans GeoGebra Online
et le fichier partagé via le bouton "Partager".

---

## Explication des équations

Chaque courbe suit le modèle **3 pendules virtuels** :

```
x(t) = R × [ A₁·sin(ω₁·t + φ₁) + A₂·sin(ω₂·t + φ₂) ]
y(t) = R × [ A₃·sin(ω₃·t + φ₃) ]

R = 185mm (rayon du bac)
```

### Paramètres par saison

#### ❄️ Hiver — Lissajous cristallin (ratio 1 : 1.503)
```
ω₁ = 1.000  (figure simple → ratio presque 2:3)
ω₂ = 1.503  (détuné de 0.003 → figure tourne lentement)
ω₃ = 1.000  (même fréquence que ω₁)
A₁ = 0.60, A₂ = 0.40, A₃ = 0.85
φ₁ = 0, φ₂ = π/4, φ₃ = π/6
```
→ Figure de Lissajous 2:3 qui tourne très lentement. Nette et cristalline.

#### 🌸 Printemps — Spirale dorée (φ = 1.618)
```
ω₁ = 1.618 = φ  (nombre d'or)
ω₂ = 1.500       (ratio complémentaire)
ω₃ = 1.333 = 4/3 (presque fermé)
A₁ = 0.65, A₂ = 0.35, A₃ = 0.92
φ₁ = π/6, φ₂ = π/3, φ₃ = π/8
```
→ Courbe ouverte et spiralée. La phase intermédiaire entre géométrique et organique.

#### ☀️ Été — Dense et tourbillonnant (√2 et φ²)
```
ω₁ = √2  ≈ 1.414  (irrationnel)
ω₂ = φ²  ≈ 2.618  (irrationnel, φ au carré)
ω₃ = √3  ≈ 1.732  (irrationnel)
A₁ = 0.75, A₂ = 0.25, A₃ = 0.95
φ₁ = π/4, φ₂ = π/2, φ₃ = π/5
```
→ Les 3 fréquences irrationnelles créent une courbe **jamais périodique**, très dense.

#### 🍂 Automne — Ellipse tournante mélancolique
```
ω₁ = 1.002  (quasi-cercle, tourne très lentement)
ω₂ = 1.618  (φ — enveloppe)
ω₃ = 1.334  (presque 4/3 mais détuné)
A₁ = 0.55, A₂ = 0.45, A₃ = 0.88
φ₁ = π/3, φ₂ = 2π/3, φ₃ = π/4
```
→ Quasi-ellipse qui se déforme lentement. Deux composantes presque égales = instabilité douce.
