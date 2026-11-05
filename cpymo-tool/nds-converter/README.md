# CPyMO for NDS 1.3.0 转换助手

## v1.3.0（维护更新）

此版本沿用 1.2.0 的缓存格式与视频转换策略，并进行 **NDS 程序及配套转换器图标焕新**：采用同一张原始插画，NDS 程序使用完整方形图标，Windows/macOS 配套游戏转换器使用平滑圆角版本。macOS 同时改善图标显示比例。

游戏库新增 NDS 固件 16 色主题（含自动跟随固件、默认经典深蓝）、统一的选中竖线和增强的多语言主题文字层次。修订了“重新开始”后标题辅助屏的回切、START 启动与选中高亮边缘。音频输出采用保守的 +1.5dB 目标增益和峰值保护；不替代各游戏自身音量设置。


NDS 移植：千岚之夏。独立转换助手 MIT；引擎 AGPL-3.0-or-later；字体与依赖保留各自许可。

程序及配套元数据由同一份 1.3.0 源码真实构建；[当前发行说明](../../cpymo-backends/nds/RELEASE-NOTES-1.3.0.md)。

## 给玩家

Windows 玩家完整解压 Starter，双击“启动转换器.bat”，也可运行“Converter/Converter.exe”。Apple Silicon Mac 玩家使用通过最终验收的 ARM64 `.app` 发行包；Intel Mac 可按下文从源码运行。
免安装套件已内置 Python 运行时和 FFmpeg，普通玩家无须另行安装。打开助手后，可选择简体中文或 English（界面选择会保存）；选择游戏 ZIP 或含 gameconfig.txt 的文件夹，
支持嵌套 ZIP 与 S60v3 / S60v5 / Android PyMO 数据（指输入数据平台，非电脑系统）。
同包多版本优先推荐 S60v3。选择新的输出位置，开始转换，结束后阅读兼容性警告与安装必读。

把输出的 cpymo-nds.nds 和 pymo 文件夹复制到 TF 根目录。路径应为
`pymo/games/<游戏目录>/gameconfig.txt`，旁边有 script、nds-cache 和 save。
保留原有烧录卡内核或 DSi 启动配置。已有同名游戏先保留自己的 save，不要用新输出的空 save 覆盖。
转换助手在电脑使用，不需要复制进 TF。本包不附游戏。

默认精简已有缓存的重复原媒体，无损压缩语音与适合的图片；BGM/音效保持 PCM，兼顾读取和存储。
无损针对 NDS 缓存：图像先缩放为 DS 尺寸 RGB555，音频先转为 22050 Hz 单声道 PCM16。
缓存压缩不会在此基础上继续降低像素或采样质量；不承诺比所有原版平台包更小。
可选择不压缩缓存作对照；全部音频压缩是实验项，不属于默认性能基线。
启用缓存压缩会同时精简重复媒体。输出专用于 NDS，保留原包供其他平台使用。

视频资源默认转换为 NVL1，玩家也可选择 NVA1（压缩音轨）或主动跳过视频。字库缺字默认替换为 □ 并生成警告，原始包、脚本和定稿汉化不修改，源存档不导入。
转换通过不等于全剧情或全部烧录卡认证。缺资源、未知指令和缺字策略记录在
`nds-cache/manifest.json` 及转换结果中。GUI 默认缺字显示 □ 并记录，脚本文字不改；英文界面转换完成后还会生成 `INSTALL-READ-ME.txt` 英文安装说明。
加密/打包脚本和其他游戏引擎不支持。

## 1.2 游戏视频转换

1.1.0 正式版不受影响。1.3.0 继承 1.2.0 的转换流程，视频默认使用 NVL1、原样保留脚本；
1.2 提供 **NVL1**（22050 Hz PCM16）与 **NVA1**（NAR2
无损压缩音频）视频转换，在 GUI 的“游戏视频”下拉框中选择（默认 NVL1，NVA1 和跳过视频可选）。默认
12 FPS、256×192 RGB555；保留原始画面比例，居中补黑边，不随意拉伸。
字幕、脚本与存档策略不改；无音轨视频补零音频。

CLI 用法（发行前请确认与同版 NDS 引擎匹配）：



```sh
python cpymo-tool/nds-converter/convert.py convert 原游戏目录 新输出目录 --cache-only
# 可选：使用压缩音轨的视频转换模式
python cpymo-tool/nds-converter/convert.py convert 原游戏目录 另一新目录 --video-mode nva1 --video-fps 12
```

