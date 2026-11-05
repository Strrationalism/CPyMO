# NDS 1.0.0 validation

This release is based on upstream core 5a746bc72a4a8c127e0aee7deb20bf415c385ae2
and the user-accepted 0.5.6.3 NDS baseline. No platform tier is assigned here.

Local build: devkitARM r68 / GCC16.1, Calico1.2.0, libnds2.0.2,
libfat-nds2.1.0-4 (libdvm), Maxmod2.1.0, ndstool2.3.1.
The ROM is 710656 bytes. The portable helper embeds the identical ROM and
release metadata; output copies are checked by SHA-256.
An independent complete source snapshot built in a different directory with
the same SDK produces the identical ROM SHA-256 after source-path normalization.

The local headless frontend uses the unmodified official melonDS1.1 core,
direct boot, DLDI-patched FAT storage, and DS MainRAMMask=0x3fffff (4MiB).
Checks include the early startup frames/library, FAT failure, dialogue,
B/X/Y/L/touch controls, per-game read-text dimming, native fonts,
standard slot1 save/load, and audio playback. Malformed or absent video
caches are skipped and the script continues into normal dialogue/audio.

Both complete existing Narcissu1/2 converted datasets were reused for focused
startup/dialogue/menu/save/load checks. Both passed with zero allocation
failures, I/O failures, missing glyphs and raw image loads. Test-modified saves
were restored byte-for-byte; no complete game-data copy was created.
These checks do not certify all chapters or unrelated PyMO games.

The frozen Windows helper was run with PATH containing only Windows System32
and without PYTHONPATH/PYTHONHOME. GUI selection/conversion and nested bundles
containing S60v3, S60v5 and Android synthetic inputs passed. Original archive
hashes and script bytes were preserved, input saves excluded, videos omitted
with audit records, and all output ROM hashes matched. An actual converted
synthetic game then displayed Chinese, played PCM audio and skipped its video
command in stock DS-mode melonDS. The bundled FFmpeg additionally decoded a
real MP3 from the existing game dataset.

Reproducible converter regression checks (synthetic fixtures, no game assets):

```sh
python -m pip install -r cpymo-tool/nds-converter/requirements.txt
python -m unittest discover -s cpymo-tool/nds-converter/tests -v
```

FFmpeg must be on PATH, or set NDS_TEST_FFMPEG to the audio-only executable.
Tests verify all three input platforms, packed S60v3 backgrounds, nested ZIP/APK
selection, unsafe-path rejection, video omission, source/script preservation,
detection of corrupted generated audio, and packed/loose uppercase Symbian
_MASK transparency. Four synthetic unit tests are provided.

The user gave final acceptance on an R4 with Wood kernel on 2026-10-11.
The accepted pre-release was listfont-r3 (SHA256
`e453644899cf6020f3098e7c229f1cf2f3b874494553a83e5715261fb012b262`).
Version 1.0.0 promotes the same engine implementation with release metadata
updated. Lid mute/resume was tested on
hardware during baseline development; historical automated lid
tests used a separately documented emulator scheduling fix. Stock melonDS
checks must not be presented as new hardware or stock-emulator lid certification.

Known first-release limit: 22050Hz mono PCM16 and retained original non-video
media increase installed game size. Video, compression and DSi enhancements
remain separate later work. Conversion audits are not compatibility guarantees.
The root README and upstream all-platform version are unchanged.

## Final pre-release fixes

List rows use each game own native Fusion Pixel 12px dialogue glyphs and
advances, ignoring stale title bitmap styles. Actual-card metadata/fonts
reproduced the older Narcissu title style; list and title-as-dialogue pixels
match exactly for three games across selection, refresh and About return.
Global character-effects OFF and cross-game/reboot persistence have separate
synthetic coverage. The portable EXE also verifies uppercase packed/loose
mask alpha pixels and input preservation. User feedback includes Narcissu1/2
and Haruka play checks; it is not a new all-route certification.
