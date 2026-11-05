"""Smoke a real frozen Windows Converter.exe worker, with source MP4 and NVL1.

Run on Windows Actions after build_portable.py. Uses original synthetic media;
never embeds private or commercial video. This verifies _MEIPASS lookups,
FFmpeg *video* decoding, generated ROM, game cache and source immutability.
Also constructs the frozen GUI and records its actual version footer.
This does not certify real NDS frame rate or all Windows display settings.
"""
from pathlib import Path
import argparse
import hashlib
import json
import shutil
import subprocess
import sys
import tempfile

HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE))
sys.path.insert(0,str(HERE.parent))
from test_converter import fixture,hashes
from convert import verify
from release_info import VERSION


def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--portable',type=Path,required=True)
    ap.add_argument('--ffmpeg',type=Path,required=True)
    opt=ap.parse_args()
    root=opt.portable.resolve()
    exe=root/'app/Converter/Converter.exe'
    internal=root/'app/Converter/_internal'
    rom=root/'cpymo-nds.nds'
    assert exe.is_file() and (internal/'ffmpeg.exe').is_file(), 'Portable app or FFmpeg missing'
    assert rom.is_file() and (internal/'cpymo-nds.nds').is_file()
    assert json.loads((root/'VERSION.json').read_text())['version']==VERSION
    digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    assert digest(rom)==digest(internal/'cpymo-nds.nds'),'Paired ROM mismatch'
    with tempfile.TemporaryDirectory(prefix='cpymo-portable-smoke-') as temp:
        work=Path(temp)
        src=work/'game'
        original_script=fixture(src,'s60v3')
        # Replace test fixture's placeholder "omitted.mp4" with real audiovisual
        # video using a full, video-capable external FFmpeg on the Windows host.
        cmd=[str(opt.ffmpeg),'-nostdin','-hide_banner','-loglevel','error',
             '-f','lavfi','-i','testsrc2=duration=1.2:size=320x240:rate=15',
             '-f','lavfi','-i','sine=frequency=440:duration=1.2:sample_rate=44100',
             '-c:v','mpeg4','-q:v','7','-c:a','aac','-y',str(src/'video/omitted.mp4')]
        subprocess.run(cmd,capture_output=True,check=True,timeout=90)
        before=hashes(src)
        result=work/'job/result.json'
        job={'action':'convert','source':str(src),'output':str(work/'converted'),
             'candidate':0,'video_mode':'nvl1','audio_lossless':'voice',
             'cache_only':True,'missing_policy':'box',
             'result':str(result),'log':str(work/'job/log.txt')}
        result.parent.mkdir(parents=True)
        job_path=work/'job/task.json'
        job_path.write_text(json.dumps(job,ensure_ascii=False),encoding='utf-8')
        completed=subprocess.run([str(exe),'--job',str(job_path)],
            capture_output=True,timeout=180)
        assert result.is_file(),(
            f'Frozen executable failed before producing result; code={completed.returncode}; '
            f'output={completed.stderr[-500:]!r}')
        report=json.loads(result.read_text(encoding='utf-8'))
        assert completed.returncode==0,report
        assert report['ok'],report
        assert report['release_version']==VERSION,report
        assert len(report['videos'])==1,report
        assert report['videos'][0]['format']=='NVL1',report
        game=Path(report['game_directory'])
        verify(game)
        assert hashes(src)==before,'Original game changed'
        assert (game/'script/START.txt').read_bytes()==original_script
        assert (Path(report['output'])/'cpymo-nds.nds').is_file(),'ROM not bundled with converted output'
        assert digest(Path(report['output'])/'cpymo-nds.nds')==digest(rom),'ROM mismatch in conversion'
        proof=root/'native-validation';proof.mkdir()
        gui_result=proof/'result.json'
        gui_cfg={'source':str(src),'output_base':str(work/'gui-output'),
                 'result':str(gui_result),'screenshot':str(proof/'window.png')}
        (work/'gui-output').mkdir()
        gui_path=work/'gui.json';gui_path.write_text(json.dumps(gui_cfg),encoding='utf8')
        gui=subprocess.run([str(exe),'--state-root',str(proof/'jobs'),'--self-test',str(gui_path)],
                           capture_output=True,timeout=180)
        assert gui.returncode==0 and gui_result.is_file(),gui.stderr[-500:]
        gui_report=json.loads(gui_result.read_text(encoding='utf8'))
        assert gui_report['ok'] and gui_report['release_version']==VERSION,gui_report
        assert VERSION in gui_report['ui_version_footer'],gui_report
        assert gui_report['ui_title'],gui_report
        verify(Path(gui_report['game_directory']))
        assert hashes(src)==before,'GUI modified original source'
        assert digest(Path(gui_report['output'])/'cpymo-nds.nds')==digest(rom)
        print(json.dumps({'frozen_worker':'PASS','ffmpeg_video':'PASS','format':'NVL1',
            'frames':report['videos'][0]['frames'],'rom_pair':'MATCH',
            'source_preservation':'PASS','release_version':VERSION,
            'frozen_gui':'PASS','actual_ui_version_footer':gui_report['ui_version_footer'],
            'ui_screenshot':gui_report.get('ui_screenshot')},ensure_ascii=True))


if __name__=='__main__':
    main()
