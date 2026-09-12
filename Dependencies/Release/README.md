# 随仓库提供的 x64 Release 依赖

这些文件是构建输入，必须和工程配置一起提交到 Git。完整克隆后直接在 Visual Studio 中构建 Release/x64，无需先编译第三方库。

| 文件 | 来源 |
| --- | --- |
| lib/reactphysics3d.lib | ReactPhysics3D 0.10.2，单精度，/MD |
| lib/lua53.lib | Lua 5.3.5，/MD |
| lib/assimp-vc143-mt.lib | Assimp 553fbc1fdb9bf1d3e1b7a2382e4727bd71a4aee3，Release 导入库 |
| bin/assimp-vc143-mt.dll | 同一版本 Assimp 的 Release DLL |

使用 VS 2022 v143 x64 编译。许可文本见 licenses。SDL2 和 SDL2_mixer 继续使用 Dependencies/SDL/lib 内已有的公共 DLL。

具体构建说明见根目录 BUILDING.md。
