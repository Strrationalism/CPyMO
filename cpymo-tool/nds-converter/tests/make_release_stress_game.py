"""Original CPyMO 1.2 final DSpico(TWL)/R4(NTR) large playable acceptance.

Builds ON TOP OF the already hardware-approved ~30 MiB dual-card fixture:
12 actual generated backgrounds, four masked sprites, eight genuine 83s
PCM16 music tracks, voice & SE. Adds real original MP4 (including silent
film), startup title choice, post-story choices, and branch/checkpoints.
No commercial media, empty padding or random filler. The official
converter processes all assets, including the source videos.

Usage: python make_release_stress_game.py GAME_SOURCE_DIR --ffmpeg ffmpeg
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import runpy
import subprocess
import sys

HERE=Path(__file__).resolve().parent
CLIPS=(
    ('OPENING',2.0,True),    # normal EOF
    ('SKIPTEST',9.0,True),   # B to skip while running
    ('SILENT',3.0,False),    # audio-absent input -> deterministic silence
    ('LONGRUN',24.0,True),  # long decode + sustained BGM/return stress
)


def write_game(target,ffmpeg):
    target=Path(target).resolve()
    if target.exists():
        raise ValueError('Use a new source directory; never overwrite a saved game')
    # Execute the old approved original generator as is, without a duplicate
    # corpus of bundled artwork or copyrighted/third-party datasets.
    old_argv=sys.argv[:]
    try:
        sys.argv=[str(HERE/'make_dual_card_game.py'),str(target)]
        runpy.run_path(str(HERE/'make_dual_card_game.py'),run_name='__main__')
    finally:
        sys.argv=old_argv
    config=target/'gameconfig.txt'
    original=config.read_text(encoding='utf-8')
    assert 'playvideo,0' in original
    config.write_text(original.replace('playvideo,0','playvideo,1').replace(
        '双卡回廊：星夜讯号','双卡回廊：终章验收'),encoding='utf-8')
    video=target/'video'
    video.mkdir(exist_ok=True)
    checks=[]
    for index,(name,duration,with_audio) in enumerate(CLIPS):
        output=video/(name+'.mp4')
        # Real 320x240, 15fps source frames with motion; the converter must
        # scale to 256x192, choose 12fps, RGB555 and encode frame deltas.
        # Use video testsrc2 for dynamic patterns, not frozen/static padding.
        args=[str(ffmpeg),'-nostdin','-hide_banner','-loglevel','error',
            '-f','lavfi','-i',
            f'testsrc2=size=320x240:rate=15:duration={duration:.1f}']
        if with_audio:
            args+=['-f','lavfi','-i',
                    f'sine=frequency={330+index*90}:sample_rate=44100:duration={duration:.1f}',
                    '-c:a','aac','-b:a','64k']
        args+=['-c:v','mpeg4','-q:v','6','-pix_fmt','yuv420p',
               '-movflags','+faststart','-y',str(output)]
        subprocess.run(args,check=True,timeout=150,capture_output=True)
        checks.append({'name':name,'source_seconds':duration,
                       'has_audio':with_audio,'source_bytes':output.stat().st_size,
                       'sha256':hashlib.sha256(output.read_bytes()).hexdigest()})

    # Official PyMO parser: '#sel <N>' consumes N following raw text lines;
    # the chosen index becomes FSEL. '#if FSEL=0,goto CHAPTER' uses
    # conditional interpreter dispatch. Never invent private engine syntax.
    story=[
        '#bg SCENE01,BG_NOFADE,0',
        '#bgm BGM01,1',
        '#label TITLE',
        '#sel 2',
        '开始综合测试',
        '直接测试视频',
        '#if FSEL=1,goto VIDEO',
        '#label CHAPTER',
        '#say 系统,双卡回廊：终章验收。',
        '#say 系统,请确认开场标题与选项完全位于上屏。',
        '#say 系统,进入剧情后请按 L 轮换三种显示模式。',
        '#say 系统,按 START 快速保存，SELECT 快速读取。',
        '#say 系统,请通过 X 打开原有菜单并查看时钟。',
        '#say 系统,按 UP 打开历史对话，再用 B 返回。',
    ]
    chapters=[
        '星夜启程','安静街灯','信号交汇','雨后街巷',
        '远方归途','海面回声','另一张卡','梦的轮廓',
        '夜色余温','最后的窗口','切换场景','星夜再会',
    ]
    for i,title in enumerate(chapters):
        num=i+1
        story.extend([
            f'#bg SCENE{num:02},BG_ALPHA,250',
            f'#bgm BGM{(i%8)+1:02},1',
        ])
        if i%3==0:
            story.extend(['#chara_cls a,0',f'#chara 1,PLAYER{(i//3)+1},50,1,180'])
        if i%4==0:story.append(f'#se TONE{(i//4)%3+1},0')
        if i==5:story.append('#vo SIGNAL')
        story.extend([
            f'#say 系统,第{num:02}幕：{title}。',
            '#say 千岚之夏,请观察图片切换、音频持续播放以及触控响应。'
        ])
        if i in (3,8):
            story.extend([
                '#sel 2',
                '继续下一个场景',
                '重听这一幕的音乐',
                '#if FSEL=1,bgm BGM01,1',
                '#say 系统,剧情选项已返回；请确认操作发生在下屏。'
            ])
    story.extend([
        '#chara_cls a,0',
        '#label VIDEO',
        '#bg SCENE08,BG_ALPHA,200',
        '#bgm BGM02,1',
        '#say 系统,视频一：等影片自然结束，再继续对白。',
        '#movie OPENING',
        '#say 系统,正常播放返回成功；检查背景音乐是否恢复。',
        '#say 系统,视频二：请在视频播放中按 B 提前跳过。',
        '#movie SKIPTEST',
        '#say 系统,B 跳过视频返回成功，不能卡住或漏播下一句。',
        '#say 系统,视频三：原视频没有声音，转换后应为静音。',
        '#movie SILENT',
        '#say 系统,静音视频返回成功；音轨处理不应报错。',
        '#say 系统,视频四：较长连续播放，可观察帧率和音画同步。',
        '#movie LONGRUN',
        '#say 系统,长视频结束。请确认没有明显拖慢、声音错位或崩溃。',
        '#bg SCENE12,BG_ALPHA,250',
        '#bgm BGM08,1',
        '#say 系统,综合验收完成。现在保存，再读取存档，确认恢复位置。',
        '#say 系统,返回标题菜单可重复体验；建议两张卡分别保存测试记录。',
        '#goto TITLE',
    ])
    (target/'script'/'START.txt').write_text('\n'.join(story)+'\n',encoding='utf-8')
    (target/'README_ORIGINAL.txt').write_text(
        '《双卡回廊：终章验收》是 CPyMO 1.2 用原创素材生成的压力测试。\n'
        '基于已验收的 30MiB 场景/语音/BGM 测试，增加独立生成的 MP4、标题、剧情选择。\n'
        '不是商业游戏；无需付费素材，也不附任何外部游戏 ROM 或版权视频。\n'
        '正常流程：启动选择「开始综合测试」→ 12 幕剧情 → 四段视频 → 存读档。\n'
        '请在 SKIPTEST 段按 B，其他视频优先正常播放。\n'
        '同一游戏目录可用于 DS/NTR 与 DSi/TWL，两端各自使用正确的 ROM。\n',
        encoding='utf-8')
    (target/'SOURCE_INVENTORY.json').write_text(
        json.dumps({'project':'CPyMO 1.2 original final stress game',
          'script_lines':len(story),'scenes':12,'bgm_tracks':8,
          'sprites':4,'clip_sources':checks},ensure_ascii=False,indent=2)+'\n',
        encoding='utf-8')
    return {'script_lines':len(story),'clips':checks}


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('destination',type=Path)
    parser.add_argument('--ffmpeg',default='ffmpeg')
    args=parser.parse_args()
    print(json.dumps(write_game(args.destination,args.ffmpeg),
                     ensure_ascii=False,indent=2))
