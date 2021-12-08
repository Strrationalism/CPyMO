# cpymo

此项目尚在工作中！欢迎提交代码！

这是一个使用C实现的pymo引擎的复刻，以方便在各种平台上运行pymo游戏。

pymo原版参见：https://github.com/pymo/pymo    
pymo官网：https://www.pymogames.com/    
原作者：chen_xin_ming    

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

首先你需要确保已经安装了devkitPro及其中的3DS开发套件，在其控制台中，于`./makefiles/3ds/`目录下执行`make`即可生成3DSX程序。
