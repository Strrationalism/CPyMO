"""Build a paired Windows ROM/converter distribution. MIT; Python 3.10+."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import importlib

ROOT = Path(__file__).resolve().parent
BACKEND = ROOT.parents[1] / 'cpymo-backends/nds'

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ffmpeg', type=Path, required=True)
    parser.add_argument('--rom', type=Path, help='Explicit paired, validated NTR/TWL release ROM')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--rebuild', action='store_true', help='Reuse an owned portable-build directory')
    parser.add_argument('--require-desktop-icon', action='store_true',
                        help='Fail packaging if the owner-approved converter icon is missing')
    args = parser.parse_args()
    if os.name != 'nt':
        parser.error('Build the Windows portable executable on Windows.')
    # Fail before creating a package which starts but cannot compress images.
    for module in ('PyInstaller', 'PIL.Image', 'lz4.block'):
        try:
            importlib.import_module(module)
        except ImportError as error:
            parser.error(f'Missing build dependency {module}: {error}')
    ffmpeg = args.ffmpeg.resolve()
    output = args.output.resolve()
    rom = args.rom.resolve() if args.rom else BACKEND / 'engine.nds'
    owned = (output/'build/Converter.spec').is_file() and (output/'helper-build.log').is_file()
    if (output.exists() and not (args.rebuild and owned)) or not ffmpeg.is_file() or not rom.is_file():
        parser.error('Use a new output directory and existing FFmpeg/ROM files.')
    # A release candidate must never bundle the staging image placeholder.
    # Developer builds that omit --rom remain possible, but cannot be
    # mistaken for the explicitly validated single-ROM release path.
    if args.rom:
        from importlib import util as importlib_util
        verifier = BACKEND / 'tools/verify-release-artifact.py'
        spec = importlib_util.spec_from_file_location('cpymo_release_gate', verifier)
        module = importlib_util.module_from_spec(spec)
        spec.loader.exec_module(module)
        module.validate_rom_bytes(rom.read_bytes())
    # Old 1.1 Starter's custom ffmpeg.exe intentionally had --disable-swscale
    # and --disable-everything. It plays audio but can never convert MP4.
    # Refuse to ship a deceptive 1.2 bundle even when PyInstaller succeeds.
    from video_nvl import validate_ffmpeg
    try:
        validate_ffmpeg(str(ffmpeg))
    except ValueError as error:
        parser.error(str(error))
    release = json.loads((BACKEND/'release.json').read_text(encoding='utf8'))
    output.mkdir(parents=True, exist_ok=args.rebuild)
    build = output/'build'
    resources = build/'resources'
    resources.mkdir(parents=True, exist_ok=True)
    source_icon = ROOT/'assets/converter-icon-source.png'
    icon_out = build/'branding'
    icon_args=[]
    icon_manifest=None
    if source_icon.is_file():
        from tools.build_desktop_icons import build as build_icons
        icon_manifest=build_icons(source_icon,icon_out)
        shutil.copy2(icon_out/'icon-manifest.json',output/'ICON-MANIFEST.json')
        icon_args=['--icon',str(icon_out/'converter-icon.ico'),
                   '--add-data',f"{icon_out/'converter-icon-64.png'};assets"]
    elif args.require_desktop_icon:
        parser.error("Missing approved converter artwork: "+str(source_icon))
    paired_rom = resources/'cpymo-nds.nds'
    shutil.copy2(rom, paired_rom)
    command = [sys.executable, '-m', 'PyInstaller', '--noconfirm', '--clean',
               '--onedir', '--windowed', '--name', 'Converter',
               '--distpath', str(output/'app'), '--workpath', str(build),
               '--specpath', str(build), '--add-data', f'{ROOT / "fonts"};fonts',
               '--add-data', f'{BACKEND / "release.json"};.',
               '--add-data', f'{paired_rom};.', '--add-binary', f'{ffmpeg};.',
               '--exclude-module', 'numpy', '--exclude-module', 'scipy',
               '--exclude-module', 'matplotlib', '--exclude-module', 'pytest',
               str(ROOT/'app.py')]
    command[-1:-1]=icon_args
    if (ROOT/'lossless-native.dll').is_file():
        command[-1:-1]=['--add-binary',f'{ROOT / "lossless-native.dll"};.']
    with (output/'helper-build.log').open('w', encoding='utf8') as log:
        subprocess.run(command, check=True, stdout=log, stderr=subprocess.STDOUT)
    shutil.copy2(rom, output/'cpymo-nds.nds')
    digest = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
    assert digest(rom) == digest(output/'app/Converter/_internal/cpymo-nds.nds')
    release.update(rom_sha256=digest(rom), ffmpeg_sha256=digest(ffmpeg),
                   desktop_icon=icon_manifest["artwork_id"] if icon_manifest else "default-not-public",
                   desktop_icon_pixel_sha256=icon_manifest["source_rgba_sha256"] if icon_manifest else None,
                   desktop_icon_source_sha256=digest(source_icon) if icon_args else None)
    (output/'VERSION.json').write_text(json.dumps(release, indent=2)+'\n', encoding='utf8')
    (output/'BUILD-COMMAND.json').write_text(json.dumps(command, indent=2), encoding='utf8')
    print(json.dumps(release, indent=2))

if __name__ == '__main__':
    main()
