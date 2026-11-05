# CPyMO for NDS — desktop converter icon

This folder is reserved for the desktop **converter app**, **not** the 32×32 NDS/DSi ROM banner. The owner chose the archived **04_高清像素图标_透明外框.png** on 2026-10-27 after direct 02/04 comparison.

## Canonical source and deterministic derivatives

The approved image is 1254×1254 RGBA with a genuinely transparent outer area. Its **exact original bytes** have SHA-256:

`3f110860ce4e242b79147ccd7b6c6d260efd1a232fa8039b41c02677aa3f8c62`

Before producing an icon-branded public installer, put those original bytes at:

`cpymo-tool/nds-converter/assets/converter-icon-source.png`

Generate build assets with:

```sh
python cpymo-tool/nds-converter/tools/build_desktop_icons.py \
  --source cpymo-tool/nds-converter/assets/converter-icon-source.png \
  --output cpymo-tool/nds-converter/assets/generated
```

This generates a 1024px PNG, 64px Tk PNG, Windows multi-resolution ICO,
macOS Retina ICNS and icon SHA manifest. The archived original stays unchanged.
Only desktop derivatives are horizontally scaled by 0.976 and recentered on
square transparent canvases (visible bounds 1119x1092 become about 1097x1092).
The NDS ROM banner icon is never modified. Windows/macOS packagers can generate
these from the approved source when present, without modifying the DS ROM.

**For the public RC**, pass `--require-desktop-icon` to both
`build_portable.py` and `build_portable_macos.py`: the release build must
stop instead of silently falling back to PyInstaller's default icon.

## Provenance and public distribution

The selected picture is a user-approved high-resolution pixel-style
derivative of a historical PyMO logo/icon design (girl, yellow character,
game controller); the CPyMO project does **not** thereby establish ownership
or commercial redistribution rights to the original visual elements.
Confirm the upstream artwork's applicable permission and include appropriate
attribution or choose separately licensed art **before public redistribution**.

The original high-resolution image remains archived in the user's ChatGPT
Library under `游戏与项目资料/PyMO引擎与跨平台移植/03_NDS图标重制_2026-10-25`. The source PNG is committed in this repository and validated against the pinned digest above.
