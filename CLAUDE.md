# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

CPyMO is a C-language reimplementation of the PyMO visual novel game engine, designed for cross-platform deployment. It supports 15+ platforms including Windows, Linux, macOS, Nintendo 3DS, Switch, PSP, PS Vita, Wii, iOS, Android, and UWP.

## Architecture

### Core Engine (`cpymo/`)
- Platform-agnostic game engine logic
- Script interpreter, audio system, video playback, save/load
- No platform-specific dependencies

### Backend Interface (`cpymo-backends/include/`)
- Abstract interfaces for graphics, audio, input, video, etc.
- Each backend implements these interfaces for a specific platform
- Key headers: `cpymo_backend_image.h`, `cpymo_backend_audio.h`, `cpymo_backend_input.h`, etc.

### Platform Backends (`cpymo-backends/`)
- `sdl2/` - Primary SDL2 backend for desktop platforms
- `sdl1/` - Legacy SDL1.2 backend
- `3ds/` - Nintendo 3DS backend
- `android/` - Android backend
- `ios/` - iOS backend
- `uwp/` - Universal Windows Platform backend
- `software/` - Software renderer
- `ascii-art/` - ASCII art renderer (console)
- `text/` - Text-only version (console)

### Development Tools (`cpymo-tool/`)
- Package creation/extraction
- Platform conversion utilities

## Common Build Commands

### Desktop Platforms (SDL2 Backend)

#### Using CMake (Recommended for Desktop)
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

#### Using GNU Make
```bash
cd cpymo-backends/sdl2
make -j

# Debug build
DEBUG=1 make -j

# Disable audio/video
DISABLE_AUDIO=1 make -j
```

#### Windows NMake
```bash
cd cpymo-backends/sdl2
nmake -f Makefile.Win32
```

### Nintendo Platforms

#### 3DS
```bash
cd cpymo-backends/3ds
make              # Build 3dsx
make cia          # Build CIA package
make run          # Run in Citra emulator
```

#### Switch
```bash
cd cpymo-backends/sdl2
make -j -f Makefile.Switch
make run -j -f Makefile.Switch  # Run in yuzu emulator
```

#### Wii
```bash
cd cpymo-backends/sdl1
make -f Makefile.Wii
```

### Sony Platforms

#### PSP
```bash
cd cpymo-backends/sdl2
make -f Makefile.PSP
```

#### PS Vita
```bash
cd cpymo-backends/sdl2
make -f Makefile.PSV
```

### Mobile Platforms

#### Android
```bash
# Using Gradle
./gradlew assembleRelease

# Or open in Android Studio
```

#### iOS
- Open `cpymo-backends/ios/` in Xcode
- Or use CMake with iOS toolchain

### Universal Windows Platform (UWP)
- Open `cpymo-backends/uwp/` in Visual Studio
- Or use CMake with UWP toolchain

### Web (Emscripten)
```bash
cd cpymo-backends/sdl2

# Build with default settings (SDL2_mixer audio backend, no video)
make -j -f Makefile.Emscripten

# Build with FFmpeg audio/video backend (requires building FFmpeg first)
./build-emscripten-ffmpeg.sh
export AUDIO_BACKEND=ffmpeg
make -j -f Makefile.Emscripten

# Build without audio/video
export AUDIO_BACKEND=none
make -j -f Makefile.Emscripten

# Run with emrun (if game integrated)
make run -j -f Makefile.Emscripten
```

Configure `Makefile.Emscripten`:
- `WASM=1` for WebAssembly, `WASM=0` for JavaScript
- `BUILD_GAME_DIR` to integrate a game directory

### Development Tool
```bash
cd cpymo-tool
make
```

## Environment Variables for Configuration

### Audio/Video Backend Selection
- `SDL2` - Path to SDL2 binaries (include/lib directories)
- `FFMPEG` - Path to FFmpeg binaries (enables video playback)
- `SDL2_MIXER` - Path to SDL2_mixer binaries (alternative audio)
- `USE_SDL2_MIXER=1` - Use SDL2_mixer instead of FFmpeg for audio
- `DISABLE_AUDIO=1` - Disable all audio/video support

