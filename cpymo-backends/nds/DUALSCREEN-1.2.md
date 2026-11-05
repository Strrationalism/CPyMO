# CPyMO NDS / DSi 1.2 · reversible 3DS-inspired dual-screen prototype

Base: `codex/nds-v1.2-video-foundation`; official R7 / `nds-v1.1.0` releases remain untouched. This feature belongs in 1.2 only after real NTR and TWL hardware acceptance.

## Modes

- **Classic (default):** unchanged R7 single-picture play on the main LCD; the other LCD shows the existing localized control/clock panel. Existing A, B, X, Y, START, SELECT and L behavior is retained.
- **Enhanced dual-screen (opt-in):** the physical **upper LCD** displays the scene (background, sprites and scene effects), the physical **lower LCD** displays the in-game dialogue box and text, or the core UI menu while open. The renderer partitions existing `cpymo_backend_image_draw_type` categories at the backend boundary, matching the *behavioral* principle of upstream CPyMO 3DS. Original 3DS code was not copied.
- **Switch:** while playing, press **L + SELECT** to toggle classic/enhanced without exiting the story, restarting, or rewriting script assets. The old quick-load remains on SELECT alone; L alone swaps LCDs in classic mode and does not invert the mandatory top/bottom roles in enhanced mode. Y still hides/unhides the auxiliary LCD; a hidden lower LCD temporarily uses classic dialogue rendering on the visible main LCD, so game text remains readable. Re-enable Y to restore enhanced layout.
- **Saved preference:** `<game>/save/nds-dualscreen-mode.txt` contains `1` (enhanced) or `0` (classic). Absent/invalid content means classic. This is separate from core game save slots and R7's `nds-ui.cfg`; no core or asset format changes. Failed preference write leaves gameplay functional.

## Implementation

- Main renderer keeps the existing framebuffer and VRAM bank owners. The top scene is drawn and DMA-published first. With enhanced mode enabled, the same RAM framebuffer is cleared, and only CPyMO's lower-screen dialogue/menu draw types are rendered before publishing to the pre-existing sub-screen VRAM. This deliberately avoids allocating another 256×192×RGB555 heap buffer.
- The low-level NDS image, rectangle and text drawing entrypoints filter by CPyMO draw types, keeping base-image caching and audio service logic independent. The dialogue's baseline shifts upward to bring the original name/textbox into the lower LCD's useful area. The core's game scripts are unchanged.
- Existing 3DS reference has a 400×240 upper and 320×240 lower screen; **DSi has 256×192 on both**. Therefore do not assume 3DS offsets/zoom/texture sizing will directly match; a local 20%-height dialogue target is provisional, to be tuned only after physical display results.
- Classic auxiliary help and minute-clock updates are suppressed while enhanced mode owns the lower screen. Y hide and mode changes trigger a complete redraw to avoid orphaned dialogue / old help panel pixels.
- All interactions remain in the established CPyMO main loop; no new libnds VRAM mapping, IRQ/FIFO ownership, ARM7 services, or exclusive video GPU use is introduced.

## Real-hardware acceptance — not yet run

Use a converted **existing** PyMO game, ideally the previously accepted 30 MB dual-card fixture or a known NVL1 demo. Test one play session:

1. Start with **classic** (delete `save/nds-dualscreen-mode.txt` only if an earlier prototype enabled dual; no other saves must be deleted). Confirm original scene, controls, time and launcher appearance.
2. While a normal name + dialogue box is visible, press **L+SELECT**. Upper: scene/characters without dialogue. Lower: the current name/dialogue box and text. Press A to advance and check multi-line dialogue/auto text.
3. Press **X** to open the CPyMO menu. Confirm menu content and controls remain usable on the lower LCD; press B to return. Check menus, choices, speaker names and scene transitions.
4. Press **L+SELECT** again. Confirm classic restores immediately, dialogue returns to original main LCD, lower help panel returns and SELECT standalone still quick-loads.
5. Turn enhanced on, then press **Y** to hide the auxiliary screen. The top LCD should still show the dialogue in temporary classic mode; press Y again to restore enhanced lower dialogue. Confirm ordinary **L** still swaps screens in classic and does not swap top/bottom enhanced roles.
6. Close and reopen the *same game*. Confirm the previously selected mode persisted. Restart test with both an original NTR build and DSi TWL build, and with the game stored in `sd:` or `fat:` according to the confirmed *directional* storage limitations.
7. Run `#movie` both in classic and enhanced. Check video is still visible, after EOF or B skip the game returns to the original chosen dual/classic mode with music intact. **FVDS remains I/O preflight only**, not playback.

