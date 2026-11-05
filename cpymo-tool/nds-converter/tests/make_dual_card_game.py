"""Produce an original, playable ~30 MB PyMO fixture for CPyMO NDS 1.1.
All images and sounds generated here; no commercial game assets or dummy padding.
The official NDS converter then produces the real on-device nds-cache tree.
"""
from pathlib import Path
from PIL import Image, ImageDraw
import math, wave, struct, random, sys

ROOT = Path(sys.argv[1]).resolve()
ROOT.mkdir(parents=True, exist_ok=True)
for name in ('bg', 'chara', 'bgm', 'voice', 'se', 'script', 'save'):
    (ROOT/name).mkdir(exist_ok=True)

(ROOT/'gameconfig.txt').write_text(
    'gametitle,双卡回廊：星夜讯号\n'
    'platform,s60v3\n'
    'imagesize,320,240\n'
    'scripttype,pymo\n'
    'startscript,START\n'
    'engineversion,0.5\n'
    'bgformat,.png\n'
    'charaformat,.png\n'
    'charamaskformat,.png\n'
    'bgmformat,.wav\n'
    'voiceformat,.wav\n'
    'seformat,.wav\n'
    'bgmvolume,5\n'
    'vovolume,5\n'
    'sevolume,5\n'
    'fontsize,16\n'
    'textcolor,#ffffff\n'
    'playvideo,0\n', encoding='utf-8')

# Hand-drawn procedural 320x240 panoramas; converter downsamples to 256x192
# RGB555 NPI1 to match original NDS cache constraints.
for scene in range(12):
    rng=random.Random(4101 + scene * 137)
    W,H=320,240
    im=Image.new('RGB',(W,H)); pixels=im.load()
    palettes=[((11,21,46),(56,88,137)),((22,31,51),(113,76,112)),
              ((17,47,59),(80,132,119)),((34,27,53),(100,79,142))]
    top, bottom=palettes[scene%len(palettes)]
    for y in range(H):
        t=y/(H-1)
        for x in range(W):
            horizon=(1-t)
            bloom=int(10 * math.sin((x*0.034)+(scene*0.73))*math.sin(t*3))
            noise=rng.randrange(-3,4)
            pixels[x,y]=tuple(max(0,min(255,round(top[k]*horizon+bottom[k]*t+bloom+noise))) for k in range(3))
    d=ImageDraw.Draw(im)
    d.ellipse((230-(scene%3)*16,22,257-(scene%3)*16,49),fill=(237,218,169))
    for star in range(55):
        x=rng.randrange(0,320);y=rng.randrange(4,116)
        b=rng.randrange(125,241)
        d.point((x,y),fill=(b,b,min(255,b+12)))
    skyline=142+scene%4*5
    d.polygon([(0,178),(0,160),(60,146),(123,170),(192,144),(260,165),(320,149),(320,240),(0,240)],fill=(12+scene%3*5,31,45))
    for k in range(15):
        x=(k*31+scene*17)%340-10
        h=rng.randrange(23,65)
        d.rectangle((x,skyline-h,x+20,skyline+42), fill=(15+scene%3*5,28,47+scene%4*4))
        for j in range(4):
            for z in range(2):
                if rng.random()>.42:
                    d.rectangle((x+4+j*4,skyline-h+8+z*12,x+6+j*4,skyline-h+12+z*12),fill=(186,166,89))
    d.rectangle((0,186,319,239),fill=(15,43,66))
    for k in range(26):
        y=192+k*2
        x=(k*13+scene*9)%160
        d.line((x,y,x+28,y),fill=(58+scene%4*8,70+scene%5*8,95+scene%2*12),width=1)
    d.line((0,186,319,186),fill=(91,125,137),width=2)
    im.save(ROOT/'bg'/f'SCENE{scene+1:02}.png')

# Four original character silhouettes with true alpha masks, dimensions safe
# for the 256x192 display and for the NDS image cache.
for k in range(4):
    w,h=170,205
    chara=Image.new('RGB',(w,h),(42+k*15,71+k*8,99+k*7))
    mask=Image.new('L',(w,h),0)
    d=ImageDraw.Draw(chara);m=ImageDraw.Draw(mask)
    hair=[(33,45,72),(67,55,71),(71,38,57),(44,64,55)][k]
    clothes=[(46,71,107),(88,71,94),(53,102,83),(72,77,107)][k]
    m.ellipse((46,23,125,105),fill=255)
    m.polygon(((31,205),(35,137),(53,104),(122,104),(149,137),(160,205)),fill=255)
    d.ellipse((48,26,123,104),fill=(220,184,157))
    d.pieslice((40,15,131,105),180,348,fill=hair)
    d.polygon(((31,205),(35,137),(53,104),(122,104),(149,137),(160,205)),fill=clothes)
    d.polygon(((72,109),(87,143),(103,109)),fill=(204,217,219))
    d.arc((57,67,81,76),185,355,fill=(34,34,47),width=2)
    d.arc((93,67,117,76),185,355,fill=(34,34,47),width=2)
    d.arc((75,79,100,94),10,155,fill=(149,83,75),width=2)
    chara.save(ROOT/'chara'/f'PLAYER{k+1}.png')
    mask.save(ROOT/'chara'/f'PLAYER{k+1}_MASK.png')

