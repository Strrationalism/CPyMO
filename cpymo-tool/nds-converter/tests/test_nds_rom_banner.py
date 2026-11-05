"""NDS banner credit regression independent from the devkitPro ARM SDK."""
import importlib.util
import struct
import tempfile
import unittest
from pathlib import Path

MODULE=Path(__file__).resolve().parents[3]/"cpymo-backends/nds/rom_banner.py"
SPEC=importlib.util.spec_from_file_location("rom_banner",MODULE)
banner=importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(banner)


def fake_rom(path):
    raw=bytearray(b"Q"*(0x400+0x840+64))
    struct.pack_into("<I",raw,0x68,0x400)
    blob=bytearray(b"\x00"*0x840)
    struct.pack_into("<H",blob,0,1)
    blob[0x20:0x240]=b"Z"*(0x240-0x20) # original icon/palette preserved
    stub="CPyMO for NDS\n1.2.0\nNDS port".encode("utf-16le")
    for i in range(6):
        blob[0x240+i*0x100:0x240+i*0x100+len(stub)]=stub
    struct.pack_into("<H",blob,2,banner.crc16_nds(blob[0x20:]))
    raw[0x400:0x400+0x840]=blob
    path.write_bytes(raw)
    return bytes(raw)


class ROMBannerTests(unittest.TestCase):
    def test_unicode_credit_crc_and_unmodified_rom_region(self):
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/"c.nds"
            source=fake_rom(path)
            banner.patch_nds_banner_titles(path,("CPyMO for NDS","1.2.0","千岚之夏"))
            updated=path.read_bytes()
            self.assertEqual(len(updated),len(source))
            # The banner checksum at +0x02 MUST change with its titles.
            # Only the rest of the header (including offset and icon)
            # must remain byte-identical.
            self.assertEqual(updated[:0x400+2],source[:0x400+2])
            self.assertEqual(updated[0x400+4:0x400+0x20],
                             source[0x400+4:0x400+0x20])
            self.assertNotEqual(updated[0x400+2:0x400+4],
                                source[0x400+2:0x400+4])
            self.assertEqual(updated[0x400+0x20:0x400+0x240],
                             source[0x400+0x20:0x400+0x240])
            self.assertEqual(updated[0x400+0x840:],source[0x400+0x840:])
            self.assertEqual(banner.read_nds_banner_titles(path),
                             [["CPyMO for NDS","1.2.0","千岚之夏"]]*6)
            self.assertIn("千岚之夏".encode("utf-16le"),updated)
            self.assertEqual(updated[0x400+2:0x400+4],
                             struct.pack("<H",banner.crc16_nds(updated[0x420:0xC40])))
            banner.patch_nds_banner_titles(path,("CPyMO for NDS","1.2.0","千岚之夏"))
            self.assertEqual(path.read_bytes(),updated)

    def test_invalid_banner_does_not_mutate_rom(self):
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/"bad.nds";data=bytearray(fake_rom(path))
            data[0x400+0x20]^=1;path.write_bytes(data)
            before=path.read_bytes()
            with self.assertRaises(ValueError):
                banner.patch_nds_banner_titles(path,("CPyMO","1.2.0","千岚之夏"))
            self.assertEqual(path.read_bytes(),before)

    def test_reject_malformed_titles(self):
        for entries in [("CPyMO","1.2.0"),("a\nb","v","c"),("a","b","x"*128)]:
            with self.assertRaises(ValueError):
                banner._encode_titles(entries)


if __name__=="__main__":
    unittest.main()
