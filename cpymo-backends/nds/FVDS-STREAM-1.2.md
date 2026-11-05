# FVDS stream core — CPyMO 1.2 integration milestone

Branch `codex/nds-v1.2-fvds-stream-core` derives from the hardware-validated R7-derived video foundation. It contains a **cleanly authored, read-only standard-C packet reader** (not a video decoder). It does not import or relicense FastVideoDS player/encoder code, copy copyrighted media or assume real hardware decode is functional.

## Implemented API

- `source/fvds-stream-nds.h/.c`: open/validate FVDS header and monotonic keyframe index; bound frame/audio packet sizes, check data against file boundaries; read separate video/audio chunks sequentially into caller-owned buffers; finish/skip current packet and reposition by keyframe index; reject trailing bytes when reached EOF. It uses existing `stdio.h` device paths and does **not** allocate heap buffers, change ARM7, FIFO, IRQ, DMA, VRAM, Maxmod, LCD state or game saves.
- `cpymo-tool/nds-converter/tests/test_fvds_stream.py`: compiles this same C reader on a host with GCC and passes synthetic three-frame samples into it. Exercises valid frame-by-frame reads, keyframe seek, misordered header index, malformed video alignment and trailing-packet bytes. The original `fvds_inspect.py` remains a separate, stricter host preflight tool.
- Private **host-only**, memory-sanitized test on original user handoff media: j1 159 seconds (1908 frames, 12,935,012 payload bytes, FNV1a32 56159179); 1 second (12, 66,248, D7819B62); 3 seconds (36, 176,072, 2898659E). All matched the user's prior NDSi TWL *read-only* logs. A separate seek to keyframe index 2 correctly resumed at frame 502. **No ROM/hardware streaming test for the new API yet**.

## Resource and release boundaries

FVDS stream I/O is **not connected to CPyMO `#movie` playback** and provides **no decoded RGB555 frames or audible ADPCM**. Merely compiling this source and passing host corpus checks does not fulfill the 1.2 FVDS movie feature.

Next contract for a real player:

1. Design an original decoder that consumes `nds_fvds_read_video` without copying unlicensed FastVideoDS source. Video is a 256×192 DCT/motion-compensated stream; validate pixel reconstruction and output bandwidth on NTR 4 MiB before betting on TWL-only RAM. Keyframe seek must reset all motion/reference state.
2. Integrate original compatible FVDS stereo ADPCM playback at **47,605 Hz**, with Calico/Maxmod ownership explicitly resolved; do not assume existing mono 22,050 Hz NVA1 can play FVDS audio or that a header edit is enough.
3. Coordinate dual-screen graphics. The existing CPyMO bitmap renderer owns VRAM A/C; the reference FVDS player requires its own GX/VRAM/memory and ARM7 audio infrastructure. The chosen decoder must not silently clobber these. A bounded, fully-restoring resource lease needs real NTR/TWL hardware tests.
4. Integrate `#movie` lifecycle with B-skip, natural EOF, malformed/unreadable resource, lid close/open, clean return to script/BGM and screen mode persistence. Leave ordinary converter defaults and released R7 intact.
5. Once fully verified, merge into the common 1.2 integration branch alongside classic/lower-game/enhanced dual-screen. No premature 1.2 tag, no release to end users.

Licensing: FastVideoDSPlayer issue #15 requesting licensing is still open at review; its permission must not be assumed. Public CPyMO code here contains no player/encoder implementation from that repository.
