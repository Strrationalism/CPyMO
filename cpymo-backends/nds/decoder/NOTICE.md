# libmad for CPyMO NDS

Upstream: devkitPro/libogc, libmad directory (Underbit Technologies libmad 0.15.0 beta).
https://github.com/devkitPro/libogc/tree/master/libmad

Original source copyright notices and GPL version 2 or later terms are retained.
COPYING contains GPL-2.0 text obtained from https://raw.githubusercontent.com/sezero/libmad/master/COPYING after the GNU site returned HTTP 403.

Original source identities and URLs: source-manifest.json (Git blob SHA1 and SHA256).

Local NDS changes: global.h selects the existing FPM_ARM implementation instead of FPM_PPC; config.h enables newlib's assert.h and libmad's built-in OPT_SPEED/OPT_SSO synthesis option; gctypes.h is an NDS type compatibility shim. No codec mathematics or tables were rewritten. Build selects ARM instructions for the decoder's multiply assembly. The unrelated libogc mp3player.c is not included.

NDS streaming integration is in ../source/audio-nds.c. This is a source integration, not installation of a precompiled or legacy toolchain.
