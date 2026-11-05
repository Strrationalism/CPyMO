"""Synthetic input/output checks; no copyrighted game files. MIT."""
from pathlib import Path
import argparse
import hashlib
import io
import json
import math
import os
import shutil
import struct
import sys
import tempfile
import unittest
import wave
import zipfile
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import convert
import helper_core

def fixture(folder, platform, packed=False):
    folder.mkdir(parents=True)
    for name in ['bg','bgm','script','video','save']:
        (folder/name).mkdir()
    (folder/'gameconfig.txt').write_text(
        'gametitle,合成测试\nplatform,'+platform+'\nimagesize,320,240\n'
        'scripttype,pymo\nstartscript,START\nengineversion,0.5\n'
        'bgformat,.png\ncharaformat,.png\nbgmformat,.wav\nvoiceformat,.wav\n'
        'seformat,.wav\nbgmvolume,5\nvovolume,5\nplayvideo,1\n'
        'fontsize,16\ntextcolor,#ffffff\n',encoding='utf8')
    script=b'#bg scene,BG_NOFADE\n#bgm tone\n#movie omitted\n#label LOOP\n#say '+ '视频略过后继续。'.encode() + b'\n#goto LOOP\n'
    (folder/'script/START.txt').write_bytes(script)
    (folder/'video/omitted.mp4').write_bytes(b'synthetic video must not enter output')
    (folder/'save/existing.csav').write_bytes(b'input save must not be imported')
    image=io.BytesIO();Image.new('RGB',(320,240),(50,90,120)).save(image,format='PNG')
    if packed:
        data=image.getvalue();(folder/'bg/bg.pak').write_bytes(struct.pack('<I32sII',1,b'scene',44,len(data))+data)
    else:(folder/'bg/scene.png').write_bytes(image.getvalue())
    with wave.open(str(folder/'bgm/tone.wav'),'wb') as wav:
        wav.setparams((1,2,22050,22050,'NONE','not compressed'))
        wav.writeframes(b''.join(struct.pack('<h',int(3000*math.sin(i*2*math.pi*440/22050))) for i in range(22050)))
    return script

def hashes(folder):
    return {p.relative_to(folder).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in folder.rglob('*') if p.is_file()}

class ConverterTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which('ffmpeg') or os.environ.get('NDS_TEST_FFMPEG'),'FFmpeg required')
    def test_helper_release_defaults_preserve_scripts_and_compress_voice(self):
        with tempfile.TemporaryDirectory() as tmp:
            base=Path(tmp);src=base/'input';script=fixture(src,'s60v3')
            (src/'voice').mkdir();shutil.copy2(src/'bgm/tone.wav',src/'voice/line.wav')
            before=hashes(src)
            report=helper_core.run_job({'action':'convert','source':str(src),'output':str(base/'output'),
                'result':str(base/'job/result.json'),'video_mode':'skip',
                'ffmpeg':os.environ.get('NDS_TEST_FFMPEG') or shutil.which('ffmpeg')})
            self.assertTrue(report['ok'],report)
            self.assertTrue(report['cache_only']);self.assertEqual(report['lossless']['audio_mode'],'voice')
            dst=Path(report['game_directory'])
            self.assertEqual(before,hashes(src));self.assertEqual(script,(dst/'script/START.txt').read_bytes())
            self.assertFalse((dst/'video').exists());self.assertFalse((dst/'bg/scene.png').exists())
            self.assertTrue(list((dst/'nds-cache/voice').rglob('*.nar')))
            self.assertTrue(list((dst/'nds-cache/bgm').rglob('*.pcm')))
            self.assertFalse(list((dst/'nds-cache/bgm').rglob('*.nar')))
            convert.verify(dst)

    @unittest.skipUnless(shutil.which('ffmpeg') or os.environ.get('NDS_TEST_FFMPEG'), 'FFmpeg required')
    def test_unchecked_prune_preserves_output_original_media_even_with_lossless_compression(self):
        with tempfile.TemporaryDirectory() as tmp:
            base=Path(tmp);src=base/'input';fixture(src,'s60v3')
            (src/'voice').mkdir();shutil.copy2(src/'bgm/tone.wav',src/'voice/line.wav')
            before=hashes(src)
            report=helper_core.run_job({
                'action':'convert','source':str(src),'output':str(base/'output'),
                'result':str(base/'job/result.json'),'video_mode':'skip',
                'cache_only':False,'audio_lossless':'voice',
                'ffmpeg':os.environ.get('NDS_TEST_FFMPEG') or shutil.which('ffmpeg')})
            self.assertTrue(report['ok'],report)
            self.assertFalse(report['cache_only'],
                'Unchecking the UI option must not be overridden by cache compression')
            self.assertEqual(report['lossless']['audio_mode'],'voice')
            output=Path(report['game_directory'])
            self.assertTrue((output/'bg/scene.png').is_file())
            self.assertTrue((output/'bgm/tone.wav').is_file())
            self.assertTrue(list((output/'nds-cache/voice').rglob('*.nar')))
            self.assertTrue(list((output/'nds-cache/bgm').rglob('*.pcm')))
            self.assertEqual(before,hashes(src))
            convert.verify(output)

    @unittest.skipUnless(shutil.which('ffmpeg') or os.environ.get('NDS_TEST_FFMPEG'),
                         'FFmpeg required')
    def test_optin_fvds_passthrough_and_default_omission(self):
        from test_fvds_inspect import sample
        ffmpeg=os.environ.get('NDS_TEST_FFMPEG') or shutil.which('ffmpeg')
        with tempfile.TemporaryDirectory() as tmp:
            base=Path(tmp);src=base/'game';original_script=fixture(src,'s60v3')
            media=sample()
            (src/'video/omitted.fv').write_bytes(media)
            before=hashes(src)
            dst=base/'converted'
            opts=argparse.Namespace(source=src,destination=dst,ffmpeg=ffmpeg,font=None,
                 jobs=1,missing_glyph_policy='box',cache_only=True,video_fvds=True)
            convert.convert(opts)
            hashed=convert.key('omitted')
            actual=dst/'nds-cache/video'/hashed[:2]/(hashed+'.fv')
            self.assertEqual(actual.read_bytes(),media)
            self.assertEqual((dst/'script/START.txt').read_bytes(),original_script)
            self.assertEqual(hashes(src),before)
            self.assertFalse((dst/'video').exists())
            manifest=json.loads((dst/'nds-cache/manifest.json').read_text(encoding='utf8'))
            self.assertEqual(manifest['format'],6)
            self.assertEqual(manifest['fvds_videos'][0]['frames'],3)
            self.assertEqual(manifest['fvds_videos'][0]['bytes'],len(media))
            self.assertNotIn('video/omitted.fv',{r['path'] for r in manifest['omitted_files']})
            self.assertIn('video/omitted.mp4',{r['path'] for r in manifest['omitted_files']})
            convert.verify(dst)
            default=base/'default'
            opts.destination=default
            opts.video_fvds=False
            convert.convert(opts)
            self.assertFalse((default/'nds-cache/video').exists())
            self.assertEqual(json.loads((default/'nds-cache/manifest.json').read_text())['fvds_videos'],[])
            self.assertIn('video/omitted.fv',
                          {r['path'] for r in json.loads((default/'nds-cache/manifest.json').read_text())['omitted_files']})
            convert.verify(default)

    @unittest.skipUnless(shutil.which('ffmpeg') or os.environ.get('NDS_TEST_FFMPEG'),
                         'FFmpeg required')
    def test_fvds_reject_invalid_preencoded_input(self):
        from test_fvds_inspect import sample
        ffmpeg=os.environ.get('NDS_TEST_FFMPEG') or shutil.which('ffmpeg')
        with tempfile.TemporaryDirectory() as tmp:
            base=Path(tmp);src=base/'game';fixture(src,'s60v3')
            (src/'video/omitted.fv').write_bytes(sample(tail=True))
            opts=argparse.Namespace(source=src,destination=base/'bad',ffmpeg=ffmpeg,font=None,
                                    jobs=1,missing_glyph_policy='box',video_fvds=True)
            with self.assertRaises(ValueError):
                convert.convert(opts)
            self.assertFalse((base/'bad').exists())

    def test_unsafe_archive_paths_are_rejected(self):
        for name in ['../escape','/absolute','C:/escape','a/../../escape']:
            with self.assertRaises(ValueError):helper_core.checked_name(name)

    @unittest.skipUnless(shutil.which('ffmpeg') or os.environ.get('NDS_TEST_FFMPEG'),'FFmpeg required')
    def test_platforms_video_omission_source_preservation_and_integrity(self):
        ffmpeg=os.environ.get('NDS_TEST_FFMPEG') or shutil.which('ffmpeg')
        with tempfile.TemporaryDirectory() as tmp:
            base=Path(tmp)
            for platform in ['s60v3','s60v5','android']:
                with self.subTest(platform=platform):
                    src=base/platform;script=fixture(src,platform,packed=platform=='s60v3')
                    before=hashes(src);dst=base/(platform+'-out')
                    convert.convert(argparse.Namespace(source=src,destination=dst,ffmpeg=ffmpeg,font=None,jobs=1,missing_glyph_policy='box'))
                    self.assertEqual(before,hashes(src))
                    self.assertEqual(script,(dst/'script/START.txt').read_bytes())
                    self.assertFalse((dst/'video').exists())
                    self.assertEqual(list((dst/'save').iterdir()),[])
                    manifest=json.loads((dst/'nds-cache/manifest.json').read_text(encoding='utf8'))
                    self.assertTrue(manifest['audit']['unsupported_video_commands'])
                    self.assertTrue(manifest['omitted_files'])
                    convert.verify(dst)
                    self.assertEqual(manifest['format'],6)
                    self.assertEqual(manifest['cache_layout']['name'],'sharded')
                    audio=next((dst/'nds-cache/bgm').rglob('*.pcm'))
                    self.assertGreater(audio.stat().st_size,0)
                    with audio.open('ab') as out:out.write(b'corrupt')
                    with self.assertRaises(ValueError):convert.verify(dst)

    def test_nested_platform_bundle_discovery_and_selected_extraction(self):
        with tempfile.TemporaryDirectory() as tmp:
            base=Path(tmp);source=base/'games.zip'
            with zipfile.ZipFile(source,'w') as outer:
                for platform in ['android','s60v5','s60v3']:
                    game=base/platform;fixture(game,platform)
                    nested=io.BytesIO()
                    with zipfile.ZipFile(nested,'w') as inner:
                        for p in game.rglob('*'):
                            if p.is_file():inner.writestr('Game/'+p.relative_to(game).as_posix(),p.read_bytes())
                    outer.writestr(platform+'.zip',nested.getvalue())
            candidates=helper_core.discover(source,base)
            self.assertEqual([c['platform'] for c in candidates],['s60v3','s60v5','android'])
            dest=helper_core.extract_game(candidates[0],base/'selected')
            self.assertEqual(helper_core.configuration((dest/'gameconfig.txt').read_bytes())['platform'],'s60v3')
            self.assertFalse((dest/'save').exists())

    def test_apk_installers_are_not_game_package_inputs(self):
        with tempfile.TemporaryDirectory() as tmp:
            base=Path(tmp)
            apk=base/'android-engine.apk'
            with zipfile.ZipFile(apk,'w') as z:
                z.writestr('Game/gameconfig.txt','gametitle,example\nplatform,android\n')
            with self.assertRaisesRegex(ValueError,'ZIP'):
                helper_core.discover(apk,base)

    @unittest.skipUnless(shutil.which('ffmpeg') or os.environ.get('NDS_TEST_FFMPEG'),'FFmpeg required')
    def test_uppercase_packed_and_loose_symbian_masks(self):
        ffmpeg=os.environ.get('NDS_TEST_FFMPEG') or shutil.which('ffmpeg')
        with tempfile.TemporaryDirectory() as tmp:
            for packed in [True,False]:
                with self.subTest(packed=packed):
                    base=Path(tmp)/str(packed);src=base/'src';fixture(src,'s60v3')
                    (src/'chara').mkdir()
                    pic=io.BytesIO();Image.new('RGB',(10,10),'red').save(pic,format='PNG')
                    mask=Image.new('L',(10,10),255)
                    for y in range(10):
                        for x in range(4):mask.putpixel((x,y),0)
                    buf=io.BytesIO();mask.save(buf,format='PNG')
                    if packed:
                        data=[(b'HERO',pic.getvalue()),(b'HERO_MASK',buf.getvalue())]
                        index=bytearray(struct.pack('<I',2));offset=84
                        for name,payload in data:index+=struct.pack('<32sII',name,offset,len(payload));offset+=len(payload)
                        (src/'chara/chara.pak').write_bytes(index+b''.join(p for _,p in data))
                    else:
                        (src/'chara/HERO.png').write_bytes(pic.getvalue());(src/'chara/HERO_MASK.png').write_bytes(buf.getvalue())
                    dst=base/'out'
                    convert.convert(argparse.Namespace(source=src,destination=dst,ffmpeg=ffmpeg,font=None,jobs=1,missing_glyph_policy='box'))
                    hashed=convert.key('HERO')
                    raw=(dst/'nds-cache/chara'/hashed[:2]/(hashed+'.npi')).read_bytes()
                    magic,lw,lh,w,h,alpha=struct.unpack('<4sHHHHI',raw[:16])
                    self.assertEqual((magic,lw,lh,w,h,alpha),(b'NPI1',10,10,8,8,1))
                    opacity=raw[16+w*h*2:]
                    self.assertEqual(opacity[0],0);self.assertEqual(opacity[-1],255)
                    self.assertFalse(any((dst/'nds-cache/chara').rglob(convert.key('HERO_MASK')+'.npi')))
                    convert.verify(dst)

if __name__=='__main__':unittest.main()
