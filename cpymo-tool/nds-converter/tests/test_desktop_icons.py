"""Immutable approved source, unchanged Windows output and explicit Mac padding."""
from __future__ import annotations
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import sys
import tempfile
import unittest
from unittest import mock
from types import SimpleNamespace
from PIL import Image, ImageDraw

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT))
from tools.build_desktop_icons import (build, SOURCE_SHA256, APPROVED_RGBA_SHA256,
                                      ARTWORK_ID, WINDOWS_SIZES, OPTICAL_WIDTH_SCALE,
                                      ico_frame_hashes, pixel_sha, MACOS_CONTENT_SCALE)

class DesktopIconBuildTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp=tempfile.TemporaryDirectory()
        cls.source=ROOT/"assets/converter-icon-source.png"
        cls.out=Path(cls.temp.name)/"production"
        cls.manifest=build(cls.source,cls.out)
    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()
    def test_source_hash_and_size_are_pinned(self):
        self.assertEqual(hashlib.sha256(self.source.read_bytes()).hexdigest(),SOURCE_SHA256)
        with Image.open(self.source) as im:
            self.assertEqual(im.size,(1024,1024))
            self.assertEqual(pixel_sha(im),APPROVED_RGBA_SHA256)
    def test_master_is_byte_identical_to_approved_png(self):
        self.assertEqual(self.source.read_bytes(),(self.out/"converter-icon.png").read_bytes())
        self.assertEqual(OPTICAL_WIDTH_SCALE,1.0)
        self.assertFalse(self.manifest["reapplied_crop_or_mask"])
        self.assertFalse(self.manifest["lossy_reencoding"])
        self.assertFalse((self.out/"converter-icon-source-clean.png").exists())
    def test_windows_ico_has_seven_exact_source_resizes(self):
        with Image.open(self.source) as source, Image.open(self.out/"converter-icon.ico") as icon:
            self.assertEqual(set(icon.info["sizes"]),set(WINDOWS_SIZES))
            for size in WINDOWS_SIZES:
                actual=icon.ico.getimage(size).convert("RGBA")
                expected=source.convert("RGBA").resize(size,Image.Resampling.LANCZOS)
                self.assertEqual(actual.tobytes(),expected.tobytes(),size)
        frames=ico_frame_hashes(self.out/"converter-icon.ico")
        self.assertEqual(frames,self.manifest["windows_ico_frames_sha256"])
        self.assertEqual(len(frames),7)
    def test_macos_icns_uniform_scale_and_centered_transparent_canvas(self):
        with Image.open(self.source) as source:
            expected=Image.new('RGBA',(1024,1024))
            expected.alpha_composite(source.convert('RGBA').resize((819,819),Image.Resampling.LANCZOS),(102,102))
        with Image.open(self.out/"converter-icon.icns") as icon:
            self.assertEqual(icon.size,(1024,1024))
            self.assertEqual(icon.convert('RGBA').tobytes(),expected.tobytes())
            self.assertNotEqual(pixel_sha(icon),APPROVED_RGBA_SHA256)
            self.assertIn((512,512,2),icon.info["sizes"])
            for size in icon.info['sizes']:
                frame=icon.icns.getimage(size).convert('RGBA')
                self.assertEqual(frame.tobytes(),expected.resize(frame.size,Image.Resampling.LANCZOS).tobytes(),size)
        rule=self.manifest['macos_derivation']
        self.assertEqual(MACOS_CONTENT_SCALE,0.8)
        self.assertEqual(rule['content_size'],[819,819])
        self.assertEqual(rule['offset'],[102,102])
        self.assertEqual(rule['padding'],[102,102,103,103])
        self.assertEqual(rule['derived_rgba_sha256'],pixel_sha(expected))
        self.assertIsNone(expected.getchannel('A').crop((0,0,1024,102)).getbbox())
    def test_gui_png_matches_direct_resample(self):
        with Image.open(self.source) as src, Image.open(self.out/"converter-icon-64.png") as small:
            expected=src.convert("RGBA").resize((64,64),Image.Resampling.LANCZOS)
            self.assertEqual(small.convert("RGBA").tobytes(),expected.tobytes())
    def test_manifest_hashes_cover_actual_outputs(self):
        manifest=json.loads((self.out/"icon-manifest.json").read_text())
        self.assertEqual(manifest["artwork_id"],ARTWORK_ID)
        for item in manifest["outputs"].values():
            data=(self.out/item["file"]).read_bytes()
            self.assertEqual(len(data),item["bytes"])
            self.assertEqual(hashlib.sha256(data).hexdigest(),item["sha256"])
    def test_alternate_image_is_rejected(self):
        with tempfile.TemporaryDirectory() as td:
            f=Path(td)/"fake.png"
            Image.new("RGBA",(1024,1024),(0,0,0,0)).save(f)
            with self.assertRaises(ValueError):build(f,Path(td)/"out")
    def test_wrong_dimensions_and_opaque_source_rejected(self):
        with tempfile.TemporaryDirectory() as td:
            f=Path(td)/"fixture.png"
            Image.new("RGBA",(32,32),(0,0,0,0)).save(f)
            with self.assertRaises(ValueError):build(f,Path(td)/"out",allow_alternate_source=True)
            Image.new("RGBA",(1024,1024),(10,20,30,255)).save(f)
            with self.assertRaises(ValueError):build(f,Path(td)/"out",allow_alternate_source=True)
    def test_packagers_embed_and_preserve_new_manifest(self):
        for name in ("build_portable.py","build_portable_macos.py"):
            text=(ROOT/name).read_text(encoding="utf8")
            self.assertIn("assets/converter-icon-source.png",text)
            self.assertIn("ICON-MANIFEST.json",text)
            self.assertNotIn('desktop_icon="approved-PyMO-04"',text)
            self.assertIn('desktop_icon_pixel_sha256=',text)
    def test_app_icon_resource_is_nonfatal_when_running_source(self):
        text=(ROOT/"app.py").read_text(encoding="utf8")
        self.assertIn("def apply_desktop_icon(window):",text)
        self.assertIn("window._cpymo_icon_photo=photo",text)
        self.assertNotIn("cpymo-backends/nds/assets/icon.png",text)

    def test_frozen_macos_keeps_bundle_icon_and_windows_still_sets_photo(self):
        import app
        window=mock.Mock()
        with mock.patch.object(app,'sys',SimpleNamespace(platform='darwin',frozen=True)), mock.patch.object(app.tk,'PhotoImage') as photo:
            self.assertFalse(app.apply_desktop_icon(window))
            photo.assert_not_called()
            window.iconphoto.assert_not_called()
        with tempfile.TemporaryDirectory() as td:
            base=Path(td);(base/'assets').mkdir();(base/'assets/converter-icon-64.png').write_bytes(b'fixture')
            image=object()
            with mock.patch.object(app,'sys',SimpleNamespace(platform='win32',frozen=True,_MEIPASS=td)), mock.patch.object(app.tk,'PhotoImage',return_value=image):
                self.assertTrue(app.apply_desktop_icon(window))
                window.iconphoto.assert_called_once_with(True,image)
                self.assertIs(window._cpymo_icon_photo,image)

    def test_audit_reconstructs_mac_rule_and_rejects_wrong_source_scale_or_icon(self):
        import copy,io
        audit_path=ROOT.parents[1]/'cpymo-backends/nds/tools/audit-desktop-release.py'
        spec=importlib.util.spec_from_file_location('mac_icon_audit',audit_path)
        audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit)
        source=self.source.read_bytes();icon=(self.out/'converter-icon.icns').read_bytes()
        self.assertEqual(audit.verify_macos_icon(source,self.manifest,icon)['status'],'PASS')
        wrong=copy.deepcopy(self.manifest);wrong['macos_derivation']['content_scale']=0.9
        with self.assertRaises(AssertionError):audit.verify_macos_icon(source,wrong,icon)
        with self.assertRaises(AssertionError):audit.verify_macos_icon(b'wrong source',self.manifest,icon)
        with Image.open(self.source) as im:
            buf=io.BytesIO();im.save(buf,format='ICNS')
        wrong=copy.deepcopy(self.manifest);wrong['outputs']['icns']['sha256']=hashlib.sha256(buf.getvalue()).hexdigest()
        with self.assertRaises(AssertionError):audit.verify_macos_icon(source,wrong,buf.getvalue())

if __name__=="__main__":unittest.main()
