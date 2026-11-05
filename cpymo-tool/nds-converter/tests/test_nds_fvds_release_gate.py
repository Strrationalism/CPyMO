"""Temporary publication hold for the unlicensed-source-derived FVDS decoder.

This is intentionally an engineering guard, NOT a legal provenance proof:
renaming a file will not cure a licence problem. Once explicit permission
or a reviewed independent replacement exists, update this test together
with RELEASE-LICENSE-GATE-1.2.md and its documentary evidence.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[3]
NDS = ROOT / "cpymo-backends" / "nds"
SRC = NDS / "source"
TOOL = ROOT / "cpymo-tool" / "nds-converter"

# Files from the private DeepSeek decoder handoff, NOT published source.
# In particular fvds-vlc-tables.h was mechanically created by parsing the
# unlicensed upstream Gericom.FastVideoDS/VlcTables.cs.
REVIEW_HOLD = (
    SRC / "fvds-vlc-tables.h",
    SRC / "fvds-vlc-nds.c",
    SRC / "fvds-vlc-nds.h",
    SRC / "fvds-idct-nds.c",
    SRC / "fvds-idct-nds.h",
    SRC / "fvds-video-nds.c",
    SRC / "fvds-video-nds.h",
    NDS / "tools" / "fvds-video-dump.c",
    TOOL / "tests" / "test_fvds_coefficients.py",
)

# A manually renamed copy of the exact generated table should also be
# rejected. This cannot detect every derivative implementation and does not
# replace human review or an upstream licence grant.
VENDOR_TABLE_SIGNATURES = (
    "nds_fvds_vlc_bias[224]",
    "nds_fvds_vlc_offset[4097]",
    "TableARefLinear",
    "Gericom.FastVideoDS/VlcTables.cs",
)

# Private film assets must not be committed even as test fixtures.
PRIVATE_FILM_NAMES = {
    "full-j1.fv", "source_n2_opening.mp4",
    "source_n2_opening.fv",
}

class NDSFVDSPublicReleaseGate(unittest.TestCase):
    def test_quarantined_decoder_files_not_in_public_tree(self):
        leaked = [str(path.relative_to(ROOT)) for path in REVIEW_HOLD if path.exists()]
        self.assertEqual(leaked, [], "Unreviewed FVDS decoder imported into public tree: "
                         + ", ".join(leaked))

    def test_generated_vendor_table_signature_not_present_in_nds_source(self):
        hits = []
        for path in SRC.glob("*"):
            if path.suffix not in (".c", ".h", ".py"):
                continue
            text = path.read_text(encoding="utf-8", errors="replace")
            for token in VENDOR_TABLE_SIGNATURES:
                if token in text:
                    hits.append(f"{path.name}:{token}")
        self.assertEqual(hits, [], "Vendor-derived FVDS table marker(s) in source: "
                         + ", ".join(hits))

    def test_user_private_media_not_shipped_as_source_fixtures(self):
        roots = (NDS, TOOL / "tests")
        matches = []
        for root in roots:
            for path in root.rglob("*"):
                if path.is_file() and path.name.lower() in PRIVATE_FILM_NAMES:
                    matches.append(str(path.relative_to(ROOT)))
        self.assertEqual(matches, [], "Private movie data in public repo: "
                         + ", ".join(matches))

if __name__ == "__main__":
    unittest.main()
