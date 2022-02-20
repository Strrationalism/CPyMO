
```
   __________        __  _______
  / ____/ __ \__  __/  |/  / __ \
 / /   / /_/ / / / / /|_/ / / / /
/ /___/ ____/ /_/ / /  / / /_/ /
\____/_/    \__, /_/  /_/\____/
           /____/                                                  
```

<img src="https://raw.githubusercontent.com/pymo/pymo/master/images/pymo-icon.png" width=128 height=128 />

![LICENSE](https://www.gnu.org/graphics/agplv3-88x31.png)

此项目尚在工作中！欢迎提交代码！

此项目仅用于您运行**合法持有**的游戏软件副本，持有盗版副本可能会让您面临法律问题。    
这是一个使用C实现的pymo引擎的复刻，以方便在各种平台上制作并运行pymo游戏。    

此项目基于AGPLv3协议发布，CPyMO中API的实现并不是AGPLv3定义中的系统库，如果您使用CPyMO制作和发行游戏，**那么您制作的游戏也必须以AGPLv3协议发布。**    


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
    - Nintendo 3DS

# cpymo-tool

该工具用于实现pymo原版开发工具的功能。

## 用法

```
cpymo-tool
Development tool for cpymo.

Unpack a pymo package:
    cpymo-tool unpack <pak-file> <extension_without "."> <output-dir>
```

## 编译为SDL2项目

### 额外依赖

你需要使用类似vcpkg的包安装以下依赖：

* SDL2
* ffmpeg

如果你使用Microsoft Visual Studio，默认的CMakeSettings.json中指示的依赖版本为x64-windows-static。

## 编译到任天堂3DS平台

### 额外依赖

* DevkitPro
  - libctru
  - citro2d
  - citro3d
* ffmpeg

#### 编译ffmpeg到3DS平台

将`cpymo-backends/3ds/ffmpeg-configure-3ds`复制到ffmpeg源码文件夹下：

如果你使用Windows，则需要在msys2中执行该脚本，之后执行make install。    
如果你使用其他Unix-like操作系统，则在sh中执行该脚本，之后执行make install。    
之后ffmpeg的3ds版本即可安装到devkitPro的portlibs文件夹下。    

### 产生cia文件
于`./cpymo-backends/3ds/`目录下执行`make`即可生成3DSX程序。    
你需要确保已经安装了`makerom`命令，之后在`./cpymo-backends/3ds/`下使用`make cia`来创建cia文件。    

你可以在 https://github.com/3DSGuy/Project_CTR 找到makerom的可执行文件。

### 启动
你需要将你的游戏放置于`SDMC:/pymogames/startup`下，之后CPyMO for 3DS会自动启动该目录下的游戏。

#### 如果无法启动CIA版本的话？

1. 目前仅在New 3DS日版（系统版本号Ver 11.15.0-47J）上对CIA版本进行过测试。
2. 如果你的机器在运行CIA版本的CPyMO时崩溃，请尝试切换到3dsx版。

#### 如何启动3DSX版本？

1. 将cpymo.3dsx放入SD卡的3ds目录下。
2. 启动Homebrew Launcher，建议使用这里的Homebrew Launcher Dummy（https://github.com/PabloMK7/homebrew_launcher_dummy ）。
3. 执行cpymo.3dsx。

### 关于字体

3DS版本的CPyMO不会加载游戏中自带的字体或者其他TTF字体，而是使用[思源黑体](https://github.com/adobe-fonts/source-han-sans)。    
思源黑体已经被转换为可以被3DS直接识别的bcfnt格式，CPyMO for 3DS中的思源黑体将会按照其原本的[SIL协议](https://github.com/adobe-fonts/source-han-sans/blob/master/LICENSE.txt)随CPyMO for 3DS一起分发。    

