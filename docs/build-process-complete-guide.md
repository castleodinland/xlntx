# 工程完整编译流程详解

## 1. 触发：VS Code 状态栏 "Build" 按钮

VS Code 左下角状态栏的 **Build** 按钮由 **CMake Tools 扩展** (ms-vscode.cmake-tools) 提供。

按下后，CMake Tools 扩展执行以下两个阶段：
1. **Configure（配置）** — 使用 CMake 读取 CMakeLists.txt 并生成 Ninja 构建文件
2. **Build（构建）** — 调用 Ninja 执行实际编译和链接

> 如果 CMake Configure 已经运行过且 CMakeLists.txt 未变更，则跳过 Configure，直接执行 Build。

---

## 2. 影响的配置文件（按优先级从高到低）

### 2.1 `.vscode/settings.json` — 工作区 CMake 配置

路径：`.vscode/settings.json`

这是 CMake Tools 扩展的直接配置源，控制所有 CMake 参数：

| 配置项 | 值 | 作用 |
|---|---|---|
| `cmake.cmakePath` | `D:/dev/vcpkg/downloads/tools/cmake-3.29.2-windows/.../cmake.exe` | 指定 CMake 可执行文件（vcpkg 内嵌版本 3.29.2） |
| `cmake.generator` | `"Ninja"` | 指定构建系统生成器为 Ninja |
| `cmake.configureArgs` | `-DCMAKE_TOOLCHAIN_FILE=.../vcpkg.cmake` | 传入 CMake 的命令行参数，指定 vcpkg 工具链 |
|  | `-DVCPKG_TARGET_TRIPLET=x64-windows-static` | 指定 vcpkg 目标三元组为 x64-windows-static |
| `cmake.buildDirectory` | `${workspaceFolder}/build_out` | 构建输出目录 |
| `cmake.environment` | `PATH`, `INCLUDE`, `LIB` | 传递给 CMake 的环境变量，手动指定 MSVC 工具链路径 |

### 2.2 `.vscode/cmake-kits.json` — CMake Kit 定义

路径：`.vscode/cmake-kits.json`

定义了名为 **"MSVC 14.44 x64 Static"** 的编译工具包：

| 配置项 | 值 |
|---|---|
| `compilers.C` | `C:/Program/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/cl.exe` |
| `compilers.CXX` | `C:/Program/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/cl.exe` |
| `preferredGenerator.name` | `"Ninja"` |
| `environmentVariables.PATH` | MSVC bin + Windows Kits bin + Ninja + 系统 PATH |
| `environmentVariables.INCLUDE` | MSVC include + UCRT include + UM include + Shared include |
| `environmentVariables.LIB` | MSVC lib + UCRT lib + UM lib |

### 2.3 `CMakeLists.txt`（根）— 项目构建定义

路径：`CMakeLists.txt`

- 项目名：`third_party_effects_data_gen`
- 要求 CMake >= 3.15.0
- 设置 MSVC 运行时库为静态链接 (`/MT` / `MTd`)
- 定义两个可执行目标和一个子目录

### 2.4 `xlntx/CMakeLists.txt` — xlntx 静态库定义

路径：`xlntx/CMakeLists.txt`

- 定义 `xlntx` 静态库（14 个源文件：13 个 C++ + 1 个 C）
- 设置头文件路径、C++17 标准、静态库导出宏 `XLNTX_STATIC`

### 2.5 `D:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake` — vcpkg 工具链

CMake 配置时通过 `-DCMAKE_TOOLCHAIN_FILE` 加载，提供：

- 解析 `VCPKG_TARGET_TRIPLET` = `x64-windows-static`
- 从 `D:/dev/vcpkg/triplets/x64-windows-static.cmake` 加载三元组定义（架构=x64, CRT=static, 库链接=static）
- 设置 `VCPKG_INSTALLED_DIR` = `D:/dev/vcpkg/installed`（经典模式，无 manifest）
- 将 vcpkg 安装路径注入 `CMAKE_PREFIX_PATH`，使 `find_package` 能找到 vcpkg 安装的库
- 重载 `find_package` 宏，增加 vcpkg-cmake-wrapper 支持
- 重载 `add_executable`，添加 POST_BUILD 步骤（applocal.ps1 复制依赖 DLL，静态构建下实际为空操作）

### 2.6 `D:/dev/vcpkg/triplets/x64-windows-static.cmake` — vcpkg 三元组