**Do not ship 1.2 on CI success alone.** Any UI alignment, speaker box placement, menu functionality or performance regressions on NTR 4 MiB must be fixed first. The established stable R7 visual layout is the unchanged rollback.

## Deferred larger 1.2 goal: FVDS actual playback

The user's private video research established sequential j1 video compaction versus NVL1 on ordinary NTR, but only FVDS **read-only** path has been verified through CPyMO on DSi. 1.2 still needs licensed or original FVDS frame decoding (including texture/GX and RAM constraints) and 47,605 Hz stereo ADPCM audio integration while preserving the CPyMO Calico/Maxmod ARM7, libfat double-volume behavior, lid suspend/resume, B skip and script return. The FastVideoDS repository has no clear source redistribution license; do not copy its player implementation into AGPL CPyMO pending authorization.

Both major features are separately gated; neither may silently replace the classic visual-novel experience.

## Follow-up: actual DSi hardware feedback and corrected three-mode touch model (2026-10-22)

The user physically tested the first dual-screen build on DSi TWL: the split screen's baseline works and the movie's **B skip** returns successfully. The earlier user accepted seven-video batch reached EOF, so this separate B-skip test closes that specific input path in DSi mode. The first enhanced layout, however, was visually too bare and needs further polish relative to the actual 3DS behavioral reference.

**Updated user-approved L cycling:** every standalone **L** press cycles through three physical LCD assignments:

| State | Main gameplay | Secondary display | Lower-LCD touch semantics |
| --- | --- | --- | --- |
| 0 (default) | Classic game on upper LCD | Original control+clock panel below | Entire lower panel acts as virtual **A**; NEVER project touch to upper menu coordinates, even while a core menu is open |
| 1 | Classic game on lower LCD | Original control+clock panel above | **Full PyMO coordinate projection** from physical lower LCD, including normal dialogues, choices and in-game menus |
| 2 | Enhanced split | Background/characters on upper LCD; dialogue or engine menus below | Tap narrative dialogue to advance (**A**); project physical lower touches to genuine menu coordinates when the core UI is drawn below |

The next L after state 2 returns to state 0; no more awkward **L+SELECT** chord. Single SELECT retains quick-load. Single Y retains hide/reveal of the auxiliary display and the temporary single-screen legibility policy. A transition explicitly resets auxiliary hide so the newly active LCD is visible. Classic upper or lower uses the exact original rendering, not a duplicated enhanced composition.

The per-game `save/nds-dualscreen-mode.txt` now stores `NDSDISPLAY2 0`, `NDSDISPLAY2 1`, or `NDSDISPLAY2 2`, retaining backward compatibility: the previous one-byte `1` means the prior prototype's enhanced split (now state 2). No core save slots, game config, cache formats, or users' R7 installations are changed. Localized auxiliary help now says L cycles display modes.

**Accept only after next real-device trial**, specifically all six transitions 0→1→2→0 and menu coordinate interaction in state 1/state 2. The existing simple enhanced dialogue layout is a first step rather than an accepted final 3DS-like screen composition. Determine text box safe area, choice routing, legibility and save menu UI from the DSi real hardware before release. B skip and audiovisual playback should be regressions in all three states.

## Unreleased visual/layout and choice routing refinement

The first three-mode test binary was intentionally **not** hardware-tested at the user's request. The following changes have since been implemented only in this experimental branch:

- **Persistent scene when menus open:** previous enhanced routing could blank the top LCD as soon as a CPyMO menu became active, since a UI draw pass does not emit normal background/character categories. The enhanced top path now explicitly draws the scene while the lower LCD owns the actual UI/menu draw types.
- **Story choices on the touchable lower screen:** story `cpymo_select_img` objects use `cpymo_backend_image_draw_type_sel_img`. When those choices are active, they now render on the lower LCD, and that lower touch surface supplies real PyMO coordinates. Outside menus/choices, lower touch remains a full-size **A** action, not a falsely projected cursor. Classic lower-game keeps precise touch coordinates; classic upper-game keeps full-panel A.
- **More legible lower-screen composition:** a native RGB555 muted background, restrained border and dialogue accents sit under the game's original textbox, speaker label and text; UI/choices use less decorative space. This is deliberately different from a stretched second copy of the top screen and uses the existing framebuffer and VRAM owners.
- **Correct per-game preference serialization:** fixed escaped newline handling for `NDSDISPLAY2 0/1/2`, allowing the saved three-mode preference to reload properly; the one-byte first-prototype `1` retains its enhanced-mode migration rule.
- **Build verification:** the NDS/DSi and converter workflow passed on the visual/layout change commit `ff9b450a7b10093d24c2fe0f7c5cfa5ff2e0d21d`. The later preference-only patch ran converter tests successfully, while the DS build job could not reach compilation because the upstream devkitPro package index returned HTTP 403. A green previous build does **not** replace physical dual-screen acceptance.

