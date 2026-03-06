#!/usr/bin/env python3
"""
╔══════════════════════════════════════════════════════════════════════╗
║        HARMONOGRAPHE — Simulateur texte / sans carte Mega           ║
║                                                                      ║
║  Simule l'algorithme de courbes sans aucun matériel.                 ║
║  Génère un fichier G-code et affiche la trajectoire (matplotlib).    ║
╚══════════════════════════════════════════════════════════════════════╝

Utilisation :
    python harmo_sim.py                        # valeurs par défaut
    python harmo_sim.py --temp 0.8 --hum 0.3   # capteurs simulés
    python harmo_sim.py --no-plot --gcode out.nc  # G-code seul

Dépendances :
    pip install matplotlib
"""

import argparse
import math
import sys

# ─── Constantes (identiques au firmware Arduino) ──────────────────────────
MACHINE_RADIUS_MM = 120.0
WORK_OFFSET_X     = 150.0   # Centre de la Shapeoko 2 (300×300mm)
WORK_OFFSET_Y     = 150.0
DT_DEFAULT        = 0.06    # Pas de temps en secondes
TWO_PI            = 2 * math.pi
OMEGA_BASE        = TWO_PI / 10.0  # Période de base = 10 s
PHI               = 1.6180339887
SQRT2             = math.sqrt(2)
SQRT3             = math.sqrt(3)

FREQ_TABLE_1 = [
    1.000, 1.002, PHI, 1.502, SQRT2, 2.004, PHI * PHI, SQRT3
]
FREQ_TABLE_2 = [
    1.5, PHI, 2.0, SQRT2 + 0.003, 2.503, PHI * PHI, 3.001, SQRT3 + 0.005
]
FREQ_TABLE_Y = [
    0.999, 1.333, PHI - 0.001, SQRT2 - 0.002,
    2.0 - 0.003, 2.5 + 0.001, SQRT3 - 0.001, 1.618 + 0.004
]

# ─── Fonctions utilitaires ────────────────────────────────────────────────

def smoothstep3(x):
    x = max(0.0, min(1.0, x))
    return x * x * x * (x * (x * 6.0 - 15.0) + 10.0)

def interp_table(table, norm_value):
    """Interpolation linéaire dans une table de fréquences."""
    n    = len(table) - 1
    idx  = norm_value * n
    i0   = int(idx)
    i1   = min(i0 + 1, n)
    frac = idx - i0
    return table[i0] * (1.0 - frac) + table[i1] * frac

# ─── Calcul des paramètres depuis les capteurs ───────────────────────────

def params_from_sensors(temp, hum, dist, snd):
    """
    Convertit les capteurs simulés (0.0-1.0) en paramètres de courbe.
    Même logique que curves_from_sensors() dans le firmware Arduino.
    """
    omega1 = interp_table(FREQ_TABLE_1, temp) * OMEGA_BASE
    omega2 = interp_table(FREQ_TABLE_2, hum)  * OMEGA_BASE
    y_idx  = (temp + hum) / 2.0
    omega3 = interp_table(FREQ_TABLE_Y, y_idx) * OMEGA_BASE

    dist_norm = 1.0 - dist
    phi1 = dist_norm * TWO_PI * 0.333
    phi2 = dist_norm * TWO_PI * 0.500 + 0.78
    phi3 = dist_norm * TWO_PI * 0.167

    amp1 = max(0.55 + snd * 0.30, 0.10)
    amp2 = max(1.0 - amp1, 0.10)
    amp3 = 0.80 + snd * 0.18

    sound_perturb = snd * 0.20
    perturb_freq1 = 7.391 * OMEGA_BASE
    perturb_freq2 = 5.742 * OMEGA_BASE

    feedrate = max(200.0, min(2500.0, (600.0 + temp * 800.0) * (0.5 + dist * 0.8)))

    return {
        "omega1": omega1, "omega2": omega2, "omega3": omega3,
        "phi1": phi1, "phi2": phi2, "phi3": phi3,
        "amp1": amp1, "amp2": amp2, "amp3": amp3,
        "sound_perturb": sound_perturb,
        "perturb_freq1": perturb_freq1, "perturb_freq2": perturb_freq2,
        "feedrate": feedrate,
    }

# ─── Calcul d'un point de la courbe ──────────────────────────────────────

def get_point(p, t):
    """
    Calcule (x, y) en mm depuis le centre pour le temps t.
    Formule : x = R·[A1·sin(ω1·t+φ1) + A2·sin(ω2·t+φ2)]
              y = R·[A3·sin(ω3·t+φ3)]
    """
    raw_x = (p["amp1"] * math.sin(p["omega1"] * t + p["phi1"])
           + p["amp2"] * math.sin(p["omega2"] * t + p["phi2"]))
    raw_y =  p["amp3"] * math.sin(p["omega3"] * t + p["phi3"])

    if p["sound_perturb"] > 0.001:
        dx = (p["sound_perturb"]
              * math.sin(p["perturb_freq1"] * t + 1.234)
              * math.sin(p["perturb_freq2"] * t * 0.7))
        dy = (p["sound_perturb"]
              * math.sin(p["perturb_freq2"] * t + 2.718)
              * math.sin(p["perturb_freq1"] * t * 0.6))
        raw_x += dx
        raw_y += dy

    x = raw_x * MACHINE_RADIUS_MM
    y = raw_y * MACHINE_RADIUS_MM

    r = math.sqrt(x * x + y * y)
    if r > MACHINE_RADIUS_MM * 0.97:
        scale = (MACHINE_RADIUS_MM * 0.97) / r
        x *= scale
        y *= scale

    return x, y