```cmake
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)
```

### 2.7 vcpkg 安装的包的 CMake 配置文件

#### fmt (header-only)
- `D:/dev/vcpkg/installed/x64-windows-static/share/fmt/fmt-config.cmake`
- `D:/dev/vcpkg/installed/x64-windows-static/share/fmt/fmt-targets.cmake`
- 提供 IMPORTED INTERFACE 目标 `fmt::fmt-header-only`（无实际库文件，纯头文件）
- 定义 `FMT_HEADER_ONLY=1` 宏，设置 `/utf-8` 编译选项

#### xlnt (静态库)
- `D:/dev/vcpkg/installed/x64-windows-static/share/xlnt/XlntConfig.cmake`
- `D:/dev/vcpkg/installed/x64-windows-static/share/xlnt/XlntTargets.cmake`
- 提供 IMPORTED STATIC 目标 `xlnt::xlnt`
- 定义 `XLNT_STATIC=1` 宏
- 实际库文件路径：`D:/dev/vcpkg/installed/x64-windows-static/lib/xlnt.lib`

---

## 3. 完整编译流程（分步详解）

### 阶段一：CMake Configure（配置）

CMake Tools 扩展调用 CMake 执行配置，检测编译器和环境：

```bash
cmake.exe \
  -G "Ninja" \
  -DCMAKE_TOOLCHAIN_FILE=D:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_TARGET_TRIPLET=x64-windows-static \
  -DCMAKE_BUILD_TYPE=MinSizeRel \
  -S D:/mvs_work/script_cpp/cpp_xlnt_new \
  -B D:/mvs_work/script_cpp/cpp_xlnt_new/build_out
```

**步骤 1：加载 vcpkg 工具链**

CMake 首先处理 `-DCMAKE_TOOLCHAIN_FILE`，加载 `vcpkg.cmake`。该文件执行：

- 设置 `VCPKG_TARGET_TRIPLET = x64-windows-static`（从命令行参数）
- 定位 `VCPKG_ROOT_DIR = D:/dev/vcpkg`（通过向上查找 `.vcpkg-root` 标记文件）
- 设置 `VCPKG_INSTALLED_DIR = D:/dev/vcpkg/installed`（经典模式）
- 将 `D:/dev/vcpkg/installed/x64-windows-static` 和 `.../debug` 路径注入 `CMAKE_PREFIX_PATH`、`CMAKE_LIBRARY_PATH`、`CMAKE_FIND_ROOT_PATH`
- 重载 `find_package`、`add_executable`、`add_library` 函数

**步骤 2：编译器检测**

CMake 使用 cmake-kits.json 中指定的编译器进行检测：

- 检测 C 编译器：`C:/Program/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/cl.exe`
  - 编译测试程序，验证编译器工作正常
  - 确定编译器 ID：`MSVC`，版本：`19.44.35227.0`
  - 确定目标架构：`x64`
- 检测 C++ 编译器：同上 `cl.exe`
  - 确定 C++ 标准默认级别：`14`
  - 确定支持的 C++ 特性：`cxx_std_17`, `cxx_std_20`, `cxx_std_23`

**此阶段生成的文件：**

| 文件 | 说明 |
|---|---|
| `build_out/CMakeCache.txt` | CMake 缓存，存储所有检测结果和配置变量 |
| `build_out/CMakeFiles/3.29.2/CMakeSystem.cmake` | 系统信息（平台、处理器） |
| `build_out/CMakeFiles/3.29.2/CMakeCCompiler.cmake` | C 编译器检测结果 |
| `build_out/CMakeFiles/3.29.2/CMakeCXXCompiler.cmake` | C++ 编译器检测结果 |
| `build_out/CMakeFiles/3.29.2/CMakeRCCompiler.cmake` | RC 资源编译器信息 |

**步骤 3：处理 CMakeLists.txt**

CMake 解析根 `CMakeLists.txt`：

1. `project(third_party_effects_data_gen VERSION 0.1.0)`
   - 设置项目名和版本
   - 启用 C 和 CXX 语言

2. `set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")`
   - Release/MinSizeRel 配置：`/MT`（静态链接 CRT）
   - Debug 配置：`/MTd`（静态链接调试 CRT）

3. `add_executable(third_party_effects_data_gen ...)`
   - 注册可执行目标，源文件：`EffectInfo.cpp`, `third_party_effects_data_gen_vs.cpp`, `resource.rc`

