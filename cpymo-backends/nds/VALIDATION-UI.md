# NDS 1.1.0 UI validation (2026-10-20)

- Four UI languages: Simplified Chinese, Traditional Chinese, English, Japanese.
- Visible order: Auto, 简体中文, 繁體中文, English, 日本語. Saved IDs directly follow this order: 0=Auto, 1=CHS, 2=CHT, 3=EN, 4=JA. No legacy-ID remapping, as explicitly requested.
- First normal boot defaults to CHS. Saved choices persist. Auto recognizes firmware Japanese/English/Chinese; unsupported/invalid firmware falls back to CHS. Before FAT, firmware/fallback works from ROM-only glyphs.
- Final version string and banner: 1.1.0. ROM: 807936 bytes, SHA256 `af39d5c905e4fee2281e6138818416f42e533e8f306074af007599db80f88c45`.
- 19 final-ROM tests passed on stock melonDS 1.1, ordinary DS 4 MiB: language UI order/cycling, four-language menus/settings/save-load, saved CHT/JA/EN and Auto, first boot and automatic language fallbacks, three FAT error locales, game/menu clock switching, midnight/invalid RTC, and four-game library header updates.
- Library lower-screen title contains only the translated Select a game text; X language help is only in the footer. All four title variants match the approved native Fusion 12px glyphs exactly (five pixel checks including Auto). The legacy 13px title path and the visible language diagnostics entry are removed. SELECT on About does nothing; B returns.
- ROM-only fallback contains 584 glyphs in 12/16px sizes. Added languages affect engine UI only, not scripts or game metadata.
- Clock is shown once in gameplay: auxiliary normally, menu panel while X menu is open. L/Y and minute updates preserve this rule. Child settings/save pages keep their existing layout and auxiliary clock.
- User confirmed the final title-fixed 1.1.0 short test on their Wood R4: "已测，正常" (2026-10-20). The release rebuild is byte-identical to that ROM. This is not full-route/all-flashcart certification.
- Local test records and screenshots are retained outside the repository. No user game media, saves or runtime binaries belong in a source PR.


## Release integration checks

- Fresh build in the PR integration tree: all 55 objects rebuilt; ROM hash identical to the accepted artifact above.
- Five Python converter regression tests, including actual worker defaults, video omission, source/script preservation, packed masks, archive traversal and integrity checks.
- Fresh Windows portable build includes matching root/embedded ROM, audio-only FFmpeg, LZ4 and rebuilt native lossless encoder.
- Real frozen GUI default conversions of synthetic S60v3/S60v5/Android nested archives pass with PATH restricted to Windows System32, no external Python/FFmpeg. Actual jobs use cache_only=true and audio_lossless=voice.
- All three fresh outputs boot on stock melonDS 1.1 in DS/4 MiB mode: images and audio work, movie instructions skip, no allocation/I/O/glyph errors. Test images are removed after completion.
- Existing 1.0 caches remain readable; save formats/paths and original scripts are unchanged. New compressed caches require 1.1.
- Video decoding is still experimental source code; the public converter omits videos. Default BGM/SE are PCM. No universal compatibility claim.
- NDS-only contribution: upstream root README, global version and other backends are not modified. No game data or user saves are distributed.

FAT failures render the diagnostic/error code above and recovery hints below using ROM-only glyphs; historical duplicate-frame display was intentional mirroring. Firmware detection reads Calico/libnds g_envUserSettings / PersonalData, never the R4 menu language or firmware writes. Stored manual settings can only be read after FAT succeeds.
