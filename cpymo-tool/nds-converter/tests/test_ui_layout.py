"""GUI layout and progress lifecycle checks without requiring an X display."""
import sys
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT))
from app import App


class WidgetStub:
    def __init__(self):
        self.packed=False
        self.options={}
        self.pack_args=None
        self.running=False
        self.scroll_calls=[]
    def configure(self,**kwargs):self.options.update(kwargs)
    def pack(self,**kwargs):self.packed=True;self.pack_args=kwargs
    def pack_forget(self):self.packed=False
    def start(self,interval):self.running=True;self.interval=interval
    def stop(self):self.running=False


class ConverterLayoutTests(unittest.TestCase):
    def setUp(self):
        self.app=App.__new__(App)
        for name in ('zip_button','folder_button','output_button','cancel_button',
                     'convert_button','games','video_box','compression_box',
                     'language_box','advanced_button','progress','status_label',
                     'game_scroll'):
            setattr(self.app,name,WidgetStub())
        self.app.candidates=[]

    def test_progress_is_absent_until_a_job_begins_and_disappears_afterwards(self):
        source=(ROOT/'app.py').read_text(encoding='utf8')
        self.assertIn("self.progress=ttk.Progressbar(frame,mode='indeterminate')",source)
        self.assertNotIn("self.progress=ttk.Progressbar(frame,mode='indeterminate');self.progress.pack",source)
        self.assertFalse(self.app.progress.packed)
        self.app.busy(True)
        self.assertTrue(self.app.progress.packed)
        self.assertIs(self.app.progress.pack_args['before'],self.app.status_label)
        self.assertTrue(self.app.progress.running)
        self.assertEqual(self.app.progress.interval,12)
        self.assertEqual(self.app.games.options['state'],'disabled')
        self.app.busy(False)
        self.assertFalse(self.app.progress.packed)
        self.assertFalse(self.app.progress.running)
        self.assertEqual(self.app.games.options['state'],'normal')
        self.app.busy(True)
        self.assertTrue(self.app.progress.packed)
        self.app.busy(False)
        self.assertFalse(self.app.progress.packed)

    def test_list_height_and_scrollbar_follow_number_of_candidates(self):
        for count,expected_height,scroll in [(0,1,False),(1,1,False),(2,2,False),
                                              (3,3,False),(4,3,True),(8,3,True),(1,1,False)]:
            with self.subTest(count=count):
                self.app.candidates=[{}]*count
                self.app._update_game_list_layout()
                self.assertEqual(self.app.games.options['height'],expected_height)
                self.assertEqual(self.app.game_scroll.packed,scroll)

    def test_video_choice_expands_to_available_width(self):
        source=(ROOT/'app.py').read_text(encoding='utf8')
        self.assertIn("self.video_box.pack(side='left',fill='x',expand=True)",source)


if __name__=='__main__':unittest.main()
