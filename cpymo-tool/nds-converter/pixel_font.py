"""Native bitmap subset generator (MIT); font data retains upstream OFL licenses."""
from pathlib import Path
import gzip, struct, shutil, hashlib

def read_bdf(path, codes=None):
    glyphs={}; state={}; bitmap=False
    with gzip.open(path,'rt',encoding='ascii') as stream:
        for line in stream:
            line=line.strip()
            if line.startswith('STARTCHAR'): state={}; bitmap=False
            elif line.startswith('ENCODING '): state['code']=int(line.split()[1])
            elif line.startswith('DWIDTH '): state['advance']=int(line.split()[1])
            elif line.startswith('BBX '): state['bbx']=list(map(int,line.split()[1:]))
            elif line=='BITMAP': bitmap=True; state['rows']=[]
            elif line=='ENDCHAR':
                if codes is not None and state['code'] not in codes:
                    bitmap=False
                    continue
                w,h,dx,dy=state['bbx']; rows=[0]*16
                for y,row in enumerate(state['rows']):
                    yy=13-dy-h+y
                    bits=int(row,16); n=len(row)*4
                    for x in range(w):
                        xx=dx+x
                        if bits>>(n-1-x)&1:
                            if not (0<=xx<16 and 0<=yy<16):
                                raise ValueError(f'Bitmap outside 16x16 storage: U+{state["code"]:04X}')
                            rows[yy]|=1<<xx
                glyphs[state['code']]=(state['advance'],rows)
                bitmap=False
            elif bitmap: state['rows'].append(line)
    return glyphs

def read_hex(path, codes=None):
    glyphs={}
    with gzip.open(path,'rt',encoding='ascii') as stream:
        for line in stream:
            code,data=line.strip().split(':'); w=len(data)//4
            if codes is not None and int(code,16) not in codes: continue
            if w not in (8,16): raise ValueError('Unsupported Unifont glyph width')
            rows=[]
            for y in range(16):
                bits=int(data[y*w//4:(y+1)*w//4],16)
                rows.append(sum(1<<x for x in range(w) if bits>>(w-1-x)&1))
            glyphs[int(code,16)]=(w,rows)
    return glyphs

def pixel_font_cache(codes, target, missing_policy='error'):
    from PIL import Image, ImageDraw, ImageFont
    fonts=Path(__file__).parent/'fonts/pixel'
    sources=[('small',fonts/'Fusion/fusion-pixel-12px-proportional-zh_hans.bdf.gz',12,13,read_bdf),
             ('large',fonts/'Unifont/unifont-17.0.05.hex.gz',16,14,read_hex)]
    codes=sorted(set(codes)); report=[]
    for name,path,nominal,baseline,reader in sources:
        glyphs=reader(path,set(codes))
        missing=[c for c in codes if c not in glyphs]
        fallback=[]
        # Preserve native Fusion pixels; use the existing licensed Noto face
        # only for uncovered CJK characters, never guess private-use mappings.
        noto=Path(__file__).parent/'fonts/NotoSansCJKsc-Regular.otf'
        if name=='small' and any(0x3400<=c<=0x9fff for c in missing):
            font=ImageFont.truetype(str(noto),13)
            absent=bytes(font.getmask(chr(0x10ffff)))
            for code in missing:
                if not 0x3400<=code<=0x9fff or bytes(font.getmask(chr(code)))==absent: continue
                image=Image.new('L',(13,14))
                ImageDraw.Draw(image).text((0,11),chr(code),font=font,fill=255,anchor='ls')
                rows=[0,0]+[sum(1<<x for x in range(13) if image.getpixel((x,y))>=100) for y in range(14)]
                advance=min(13,round(font.getlength(chr(code))))
                if not any(rows) or not advance: continue
                glyphs[code]=(advance,rows);fallback.append(code)
            missing=[c for c in missing if c not in glyphs]
            if fallback:
                license_dir=target/'font-licenses/Noto';license_dir.mkdir(parents=True,exist_ok=True)
                shutil.copy2(noto.parent/'OFL.txt',license_dir/'OFL.txt')
                print('INFO: small Noto fallback glyphs: '+','.join(f'U+{c:04X}' for c in fallback),flush=True)
        if missing:
            if missing_policy != 'box':
                raise ValueError(f'{name} pixel font lacks {len(missing)} characters: '+','.join(f'U+{c:04X}' for c in missing[:16]))
            box=reader(path,{0x25a1})
            if 0x25a1 not in box: raise ValueError('Fallback box glyph unavailable')
            for code in missing: glyphs[code]=box[0x25a1]
            print(f'WARNING: {name} displays missing glyphs as boxes: '+','.join(f'U+{c:04X}' for c in missing),flush=True)
        data=bytearray(struct.pack('<4sI4B',b'NPF1',len(codes),16,16,baseline,nominal))
        for code in codes:
            advance,rows=glyphs[code]
            data+=struct.pack('<IB3x16H',code,advance,*rows)
        (target/f'font-{name}.bin').write_bytes(data)
        report.append({'name':name,'nominal_pixels':nominal,'glyphs':len(codes),'source_sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'missing_glyph_policy':missing_policy,'boxed_codepoints':[f'U+{c:04X}' for c in missing],
                       'fallback_codepoints':[f'U+{c:04X}' for c in fallback],
                       'fallback_source':'Noto Sans CJK SC 13px, OFL-1.1' if fallback else None,
                       'fallback_source_sha256':hashlib.sha256(noto.read_bytes()).hexdigest() if fallback else None})
    shutil.copytree(fonts/'Fusion/LICENSES',target/'font-licenses/Fusion/LICENSES',dirs_exist_ok=True)
    shutil.copy2(fonts/'Fusion/OFL.txt',target/'font-licenses/Fusion/OFL.txt')
    shutil.copytree(fonts/'Unifont',target/'font-licenses/Unifont',ignore=shutil.ignore_patterns('*.gz'),dirs_exist_ok=True)
    (target/'font-licenses/NOTICE.txt').write_text('Small: Fusion Pixel 2026.09.25 zh_Hans, TakWolf and contributors. https://github.com/TakWolf/fusion-pixel-font\nLarge: GNU Unifont 17.0.05, Unifont contributors. https://unifoundry.com/unifont/\nUncovered small CJK glyphs, if listed in manifest: Noto Sans CJK SC 13px, Google and contributors, SIL OFL 1.1. Original font and OFL.txt accompany converter sources.\nNPF1 files are bitmap subsets prepared for CPyMO NDS, under SIL OFL 1.1. Names of upstream font families are attribution, not names of a new font family.\n',encoding='utf8')
    return report
