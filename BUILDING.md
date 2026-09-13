# 构建与运行 Engine-Demo

项目使用 Windows、Visual Studio 2022 v143、Windows SDK 和 C++20，目前支持 x64。安装 VS 的“使用 C++ 的桌面开发”组件后，克隆完整仓库即可构建。Debug 和 Release 所需的预编译依赖随仓库提供，无需安装 CMake、下载源码或运行依赖重建脚本。

## Visual Studio

1. 打开根目录的 `The 2D Engine.sln`，将 EDITOR 设为启动项目。
2. 选择 Debug/x64 或 Release/x64，生成解决方案。
3. 使用 F5 / Ctrl+F5，或直接双击 `x64/Debug/EDITOR.exe`、`x64/Release/EDITOR.exe`。

启动时根据 EXE 的实际位置向上查找同时包含解决方案和 `EDITOR/assets` 的仓库目录，再将工作目录统一设置为 `EDITOR`。因此从其他工作目录启动也可使用同一份 shader、Lua、模型、音频与 ImGui 配置。资源仍只保留在 `EDITOR/assets`，不复制到输出目录。

EXE 必须留在完整仓库内；这不是可以单独搬走 EXE 的资源发布包。资源目录无法定位或初始化失败会显示错误对话框并返回非零退出码。正常退出返回 0；Release 正常启动仍隐藏控制台。

## 预编译依赖

头文件在各库的 `include` 中共用，二进制按配置分开：

```text
Dependencies/
  Assimp/
    include/
    Debug/lib/、Debug/bin/
    Release/lib/、Release/bin/
  LUA/
    include/
    Debug/lib/
    Release/lib/
  ReactPhysics3D/
    include/
    Debug/lib/
    Release/lib/
    lib/cmake/ReactPhysics3D/
```

各库目录的 README 和 LICENSE 记录版本及许可。SDL2、SDL2_mixer 继续共用原有二进制，纯头文件依赖结构不变。ReactPhysics3D 的 CMake 导入元数据同步指向上述目录，仅供已有导入用途，不是 VS 构建步骤。

`Engine.Build.props` 按配置选择库目录和 Assimp 文件名，设置 C++20、Debug `/MDd`、Release `/MD` 与 `/O2`。`Engine.Runtime.targets` 检查预编译依赖，并将匹配的 Assimp DLL、SDL2 DLL 和可选音频 DLL 复制到输出目录。文件缺失时应恢复完整仓库文件。

Git 忽略规则允许上述 Debug/Release `.lib` 和 `.dll` 随仓库提交；`x64`、项目中间产物及个人 VS 设置继续忽略。不提供依赖重建脚本。

## 命令行构建

在 Developer PowerShell for VS 2022 的仓库根目录执行：

```powershell
MSBuild.exe 'The 2D Engine.sln' /t:Build /p:Configuration=Debug /p:Platform=x64 /m:2
MSBuild.exe 'The 2D Engine.sln' /t:Build /p:Configuration=Release /p:Platform=x64 /m:2
```

## out 清理与验证记录

此前的 `out` 约 490 MiB，保存依赖源码、依赖构建缓存、工程配置备份、临时脚本和日志。它不是工程构建或运行的输入，本次清理整个目录。普通构建不重新创建它。

此前两种配置虽能编译，但直接启动日志均显示窗口创建后找不到 `assets/shaders/forward_BlinnPhong`，因而退出；仅设置 VS 工作目录没有覆盖直接启动。本次修复启动定位，并补齐 `LoadEditorTextures()` 成功返回值。

原有 Debug Lua 使用 `/MT`，仍可能产生与 `/MDd` 的 LNK4098 告警；原有物理库没有匹配 PDB。源码也存在其他既有告警，构建成功不表示零告警。

2026-09-13 验证结果：

| 验证项 | Debug/x64 | Release/x64 |
| --- | --- | --- |
| 删除 out 后完整 Rebuild | 通过，298 警告、0 错误 | 通过，215 警告、0 错误 |
| 独立干净副本 Build | 通过，298 警告、0 错误 | 通过，215 警告、0 错误 |
| VS 启动 | 通过 VS 的 Start Without Debugging 启动并正常关闭 | 同左 |
| 输出目录 Shell 启动 | 窗口持续运行，正常退出 0 | 窗口持续运行，正常退出 0 |
| 从 C:\Windows 启动 | 进入 Demo / Game，Lua 资源加载成功 | 进入 Demo / Game，Lua 资源加载成功 |
| 独立副本从 C:\Windows 启动 | 窗口持续运行，正常退出 0 | 窗口持续运行，正常退出 0 |
| 副本中缺失整个 assets | 显示资源目录错误，退出 1 | 显示资源目录错误，退出 1 |
| 副本中缺失 forward_BlinnPhong.vert | 显示具体 shader 路径错误，退出 1 | 显示具体 shader 路径错误，退出 1 |

Shell 启动使用 Windows ShellExecute（与资源管理器双击 EXE 使用同一启动机制），工作目录设为输出目录。独立副本按 Git 可收录文件创建在仓库外，路径包含空格和中文，不携带 out、旧输出、.vs 或 .vcxproj.user；构建过程中未下载或编译第三方依赖。

Demo 验证包含拖入 scene1、切换 Game 并点击播放，画面显示模型、PBR 材质与阴影；日志确认 main.lua 加载模型、纹理、音乐和音效。本次是启动及 Demo 冒烟验证，未覆盖所有编辑器功能。

已核对两种 EDITOR.exe 的 DLL 导入及输出 Assimp DLL 哈希，配置匹配；所有八个预编译 .lib/.dll 均可被 Git 收录。工程无旧依赖路径或 out 引用，构建后 out 未重新生成。原有 Debug 依赖仅移动目录，文件内容未改变。ReactPhysics3D 的 Debug/Release CMake 导入路径也已单独配置校验通过；此检查不属于普通构建要求。验证副本和临时测试脚本已清理，构建日志与 Demo 截图保存在被 Git 忽略的 x64/validation。

## 属性页设置建议

新增共用包含目录、语言标准等设置时选择“所有配置 / x64”；运行库、优化和配置专用库应分别设置。属性按“项目 × 配置 × 平台”保存，只修改某项目的 Debug 不会同步到 Release 或其他项目。
