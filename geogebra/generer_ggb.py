#!/usr/bin/env python3
"""
generer_ggb.py — Génère le fichier GeoGebra .ggb des 4 saisons
================================================================

Un fichier .ggb est un ZIP contenant geogebra.xml.
Ce script crée automatiquement le fichier .ggb depuis le XML source.

Usage :
    python3 generer_ggb.py
    → produit : saisons_harmonographe.ggb

Ouvrir ensuite dans GeoGebra Classic (www.geogebra.org/classic)
"""

import zipfile
import os
import sys
from pathlib import Path

# ─── Configuration ────────────────────────────────────────────────────────
XML_SOURCE  = "saisons_harmonographe.xml"
GGB_OUTPUT  = "saisons_harmonographe.ggb"
SCRIPT_DIR  = Path(__file__).parent

# ─── Vérification du fichier source ──────────────────────────────────────
xml_path = SCRIPT_DIR / XML_SOURCE
if not xml_path.exists():
    print(f"❌ Erreur : {XML_SOURCE} introuvable dans {SCRIPT_DIR}")
    sys.exit(1)

print(f"📂 Lecture de {XML_SOURCE}...")

# ─── Création du .ggb (ZIP avec geogebra.xml à l'intérieur) ──────────────
ggb_path = SCRIPT_DIR / GGB_OUTPUT

with zipfile.ZipFile(ggb_path, "w", zipfile.ZIP_DEFLATED) as zf:
    # Le fichier XML doit s'appeler "geogebra.xml" dans le ZIP
    zf.write(xml_path, arcname="geogebra.xml")

print(f"✅ Fichier créé : {GGB_OUTPUT}")
print(f"   Taille : {ggb_path.stat().st_size} octets")
print()
print("═" * 60)
print("  Pour ouvrir dans GeoGebra :")
print("  1. Ouvrir GeoGebra Classic (application ou navigateur)")
print("  2. Menu Fichier → Ouvrir")
print(f"  3. Sélectionner : {GGB_OUTPUT}")
print("═" * 60)
print()
print("  Contenu du fichier .ggb :")
with zipfile.ZipFile(ggb_path, "r") as zf:
    for info in zf.infolist():
        print(f"    {info.filename} ({info.file_size} octets)")