4. `find_package(fmt CONFIG REQUIRED)`
   - 搜索路径：`D:/dev/vcpkg/installed/x64-windows-static/share/fmt/`
   - 加载 `fmt-config.cmake` → `fmt-targets.cmake`
   - 成功导入目标 `fmt::fmt-header-only`（INTERFACE 库，纯头文件）
   - 由于是 header-only，无需链接任何 `.lib` 文件

5. `add_subdirectory(xlntx)`
   - 进入 `xlntx/` 子目录，处理 `xlntx/CMakeLists.txt`
   - `add_library(xlntx STATIC ...)` 注册静态库目标，14 个源文件
   - `set_source_files_properties(third-party/miniz/miniz.c PROPERTIES LANGUAGE C)` 标记 miniz.c 为 C 源文件

6. `target_link_libraries(third_party_effects_data_gen PRIVATE xlntx fmt::fmt-header-only)`
   - 链接关系：`third_party_effects_data_gen` 依赖 `xlntx` 和 `fmt::fmt-header-only`

7. `add_executable(test_xlntx ...)`
   - 注册测试可执行目标，源文件：`test_xlntx.cpp`

8. `find_package(xlnt CONFIG REQUIRED)`
   - 搜索路径：`D:/dev/vcpkg/installed/x64-windows-static/share/xlnt/`
   - 加载 `XlntConfig.cmake` → `XlntTargets.cmake`
   - 成功导入目标 `xlnt::xlnt`（STATIC IMPORTED 库）
   - 实际库文件：`D:/dev/vcpkg/installed/x64-windows-static/lib/xlnt.lib`

9. `target_link_libraries(test_xlntx PRIVATE xlntx xlnt::xlnt fmt::fmt-header-only)`
   - 链接关系：`test_xlntx` 依赖 `xlntx`, `xlnt::xlnt`, `fmt::fmt-header-only`

**有效的编译选项合并（以 MinSizeRel 为例）**

每个源文件编译时的实际选项由多层合并而来：

| 来源 | 选项 |
|---|---|
| CMake 默认 (MSVC) | `/DWIN32 /D_WINDOWS /GR /EHsc` |
| MinSizeRel 配置 | `/O1 /Ob1 /DNDEBUG` |
| `CMAKE_MSVC_RUNTIME_LIBRARY` | `/MT`（静态 CRT） |
| `target_compile_features(... cxx_std_17)` | `-std:c++17` |
| fmt::fmt-header-only (INTERFACE) | `-DFMT_HEADER_ONLY=1 /utf-8` |
| xlntx (INTERFACE) | `-DXLNTX_STATIC=1` |
| xlnt::xlnt (INTERFACE) | `-DXLNT_STATIC=1`（仅 test_xlntx 目标） |
| `target_compile_options` (根 CMakeLists) | `/GL /Gy`（third_party_effects_data_gen 目标） |
| `target_compile_options` (xlntx CMakeLists) | `_CRT_SECURE_NO_WARNINGS=1 /Gy`（xlntx 目标） |

### 阶段二：CMake Generate（生成）

配置完成后，CMake 使用 Ninja 生成器产生构建系统文件。

**此阶段生成的文件：**

| 文件 | 说明 |
|---|---|
| `build_out/build.ninja` | Ninja 主构建文件，包含所有编译和链接规则及依赖关系 |
| `build_out/CMakeFiles/rules.ninja` | Ninja 规则文件，定义具体的编译器/链接器命令模板 |
| `build_out/cmake_install.cmake` | CMake 安装脚本 |
| `build_out/xlntx/cmake_install.cmake` | xlntx 子项目的安装脚本 |
| `build_out/compile_commands.json` | 编译数据库（可供 clangd 等语言服务器使用） |

### 阶段三：Build（构建）

CMake Tools 调用 Ninja 执行构建：

```bash
D:/dev/ninja/ninja.exe -C D:/mvs_work/script_cpp/cpp_xlnt_new/build_out
```

Ninja 读取 `build.ninja`，按依赖关系拓扑排序，并行执行编译任务。

**构建目标依赖图：**

