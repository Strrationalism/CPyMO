"""Create a tiny independently convertible NVA1 companion from the original stress source.

The main ~39MiB game remains purely NVL1, so this tests BOTH published
video modes using the ordinary, unmodified public converter. All assets
are originals from make_release_stress_game.py; no second 30MiB duplicate.
"""
import argparse
import shutil
from pathlib import Path


def create(big_source,side_source):
    source=Path(big_source).resolve()
    target=Path(side_source).resolve()
    if target.exists():
        raise ValueError('Sidecar destination must be new, never overwrite games')
    for name in ('bg','bgm','script','video'):
        (target/name).mkdir(parents=True)
    cfg=(source/'gameconfig.txt').read_text(encoding='utf-8')
    (target/'gameconfig.txt').write_text(cfg.replace(
        '双卡回廊：终章验收','双卡回廊：NVA1音轨验收'),encoding='utf-8')
    shutil.copy2(source/'bg/SCENE01.png',target/'bg/SCENE01.png')
    shutil.copy2(source/'bgm/BGM01.wav',target/'bgm/BGM01.wav')
    for name in ('OPENING','SILENT'):
        shutil.copy2(source/'video'/(name+'.mp4'),target/'video'/(name+'.mp4'))
    script=[
        '#label LOOP','#bg SCENE01,BG_NOFADE,0','#bgm BGM01,1',
        '#say 系统,这是独立的 NVA1 压缩音轨快速验证。',
        '#say 系统,确认背景音乐正在播放，再观看有声短片。',
        '#movie OPENING',
        '#say 系统,NVA1 有声短片结束，继续对白并确认 BGM 恢复。',
        '#say 系统,下一段原片无声音，NVA1 应正常生成静音音轨。',
        '#movie SILENT',
        '#say 系统,NVA1 静音短片也已经结束。',
        '#say 系统,如果两段都成功，可结束本附加测试。',
        '#goto LOOP',
    ]
    (target/'script/START.txt').write_text('\n'.join(script)+'\n',encoding='utf-8')
    return {'videos':['OPENING','SILENT'],'script_lines':len(script)}


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source',type=Path)
    parser.add_argument('destination',type=Path)
    opt=parser.parse_args()
    print(create(opt.source,opt.destination))
