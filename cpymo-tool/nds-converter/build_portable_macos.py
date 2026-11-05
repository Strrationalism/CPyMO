"""Build a self-contained macOS CPyMO NDS conversion assistant (.app).

Development candidate until native macOS bundle, third-party FFmpeg licensing
and Gatekeeper distribution are validated. Must be built ON macOS with an
explicit final ROM; never substitutes an unrelated local 1.1 engine.
"""
from __future__ import annotations
from pathlib import Path
import argparse
import hashlib
import importlib
import json
import plistlib
import platform
import shutil
import subprocess
import sys

HERE = Path(__file__).resolve().parent
BACKEND = HERE.parents[1] / "cpymo-backends/nds"


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ffmpeg", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--target-arch", choices=("arm64", "x86_64"), default=platform.machine(),
                        help="Build a native single-architecture app; ARM64 requires Apple silicon.")
    parser.add_argument("--require-desktop-icon", action="store_true",
                        help="Fail packaging unless the approved rounded original icon source is present")
    args = parser.parse_args()
    if sys.platform != "darwin":
        parser.error("Build native macOS applications on a macOS host.")
    if platform.machine() != args.target_arch:
        parser.error(f"Host architecture {platform.machine()} != {args.target_arch}; "
                     "use a matching native macOS runner, never Rosetta for arm64 builds.")
    for module in ("PyInstaller", "PIL.Image", "lz4.block", "tkinter"):
        try:
            importlib.import_module(module)
        except ImportError as exc:
            parser.error(f"Missing package {module}: {exc}")
    output = args.output.resolve()
    rom = args.rom.resolve()
    ffmpeg = args.ffmpeg.resolve()
    if output.exists() or not rom.is_file() or not ffmpeg.is_file():
        parser.error("Choose a NEW output directory and valid ROM / FFmpeg inputs.")
    from video_nvl import validate_ffmpeg
    validate_ffmpeg(str(ffmpeg))
    def require_arch(binary: Path) -> None:
        probe = subprocess.run(["lipo", "-archs", str(binary)], check=True,
                               capture_output=True, text=True, timeout=30)
        arches = probe.stdout.strip().split()
        if args.target_arch not in arches:
            raise RuntimeError(f"{binary} is {arches}, expected {args.target_arch}; "
                               "do not ship an x86_64-only app on Apple silicon.")
    require_arch(ffmpeg)
    metadata = json.loads((BACKEND / "release.json").read_text(encoding="utf8"))
    if metadata["version"] != (BACKEND / "VERSION").read_text().strip():
        parser.error("Engine version metadata mismatch.")
    output.mkdir(parents=True)
    build = output / "_build"
    resources = build / "resources"
    resources.mkdir(parents=True)
    paired_rom = resources / "cpymo-nds.nds"
    paired_ffmpeg = resources / "ffmpeg"
    shutil.copy2(rom, paired_rom)
    shutil.copy2(ffmpeg, paired_ffmpeg)
    paired_ffmpeg.chmod(paired_ffmpeg.stat().st_mode | 0o111)
    icon_source = HERE / "assets/converter-icon-source.png"
    icon_args=[]
    icon_manifest=None
    if icon_source.is_file():
        from tools.build_desktop_icons import build as build_icons
        branding = build / "branding"
        icon_manifest=build_icons(icon_source,branding)
        shutil.copy2(branding/"icon-manifest.json",output/"ICON-MANIFEST.json")
        # macOS uses only the native bundle ICNS. A Tk 64px iconphoto would
        # replace the Dock tile, discarding both its padding and resolution.
        icon_args=["--icon",str(branding/"converter-icon.icns")]
    elif args.require_desktop_icon:
        parser.error(f"Missing owner-approved desktop icon: {icon_source}")
    command = [sys.executable, "-m", "PyInstaller",
               "--noconfirm", "--clean", "--onedir", "--windowed",
               "--name", "CPyMO-NDS-Converter",
               "--target-arch", args.target_arch,
               "--osx-bundle-identifier", "dev.blueteemo.cpymo-nds-converter",
               "--distpath", str(build / "dist"),
               "--workpath", str(build / "temp"),
               "--specpath", str(build),
               "--add-data", f"{HERE / 'fonts'}:fonts",
               "--add-data", f"{BACKEND / 'release.json'}:.",
               "--add-data", f"{paired_rom}:.",
               "--add-binary", f"{paired_ffmpeg}:.",
               "--exclude-module", "numpy", "--exclude-module", "scipy",
               "--exclude-module", "matplotlib", "--exclude-module", "pytest",
               str(HERE / "app.py")]
    command[-1:-1]=icon_args
    # The runtime uses sys._MEIPASS for the bundled FFmpeg and ROM. macOS's
    # app contains Resources/Frameworks cross-links; inspect both locations.
    with (output / "build.log").open("w", encoding="utf8") as log:
        subprocess.run(command, check=True, stdout=log, stderr=subprocess.STDOUT)
    app = build / "dist/CPyMO-NDS-Converter.app"
    if not app.is_dir():
        raise RuntimeError("Missing .app from PyInstaller")
    # Confirm the actual bundle icon is our selected design rather than
    # PyInstaller's default icon, irrespective of its eventual basename.
    icon_name=None
    if icon_args:
        expected_icon=digest(branding/"converter-icon.icns")
        candidates=list((app/"Contents/Resources").glob("*.icns"))
        verified=[p for p in candidates if digest(p)==expected_icon]
        if len(verified)!=1:
            raise RuntimeError("Configured Finder/Dock icon does not match approved rounded original ICNS")
        icon_name=verified[0].name
    # PyInstaller defaults to Finder's 0.0.0 unless the actual release is set.
    # Set the release metadata and re-apply only an ad-hoc local code signature:
    # this is NOT Apple Developer ID signing or notarization.
    info = app / "Contents/Info.plist"
    with info.open("rb") as file:
        plist = plistlib.load(file)
    plist["CFBundleShortVersionString"] = metadata["version"]
    plist["CFBundleVersion"] = metadata["version"]
    plist["CFBundleGetInfoString"] = f"CPyMO NDS Converter {metadata['version']}"
    if icon_name:
        plist["CFBundleIconFile"] = icon_name
    with info.open("wb") as file:
        plistlib.dump(plist, file)
    subprocess.run(["codesign", "--force", "--deep", "--sign", "-", str(app)],
                   check=True, capture_output=True, text=True, timeout=180)
    subprocess.run(["codesign", "--verify", "--deep", "--strict", str(app)],
                   check=True, capture_output=True, text=True, timeout=90)
    with info.open("rb") as file:
        assert plistlib.load(file)["CFBundleShortVersionString"] == metadata["version"]
    require_arch(app / "Contents/MacOS/CPyMO-NDS-Converter")
    if icon_name:
        if digest(app/"Contents/Resources"/icon_name)!=expected_icon:
            raise RuntimeError("Packaged Finder/Dock icon changed after ad-hoc signing")
    for resource in ("cpymo-nds.nds", "ffmpeg", "release.json"):
        present = any((app / location / resource).is_file() for location in
                      ("Contents/Resources", "Contents/Frameworks", "Contents/MacOS"))
        if not present:
            raise RuntimeError("Missing bundled resource " + resource)
    internal_rom = next(p for p in (app / "Contents/Resources/cpymo-nds.nds",
                                  app / "Contents/Frameworks/cpymo-nds.nds") if p.is_file())
    if digest(internal_rom) != digest(rom):
        raise RuntimeError("Bundled ROM does not match the supplied input")
    bundled_ffmpeg = next((p for p in (
        app / "Contents/Resources/ffmpeg",
        app / "Contents/Frameworks/ffmpeg",
    ) if p.is_file()), None)
    if bundled_ffmpeg is None:
        raise RuntimeError("Bundled FFmpeg executable not found")
    require_arch(bundled_ffmpeg)
    # PyInstaller applies an ad-hoc macOS code signature to its collected
    # binaries; the app's actual executable can differ byte-for-byte from
    # the source-built input. Record BOTH hashes, not a misleading single one.
    ffmpeg_input_sha = digest(ffmpeg)
    ffmpeg_bundle_sha = digest(bundled_ffmpeg)
    shutil.copytree(app, output / app.name, symlinks=True)
    shutil.copy2(rom, output / "cpymo-nds.nds")
    shutil.rmtree(build)
    metadata.update(desktop_icon=icon_manifest["artwork_id"] if icon_manifest else "default-not-public",
                    desktop_icon_source_sha256=icon_manifest["source_sha256"] if icon_manifest else None,
                    desktop_icon_pixel_sha256=icon_manifest["source_rgba_sha256"] if icon_manifest else None,
                    macos_dock_icon_source="bundle_icns",
                    macos_icon_derivation=icon_manifest["macos_derivation"] if icon_manifest else None,
                    macos_architecture=args.target_arch,
                    app_bundle_version=metadata["version"],
                    rosetta_required=False if args.target_arch=="arm64" else "Apple silicon only",
                    rom_sha256=digest(rom),
                    ffmpeg_input_sha256=ffmpeg_input_sha,
                    ffmpeg_bundled_sha256=ffmpeg_bundle_sha,
                    ffmpeg_sha256=ffmpeg_input_sha,
                    macos_bundle_validation="paired ROM and FFmpeg captured; ad-hoc signed/not notarized")
    (output / "VERSION.json").write_text(json.dumps(metadata, ensure_ascii=False, indent=2) + "\n", encoding="utf8")
    (output / "README-macOS.txt").write_text(
        f"CPyMO NDS 转换助手 {metadata['version']} macOS ARM64\n"
        "打开 CPyMO-NDS-Converter.app 转换原版 PyMO 游戏。\n"
        "本应用采用临时签名，未经 Apple Developer ID 签名及 Apple 公证。\n"
        "请确认从可信发布来源获取并核对 SHA-256 后再决定是否打开。\n"
        "若被 Gatekeeper 拦截，可参照 Apple 官方说明在「系统设置 > 隐私与安全性」\n"
        "中检查「仍要打开」；不要全局关闭 Gatekeeper 或系统完整性保护。\n"
        "官方文档：https://support.apple.com/zh-cn/102445\n"
        "第三方来源、构建信息和许可随包提供，请保留对应材料。\n"
        "可运行游戏程序为同目录 cpymo-nds.nds，不含商业游戏。\n",
        encoding="utf8")
    print(json.dumps({"version": metadata["version"], "architecture":args.target_arch,
                      "rom_sha256": digest(rom),
                      "app": str(output / app.name), "macos_package": True},
                     ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