```
third_party_effects_data_gen.exe ──┬── xlntx.lib ──────┬── 12个C++源文件(.cpp) → .obj
                                   │                    └── miniz.c (C源文件) → .obj
                                   ├── EffectInfo.cpp → .obj
                                   ├── third_party_effects_data_gen_vs.cpp → .obj
                                   └── resource.rc → .res

test_xlntx.exe ──┬── xlntx.lib ──── (同上)
                 ├── test_xlntx.cpp → .obj
                 └── xlnt.lib (vcpkg预编译)
```

**步骤 3.1：编译 xlntx C++ 源文件（12 个 .cpp → .obj）**

编译命令模板（来自 `rules.ninja`）：

```
cl.exe /nologo /TP <DEFINES> <INCLUDES> <FLAGS> /showIncludes /Fo<out>.obj /Fd<...>.pdb /FS -c <in>.cpp
```

每个 xlntx C++ 源文件的编译选项：

```
/DWIN32 /D_WINDOWS /GR /EHsc          # CMake MSVC 默认
/O1 /Ob1 /DNDEBUG                      # MinSizeRel 优化
-std:c++17                             # C++17 标准
-MT                                    # 静态 CRT
-DXLNTX_STATIC=1                       # 静态库导出宏
-D_CRT_SECURE_NO_WARNINGS=1            # 抑制安全警告
/utf-8                                 # UTF-8 源文件编码（来自 fmt 的 INTERFACE 传递）

Include 路径：
  -I .../xlntx/include                 # PUBLIC 头文件
  -I .../xlntx/source                  # PRIVATE 内部头文件
  -I .../xlntx/third-party/miniz       # PRIVATE miniz 头文件
```

生成 12 个 `.obj` 文件，位于：
- `build_out/xlntx/CMakeFiles/xlntx.dir/source/*.obj`
- `build_out/xlntx/CMakeFiles/xlntx.dir/source/detail/*.obj`

**步骤 3.2：编译 xlntx C 源文件（1 个 .c → .obj）**

```
cl.exe /nologo <DEFINES> <INCLUDES> <FLAGS> /showIncludes /Fo<out>.obj /Fd<...>.pdb /FS -c miniz.c
```

注意：C 编译没有 `/TP` 标志（不强制 C++），没有 `-std:c++17`。

生成：
- `build_out/xlntx/CMakeFiles/xlntx.dir/third-party/miniz/miniz.c.obj`

**步骤 3.3：链接 xlntx 静态库（14 .obj → xlntx.lib）**

```
lib.exe /nologo /machine:x64 /out:xlntx\xlntx.lib <14个.obj文件>
```

使用 MSVC 库管理器 `lib.exe` 将所有目标文件打包为一个静态库。

生成：
- `build_out/xlntx/xlntx.lib`（约 5.7 MB）

**步骤 3.4：编译 third_party_effects_data_gen C++ 源文件（2 .cpp → .obj）**

编译选项（在 xlntx 基础上增加）：

```
-O1 /Ob1 /DNDEBUG                      # MinSizeRel
/GL                                    # 全程序优化（来自 target_compile_options）
/Gy                                    # 函数级链接（来自 target_compile_options）
-DFMT_HEADER_ONLY=1                    # 来自 fmt::fmt-header-only 的 INTERFACE 定义
-utf-8                                 # 来自 fmt::fmt-header-only 的 INTERFACE 选项

Include 路径：
  -I .../xlntx/include                 # 来自 xlntx PUBLIC
  -I D:/dev/vcpkg/installed/x64-windows-static/include  # 来自 fmt (vcpkg)
```

生成：
- `build_out/CMakeFiles/third_party_effects_data_gen.dir/EffectInfo.cpp.obj`
- `build_out/CMakeFiles/third_party_effects_data_gen.dir/third_party_effects_data_gen_vs.cpp.obj`

**步骤 3.5：编译资源文件（1 .rc → .res）**

使用 cmake 的 `cmcldeps.exe` 包装 RC 编译器：

```
cmcldeps.exe RC resource.rc <depfile> "注意: 包含文件:  " "cl.exe" \
  rc.exe -DWIN32 -I .../xlntx/include -I .../vcpkg/installed/x64-windows-static/include /fo resource.rc.res resource.rc
```

生成：
- `build_out/CMakeFiles/third_party_effects_data_gen.dir/resource.rc.res`

**步骤 3.6：链接 third_party_effects_data_gen.exe**

链接命令使用 CMake 的 `vs_link_exe` 脚本包装：

