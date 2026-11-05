"""macOS frozen .app smoke: real MP4->NVL1, glyph fallback, paired ROM.

Run on an actual macOS CI runner after build_portable_macos.py.
Synthetic assets only; does not attest to Finder/Gatekeeper acceptance.
"""
from pathlib import Path
import argparse
import hashlib
import json
import platform
import plistlib
import subprocess
import sys
import tempfile

from test_converter import fixture, hashes
from release_info import VERSION


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    p=argparse.ArgumentParser()
    p.add_argument("--portable", type=Path, required=True)
    p.add_argument("--ffmpeg", type=Path, required=True)
    p.add_argument("--target-arch", choices=("arm64","x86_64"), required=True)
    a=p.parse_args()
    assert platform.machine()==a.target_arch, f"Host is {platform.machine()}, not {a.target_arch}"
    root=a.portable.resolve()
    app=root/"CPyMO-NDS-Converter.app"
    exe=app/"Contents/MacOS/CPyMO-NDS-Converter"
    assert exe.is_file(), "Frozen .app executable is missing"
    def check_arch(binary):
        out=subprocess.run(["lipo","-archs",str(binary)],check=True,
                           capture_output=True,text=True,timeout=30)
        assert a.target_arch in out.stdout.split(), (str(binary),out.stdout,a.target_arch)
    check_arch(exe)
    rom=root/"cpymo-nds.nds"
    assert rom.is_file()
    metadata=json.loads((root/"VERSION.json").read_text())
    assert metadata["version"]==VERSION
    info=plistlib.loads((app/"Contents/Info.plist").read_bytes())
    assert info["CFBundleVersion"]==info["CFBundleShortVersionString"]==VERSION
    assert metadata["macos_architecture"]==a.target_arch
    assert digest(rom)==metadata["rom_sha256"]
    bundled=app/"Contents/Frameworks/ffmpeg"
    if not bundled.is_file():
        bundled=app/"Contents/Resources/ffmpeg"
    assert bundled.is_file(), "Bundled FFmpeg missing from frozen application"
    check_arch(bundled)
    bundled_hash=digest(bundled)
    expected_hash=metadata.get("ffmpeg_bundled_sha256")
    if expected_hash:
        assert bundled_hash==expected_hash, ("Bundled FFmpeg checksum mismatch",
                                            bundled_hash,expected_hash)
    cfg=subprocess.run([str(bundled),"-buildconf"],check=True,
                       capture_output=True,text=True,timeout=45)
    assert "--disable-gpl" in cfg.stdout and "--disable-nonfree" in cfg.stdout
    assert "--enable-nonfree" not in cfg.stdout and "--enable-gpl" not in cfg.stdout
    with tempfile.TemporaryDirectory(prefix="cpymo-macos-smoke-") as temp:
        base=Path(temp);source=base/"source";fixture(source,"s60v3")
        movie=source/"video/omitted.mp4"
        cmd=[str(a.ffmpeg),"-nostdin","-hide_banner","-loglevel","error",
             "-f","lavfi","-i","testsrc2=duration=1.2:size=320x240:rate=12",
             "-f","lavfi","-i","sine=frequency=440:duration=1.2:sample_rate=44100",
             "-c:v","mpeg4","-q:v","7","-c:a","aac","-y",str(movie)]
        subprocess.run(cmd,check=True,timeout=90)
        original=hashes(source)
        result=base/"result.json";log=base/"log.txt";out=base/"converted"
        job={"action":"convert","source":str(source),"output":str(out),
             "result":str(result),"log":str(log)}
        cfg=base/"job.json";cfg.write_text(json.dumps(job),encoding="utf8")
        done=subprocess.run([str(exe),"--job",str(cfg)],
                            capture_output=True,text=True,timeout=180)
        assert result.is_file(),f"Frozen conversion failed {done.returncode}: {done.stderr[-1200:]}"
        report=json.loads(result.read_text(encoding="utf8"))
        assert done.returncode==0 and report["ok"], (report,log.read_text(errors="replace")[-1400:])
        assert report["release_version"]==VERSION,report
        assert report["videos"][0]["format"]=="NVL1",report
        assert report["missing_policy"]=="box",report
        assert hashes(source)==original,"Source was modified"
        assert digest(out/"cpymo-nds.nds")==digest(rom),"Bundled ROM mismatch"
        assert (Path(report["game_directory"])/"nds-cache/video").is_dir()
        print(json.dumps({"mac_frozen_worker":"PASS","release_version":VERSION,"default_video":"NVL1",
                          "missing_glyph_policy":"box","paired_rom":"MATCH",
                          "source_integrity":"PASS","ffmpeg_license_flags":"LGPL-compatible",
                          "ffmpeg_bundle_sha256":bundled_hash,"native_arch":a.target_arch,
                          "rosetta_required":"NO" if a.target_arch=="arm64" else "Intel native"},ensure_ascii=False))


if __name__=="__main__":
    main()
