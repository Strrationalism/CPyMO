
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
原作者：chen_xin_ming    

感谢幻世为cpymo提供测试样例以使得cpymo与pymo的运行结果尽可能一致。    
感谢守望、heiyu04为cpymo的开发提供协助。

主要目标：

* 提供与原版pymo兼容的pymo开发工具
* 在带有硬件加速的情况下跨平台
    - Windows
    - Linux
    - macOS
    - Nintendo 3DS（[在Universal Updater应用商店中查看](https://db.universal-team.net/3ds/cpymo)）
    - Nintendo Switch
    - Sony Playstation Portable

# 编译到Windows、Linux或macOS

## 额外依赖

你需要使用vcpkg包管理器安装以下依赖：

* SDL2
* ffmpeg

如果你使用Microsoft Visual Studio，默认的CMakeSettings.json中指示的依赖版本为x64-windows-static。

## 视障帮助功能

github action及release上的版本默认会开启视障帮助功能，如果你需要禁用视障帮助功能，可在编译时定义宏NON_VISUALLY_IMPAIRED_HELP.

视障帮助功能将会把游戏中的文本复制到剪切板供读屏软件读取。    

仅Windows、macOS和Linux版本支持视障帮助功能。

## 全屏

按下Alt + Enter键可在全屏/窗口模式中切换。

# 编译到任天堂3DS平台

## 额外依赖

* devkitPro + 3ds dev
* FFmpeg 5.0

### 安装3DS用的FFmpeg依赖

在终端或devkitPro的MSYS2控制台（如果你使用Windows的话），cd到`cpymo-backends/3ds/`目录下，执行`./install-3ds-ffmpeg.sh`。

## 产生cia文件
于`./cpymo-backends/3ds/`目录下执行`make`即可生成3DSX程序。    
你需要确保已经安装了`makerom`命令，之后在`./cpymo-backends/3ds/`下使用`make cia`来创建cia文件。    

你可以在 https://github.com/3DSGuy/Project_CTR 找到makerom的可执行文件。

## 启动
你需要将你的游戏放置于`SDMC:/pymogames/`下，保证路径中只有半角英文、数字和下划线，之后该游戏便会被CPyMO for 3DS检测到。   
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

# 编译到任天堂Switch平台

## 额外依赖

* devkitPro + switch dev
* switch-sdl2
* switch-ffmpeg

直接使用devkitPro pacman安装即可。

## 编译

cd到`cpymo-backends/sdl2`，执行`make -j -f Makefile.Switch`即可编译到任天堂Switch平台。    
使用`make run -j -f Makefile.Swtich`即可使用yuzu模拟器运行。  

## 启动

将游戏放入SD卡的`/pymogames/`目录即可。

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

# 编译到索尼PSP平台

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

在使用PPSSPP模拟器的情况下，你需要关闭“系统设置 - 快速内存访问”。    

## 缺陷

这些缺陷将不会得到修复：

* 由于PSP生态中没有移植的FFmpeg库
    - 视频播放器功能已经禁用
	- 音频播放使用SDL2_mixer后端
        - 音频不支持mp3格式
* 由于PSP机能有限
    - 仅能加载s60v3数据包或下述推荐的PSP数据包
	- 在放入太多游戏时将无法加载游戏列表
	- 加载音效会导致严重卡顿，故禁用音效
	- 将不会在启动游戏时加载游戏目录下的`system/default.ttf`字体
* 由于stb_image会在PSP中崩溃
    - 某些情况下使用icon.png会导致崩溃，如果出现了这种情况请删除icon.png
* 由于SDL2 for PSP存在问题
    - 游戏将会在屏幕左上角显示，而不是居中显示
* 由于缺乏PSP实体机进行调试
    - 目前仅可在PPSSPP在关闭“快速内存访问”时可以正常启动

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

### 利用pymo-converter从PyMO Android版本数据包创建psp版本的数据包

1. 你需要安装cpymo-tool到你的系统中（添加到PATH环境变量），确保该命令可用。
2. 下载pymo-converter.ps1。
3. 使用命令行pymo-converter.ps1 psp <PyMO Android版本数据包路径> <输出的PSP版本的数据包路径>。
4. 手动转换BGM音频为ogg格式，并修改gameconfig.txt中bgmformat一栏为`.ogg`，如果原本就是ogg格式则无需修改。
5. 手动转换VO音频为ogg格式，并修改gameconfig.txt中的voformat一栏为`.ogg`，如果原本就是ogg格式则无需修改。

# CPyMO移植提示

CPyMO由一套完全跨平台的通用代码和适配于多平台的“后端”组成。

通用代码放在`cpymo`文件夹中，后端放在`cpymo-backends`文件夹中。

其中`cpymo-backends/include`中的代码规定了每个后端都应当实现的接口。

## 通用部分

### 视障帮助

使用宏`NON_VISUALLY_IMPAIRED_HELP`可以关闭视障帮助功能。

### 音频系统

使用宏`DISABLE_FFMPEG_AUDIO`可关闭音频播放器对FFmpeg的依赖，你需要替换成自己的音频系统实现。

使用宏`DISABLE_AUDIO`可完全关闭音频系统。

### 视频播放器

使用宏`DISABLE_FFMPEG_MOVIE`可关闭视频播放器对FFmpeg的依赖，你可以替换为自己的`error_t cpymo_movie_play(cpymo_engine * e, cpymo_parser_stream_span videoname)`函数进行视频播放。

使用宏`DISABLE_MOVIE`可完全播放所有的视频播放功能。

## SDL2后端

SDL2后端在目录`cpymo-backends/sdl2`中。

### 解除FFmpeg依赖

某些平台上FFmpeg难以编译，同时定义`DISABLE_FFMPEG_AUDIO`和`DISABLE_FFMPEG_MOVIE`即可彻底对解除FFmpeg的依赖，并替换为你的音频视频后端。

如果你只想接触FFmpeg依赖，并且不想提供后端，则可通过同时定义`DISABLE_AUDIO`和`DISABLE_MOVIE`来彻底关闭音频和视频播放器支持。

### SDL2_mixer音频后端

如果你想解除FFmpeg依赖后依然可以播放音频，可以考虑启动SDL2_mixer音频后端支持。    
SDL2_mixer音频后端可能无法播放mp3格式的语音和音效。    

启用SDL2_mixer音频后端之前，必须禁用FFmpeg视频播放器：

* 定义`DISABLE_FFMPEG_MOVIE`来替换为自己的视频播放后端
* 或者定义`DISABLE_MOVIE`彻底关闭视频播放器功能

之后定义：

1. 定义`DISABLE_FFMPEG_AUDIO`关闭FFmpeg音频依赖
2. 定义`ENABLE_SDL2_MIXER_AUDIO_BACKEND`启用SDL2_Mixer音频后端。

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

# 工具

## cpymo-tool

该工具用于开发PyMO游戏，与原版PyMO工具完全兼容，它提供以下功能：

* PyMO数据包打包
* PyMO数据包解包
* 游戏图片缩放

启动此程序即可看到详细用法。

## pymo-converter

该工具用于将高分辨率的PyMO游戏数据包转换为适配各种低性能设备的PyMO游戏数据包。    
要使用该工具，需要确保你已经安装了最新版本的PowerShell，并已经将cpymo-tool安装到命令行中。    

pymo-converter目前支持将游戏适配到以下设备：

* s60v3
* s60v5
* pymo-android
* 3ds
* psp

## mo2pymo

该工具用于将特定mo1、mo2游戏转换为PyMO游戏。

## pymo2ykm

该工具用于将PyMO脚本语言编译到[YukimiScript](github.com/Strrationalism/YukimiScript)语言。

## libpymo

该工具用于将PyMO API公开到[YukimiScript](github.com/Strrationalism/YukimiScript)语言中，    
使得YukimiScript语言可以访问PyMO/CPyMO引擎的各项功能。    


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
