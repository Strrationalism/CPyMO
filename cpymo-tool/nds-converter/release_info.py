"""Shared release metadata; MIT. No independent GUI/engine version counter."""
from pathlib import Path
import json
import sys

if getattr(sys, 'frozen', False):
    METADATA = Path(sys._MEIPASS) / 'release.json'
else:
    METADATA = Path(__file__).resolve().parents[2] / 'cpymo-backends/nds/release.json'
RELEASE = json.loads(METADATA.read_text(encoding='utf8'))
VERSION = RELEASE['version']