### Build Options
- `DEBUG=1` - Enable debug symbols and gdb debugging
- `LEAKCHECK=1` - Enable memory leak checking with stb_leakcheck
- `NO_CONSOLE=1` - Disable console window (Windows only)
- `RC_FILE` - Custom resource file for icons (Windows only)
- `TARGET` - Output executable name
- `CPYMO_MAX_SAVES` - Maximum number of save slots
- `CPYMO_LANG` - Default language

### Accessibility Features
- `ENABLE_TEXT_EXTRACT_COPY_TO_CLIPBOARD=1` - Export game text to clipboard for visually impaired players
- `ENABLE_EXIT_CONFIRM=1` - Prompt confirmation on exit

### Performance Tuning
- `DISABLE_VSYNC=1` - Disable vertical sync for maximum framerate

## Development Workflow

### Cloning with Submodules
```bash
git clone --recursive https://github.com/Strrationalism/CPyMO.git
```

If the repository was cloned without `--recursive`, initialize submodules:
```bash
git submodule update --init --recursive
```

### Dependencies Management
- Desktop: Use vcpkg (`vcpkg install sdl2 ffmpeg`)
- 3DS: Requires devkitPro + 3ds dev, FFmpeg 5.0 (build with `./build-3ds-ffmpeg.sh`)
- Switch: devkitPro + switch dev, switch-sdl2, switch-ffmpeg
- PSP: pspsdk, SDL2, FFmpeg (build with `./build-psp-ffmpeg.sh`)
- Emscripten: Requires emscripten SDK, FFmpeg optional (build with `./build-emscripten-ffmpeg.sh`)
- Wii: devkitPro + wii dev, wii-sdl, wii-sdl_mixer, wii-sdl_image
- PS Vita: VitaSDK, SDL2, SDL2_mixer
- Android: Android SDK/NDK, FFmpeg (build with `./build-android-ffmpeg.sh`)
- iOS: Xcode, iOS SDK
- UWP: Visual Studio, Windows SDK, vcpkg with UWP packages

### Testing
- No formal unit test suite
- Manual testing via game execution
- CI builds 15+ platforms (see `.github/workflows/ci.yml`)
- For desktop: Build and copy executable to game directory, run with game assets

### Game Assets Preparation
- Use `cpymo-tool` for package creation/conversion
- Platform-specific asset optimization guidelines in README.md

## Platform-Specific Notes

### Desktop (Windows/Linux/macOS)
- Full feature set: video playback, system font loading, accessibility features
- Alt+Enter toggles fullscreen
- Visual Studio integration via CMakeSettings.json

### Nintendo 3DS
- Requires DSP firmware dump for audio
- Games placed in `/pymogames/` on SD card
- 3D display support, screen mode switching with SELECT
- Debug mode: Hold L while selecting game

### Sony PSP
- No video playback support
- EBOOT.PBP output format

### Text/ASCII Backends
- Console-only versions for POSIX systems
- Useful for debugging and low-resource environments

## Tool Paths (Windows/MSYS2 Environment)

This repository uses MSYS2 environment on Windows 11. The PATH environment variable may be incorrect for common development tools. Use absolute paths:

- **git**: `C:\msys64\bin\git.exe`
- **dotnet**: `C:\Program Files\dotnet\dotnet.exe`

For other tools (cmake, make, etc.), verify they are in PATH or use absolute paths. The MSYS2 installation typically provides these tools at `C:\msys64\usr\bin\`.

## Important Files

- `README.md` - Extensive documentation (859 lines) covering all platforms
- `CMakeLists.txt` - Root CMake configuration
- `.github/workflows/ci.yml` - CI pipeline for 15+ platforms
- `cpymo-backends/sdl2/Makefile` - Primary Makefile for SDL2 backend
- `cpymo-backends/include/` - Backend interface definitions
- `cpymo/` - Core engine implementation

## Common Issues

- **Missing dependencies**: Ensure SDL2 and FFmpeg are installed or set in environment variables
- **No audio on 3DS**: Requires DSP firmware dump (install DSP1 and dump)
- **Game compatibility**: Some games require MO2PyMO patch (see README)
- **Font rendering**: Some platforms require external `default.ttf` in game directory

## Contribution Guidelines

- Follow existing code style (C with no C++ features)
- Maintain platform-agnostic core
- Implement backend interfaces for new platforms
- Update CI configuration for new platform builds
- Document platform-specific requirements in README.md