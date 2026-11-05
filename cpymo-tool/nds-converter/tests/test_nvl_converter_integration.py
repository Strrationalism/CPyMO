"""Integration: actual synthetic PyMO #movie produces hashed playable cache."""
import argparse,hashlib,json,shutil,sys,tempfile,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from test_converter import fixture,hashes
from test_video_nvl import NVLVideoTests
import convert
import helper_core


@unittest.skipUnless(shutil.which('ffmpeg'),'FFmpeg required')
class NVLConverterTests(unittest.TestCase):
    def test_playable_movie_cache_preserves_script_and_source(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);source=root/'source';script=fixture(source,'s60v3')
            video=NVLVideoTests().make_movie(root)
            shutil.copyfile(video,source/'video/omitted.mp4')
            original=hashes(source)
            for mode,magic in [('nvl1',b'NVL1'),('nva1',b'NVA1')]:
                dst=root/mode
                convert.convert(argparse.Namespace(
                    source=source,destination=dst,ffmpeg=shutil.which('ffmpeg'),
                    font=None,jobs=1,missing_glyph_policy='box',cache_only=True,
                    video_mode=mode,video_fps=12))
                convert.verify(dst)
                self.assertEqual(hashes(source),original)
                self.assertEqual((dst/'script/START.txt').read_bytes(),script)
                manifest=json.loads((dst/'nds-cache/manifest.json').read_text())
                self.assertEqual(manifest['audit']['unsupported_video_commands'],[])
                self.assertEqual(len(manifest['videos']),1)
                item=manifest['videos'][0]
                self.assertEqual(item['name'],'omitted')
                self.assertEqual(item['format'].encode(),magic)
                self.assertEqual(item['source_sha256'],original['video/omitted.mp4'])
                movie=dst/item['path']
                self.assertEqual(movie.read_bytes()[:4],magic)
                self.assertFalse((dst/'video/omitted.mp4').exists())
                hashed=convert.key('omitted')
                self.assertEqual(item['path'],f'nds-cache/video/{hashed[:2]}/{hashed}.nvl')


    def test_default_helper_converts_movie_and_warns_for_missing_glyph(self):
        # A first-time player should get working video and best-effort fonts
        # without enabling advanced checkboxes or passing --video-mode.
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);source=root/'source';fixture(source,'s60v3')
            shutil.copyfile(NVLVideoTests().make_movie(root),source/'video/omitted.mp4')
            script=source/'script/START.txt'
            script.write_text(script.read_text(encoding='utf8')+'\n#say 🦄\n',encoding='utf8')
            original=hashes(source)
            report=helper_core.run_job({
                'action':'convert','source':str(source),
                'output':str(root/'result'),
                'result':str(root/'jobs/result.json'),
                'ui_language':'English',
                'ffmpeg':shutil.which('ffmpeg')})
            self.assertTrue(report['ok'],report)
            self.assertEqual(report['video_policy'].split()[0],'playable')
            self.assertEqual(report['videos'][0]['format'],'NVL1')
            self.assertEqual(hashes(source),original)
            self.assertEqual(report['missing_policy'],'box')
            english=(Path(report['output'])/'INSTALL-READ-ME.txt')
            self.assertTrue(english.is_file())
            self.assertIn('Video mode: NVL1',english.read_text(encoding='utf8'))
            self.assertIn('Back up the existing save',english.read_text(encoding='utf8'))
            dst=Path(report['game_directory'])
            manifest=json.loads((dst/'nds-cache/manifest.json').read_text(encoding='utf8'))
            boxed={c for data in manifest['pixel_fonts'] for c in data['boxed_codepoints']}
            self.assertIn('U+1F984',boxed)
            self.assertTrue(any('U+1F984' in item for item in report['warnings']))
            self.assertIn('🦄',(dst/'script/START.txt').read_text(encoding='utf8'))
            convert.verify(dst)


if __name__=='__main__':unittest.main()
