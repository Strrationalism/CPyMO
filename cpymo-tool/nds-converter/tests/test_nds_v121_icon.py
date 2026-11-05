"""NDS external banner and internal launcher icon must share the full-square art."""
from pathlib import Path
import re
import unittest
from PIL import Image

ROOT=Path(__file__).resolve().parents[3]
NDS=ROOT/"cpymo-backends/nds"

def rgb555(v):
 return tuple(((v>>shift)&31)*255//31 for shift in (0,5,10))

class NdsV121IconTests(unittest.TestCase):
 def test_square_banner_and_internal_icon_identical(self):
  with Image.open(NDS/"assets/icon.bmp") as bmp, Image.open(NDS/"assets/icon.png") as png:
   self.assertEqual(bmp.size,(32,32))
   self.assertEqual(bmp.mode,"P")
   self.assertEqual(png.size,(32,32))
   actual=list(bmp.convert("RGB").getdata())
   self.assertEqual(actual,list(png.convert("RGB").getdata()))
   self.assertEqual(len(set(actual)),15)
   self.assertTrue(all(1<=idx<=15 for idx in bmp.getdata()),
                   "no rounded transparent corners on the NDS icon")
  source=(NDS/"source/app-icon.h").read_text(encoding="utf8")
  match=re.search(r"app_icon\[1024\]\s*=\s*\{([^}]+)\}",source,re.S)
  self.assertIsNotNone(match)
  pixels=[int(x) for x in re.findall(r"\d+",match.group(1))]
  self.assertEqual(len(pixels),1024)
  self.assertTrue(all(v&0x8000 for v in pixels))
  self.assertEqual([rgb555(v) for v in pixels],actual)
 def test_release_language_precise(self):
  self.assertEqual((NDS/"VERSION").read_text().strip(),"1.3.0")
  notes=(NDS/"RELEASE-NOTES-1.3.0.md").read_text(encoding="utf8")
  self.assertIn("NDS 程序及配套转换器图标焕新",notes)
  self.assertIn("配套游戏转换器",notes)

if __name__=="__main__":
 unittest.main()