Next hardware test remains deferred until combined with a meaningful FVDS decoder/audio development milestone. Test the choice hitboxes, X menu transitions, L three-mode cycle, save preference persistence, original audio/video path and B-skip before qualifying 1.2.


## Visual refinement — CPyMO 3DS behavior verified against original code (2026-10-22)

The user requested that enhanced mode **look intentional and near the original 3DS implementation**, not merely satisfy the two-screen functionality. On reading the actual upstream `cpymo-backends/3ds/main.c`, `cpymo_backend_image.c` and `cpymo_backend_text.c`, the precise relevant behavior is:
- The 3DS scene uses the top screen while the **game's own message box, name box and say text** render on the lower screen, with a vertical offset derived from the original gameconfig and message-box dimensions (original 3DS top-margin formula uses a logical `+16`).
- **In CPyMO UI mode, top gets `ui_bg` and bottom gets `ui_element` / `ui_element_bg`**, not the entire interface background on the lower screen. NDS previously misrouted `ui_bg` to bottom and forced the storyline scene on top, so this has now been corrected to use 3DS's real categories.
- 3DS defaults to enhanced in the upstream implementation but NDS **intentionally remains classic by default** with L cycling classic-top → classic-bottom → enhanced; the user explicitly requested retaining these three choices.
- The original 3DS normal-story rendering actually keeps `sel_img` on top, whereas NDS intentionally routes story choices below for direct touchscreen selection. This is a **documented practical NDS improvement**, not presented as a verbatim 3DS clone.

1. New `dual-layout-nds.h` computes a game-resolution-aware lower-screen vertical offset from the original message artwork height, the name box font height and name-box origin. Dialogue is anchored around **14 physical pixels** below the top of NDS's 192px screen, instead of the old arbitrary **20% logical-height** position. When artwork is too tall, the helper deliberately leaves its original position intact rather than increasing clipping. The same shift applies to game dialogue artwork, translucent fallback rectangle, speaker name and dialogue text.
2. Enhanced lower screen now has an understated native RGB555 dusk gradient and a compact, localizable footer with `A / 轻触：继续` in narrative mode, rendered **under** the game's own textbox. Choice/menu modes do not draw the narrative footer, keeping the lower screen free for direct touch targets.
3. Native `cpymo_engine_draw` is now allowed to draw the menu's authored background on the upper display, while low-level draw-type dispatch sends the interactive elements to the lower display. This matches the upstream 3DS split more precisely than the first prototype. `nds_dual_force_scene` was removed as dead state.
4. Added `test_nds_dual_layout.py`: compiles the actual layout header with host GCC and checks geometry for multiple original game resolutions, letterboxing, with or without name labels, adjusted name origins and oversized authored dialog assets.

**The actual NDS visual output remains unverified on hardware.** The next visual acceptance must inspect a game with a real namebox/messagebox, a game without custom assets, multiple-line names/text, an X settings/backlog menu, script choices, L mode cycling, touch targets, Y hide/reveal, return to library and game switching. Do not state "3DS-quality visuals accepted" merely because the source composition and CI build succeeded.

Plan remaining hardware sessions as **roughly 2–3 concentrated rounds**, not a fixed guaranteed number: (1) 3DS-inspired visual/UI layout checkpoint; (2) actual FVDS decoded video/audio/skip checkpoint once it exists; (3) optionally final candidate regression on both TWL DSi and NTR DS mode. Any hardware-only discrepancy or video decoder performance issue may require another focused round. Stable R7 remains untouched.


## Authentic visual captures — hardware review path (opt-in, unreleased)

The user explicitly rejected AI-generated concept art as a proxy for the actual CPyMO interface. Only genuine program-rendered images should be presented as UI evidence.

