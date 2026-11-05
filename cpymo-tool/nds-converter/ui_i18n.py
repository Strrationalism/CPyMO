"""Small, dependency-free player-facing UI translations (MIT).

The canonical UI strings are Simplified Chinese. Translation is applied at
display time only: PyMO scripts, conversion diagnostics and output paths never
change when users switch the assistant's language.
"""

LANGUAGES = ("简体中文", "English")
UI_EN = {
    "CPyMO NDS 转换助手": "CPyMO NDS Converter",
    "选择游戏包，生成可复制到 DS / DSi 存储卡的游戏目录。无需另装 Python 或 FFmpeg。":
        "Choose a PyMO game and prepare its DS / DSi files. No separate Python or FFmpeg installation.",
    "1  选择游戏": "1  Select a game",
    "选压缩包": "Open ZIP",
    "选文件夹": "Open folder",
    "支持 S60v3 / S60v5 / Android PyMO 数据；同包有多个版本时优先推荐 S60v3。":
        "Supports S60v3 / S60v5 / Android PyMO data. If multiple versions exist, S60v3 is preferred.",
    "2  选择保存位置": "2  Choose output location",
    "选保存位置": "Browse",
    "自动创建新文件夹，不覆盖原包或已有存档；视频默认转换为 NVL1。":
        "Creates a new output folder. Source games and existing saves are never overwritten. NVL1 video is on by default.",
    "缺字自动以 □ 显示并记录警告，不中断转换；原脚本保持不变。":
        "Missing font glyphs appear as □ with a warning. Conversion continues and scripts stay unchanged.",
    "精简输出：不重复保存已转换的图片／音频（原游戏、脚本与署名不变）":
        "Smaller output: omit converted source images/audio (original game, scripts and credits unchanged)",
    "不压缩缓存（读取优先）": "Uncompressed cache (faster reads)",
    "无损压缩语音与适合的图片（推荐）":
        "Lossless voice and image cache compression (recommended)",
    "无损压缩全部音频与适合的图片（实验）":
        "Lossless compression for all audio and images (experimental)",
    "缓存压缩：": "Cache compression:",
    "安装包已包含 CPyMO {version}；缓存压缩不会进一步降低转换后素材质量。":
        "CPyMO {version} is included. Cache compression does not further reduce converted asset quality.",
    "不转换视频（兼容旧流程）": "Skip videos (legacy mode)",
    "转换视频 NVL1（推荐：兼容优先）": "Convert to NVL1 (recommended)",
    "转换视频 NVA1（压缩音频）": "Convert to NVA1 (compressed audio)",
    "游戏视频：": "Game videos:",
    "视频默认转换为 256×192 / 12 FPS；可选 NVA1 或跳过，安装包已含对应引擎。":
        "Video defaults to 256×192 at 12 FPS. NVA1 or skip is optional; the matching engine is bundled.",
    "3  开始转换": "3  Convert",
    "取消": "Cancel",
    "打开结果文件夹": "Open output folder",
    "NDS 平台移植：千岚之夏  ·  基于 CPyMO / PyMO  ·  转换助手 {version}":
        "NDS port: 千岚之夏  ·  Based on CPyMO / PyMO  ·  Converter {version}",
    "先选择游戏压缩包或游戏文件夹。": "Select a game ZIP or folder to begin.",
    "选择 PyMO 游戏包": "Select a PyMO game package",
    "游戏包": "Game packages",
    "全部文件": "All files",
    "选择含 gameconfig.txt 的游戏数据目录": "Select the game folder containing gameconfig.txt",
    "选择保存位置": "Choose an output folder",
    "正在识别游戏包里的平台版本……": "Detecting platform variants in the game package…",
    "推荐  ": "Recommended  ",
    "已识别游戏。确认保存位置后，点击“开始转换”。":
        "Game recognized. Choose an output folder, then click Convert.",
    "保存位置不能位于原游戏目录内部。":
        "The output folder cannot be inside the source game's directory.",
    "正在转换，请稍候。语音较多的游戏需要更长时间……":
        "Converting… Games with extensive voice audio may take longer.",
    "。点击“打开结果文件夹”查看安装必读。":
        ". Open the output folder and read the installation instructions.",
    "已转换，仍需实际运行确认":
        "Converted with warnings; on-device verification is still recommended",
    "转换和完整性检查通过，仍需实际运行确认":
        "Conversion and integrity checks passed; on-device verification is still recommended",
    "未发现静态兼容性警告。": "No static compatibility warnings were found.",
    "已省去重复媒体 {size:.2f} MiB。脚本与署名保留，原包未修改。":
        "Removed {size:.2f} MiB of redundant media. Scripts and credits kept; source unchanged.",
    "无损缓存另节省 {size:.2f} MiB。请使用随附的 {version} 引擎。":
        "Lossless caching saved another {size:.2f} MiB. Use the bundled {version} engine.",
    "未完成：": "Not completed: ",
    "暂时不能完成转换": "Conversion could not be completed",
    "原游戏包未改动。诊断目录：": "Source game was not modified. Diagnostic folder: ",
    "已取消。原游戏包未改动；未完成目录和日志保留供检查。":
        "Cancelled. Source unchanged; incomplete output and logs remain for inspection.",
    "转换进程意外退出，请保留诊断日志。":
        "The conversion worker exited unexpectedly. Please keep the diagnostic logs.",
    "界面语言：": "Language:",
    "检测到不支持的素材；请查看诊断日志。": "Unsupported assets were detected; see diagnostic logs.",
    "默认保留并转换游戏视频；如不需要视频，也可以选择跳过。":
        "Videos are converted automatically. Choose Skip only if you do not want them.",
    "更多设置（可选）": "More settings (optional)",
    "收起更多设置": "Hide additional settings",
    "文件体积与读取：": "Storage and loading:",
    "默认设置兼顾体积和读取速度，普通使用无需调整。":
        "Recommended settings balance file size and loading speed. Usually no changes are needed.",
    "查看错误日志": "Open diagnostic logs",
    "转换完成！请打开结果文件夹，按说明复制到 TF 卡。":
        "Done! Open the output folder and follow the instructions to copy files to your storage card.",
    "转换完成！有一些提示，建议先查看下方记录。":
        "Conversion finished with some notes. Please read the messages below.",
    "自动识别游戏版本；如果找到多个版本，会优先推荐适合 NDS 的版本。":
        "Game versions are detected automatically. A recommended version is selected if multiple are found.",
    "转换后保存在新文件夹，不会修改原游戏或覆盖已有存档。":
        "A new output folder is created. Your original game and existing saves are left untouched.",
    "选择自己的 PyMO 游戏，转换好后复制到 DS / DSi 的存储卡即可。":
        "Select your PyMO game, convert it, then copy the result to your DS / DSi storage card.",
    "缺少视频和音频转换组件 FFmpeg，请完整解压转换助手。":
        "FFmpeg is missing. Extract the complete converter package.",
    "视频转换模式无效。": "Invalid video conversion mode.",
    "无损压缩模式无效。": "Invalid lossless compression mode.",
    "嵌套游戏包太多，请单独选择要转换的游戏包。":
        "Too many nested game packages; select one game package.",
}

