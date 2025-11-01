# Build Instructions for mType Language Server

## Quick Start

### Windows (Visual Studio)

```batch
@echo off
echo Building mType Language Server...

REM Option 1: Using Premake5 (Recommended)
cd languageserver
premake5 vs2022
echo.
echo Visual Studio solution generated!
echo Open languageserver/mType-LanguageServer.sln and build in Visual Studio
pause

REM Option 2: Using CMake
REM cd languageserver
REM mkdir build
REM cd build
REM cmake ..
REM cmake --build . --config Release
```

### Linux/macOS

```bash
#!/bin/bash
echo "Building mType Language Server..."

# Option 1: Using Premake5
cd languageserver
premake5 gmake2
make config=release

# Option 2: Using CMake
# cd languageserver
# mkdir -p build && cd build
# cmake ..
# cmake --build . --config Release

echo "Build complete!"
echo "Executable: languageserver/bin/Release-linux-x86_64/mtype-language-server"
```

## Prerequisites

### 1. C++17 Compiler

**Windows:**
- Visual Studio 2017 or later (with C++ workload)
- Or MinGW-w64

**Linux:**
- GCC 7+ or Clang 5+
```bash
# Ubuntu/Debian
sudo apt-get install build-essential

# Fedora/RHEL
sudo dnf install gcc-c++

# Arch
sudo pacman -S base-devel
```

**macOS:**
- Xcode Command Line Tools
```bash
xcode-select --install
```

### 2. nlohmann/json Library

This is the **only external dependency** required.

#### Option A: vcpkg (Recommended for Windows)

```batch
REM Install vcpkg if you haven't
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
bootstrap-vcpkg.bat
vcpkg integrate install

REM Install nlohmann-json
vcpkg install nlohmann-json:x64-windows
```

Then in CMakeLists.txt or premake5.lua, the paths will be automatically configured.

#### Option B: Package Manager

**Linux:**
```bash
# Ubuntu/Debian
sudo apt-get install nlohmann-json3-dev

# Fedora/RHEL
sudo dnf install json-devel

# Arch
sudo pacman -S nlohmann-json
```

**macOS:**
```bash
brew install nlohmann-json
```

#### Option C: Manual Installation (Header-Only)

1. Download `json.hpp` from https://github.com/nlohmann/json/releases/latest
2. Create directory: `languageserver/vendor/nlohmann/`
3. Place `json.hpp` there
4. Update include path in build config:

**premake5.lua:**
```lua
includedirs {
    "src",
    "../mType",
    "vendor/nlohmann"  -- Add this line
}
```

**CMakeLists.txt:**
```cmake
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../mType
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/nlohmann  # Add this line
)
```

### 3. Build Tool

**Premake5 (Recommended):**
- Download from: https://premake.github.io/download
- Or via package manager:
  ```bash
  # Windows (Scoop)
  scoop install premake

  # macOS
  brew install premake

  # Linux
  # Download binary from website
  ```

**CMake (Alternative):**
- Version 3.15 or later
- Download from: https://cmake.org/download/
- Or via package manager:
  ```bash
  # Windows
  winget install Kitware.CMake

  # macOS
  brew install cmake

  # Linux
  sudo apt-get install cmake  # Ubuntu/Debian
  sudo dnf install cmake      # Fedora/RHEL
  ```

## Build Steps

### Method 1: Premake5 (Recommended)

**Windows:**
```batch
cd languageserver
premake5 vs2022

REM Now open mType-LanguageServer.sln in Visual Studio
REM Build Solution (Ctrl+Shift+B)
REM Executable will be in: bin\Release-windows-x86_64\mtype-language-server.exe
```

**Linux:**
```bash
cd languageserver
premake5 gmake2
make config=release

# Executable will be in: bin/Release-linux-x86_64/mtype-language-server
```

