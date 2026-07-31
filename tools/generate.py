# -*- coding: utf-8 -*-
"""Erzeugt alle generierten Quellen des Moduls. Nach jeder Aenderung an slots.py oder
profiles.py aufrufen, danach die knxprod neu bauen.

    python tools/generate.py

Erzeugt:
    src/SolarmanPV.templ.xml            Geraeteseite mit 64 Messwertzeilen
    src/SlotCatalog.h                   derselbe Katalog fuer die Firmware
    src/SolarmanPV.profiles.parts.xml   Auswahlliste der Geraeteprofile
    src/SolarmanPV.assistant.parts.xml  globale Seite "Assistent"
    src/SolarmanPV.script.js            ETS-Skript samt Profildatenbank
"""
import os
import runpy
import sys

here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, here)

for script in ("gen_templ.py", "gen_profiles.py", "gen_assistant.py"):
    runpy.run_path(os.path.join(here, script), run_name="__main__")
