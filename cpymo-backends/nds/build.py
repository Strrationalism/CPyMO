"""Build the native DS backend using the pinned devkitPro SDK; Python 3.10+."""
from pathlib import Path
import subprocess, json, hashlib, os, sys
from rom_banner import patch_nds_banner_titles
experiment = sys.argv[1:] == ['--dsi-experiment']
if sys.argv[1:] and not experiment:
 raise SystemExit('usage: build.py [--dsi-experiment]')
project=Path(__file__).resolve().parent
sdk=Path(os.environ['DEVKITPRO']).resolve()
exe='.exe' if os.name=='nt' else ''
engine=project
build=engine/('build-dsi-experiment' if experiment else 'build')
build.mkdir(exist_ok=True)
core=project.parents[1]
prefix_flags=[]
for path,replacement in [(core,'.'),(sdk,'/opt/devkitpro')]:
 for spelling in sorted({str(path),path.as_posix()}):
  prefix_flags.append(f'-ffile-prefix-map={spelling}={replacement}')
record=engine/('build-records-dsi-experiment' if experiment else 'build-records')
record.mkdir(parents=True,exist_ok=True)
# Regenerate the self-contained ROM bitmap UI font before compiling.
# Every released locale must draw new engine-owned menu labels even for
# legacy converted games whose per-title cache lacks these glyphs.
subprocess.run([sys.executable,str(engine/'tools/generate-ui-font.py')],check=True)
release=json.loads((engine/'release.json').read_text(encoding='utf8'))
assert (engine/'VERSION').read_text().strip()==release['version'], 'VERSION/release.json mismatch'
(build/'release-version.h').write_text('#define CPYMO_NDS_VERSION '+json.dumps(release['version'])+'\n',encoding='ascii')
commands=[]
with (record/'native-windows-build.log').open('w',encoding='utf-8') as log:
 def run(args):
  args=list(map(str,args));commands.append(args)
  log.write(json.dumps(args)+'\n');log.flush()
  try:
   subprocess.run(args,cwd=build,stdout=log,stderr=subprocess.STDOUT,check=True)
  except subprocess.CalledProcessError:
   # CI previously emitted only the Python traceback; show the actual GCC
   # diagnostics while retaining the full reproducible build record on disk.
   log.flush()
   lines=(record/'native-windows-build.log').read_text(encoding='utf-8',errors='replace').splitlines()
   print('NDS compiler/linker failure — last 90 build-log lines:',file=sys.stderr)
   errors=[i for i,line in enumerate(lines) if ('error:' in line or 'undefined reference' in line or 'fatal error:' in line)]
   for i in errors[:15]:
    print('\\n'.join(lines[max(0,i-3):min(len(lines),i+3)]),file=sys.stderr)
   print('\\n'.join(lines[-25:]),file=sys.stderr)
   raise
 gcc=sdk/('devkitARM/bin/arm-none-eabi-gcc'+exe)
 flags=[*prefix_flags,'-D__NDS__','-DARM9','-std=gnu11','-g','-Wall','-O2','-DHAVE_CONFIG_H','-DDISABLE_MOVIE','-DDISABLE_FFMPEG_AUDIO','-DENABLE_TEXT_EXTRACT','-DSTREAMING_LOAD_IMAGE','-ffunction-sections','-fdata-sections','-march=armv5te','-mtune=arm946e-s','-marm']
 if experiment:
  flags.append('-DCPYMO_DSI_MEMORY_LAYOUT')
 for inc in [sdk/'calico/include',sdk/'libnds/include',engine/'include',build,engine/'decoder',core/'cpymo',core/'cpymo-backends/include',core/'stb']:
  flags+=['-I'+str(inc)]
 objects=[]
 local_sources={p.name for p in (engine/'source').glob('*.c')}
 for folder in [engine/'source',engine/'decoder',core/'cpymo']:
  for source in sorted(folder.glob('*.c')):
   if source.name=='cpymo_audio.c':continue
   if folder==core/'cpymo' and source.name in local_sources:continue
   obj=build/(source.stem+'.o');objects.append(obj)
   run([gcc,*flags,'-MMD','-MP','-MF',obj.with_suffix('.d'),'-c',source,'-o',obj])
 wraps='-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=stbi_load_from_callbacks,--wrap=cpymo_assetloader_load_bg_image,--wrap=cpymo_assetloader_load_chara_image,--wrap=cpymo_assetloader_load_system_image,--wrap=cpymo_movie_play,--wrap=cpymo_localization_get,--wrap=cpymo_rmenu_enter,--wrap=cpymo_ui_enter,--wrap=cpymo_charas_wait'
 if (engine/'source/masktrans-nds.h').exists():wraps+=',--wrap=cpymo_assetloader_load_system_masktrans'
 elf=engine/('engine-dsi-experiment.elf' if experiment else 'engine.elf')
 run([gcc,'-L'+str(sdk/'calico/lib'),'-L'+str(sdk/'libnds/lib'),'-specs='+str(sdk/'calico/share/ds9.specs'),'-g','-march=armv5te','-mtune=arm946e-s','-marm','-Wl,-Map,engine.map',wraps,*objects,'-lmm9','-lfat','-lnds9','-lm','-lcalico_ds9','-o',elf])
 icon=engine/'assets/icon.bmp'
 arm7=sdk/'calico/bin/ds7_maine.elf'
 if (engine/'arm7/main.c').exists():
  arm7=engine/('arm7-dsi-experiment.elf' if experiment else 'arm7.elf')
  run([gcc,*prefix_flags,'-D__NDS__','-DARM7','-std=gnu11','-g','-Wall','-O2','-ffunction-sections','-fdata-sections','-march=armv4t','-mtune=arm7tdmi','-mthumb','-I'+str(sdk/'calico/include'),'-I'+str(sdk/'libnds/include'),'-c',engine/'arm7/main.c','-o',build/'arm7-main.o'])
  run([gcc,'-L'+str(sdk/'calico/lib'),'-L'+str(sdk/'libnds/lib'),'-specs='+str(sdk/'calico/share/ds7.specs'),'-g','-march=armv4t','-mtune=arm7tdmi','-mthumb',build/'arm7-main.o','-lmm7','-lcalico_ds7','-lm','-o',arm7])
 version=(engine/'VERSION').read_text().strip() if (engine/'VERSION').exists() else '0.5.0-test'
 rom=engine/('cpymo-dsi-experiment.nds' if experiment else 'engine.nds')
 # ndstool's -b parser handles one-byte characters, not UTF-8 text.
 # Generate a safe ASCII banner first; then write the desired UTF-16LE
 # attribution and refresh the banner CRC16 in the finished ROM.
 run([sdk/('tools/bin/ndstool'+exe),'-c',rom,'-9',elf,'-7',arm7,'-b',icon if icon.exists() else sdk/'calico/share/nds-icon.bmp',f'CPyMO for NDS;{version};NDS port' if icon.exists() else 'engine;built with devkitARM;http://devkitpro.org'])
 if icon.exists():
  patch_nds_banner_titles(rom,('CPyMO for NDS',version,'千岚之夏'))
 run([sdk/('devkitARM/bin/arm-none-eabi-size'+exe),elf])
(record/'native-windows-build-commands.json').write_text(json.dumps(commands,indent=2),encoding='utf-8')
rom=engine/('cpymo-dsi-experiment.nds' if experiment else 'engine.nds')
print(json.dumps({'experiment':experiment,'bytes':rom.stat().st_size,'sha256':hashlib.sha256(rom.read_bytes()).hexdigest(),'objects_rebuilt':len(objects)}))
