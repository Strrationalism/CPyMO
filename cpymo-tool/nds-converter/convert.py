"""CPyMO NDS cache converter, MIT. Python 3.10+, Pillow, external FFmpeg.

Scripts/config and retained resources remain byte-identical in a new directory.
Normal CLI conversion defaults to playable NVL1 movie caches; users can opt out.
Original source video bytes stay untouched and hashes remain in the manifest.
No game names, chapters, translated text, or machine-specific paths are assumed.
"""
from __future__ import annotations
import argparse, concurrent.futures, hashlib, io, json, os, shutil, struct
import subprocess, sys, uuid, re
from dataclasses import dataclass
from pathlib import Path
from PIL import Image, ImageFont, ImageDraw, __version__ as pillow_version
from pixel_font import pixel_font_cache
import lossless
import video_nvl
from fvds_inspect import inspect_fvds

from release_info import VERSION
CORE_COMMANDS = frozenset(['add', 'album', 'anime_off', 'anime_on', 'bg', 'bgm', 'bgm_stop', 'call', 'change', 'chara', 'chara_anime', 'chara_cls', 'chara_down', 'chara_pos', 'chara_quake', 'chara_scroll', 'chara_up', 'chara_y', 'config', 'date', 'fade_in', 'fade_out', 'flash', 'goto', 'if', 'label', 'load', 'movie', 'music', 'quake', 'rand', 'ret', 'say', 'scroll', 'se', 'se_stop', 'sel', 'select_img', 'select_imgs', 'select_text', 'select_var', 'set', 'sub', 'text', 'text_off', 'textbox', 'title', 'title_dsp', 'vo', 'wait', 'wait_se', 'waitkey'])
CACHE = 'nds-cache'
# Unicode repertoire for upstream core UI, not game dialogue.
# Stable superset of upstream/NDS UI glyphs, including printable ASCII.
# Game dialogue/config characters are collected separately during conversion.
UI_CODEPOINTS = (
    32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,
    50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,
    68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,
    86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,
    104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,
    122,123,124,125,126,183,12289,12290,19969,19978,19979,19981,19982,20004,20006,20013,20026,20043,
    20048,20063,20110,20132,20165,20197,20214,20250,20294,20301,20303,20307,20316,20320,20351,20379,20381,20445,
    20462,20677,20808,20837,20840,20851,20855,20877,20986,21010,21015,21040,21047,21069,21095,21152,21160,21205,
    21253,21270,21315,21333,21345,21382,21407,21462,21482,21487,21488,21490,21491,21495,21508,21517,21518,21527,
    21547,21551,21644,21843,21855,21966,22238,22312,22519,22797,22799,22823,22833,22841,22846,23383,23384,23433,
    23436,23450,23545,23558,23559,23565,23567,23631,23706,24037,24038,24050,24120,24149,24179,24182,24211,24213,
    24230,24335,24341,24403,24452,24456,24465,24555,24773,24930,25103,25110,25138,25165,25191,25214,25321,25345,
    25353,25442,25552,25773,25806,25903,25918,25928,25943,25968,25972,25976,25991,26009,26032,26080,26085,26223,
    26371,26376,26377,26410,26412,26448,26694,26696,26723,26893,27138,27169,27284,27425,27491,27492,27511,27721,
    27809,27839,28040,28216,28304,28857,28858,29256,29486,29992,30005,30041,30053,30070,30340,30636,30721,30830,
    30906,31163,31227,31435,31471,32032,32147,32218,32447,32463,32472,32487,32493,32531,32622,32626,32773,32972,
    33041,33050,33258,33521,33756,34255,34892,34917,34920,35013,35036,35037,35201,35270,35302,35373,35377,35441,
    35442,35486,35712,35768,35774,35805,35813,35821,35831,35835,36039,36129,36133,36164,36182,36317,36335,36617,
    36716,36731,36733,36807,36816,36820,36827,36864,36873,36895,36914,36938,36984,37096,37197,37325,37327,38291,
    38388,38543,38544,38577,38656,38899,38917,39033,39057,65292,65306,65311,
)
# All three mode names are engine UI, not game script dialogue. Always embed
# them in newly converted games even when their novels never use these glyphs.
# The ROM fallback font independently covers already-converted game caches.
UI_CODEPOINTS = tuple(sorted(set(UI_CODEPOINTS) | set(map(ord, '立绘特效(全局)增强双屏增強雙強化画面操控モむ退出到游戏库确定未保存的进度将丢失。遊戲庫確定未儲存的進度將遺失。ゲーム一覧へ終了戻りますか進行は失われます'))))

