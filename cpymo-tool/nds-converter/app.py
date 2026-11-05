"""CPyMO NDS conversion assistant UI, MIT. NDS port: 千岚之夏."""
from pathlib import Path
import argparse, contextlib, datetime, json, os, subprocess, sys, tempfile
import tkinter as tk
from tkinter import ttk, filedialog, messagebox, font as tkfont
from helper_core import run_job

from release_info import VERSION
from ui_i18n import tr, tr_log, LANGUAGES

def apply_desktop_icon(window):
    """Keep the native macOS bundle icon; use the small PNG on other hosts."""
    # Tk's iconphoto replaces the macOS Dock tile with the 64px window PNG.
    # Frozen .apps already carry the correctly sized, high-resolution ICNS.
    if sys.platform == 'darwin' and getattr(sys, 'frozen', False):
        return False
    base=Path(sys._MEIPASS) if getattr(sys,'frozen',False) else Path(__file__).resolve().parent
    icon=base/'assets/converter-icon-64.png'
    if not icon.is_file():
        return False
    try:
        photo=tk.PhotoImage(master=window,file=str(icon))
        window.iconphoto(True,photo)
        window._cpymo_icon_photo=photo  # Keep a live Tk image reference.
        return True
    except tk.TclError:
        return False  # Never block conversion over a window manager limitation.


