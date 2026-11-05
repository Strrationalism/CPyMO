"""The current release must be compiled into ARM9 and every Banner title."""
from pathlib import Path
import importlib.util
import json
import struct
import unittest

ROOT = Path(__file__).resolve().parents[3]
NDS = ROOT / 'cpymo-backends/nds'
spec = importlib.util.spec_from_file_location('current_release_gate', NDS / 'tools/verify-release-artifact.py')
gate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gate)


def synthetic_rom(runtime='1.3.0', banner_version='1.3.0'):
    # Real approved background, synthetic executable/title layout; no game data.
    art = (NDS / 'assets/nds-dual-bg.rgb555').read_bytes()
    assert art[:8] == b'CPYBG12\0'
    arm9 = gate.READY + art[8:]
    arm9 += f'CPyMO for NDS   {runtime}'.encode('ascii') + b'\0'
    arm9_at = 0x400
    banner_at = (arm9_at + len(arm9) + 511) & ~511
    data = bytearray(banner_at + gate._banner_module.BANNER_BYTES)
    data[0x12] = 2
    struct.pack_into('<I', data, 0x20, arm9_at)
    struct.pack_into('<I', data, 0x2c, len(arm9))
    struct.pack_into('<I', data, 0x68, banner_at)
    data[arm9_at:arm9_at + len(arm9)] = arm9
    banner = bytearray(gate._banner_module.BANNER_BYTES)
    struct.pack_into('<H', banner, 0, 1)
    title = gate._banner_module._encode_titles(('CPyMO for NDS', banner_version, '千岚之夏'))
    for i in range(6):
        start = 0x240 + i * 0x100
        banner[start:start + 0x100] = title
    struct.pack_into('<H', banner, 2, gate._banner_module.crc16_nds(banner[0x20:]))
    data[banner_at:] = banner
    return data


class ReleaseVersionTests(unittest.TestCase):
    def test_shared_version_sources_are_current(self):
        from release_info import VERSION, RELEASE
        self.assertEqual(VERSION, '1.3.0')
        self.assertEqual((NDS / 'VERSION').read_text().strip(), VERSION)
        self.assertEqual(json.loads((NDS / 'release.json').read_text())['version'], VERSION)
        self.assertEqual(RELEASE['cache_format'], 6)
        self.assertEqual(RELEASE['cache_format_compatibility'], [1, 2, 3, 4, 5, 6])

    def test_gate_accepts_current_compiled_and_banner_version(self):
        result = gate.validate_rom_bytes(synthetic_rom())
        self.assertEqual(result['compiled_runtime_version'], '1.3.0')
        self.assertEqual(result['banner_crc16'], 'PASS')
        self.assertTrue(all(t[1] == '1.3.0' for t in result['banner_titles']))

    def test_old_runtime_cannot_be_relabelled_with_current_banner(self):
        with self.assertRaisesRegex(ValueError, 'Compiled runtime version'):
            gate.validate_rom_bytes(synthetic_rom(runtime='1.2.1'))

    def test_current_runtime_cannot_keep_old_banner(self):
        with self.assertRaisesRegex(ValueError, 'Banner version/title'):
            gate.validate_rom_bytes(synthetic_rom(banner_version='1.2.1'))

    def test_corrupt_banner_crc_is_rejected(self):
        data = synthetic_rom()
        banner = struct.unpack_from('<I', data, 0x68)[0]
        data[banner + 0x240] ^= 1
        with self.assertRaisesRegex(ValueError, 'CRC'):
            gate.validate_rom_bytes(data)


if __name__ == '__main__':
    unittest.main()
