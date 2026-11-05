"""Check bit-perfect native LCD RGB555 -> PNG conversion with synthetic
pixel fixtures. These are tests only, NEVER presented as rendered game images.
"""
import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest
import zlib

MODULE=Path(__file__).resolve().parents[3]/"cpymo-backends/nds/tools/nds-capture-to-png.py"
SPEC=importlib.util.spec_from_file_location("nds_capture_to_png",MODULE)
capture=importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(capture)


def fixture(kind=1):
    # Top: true RGB555 red / green alternating; bottom: blue/black.
    red=0x801f
    green=0x83e0
    blue=0xfc00
    top=struct.pack("<HH",red,green)*(256*192//2)
    bottom=struct.pack("<HH",blue,0x8000)*(256*192//2)
    return (struct.pack("<8s4H",b"NDSPIX1\0",256,192,2,kind)
            + top+bottom)


def parse_png(blob):
    assert blob[:8]==b"\x89PNG\r\n\x1a\n"
    pos=8
    payload=b""
    width=height=None
    while pos<len(blob):
        length=struct.unpack_from(">I",blob,pos)[0]
        tag=blob[pos+4:pos+8]
        data=blob[pos+8:pos+8+length]
        crc=struct.unpack_from(">I",blob,pos+8+length)[0]
        assert zlib.crc32(tag+data)&0xffffffff==crc
        pos+=length+12
        if tag==b"IHDR":
            width,height=struct.unpack_from(">II",data)
            assert data[8:]==bytes((8,2,0,0,0))
        elif tag==b"IDAT":
            payload+=data
        elif tag==b"IEND":
            break
    raw=zlib.decompress(payload)
    assert (width,height)==(256,192)
    assert len(raw)==192*(256*3+1)
    assert all(raw[y*(256*3+1)]==0 for y in range(192))
    return b"".join(raw[y*(256*3+1)+1:(y+1)*(256*3+1)]
                    for y in range(192))


class NativeLCDCaptureTests(unittest.TestCase):
    def test_capture_header_and_rgb555_color(self):
        kind,(top,bottom)=capture.decode_capture(fixture())
        self.assertEqual(kind,1)
        self.assertEqual(top[:6],bytes((255,0,0, 0,255,0)))
        self.assertEqual(bottom[:6],bytes((0,0,255, 0,0,0)))
        self.assertEqual(parse_png(capture.png_rgb(256,192,top)),top)
        self.assertEqual(parse_png(capture.png_rgb(256,192,bottom)),bottom)

    def test_four_capture_kinds_and_screen_order(self):
        for kind in (1, 2, 3, 4):
            with self.subTest(kind=kind):
                actual_kind, (top, bottom) = capture.decode_capture(fixture(kind))
                self.assertEqual(actual_kind, kind)
                self.assertEqual(top[:3], bytes((255, 0, 0)))
                self.assertEqual(bottom[:3], bytes((0, 0, 255)))

    def test_reject_truncated_and_corrupt(self):
        cases=[fixture()[:-1],fixture()+b"x",b"X"+fixture()[1:],
               fixture(kind=0),fixture(kind=5),fixture(kind=99)]
        for blob in cases:
            with self.subTest(length=len(blob)):
                with self.assertRaises(ValueError):
                    capture.decode_capture(blob)

    def test_file_outputs_are_two_separate_native_lcd_pngs(self):
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/"nds-capture-dialogue.ndsraw"
            path.write_bytes(fixture())
            top,bottom=capture.convert(path)
            self.assertEqual(top.name,"nds-capture-dialogue-top.png")
            self.assertEqual(bottom.name,"nds-capture-dialogue-bottom.png")
            self.assertEqual(parse_png(top.read_bytes())[:3],bytes((255,0,0)))
            self.assertEqual(parse_png(bottom.read_bytes())[:3],bytes((0,0,255)))
            self.assertEqual(path.read_bytes(),fixture())