# All eight BGM tracks are playable PCM16 audio, not padded noise.
# A 6.4-second multivoice original sequence is looped to 83 seconds per track.
# 8*83*22050*2 = 29,282,400 audio bytes before additional scene assets.
RATE=22050
SAMPLES=RATE*8
notes=[220,247,262,294,330,349,392,440,494,523,587]
for k in range(8):
    frame=bytearray(SAMPLES*2)
    notes_for_track=[notes[(k*2 + i*3 + ((i//2)%3))%len(notes)] for i in range(16)]
    for n in range(SAMPLES):
        note=notes_for_track[(n*16)//SAMPLES]
        t=n/RATE
        beat=(n%(RATE//2))/(RATE//2)
        env=0.38+0.62*(1.0-beat)**2
        val=(math.sin(2*math.pi*note*t)*0.58 +
             math.sin(2*math.pi*(note/2)*t)*0.23+
             math.sin(2*math.pi*(note*1.5)*t)*0.12)
        v=max(-32767,min(32767,round(11000*env*val)))
        struct.pack_into('<h',frame,n*2,v)
    name=ROOT/'bgm'/f'BGM{k+1:02}.wav'
    with wave.open(str(name),'wb') as w:
        w.setnchannels(1);w.setsampwidth(2);w.setframerate(RATE)
        for j in range(10):
            w.writeframesraw(frame)
        # 80-second track; length varied by additional 3 sec outro
        w.writeframesraw(frame[:RATE*3*2])
        w.writeframes(b'')
for k in range(3):
    duration=0.38 + k*.19
    buf=bytearray()
    for n in range(int(duration*RATE)):
        t=n/RATE
        env=(1-t/duration)**2
        freq=(520+130*k)*(1-.35*t/duration)
        value=round(8500*env*math.sin(2*math.pi*freq*t))
        buf+=struct.pack('<h',value)
    with wave.open(str(ROOT/'se'/f'TONE{k+1}.wav'),'wb') as w:
        w.setnchannels(1);w.setsampwidth(2);w.setframerate(RATE);w.writeframes(buf)
# A short voice-channel test tone: intentionally no impersonated narration.
with wave.open(str(ROOT/'voice'/'SIGNAL.wav'),'wb') as w:
    w.setnchannels(1);w.setsampwidth(2);w.setframerate(RATE)
    w.writeframes(b''.join(struct.pack('<h',int(3800*math.sin(2*math.pi*660*n/RATE)*math.sin(math.pi*n/(RATE*.6))))
                           for n in range(round(RATE*.6))))

# Official interpreter grammar:
# bg(name, transition, milliseconds), chara(id,name,xPercent,layer,ms),
# say(name,text), bgm(name,loop), se(name,loop), vo(name).
script=['#label LOOP',
        '#bg SCENE01,BG_NOFADE,0',
        '#bgm BGM01,1',
        '#say 系统,欢迎来到双卡回廊。',
        '#say 系统,这是一段专为 NDS 制作的原创测试剧情。',
        '#say 系统,这次我们从机身 SD 卡读取游戏。']
scene_titles=['星夜启程','另一张卡','街灯之下','微光抵达','信号交汇','水面的风',
              '无声街道','新的约定','夜色回声','跨越边界','回到原点','测试完成']
texts=['第一处场景已经载入。','画面和音乐都来自完整游戏数据。',
       '如果能看到这里，剧情脚本读取正常。','稍后可以尝试打开菜单保存进度。',
       '这张卡虽小，也能容纳一段旅程。','听听背景音乐，确认连续播放没有中断。',
       '人物立绘正在按原生格式切换。','所有背景都按 NDS 尺寸生成缓存。',
       '按 START 可以把进度保存到存档一。','按 SELECT 可以重新读取存档一。',
       '我们没有修改 CPyMO 原本的规则。','双卡测试结束，还能再次开始。']
for i in range(12):
    n=i+1
    if i:
        script.append(f'#bg SCENE{n:02},BG_ALPHA,300')
        if i<8:script.append(f'#bgm BGM{n:02},1')
    if i%3==0:
        script.append('#chara_cls a,0')
        script.append(f'#chara 1,PLAYER{i//3+1},50,1,180')
    if i%4==0:
        script.append(f'#se TONE{(i//4)%3+1},0')
    if i==5:script.append('#vo SIGNAL')
    script.append(f'#say 系统,第{n:02}幕：{scene_titles[i]}')
    script.append(f'#say 千岚之夏,{texts[i]}')
    if i in (2,6,10):
        script.append('#say 系统,点击继续，也可以查看历史记录。')
script += ['#chara_cls a,0','#bgm_stop',
           '#say 系统,本轮测试完毕。可按 START 存档。',
           '#say 系统,继续操作将返回开头，以便重复检查。',
           '#goto LOOP']
(ROOT/'script'/'START.txt').write_text('\n'.join(script)+'\n', encoding='utf-8')
(ROOT/'README_ORIGINAL.txt').write_text(
    '《双卡回廊：星夜讯号》是为 CPyMO for NDS 双卡实验制作的原创合成短篇。\n'
    '图形、音频、剧本均由本生成器创建；没有商业游戏资源。\n'
    '所有转场、立绘和声音均通过 CPyMO 官方 NDS 转换器生成缓存。\n'
    '建议将转换后的完整游戏目录放在 NDSi 机身 SD 卡的 pymo/games/。\n'
    '剧情内按 START 保存至本游戏 save 目录（不要与原有游戏混淆）。\n',encoding='utf-8')
print('PyMO source generated at:',ROOT)
