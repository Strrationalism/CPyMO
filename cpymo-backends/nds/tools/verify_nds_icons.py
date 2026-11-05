#!/usr/bin/env python3
"""Read-only checks of generated NDS assets and optionally the actual ROM.

Standard library only so the pinned SDK container can verify the same
committed resources without installing an image stack during compilation.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct

NDS = Path(__file__).resolve().parents[1]

def verify(nds: Path, rom: Path | None = None) -> dict:
    manifest = json.loads((nds / 'assets/NDS-ICON-MANIFEST.json').read_text())
    source = nds / 'assets' / manifest['source']
    if hashlib.sha256(source.read_bytes()).hexdigest() != manifest['source_sha256']:
        raise ValueError('NDS source identity mismatch')
    for relative, identity in manifest['outputs'].items():
        data = (nds / relative).read_bytes()
        if len(data) != identity['bytes'] or hashlib.sha256(data).hexdigest() != identity['sha256']:
            raise ValueError('NDS icon resource identity mismatch: ' + relative)
    header = (nds / 'source/app-icon.h').read_text()
    arrays = {}
    edge = 0x8000 | sum(v << shift for v, shift in zip(manifest['border_rgb555'], (0, 5, 10)))
    for symbol, side in (('app_icon', 32), ('app_icon_small', 24), ('app_icon_splash', 64)):
        match = re.search(r'\b' + symbol + r'\[' + str(side * side) + r'\]\s*=\s*\{([^}]+)\}', header, re.S)
        if not match:
            raise ValueError('Missing native NDS icon array: ' + symbol)
        values = [int(v) for v in re.findall(r'\d+', match.group(1))]
        if len(values) != side * side or any(not (0x8000 <= v <= 0xffff) for v in values):
            raise ValueError('Wrong native dimensions/opacity: ' + symbol)
        if any(values[y*side+x] != edge for y in range(side) for x in range(side)
               if x in (0, side-1) or y in (0, side-1)):
            raise ValueError('Wrong native border: ' + symbol)
        arrays[symbol] = values
    if len(set(arrays['app_icon'])) > 15:
        raise ValueError('Banner exceeds 15 opaque colors')
    result = {'assets': 'PASS', 'native_sizes': [24, 32, 64],
              'border_rgb555': manifest['border_rgb555'], 'source_sha256': manifest['source_sha256']}
    if rom is not None:
        data = rom.read_bytes()
        for symbol, values in arrays.items():
            if struct.pack('<' + str(len(values)) + 'H', *values) not in data:
                raise ValueError('Compiled ROM is missing native resource: ' + symbol)
        if len(data) < 0x200:
            raise ValueError('Truncated NDS header')
        offset = struct.unpack_from('<I', data, 0x68)[0]
        banner = data[offset:offset+0x840]
        if len(banner) != 0x840:
            raise ValueError('Truncated NDS banner')
        palette = struct.unpack_from('<16H', banner, 0x220)
        for y in range(32):
            for x in range(32):
                at = 0x20 + (y//8*4+x//8)*32 + (y%8)*4 + x%8//2
                index = (banner[at] >> (4*(x%2))) & 15
                if index == 0 or (palette[index] | 0x8000) != arrays['app_icon'][y*32+x]:
                    raise ValueError('Actual banner differs from approved generated icon')
        result.update(rom='PASS', rom_sha256=hashlib.sha256(data).hexdigest(),
                      actual_banner_matches_internal_icon=True,
                      all_native_arrays_present=True, rom_bytes=len(data))
    return result

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--nds', type=Path, default=NDS)
    parser.add_argument('--rom', type=Path)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    result = verify(args.nds, args.rom)
    text = json.dumps(result, indent=2) + '\n'
    if args.report:
        args.report.write_text(text)
    print(text, end='')