class App:
    def __init__(self,window,state_root=None,self_test=None):
        self.window=window;self.process=None;self.self_test=self_test;self.test_started=False
        apply_desktop_icon(window)
        self.state_root=Path(state_root or (Path(os.environ['LOCALAPPDATA'])/'CPyMO-NDS-转换助手'/'jobs' if os.name=='nt' and os.environ.get('LOCALAPPDATA') else Path.home()/'Library/Application Support/CPyMO-NDS-Converter/jobs' if sys.platform=='darwin' else Path.home()/'.local/share/CPyMO-NDS-Converter/jobs'))
        self.preference_file=self.state_root.parent/'ui-preferences.json'
        try:
            saved=json.loads(self.preference_file.read_text(encoding='utf8')).get('language')
        except (OSError, ValueError, TypeError):
            saved=None
        self.language=tk.StringVar(value=saved if saved in LANGUAGES else LANGUAGES[0])
        self._ui_sources={}
        self._status_zh='先选择游戏压缩包或游戏文件夹。'
        self.source=tk.StringVar();self.output=tk.StringVar(value=str(Path.home()/'Desktop'))
        self.status=tk.StringVar(value=self.tr(self._status_zh))
        self.candidates=[];self.serial=0;self.last_result=None;self.last_output=None
        self._log_disk_text=''
        self._log_extra_text=''
        self._log_extra_report=None
        self._last_log_display=''
        window.title('CPyMO NDS 转换助手');window.geometry(f"840x{min(890,max(700,window.winfo_screenheight()-110))}");window.minsize(760,670)
        ui_font='PingFang SC' if sys.platform=='darwin' else 'Microsoft YaHei UI'
        window.option_add('*Font',(ui_font,10))
        style=ttk.Style();style.theme_use('clam');style.configure('TButton',padding=(12,7));style.configure('Title.TLabel',font=(ui_font,19,'bold'))
        frame=ttk.Frame(window,padding=22);frame.pack(fill='both',expand=True)
        head=ttk.Frame(frame);head.pack(fill='x')
        ttk.Label(head,text='CPyMO NDS 转换助手',style='Title.TLabel').pack(side='left')
        self.language_box=ttk.Combobox(head,textvariable=self.language,values=LANGUAGES,state='readonly',width=12)
        self.language_box.pack(side='right')
        ttk.Label(head,text='界面语言：').pack(side='right',padx=(0,8))
        self.language_box.bind('<<ComboboxSelected>>',self.change_language)
        ttk.Label(frame,text='选择自己的 PyMO 游戏，转换好后复制到 DS / DSi 的存储卡即可。',wraplength=780).pack(anchor='w',pady=(6,16))
        ttk.Label(frame,text='1  选择游戏',font=(ui_font,11,'bold')).pack(anchor='w')
        row=ttk.Frame(frame);row.pack(fill='x',pady=(7,8))
        ttk.Entry(row,textvariable=self.source,state='readonly').pack(side='left',fill='x',expand=True)
        self.zip_button=ttk.Button(row,text='选压缩包',command=self.pick_zip);self.zip_button.pack(side='left',padx=(8,4))
        self.folder_button=ttk.Button(row,text='选文件夹',command=self.pick_folder);self.folder_button.pack(side='left')
        game_row=ttk.Frame(frame);game_row.pack(fill='x')
        self.games=tk.Listbox(game_row,height=3,exportselection=False,activestyle='none',background='#ffffff',foreground='#233247',selectbackground='#27659c',selectforeground='#ffffff',relief='solid',borderwidth=1)
        self.games.pack(side='left',fill='x',expand=True)
        game_scroll=ttk.Scrollbar(game_row,orient='vertical',command=self.games.yview)
        game_scroll.pack(side='right',fill='y');self.games.configure(yscrollcommand=game_scroll.set)
        ttk.Label(frame,text='自动识别游戏版本；如果找到多个版本，会优先推荐适合 NDS 的版本。',foreground='#526579',wraplength=780).pack(anchor='w',pady=(5,12))
        ttk.Label(frame,text='2  选择保存位置',font=(ui_font,11,'bold')).pack(anchor='w')
        row=ttk.Frame(frame);row.pack(fill='x',pady=(7,6));ttk.Entry(row,textvariable=self.output,state='readonly').pack(side='left',fill='x',expand=True)
        self.output_button=ttk.Button(row,text='选保存位置',command=self.pick_output);self.output_button.pack(side='left',padx=(8,0))
        ttk.Label(frame,text='转换后保存在新文件夹，不会修改原游戏或覆盖已有存档。',foreground='#526579',wraplength=780).pack(anchor='w',pady=(0,10))
        # Media de-duplication is a safe default, not a player decision.
        # Keep the variable for existing CLI and GUI self-test fixtures.
        self.cache_only=tk.BooleanVar(value=True)
        self.video_modes=['不转换视频（兼容旧流程）','转换视频 NVL1（推荐：兼容优先）','转换视频 NVA1（压缩音频）']
        self.video=tk.StringVar(value=self.video_modes[1])
        row=ttk.Frame(frame);row.pack(fill='x',pady=(7,0));ttk.Label(row,text='游戏视频：').pack(side='left')
        self.video_box=ttk.Combobox(row,textvariable=self.video,values=self.video_modes,
                     state='readonly',width=47)
        self.video_box.pack(side='left')
        ttk.Label(frame,text='默认保留并转换游戏视频；如不需要视频，也可以选择跳过。',
                  foreground='#526579',wraplength=780).pack(anchor='w',pady=(3,0))
        self.compression=tk.StringVar(value='无损压缩语音与适合的图片（推荐）')
        self.compression_modes=['不压缩缓存（读取优先）','无损压缩语音与适合的图片（推荐）','无损压缩全部音频与适合的图片（实验）']
        self.advanced_visible=False
        self.advanced_button=ttk.Button(frame,text='更多设置（可选）',command=self.toggle_advanced)
        self.advanced_button.pack(anchor='w',pady=(8,0))
        self.advanced_frame=ttk.Frame(frame)
        ttk.Label(self.advanced_frame,text='缺字自动以 □ 显示并记录警告，不中断转换；原脚本保持不变。',foreground='#526579',wraplength=780).pack(anchor='w',pady=(4,3))
        row=ttk.Frame(self.advanced_frame);row.pack(fill='x',pady=(5,0))
        ttk.Label(row,text='文件体积与读取：').pack(side='left')
        self.compression_box=ttk.Combobox(row,textvariable=self.compression,values=self.compression_modes,
                        state='readonly',width=64)
        self.compression_box.pack(side='left',fill='x',expand=True,padx=(9,0))
        ttk.Label(self.advanced_frame,text='默认设置兼顾体积和读取速度，普通使用无需调整。',
                  foreground='#526579',wraplength=780).pack(anchor='w',pady=(3,0))
        row=ttk.Frame(frame);row.pack(fill='x',pady=(13,8))
        self.convert_button=ttk.Button(row,text='3  开始转换',command=self.begin,state='disabled');self.convert_button.pack(side='left')
        self.cancel_button=ttk.Button(row,text='取消',command=self.cancel,state='disabled');self.cancel_button.pack(side='left',padx=8)
        self.open_button=ttk.Button(row,text='打开结果文件夹',command=self.open_output,state='disabled');self.open_button.pack(side='right')
        self.progress=ttk.Progressbar(frame,mode='indeterminate');self.progress.pack(fill='x')
        ttk.Label(frame,textvariable=self.status,wraplength=790).pack(anchor='w',pady=(8,4))
        # Never inherit a white macOS dark-mode foreground over a pale log panel.
        # Give Tk's CJK fallback extra line height on Retina screens and avoid
        # repainting the entire Text control every polling interval.
        log_font=tkfont.Font(window,family='Menlo' if sys.platform=='darwin' else ui_font,size=10)
        self.log=tk.Text(frame,height=4,wrap='word',background='#f1f4f8',foreground='#233247',insertbackground='#233247',selectbackground='#27659c',selectforeground='#ffffff',relief='solid',borderwidth=1,font=log_font,spacing1=3,spacing2=4,spacing3=5,padx=10,pady=8)
        self.log.pack(fill='both',expand=True,pady=(4,8))
        self.log.configure(state='disabled')
        self.version_label=ttk.Label(frame,text='NDS 平台移植：千岚之夏  ·  基于 CPyMO / PyMO  ·  转换助手 '+VERSION,foreground='#526579')
        self.version_label.pack(anchor='w')
        window.protocol('WM_DELETE_WINDOW',self.close)
        self._capture_localizable_widgets(self.window)
        self.change_language()
        if self_test:
            if 'cache_only' in self_test:self.cache_only.set(self_test['cache_only'])
            if 'audio_lossless' in self_test:self.compression.set(self.compression_modes[{'off':0,'voice':1,'all':2}[self_test['audio_lossless']]])
            if 'video_mode' in self_test:self.video.set(self.video_modes[{'skip':0,'nvl1':1,'nva1':2}[self_test['video_mode']]])
            self.source.set(self_test['source']);self.output.set(self_test['output_base']);window.after(300,self.inspect)

    def tr(self,text):
        return tr(text,self.language.get())

    def set_status(self,raw):
        self._status_zh=raw
        suffix='。点击“打开结果文件夹”查看安装必读。'
        if raw.endswith(suffix):
            translated=self.tr(raw[:-len(suffix)])+self.tr(suffix)
        elif raw.startswith('未完成：'):
            translated=self.tr('未完成：')+self.tr(raw[len('未完成：'):])
        else:
            translated=self.tr(raw)
        self.status.set(translated)

    def _capture_localizable_widgets(self,parent):
        for widget in parent.winfo_children():
            if 'text' in widget.keys():
                original=widget.cget('text')
                if original:self._ui_sources[widget]=original
            self._capture_localizable_widgets(widget)

    def _apply_language(self):
        self.window.title(self.tr('CPyMO NDS 转换助手'))
        for widget,source in self._ui_sources.items():
            widget.configure(text=self.tr(source))
        self.set_status(self._status_zh)

    def change_language(self,_event=None):
        audio_idx=self.compression_modes.index(self.compression.get())
        video_idx=self.video_modes.index(self.video.get())
        raw_compression=['不压缩缓存（读取优先）',
            '无损压缩语音与适合的图片（推荐）',
            '无损压缩全部音频与适合的图片（实验）']
        raw_video=['不转换视频（兼容旧流程）',
            '转换视频 NVL1（推荐：兼容优先）',
            '转换视频 NVA1（压缩音频）']
        self.compression_modes=[self.tr(x) for x in raw_compression]
        self.video_modes=[self.tr(x) for x in raw_video]
        self.compression_box.configure(values=self.compression_modes)
        self.video_box.configure(values=self.video_modes)
        self.compression.set(self.compression_modes[audio_idx])
        self.video.set(self.video_modes[video_idx])
        self._apply_language()
        self._update_advanced_caption()
        self._render_log(force=True)
        if self.last_output and self.last_result and not self.last_result.get('ok'):
            self.open_button.configure(text=self.tr('查看错误日志'))
        if self.candidates:
            selected=self.games.curselection()
            self.games.delete(0,'end')
            for i,c in enumerate(self.candidates):
                prefix=self.tr('推荐  ') if i==0 else ''
                self.games.insert('end',f"{prefix}[{c['platform']}]  {c['title']}")
            if selected:self.games.selection_set(selected[0])
        try:
            self.preference_file.parent.mkdir(parents=True,exist_ok=True)
            self.preference_file.write_text(json.dumps({'language':self.language.get()},ensure_ascii=False),encoding='utf8')
        except OSError:
            pass  # Keep a usable UI if preferences are read-only.

    def _update_advanced_caption(self):
        label='收起更多设置' if self.advanced_visible else '更多设置（可选）'
        self.advanced_button.configure(text=self.tr(label))

    def toggle_advanced(self):
        self.advanced_visible=not self.advanced_visible
        if self.advanced_visible:
            self.advanced_frame.pack(fill='x',after=self.advanced_button)
        else:
            self.advanced_frame.pack_forget()
        self._update_advanced_caption()

    def pick_zip(self):
        p=filedialog.askopenfilename(title=self.tr('选择 PyMO 游戏包'),filetypes=[(self.tr('游戏包'),'*.zip'),(self.tr('全部文件'),'*.*')])
        if p:self.source.set(p);self.inspect()

    def pick_folder(self):
        p=filedialog.askdirectory(title=self.tr('选择含 gameconfig.txt 的游戏数据目录'))
        if p:self.source.set(p);self.inspect()

    def pick_output(self):
        p=filedialog.askdirectory(title=self.tr('选择保存位置'))
        if p:self.output.set(p)

    def busy(self,value):
        for button in (self.zip_button,self.folder_button,self.output_button):button.configure(state='disabled' if value else 'normal')
        self.cancel_button.configure(state='normal' if value else 'disabled')
        self.convert_button.configure(state='normal' if not value and self.candidates else 'disabled')
        self.games.configure(state='disabled' if value else 'normal')
        for widget in (self.video_box,self.compression_box,self.language_box):
            widget.configure(state='disabled' if value else 'readonly')
        self.advanced_button.configure(state='disabled' if value else 'normal')
        if value:self.progress.start(12)
        else:self.progress.stop()

    def start(self,job,callback):
        self._log_disk_text=''
        self._log_extra_text=''
        self._log_extra_report=None
        self._render_log(force=True)
        self.serial+=1;stamp=datetime.datetime.now().strftime('%Y%m%d-%H%M%S-%f');folder=self.state_root/(stamp+'-'+str(self.serial));folder.mkdir(parents=True)
        self.job_file=folder/'job.json';self.result_file=folder/'result.json';self.log_file=folder/'log.txt'
        job.update(result=str(self.result_file.resolve()),log=str(self.log_file.resolve()));self.job_file.write_text(json.dumps(job,ensure_ascii=False,indent=2),encoding='utf8')
        cmd=[sys.executable]+([] if getattr(sys,'frozen',False) else [str(Path(__file__).resolve())])+['--job',str(self.job_file.resolve())]
        self.process=subprocess.Popen(cmd,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,creationflags=0x08000000 if os.name=='nt' else 0)
        self.callback=callback;self.busy(True);self.window.after(200,self.poll)

    def _format_conversion_notes(self,report):
        notes=['\n'+('\n'.join(self.tr(w) for w in report['warnings'])
                       if report['warnings'] else self.tr('未发现静态兼容性警告。'))+'\n']
        if report.get('cache_only'):
            notes.append(self.tr('已省去重复媒体 {size:.2f} MiB。脚本与署名保留，原包未修改。').format(size=report['redundant_media_bytes']/1048576)+'\n')
        comp=report.get('lossless',{})
        if comp.get('audio_mode','off')!='off':
            notes.append(self.tr('无损缓存另节省 {size:.2f} MiB。请使用随附的 {version} 引擎。').format(size=(comp['audio_saved_bytes']+comp['image_saved_bytes'])/1048576,version=VERSION)+'\n')
        return ''.join(notes)

    def _render_log(self,force=False):
        extra=(self._format_conversion_notes(self._log_extra_report)
               if self._log_extra_report is not None else self._log_extra_text)
        displayed=(tr_log(self._log_disk_text,self.language.get())+extra)[-12000:]
        if not force and displayed==self._last_log_display:
            return
        self.log.configure(state='normal')
        # Append only when the new content really extends the old content;
        # wholesale deletion every 300 ms caused macOS Tk repaint artifacts.
        if not force and displayed.startswith(self._last_log_display):
            self.log.insert('end',displayed[len(self._last_log_display):])
        else:
            self.log.delete('1.0','end')
            self.log.insert('end',displayed)
        self._last_log_display=displayed
        self.log.see('end')
        self.log.configure(state='disabled')

    def poll(self):
        if not self.process:return
        if self.log_file.exists():
            text=self.log_file.read_text(encoding='utf8',errors='replace')
            if text!=self._log_disk_text:
                self._log_disk_text=text
                self._render_log()
        if self.process.poll() is None:self.window.after(300,self.poll);return
        self.process=None;self.busy(False)
        report=json.loads(self.result_file.read_text(encoding='utf8')) if self.result_file.exists() else {'ok':False,'error':'转换进程意外退出，请保留诊断日志。'}
        self.last_result=report;self.callback(report)

    def inspect(self):
        self.candidates=[];self.games.delete(0,'end');self.set_status('正在识别游戏包里的平台版本……')
        self.start({'action':'inspect','source':self.source.get()},self.inspected)

    def inspected(self,report):
        if not report['ok']:self.failed(report);return
        self.candidates=report['candidates']
        for i,c in enumerate(self.candidates):self.games.insert('end',f"{self.tr('推荐  ') if i==0 else ''}[{c['platform']}]  {c['title']}")
        selected=self.self_test.get('candidate',0) if self.self_test else 0
        self.games.selection_set(selected);self.convert_button.configure(state='normal');self.set_status('已识别游戏。确认保存位置后，点击“开始转换”。')
        if self.self_test and not self.test_started:self.test_started=True;self.window.after(200,self.begin)

    def begin(self):
        selected=self.games.curselection()
        if not selected:return
        source=Path(self.source.get()).resolve();base=Path(self.output.get()).resolve()
        if source.is_dir() and base.is_relative_to(source):self.failed({'ok':False,'error':self.tr('保存位置不能位于原游戏目录内部。')});return
        name='CPyMO_NDS_转换结果_'+datetime.datetime.now().strftime('%Y%m%d_%H%M%S_%f');output=base/name
        self.set_status('正在转换，请稍候。语音较多的游戏需要更长时间……');self.open_button.configure(state='disabled',text=self.tr('打开结果文件夹'))
        audio_mode=['off','voice','all'][self.compression_modes.index(self.compression.get())]
        self.start({'action':'convert','source':str(source),'candidate':int(selected[0]),'output':str(output),'missing_policy':'box','cache_only':self.cache_only.get(),'audio_lossless':audio_mode,'video_mode':['skip','nvl1','nva1'][self.video_modes.index(self.video.get())],'ui_language':self.language.get()},self.converted)

    def converted(self,report):
        if not report['ok']:self.failed(report);return
        self.last_output=Path(report['output']);self.open_button.configure(state='normal',text=self.tr('打开结果文件夹'))
        self.set_status('转换完成！有一些提示，建议先查看下方记录。' if report.get('warnings') else '转换完成！请打开结果文件夹，按说明复制到 TF 卡。')
        # Keep the original structured result: the displayed summary must
        # follow UI language changes, without rewriting diagnostic files.
        self._log_extra_report=report
        self._render_log()
        if self.self_test:self.finish_test(report)

    def failed(self,report):
        if self.result_file.parent.is_dir():
            self.last_output=self.result_file.parent
            self.open_button.configure(state='normal',text=self.tr('查看错误日志'))
        self.set_status('未完成：'+report['error'])
        if self.self_test:self.finish_test(report)
        else:messagebox.showerror(self.tr('暂时不能完成转换'),self.tr(report['error'])+'\n\n'+self.tr('原游戏包未改动。诊断目录：')+str(self.result_file.parent))

    def finish_test(self,report):
        def capture():
            result=dict(report)
            try:
                self.window.lift();self.window.update()
                result['ui_title']=self.window.title()
                result['ui_version_footer']=str(self.version_label.cget('text'))
                from PIL import ImageGrab
                # Screenshot capture is ancillary on macOS: permissions and
                # unsupported window capture APIs must not discard the actual
                # conversion result or leave the frozen test GUI running.
                ImageGrab.grab(window=self.window.winfo_id()).save(self.self_test['screenshot'])
                result['ui_screenshot']='captured'
            except Exception as exc:
                result['ui_screenshot']='unavailable'
                result['ui_screenshot_error']=str(exc)
            finally:
                Path(self.self_test['result']).write_text(
                    json.dumps(result,ensure_ascii=False,indent=2),encoding='utf8')
                self.window.after(250,self.close)
        self.window.after(250,capture)

    def cancel(self):
        if self.process and self.process.poll() is None:
            if os.name=='nt':subprocess.run(['taskkill','/PID',str(self.process.pid),'/T','/F'],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,creationflags=0x08000000)
            else:self.process.terminate()
        self.process=None;self.busy(False);self.set_status('已取消。原游戏包未改动；未完成目录和日志保留供检查。')
        if getattr(self,'log_file',None) and self.log_file.parent.is_dir():
            self.last_output=self.log_file.parent
            self.open_button.configure(state='normal',text=self.tr('查看错误日志'))

    def open_output(self):
        if self.last_output:
            if sys.platform=='darwin':subprocess.Popen(['open',str(self.last_output)])
            elif os.name=='nt':os.startfile(self.last_output)
            else:subprocess.Popen(['xdg-open',str(self.last_output)])

    def close(self):
        if self.process:self.cancel()
        self.window.destroy()

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--job',type=Path);parser.add_argument('--state-root',type=Path);parser.add_argument('--self-test',type=Path);args=parser.parse_args()
    if args.job:
        job=json.loads(args.job.read_text(encoding='utf8'))
        with Path(job['log']).open('w',encoding='utf8',buffering=1) as log,contextlib.redirect_stdout(log),contextlib.redirect_stderr(log):report=run_job(job)
        return 0 if report['ok'] else 1
    test=json.loads(args.self_test.read_text(encoding='utf8')) if args.self_test else None

    if os.name=='nt':
        import ctypes
        ctypes.windll.user32.SetProcessDPIAware()
    window=tk.Tk();App(window,args.state_root,test);window.mainloop();return 0

if __name__=='__main__':sys.exit(main())
