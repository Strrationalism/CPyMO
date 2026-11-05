"""Audit coded picture I/P headers through the production FVDS bitreader.

Synthetic 3-frame FVDS fixture tests success, index/picture mismatch and
container trailing data. Neither proprietary samples nor decoder source are
committed. A user-provided private 159-second sample can be tested locally.
"""
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import unittest
from test_fvds_inspect import sample

ROOT = Path(__file__).resolve().parents[3]
SRC = ROOT / "cpymo-backends/nds/source"
CLI = ROOT / "cpymo-backends/nds/tools/fvds-picture-audit.c"


def picture_fixture():
    raw = bytearray(sample())
    offset = 36
    for frame in range(3):
        header = struct.unpack_from("<I", raw, offset)[0]
        vlen = header & 0x1FFFF
        blocks = header >> 17
        struct.pack_into("<H", raw, offset+4,
                         0x3c0f if frame == 0 else 0x8000)
        offset += 4+vlen+blocks*2*132
    assert offset == len(raw)
    return raw


@unittest.skipUnless(shutil.which("gcc"), "gcc unavailable")
class FVDSPictureAuditTests(unittest.TestCase):
    def test_genuine_production_bitreader_3_frame_index(self):
        with tempfile.TemporaryDirectory() as folder:
            d = Path(folder)
            executable = d / "audit"
            subprocess.run(["gcc", "-std=c11", "-O2", "-Wall", "-Wextra",
                            "-Werror", "-fsanitize=undefined",
                            "-I", str(SRC),
                            str(SRC/"fvds-stream-nds.c"),
                            str(SRC/"fvds-bits-nds.c"), str(CLI),
                            "-o", str(executable)],
                           check=True, capture_output=True, text=True)

            def check(blob, succeeds):
                movie = d / "fixture.fv"
                movie.write_bytes(blob)
                result = subprocess.run([str(executable), str(movie)],
                                        capture_output=True, text=True)
                self.assertEqual(result.returncode == 0, succeeds,
                                 result.stdout+"\n"+result.stderr)
                return result.stdout

            valid = picture_fixture()
            summary = check(valid, True)
            self.assertIn("I 0 q=30 bytes=8", summary)
            self.assertIn("PASS frames=3 I=1 P=2", summary)
            self.assertIn("audio_bytes=12408", summary)
            bad_first = bytearray(valid)
            struct.pack_into("<H", bad_first, 40, 0x8000)
            check(bad_first, False)
            bad_second = bytearray(valid)
            offset2 = 36+4+8+16*264
            struct.pack_into("<H", bad_second, offset2+4, 0x3c0f)
            check(bad_second, False)
            check(valid+b"X", False)
            check(valid[:-1], False)