```
cmake.exe -E vs_link_exe \
  --intdir=CMakeFiles/third_party_effects_data_gen.dir \
  --rc=.../rc.exe --mt=.../mt.exe --manifests <manifests> \
  -- \
  link.exe /nologo \
    EffectInfo.cpp.obj third_party_effects_data_gen_vs.cpp.obj resource.rc.res \
    /out:third_party_effects_data_gen.exe \
    /implib:third_party_effects_data_gen.lib \
    /pdb:third_party_effects_data_gen.pdb /version:0.0 \
    /machine:x64 /INCREMENTAL:NO /subsystem:console \
    xlntx\xlntx.lib \
    kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib \
    ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib
```

注意：
- `/INCREMENTAL:NO`：Release 模式下关闭增量链接
- `/subsystem:console`：控制台应用程序
- 不链接 `xlnt.lib`（`third_party_effects_data_gen` 只依赖 `xlntx`，不依赖 `xlnt`）
- 不链接 `fmt` 库（header-only，无需链接）
- 最终 EXE 仅依赖 `kernel32.dll` 和 `user32.dll`（以及系统 DLL 的传递依赖）

生成：
- `build_out/third_party_effects_data_gen.exe`（约 681 KB）

**步骤 3.7：Post-Build（third_party_effects_data_gen）**

```
powershell.exe -noprofile -executionpolicy Bypass \
  -file D:/dev/vcpkg/scripts/buildsystems/msbuild/applocal.ps1 \
  -targetBinary .../third_party_effects_data_gen.exe \
  -installedDir D:/dev/vcpkg/installed/x64-windows-static/bin \
  -OutVariable out
```

在静态链接配置下，没有 DLL 需要复制，此步骤几乎为空操作。

**步骤 3.8：编译 test_xlntx.cpp → .obj**

编译选项包含：
```
-DXLNT_STATIC=1                        # 来自 xlnt::xlnt 的 INTERFACE 定义
/O1 /Ob1 /DNDEBUG                      # MinSizeRel（无 /GL，因 test_xlntx 目标未设置全程序优化）
```

生成：
- `build_out/CMakeFiles/test_xlntx.dir/test_xlntx.cpp.obj`

**步骤 3.9：链接 test_xlntx.exe**

与步骤 3.6 类似，但额外链接 `xlnt.lib`：

```
link.exe /nologo \
  test_xlntx.cpp.obj \
  /out:test_xlntx.exe \
  /machine:x64 /INCREMENTAL:NO /subsystem:console \
  xlntx\xlntx.lib \
  D:\dev\vcpkg\installed\x64-windows-static\lib\xlnt.lib \
  kernel32.lib user32.lib ... (其他系统库)
```

生成：
- `build_out/test_xlntx.exe`（约 1.6 MB，因同时链接了 xlntx 和 xlnt 两个库）

**步骤 3.10：Post-Build（test_xlntx）**

同步骤 3.7，对 `test_xlntx.exe` 执行 applocal.ps1。

---

## 4. 工具链全景

### 4.1 工具路径汇总

| 工具 | 路径 | 版本 |
|---|---|---|
| **CMake** | `D:/dev/vcpkg/downloads/tools/cmake-3.29.2-windows/.../bin/cmake.exe` | 3.29.2 |
| **Ninja** | `D:/dev/ninja/ninja.exe` | (系统安装) |
| **C/C++ 编译器 (cl.exe)** | `C:/Program/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/cl.exe` | 19.44.35227.0 (VS2022 14.44) |
| **链接器 (link.exe)** | `C:/Program/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/link.exe` | 14.44.35227.0 |
| **库管理器 (lib.exe)** | `C:/Program/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/lib.exe` | 14.44.35227.0 |
| **资源编译器 (rc.exe)** | `C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/rc.exe` | Windows SDK 10.0.26100.0 |
| **清单工具 (mt.exe)** | `C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/mt.exe` | Windows SDK 10.0.26100.0 |
| **vcpkg** | `D:/dev/vcpkg/vcpkg.exe` | (vcpkg 根目录) |

### 4.2 第三方依赖

| 依赖 | 类型 | 来源 | 提供的 CMake 目标 |
|---|---|---|---|
| **fmt** | header-only | vcpkg (`x64-windows-static`) | `fmt::fmt-header-only` |
| **xlnt** | 静态库 | vcpkg (`x64-windows-static`) | `xlnt::xlnt` → `xlnt.lib` |
| **miniz** | C 源码 (内置) | `xlntx/third-party/miniz/miniz.c` | 直接编译进 `xlntx.lib` |

