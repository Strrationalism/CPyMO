# CPyMO for NDS 1.3.0

**维护更新：界面主题、双屏辅助指南、选中高亮、音频输出，以及 NDS 程序及配套转换器图标焕新。**

## 本次更新

- 新增原版 Nintendo DS／DS Lite 固件的 16 种主题颜色，支持“默认”经典配色与“自动”跟随主机设置。自动模式的 RGB555 配色在载入和设置时缓存，避免反复读取主机固件影响游戏列表操作响应。
- 统一 2 像素选中竖线，默认深蓝版采用最终校准的蓝青色；彩色主题采用清晰的浅色主文字与柔和的主题强调文字。修正“默认／自动”选项的居中。
- 从游戏资料库按 **START** 可启动游戏；菜单脚注和多语言辅助说明按最终版更新。
- 游戏内确认“重新开始”时恢复标题阶段的辅助屏指引，进入正文后再次自动切换；三种显示模式仍可通过 **L** 切换。原游戏标题菜单图像与脚本不改动。
- 修复《水仙1》《水仙2》标题菜单选中高亮的 RGB555 Alpha 混合取整问题，无需重新转换原游戏资源。
- 音频增加约 **+1.5 dB** 的保守目标增益，峰值保护避免 PCM 输出数字削波；BGM、语音、音效和视频音轨共用保护逻辑，不改变游戏配置或硬件音量。听感仍因录音响度和主机而异。
- **NDS 程序及配套转换器图标焕新。** 采用同源的用户提供插画：NDS 保留完整方形构图，搭配柔和细边框，小图轮廓更清楚，加载画面的图标也更细腻；Windows／macOS 的**配套游戏转换器**采用平滑圆角版本。macOS 同时改善图标显示比例。两款桌面工具为配套游戏转换器。

## 兼容、升级与下载

1. 继续使用同一个自动识别 DS/NTR、DSi/TWL 的通用 `cpymo-nds.nds`；已有可运行的 1.2.0 游戏缓存与存档可以保留，通常只需更新 ROM。请勿覆盖已有游戏目录及 save 数据。
2. Windows 转换器提供免安装 ZIP；Apple Silicon Mac 提供 ARM64 原生 `.app`（未经 Apple Developer ID 签名、公证）。Intel Mac/Linux 可使用适配环境下的源码工具；不提供 Intel Mac 的预编译版。
3. 转换器接收 PyMO 游戏 **ZIP／已解压文件夹**；Android PyMO 游戏数据可转换，但不把 APK 安装包作为直接输入。NVL1 视频仍为默认，可选 NVA1 或跳过视频。发行包**不包含任何游戏文件**。
4. 音频增益、主题配色仅属于 NDS 移植后端；不改变上游通用 CPyMO 的行为。

## 法律、作者与验证

NDS 移植：**千岚之夏（Blueteemo）**；CPyMO：Seng Jik 与贡献者；PyMO：chen_xin_ming 与贡献者。引擎／后端为 AGPL-3.0-or-later，独立转换器为 MIT；Fusion Pixel、GNU Unifont 及 FFmpeg 的对应来源与许可证随源码和打包发行。桌面图标源自 PyMO 原图案的 AI 辅助高清衍生设计，保留原作者声明与 MIT 许可证，不暗示 PyMO 原作者参与或认可本次发布。

发行时应提供同一构建的 ROM、Windows 版、macOS ARM64 版、源码与 SHA-256 清单。macOS 首次运行可能触发 Gatekeeper：请验证下载来源与 SHA-256，按 Apple 系统设置中的官方流程处理；不要关闭 SIP 或全局 Gatekeeper。

程序与配套转换器实际版本统一为 **1.3.0**，由同一份源码重新构建。此前未正式发布的 1.2.1 候选记录保留为研究历史。正式文件、实际 SHA-256 及构建验收记录以 [1.3.0 发行页](https://github.com/Blueteemo/CPyMO/releases/tag/nds-v1.3.0) 和随附清单为准。

[项目仓库](https://github.com/Blueteemo/CPyMO) · [NDS 后端说明](https://github.com/Blueteemo/CPyMO/tree/main/cpymo-backends/nds) · [上游 PR #60](https://github.com/Strrationalism/CPyMO/pull/60)
