"""CPyMO NDS conversion assistant backend. MIT. NDS port: 千岚之夏.

Input archives and game scripts are read-only. Each conversion gets a new job
directory; only a selected game's files are extracted, never the whole bundle.
"""
from pathlib import Path, PurePosixPath
import argparse, contextlib, hashlib, json, os, shutil, stat, sys, tempfile
import zipfile
import convert
from release_info import VERSION

MAX_FILES = 30000
MAX_BYTES = 8 * 1024**3
ROOT = Path(__file__).resolve().parent

def checked_name(name):
    name=name.replace('\\','/')
    path=PurePosixPath(name)
    if not name or path.is_absolute() or '..' in path.parts or ':' in name or '\0' in name:
        raise ValueError('压缩包含不安全的文件路径，已停止。')
    return path.as_posix()

def archive_entries(archive):
    entries=archive.infolist()
    if len(entries)>MAX_FILES or sum(i.file_size for i in entries)>MAX_BYTES:
        raise ValueError('压缩包过大，超出本工具的安全处理范围。')
    for entry in entries:
        checked_name(entry.filename)
        if stat.S_ISLNK(entry.external_attr>>16):
            raise ValueError('压缩包含符号链接，已停止。')
    return entries

def configuration(data):
    if len(data)>65536:raise ValueError('游戏配置文件异常过大。')
    values={}
    for line in data.decode('utf-8-sig').splitlines():
        key,sep,value=line.partition(',')
        if sep:values[key.strip().lower()]=value.strip()
    return values

def discover(source,temp):
    source=Path(source).resolve();temp=Path(temp).resolve();candidates=[]
    if source.is_dir():
        count=0
        for folder,dirs,files in os.walk(source,followlinks=False):
            dirs[:]=sorted(d for d in dirs if d not in ('save','nds-cache','.git') and not (Path(folder)/d).is_symlink())
            count+=len(files)
            if count>MAX_FILES:raise ValueError('选择的目录文件太多，请直接选择游戏数据目录。')
            if 'gameconfig.txt' in files:
                p=Path(folder)/'gameconfig.txt';c=configuration(p.read_bytes())
                candidates.append({'chain':[],'root':str(Path(folder).resolve()),'title':c.get('gametitle',Path(folder).name).replace('\\n',' / '),'platform':c.get('platform','未知'),'kind':'folder'})
    elif source.suffix.casefold() in ('.zip',):
        def scan(path,chain,depth):
            with zipfile.ZipFile(path) as archive:
                entries=archive_entries(archive)
                for entry in entries:
                    name=checked_name(entry.filename)
                    if PurePosixPath(name).name=='gameconfig.txt' and not entry.is_dir():
                        c=configuration(archive.read(entry));root=name[:-len('gameconfig.txt')]
                        candidates.append({'chain':chain,'root':root,'title':c.get('gametitle',PurePosixPath(root).name or source.stem).replace('\\n',' / '),'platform':c.get('platform','未知'),'kind':'archive','archive':str(path)})
                if depth<2:
                    nested=sorted((e for e in entries if Path(e.filename).suffix.casefold() in ('.zip',) and not e.is_dir()),key=lambda e:e.filename.casefold())
                    if len(nested)>12:raise ValueError('嵌套游戏包太多，请单独选择要转换的游戏包。')
                    for entry in nested:
                        name=checked_name(entry.filename);next_path=temp/(hashlib.sha256(('/'.join(chain+[name])).encode()).hexdigest()+'.zip')
                        with archive.open(entry) as inp,next_path.open('wb') as out:shutil.copyfileobj(inp,out,1024*1024)
                        scan(next_path,chain+[name],depth+1)
        scan(source,[],0)
    else:raise ValueError('请选择游戏文件夹，或 ZIP 压缩包。')
    rank={'s60v3':0,'symbian':0,'s60v5':1,'android':2,'pygame':3}
    candidates.sort(key=lambda c:(rank.get(c['platform'].casefold(),9),c['title'],c['chain'],c['root']))
    for index,candidate in enumerate(candidates):candidate['id']=index
    if not candidates:raise ValueError('没有找到 gameconfig.txt。请选 PyMO 游戏数据包；加密脚本或其他引擎的游戏暂不支持。')
    return candidates

