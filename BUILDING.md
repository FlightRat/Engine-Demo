# 构建 Engine-Demo（Visual Studio 2022 / x64）

本工程当前使用 Windows、Visual Studio 2022 v143、Windows SDK 和 C++20。以下步骤针对 x64；目录内第三方二进制不是一套完整的 Win32 依赖，不能切换到 x86 后直接使用。

**Debug 和 Release 所需的预编译库都随仓库提供。克隆完整仓库后，直接在 VS 中生成即可，无需先编译第三方库，也不需要为普通构建安装 CMake。**

## 在 Visual Studio 中操作

1. 打开根目录的 The 2D Engine.sln。
2. 将 EDITOR 设为启动项目。
3. 工具栏选择 Release、x64，生成解决方案。
4. 使用 F5 或 Ctrl+F5 启动。共享属性已将调试工作目录设为 EDITOR，供相对 assets 路径使用。

Release 输出为 x64/Release/EDITOR.exe，Debug 输出为 x64/Debug/EDITOR.exe。构建后自动将对应的 Assimp DLL、SDL2.dll、SDL2_mixer.dll 和可选音频解码 DLL 复制到输出目录。

不要将运行工作目录误设为输出目录：资产仍位于 EDITOR/assets。本次没有制作包含全部资产的发布包。

## 本次修复的配置

| 配置项 | 处理 |
| --- | --- |
| 包含目录与库目录 | 各项目原先只写在 Debug 的目录改为两种 x64 配置共用，保留每个项目自己的依赖范围 |
| C++ 标准 | Engine.Build.props 统一 C++20 |
| 项目类型 | PHYSICS、SOUNDS、ImGui、FILESYSTEM 的 Release 改为静态库；EDITOR 保持应用程序 |
| 运行库与优化 | Debug 使用 /MDd；Release 使用 /MD 和 /O2，保留各自的 _DEBUG/NDEBUG |
| 第三方链接 | 集中在 EDITOR 的最终链接阶段；避免将同一组外部库重复打入多个引擎静态库 |
| Release 依赖 | 在 Dependencies/Release/lib 与 bin 单独保存，优先于原有 Debug 库目录 |
| SOIL | 移除 CORE 对旧 SOIL 工程的引用及相关无用包含目录；现有纹理实现使用 stb_image |
| DLL 与工作目录 | Engine.Runtime.targets 自动复制运行 DLL；Engine.Build.props 设置编辑器工作目录 |

这里只共享与配置无关的设置，没有把整个 Debug 配置复制成 Release，也没有关闭运行库一致性检查或强制混用 Debug/Release C++ 库。

## Release 第三方依赖

原目录只有 Debug 版 ReactPhysics3D、Debug 版 Assimp，以及使用 /MT 的 Lua 静态库。因此单独补目录仍不足以构建正确的 /MD Release。

本次按对应版本重新编译：

| 依赖 | 源版本 | Release 产物 |
| --- | --- | --- |
| ReactPhysics3D | 0.10.2；官方 tag 对应 cd958bbc0c6e84a869388cba6613f10cc645b3cb | reactphysics3d.lib，/MD |
| Assimp | 现有头文件标记的提交 553fbc1fdb9bf1d3e1b7a2382e4727bd71a4aee3 | assimp-vc143-mt.lib / .dll，动态 CRT |
| Lua | 5.3.5，官方源码归档并校验 SHA-256 | lua53.lib，/MD |

ReactPhysics3D 预编译库所用源码的全部 .h 文件已与工程自带头文件逐一校验一致。原有 Debug 依赖文件没有被替换。

## 换电脑

安装 VS 2022 的“使用 C++ 的桌面开发”（包括 v143 和 Windows SDK），克隆仓库，按上面的 Visual Studio 步骤选择 Debug/x64 或 Release/x64 生成即可。

Dependencies/Release 下的三个 .lib 和一个 .dll 是随仓库保存的构建输入，已经通过 .gitignore 例外规则允许纳入版本控制。out、x64 和各项目生成目录继续被忽略。

如果缺少 Release 库，EDITOR 构建会提示恢复仓库内的预编译文件；不会把手工构建依赖作为普通用户的必需步骤，也不会回退链接 Debug 物理库。

## 命令行构建

在 Developer PowerShell for VS 2022 中进入项目根目录：

~~~powershell
MSBuild.exe 'The 2D Engine.sln' /t:Build /p:Configuration=Release /p:Platform=x64 /m:2
MSBuild.exe 'The 2D Engine.sln' /t:Build /p:Configuration=Debug /p:Platform=x64 /m:2
~~~

本机 MSBuild 路径为 D:/VisualStudio/2022/MSBuild/Current/Bin/MSBuild.exe。

## 2026-09-12 验证记录

- Release x64：完整解决方案 Build 成功，生成 x64/Release/EDITOR.exe。
- 二进制依赖检查：Release 编辑器链接 assimp-vc143-mt.dll 和 Release CRT；Assimp Release DLL 未依赖带 D 后缀的 Debug CRT。
- Debug x64：完整解决方案回归 Build 成功，生成 x64/Debug/EDITOR.exe；保留原有 Debug 依赖，未改变其版本。
- 本次以构建与二进制依赖验证为范围，未启动图形界面测试各个场景。

源码仍存在既有编译告警，例如 Application::LoadEditorTextures 和 AssetManager::GetAssetKeyName 部分控制路径缺少返回值。它们没有阻止链接成功，但需要单独修复和运行验证，尤其不能据此认为优化后的运行行为已经全部验证。

Debug 仍使用原有第三方二进制，存在物理库缺少 PDB，以及旧 Lua /MT 与 /MDd 引起的 LNK4098 运行库告警；本次没有将“构建成功”描述为“零告警”。本轮记录为 Release 80 个警告、0 个错误，Debug 299 个警告、0 个错误；Release 没有 LNK4098。

## 以后在属性页中怎样避免同类问题

新增头文件目录、与配置无关的宏、语言标准时，在属性页上方选择“所有配置”，平台选 x64；共同语言和链接设置也可以编辑 Engine.Build.props。

运行库、优化、Debug/Release 专用第三方库必须分别设置。Debug 的 /MDd 与 Release 的 /MD，以及 assimp-vc143-mtd 和 assimp-vc143-mt，不能用同一个固定值覆盖。

VS 的属性是按“项目 × 配置 × 平台”保存的。只在 Debug 下设置一个项目，并不会自动同步到 Release 或其他项目。
