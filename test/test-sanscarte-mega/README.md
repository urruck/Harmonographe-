# Test sans carte Mega — Simulateur Python

Simule l'algorithme harmonographe **entièrement en logiciel**, sans aucun matériel.
Résultat : résumé texte dans le terminal + graphique matplotlib + export G-code optionnel.

## Prérequis

```bash
pip install matplotlib
```

(Sans `matplotlib` : seul l'aperçu ASCII et le G-code fonctionnent.)

## Utilisation

### Lancement rapide (valeurs par défaut)
```bash
python harmo_sim.py
```

### Avec capteurs simulés
```bash
python harmo_sim.py --temp 0.8 --hum 0.3 --dist 0.5 --snd 0.2
```

### G-code seul, sans graphique
```bash
python harmo_sim.py --temp 0.6 --hum 0.7 --no-plot --gcode courbe.nc
```

### Aperçu ASCII dans le terminal (sans matplotlib)
```bash
python harmo_sim.py --ascii --no-plot
```

### Augmenter le nombre de points
```bash
python harmo_sim.py --points 5000
```

## Paramètres

| Argument  | Plage   | Effet sur la courbe                              |
|-----------|---------|--------------------------------------------------|
| `--temp`  | 0.0-1.0 | Fréquence principale (simple géo. → dense organique) |
| `--hum`   | 0.0-1.0 | Fréquence secondaire (compact → ouvert)          |
| `--dist`  | 0.0-1.0 | Phase (orientation / rotation de la figure)      |
| `--snd`   | 0.0-1.0 | Asymétrie + perturbation organique               |
| `--points`| entier  | Nombre de points (= durée simulée × 17 pts/s)    |
| `--gcode` | fichier | Exporter le G-code compatible GRBL/Shapeoko 2    |

## Sortie console (exemple)

```
┌────────────────────────────────────────────────────┐
│          HARMONOGRAPHE — Simulation                │
├────────────────────────────────────────────────────┤
│  Capteurs simulés :                                │
│    Température : 0.50  (0=froid, 1=chaud)          │
│    Humidité    : 0.50  (0=sec,   1=humide)         │
│    Distance    : 0.80  (0=proche, 1=loin)          │
│    Son         : 0.00  (0=silence, 1=fort)         │
├────────────────────────────────────────────────────┤
│  Paramètres calculés :                             │
│    ω1 = 0.9058 rad/s   (pendule X principal)       │
│    ω2 = 1.3580 rad/s   (pendule X secondaire)      │
│    ω3 = 0.6277 rad/s   (pendule Y)                 │
│    A1 = 0.55  A2 = 0.45  A3 = 0.80                │
│    Vitesse : 936 mm/min                            │
└────────────────────────────────────────────────────┘
```

## G-code généré (extrait)

```gcode
; Harmonographe — simulation Python
G90 G21 G17
M5
G0 X150.000 Y222.834  ; Aller au départ
G1 X152.340 Y219.410 F936
G1 X154.621 Y215.833 F936
...
G1 X150.000 Y150.000 F936  ; Retour centre
M2
```