def extract_game(candidate,destination):
    destination.mkdir(parents=True)
    with zipfile.ZipFile(candidate['archive']) as archive:
        for entry in archive_entries(archive):
            name=checked_name(entry.filename)
            if not name.startswith(candidate['root']):continue
            rel=PurePosixPath(name[len(candidate['root']):])
            if not rel.parts or any(p in ('save','nds-cache') for p in rel.parts):continue
            target=destination.joinpath(*rel.parts)
            if not target.resolve().is_relative_to(destination.resolve()):raise ValueError('解压路径异常。')
            if entry.is_dir():target.mkdir(parents=True,exist_ok=True)
            else:
                target.parent.mkdir(parents=True,exist_ok=True)
                with archive.open(entry) as inp,target.open('xb') as out:shutil.copyfileobj(inp,out,1024*1024)
    return destination

def public_candidates(candidates):
    return [{k:v for k,v in c.items() if k!='archive'} for c in candidates]

def run_job(job):
    """Jobs are also used by the frozen EXE tests; no external Python required."""
    source=Path(job['source']).resolve();result=Path(job['result']).resolve()
    if source.is_dir() and result.parent.is_relative_to(source):
        raise ValueError('诊断位置不能放在原游戏目录内部，请选其他文件夹。')
    result.parent.mkdir(parents=True,exist_ok=True)
    try:
        with tempfile.TemporaryDirectory(prefix='inspect-',dir=result.parent) as temporary:
            candidates=discover(source,Path(temporary))
            if job['action']=='inspect':
                report={'ok':True,'candidates':public_candidates(candidates)}
            else:
                index=int(job.get('candidate',0))
                if not 0<=index<len(candidates):raise ValueError('所选游戏已改变，请重新选择游戏包。')
                candidate=candidates[index];output=Path(job['output']).resolve()
                if output.exists():raise ValueError('目标目录已经存在，请使用新目录。原目录不会被覆盖。')
                if source.is_dir() and output.is_relative_to(source):raise ValueError('输出目录不能放在原游戏目录内部。')
                extracted=Path(temporary)/'selected-game'
                game_source=Path(candidate['root']) if candidate['kind']=='folder' else extract_game(candidate,extracted)
                print('已识别：'+candidate['title']+' / '+candidate['platform'],flush=True)
                config=convert.inspect_source(game_source)
                name=Path(candidate['root'].rstrip('/\\')).name or 'Game'
                name=''.join(c for c in name if c.isascii() and (c.isalnum() or c in '_-')) or 'Game'
                destination=output/'pymo/games'/name
                bundle=Path(getattr(sys,'_MEIPASS',ROOT))
                ffmpeg=next((p for p in (bundle/'ffmpeg.exe',bundle/'ffmpeg',ROOT/'ffmpeg.exe',ROOT/'ffmpeg') if p.is_file()),None)
                if ffmpeg is None:
                    external=job.get('ffmpeg') or shutil.which('ffmpeg')
                    ffmpeg=Path(external) if external else None
                if ffmpeg is None or not ffmpeg.is_file():raise ValueError('缺少视频和音频转换组件 FFmpeg，请完整解压转换助手。')
                print('正在转换图片、音频和字体，游戏较大时需要等待……',flush=True)
                cache_only=job.get('cache_only',True)
                if not isinstance(cache_only,bool):raise ValueError('精简资源选项必须为 true 或 false。')
                audio_mode=job.get('audio_lossless','voice')
                if audio_mode not in ('off','voice','all'):raise ValueError('无损压缩模式无效。')
                video_mode=job.get('video_mode','nvl1')
                if video_mode not in ('skip','nvl1','nva1'):raise ValueError('视频转换模式无效。')
                convert.convert(argparse.Namespace(source=game_source,destination=destination,ffmpeg=str(ffmpeg),font=None,jobs=4,missing_glyph_policy=job.get('missing_policy','box'),cache_only=cache_only,audio_lossless=audio_mode,compress_images=audio_mode!='off',video_mode=video_mode,video_fps=12))
                manifest=json.loads((destination/'nds-cache/manifest.json').read_text(encoding='utf8'))
                warnings=[];audit=manifest['audit']
                for label,key in [('未知脚本指令','unknown_commands'),('缺失资源引用','missing_resource_references'),('需运行检查的指令','commands_requiring_game_runtime_check')]:
                    if audit[key]:warnings.append(f'{label}：{len(audit[key])}项')
                boxed=sorted(set(c for font in manifest.get('pixel_fonts',[]) for c in font.get('boxed_codepoints',[])))
                if boxed:warnings.append('字库缺字以□显示，原脚本未改：'+', '.join(boxed))
                rom=Path(getattr(sys,'_MEIPASS',ROOT))/'cpymo-nds.nds'
                if rom.is_file():shutil.copy2(rom,output/'cpymo-nds.nds')
                report={'ok':True,'status':'已转换，仍需实际运行确认' if warnings else '转换和完整性检查通过，仍需实际运行确认','game':candidate['title'],'platform':candidate['platform'],'game_directory':str(destination),'output':str(output),'original_files_checked':len(manifest.get('source_files',manifest['original_files'])),'cache_files_checked':len(manifest['cache_files']),'warnings':warnings,'compatibility_certified':False,'video_policy':manifest['video_policy'],'videos':manifest.get('videos',[]),'missing_policy':job.get('missing_policy','box'),'source_modified':False}
                report.update(release_version=convert.VERSION,cache_format=manifest['format'],rom_sha256=hashlib.sha256(rom.read_bytes()).hexdigest() if rom.is_file() else None)
                report.update(cache_only=cache_only,resource_policy=manifest['resource_policy'],redundant_media_bytes=sum(item['bytes'] for item in manifest.get('omitted_files',[]) if item.get('reason','').startswith('redundant original media')))
                report.update(lossless=manifest['lossless'])
                instructions='安装：先备份TF卡上同一游戏的save。将本目录中的cpymo-nds.nds和pymo文件夹复制到TF根目录，保留现有烧录卡内核。不要用本包空save覆盖自己的存档。\n原游戏包、脚本和汉化内容未修改。视频按本次选择转换或省略。转换通过不等于已经验证所有剧情、烧录卡或 DSi 启动方式的兼容性。\nNDS平台移植：千岚之夏；CPyMO/PyMO与依赖的原署名和许可证保留。\n'+('\n注意：\n'+'\n'.join(warnings) if warnings else '')
                if cache_only:instructions+='\n本次使用精简资源：仅省去已有NDS缓存的重复媒体；画质音质不变，脚本与署名保留。精简后的目录用于NDS，请保留原包供其他平台使用。\n'
                if audio_mode!='off':instructions+=f'\n本次启用无损缓存压缩，请使用随附的 {VERSION} 引擎。像素和音频采样与未压缩 NDS 缓存一致；BGM/音效默认保持 PCM。\n' if audio_mode=='voice' else f'\n本次启用实验性全部音频压缩，请使用随附的 {VERSION} 引擎；此选项不属于默认性能基线。\n'
                (output/'安装必读.txt').write_text(instructions,encoding='utf-8-sig')
                if job.get('ui_language')=='English':
                    english=(
                        "CPyMO for NDS — installation guide\n"
                        "Back up the existing save/ folder for the same game first.\n"
                        "Copy cpymo-nds.nds and the pymo folder from this output\n"
                        "to the root of your DS / DSi storage card. Keep your\n"
                        "flashcart launcher/kernel and existing personal saves.\n"
                        "NEVER overwrite your save with this package's empty one.\n"
                        "The original game, scripts and localization credits are unchanged.\n"
                        "Video mode: "+video_mode.upper()+". Fonts lacking glyphs use □\n"
                        "with warnings, but source scripts are never rewritten.\n"
                        "Successful conversion does not guarantee compatibility\n"
                        "with every game, video or DS/DSi launch environment.\n"
                        "See 转换结果.json and nds-cache/manifest.json for the audit.\n"
                        "NDS port: 千岚之夏. Upstream and third-party credits retained.\n"
                    )
                    (output/'INSTALL-READ-ME.txt').write_text(english,encoding='utf8')
                (output/'转换结果.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8')
    except Exception as error:
        report={'ok':False,'error':str(error),'source_modified':False,'partial_output_may_remain':True}
        import traceback;traceback.print_exc()
    result.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8')
    return report
