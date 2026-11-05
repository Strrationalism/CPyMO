"""Protect reproducible NDS-only icon derivation and actual ROM identity gates."""
import importlib.util
import json
from pathlib import Path
import struct
import tempfile
import unittest
from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
NDS = ROOT / 'cpymo-backends/nds'

def load(name, filename):
    spec = importlib.util.spec_from_file_location(name, NDS / 'tools' / filename)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module

generator = load('native_icon_generator', 'build_nds_icons.py')
verifier = load('native_icon_verifier', 'verify_nds_icons.py')

class NdsIconDerivationTests(unittest.TestCase):
    def test_committed_outputs_reproduce_from_pinned_source(self):
        with tempfile.TemporaryDirectory() as tmp:
            target = Path(tmp)
            actual = generator.build(NDS / 'assets/icon-source-square.png', target)
            expected = json.loads((NDS / 'assets/NDS-ICON-MANIFEST.json').read_text())
            # PNG compression bytes can differ between macOS/Linux zlib builds.
            # Pin decoded pixels and dimensions for regeneration, while the
            # verifier separately checks the hashes of actual committed files.
            self.assertEqual(verifier.verify(NDS)['assets'], 'PASS')
            for relative in actual['outputs']:
                if relative.endswith('.png'):
                    a, b = actual['outputs'][relative], expected['outputs'][relative]
                    self.assertEqual((a['size'], a['rgb_sha256']), (b['size'], b['rgb_sha256']))
                    with Image.open(target / relative) as generated, Image.open(NDS / relative) as committed:
                        self.assertEqual(generated.size, committed.size)
                        self.assertEqual(generated.convert('RGB').tobytes(), committed.convert('RGB').tobytes())
                else:
                    self.assertEqual(actual['outputs'][relative], expected['outputs'][relative])
                    self.assertEqual((target / relative).read_bytes(), (NDS / relative).read_bytes())
            self.assertEqual({k:v for k,v in actual.items() if k != 'outputs'},
                             {k:v for k,v in expected.items() if k != 'outputs'})

    def test_banner_has_reserved_transparency_and_native_warm_frame(self):
        self.assertEqual(verifier.verify(NDS)['assets'], 'PASS')
        with Image.open(NDS / 'assets/icon.bmp') as bmp:
            self.assertEqual(bmp.size, (32, 32))
            self.assertEqual(bmp.mode, 'P')
            self.assertTrue(all(1 <= v <= 15 for v in bmp.get_flattened_data()))
            self.assertEqual(len(set(bmp.get_flattened_data())), 15)
        for name, side in [('icon.png',32), ('icon-small.png',24), ('icon-splash.png',64)]:
            with Image.open(NDS / 'assets' / name) as image:
                self.assertEqual(image.size, (side, side))
                self.assertTrue(all(image.getpixel((x,y)) == generator.BORDER_RGB
                                    for y in range(side) for x in range(side)
                                    if x in (0,side-1) or y in (0,side-1)))

    def test_rejects_changed_source_even_if_image_still_decodes(self):
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp) / 'other.png'
            source.write_bytes((NDS / 'assets/icon-source-square.png').read_bytes() + b'changed')
            with self.assertRaisesRegex(ValueError, 'source file'):
                generator.derive(source)

    def test_rejects_rom_missing_new_high_resolution_resource(self):
        images = generator.derive(NDS / 'assets/icon-source-square.png')
        data = b'\0' * 0x200
        for name in ('icon.png', 'icon-small.png'):
            values = generator.pixels(images[name])
            data += struct.pack('<' + str(len(values)) + 'H', *values)
        with tempfile.TemporaryDirectory() as tmp:
            rom = Path(tmp) / 'old-resources.nds'
            rom.write_bytes(data)
            with self.assertRaisesRegex(ValueError, 'app_icon_splash'):
                verifier.verify(NDS, rom)

    def test_rejects_banner_not_matching_compiled_runtime_icon(self):
        images = generator.derive(NDS / 'assets/icon-source-square.png')
        data = bytearray(0x200 + 0x840)
        struct.pack_into('<I', data, 0x68, 0x200)
        for image in images.values():
            values = generator.pixels(image)
            data.extend(struct.pack('<' + str(len(values)) + 'H', *values))
        with tempfile.TemporaryDirectory() as tmp:
            rom = Path(tmp) / 'wrong-banner.nds'
            rom.write_bytes(data)
            with self.assertRaisesRegex(ValueError, 'Actual banner'):
                verifier.verify(NDS, rom)

if __name__ == '__main__':
    unittest.main()