def sha(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as f:
        for block in iter(lambda: f.read(1024*1024), b''): h.update(block)
    return h.hexdigest()

def key(name):
    value = 14695981039346656037
    for b in name.translate(str.maketrans('ABCDEFGHIJKLMNOPQRSTUVWXYZ','abcdefghijklmnopqrstuvwxyz')).encode('utf-8'): value = ((value ^ b)*1099511628211) & ((1<<64)-1)
    return f'{value:016x}'

def safe_name(name):
    if not name or name in ('.','..') or any(c in name for c in '/\\\0:'):
        raise ValueError(f'Unsafe asset name: {name!r}')
    return name

@dataclass(frozen=True)
class Asset:
    path: Path
    offset: int = 0
    length: int | None = None
    def read(self):
        with self.path.open('rb') as f:
            f.seek(self.offset)
            data = f.read() if self.length is None else f.read(self.length)
        if self.length is not None and len(data) != self.length: raise ValueError('Truncated package')
        return data

def assets(folder, extension=None):
    package = folder/(folder.name+'.pak')
    if package.is_file():
        size = package.stat().st_size
        result = {}
        with package.open('rb') as f:
            header = f.read(4)
            if len(header) != 4: raise ValueError(f'Invalid package {package}')
            count, = struct.unpack('<I', header)
            if count > 50000 or 4+count*40 > size: raise ValueError('Invalid package index')
            for _ in range(count):
                raw, offset, length = struct.unpack('<32sII', f.read(40))
                name = safe_name(raw.split(b'\0')[0].decode('utf-8'))
                if name in result or offset < 4+count*40 or offset+length > size:
                    raise ValueError(f'Overlapping index/name or out-of-bounds asset {name}')
                result[name] = Asset(package, offset, length)
        return result
    result = {}
    if folder.is_dir():
        for f in sorted(folder.iterdir()):
            if f.is_file() and (extension is None or f.suffix.lower() == '.'+extension.lower().lstrip('.')):
                if f.suffix.lower() not in ('.jpg','.jpeg','.png','.bmp','.mp3','.ogg','.wav','.pcm'): continue
                name = safe_name(f.stem)
                if name in result: raise ValueError(f'Duplicate loose asset {name}')
                result[name] = Asset(f)
    return result

def image_cache(asset, target, scale, mask=None, rgb=False):
    data = asset.read()
    image = Image.open(io.BytesIO(data)).convert('RGB' if rgb else 'RGBA')
    lw, lh = image.size
    if mask is not None:
        image.putalpha(Image.open(io.BytesIO(mask.read())).convert('L').resize(image.size, Image.Resampling.BILINEAR))
    pw, ph = max(1,round(lw*scale)), max(1,round(lh*scale))
    if max(lw,lh)>65535 or max(pw,ph)>2048 or pw*ph>262144: raise ValueError('Image exceeds cache/DS allocation limit')
    image = image.resize((pw,ph), Image.Resampling.LANCZOS)
    colors = bytearray(); alpha = bytearray()
    for p in (image.get_flattened_data() if hasattr(image,"get_flattened_data") else image.getdata()):
        colors += struct.pack('<H', 0x8000 | (p[0]>>3) | ((p[1]>>3)<<5) | ((p[2]>>3)<<10))
        if not rgb: alpha.append(p[3])
    if alpha and all(a==255 for a in alpha): alpha.clear()
    target.parent.mkdir(parents=True,exist_ok=True)
    target.write_bytes(struct.pack('<4sHHHHI', b'NPI1', lw, lh, pw, ph, bool(alpha))+colors+alpha)
    return {'source_sha256':hashlib.sha256(data).hexdigest(),'logical_size':[lw,lh],'physical_size':[pw,ph],'alpha':bool(alpha)}

def font_cache(text, font_path, target):
    codes = sorted(set(map(ord,text)) | set(range(32,127)) | set(UI_CODEPOINTS))
    codes = [cp for cp in codes if cp>=32 and cp not in (0xfeff,0xfffe,0xffff)]
    if len(codes)>20000: raise ValueError('More than 20000 glyphs; split game or use a future streamed-font backend')
    font = ImageFont.truetype(str(font_path),13)
    output = bytearray(struct.pack('<4sI',b'NF13',len(codes)))
    for cp in codes:
        image=Image.new('L',(13,14));ImageDraw.Draw(image).text((0,11),chr(cp),font=font,fill=255,anchor='ls')
        rows=[sum(1<<x for x in range(13) if image.getpixel((x,y))>=100) for y in range(14)]
        output += struct.pack('<IB3x14H',cp,min(32,round(font.getlength(chr(cp)))),*rows)
    target.write_bytes(output)
    return len(codes)

def mask_cache(asset, target, width, height):
    image=Image.open(io.BytesIO(asset.read())).convert('L').resize((width,height),Image.Resampling.BILINEAR)
    target.parent.mkdir(parents=True,exist_ok=True)
    target.write_bytes(struct.pack('<4sHH',b'NMT1',width,height)+image.tobytes())
    return {'source_sha256':hashlib.sha256(asset.read()).hexdigest(),'physical_size':[width,height]}

def audit(scripts, collections, playable_videos=()):
    missing=[]; unsupported=[]; dynamic=[]; unknown=[]; runtime=[]
    mapping={'bg':'bg','bgm':'bgm','se':'se','vo':'voice'}
    for path in scripts:
        for line_number,line in enumerate(path.read_text(encoding='utf-8-sig').splitlines(),1):
            line=line.lstrip()
            if not line.startswith('#'): continue
            parts=line[1:].split(None,1);command=parts[0] if parts else '';args=parts[1] if len(parts)>1 else ''
            if command and command not in CORE_COMMANDS: unknown.append({'script':path.name,'line':line_number,'command':command,'behavior':'upstream skips unknown directive; review before distribution'})
            if command in ('album','music','chara_anime','anime_on','scroll'): runtime.append({'script':path.name,'line':line_number,'command':command})
            if command in ('movie','video') and args.split(',')[0].strip().casefold() not in playable_videos:
                unsupported.append({'script':path.name,'line':line_number,'command':command,'name':args.split(',')[0].strip()})
            folder=mapping.get(command)
            if folder is None: continue
            name=args.split(',')[0].strip()
            if command=='vo' and name and (name[0].isdigit() or name.startswith('-')): name=args.split(',')[-1].strip()
            if not name or name in ('NULL','none','-1'): continue
            if any(c in name for c in '$%{}'): dynamic.append({'script':path.name,'line':line_number,'name':name});continue
            if name.lower() not in {n.lower() for n in collections.get(folder,{})}: missing.append({'script':path.name,'line':line_number,'folder':folder,'name':name})
    return {'unknown_commands':unknown,'commands_requiring_game_runtime_check':runtime,'missing_resource_references':missing,'unsupported_video_commands':unsupported,'dynamic_references_not_resolved':dynamic,'limitations':['video safely skipped; masks supported with fade fallback for invalid masks','other games require runtime verification','font coverage depends on the supplied font; characters absent from it may render as tofu','memory use depends on concurrent images, not only individual file limits']}

def verify(destination):
    manifest=json.loads((destination/CACHE/'manifest.json').read_text(encoding='utf-8'))
    for section in ('original_files','cache_files'):
        for item in manifest[section]:
            rel=Path(item['path'])
            if rel.is_absolute() or '..' in rel.parts: raise ValueError('Unsafe manifest path')
            if sha(destination/rel)!=item['sha256']: raise ValueError(f'Hash mismatch: {rel}')
    for item in manifest.get('omitted_files',[]):
        rel=Path(item['path'])
        if rel.is_absolute() or '..' in rel.parts: raise ValueError('Unsafe omitted path')
        if (destination/rel).exists(): raise ValueError(f'Omitted source asset remains: {rel}')
    print(f'VERIFIED {len(manifest["original_files"])} original files and {len(manifest["cache_files"])} cache files')

def inspect_source(src):
    if not (src/'gameconfig.txt').is_file(): raise ValueError('Choose the extracted GAME folder containing gameconfig.txt, not the platform bundle or Android installer')
    config={}
    for line in (src/'gameconfig.txt').read_text(encoding='utf-8-sig').splitlines():
        k,sep,v=line.partition(',')
        if sep: config[k.strip().lower()]=v.strip()
    if config.get('scripttype','').lower() not in ('pymo','pym'): raise ValueError('Only text PyMO scripts are supported; other script languages need a separate importer')
    scripts=sorted((src/'script').glob('*.txt'))
    if not scripts: raise ValueError('Expected UTF-8 script/*.txt; packed/encrypted scripts are not supported')
    start=safe_name(config.get('startscript',''))
    if not (src/'script'/(start+'.txt')).is_file(): raise ValueError(f'Missing start script: script/{start}.txt')
    for script in scripts: script.read_text(encoding='utf-8-sig')
    return config

def video_file(relative):
    # PyMO movie assets live under movie/video, including packed containers.
    return any(part.casefold() in ('movie','video','movies','videos') for part in relative.parts[:-1]) or relative.suffix.casefold() in ('.mp4','.avi','.mpg','.mpeg','.mkv','.webm','.3gp','.wmv','.mov','.ogv','.flv','.m4v')

def launcher_bitmap(lines):
    from pixel_font import read_bdf
    codes=set(map(ord,''.join(lines)))|{ord('…'),ord('?')}
    glyphs=read_bdf(Path(__file__).parent/'fonts/pixel/Fusion/fusion-pixel-12px-proportional-zh_hans.bdf.gz',codes)
    image=Image.new('L',(200,14*len(lines)))
    for row,text in enumerate(lines):
        def width(value): return sum(glyphs.get(ord(c),glyphs[ord('?')])[0] for c in value)
        if width(text)>200:
            while text and width(text+'…')>200:text=text[:-1]
            text+='…'
        x=0
        for c in text:
            advance,bits=glyphs.get(ord(c),glyphs[ord('?')])
            for y in range(14):
                for xx in range(16):
                    if x+xx<200 and bits[y+2]&(1<<xx):image.putpixel((x+xx,row*14+y),255)
            x+=advance
    return bytes(sum(1<<bit for bit in range(8) if image.getpixel((x+bit,y))) for y in range(image.height) for x in range(0,200,8))

def launcher_title(config,font,target):
    title=config.get('gametitle','Game').replace('\\n','\n').splitlines()[0]
    rows=launcher_bitmap([title])
    target.write_bytes(struct.pack('<4sHH',b'NLT1',200,14)+rows)

def launcher_presentation(src, config, font, cache):
    lines=config.get('gametitle','Game').replace('\\n','\n').splitlines()[:3]
    lines += ['']*(3-len(lines));bits=launcher_bitmap(lines)
    (cache/'launcher-meta.bin').write_bytes(struct.pack('<4sHH',b'NLM1',200,42)+bits)
    icon=src/'icon.png';result={'metadata_lines':lines,'icon':'engine fallback'}
    if icon.is_file():
        with Image.open(icon) as source:
            if source.width*source.height>16777216:raise ValueError('Game icon exceeds presentation conversion limit')
            image=source.convert('RGBA');image.thumbnail((48,48),Image.Resampling.LANCZOS)
        canvas=Image.new('RGBA',(48,48));canvas.alpha_composite(image,((48-image.width)//2,(48-image.height)//2))
        pixels=canvas.get_flattened_data() if hasattr(canvas,'get_flattened_data') else canvas.getdata()
        data=b''.join(struct.pack('<H',0 if a<128 else 0x8000|(r>>3)|((g>>3)<<5)|((b>>3)<<10)) for r,g,b,a in pixels)
        (cache/'launcher-icon.bin').write_bytes(struct.pack('<4sHH',b'NLI1',48,48)+data);result['icon']='icon.png'
    return result

def convert(args):
    src=args.source.resolve();dst=args.destination.resolve()
    if not (src/'gameconfig.txt').is_file(): raise ValueError('Source must be a game directory containing gameconfig.txt')
    if dst.exists() or dst==src or src in dst.parents: raise ValueError('Destination must be new and outside source')
    source_files=sorted(p for p in src.rglob('*') if p.is_file() and 'save' not in p.relative_to(src).parts and CACHE not in p.relative_to(src).parts)
    if any(p.is_symlink() for p in src.rglob('*')): raise ValueError('Symlinks are not accepted')
    originals=[{'path':p.relative_to(src).as_posix(),'sha256':sha(p)} for p in source_files]
    source_hashes=originals
    omitted=[dict(item,bytes=(src/item['path']).stat().st_size,reason='video safely skipped on NDS') for item in originals if video_file(Path(item['path']))]
    originals=[item for item in originals if not video_file(Path(item['path']))]
    config=inspect_source(src)
    width,height=map(int,config['imagesize'].split(','))
    if not 1<=width<=65535 or not 1<=height<=65535: raise ValueError('Invalid imagesize')
    scale=min(256/width,192/height,1)
    font=Path(args.font or Path(__file__).parent/'fonts/NotoSansCJKsc-Regular.otf').resolve()
    if not font.is_file(): raise ValueError('Font missing; use --font')
    ffmpeg=shutil.which(args.ffmpeg)
    if not ffmpeg: raise ValueError('FFmpeg missing; install it or provide --ffmpeg PATH')
    ffversion=subprocess.check_output([ffmpeg,'-version'],creationflags=0x08000000 if os.name=='nt' else 0,text=True,encoding='utf-8',errors='replace').splitlines()[0]
    stage=dst.parent/('.'+dst.name+'.partial-'+uuid.uuid4().hex)
    stage.mkdir(parents=True)
    print('Staging:',stage,flush=True)
    cache_only=getattr(args,'cache_only',False)
    if cache_only:
        media_ext={'.jpg','.jpeg','.png','.bmp','.webp','.mp3','.wav','.ogg','.flac','.pcm','.pak'}
        dropped=[item for item in originals if Path(item['path']).parts[0] in ('bg','bgm','chara','se','voice','system') and Path(item['path']).suffix.lower() in media_ext]
        omitted.extend(dict(item,bytes=(src/item['path']).stat().st_size,reason='redundant original media; NDS cache generated and verified') for item in dropped)
        drop_names={x['path'] for x in dropped};originals=[item for item in originals if item['path'] not in drop_names]
    for item in originals:
        target=stage/item['path'];target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(src/item['path'],target)
    (stage/'save').mkdir(exist_ok=True)
    cache=stage/CACHE;cache.mkdir();(cache/'format.txt').write_bytes(b'CPYMO-NDS-CACHE-1\n');launcher_title(config,font,cache/'launcher-title.bin');presentation=launcher_presentation(src,config,font,cache)
    folders={}
    for kind in ('bg','chara','bgm','voice','se','system'):
        ext=config.get(kind+'format','png' if kind=='system' else 'mp3' if kind in ('bgm','voice','se') else 'jpg')
        folders[kind]=assets(src/kind,ext)
        if kind=='chara' and not (src/kind/(kind+'.pak')).is_file():
            masks=assets(src/kind,config.get('charamaskformat',ext))
            folders[kind].update({k:v for k,v in masks.items() if k.lower().endswith('_mask')})
    # Opt-in 1.2 research: ingest only already-encoded FVDS files, without
    # re-encoding or changing any game scripts, saves or the existing cache
    # formats. The normal conversion still omits all videos.
    video_entries=[]
    if getattr(args,'video_fvds',False):
        fv_candidates=[p for p in source_files
                       if len(p.relative_to(src).parts)==2
                       and p.parent.name.casefold() in ('video','movie')
                       and p.suffix.casefold()=='.fv']
        seen_video={}
        for asset in fv_candidates:
            name=asset.stem
            hashed=key(name)
            if hashed in seen_video:
                raise ValueError(f'Ambiguous FVDS movie identity: {seen_video[hashed]} vs {asset}')
            seen_video[hashed]=asset
            report=inspect_fvds(asset,hash_file=True)
            if (report['width'],report['height'])!=(256,192) or report['frames']>18000:
                raise ValueError(f'FVDS unsupported display geometry/duration: {asset}')
            if report['audio_sample_rate']!=47605 or report['audio_channels']!=2:
                raise ValueError(f'FVDS audio format not supported for probe: {asset}')
            output=cache/'video'/hashed[:2]/(hashed+'.fv')
            output.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(asset,output)
            if sha(output)!=report['sha256']:
                raise ValueError(f'FVDS copy checksum mismatch: {asset}')
            video_entries.append({'name':name,'path':output.relative_to(stage).as_posix(),
                                  'bytes':report['size_bytes'],'frames':report['frames'],
                                  'keyframes':report['keyframe_count'],'duration_seconds':report['duration_seconds'],
                                  'source_sha256':report['sha256'],'status':'preflight only; not yet playable'})
        retained={p.relative_to(src).as_posix() for p in fv_candidates}
        omitted=[item for item in omitted if item['path'] not in retained]

    # 1.2 public playback path: independently encoded NVL1/NVA1 from
    # FFmpeg-decoded source movie; original game assets/scripts are untouched.
    video_mode=getattr(args,'video_mode','skip')
    if video_mode not in ('skip','nvl1','nva1'):
        raise ValueError('Unknown NDS video conversion mode')
    if video_mode!='skip' and getattr(args,'video_fvds',False):
        raise ValueError('Choose playable NVL1/NVA1 or research FVDS preflight, not both')
    playable_videos=[]
    if video_mode!='skip':
        names={}
        for src_video in source_files:
            rel=src_video.relative_to(src)
            if (len(rel.parts)!=2 or
                    rel.parts[0].casefold() not in ('movie','video','movies','videos') or
                    rel.suffix.casefold() not in video_nvl.VIDEO_SUFFIXES):
                continue
            name=src_video.stem
            hashed=key(name)
            if hashed in names:
                raise ValueError(f'Ambiguous video name: {names[hashed]} vs {src_video}')
            names[hashed]=str(src_video)
            output=cache/'video'/hashed[:2]/(hashed+'.nvl')
            print(f'Converting movie {name}: {video_mode}',flush=True)
            meta=video_nvl.encode(src_video,output,ffmpeg,
                                  fps=getattr(args,'video_fps',12),
                                  audio='nar' if video_mode=='nva1' else 'pcm')
            playable_videos.append({'name':name,'path':output.relative_to(stage).as_posix(),
                'source_sha256':sha(src_video),**meta})
            for item in omitted:
                if item['path']==rel.as_posix():
                    item['reason']='source video replaced with playable '+meta['format']+' cache'
    # Some legacy packers included Windows Thumbs.db in image packages.
    # Recognize both its name and OLE signature; never hide corrupt game images.
    ignored=[]
    for kind in ('bg','chara','system'):
        for name,asset in list(folders[kind].items()):
            if name.casefold() in ('thumbs','thumbs.db') and asset.read().startswith(b'\xd0\xcf\x11\xe0\xa1\xb1\x1a\xe1'):
                ignored.append({'kind':kind,'name':name,'reason':'Windows thumbnail database, preserved in original package'})
                del folders[kind][name]
    images=[];audio=[];masks=[];seen={}
    def target(kind,name,suffix):
        hashed=key(name);identity=(kind,hashed)
        if identity in seen and seen[identity]!=name: raise ValueError('Hash collision; rename the conflicting source asset')
        seen[identity]=name
        return cache/kind/(hashed+suffix)
    for kind in ('bg','chara','system'):
        entries=folders[kind]
        # Legacy Symbian packers may uppercase names, including the mask suffix.
        # The runtime asset/cache identity is already ASCII case-insensitive.
        by_lower={name.lower():asset for name,asset in entries.items()}
        for name,asset in entries.items():
            if kind!='bg' and name.lower().endswith('_mask') and name[:-5].lower() in by_lower: continue
            variants=('system0','system1') if kind=='system' else (kind,)
            for variant in variants:
                mask=by_lower.get((name+'_mask').lower()) if variant in ('chara','system1') and config.get('platform','').lower() in ('s60v3','s60v5','symbian') else None
                meta=image_cache(asset,target(variant,name,'.npi'),scale,mask,kind=='bg')
                images.append({'kind':variant,'name':name,**meta})
    for name,asset in folders['system'].items():
        meta=mask_cache(asset,target('mask',name,'.nmt'),min(256,max(1,round(width*scale))),min(192,max(1,round(height*scale))))
        masks.append({'name':name,**meta})
    audio_jobs=[]
    audio_mode=getattr(args,'audio_lossless','off')
    if audio_mode not in ('off','voice','all'):raise ValueError('Unknown lossless audio mode')
    for kind in ('bgm','se','voice'):
        for name,asset in folders[kind].items(): audio_jobs.append((kind,name,asset,target(kind,name,'.pcm')))
    def audio_job(job):
        kind,name,asset,out=job;out.parent.mkdir(parents=True,exist_ok=True)
        data=asset.read();temp=out.with_suffix('.input');temp.write_bytes(data)
        input_flags=['-f','s16le','-ar','22050','-ac','1'] if asset.path.suffix.lower()=='.pcm' else []
        try:
            subprocess.run([ffmpeg,'-nostdin','-hide_banner','-loglevel','error','-threads','1','-y',*input_flags,'-i',str(temp),'-vn','-ac','1','-ar','22050','-f','s16le',str(out)],check=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE,creationflags=0x08000000 if os.name=='nt' else 0)
        finally: temp.unlink(missing_ok=True)
        meta={'kind':kind,'name':name,'source_sha256':hashlib.sha256(data).hexdigest(),'bytes':out.stat().st_size,'encoding':'PCM16','pcm_bytes':out.stat().st_size,'decoded_pcm_sha256':sha(out)}
        if audio_mode=='all' or (audio_mode=='voice' and kind=='voice'):
            compressed=out.with_suffix('.nar');result=lossless.encode_audio(out,compressed)
            meta.update(pcm_bytes=result['pcm_bytes'],decoded_pcm_sha256=result['pcm_sha256'])
            if result['nar_bytes']<=result['pcm_bytes']*.9:
                out.unlink();meta.update(bytes=result['nar_bytes'],encoding='NAR2')
            else:compressed.unlink()
        return meta
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        for result in pool.map(audio_job,audio_jobs): audio.append(result)
    from audio_aliases import deduplicate_audio
    audio_dedup=deduplicate_audio(cache,audio,key) if audio_mode!='off' else {'aliases':0,'saved_bytes':0,'index_bytes':0}
    # This converter produces PCM on all channels. Experimental engines can
    # avoid repeated missing-NAR probes; 1.0.0 safely ignores this optional index.
    (cache/'audio-formats.bin').write_bytes(b'NAF1'+bytes([7 if audio_mode=='all' else 4 if audio_mode=='voice' else 0]))
    scripts=sorted((src/'script').glob('*.txt'))
    text=''.join(p.read_text(encoding='utf-8-sig') for p in scripts)+(src/'gameconfig.txt').read_text(encoding='utf-8-sig')
    glyphs=font_cache(text,font,cache/'font.bin')
    pixel_fonts=pixel_font_cache([cp for cp in (set(map(ord,text))|set(range(32,127))|set(UI_CODEPOINTS)) if cp>=32 and cp not in (0xfeff,0xfffe,0xffff)],cache,args.missing_glyph_policy)
    findings=audit(scripts,folders,{v['name'].casefold() for v in playable_videos})
    allowed=sorted({(x['folder'],key(x['name'])) for x in findings['missing_resource_references'] if x['folder'] in ('bgm','se','voice')})
    (cache/'missing-audio.txt').write_bytes(''.join(f'{kind} {hashed}\n' for kind,hashed in allowed).encode('ascii'))
    compressed_images=[]
    if getattr(args,'compress_images',False):
        for path in sorted(cache.rglob('*.npi')):
            raw=path.read_bytes();packed=lossless.image_candidate(raw,compress_spans=getattr(args,'experimental_sparse',False))
            if packed!=raw:
                path.write_bytes(packed);compressed_images.append({'path':path.relative_to(stage).as_posix(),'raw_bytes':len(raw),'bytes':len(packed),'decoded_sha256':hashlib.sha256(raw).hexdigest()})
    layout=getattr(args,'cache_layout','sharded')
    if layout not in ('flat','sharded'):raise ValueError('Unknown cache layout')
    layout_files=0
    if layout=='sharded':
        moved={}
        for folder in sorted(cache.iterdir()):
            if not folder.is_dir():continue
            for path in sorted(folder.iterdir()):
                if not path.is_file() or not re.fullmatch('[0-9a-f]{16}',path.stem):continue
                dest=folder/path.stem[:2]/path.name;dest.parent.mkdir(exist_ok=True)
                old=path.relative_to(stage).as_posix();path.rename(dest);moved[old]=dest.relative_to(stage).as_posix();layout_files+=1
        for entry in compressed_images:entry['path']=moved.get(entry['path'],entry['path'])
    cache_files=[{'path':p.relative_to(stage).as_posix(),'sha256':sha(p)} for p in sorted(cache.rglob('*')) if p.is_file()]
    manifest={'format':1,'converter_version':VERSION,'logical_screen':[width,height],'display_scale':scale,'original_files':originals,'cache_files':cache_files,'images':images,'audio':sorted(audio,key=lambda x:(x['kind'],x['name'])),'font_glyphs':glyphs,'font_sha256':sha(font),'audio_format':'22050 Hz mono signed 16-bit little endian PCM','tools':{'python':sys.version.split()[0],'pillow':pillow_version,'ffmpeg':ffversion},'audit':findings,'save_policy':'new empty save directory; existing saves not imported'}
    if audio_mode!='off' or getattr(args,'compress_images',False):
        manifest['format']=5;(cache/'format.txt').write_bytes(b'CPYMO-NDS-CACHE-5\n')
        manifest['minimum_engine']='r13 research / future 1.1; not 1.0.0 or r9-r12'
        for entry in cache_files:
            if entry['path']=='nds-cache/format.txt':entry['sha256']=sha(cache/'format.txt')
    if layout=='sharded':
        manifest['format']=6;(cache/'format.txt').write_bytes(b'CPYMO-NDS-CACHE-6\n')
        manifest['minimum_engine']='r15 research / future 1.1; not 1.0.0 or r9-r14'
        for entry in cache_files:
            if entry['path']=='nds-cache/format.txt':entry['sha256']=sha(cache/'format.txt')
    manifest['cache_layout']={'name':layout,'prefix_hex_digits':2 if layout=='sharded' else 0,'files':layout_files,'resource_bytes_changed':False}
    manifest['omitted_files']=omitted
    manifest['lossless']={'audio_mode':audio_mode,'audio_dedup':audio_dedup,'audio_saved_bytes':sum(x.get('pcm_bytes',x['bytes'])-x['bytes'] for x in audio),'image_saved_bytes':sum(x['raw_bytes']-x['bytes'] for x in compressed_images),'images':compressed_images,'image_threshold':{'NPA4':0.2,'NPS1':0.9},'audio_threshold':0.9,'quality':'exact relative to existing NDS RGB555/PCM16 cache; no extra downsampling or quantization'}
    manifest['lossless']['experimental_sparse']=getattr(args,'experimental_sparse',False)
    if audio_mode!='off':manifest['audio_format']='PCM16/NAR2 per file; identical 22050 Hz mono PCM16 samples'
    manifest['source_files']=source_hashes
    manifest['video_policy']=('playable NVL1/NVA1 converted from source videos; unconverted movie references still skipped'
                              if video_mode!='skip'
                              else 'opt-in FVDS cache pass-through; 1.2 runtime preflight only; video decoding not implemented'
                              if getattr(args,'video_fvds',False)
                              else 'omit video assets; keep scripts; runtime safely skips movie commands')
    manifest['videos']=playable_videos
    manifest['fvds_videos']=video_entries
    manifest['resource_policy']='cache-only experimental' if cache_only else 'original media retained'
    manifest['presentation']=presentation
    manifest['source_platform']=config.get('platform','pygame')
    manifest['pixel_fonts']=pixel_fonts
    manifest['mask_transitions']=masks
    manifest['ignored_non_game_entries']=ignored
    (cache/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
    for item in source_hashes:
        if sha(src/item['path'])!=item['sha256']: raise ValueError('Source changed during conversion')
    verify(stage)
    if dst.exists(): raise ValueError('Destination appeared during conversion; partial output preserved')
    stage.rename(dst)
    print(f'COMPLETE: {dst}\n{len(images)} image variants, {len(audio)} audio files, {glyphs} glyphs; source and scripts unchanged',flush=True)
    print(f'Audit: {len(findings["missing_resource_references"])} missing references, {len(findings["unknown_commands"])} unknown directives, {len(findings["unsupported_video_commands"])} skipped video commands; see nds-cache/manifest.json')

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    sub=parser.add_subparsers(dest='command',required=True)
    c=sub.add_parser('convert');c.add_argument('source',type=Path);c.add_argument('destination',type=Path);c.add_argument('--ffmpeg',default='ffmpeg');c.add_argument('--font');c.add_argument('--jobs',type=int,choices=range(1,9),default=4);c.add_argument('--cache-only',action='store_true',help='Omit redundant source media, retain scripts/config/credits and verify caches');c.add_argument('--missing-glyph-policy',choices=('error','box'),default='box',help='Default box fallback with warning for missing pixel-font characters; original scripts unchanged')
    c.add_argument('--audio-lossless',choices=('off','voice','all'),default='off');c.add_argument('--compress-images',action='store_true',help='Lossless NPA4 or row-span NPS1 selected per image; keep raw when not worthwhile')
    c.add_argument('--cache-layout',choices=('flat','sharded'),default='sharded',help='Two-hex directory fanout avoids large flat FAT directories; requires r15 engine')
    c.add_argument('--experimental-sparse',action='store_true',help='Private NPS2: smaller transparent images; may increase scene-change stalls')
    c.add_argument('--video-fvds',action='store_true',help='Research-only: validate and copy pre-encoded video/*.fv into hashed NDS cache; NOT playback')
    c.add_argument('--video-mode',choices=('skip','nvl1','nva1'),default='nvl1',help='Source video conversion: default NVL1 PCM, optionally NVA1 NAR2 or skip (legacy)')
    c.add_argument('--video-fps',type=int,choices=(6,8,12,15,20,24),default=12,help='Output video FPS, default 12 for NDS')
    v=sub.add_parser('verify');v.add_argument('destination',type=Path)
    i=sub.add_parser('inspect');i.add_argument('source',type=Path)
    args=parser.parse_args()
    try:
        if args.command=='convert': convert(args)
        elif args.command=='inspect': print(json.dumps(inspect_source(args.source.resolve()),ensure_ascii=False,indent=2))
        else: verify(args.destination.resolve())
    except (ValueError,OSError,subprocess.SubprocessError,UnicodeError,KeyError) as error:
        if isinstance(error,subprocess.CalledProcessError) and error.stderr: print(error.stderr.decode('utf-8',errors='replace') if isinstance(error.stderr,bytes) else error.stderr,file=sys.stderr)
        print(f'ERROR: {error}\nNo source files were modified; an owned .partial directory may remain for diagnosis.',file=sys.stderr)
        return 1
    return 0

if __name__=='__main__': raise SystemExit(main())
