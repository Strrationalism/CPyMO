"""Regression for the 1.2 owner-approved help text and unmodified LCD landmarks.

This intentionally checks the authored help layout rather than rasterizing a
different font or pretending to be a physical-screen capture. The NDS SDK
build still validates compilation with all four actual interface languages.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT / "cpymo-backends/nds/source"


class NDSHelpGuideTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.launcher = (SOURCE / "launcher.h").read_text(encoding="utf-8")
        cls.panel = cls.launcher.split("static void launcher_play_panel(", 1)[1].split(
            "static void nds_clock_poll(", 1
        )[0]
        cls.i18n = (SOURCE / "ui-language-nds.c").read_text(encoding="utf-8")
        cls.extra = (SOURCE / "ui-language-extra.h").read_text(encoding="utf-8")

    def test_guide_has_owner_approved_controller_labels(self):
        panel = self.panel
        for msg in ("A / DOWN：继续", "UP：历史对话", "X：菜单 · B：返回",
                    "B：按住隐藏文字", "Y：隐藏／唤回辅助屏",
                    "R：快进 · L：切换显示模式",
                    "上屏游戏 / 下屏游戏 / 增强双屏",
                    "START：快速保存  SELECT：快速读取"):
            self.assertIn(f'nds_ui("{msg}")', panel, msg)
        self.assertNotRegex(panel, r'%u/3|[123]/3')
        self.assertNotIn("存档1", panel)
        self.assertIn('launcher_text(message&&*message?nds_ui(message):nds_runtime_mode_label(),', panel)
        self.assertNotIn('launcher_text(nds_ui("SELECT：快速读取")', panel)
        self.assertNotIn('显示、字号、音量：X 菜单', panel)

    def test_landmarks_and_conservative_clip_areas_unchanged(self):
        panel = self.panel
        self.assertIn("launcher_game_icon(&launcher_games[launcher_selected],196,40,true)", panel)
        self.assertIn("nds_clock_paint_impl(framebuffer,196,96,", panel)
        positions = re.findall(
            r'launcher_text\(nds_ui\("([^"]+)"\),12,(\d+),(?:white|accent|muted),(\d+)\)',
            panel
        )
        self.assertEqual([int(y) for _, y, _ in positions],
                         [32, 50, 68, 86, 104, 122, 140, 158, 32, 50, 68, 86, 104, 122])
        for _, y, max_x in positions:
            if int(y) < 114:
                self.assertLessEqual(int(max_x), 183)
            else:
                self.assertLessEqual(int(max_x), 245)
        self.assertIn('12,176,message&&*message?accent:muted,245)', panel)

    def test_translation_index_count_and_no_missing_new_keys(self):
        source = self.i18n
        section = source.split("static const struct", 1)[1].split(
            '#include "ui-language-extra.h"', 1
        )[0]
        pair_count = len(re.findall(r'\{"(?:[^"\\]|\\.)*","(?:[^"\\]|\\.)*"\}', section))
        self.assertEqual(pair_count, 89)
        self.assertIn("extra_ui[2][89]", self.extra)
        for value in ("A / DOWN：继续", "UP：历史对话",
                      "R：快进 · L：切换显示模式",
                      "上屏游戏 / 下屏游戏 / 增强双屏",
                      "START：快速保存  SELECT：快速读取", "SELECT：快速读取"):
            self.assertIn(f'{{"{value}",', section)
        cht = '{"正在讀取' + self.extra.split('{"正在讀取', 1)[1].split('{"ゲーム一覧', 1)[0]
        jap = '{"ゲーム一覧' + self.extra.split('{"ゲーム一覧', 1)[1].split("static error_t jp_saved", 1)[0]
        for localized in (cht, jap):
            self.assertEqual(len(re.findall(r'"(?:[^"\\]|\\.)*"', localized)), 89)


if __name__ == "__main__":
    unittest.main()
