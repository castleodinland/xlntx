# third_party_effects_data_gen

基于 Excel 配置表生成游戏特效/效果数据的命令行工具。

- **语言**: C++17
- **编译器**: MSVC (Visual Studio 2022 toolchain)
- **构建系统**: CMake + Ninja
- **包管理**: vcpkg (x64-windows-static, 静态链接)
- **Release 输出**: 约 1.92 MB 独立 EXE, 仅依赖 KERNEL32.dll + USER32.dll

## 依赖项

| 库 | 用途 |
|---|---|
| [fmt](https://github.com/fmtlib/fmt) | 字符串格式化 (header-only) |
| [xlntx](xlntx/) | 自研 Excel .xlsx 只读库，替代 xlnt，容错性更强 |
| [Boost](https://www.boost.org/) | program_options, filesystem, system, regex, range |

---

## xlntx 库

xlntx 是一个自研的 C++17 静态库，提供只读 Excel `.xlsx` 解析功能。它是 [xlnt](https://github.com/tfussell/xlnt) 的平替（API 兼容），核心特点：

- **自定义 XML 解析器**：不依赖 libstudxml，对 WPS / Office 不同版本的 xlsx 格式差异容错性更强
- **极简依赖**：仅依赖 miniz (单文件 C 库，内置)，无其他第三方依赖
- **API 兼容**: 公开头文件接口与 xlnt 一致，只需替换 `#include` 和命名空间即可迁移

### 库目录结构

```
xlntx/
├── CMakeLists.txt
├── include/xlntx/          # 公开头文件
│   ├── xlntx.hpp           # ★ 聚合单头文件 (对外发布只需此文件)
│   ├── xlntx_config.hpp
│   ├── cell/               # cell, cell_reference, index_types, rich_text...
│   ├── worksheet/           # worksheet, range, range_reference, cell_vector...
│   ├── workbook/            # workbook theme, named_range...
│   ├── styles/              # font, fill, border, alignment, format, style...
│   ├── utils/               # datetime, variant, path, exceptions, optional...
│   ├── packaging/           # uri, relationship, manifest
│   └── drawing/             # spreadsheet_drawing
├── source/                  # 实现源文件
│   ├── detail/              # xml_parser, zip_reader, xlsx_reader, *_impl
│   └── *.cpp
└── third-party/miniz/       # miniz ZIP 解压库 (内置)
    ├── miniz.h
    └── miniz.c
```

> **聚合头文件**: `include/xlntx/xlntx.hpp` 是一个自包含的单文件头，合并了所有公开 API 声明。对外提供时，只需这个文件 + `xlntx.lib` 即可。

### 编译 xlntx.lib (独立编译，无外部依赖)

xlntx 自身不依赖 vcpkg 管理的任何包。只需 MSVC + miniz 即可编译。

```powershell
# 1. 配置 MSVC 环境
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

# 2. 用 CMake 编译 xlntx 静态库
$cmake = "D:\dev\vcpkg\downloads\tools\cmake-3.29.2-windows\cmake-3.29.2-windows-i386\bin\cmake.exe"
& $cmake -S xlntx -B xlntx_build
& $cmake --build xlntx_build --config Release

# 输出文件: xlntx_build/Release/xlntx.lib
```

### 迁移到内网环境 (离线使用)

**推荐方式 — 复制 xlntx 源码目录，用 `add_subdirectory` 编译：**

> **注意**: C++ 静态库与 MSVC 版本绑定。预编译的 `xlntx.lib` 只能在**相同 MSVC 版本**的机器上链接（否则会出现 `__std_find_last_trivial_1` 等符号错误）。跨机器迁移推荐从源码编译。

**需要的文件：**

```
xlntx/                              # 整个目录复制到内网项目根目录
├── CMakeLists.txt
├── include/xlntx/
│   ├── xlntx.hpp                   # 聚合单头文件 (对外只需这一个)
│   ├── xlntx_config.hpp
│   ├── cell/                       # 单独头文件 (内部编译用)
│   ├── worksheet/
│   ├── workbook/
│   ├── styles/
│   ├── utils/
│   ├── packaging/
│   └── drawing/
├── source/                         # 实现源文件
│   └── detail/
└── third-party/miniz/              # miniz.h + miniz.c
```

**方式 A — CMake `add_subdirectory` (推荐)：**

```cmake
# 在你的 CMakeLists.txt 中
add_subdirectory(xlntx)
target_link_libraries(your_target PRIVATE xlntx)
```

xlntx 会自动定义 `XLNTX_STATIC=1`、设置 C++17，并开启 Release 体积优化 (`/O1 /Gy`)。

在你的代码中：

```cpp
#include "xlntx/xlntx.hpp"           // 聚合头文件
namespace xlnt = xlntx;              // 可选: 一行兼容旧 xlnt 代码
```

**方式 B — 直接链接预编译 .lib（仅限 MSVC 版本相同的情况）：**

如果内外网 MSVC 版本完全一致，可以只用 2 个文件：

```
xlntx.lib                           # 预编译产物
xlntx/include/xlntx/xlntx.hpp       # 聚合单头文件
```

```cmake
target_include_directories(your_target PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(your_target PRIVATE ${CMAKE_SOURCE_DIR}/xlntx.lib)
target_compile_definitions(your_target PRIVATE XLNTX_STATIC=1)
target_compile_features(your_target PRIVATE cxx_std_17)
```

```cpp
#include "xlntx/xlntx.hpp"
```

> **警告**: 若编译时出现 `LNK2019: 无法解析的外部符号 __std_find_last_trivial_1` 等错误，说明 MSVC 版本不匹配，必须改用方式 A。

**方式 C — 源文件直接编译 (无 CMake)：**

将以下文件加入你的构建系统：

```
xlntx/source/*.cpp
xlntx/source/detail/*.cpp
xlntx/third-party/miniz/miniz.c
```

编译时设置：
- `C++17` (或更高)
- 头文件路径：`xlntx/include`, `xlntx/source`, `xlntx/third-party/miniz`
- 宏定义：`XLNTX_STATIC=1`
- MSVC 额外宏：`_CRT_SECURE_NO_WARNINGS=1`

**xlnt → xlntx 命名空间兼容技巧：**

如果想保持旧代码中 `xlnt::` 前缀不变，在每个 .cpp 文件 include 之后加一行：

```cpp
#include "xlntx/xlntx.hpp"
namespace xlnt = xlntx;              // 此行之后所有 xlnt:: 自动指向 xlntx
```

切回原始 xlnt 时，改回 `#include "xlnt/xlnt.hpp"` 并删掉 namespace alias 即可。

### 从 xlnt 迁移到 xlntx

迁移非常简单，只需两步：

```cpp
// 修改前 (xlnt)
#include "xlnt/xlnt.hpp"
xlnt::workbook wb;
wb.load("file.xlsx");
xlnt::worksheet ws = wb.sheet_by_index(0);

// 修改后 (xlntx)
#include "xlntx/xlntx.hpp"
xlntx::workbook wb;
wb.load("file.xlsx");
xlntx::worksheet ws = wb.sheet_by_index(0);
```

公开 API 完全兼容（相同的方法签名、类名、类型名），注意以下细微差异：

| 差异点 | xlnt | xlntx |
|--------|------|-------|
| `column_t` 构造 | 允许隐式 `int` → `column_t` | 同样允许（已适配） |
| `cell_reference` 字符串构造 | 允许隐式转换 | 同样允许（已适配） |
| streaming reader/writer | 支持写入 | 仅读取，写入为 stub |
| 样式/格式修改 | 完整支持 | stub 实现（只读） |

---

## 环境搭建 (干净电脑从零开始)

以下按安装顺序描述。

### 1. Visual Studio Code

下载安装 [VS Code](https://code.visualstudio.com/)，然后安装扩展：

```
Ctrl+Shift+X → 搜索以下扩展并安装:
  - CMake Tools  (ms-vscode.cmake-tools)
  - C/C++        (ms-vscode.cpptools)
```

### 2. MSVC 编译工具链 + Windows SDK

**方式 A — Visual Studio 2022 BuildTools（推荐）**

1. 下载 [Visual Studio 2022 BuildTools](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022)
2. 运行安装程序，在 **"工作负荷"** 选项卡中勾选 **"使用 C++ 的桌面开发"**
3. 在 **"单个组件"** 选项卡中确认以下组件被勾选：
   - MSVC v143 - VS 2022 C++ x64/x86 生成工具
   - Windows 10 SDK (或 Windows 11 SDK)
   - CMake 的 C++ 工具 (可选，我们会用 vcpkg 自带的 cmake)

安装完成后，编译器目录为：
```
C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\<version>\
```

**方式 B — 独立 MSVC 工具链（轻量，但需要手动配置环境）**

```powershell
# 通过 winget 安装
winget install Microsoft.VisualStudio.2022.BuildTools --override "--add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 --add Microsoft.VisualStudio.Component.Windows10SDK.20348"
```

安装完成后，编译器可能位于 `C:\Program\VC\Tools\MSVC\<version>\` 或标准 BuildTools 路径。

**关键文件（两种方式都会包含）**：
- `cl.exe` — C/C++ 编译器
- `link.exe` — 链接器
- `vcvars64.bat` — 环境配置脚本 (设置 PATH/INCLUDE/LIB)

### 3. Ninja 构建工具

下载预编译二进制，放入固定路径：

```powershell
# 创建目录
New-Item -ItemType Directory -Force -Path D:\dev\ninja

# 下载 ninja-win.zip (以 v1.12.1 为例)
Invoke-WebRequest -Uri "https://github.com/ninja-build/ninja/releases/download/v1.12.1/ninja-win.zip" -OutFile "$env:TEMP\ninja-win.zip"

# 解压 ninja.exe 到 D:\dev\ninja
Expand-Archive "$env:TEMP\ninja-win.zip" -DestinationPath D:\dev\ninja
```

验证安装：
```powershell
D:\dev\ninja\ninja.exe --version
# 输出: 1.12.1
```

### 4. vcpkg 包管理器

```powershell
# 克隆 vcpkg 到 D:\dev\vcpkg
git clone https://github.com/microsoft/vcpkg.git D:\dev\vcpkg

# 进入 vcpkg 目录并运行 bootstrap
cd D:\dev\vcpkg
.\bootstrap-vcpkg.bat
```

> **说明**: vcpkg 会下载自己的 cmake (放在 `downloads/tools/cmake-*/bin/cmake.exe`)，无需单独安装 cmake。

### 5. 安装 C++ 依赖库

```powershell
cd D:\dev\vcpkg
```

安装第一个包时，vcpkg 可能会尝试下载 `msys2` 的 `pkgconf` 包用于生成 `.pc` 文件。由于所有 msys2 镜像都返回 404（旧版本已下架），**需要提前修改 vcpkg 的 portfile**。

#### 5.1 修复 vcpkg portfile

修改两个文件，注释掉 `vcpkg_fixup_pkgconfig()` 行：

**文件 1**: `D:\dev\vcpkg\ports\fmt\portfile.cmake`

找到这一行：
```cmake
vcpkg_fixup_pkgconfig()
```
改为：
```cmake
# vcpkg_fixup_pkgconfig() # disabled: msys2 download fails due to stale package versions
```

**文件 2**: `D:\dev\vcpkg\ports\xlnt\portfile.cmake`

同上操作。

#### 5.2 安装包

```powershell
# 安装 fmt (header-only)
.\vcpkg.exe install fmt:x64-windows-static

# 安装 xlnt (Excel 读写库, v1.5.0)
.\vcpkg.exe install xlnt:x64-windows-static

# 安装 Boost 组件
.\vcpkg.exe install boost-program-options:x64-windows-static
.\vcpkg.exe install boost-filesystem:x64-windows-static
.\vcpkg.exe install boost-system:x64-windows-static
.\vcpkg.exe install boost-regex:x64-windows-static
.\vcpkg.exe install boost-range:x64-windows-static
# 以上会自动安装依赖: boost-container, boost-atomic, boost-assert, 等
```

### 6. 检出项目代码

```powershell
git clone <repo-url> <project-dir>
cd <project-dir>
```

---

## VS Code 配置说明

本项目 `.vscode/` 目录包含三个配置文件：

### settings.json

```jsonc
{
    // cmake 可执行文件 (使用 vcpkg 自带的 cmake)
    "cmake.cmakePath": "D:/dev/vcpkg/downloads/tools/cmake-3.29.2-windows/.../bin/cmake.exe",

    // 使用 Ninja 生成器 (单配置, 速度快)
    "cmake.generator": "Ninja",

    // 传递给 cmake 的参数
    "cmake.configureArgs": [
        "-DCMAKE_TOOLCHAIN_FILE=D:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake",
        "-DVCPKG_TARGET_TRIPLET=x64-windows-static"
    ],

    // 构建输出目录
    "cmake.buildDirectory": "${workspaceFolder}/build_out",

    // MSVC 环境变量 (PATH/INCLUDE/LIB)
    "cmake.environment": { ... }
}
```

### cmake-kits.json

手动定义 CMake 工具包，告诉 CMake Tools 编译器在哪里：

```jsonc
[{
    "name": "MSVC 14.44 x64 Static",
    "compilers": {
        "C": "C:/Program/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/cl.exe",
        "CXX": "C:/Program/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/cl.exe"
    },
    "preferredGenerator": { "name": "Ninja" },
    "environmentVariables": { ... }
}]
```

> **迁移到新电脑时**: 如果 MSVC 安装路径不同，需要更新 `cmake-kits.json` 和 `settings.json` 中 `cmake.environment` 的 PATH / INCLUDE / LIB 路径。如果你是用 VS 2022 BuildTools 标准安装的，可以使用 `vcvars64.bat` 来获取正确的路径值。

---

## 编译

### 通过 VS Code 界面

1. 用 VS Code 打开项目文件夹
2. 按 `Ctrl+Shift+P` → 执行 `CMake: Select Kit` → 选择 `MSVC 14.44 x64 Static`
3. 按 `Ctrl+Shift+P` → 执行 `CMake: Select Variant` → 选择 `Debug` 或 `Release`
4. 点击底部状态栏的 **Build** 按钮，或按 `F7`

### 通过命令行

```powershell
# --- 配置 MSVC 环境 ---
# 方式 A: 用 vcvars64.bat (标准 BuildTools 安装)
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

# 方式 B: 手动设置 (独立工具链安装)
$msvc = "C:\Program\VC\Tools\MSVC\14.44.35207"
$sdk  = "C:\Program Files (x86)\Windows Kits\10"
$ver  = "10.0.26100.0"
$env:PATH    = "$msvc\bin\Hostx64\x64;$sdk\bin\$ver\x64;D:\dev\ninja;$env:PATH"
$env:INCLUDE = "$msvc\include;$sdk\Include\$ver\ucrt;$sdk\Include\$ver\um;$sdk\Include\$ver\shared"
$env:LIB     = "$msvc\lib\x64;$sdk\Lib\$ver\ucrt\x64;$sdk\Lib\$ver\um\x64"

# --- 配置 cmake (Release) ---
$cmake = "D:\dev\vcpkg\downloads\tools\cmake-3.29.2-windows\cmake-3.29.2-windows-i386\bin\cmake.exe"
& $cmake -S . -B build_out -G Ninja `
    "-DCMAKE_TOOLCHAIN_FILE=D:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake" `
    "-DVCPKG_TARGET_TRIPLET=x64-windows-static" `
    "-DCMAKE_BUILD_TYPE=Release"

# --- 编译 ---
& $cmake --build build_out --config Release
```

### Debug 版本

Debug 使用 `/MTd` (静态调试 CRT)，EXE 约 8 MB：

```powershell
& $cmake -S . -B build_out_debug -G Ninja `
    ... `
    "-DCMAKE_BUILD_TYPE=Debug"
& $cmake --build build_out_debug --config Debug
```

---

## 目录结构

```
项目根目录/
├── .vscode/
│   ├── settings.json          # CMake Tools 配置
│   └── cmake-kits.json        # 编译器工具包定义
├── CMakeLists.txt             # cmake 构建脚本
├── EffectInfo.h               # 特效信息结构体定义
├── EffectInfo.cpp             # 特效二进制文件解析
├── third_party_effects_data_gen.cpp  # 主程序入口
├── build_out/                 # 构建输出目录 (gitignore)
│   └── third_party_effects_data_gen.exe
└── README.md
```

---

## 常见问题

### Q: CMake 报错 `MSVC 工具集版本不兼容`

设置为 Ninja 生成器而非 Visual Studio 生成器。Visual Studio 生成器依赖 MSBuild，当系统中存在多个 VS 版本时容易混乱。

### Q: 链接时找不到 `powershell.exe`

`settings.json` 中 `cmake.environment` 的 PATH 需要包含 `C:\Windows\System32\WindowsPowerShell\v1.0`。vcpkg 在链接后需要调用的 `applocal.ps1` 脚本。

### Q: vcpkg install 失败 (msys2 下载 404)

需要手动注释掉对应 portfile 中的 `vcpkg_fixup_pkgconfig()` 行。参见上文 5.1 节。

### Q: 编译时警告 `C4828: 文件包含...无效字符`

源文件中包含了中文/Unicode 注释字符，在当前代码页 (65001) 下有部分字节无法识别。这些是警告，不影响编译结果。