**macOS:**
```bash
cd languageserver
premake5 xcode4

# Open mType-LanguageServer.xcworkspace in Xcode and build
# Or use xcodebuild:
xcodebuild -configuration Release

# Executable will be in: bin/Release-macosx-x86_64/mtype-language-server
```

### Method 2: CMake

**All Platforms:**
```bash
cd languageserver
mkdir build && cd build
cmake ..
cmake --build . --config Release

# Executable location:
# Windows: build/Release/mtype-language-server.exe
# Linux/macOS: build/mtype-language-server
```

## Installation

### Option 1: System-Wide Installation

**Linux/macOS:**
```bash
sudo cp bin/Release-*/mtype-language-server /usr/local/bin/

# Verify installation
which mtype-language-server
mtype-language-server --help  # Should show help (not yet implemented)
```

**Windows:**
```batch
REM Copy to a directory in PATH
copy bin\Release-windows-x86_64\mtype-language-server.exe "C:\Program Files\mType\"

REM Add C:\Program Files\mType to PATH environment variable
REM System Properties → Advanced → Environment Variables → PATH → Add
```

### Option 2: Local/Project Installation

```bash
# Copy to mType project bin directory
cp bin/Release-*/mtype-language-server ../bin/

# Or add languageserver/bin to PATH in your shell config
export PATH="$PATH:/path/to/mType/languageserver/bin/Release-linux-x86_64"
```

## Troubleshooting

### Issue: "nlohmann/json.hpp not found"

**Solution 1:** Install via package manager (see Prerequisites step 2)

**Solution 2:** Download manually and update include paths

**Solution 3:** Use vcpkg (Windows):
```batch
vcpkg install nlohmann-json:x64-windows
vcpkg integrate install
```

### Issue: "Lexer.hpp not found" or similar mType header issues

**Solution:** Ensure you're building from the `languageserver/` directory and that `../mType/` contains the mType core source files. The build system expects this directory structure:
```
mType/
├── mType/          # Core source
│   ├── lexer/
│   ├── parser/
│   └── ...
└── languageserver/ # Language server (build from here)
    ├── src/
    ├── main.cpp
    └── premake5.lua
```

### Issue: Build fails on Linux with "undefined reference to std::filesystem"

**Solution:** Link with `stdc++fs` on older GCC versions:

**premake5.lua:**
```lua
filter "system:linux"
    links { "stdc++fs" }
```

**CMakeLists.txt:**
```cmake
if(UNIX AND NOT APPLE)
    target_link_libraries(mtype-language-server stdc++fs)
endif()
```

### Issue: Build errors on Windows with MSVC

**Solution:** Ensure you have the latest MSVC compiler and Windows SDK:
- Open Visual Studio Installer
- Modify your installation
- Ensure "MSVC v142+" and "Windows 10/11 SDK" are checked

## Testing

After building, test the language server manually:

```bash
# The server communicates via stdin/stdout
echo '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{}}' | ./bin/Release-*/mtype-language-server

# Or let your editor start it automatically (recommended)
```

## Next Steps

After successful build:

1. **VS Code:** Configure the extension (see `languageserver/README.md`)
2. **Vim/Neovim:** Set up coc.nvim or nvim-lspconfig (see `configs/vim/mtype.vim`)
3. **Emacs:** Load mtype-mode.el (see `configs/emacs/mtype-mode.el`)

## Build Artifacts

After building, you'll find:

```
languageserver/
├── bin/
│   └── Release-<platform>-x86_64/
│       └── mtype-language-server[.exe]  # The executable
└── bin-int/                              # Intermediate files (can be deleted)
```

Clean build:
```bash
# Premake
make clean

# Or delete directories
rm -rf bin/ bin-int/ build/
```

## Development Build

For development with debugging symbols:

**Premake:**
```bash
premake5 gmake2  # or vs2022, xcode4
make config=debug
# Executable: bin/Debug-<platform>-x86_64/mtype-language-server
```

**CMake:**
```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
```

Debug symbols allow you to use gdb, lldb, or Visual Studio debugger to step through language server code.
