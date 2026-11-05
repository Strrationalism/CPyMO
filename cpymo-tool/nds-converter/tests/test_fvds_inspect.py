"""Synthetic FVDS format tests. No external game media or player code."""
import struct
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from fvds_inspect import FVDSFormatError, inspect_fvds


def sample(bad_key=False, bad_video=False, tail=False):
    header = struct.pack("<4sHHIIHHII", b"FVDS", 256, 192, 12, 1,
                         47605, 2, 3, 1)
    first = 40 if bad_key else 36
    data = [header, struct.pack("<II", 0, first)]
    for index, blocks in enumerate((16, 16, 15)):
        vlen = 5 if bad_video and index == 0 else 8
        data += [struct.pack("<I", (blocks << 17) | vlen),
                 bytes([index + 1]) * vlen,
                 bytes(blocks * 2 * 132)]
    return b"".join(data) + (b"X" if tail else b"")


class FVDSInspectorTest(unittest.TestCase):
    def inspect(self, raw):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "synthetic.fv"
            path.write_bytes(raw)
            return inspect_fvds(path, hash_file=True, payload_hash=True)

    def test_synthetic_fixture(self):
        info = self.inspect(sample())
        self.assertEqual(info["frames"], 3)
        self.assertEqual(info["header_and_index_bytes"], 36)
        self.assertEqual(info["frame_headers_bytes"], 12)
        self.assertEqual(info["video_payload_bytes"], 24)
        self.assertEqual(info["audio_bytes"], 47 * 264)
        self.assertEqual(info["container_exact_bytes"], info["size_bytes"])
        self.assertEqual(len(info["sha256"]), 64)
        self.assertEqual(info["payload_read_bytes"],
                         info["video_payload_bytes"]+info["audio_bytes"])
        self.assertEqual(len(info["payload_fnv1a32"]),8)

    def test_bad_first_keyframe(self):
        with self.assertRaises(FVDSFormatError):
            self.inspect(sample(bad_key=True))

    def test_trailing_bytes(self):
        with self.assertRaises(FVDSFormatError):
            self.inspect(sample(tail=True))

    def test_invalid_video_length(self):
        with self.assertRaises(FVDSFormatError):
            self.inspect(sample(bad_video=True))

    def test_truncated_last_packet(self):
        with self.assertRaises(FVDSFormatError):
            self.inspect(sample()[:-300])

    def test_wrong_magic(self):
        with self.assertRaises(FVDSFormatError):
            self.inspect(b"NOPE" + sample()[4:])


if __name__ == "__main__":
    unittest.main()