# ─── Génération de la trajectoire ────────────────────────────────────────

def generate_points(p, n_points, dt=DT_DEFAULT):
    """Génère n_points points de courbe et retourne deux listes xs, ys."""
    xs, ys = [], []
    t = 0.0
    for _ in range(n_points):
        x, y = get_point(p, t)
        xs.append(x)
        ys.append(y)
        t += dt
    return xs, ys

# ─── Export G-code ────────────────────────────────────────────────────────

def export_gcode(xs, ys, feedrate, filename):
    """
    Génère un fichier G-code compatible GRBL / Shapeoko 2.
    Coordonnées machine : centre_bac + (x, y) en mm.
    """
    with open(filename, "w") as f:
        f.write("; Harmonographe — simulation Python\n")
        f.write(f"; Points : {len(xs)}\n")
        f.write(f"; Zone de travail : 300x300mm, centre = ({WORK_OFFSET_X},{WORK_OFFSET_Y})\n")
        f.write(";\n")
        f.write("G90 G21 G17  ; Absolu, mm, plan XY\n")
        f.write("M5           ; Broche off\n")

        # Déplacement rapide vers le premier point
        x0 = max(5.0, min(295.0, xs[0] + WORK_OFFSET_X))
        y0 = max(5.0, min(295.0, ys[0] + WORK_OFFSET_Y))
        f.write(f"G0 X{x0:.3f} Y{y0:.3f}  ; Aller au départ\n")

        # Tracé
        for x_mm, y_mm in zip(xs, ys):
            mx = max(5.0, min(295.0, x_mm + WORK_OFFSET_X))
            my = max(5.0, min(295.0, y_mm + WORK_OFFSET_Y))
            f.write(f"G1 X{mx:.3f} Y{my:.3f} F{feedrate:.0f}\n")

        # Retour centre
        f.write(f"G1 X{WORK_OFFSET_X:.3f} Y{WORK_OFFSET_Y:.3f} F{feedrate:.0f}  ; Retour centre\n")
        f.write("M2  ; Fin programme\n")

    print(f"G-code exporté : {filename}  ({len(xs)} lignes)")

# ─── Affichage console ────────────────────────────────────────────────────

def print_summary(temp, hum, dist, snd, p, n_points):
    print()
    print("┌────────────────────────────────────────────────────┐")
    print("│          HARMONOGRAPHE — Simulation                │")
    print("├────────────────────────────────────────────────────┤")
    print(f"│  Capteurs simulés :                                │")
    print(f"│    Température : {temp:.2f}  (0=froid, 1=chaud)        │")
    print(f"│    Humidité    : {hum:.2f}  (0=sec,   1=humide)      │")
    print(f"│    Distance    : {dist:.2f}  (0=proche, 1=loin)       │")
    print(f"│    Son         : {snd:.2f}  (0=silence, 1=fort)      │")
    print("├────────────────────────────────────────────────────┤")
    print(f"│  Paramètres calculés :                             │")
    print(f"│    ω1 = {p['omega1']:.4f} rad/s   (pendule X principal)  │")
    print(f"│    ω2 = {p['omega2']:.4f} rad/s   (pendule X secondaire) │")
    print(f"│    ω3 = {p['omega3']:.4f} rad/s   (pendule Y)            │")
    print(f"│    φ1 = {math.degrees(p['phi1']):.1f}°  φ2 = {math.degrees(p['phi2']):.1f}°  φ3 = {math.degrees(p['phi3']):.1f}°         │")
    print(f"│    A1 = {p['amp1']:.2f}  A2 = {p['amp2']:.2f}  A3 = {p['amp3']:.2f}           │")
    print(f"│    Perturbation son : {p['sound_perturb']:.3f}                    │")
    print(f"│    Vitesse : {p['feedrate']:.0f} mm/min                    │")
    print("├────────────────────────────────────────────────────┤")
    print(f"│  Points générés : {n_points:<5}                          │")
    print(f"│  Durée simulée  : {n_points * DT_DEFAULT:.1f} s                       │")
    print("└────────────────────────────────────────────────────┘")
    print()

# ─── Affichage ASCII (aperçu terminal, sans matplotlib) ──────────────────

def print_ascii_preview(xs, ys, width=60, height=24):
    """Rendu ASCII grossier de la courbe dans le terminal."""
    grid = [[" "] * width for _ in range(height)]

    for x, y in zip(xs, ys):
        col = int((x / MACHINE_RADIUS_MM + 1.0) / 2.0 * (width  - 1))
        row = int((1.0 - (y / MACHINE_RADIUS_MM + 1.0) / 2.0) * (height - 1))
        col = max(0, min(width  - 1, col))
        row = max(0, min(height - 1, row))
        grid[row][col] = "·"

    # Marquer le centre
    cx = width  // 2
    cy = height // 2
    grid[cy][cx] = "+"

    print("  ┌" + "─" * width + "┐")
    for row in grid:
        print("  │" + "".join(row) + "│")
    print("  └" + "─" * width + "┘")
    print(f"  Zone ±{MACHINE_RADIUS_MM:.0f}mm  (+= centre)")
    print()

