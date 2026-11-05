"""ROM UI must render in every game, not rely on a novel's font subset.

Regression for the confirmed DSi symptoms:
  简体中文: 上屏游戏 / 下屏游戏 / ???屏
  日本語:   上画面 / 下画面 / ?化2画面
"""
from pathlib import Path
import ast
import re
import unittest

ROOT=Path(__file__).resolve().parents[3]
NDS=ROOT/'cpymo-backends/nds/source'


def source_ui_codepoints():
    """Read the converter's literal set without importing Pillow on UI CI."""
    code=(ROOT/'cpymo-tool/nds-converter/convert.py').read_text(encoding='utf-8')
    numeric=set()
    for node in ast.parse(code).body:
        if not isinstance(node,ast.Assign) or not any(
                isinstance(t,ast.Name) and t.id=='UI_CODEPOINTS'
                for t in node.targets):
            continue
        if isinstance(node.value,ast.Tuple):
            numeric.update(int(t.value) for t in node.value.elts
                           if isinstance(t,ast.Constant) and isinstance(t.value,int))
        else:
            for expr in ast.walk(node.value):
                if isinstance(expr,ast.Constant) and isinstance(expr.value,str):
                    numeric.update(map(ord,expr.value))
    return numeric

UI_CODEPOINTS=source_ui_codepoints()

CJK_LABELS=(
    '上屏游戏 / 下屏游戏 / 增强双屏',
    '上屏遊戲 / 下屏遊戲 / 增強雙屏',
    '上画面 / 下画面 / 強化2画面',
)
LANG_FILES=('launcher.h','ui-language-nds.c','ui-language-extra.h')


class RomUIFontCoverageTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        raw=(NDS/'ui-fallback-font.h').read_text(encoding='utf-8')
        blocks={}
        patterns={
            'codes':r'ui_fallback_codes\[\]=\{([^}]+)\}',
            's_adv':r'ui_fallback_advances\[\]=\{([^}]+)\}',
            'l_adv':r'ui_large_advances\[\]=\{([^}]+)\}',
            's_rows':r'ui_fallback_rows\[\]\[16\]=\{([\s\S]*?)\};',
            'l_rows':r'ui_large_rows\[\]\[16\]=\{([\s\S]*?)\};'
        }
        for key,pattern in patterns.items():
            match=re.search(pattern,raw)
            assert match, key
            values=match.group(1)
            if key.endswith('rows'):
                blocks[key]=[tuple(int(n) for n in row.split(','))
                             for row in re.findall(r'\{([0-9,\s]+)\}',values)]
            else:
                blocks[key]=[int(n) for n in values.split(',')]
        cls.table=blocks
        cls.codes={c:i for i,c in enumerate(blocks['codes'])}

    def test_arrays_aligned_sorted_and_not_blank(self):
        t=self.table
        self.assertEqual(len(t['codes']),len(set(t['codes'])))
        self.assertEqual(t['codes'],sorted(t['codes']))
        for key in ('s_adv','l_adv','s_rows','l_rows'):
            self.assertEqual(len(t[key]),len(t['codes']),key)
        for key in ('s_rows','l_rows'):
            for rows in t[key]:
                self.assertEqual(len(rows),16)
                self.assertTrue(all(0<=v<=65535 for v in rows))
        for label in CJK_LABELS:
            for c in set(map(ord,label)):
                self.assertIn(c,self.codes,'missing codepoint U+%04X'%c)
                i=self.codes[c]
                if chr(c).strip():
                    self.assertTrue(any(self.table['s_rows'][i]))
                    self.assertTrue(any(self.table['l_rows'][i]))

    def test_all_literal_multi_language_ui_text_is_in_rom(self):
        # Includes existing screens and future official UI strings.
        for filename in LANG_FILES:
            text=(NDS/filename).read_text(encoding='utf-8')
            for literal in re.findall(r'"((?:\\.|[^"\\])*)"',text):
                for ch in literal:
                    cp=ord(ch)
                    if cp>=32:
                        self.assertIn(cp,self.codes,
                            f'{filename}: U+{cp:04X} {ch!r} missing from ROM UI font')

    def test_future_converter_caches_contain_new_ui_labels(self):
        for label in CJK_LABELS:
            for cp in map(ord,label):
                self.assertIn(cp,UI_CODEPOINTS,
                    f'converter will not embed UI glyph U+{cp:04X}')


if __name__=='__main__':
    unittest.main()