DIAGNOSTIC_PREFIXES_EN = {
    "已识别：": "Detected: ",
    "正在转换图片、音频和字体，游戏较大时需要等待……":
        "Converting images, audio and fonts; larger games may take longer…",
    "未知脚本指令：": "Unknown script commands: ",
    "缺失资源引用：": "Missing asset references: ",
    "需运行检查的指令：": "Commands requiring runtime checks: ",
    "字库缺字以□显示，原脚本未改：": "Missing glyphs displayed as □; original script unchanged: ",
}

def tr(value: str, language: str = "简体中文") -> str:
    if language != "English":
        return value
    if value in UI_EN:
        return UI_EN[value]
    for prefix, translated in DIAGNOSTIC_PREFIXES_EN.items():
        if value.startswith(prefix):
            return translated + value[len(prefix):]
    # A version number embedded in a static UI caption is not a translation
    # key; replacing it temporarily makes the label stable across releases.
    import re
    normalized = re.sub(r"(?<![\d])\d+\.\d+\.\d+(?![\d])", "{version}", value)
    if normalized in UI_EN:
        match = re.search(r"\d+\.\d+\.\d+", value)
        if match:
            return UI_EN[normalized].format(version=match.group())
    return value


def tr_log(raw: str, language: str = "简体中文") -> str:
    # Display-only translation. Preserve the original diagnostic text on disk.
    if language == "English":
        return "\n".join(tr(line, language) for line in raw.split("\n"))
    if language == "简体中文":
        return "\n".join(
            "正在准备输出：" + line[len("Staging:"):] if line.startswith("Staging:")
            else line
            for line in raw.split("\n")
        )
    return raw
