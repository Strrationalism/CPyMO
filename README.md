
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

# cpymo-tool

该工具用于实现pymo原版开发工具的功能。

## 用法

```
cpymo-tool
Development tool for cpymo.

Unpack a pymo package:
    cpymo-tool unpack <pak-file> <extension_without "."> <output-dir>
```

## 编译到Windows、Linux或macOS

### 额外依赖

你需要使用vcpkg包管理器安装以下依赖：

* SDL2
* ffmpeg

如果你使用Microsoft Visual Studio，默认的CMakeSettings.json中指示的依赖版本为x64-windows-static。

## 编译到任天堂3DS平台

### 额外依赖

* devkitPro + 3ds dev
* FFmpeg 5.0

#### 安装3DS用的FFmpeg依赖

在终端或devkitPro的MSYS2控制台（如果你使用Windows的话），cd到`cpymo-backends/3ds/`目录下，执行`./install-3ds-ffmpeg.sh`。

### 产生cia文件
于`./cpymo-backends/3ds/`目录下执行`make`即可生成3DSX程序。    
你需要确保已经安装了`makerom`命令，之后在`./cpymo-backends/3ds/`下使用`make cia`来创建cia文件。    

你可以在 https://github.com/3DSGuy/Project_CTR 找到makerom的可执行文件。

### 启动
你需要将你的游戏放置于`SDMC:/pymogames/`下，保证路径中只有半角英文、数字和下划线，之后该游戏便会被CPyMO for 3DS检测到。   
如果你已经安装了Citra且citra命令可用，你可以直接使用`make run`来调用Citra模拟器来启动CPyMO。    

CPyMO for 3DS支持3D显示，可使用3D滑块来打开3D显示功能。    
按下START键可以快速退出CPyMO。       
按下SELECT键在四种屏幕模式之间切换。
ZL和ZR键功能和A、Y键相同，用于单手操作。    

#### 以调试模式启动
如果你需要查看CPyMO控制台，你需要在游戏列表中**按住L键**，同时按下A键选择游戏，即可激活调试模式。    
在这种模式下，下屏会显示CPyMO控制台，Start键将不再可用，对话文本会被强制显示在上屏。    

#### 如果无法启动CIA版本的话？

1. 目前仅在New 3DS日版（系统版本号Ver 11.15.0-47J）上对CIA版本进行过测试。
2. 如果你的机器在运行CIA版本的CPyMO时崩溃，请尝试切换到3dsx版。

#### 如何启动3DSX版本？

1. 将cpymo.3dsx放入SD卡的3ds目录下。
2. 启动Homebrew Launcher，建议使用这里的Homebrew Launcher Dummy（https://github.com/PabloMK7/homebrew_launcher_dummy ）。
3. 执行cpymo.3dsx。

### 在3DS中没有声音？

你需要确保已经Dump了3DS的DSP固件。    
如果你没有Dump，那么你需要先安装DSP1（https://github.com/zoogie/DSP1/releases/tag/v1.0 ），并使用它Dump你的3DS的DSP固件。


### 关于字体

3DS版本的CPyMO不会加载游戏中自带的字体或者其他TTF字体，而是使用[思源黑体](https://github.com/adobe-fonts/source-han-sans)。    
思源黑体已经被转换为可以被3DS直接识别的bcfnt格式，CPyMO for 3DS中的思源黑体将会按照其原本的[SIL协议](https://github.com/adobe-fonts/source-han-sans/blob/master/LICENSE.txt)随CPyMO for 3DS一起分发。    

## 编译到任天堂Switch平台

### 额外依赖

* devkitPro + switch dev
* switch-sdl2
* switch-ffmpeg

直接使用devkitPro pacman安装即可。

### 编译

cd到`cpymo-backends/sdl2`，执行`make -j -f Makefile.Switch`即可编译到任天堂Switch平台。    
使用`make run -j -f Makefile.Swtich`即可使用yuzu模拟器运行。    

### 启动

将游戏放入SD卡的`/pymogames/startup`目录，使得`/pymogames/startup`可用，之后即可启动CPyMO运行此目录下的游戏。    