# ─── Affichage graphique (matplotlib) ────────────────────────────────────

def show_plot(xs, ys, temp, hum, dist, snd):
    try:
        import matplotlib.pyplot as plt
        import matplotlib.patches as patches
        import numpy as np
    except ImportError:
        print("matplotlib non installé — aperçu ASCII uniquement.")
        print("  pip install matplotlib")
        return

    fig, ax = plt.subplots(figsize=(7, 7))
    fig.patch.set_facecolor("#14161e")
    ax.set_facecolor("#161920")

    # Cercles de référence
    for r in [50, 100, 120]:
        circle = plt.Circle((0, 0), r, color="#2d3040", fill=False, linewidth=0.8)
        ax.add_patch(circle)
        ax.text(r * 0.707, -r * 0.707, f"{r}mm",
                color="#505568", fontsize=7, ha="center")

    # Axes
    ax.axhline(0, color="#2d3040", linewidth=0.8)
    ax.axvline(0, color="#2d3040", linewidth=0.8)

    # Courbe (dégradé de couleur selon le temps)
    n = len(xs)
    for i in range(1, n):
        alpha = 0.15 + 0.85 * (i / n)
        ax.plot(xs[i-1:i+1], ys[i-1:i+1],
                color=(0.31, 0.63, 1.0, alpha), linewidth=0.8)

    # Point de départ / arrivée
    ax.plot(xs[0],  ys[0],  "o", color="#3cdc64", markersize=5, label="départ")
    ax.plot(xs[-1], ys[-1], "o", color="#ff8c3c", markersize=5, label="arrivée")

    ax.set_xlim(-150, 150)
    ax.set_ylim(-150, 150)
    ax.set_aspect("equal")
    ax.set_xlabel("X (mm)", color="#6e7388")
    ax.set_ylabel("Y (mm)", color="#6e7388")
    ax.tick_params(colors="#6e7388")
    for spine in ax.spines.values():
        spine.set_edgecolor("#2d3040")

    title = (f"Harmonographe  |  T={temp:.2f}  H={hum:.2f}  "
             f"D={dist:.2f}  S={snd:.2f}  |  {n} pts")
    ax.set_title(title, color="#dce1f0", fontsize=10)
    ax.legend(facecolor="#14161e", labelcolor="#dce1f0", fontsize=8)

    plt.tight_layout()
    plt.show()

# ─── main() ──────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(
        description="Simulateur harmonographe — sans carte Mega")
    parser.add_argument("--temp",    type=float, default=0.5,
                        help="Température simulée [0.0-1.0] (défaut 0.5)")
    parser.add_argument("--hum",     type=float, default=0.5,
                        help="Humidité simulée [0.0-1.0] (défaut 0.5)")
    parser.add_argument("--dist",    type=float, default=0.8,
                        help="Distance simulée [0.0-1.0] (défaut 0.8)")
    parser.add_argument("--snd",     type=float, default=0.0,
                        help="Son simulé [0.0-1.0] (défaut 0.0)")
    parser.add_argument("--points",  type=int,   default=2000,
                        help="Nombre de points à générer (défaut 2000)")
    parser.add_argument("--gcode",   type=str,   default=None,
                        help="Fichier G-code de sortie (ex: courbe.nc)")
    parser.add_argument("--no-plot", action="store_true",
                        help="Désactiver l'affichage graphique")
    parser.add_argument("--ascii",   action="store_true",
                        help="Forcer l'aperçu ASCII dans le terminal")
    args = parser.parse_args()

    # Clamp des valeurs
    temp = max(0.0, min(1.0, args.temp))
    hum  = max(0.0, min(1.0, args.hum))
    dist = max(0.0, min(1.0, args.dist))
    snd  = max(0.0, min(1.0, args.snd))

    # Calcul des paramètres
    p = params_from_sensors(temp, hum, dist, snd)

    # Résumé console
    print_summary(temp, hum, dist, snd, p, args.points)

    # Génération des points
    print(f"Calcul de {args.points} points...")
    xs, ys = generate_points(p, args.points)
    print(f"  X : [{min(xs):.1f}, {max(xs):.1f}] mm")
    print(f"  Y : [{min(ys):.1f}, {max(ys):.1f}] mm")
    print()

    # Aperçu ASCII
    if args.ascii or args.no_plot:
        print_ascii_preview(xs, ys)

    # Export G-code
    if args.gcode:
        export_gcode(xs, ys, p["feedrate"], args.gcode)

    # Graphique matplotlib
    if not args.no_plot:
        show_plot(xs, ys, temp, hum, dist, snd)
    else:
        print("(--no-plot : affichage graphique désactivé)")

if __name__ == "__main__":
    main()