### 4.3 编译/链接选项速查

| 选项 | 含义 | 来源 |
|---|---|---|
| `/O1` | 最小化代码大小 | MinSizeRel 配置 |
| `/Ob1` | 仅内联标记为 `inline` 的函数 | MinSizeRel 配置 |
| `/DNDEBUG` | 禁用 assert | MinSizeRel 配置 |
| `/MT` | 静态链接 MSVC 运行时 | `CMAKE_MSVC_RUNTIME_LIBRARY` |
| `-std:c++17` | C++17 标准 | `target_compile_features` |
| `/GL` | 全程序优化（LTCG） | `target_compile_options` (third_party_effects_data_gen) |
| `/Gy` | 函数级链接（允许链接器移除未引用函数） | `target_compile_options` |
| `/utf-8` | 源文件 UTF-8 编码 | fmt::fmt-header-only INTERFACE 选项 |
| `/GR` | 启用 RTTI | CMake MSVC 默认 |
| `/EHsc` | 启用 C++ 异常处理 | CMake MSVC 默认 |
| `/LTCG` | 链接时代码生成 | `target_link_options` (third_party_effects_data_gen) |
| `/OPT:REF` | 移除未引用代码/数据 | `target_link_options` |
| `/OPT:ICF` | 合并相同的函数 | `target_link_options` |
| `/INCREMENTAL:NO` | 禁用增量链接 | MinSizeRel 链接选项 |

---

## 5. 最终产物清单

### 清理后全量构建的输出

| 文件 | 大小（约） | 说明 |
|---|---|---|
| `build_out/third_party_effects_data_gen.exe` | 697 KB | 主程序独立 EXE，仅依赖 kernel32.dll + user32.dll |
| `build_out/test_xlntx.exe` | 1,689 KB | 测试程序，链接 xlntx + xlnt |
| `build_out/xlntx/xlntx.lib` | 5,841 KB | xlntx 静态库（含 miniz 代码） |
| `build_out/build.ninja` | 31 KB | Ninja 主构建文件 |
| `build_out/CMakeFiles/rules.ninja` | ~5 KB | Ninja 编译规则 |
| `build_out/CMakeCache.txt` | 18 KB | CMake 缓存 |
| `build_out/compile_commands.json` | 14 KB | 编译数据库 |
| `build_out/cmake_install.cmake` | 2 KB | 安装脚本 |
| `build_out/xlntx/cmake_install.cmake` | 1 KB | xlntx 安装脚本 |
| `build_out/.ninja_deps` | 88 KB | Ninja 依赖信息 |
| `build_out/.ninja_log` | 2 KB | Ninja 构建日志 |
| `build_out/CMakeFiles/3.29.2/CMakeSystem.cmake` | ~1 KB | 系统检测信息 |
| `build_out/CMakeFiles/3.29.2/CMakeCCompiler.cmake` | ~3 KB | C 编译器检测信息 |
| `build_out/CMakeFiles/3.29.2/CMakeCXXCompiler.cmake` | ~4 KB | C++ 编译器检测信息 |
| `build_out/CMakeFiles/3.29.2/CMakeRCCompiler.cmake` | ~1 KB | RC 编译器检测信息 |

### 中间产物（.obj 文件）

**xlntx 库目标文件**（位于 `build_out/xlntx/CMakeFiles/xlntx.dir/`）：

```
source/cell_reference.cpp.obj
source/index_types.cpp.obj
source/exceptions.cpp.obj
source/rich_text.cpp.obj
source/path.cpp.obj
source/cell.cpp.obj
source/worksheet.cpp.obj
source/workbook.cpp.obj
source/range.cpp.obj
source/range_reference.cpp.obj
source/detail/xml_parser.cpp.obj
source/detail/xlsx_reader.cpp.obj
source/detail/zip_reader.cpp.obj
third-party/miniz/miniz.c.obj
```

**主程序目标文件**（位于 `build_out/CMakeFiles/third_party_effects_data_gen.dir/`）：

```
EffectInfo.cpp.obj
third_party_effects_data_gen_vs.cpp.obj
resource.rc.res
```

**测试程序目标文件**（位于 `build_out/CMakeFiles/test_xlntx.dir/`）：

```
test_xlntx.cpp.obj
```

---

## 6. 流程图

