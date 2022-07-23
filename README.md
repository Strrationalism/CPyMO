
```
   __________        __  _______
  / ____/ __ \__  __/  |/  / __ \
 / /   / /_/ / / / / /|_/ / / / /
/ /___/ ____/ /_/ / /  / / /_/ /
\____/_/    \__, /_/  /_/\____/
           /____/                                                  
```

![BANNER](https://github.com/Strrationalism/CPyMO/raw/main/cpymo-backends/3ds/banner.png)

![LICENSE](https://www.gnu.org/graphics/agplv3-88x31.png)

此项目仅用于您运行**合法持有**的游戏软件副本，持有盗版副本可能会让您面临法律问题。    
这是一个使用C实现的pymo引擎的复刻，以方便在各种平台上制作并运行pymo游戏。      

如果你需要使用CPyMO制作游戏，可以参见[CPyMO + YukimiScript开发模板](https://github.com/Strrationalism/CPyMO-YukimiScript-Template)。

pymo原版参见：https://github.com/pymo/pymo    

# 平台支持列表

### 第一梯队
**CPyMO主要在这些平台上开发，并将主要在这些平台上进行测试和除错。**

平台          | 后端 | 视频播放器 | 音频支持            | 字体支持          | 额外功能
------------ | ---- | -------- | ------------------ | ---------------- | -------
Windows      | SDL2 | FFmpeg   | FFmpeg, SDL2_mixer | 加载系统字体       | 视障帮助
Nintendo 3DS | 3DS  | FFmpeg   | FFmpeg             | 自带字体          | 游戏选择器

### 第二梯队
**CPyMO可以编译到这些平台，保证全部功能可用，但不保证用户体验，也没有进行全面的测试。**

平台            | 后端  | 视频播放器 | 音频支持            | 字体支持     | 额外功能
--------------- | ---- | -------- | ------------------ | ----------- | ---------------
Linux           | SDL2 | FFmpeg   | FFmpeg, SDL2_mixer | 外置字体  | 视障帮助
macOS           | SDL2 | FFmpeg   | FFmpeg, SDL2_mixer | 加载系统字体  | 视障帮助
Nintendo Switch | SDL2 | FFmpeg   | FFmpeg             | 加载系统字体  | 游戏选择器
UWP             | SDL2 | FFmpeg   | FFmpeg             | 加载系统字体  | 游戏选择器
Emscripten      | SDL2 | FFmpeg   | FFmpeg, SDL2_mixer | 外置字体     | 
Android         | SDL2 | FFmpeg   | FFmpeg             | 外置字体     | 游戏选择器,视障帮助

### 第三梯队
**CPyMO可以编译到这些平台，但可能有部分次要功能不可用。**

平台              | 后端       | 视频播放器 | 音频支持                      | 字体支持      | 额外功能
---------------- | --------- | --------- | ---------------------------- | ---------- | ----------
Sony PSP         | SDL2      | 不支持     | SDL2_mixer(仅支持BGM通道)      | 外置字体     | 游戏选择器
Sony PSV         | SDL2      | FFmpeg    | FFmpeg                       | 外置字体     | 游戏选择器
Nintendo Wii     | SDL 1.2   | 不支持     | SDL_mixer                    | 外置字体     | 游戏选择器
Windows (SDL 1.2)| SDL 1.2   | FFmpeg    | FFmpeg, SDL_mixer            | 加载系统字体  | 
Linux (SDL 1.2)  | SDL 1.2   | FFmpeg    | FFmpeg, SDL_mixer            | 外置字体     | 
POSIX (ASCII ART)| ASCII ART | 不支持     | 不支持                        | 外置字体     | 在控制台上操作并输出画面
POSIX (Text)     | Text      | 不支持     | 不支持                        | 无需        | 在控制台上操作并输出游戏文本

**注：SDL2_mixer音频后端仅支持在BGM通道播放MP3文件，其余通道不支持。**   
**注：SDL 1.2后端不支持对Alpha图像再次设置透明度。**   
**注：SDL_mixer音频后端音频支持格式有限。**   

# 与pymo行为差异

* CPyMO的存档位置和存档格式与pymo不同，不能与之互换。
* CPyMO最小音频支持仅包含OGG格式，且CPyMO不会考虑MID格式的音频播放。
* CPyMO的蒙版渐变动画效果与PyMO不同。
* 涉及动画时间曲线的部分CPyMO均做了缓动效果，而pymo则使用线性效果。
* CPyMO的菜单和UI与PyMO不同。
* CPyMO在可以播放视频的情况下比pymo支持更多编码格式的视频。
* CPyMO不支持预载，预载选项会被CPyMO直接忽略。
* CPyMO忽略除字体大小以外所有的字体设置，将自动启用字体阴影和字体反锯齿。

# 由SDL2支持的桌面平台 (Windows、Linux与macOS)

## 额外依赖

你需要使用vcpkg包管理器安装以下依赖：

* SDL2
* ffmpeg

如果你使用Microsoft Visual Studio，默认的CMakeSettings.json中指示的依赖版本为x64-windows-static。

如果你需要在macOS上运行，那么你需要首先安装libxcb:

```bash
brew install libxcb
```

## 全屏

按下Alt + Enter键可在全屏/窗口模式中切换。

## 在Windows下使用nmake进行构建

首先你需要确保：

* 你已经构建了当前平台的SDL2库，并将二进制目录（含有include和lib文件夹）设置为环境变量%SDL2%（或通过nmake的-a参数传入）。
* 你已经构建了当前平台的FFmpeg库，并将二进制目录（含有include和lib文件夹）设置为环境变量%FFMPEG%（或通过nmake的-a参数传入）。
* 已经安装了某一版本的Windows SDK和MSVC编译器工具链。

关于音频支持，将会按照以下路径进行检查：
* 将会首先检查环境变量FFMPEG是否存在，它需要指向FFMPEG二进制库目录，如果存在则会编译生成依赖于FFMPEG并带有播放视频功能的CPyMO。
* 如果FFMPEG环境变量不存在，则会检查环境变量SDL2_MIXER是否存在，它需要指向SDL2_MIXER的二进制库目录，如果存在则编译生成依赖于SDL2_MIXER的CPyMO。
* 如果两个环境变量均不存在则编译生成不能播放音频和视频文件的CPyMO。

除此之外，你还可以使用以下编译开关：

* 若`NO_CONSOLE`环境变量存在或通过-a传入并设置为1，则禁用命令行窗口，使得CPyMO仅创建一个游戏窗口。
* 若`RC_FILE`环境变量存在或通过-a传入，则允许传入用户指定的资源文件（主要用于修改图标）。
* 若`TARGET`环境变量存在或通过-a传入，则允许用户通过TARGET指定输出的可执行文件名称。

之后启动Visual Studio开发人员命令提示符，使用cd命令进入`cpymo-backends/sdl2`目录，执行`nmake -f Makefile.Win32`即可构建CPyMO。

## 使用GNU Make进行构建

1. 你需要安装SDL2，可以选择直接安装`libsdl2-dev`，或者将SDL2的预编译二进制库存入环境变量`SDL2`中。
2. 如果你需要使用FFmpeg播放音频和视频，可以选择安装以下库，或者将其预编译的二进制库存入环境变量`FFmpeg`中：
    * libavformat-dev
    * libavutil-dev
    * libavcodec-dev
    * libswscale-dev
    * libswresample-dev
3. 如果你需要使用SDL2_mixer播放音频，则可以选择安装`libsdl2-mixer-dev`，或者将SDL2_mixer的预编译二进制库存入环境变量`SDL2_mixer`中，并定义环境变量`export USE_SDL2_MIXER=1`.
4. 如果你不想播放音视频，可直接定义`export DISABLE_AUDIO=1`。
5. 通过定义`TARGET`环境变量来指定输出文件名的名称。
6. 通过定义`DEBUG=1`环境变量来启动调试信息，之后便可使用gdb进行调试。
7. 执行`make -j`即可产生可执行文件。

在Windows下，你也许需要拷贝以下dll文件到可执行文件目录才可以在非MSYS2环境下运行CPyMO并启动调试：

- avcodec-58.dll
- avformat-58.dll
- avutil-56.dll
- libwinpthread-1.dll
- SDL2.dll
- swresample-3.dll


# Nintendo 3DS 平台

[在Universal Updater应用商店中查看](https://db.universal-team.net/3ds/cpymo)

## 额外依赖

* devkitPro + 3ds dev
* FFmpeg 5.0

### 编译3DS用的FFmpeg依赖

在终端或devkitPro的MSYS2控制台（如果你使用Windows的话），cd到`cpymo-backends/3ds/`目录下，执行`./build-3ds-ffmpeg.sh`，这将为3DS编译FFmpeg。

## 产生3dsx与cia文件
于`./cpymo-backends/3ds/`目录下执行`make`即可生成3dsx程序。    

如果你需要创建cia文件，首先需要确保已经安装了`makerom`命令，之后在`./cpymo-backends/3ds/`下执行`make cia`即可。    

你可以在 https://github.com/3DSGuy/Project_CTR 找到makerom的可执行文件。

## 启动
你需要将你的游戏放置于SD卡的`/pymogames/`下，保证路径中只有半角英文、数字和下划线，之后该游戏便会被CPyMO for 3DS检测到。   
如果你已经安装了Citra且citra命令可用，你可以直接使用`make run`来调用Citra模拟器来启动CPyMO。    

CPyMO for 3DS支持3D显示，可使用3D滑块来打开3D显示功能。    
按下START键可以快速退出CPyMO。       
按下SELECT键在四种屏幕模式之间切换。
ZL和ZR键功能和A、Y键相同，用于单手操作。    

### 以调试模式启动
如果你需要查看CPyMO控制台，你需要在游戏列表中**按住L键**，同时按下A键选择游戏，即可激活调试模式。    
在这种模式下，下屏会显示CPyMO控制台，Start键将不再可用，对话文本会被强制显示在上屏。    

### 如果无法启动CIA版本的话？

1. 目前仅在New 3DS日版（系统版本号Ver 11.15.0-47J）上对CIA版本进行过测试。
2. 如果你的机器在运行CIA版本的CPyMO时崩溃，请尝试切换到3dsx版。

### 如何启动3DSX版本？

1. 将cpymo.3dsx放入SD卡的3ds目录下。
2. 启动Homebrew Launcher，建议使用这里的Homebrew Launcher Dummy（https://github.com/PabloMK7/homebrew_launcher_dummy ）。
3. 执行cpymo.3dsx。

### 在3DS中没有声音？

你需要确保已经Dump了3DS的DSP固件。    
如果你没有Dump，那么你需要先安装DSP1（https://github.com/zoogie/DSP1/releases/tag/v1.0 ），并使用它Dump你的3DS的DSP固件。


## 为3DS适配游戏

如果你需要为3DS适配PyMO游戏，那么建议你使用以下参数：

* 分辨率：400×240
* 音频格式：ogg，16bit signed little-endian，32000Hz
* 背景格式：jpg
* 其他图片：带透明通道的png，不要使用额外的mask灰阶图片
* platform参数：pygame
* 默认字体大小：28
* 视频格式：H264 MP4，低于30FPS，Old 3DS请不要使用视频

## 关于字体

3DS版本的CPyMO不会加载游戏中自带的字体或者其他TTF字体，而是使用[思源黑体](https://github.com/adobe-fonts/source-han-sans)。    
思源黑体已经被转换为可以被3DS直接识别的bcfnt格式，CPyMO for 3DS中的思源黑体将会按照其原本的[SIL协议](https://github.com/adobe-fonts/source-han-sans/blob/master/LICENSE.txt)随CPyMO for 3DS一起分发。    

如果自带的字体不能满足你的需求，那么你可以将bcfnt格式的字体放入SD卡中的`/pymogames/font.bcfnt`路径处，CPyMO将会优先加载这个字体。

# Nintendo Switch 平台

## 额外依赖

* devkitPro + switch dev
* switch-sdl2
* switch-ffmpeg

直接使用devkitPro pacman安装即可。

## 编译

cd到`cpymo-backends/sdl2`，执行`make -j -f Makefile.Switch`即可编译到任天堂Switch平台。    
使用`make run -j -f Makefile.Switch`即可使用yuzu模拟器运行。  

## 启动

将游戏放入SD卡的`/pymogames/`目录即可。

如果产生乱码，请自行在`/pymogames/`目录下放置`default.ttf`。

## 自定义字体

将字体文件改名为`default.ttf`放入`/pymogames/`目录即可加载该字体文件。

默认将会加载Switch自带的字体。

## 为Switch适配游戏

如果你需要为Switch适配游戏，那么建议你使用以下参数：

* 分辨率：1920×1080
* 音频格式：ogg，32bit signed float little-endian，48000Hz
* 图片格式：带透明通道的png，不要使用mask灰阶图片
* platform参数：pygame
* 视频格式：H264 MP4

# Sony Playstation Portable 平台

PSP平台的CPyMO仅能运行s60v3数据包或下述“适配”的包。

## 额外依赖

* [pspdev/pspdev](github.com/pspdev/pspdev)
* psp-sdl2（已经包括到pspdev中）
* psp-sdl2_mixer（已经包括到pspdev中）
* psp-libvorbis（已经包括到pspdev中）
* psp-libogg（已经包括到pspdev中）

## 编译

cd到`cpymo-backends/sdl2`，执行`make -f Makefile.PSP`即可编译到索尼PSP平台。    

## 启动

将EBOOT.PBP、游戏文件夹和default.ttf放在一起。    
你需要自己准备可以使用的default.ttf，CPyMO将会加载这个文件作为字体使用。    

在使用PPSSPP模拟器的情况下，你需要关闭“系统设置 - 快速内存访问”，并开启“忽略读写错误”。    

## 缺陷

这些缺陷将不会得到修复：

* 由于PSP生态中没有移植的FFmpeg库
    - 视频播放器功能已经禁用
	- 音频播放使用SDL2_mixer后端
        - 音频不支持mp3格式
* 由于PSP机能有限
    - 仅能加载s60v3数据包或下述推荐的PSP数据包
	- 在放入太多游戏时将无法加载游戏列表
	- 加载音效和语音可能会导致内存占用过高，故禁用音效和语音
	- 将不会在启动游戏时加载游戏目录下的`system/default.ttf`字体
* 由于SDL2 for PSP存在问题
    - 游戏将会在屏幕左上角显示，而不是居中显示
* PPSSPP的“快速内存访问”可能存在问题
    - 目前仅可在PPSSPP在关闭“快速内存访问”时才可以正常启动
* 由于本人不会编写makefile :(
	- Makefile.PSP将会把.o文件弄得到处都是

## 为PSP适配游戏

如果你需要为PSP适配游戏，那么建议你使用以下参数：

* 分辨率：480×272
* 背景、立绘图像：jpg，立绘图像应当带jpg格式的透明通道mask图
* 系统图像：png，带mask图
* 声音格式：ogg, 16bit signed little-endian, 44100Hz
* platform参数：s60v3
* 不支持的内容：
    * 视频
	* 音效（se）

# Sony Playstation Vita 平台

## 额外依赖

1. 你需要安装[vdpm](https://github.com/vitasdk/vdpm)，并使用其安装vitasdk。
2. 通过命令行`vdpm sdl2`安装SDL2。
3. cd到`cpymo-backends/sdl2`下执行`./build-psv-ffmpeg.sh`编译FFmpeg，注意这个版本和vdpm中的ffmpeg使用的剪裁参数不同，不要使用vdpm中的ffmpeg。

## 编译

在`cpymo-backends/sdl2`下执行`make -f Makefile.PSV`即可得到CPyMO.vpk。

## 安装

安装CPyMO.vpk之后，你还需要在`ux0:/pymogames`目录下放置`default.ttf`和游戏文件。    
		
## 为PSV适配游戏

如果你需要为PSV适配游戏，那么建议你使用以下参数：

* 分辨率：960×544
* 图像：带透明通道的png，不要使用mask灰阶图片
* 声音：ogg，16bit signed，little endian，44100Hz

# Emscripten 平台

你可以使用Emscripten将CPyMO编译到Web Assembly或者JavsScript。

[在这里你可以找到部署在Github Pages上的Demo](https://strrationalism.github.io/CPyMO-Web-Demo/)。

## 编译

在`cpymo-backends/sdl2`下修改`Makefile.Emscripten`.

变量`WASM`设置为1时编译到 Web Assembly 二进制文件，为0时编译到 JavaScript。    
变量`BUILD_GAME_DIR`指定要集成的游戏目录，留空则不集成游戏。    
以上两个变量可通过环境传入。    

默认编译出的结果将不支持视频播放，并且采用SDL2_mixer作为音频后端（只能在BGM通道播放mp3，其他通道不支持mp3），默认采用这种方式是出于编译结果大小的考虑。    
* 如果你需要播放MP3格式的语音或音效，或者需要播放视频，那么你需要在编译前执行：
    1. 在`cpymo-backends/sdl2`目录下执行`build-emscripten-ffmpeg.sh`。
    2. 执行`export AUDIO_BACKEND=ffmpeg`启动FFmpeg音视频后端。    
* 如果你完全不需要音视频功能，可以执行`export AUDIO_BACKEND=none`关闭音视频功能以降低编译结果大小。    

游戏目录中必须存在`/system/default.ttf`作为游戏字体使用。    

之后使用`make -j`即可构建。

## 启动

仅当集成了游戏时，可通过`make run -j`来使用emrun启动CPyMO。

# Android 平台

仅支持 Android 4.3 及以上的系统，如果你需要在更低版本的Android上运行，则可以使用原版PyMO，而不是使用CPyMO。

## 视障帮助

GitHub Action和Release中的“CPyMO for Android (Accessibility)”版本即为带有视障帮助的版本。

视障帮助功能会调用系统TTS进行朗读，并加入了一些手势操作：

+ 单指轻点 ==> 移动鼠标位置但不点击
+ 单指按住并移动 ==> 持续移动鼠标位置但不点击
+ 单指双击 ==> 回车键
+ 单指长按 ==> ESC键
+ 单指 **上/下/左/右** 滑动 ==> **上/下/左/右** 方向键
+ 双指双击 ==> CTRL键
+ 双指双击并按住 ==> 长按CTRL键
+ 双指向左滑动 ==> 将上一次TTS朗读的内容复制到剪贴板
+ 双指向右滑动 ==> 将上一次TTS朗读的内容追加到剪贴板
+ 双指向下滑动 ==> ESC键

## 编译

1. cd到`cpymo-backends/android`后执行`./build-android-ffmpeg.sh`以构建FFmpeg。
2. 如果需要禁用视障帮助功能，请修改`gradle.properties`中的`NON_VISUALLY_IMPAIRED_HELP`属性为`true`.
3. 使用Android Studio打开`cpymo-backends/android`即可构建APK。

## 启动

1. 安装CPyMO，并在“设置 - 应用 - CPyMO”中允许其一切权限。
1. 在绝对路径`/sdcard/pymogames/`或`/storage/emulated/0/pymogames/`中放置`default.ttf`和游戏文件夹。
1. 启动CPyMO。


# Universal Windows Platform 平台

最低支持Windows 10 (10.0.10240.0)。

## 编译

前置条件：

* 已经安装vcpkg，并安装以下包：
    - ffmpeg:x86-uwp
    - ffmpeg:x64-uwp
    - ffmpeg:arm-uwp
* 已经安装Visual Studio，并安装以下组件：
    - 使用C++的通用Windows平台开发

使用Visual Studio 2022打开解决方案`cpymo-backends/sdl2/uwp/CPyMO.sln`即可编译。

## 启动

你的游戏需要放置在文件夹`%LOCALAPPDATA%\Packages\7062935b-c643-4df5-97d1-2744bf120181_*\LocalState`中。

其中\*的部分可能会不同，找到符合这个模式的目录即可。

之后从开始菜单启动CPyMO。


# Nintendo Wii 平台

## 依赖

* devkitPro + wii dev
* wii-sdl
* wii-sdl_mixer
* ppc-libogg
* ppc-libvorbis
* ppc-libmodplug
* ppc-libmad

直接使用devkitPro pacman安装即可。

## 编译

cd到`cpymo-backends/sdl1`，执行`make -j -f Makefile.Wii`即可生成dol文件。

## 启动

将游戏和`default.ttf`放入SD卡的`/pymogames/`目录即可。

注意，Wii平台仅s60v5数据包才可有较好体验。

如果你使用Dolphin模拟器，则需要将“图形” - “修正” - “纹理缓存”的“精确度”选项设置为“安全”，否则会导致画面刷新卡顿。


# 使用CPyMO开发新游戏

我们推荐你使用[YukimiScript](https://github.com/Strrationalism/YukimiScript)作为开发语言，当然也可以使用传统PyMO游戏的开发方式。    

[CPyMO-YukimiScript-Template](https://github.com/Strrationalism/CPyMO-YukimiScript-Template)是一套使用YukimiScript语言和Pipe构建系统的CPyMO项目模板，我们建议你基于它来开发游戏。    

除此之外，我们建议你使用以下格式：

* 分辨率：CPyMO没有分辨率限制，你可以根据自己的需要选择分辨率
* 音频：ogg，48000Hz，16bit signed
    - 注：CPyMO仅保证wav和ogg两种格式在所有平台上都可识别。
* 视频：mp4
    - 注：CPyMO不保证所有平台都可以播放视频。
* 图片：png

如果你需要生成特定于平台的包，你可以先创建发布包，然后再使用pymo-convert工具将其转换为各个平台上通用的包。

# 将CPyMO移植到其他平台

CPyMO由一套完全跨平台的通用代码和适配于多平台的“后端”组成。

通用代码放在`cpymo`文件夹中，后端放在`cpymo-backends`文件夹中。

其中`cpymo-backends/include`中的代码规定了每个后端都应当实现的接口。

## 通用部分

### 音频系统

使用宏`DISABLE_FFMPEG_AUDIO`可关闭音频播放器对FFmpeg的依赖，你需要替换成自己的音频系统实现。

使用宏`DISABLE_AUDIO`可完全关闭音频系统。

### 视频播放器

使用宏`DISABLE_FFMPEG_MOVIE`可关闭视频播放器对FFmpeg的依赖，你可以替换为自己的`error_t cpymo_movie_play(cpymo_engine * e, cpymo_parser_stream_span videoname)`函数进行视频播放。

使用宏`DISABLE_MOVIE`可完全关闭视频播放功能。

### 低帧率模式

某些设备可能刷新屏幕会造成闪烁，需要尽可能减少屏幕刷新，这时可定义LOW_FRAME_RATE宏来启用低帧率模式，它将关闭动画效果并显著减少刷新次数。

## SDL2后端

SDL2后端在目录`cpymo-backends/sdl2`中。

### 解除FFmpeg依赖

某些平台上FFmpeg难以编译，同时定义`DISABLE_FFMPEG_AUDIO`和`DISABLE_FFMPEG_MOVIE`即可彻底对解除FFmpeg的依赖，并替换为你的音频视频后端。

如果你只想解除FFmpeg依赖，并且不想提供后端，则可通过同时定义`DISABLE_AUDIO`和`DISABLE_MOVIE`来彻底关闭音频和视频播放器支持。

### SDL2_mixer音频后端

如果你想解除FFmpeg依赖后依然可以播放音频，可以考虑启动SDL2_mixer音频后端支持。    
SDL2_mixer音频后端可能无法播放mp3格式的语音和音效。    

启用SDL2_mixer音频后端之前，必须禁用FFmpeg视频播放器：

* 定义`DISABLE_FFMPEG_MOVIE`来替换为自己的视频播放后端
* 或者定义`DISABLE_MOVIE`彻底关闭视频播放器功能

之后：

1. 定义`DISABLE_FFMPEG_AUDIO`关闭FFmpeg音频依赖
2. 定义`ENABLE_SDL2_MIXER_AUDIO_BACKEND`启用SDL2_Mixer音频后端。

如果你的设备上SDL_mixer中`Mix_Music`不能正常工作，则可以使用`DISABLE_SDL2_MIXER_MUSIC`宏将其替换为使用`Mix_Chunk`来播放BGM。

### 全屏切换

定义宏`ENABLE_ALT_ENTER_FULLSCREEN`可启用按下Alt+Enter键切换全屏的功能。

### 游戏选择器

使用宏`USE_GAME_SELECTOR`可启用游戏选择器。    
一旦使用游戏选择器：
* 你需要修改`main.c`中的`static char *get_last_selected_game_dir()`以获取上一次启动的游戏。
* 你需要修改`main.c`中的`cpymo_game_selector_item *get_game_list()`以获取游戏列表。
* 你需要定义宏`SCREEN_WIDTH`和`SCREEN_HEIGHT`来定义屏幕宽度和高度。
* 你需要定义以下宏以布局游戏选择器UI：
    - `GAME_SELECTOR_FONTSIZE`
    - `GAME_SELECTOR_EMPTY_MSG_FONTSIZE`
    - `GAME_SELECTOR_COUNT_PER_SCREEN`
* 定义宏`GAME_SELECTOR_DIR`以游戏选择目录。

### 加载系统字体

定位到`cpymo_backend_font.c`中的函数`error_t cpymo_backend_font_init(const char *gamedir)`，向此函数添加用于加载系统字体的代码。

### 选择帧缓存格式

定义`FRAMEBUFFER_PIXELFORMAT`为需要使用的SDL2像素格式，即可使用对应的像素格式作为帧缓存格式。

### 禁用图片组件缩放

有些平台可能不支持图片组件的缩放，定义`DISABLE_IMAGE_SCALING`宏即可阻止使用图片组件缩放行为。

### 从文件包中流式加载图片

流式加载图片的性能在不同平台上可能会有不同表现，如果某些设备需要占用更小的内存空间或流式加载图片更快，可定义宏`STREAMING_LOAD_IMAGE`来流式加载图片。

## SDL 1.2后端

该后端用于兼容较为老旧的平台，位于`cpymo-backends/sdl1`中。

SDL 1.2的Alpha混合与缩放功能受限，将不会支持已有透明图层的Alpha变化，同时也禁用了右键菜单的背景缩放功能。

此外，SDL 1.2后端使用了SDL2后端的`cpymo_backend_font.c`和`cpymo_backend_save.c`。

### 桌面版本 (Windows 95/98/Me/2000/XP, Unix)

这里的产生的版本支持较旧的Windows和Unix-like操作系统。

如果你使用Windows，那么你将只能使用msys2和mingw来编译。

你可以使用以下环境变量来修改开关：

* `USE_FFMPEG`，此变量为1时将会连接到FFmpeg库，并启用FFmpeg音视频支持。
* 当`USE_FFMPEG`不为1时，若`USE_SDL_MIXER`为1将连接到`SDL_mixer`，仅启动受限的音频支持。
* 可以使用`SDL`环境变量传入使用自定义`SDL`二进制库目录，如果不传入则使用系统安装的库。
* 设置`DEBUG`为1时，将会启动调试信息。

之后在`cpymo-backends/sdl1`中执行`make`即可编译。

在Windows上，如果你要使用MSYS2进行编译，那么你可能需要以下DLL：
- avcodec-58.dll
- avformat-58.dll
- avutil-56.dll
- libwinpthread-1.dll
- SDL.dll
- swresample-3.dll

### 使用宏适配目标系统

* 画面
    - `FAST_FILL_RECT`定义后，填充矩形时将禁用Alpha混合并提升性能。
    - `SCREEN_BPP`可以定义为8、16、24、32以选择游戏画面的位深度（可选）。
    - `SCREEN_WIDTH`将设置游戏起始画面宽度，如果你使用了游戏选择器，则必须定义此项，并且必须和`SCREEN_HEIGHT`一起定义。
    - `SCREEN_HEIGHT`将设置游戏起始画面高度，如果你使用了游戏选择器，则必须定义此项，并且必须和`SCREEN_WIDTH`一起定义。
    - `SCREEN_FLAGS`可定义`SDL_SetVideoMode`的`flags`参数。
    - `SCREEN_RESIZABLE`定义时，将允许调整游戏窗口大小。
    - `TOGGLE_FULLSCREEN`定义时，将允许用户通过`Alt+Enter`键切换全屏。
    - `REDRAW_WHAT_EVER`定义时，将会不断刷新屏幕，忽略CPyMO的按需刷新功能。
    - `FONT_RENDER_QUALITY`宏可定义为0~3之间的一个值，该值越大，质量越好，该值越小，性能越好。
* FFmpeg支持的音频系统
    - `DEFAULT_CHANNELS`将设置默认音频输出通道数。
    - `DEFAULT_FREQ`将设置默认音频输出的频率。
    - `DEFAULT_SAMPLES`将设置默认缓冲区大小（以采样为单位）。
* `ENABLE_SDL_MIXER_AUDIO_BACKEND`宏将启用SDL_Mixer音频后端，在这之前需要先定义`DISABLE_FFMPEG_AUDIO`。
    - `SDL_MIXER_FREQ`将设置音频的默认输出频率（可选）。
    - `SDL_MIXER_CHANNELS`将设置音频默认输出的通道数（可选）。
    - `SDL_MIXER_CHUNKSIZE`将设置缓冲区大小（可选）。
* `LOAD_GAME_ICON`定义时，将加载游戏图标，此项目与`USE_GAME_SELECTOR`冲突。
* 游戏选择器宏与SDL2后端一致
    - 扩展：定义`GAME_SELECTOR_RESET_SCREEN_SIZE_AFTER_START_GAME`后将允许在启动游戏后重设游戏画面大小。


## CPyMO ASCII ART

这是一个CPyMO变种，没有音频和视频播放器支持，它将会在控制台上输出画面，Just for fun!

### 编译

如果你需要编译到Windows，那么你应该使用`MinGW`和`MSYS2`来构建它。

cd到`cpymo-backends/ascii-art`，执行`make`或`mingw32-make`即可生成可执行文件。

### 启动

参见“CPyMO 桌面平台”的启动方式。

注意：**光敏性癫痫患者请不要使用该版本。**    
注意：Windows上控制台输出效率较低，帧率可能会很差，建议使用Linux或macOS来执行该程序。

CPyMO ASCII ART仅支持键盘操作：

* WSAD为方向键
* J或空格为确认
* K为取消
* L为快进

## CPyMO Text

这是一个CPyMO变种，仅在控制台上输出游戏文本，它在`cpymo-backends/text`内。

关于编译和启动，均与CPyMO ASCII ART相同。


# 工具

## cpymo-tool

该工具用于开发PyMO游戏，与原版PyMO工具完全兼容，它提供以下功能：

* PyMO数据包打包
* PyMO数据包解包
* 游戏图片缩放

启动此程序即可看到详细用法。

### 构建cpymo-tool

你可以选择以下三种方式之一来构建cpymo-tool：

1. 直接使用CMake构建整个项目。
2. 使用GNU Make在`cpymo-tool`目录中执行`make`命令。
2. 使用Microsoft NMake在`cpymo-tool`目录中执行`nmake -f Makefile.Win32`命令构建。

## pymo-strip

该工具用于删除PyMO游戏数据包中没有被使用的部分。

## pymo-convert

该工具用于将高分辨率的PyMO游戏数据包转换为适配各种低性能设备的PyMO游戏数据包，并同时转换视频和音频到目标平台。    
要使用该工具，需要确保你已经安装了最新版本的PowerShell，并已经将`cpymo-tool`、`ffmpeg`、`pymo-convert-audio-to-ogg`安装到命令行中。    

## pymo-convert-audio-to-ogg

由于某些平台上的CPyMO对音频格式支持有限，该脚本可以将所有的音频转换为ogg格式，在运行此脚本前需要确保已经将cpymo-tool和ffmpeg安装到命令行中。

## mo2pymo

该工具用于将特定mo1、mo2游戏转换为PyMO游戏。

## pymo2ykm

该工具用于将PyMO脚本语言编译到[YukimiScript](github.com/Strrationalism/YukimiScript)语言。

## libpymo

用于将PyMO API公开到[YukimiScript](github.com/Strrationalism/YukimiScript)语言中，    
使得YukimiScript语言可以访问PyMO/CPyMO引擎的各项功能。    

# 贡献者

* PyMO原作者
  - chen_xin_ming    
* CPyMO主要作者
  - 许兴逸
* Android无障碍
  - 小涂
* 协助
  - 守望
  - heiyu04
* 测试
  - 幻世
  - °SARTINCE。
  - 白若秋
  - 卢毅
  - benhonjen
  - 镜面倾斜
  - 七月缘
  - __
  - 约汉

# 赞助

如果您有兴趣推动本软件的发展，可以对我们进行赞助。

## 弦语蝶梦独立游戏工作室 开源软件事业

您可以在爱发电对我们进行赞助：

https://afdian.net/order/create?user_id=4ffe65ae104a11ec9bdf52540025c377

## 本软件主要作者 许兴逸

您可以通过支付宝或微信对本软件主要作者许兴逸进行赞助：

![alipay](alipay.jpg)
![wechat](wechatpay.jpg)

## 帮助我们移植到更多平台

如果您有兴趣帮助我们移植到更多平台，您可以考虑向本软件主要作者许兴逸以借用的形式寄送您需要移植CPyMO的设备。    
我们将会评估该平台是否适用CPyMO，在移植和测试工作完成后，该设备将会被退还。    

我们无意于为私人开发的硬件平台进行移植，您所提供的设备必须是在市场上占有一定份额的平台。    

详细信息您可以联系QQ853974536（许兴逸）


如果您有兴趣，您可以直接向仓库推送支持平台的pull request，但我们不会接受：

* 移植到私人开发的硬件平台的补丁
* 含有AGPL不兼容的代码的补丁
* 导致CPyMO与PyMO不再兼容的补丁
* 代码与文字说明不符合的补丁
* 其他有争议的内容
