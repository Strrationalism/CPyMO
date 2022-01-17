# cpymo

此项目尚在工作中！欢迎提交代码！

此项目仅用于您运行**合法持有**的游戏软件副本，持有盗版副本可能会让您面临法律问题。

这是一个使用C实现的pymo引擎的复刻，以方便在各种平台上运行pymo游戏。

pymo原版参见：https://github.com/pymo/pymo    
pymo官网：https://www.pymogames.com/
pymo官网镜像：https://seng-jik.github.io/cpymo.github.io/    
原作者：chen_xin_ming    

感谢幻世为cpymo提供测试样例以使得cpymo与pymo的运行结果尽可能一致。    
感谢守望、heiyu04为cpymo的开发提供协助。

主要目标：

* 兼容原版pymo的游戏、存档的情况下运行游戏
* 提供与原版pymo兼容的pymo开发工具
* 在带有硬件加速的情况下跨平台
    - Windows
    - Linux
    - Android
    - Nintendo 3DS
* libcpymo库可将cpymo或cpymo中的组件嵌入到其他应用程序中

# cpymo-tool

该工具用于实现pymo原版开发工具的功能。

## 用法

```
cpymo-tool
Development tool for cpymo.

Unpack a pymo package:
    cpymo-tool unpack <pak-file> <extension_without "."> <output-dir>
```

## 编译到任天堂3DS平台

首先你需要确保已经安装了devkitPro及其中的3DS开发套件，在其控制台中，于`./cpymo-backends/3ds/`目录下执行`make`即可生成3DSX程序。

### 产生cia文件

你需要确保已经安装了`makerom`命令，之后在`./cpymo-backends/3ds/`下使用`make cia`来创建cia文件。

### 游戏兼容性提示

3DS兼容所有版本的PyMO游戏数据包，但s60v5版本体验最好，如果没有s60v5版本，也可以使用s60v3版本。    
3DS上使用安卓版本的PyMO数据包可能会导致游戏运行卡顿，或游戏画面产生锯齿等问题。    