```
用户按下 VS Code Build 按钮
        │
        ▼
┌─────────────────────────────────────────────────┐
│ CMake Tools 扩展读取 .vscode/settings.json       │
│   - cmake.cmakePath → CMake 可执行文件           │
│   - cmake.generator → Ninja                     │
│   - cmake.configureArgs → 工具链文件 + 三元组    │
│   - cmake.buildDirectory → build_out/           │
│   - cmake.environment → PATH, INCLUDE, LIB      │
└─────────────────────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────────────────────┐
│ 阶段一: CMake Configure                          │
│                                                   │
│ 1. 加载 vcpkg.cmake 工具链                        │
│    - 设置 VCPKG_TARGET_TRIPLET=x64-windows-static │
│    - 加载三元组文件 (架构x64, 静态CRT, 静态库)      │
│    - 设置 VCPKG_INSTALLED_DIR                    │
│    - 注入 vcpkg 路径到 CMAKE_PREFIX_PATH          │
│                                                   │
│ 2. 检测编译器 (cmake-kits.json)                   │
│    - C:   cl.exe (MSVC 19.44.35227.0)            │
│    - CXX: cl.exe (MSVC 19.44.35227.0)            │
│    生成: CMakeSystem/CCXXCompiler.cmake           │
│                                                   │
│ 3. 处理根 CMakeLists.txt                          │
│    - project(third_party_effects_data_gen)        │
│    - 设置 /MT (静态CRT)                            │
│    - add_executable(third_party_effects_data_gen) │
│    - find_package(fmt CONFIG) → fmt::fmt-header-only │
│    - add_subdirectory(xlntx)                      │
│      └─ add_library(xlntx STATIC ... 14源文件)    │
│    - add_executable(test_xlntx)                  │
│    - find_package(xlnt CONFIG) → xlnt::xlnt      │
│    - target_link_libraries(...)                  │
│    生成: CMakeCache.txt                           │
└─────────────────────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────────────────────┐
│ 阶段二: CMake Generate (Ninja 生成器)             │
│                                                   │
│ 生成文件:                                         │
│   - build_out/build.ninja (全部构建规则)           │
│   - build_out/CMakeFiles/rules.ninja (规则模板)    │
│   - build_out/compile_commands.json               │
│   - build_out/cmake_install.cmake                 │
└─────────────────────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────────────────────┐
│ 阶段三: Ninja Build                              │
│                                                   │
│ 按拓扑顺序执行:                                    │
│                                                   │
│ ┌─ 编译 xlntx C++ 源文件 (12 × cl.exe) ──────────┐│
│ │  12个 .cpp → 12个 .obj                          ││
│ └────────────────────────────────────────────────┘│
│ ┌─ 编译 xlntx C 源文件 (1 × cl.exe) ─────────────┐│
│ │  miniz.c → miniz.c.obj                          ││
│ └────────────────────────────────────────────────┘│
│ ┌─ 链接 xlntx 静态库 (1 × lib.exe) ──────────────┐│
│ │  14个 .obj → xlntx.lib                          ││
│ └────────────────────────────────────────────────┘│
│ ┌─ 编译主程序 C++ (2 × cl.exe) ──────────────────┐│
│ │  EffectInfo.cpp → .obj                          ││
│ │  third_party_effects_data_gen_vs.cpp → .obj     ││
│ └────────────────────────────────────────────────┘│
│ ┌─ 编译资源文件 (1 × rc.exe via cmcldeps) ───────┐│
│ │  resource.rc → resource.rc.res                  ││
│ └────────────────────────────────────────────────┘│
│ ┌─ 链接主程序 EXE (1 × link.exe via vs_link_exe) ┐│
│ │  2.obj + 1.res + xlntx.lib → .exe (697KB)      ││
│ │  Post-Build: applocal.ps1 (空操作)               ││
│ └────────────────────────────────────────────────┘│
│ ┌─ 编译测试程序 (1 × cl.exe) ────────────────────┐│
│ │  test_xlntx.cpp → .obj                          ││
│ └────────────────────────────────────────────────┘│
│ ┌─ 链接测试程序 EXE (1 × link.exe) ──────────────┐│
│ │  1.obj + xlntx.lib + xlnt.lib → .exe (1.6MB)   ││
│ │  Post-Build: applocal.ps1 (空操作)               ││
│ └────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────┘
        │
        ▼
    构建完成 ─── 2 个 EXE + 1 个 .lib 就绪
```