识别游戏目录内的 \`video/\`、\`movie/\`、\`videos/\`、\`movies/\`
普通视频（FFmpeg 支持的 MP4/AVI/MKV/MOV 等）。输出为
\`nds-cache/video/<哈希前缀>/<视频名哈希>.nvl\`；原 PyMO \`#movie\` 指令
无需修改。全程校验帧的 Adler32 与像素还原，音轨严格按视频帧数对齐；
不支持的视频会在审计中继续标注为未转换。单段视频受现有 NDS 播放器
10 分钟限制。生成缓存及主机验证通过**不等于** NTR/DSi 实机视频性能、
跳过恢复、所有原始游戏视频的兼容性已验收。

1.2 对玩家公开的可播放视频格式为 NVL1／NVA1；不要将其他研究阶段的格式当成正式支持功能。


## 从源码运行与构建

Python 3.10+，安装 requirements.txt，另备 FFmpeg；完整仓库内运行 `python app.py`。
Windows 提供免安装包；macOS **仅发行 Apple Silicon 原生 ARM64 `.app`**，不提供 Intel macOS 预编译包。Apple Silicon 版本使用 ARM64 Python、PyInstaller 和 FFmpeg 构建，通过同提交原生构建及配套验收发布。Intel Mac、Linux 仍可按要求从源码运行命令行工具；Mac 采用临时签名，未经 Apple Developer ID 签名及 Apple 公证；请核对来源与 SHA-256。

```sh
python -m pip install -r cpymo-tool/nds-converter/requirements.txt
python cpymo-tool/nds-converter/convert.py convert /path/to/game /path/to/new-output --ffmpeg /path/to/ffmpeg --missing-glyph-policy box --cache-only --audio-lossless voice --compress-images
python cpymo-tool/nds-converter/convert.py verify /path/to/new-output
```

CLI 默认也将缺字替换为 □ 并记录；若开发者希望针对字体严格检查，可明确传入 `--missing-glyph-policy error`。CLI 保留便于对照的原媒体／未压缩缓存，GUI 默认启用精简与推荐压缩方案。
缓存格式6按哈希前两位分目录，兼容引擎旧格式1～5；`--cache-layout flat` 用于对照测试。
图像/音频去重不改脚本资源名称。保留 fonts/ 的字体文件和全部许可，不要只拷单个 Python 文件。
版本从 cpymo-backends/nds/release.json 读取。macOS ARM64 构建由 `build_portable_macos.py --target-arch arm64` 在 Apple Silicon CI 上完成，打包器会对主程序和 FFmpeg 都做 ARM64 架构校验。

构建 Windows 包：先构建 ROM，安装 requirements-build.txt，使用可信 Windows x86_64 C 编译器构建加速库：

```sh
python build_native.py --cc /path/to/x86_64-w64-mingw32-gcc.exe
python build_portable.py --rom /path/to/verified/cpymo-nds.nds --ffmpeg /path/to/ffmpeg.exe --output /path/to/new-portable
```

加速库可选，源码模式缺失时使用 Python 实现；公开 Windows 包包含该库。
打包脚本嵌入并检查同一 ROM 与版本元数据。发布时还需附玩家必读、第三方许可和完整对应源码。
FFmpeg 独立进程运行，不链接进 MIT 转换器；tools/ 提供音频专用构建脚本。
Windows/macOS 包内的 `FFmpeg-Corresponding-Source` 提供对应 FFmpeg 官方源码、编译配置及许可；发行源码包提供引擎、转换器和构建脚本。


macOS ARM64 源码构建（须在 Mac 上运行，且显式传入与目标发行版本匹配的 ROM）：

```sh
python cpymo-tool/nds-converter/build_portable_macos.py \
  --rom /path/to/cpymo-nds.nds --ffmpeg /path/to/ffmpeg \
  --output /path/to/new-macos-package
```

历史原生构建记录：[原生 macOS CI](https://github.com/Blueteemo/CPyMO/actions/runs/40484402394)：
使用打包后的 `.app` 工作进程转换原创 MP4 为 NVL1 并校验配套 ROM 的 SHA。
该 CI 的 ROM 是**内部构建测试输入**，不是用户最终验收并内置星夜素材的发行 ROM，
不能直接拿该构建的研究包替代正式版；仍需完整许可证、最终 ROM 配套及发行门槛。

回归：`python -m unittest discover -s cpymo-tool/nds-converter/tests -v`，可用 NDS_TEST_FFMPEG 指定程序。
app.py 的 `--self-test JSON` 通过真实 GUI 与工作进程完成转换，未指定压缩字段时使用实际界面默认值。