A new optional **NDSPIX1** capture facility provides this without bundling an emulator, inventing a scene or introducing a second runtime framebuffer:
- In the selected game's existing `save/` folder, the tester creates an empty `nds-visual-capture.flag` file **before launching the game**. Without the file, no frame captures, no additional SD writes and no extra framebuffer allocation occur.
- While the game is in **enhanced dual-screen mode**, the first actual dialogue redraw and the first actual core UI/menu redraw each generate one `nds-capture-dialogue.ndsraw` or `nds-capture-menu.ndsraw`. Each file contains a 16-byte `NDSPIX1` header and exactly two 256×192 RGB555 screen buffers in **physical upper, then lower screen** order. The saved pixels are the real buffers used for VRAM presentation, not high-resolution art or speculative reconstructions.
- The source ROM exports actual top/bottom pixels on the ARM9. The local host script `python cpymo-backends/nds/tools/nds-capture-to-png.py <path-to-ndsraw>` produces `-top.png` and `-bottom.png` at **exact native 256×192** resolution, standard-library only, without scaling, borders or decorative changes.
- Capture is **opt-in, diagnostic only**. Each capture writes ~192 KiB to SD and may momentarily pause the game; disable it for performance/sound/video timing tests. It does not capture NVL1/FVDS movie decoding, and cannot prove 3DS visual fidelity without comparing actual CPyMO 3DS runtime evidence.
- Separate SDK-independent regression tests compile and exercise the production C writer with synthetic RGB555 *test vectors* and check the exact raw format + PNG conversion. Those synthetic buffers are **never presented as screenshots of CPyMO gameplay**.
- The current environment has no accessible running DS emulator. **No actual 1.2 enhanced-mode image has yet been captured**, and nothing in this toolchain should be described as an authentic visual preview until the raw pixel dump comes from the physical DSi or from a running emulator.

This capture feature will be folded into the user's already-planned first concentrated UI/visual acceptance round; no separate immediate hardware testing is requested. Stable R7 is not changed.


## Source-executed lower-LCD visual preview (without game art)

The user asked for visual proof when an untested 1.2 UI change is made and explicitly prohibited AI image generation. The lower-LCD RGB555 backdrop has therefore been extracted from `main.c` into **the exact same production** `source/dual-canvas-nds.h` static C function used at runtime. `main.c` calls it before rendering the game's original namebox, textbox, glyphs, choices and menu.

`python cpymo-backends/nds/tools/nds-dual-canvas-preview.py --output <directory>` builds that actual C function with a normal GCC host compiler, executes it for dialogue/menu/idle states, and converts its RGB555 buffers into three native 256×192 PNG images using the existing checked `nds-capture-to-png.py` logic. The separate SDK-free UI CI uploads these files as a visual artifact only at a manually requested checkpoint.

**Accurate scope:** the three PNGs show ONLY the screen's underlying gradient, accents and dialogue footer band, *not* the user's actual PyMO game textbox, Chinese font glyphs or character sprite. They are exact source-executed backdrop pixels but emphatically **not NDS game screenshots**, and may not be labeled or presented as complete enhanced-screen visuals. The complete result still requires the user's or emulator's first real `NDSPIX1` capture from the actual game. This design preserves an honest source-driven visual preview now while deferring full visual acceptance to the already-planned concentrated device round.

## 2026-10-22 — one-session NDSPIX1 physical display evidence for the three-mode trial

The opt-in `nds-visual-capture.flag` now collects **four** first-occurrence,
pixel-exact physical screen pairs during game redraws:

| Kind | Output (.ndsraw, in the same save directory) | Trigger |
| --- | --- | --- |
| 1 | `nds-capture-dialogue.ndsraw` | Enhanced split: original visible dialogue |
| 2 | `nds-capture-menu.ndsraw` | Enhanced split: core UI/menu |
| 3 | `nds-capture-choice.ndsraw` | Enhanced split: active story choices (priority over lingering dialogue) |
| 4 | `nds-capture-classic-lower.ndsraw` | Classic lower: visible dialogue, physical upper help panel and lower game frame |

The latter explicitly writes **sub VRAM as physical TOP and main framebuffer as
physical BOTTOM**, since `lcdSwap()` reverses their physical assignments.
Enhanced captures still write the published main scene TOP and actual
lower UI BOTTOM. Every file remains one `NDSPIX1` header plus two native
256x192 RGB555 buffers; the converter accepts all four kinds without
rescaling. The C exporter and PNG converter have expanded synthetic
round-trip regression tests. These fixtures are **not** gameplay screenshots.

This enables comparison of layout and touch target position within one
optional DSi session; no capture occurs without the explicit flag. Each
192-KiB snapshot is an intentional synchronous FAT diagnostic and may cause
momentary pauses, so remove the flag during performance/audio measurements.
A missing state will not fabricate an image. Neither the two previously
compiled ROMs nor any user's saves or stable R7 binaries are modified by
this source-level change. Rebuild and then verify true captures on hardware;
do not claim actual 3DS-equivalent quality from synthetic tests.
