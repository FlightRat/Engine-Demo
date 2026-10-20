# Engine-Demo MCP

本地 Codex 通过 stdio MCP Server 连接正在运行的 Windows 编辑器。
模型与对话由 Codex 管理；本包不调用模型 API，也不要求模型密钥。
支持非播放场景的物理配置、实体删除、本地资源导入、局部变换、灯光、材质、单选、Scene 图片、原子批次及多级撤销/重做。

## 安装与构建

需要 Windows x64、Visual Studio v143/Windows SDK、Python 3.11+。
以下 PowerShell 命令从仓库根目录运行，不需要激活虚拟环境：

```powershell
python -m venv tools/engine_mcp/.venv
& tools/engine_mcp/.venv/Scripts/python.exe -m pip install -r tools/engine_mcp/requirements.lock
& tools/engine_mcp/.venv/Scripts/python.exe -m pip install --no-build-isolation --no-deps -e tools/engine_mcp
python tools/engine_mcp/build.py editor
python tools/engine_mcp/build.py tests
```

锁文件包含实测依赖及构建工具；不使用 `--no-deps` 安装未准备依赖的环境。
build.py 从开发者环境寻找 MSBuild，也可传 `--msbuild '完整路径/MSBuild.exe'`。
脚本为各项目传入正确 SolutionDir，并消除重复 Path/PATH 环境键；构建日志保存在本目录的忽略文件中。
`--configuration Release` 构建 Release。现有编辑器资源目录定位及启动诊断保持原样。

## 启用指定实例

编辑器与 Codex 的本地执行器必须使用同一 Windows 用户。桥接默认关闭：

```powershell
& './x64/Debug/EDITOR.exe' --enable-mcp-bridge --mcp-instance default
```

编辑器自己定位仓库内的 EDITOR/assets。MCP Server 的 cwd 不负责引擎资源目录。
同名实例冲突会停用新进程的桥接并记录错误，保留手动编辑器。
可以用另一个名称运行第二个实例，但单个实例仅接受一个桥接客户端。
实例名仅允许 1..64 个英文字母、数字、下划线或连字符。

## 连接 Codex

把 `codex-config.example.toml` 的表复制到你自己的 Codex 配置，修改其中的绝对路径。
本仓库不会自动写入用户全局配置。CLI 等效命令为：

```powershell
codex mcp add engine_demo -- 'D:/代码-CG/Engine-Demo/tools/engine_mcp/.venv/Scripts/python.exe' -m engine_mcp.server --instance default
codex mcp list
```

在新建或重新加载配置的本地 Codex 会话中先调用 get_editor_status，再调用 get_scene_state。
`mcp list` 只验证配置注册；实际查询到 session、实例和场景才证明连通。
使用既有 Codex 登录和模型配置；编辑器桥接不提供模型，也不管理模型费用。
官方配置文档：[Codex MCP](https://learn.chatgpt.com/docs/extend/mcp?surface=cli)。

## 工具与使用约定

| 工具 | 作用 |
| --- | --- |
| get_editor_status | 实例、会话、revision、播放状态及可撤销 operation_id |
| get_scene_state | 场景清单、当前或指定场景快照、名称/组件过滤与分页 |
| list_assets | 已加载 model/texture 的键，默认页长 50，最大 200 |
| create_scene | 创建并激活空场景；名称唯一 |
| create_object | 从已加载模型创建可见 mesh，或创建启用的点光源 |
| set_transform | 局部位置、XYZ 欧拉角（度）和正缩放 |
| set_light | 灯光类型、线性 RGB、强度及适用的类型专属参数 |
| apply_operations | 当前场景内最多 32 个创建/变换/灯光/材质/物理/删除动作，失败整体恢复 |
| undo_operation | 按原内容 operation_id 撤销 undo 栈顶；支持连续撤销 |
| get_operation_status | 根据原 session_id/request_id 查询排队或最终结果 |
| set_material | 修改现有材质槽的颜色、PBR 参数及已加载二维纹理绑定；支持批次与撤销 |
| get_selection | 查询当前编辑场景的单选实体及 selection_serial |
| set_selection | 按 entity_id 选中，或以 null 清空；同步层级、GO Details 和 Gizmo |
| capture_viewport | 返回新的 Scene 相机 PNG 与尺寸、frame_id、相机和版本元数据 |
| get_history | 分页查询 undo/redo、原内容 ID、作用域、数量与预算 |
| redo_operation | 重做 redo 栈顶，恢复对象时返回新 ID 映射 |
| add_physics | 为有 Transform 的根实体添加物理组件 |
| set_physics | 修改已应用的刚体、碰撞形状和物理材质配置 |
| remove_physics | 移除物理组件及其运行资源，保留实体 |
| delete_entity | 删除可恢复实体；有子实体需显式 cascade=true |
| import_asset | 以明确资源键导入模型、二维纹理、音乐、音效或顶点/片元 Shader |


先查询，再把返回的 session_id/editor_revision 与新的 request_id 用于写入。
实体写入必须指向当前 scene_id；import_asset 是无场景要求的全局操作；undo/redo 按历史项作用域校验。重名物体用 entity_id 区分。
不要自行猜测实体 ID；删除、加载或编辑器重启后旧 ID 失效。
场景/资源/播放状态变化会使旧分页游标和写入前置条件失效，需重新查询。

Transform 使用引擎单位和 local 空间，根对象 local 即 world；不支持 world 转换；物理根实体支持位置/角度同步，但拒绝 scale 写入，碰撞尺寸通过 set_physics 设置。
灯光位置使用 set_light.position，与 Transform 独立。点光衰减常数必须 >0，方向光方向不得为零。
颜色为线性 RGB [0,1]，强度 [0,10000]，面积光半宽/高 [.001,4.999]，完整边界见 PROTOCOL.md。
切换灯光类型保留颜色、强度和启用状态，其他类型字段恢复有效默认值。

使用 apply_operations 将一次布局组合为一个撤销单元。client_ref 唯一，target_ref 只能引用批内更早创建的对象。
场景创建单独调用，不放入批次。历史按会话全局排序，undo/redo 合计默认最多 64 项及 256 MiB 估算保留载荷。新成功内容写入清空 redo；人工内容/资源修改、外部场景切换、加载及播放切换清空两栈。失败不清空历史，选择和截图保留历史。超预算返回 history_capacity_exceeded，不能执行不可撤销的内容修改。

通信超时不等于操作失败。pending/queued/running 时应查询 get_operation_status；
响应丢失时只有相同 request_id、原参数与原 expected_revision 的重试才会返回同一结果。
不要换一个 ID 重发可能已执行的创建操作。编辑器重启会产生新 session，不能跨会话自动重放。

## 可重复验收

编辑器已加载 cube 等默认模型，可先以它验收；茶壶需要事先通过编辑器导入并查询实际模型键。

```powershell
& tools/engine_mcp/.venv/Scripts/python.exe -m engine_mcp.demo --instance default --model cube --pause-seconds 5
```

该命令使用官方 SDK 启动 stdio Server，依次查询状态/资源、创建唯一展示场景、
在一个批次中创建模型与暖色点光源、查询真实属性、等待 5 秒，再撤销该批次。
预期：视口出现旋转的模型和暖色照明，最后两个物体消失，空展示场景保留。
启动默认显示 Game 标签，观察编辑结果需切到 Scene；底部默认显示 Logs，
手动管理场景和资源需切到 Asset。MCP 创建场景会自动激活。
用 `--keep` 可保留对象供检查，输出包含 operation_id 和实体引用。
要验证人工修改冲突，先 `--keep`，用属性面板或 Gizmo 改动物体，重新查询后请求撤销原 operation_id，
预期 history_conflict 且人工结果不变。

在 Codex 中可输入：

> 使用 engine_demo 工具查询资源和当前状态，创建展示场景，在一个批次中把 cube 放在中央并旋转 30 度，
> 在左上方创建暖色点光源，查询确认后撤销该批次。不要修改代码或执行 Lua。

场景不会自动保存；关闭编辑器会丢失未保存内容。本次没有修复现有 JSON 序列化覆盖范围，不用它实现撤销。

## 测试与故障排查

```powershell
& './x64/Debug/AutomationTests.exe'
& tools/engine_mcp/.venv/Scripts/python.exe -m pytest tools/engine_mcp/tests -q
```

原生测试覆盖真实 ECS、身份、旋转、分页、事务、撤销及队列。
Python 集成测试使用 `AutomationTests.exe --serve <唯一实例>` 提供真实 C++ 桥接，不创建 OpenGL 窗口；
SDK stdio 测试与无编辑器测试也在其中。画面验收必须另外在实际 EDITOR 中运行上面的 demo，不能以这些测试代替。

实际版本、测试范围、真实 EDITOR 的查询结果、用户已通过的 Codex 快速冒烟，以及待完成的界面验收见
[VERIFICATION.md](VERIFICATION.md)。当前自动测试通过不表示全部接入验收已经完成。

`run_codex_acceptance.py --codex <本地原生codex.exe绝对路径> --instance <实例>`
可用进程级配置运行一次受限验收，不写用户全局配置；它沿用既有 Codex 登录和模型设置，
会产生模型调用。只有实际完成必要的 engine_demo 工具调用才返回成功，输出保存在本目录的 `qa-codex-*.log`。

- editor_unavailable：检查编辑器已打开、启用桥接、实例与 Windows 用户匹配；Server 可先于编辑器启动。
- 切换分支后连接失效：x64 构建产物不会随 Git 分支切换，先重新构建当前 MCP 分支的 Debug/Release，再带桥接参数启动对应 EXE。
- editor_busy：该实例已被另一个客户端连接；关闭旧连接再试，不要改连其他实例。
- revision_conflict/stale_target：重新查询状态，确认目标后生成新的明确修改请求。
- history_conflict：旧撤销已失效，不会覆盖人工内容；先查看当前状态。
- request_capacity_exceeded：本会话写账本已满；保留旧请求可查询性，正常保存后重开编辑器建立新会话。
- recovery_required：停止自动化写入、检查当前场景，重开编辑器后再继续。
- Server 日志写 stderr；不要把调试打印写 stdout。CLI 参数错误退出码为 2，正常关闭为 0。

回退：关闭带桥接开关的编辑器，以不带该开关的方式重开，并从自己的 Codex 配置移除 engine_demo 表。
手动编辑流程和现有场景文件格式无需迁移。

## 材质、当前选择与 Scene 图片

当前工具集共 21 个。重新构建并启动新版 EDITOR 后，重新加载 MCP 工具目录。
模型调用仍由外部 Codex 管理；没有增加引擎模型密钥或公网服务。

材质查询复用 `get_scene_state`：实体的 materials 数组包含 material_index（从 0 开始）、
color（三个线性 RGB 分量）、独立 alpha、metallic、roughness、ao、use_texture、textures 和只读 shading_model。
`set_material` 除现有写入前置条件外，要求 entity_id、material_index 及至少一个修改字段。
颜色/alpha/PBR 数值范围均为 [0,1]；不会创建或扩展材质槽，不修改着色模型或显示开关。
textures 接受 albedo/normal/metallic/roughness/ao 到已加载二维纹理键的部分映射，
null 清空绑定；不会自动导入纹理，也不会隐式改变 use_texture。设置纯色时通常显式关闭 use_texture。
材质修改支持 apply_operations 的 target_ref，可与模型创建放入同一批次，进入同一多级历史。

`get_selection({})` 返回当前 scene_id、entity_id/name（未选择为 null）及 selection_serial。
助手应先获取选中实体，再将明确 entity_id 传给材质或其他内容工具。
`set_selection` 需要 session_id、request_id、expected_revision、当前 scene_id 和 entity_id；
entity_id=null 清空选择。仅支持单选，不自动切标签或聚焦相机。
选择操作有可查询/去重的回执，但其 operation_id 不可撤销；选择和清空不推进内容 revision，
不会消耗或替换之前的材质撤销。重试原选择请求只返回原回执，不覆盖之后的人工改选。

`capture_viewport` 需要 session_id、当前 scene_id、expected_revision，没有写入 request_id。
固定返回 Scene 相机的最终渲染结果，不包含编辑器面板或 Gizmo；默认 Game 前台也会按需渲染新图，
无需切到 Scene，不会移动相机、改变选择/内容 revision/撤销或保存场景。
默认 640×360，width/height 支持 64..1024 整数，按比例限制至 230400 像素，1024×1024 实际返回 480×480。
结果为一个 image/png 图片块及不含像素的结构化元数据，PNG 最大 700 KiB；内部桥接仍限制单消息 1 MiB。
同一实例最多一个未完成截图；capture_busy 可稍后重试，capture_timeout/capture_unavailable 明确表示未获得图片。
截图超时不会修改场景，不返回缓存旧帧。不提供连续视频、高分辨率分块或相机控制。

独立 SDK 冒烟命令（只创建并修改新的唯一 QA 场景，模型/灯光保留供检查，不保存场景）：

```powershell
& tools/engine_mcp/.venv/Scripts/python.exe -m engine_mcp.demo_visual --instance default
```

若明确需要保存 PNG 和工具元数据作为验收证据，可增加
`--output-dir tools/engine_mcp/qa-visual-local`；未指定时不落盘。
同一实例只能有一个客户端，运行脚本前应释放其他 Codex 对该实例的连接，或使用独立 QA 实例。

可在已连接的 Codex 中一次输入：

> 只使用 engine_demo。在新建的唯一测试场景创建 Cube（mesh，model=cube）和暖色灯，
> 查询材质槽后选中 Cube，确认 get_selection 返回它。在一个材质批次中关闭纹理、改为红色金属，
> 查询并获取 Scene 截图。清空选择，再撤销刚才的材质批次，查询和截图确认材质恢复。
> 不修改其他场景，不保存文件，不执行脚本。

完整受限 CLI 验收可运行 `run_codex_acceptance.py --visual --codex <原生codex.exe路径> --instance <QA实例>`。
该命令会调用既有 Codex 模型，使用临时进程配置；日志移除图像字节，保留工具元数据和成功/失败证据。

## 物理、删除、多级历史与外部资源

现有 14 个工具的合法参数继续可用，新增 7 个工具后共 21 个。所有内容写入及 undo/redo 都拒绝播放状态。

- add_physics 要求有 Transform、无父级且尚无物理组件；set_physics/remove_physics 要求已有物理组件。
- 字段包括 body_type（static/dynamic/kinematic）、enable_gravity、mass、linear_damping、angular_damping、两个 *_axis_factor、shape（box/sphere/capsule）、trigger、friction、bounciness、mass_density。box 用 half_extents[3]，sphere/capsule 用 radius，capsule 还用 half_height。未知、不适用字段及无效数值整项拒绝。
- get_scene_state.physics 返回已应用配置。属性面板保留 Apply，尚未应用的草稿不进入查询或 MCP 历史。物理根对象的位置/角度通过 set_transform 更新，同时同步刚体及插值；视觉缩放不隐式改变碰撞尺寸，物理对象的 scale 写入拒绝。
- delete_entity 默认 cascade=false，有子实体返回 entity_has_children；显式 true 删除可恢复子树。支持身份、变换、网格、材质、灯光、物理及两类旧碰撞组件；Script/未知组件返回 unsupported_component 和类型清单，删除前不会丢弃它们。
- 恢复实体或场景产生新公开 ID，使用 entity_id_remap/scene_id_remap；旧 ID 永远失效。历史内部跟随逻辑目标。选择回执与 undo/redo 遍历回执的 operation_id 不是新的内容历史项。
- get_history({"stack":"undo","page_size":50}) 或 stack=redo 查询栈顶原内容 operation_id，再用新 request_id、当前 revision 调用 undo_operation/redo_operation。两栈默认合计 64 项、256 MiB 估算保留载荷，披露数量/使用量/上限。单项过大返回 history_capacity_exceeded，历史淘汰不删除请求去重回执。

import_asset 不要求激活场景，不放进 apply_operations。参数为公共 session/request/revision、asset_type、未占用的 asset_key，以及 model/texture/music/soundfx 的绝对本地 path；shader 改传 vertex_path 和 fragment_path。texture 可设 pixel_art，默认 false。它只加载当前会话，不弹文件选择器、不复制/删除/覆盖源文件、不保存导入清单。Asset 窗口 Add Scene 继续用 create_scene。

模型与新增纹理依赖原子发布；缺失/损坏依赖或暂不支持的 embedded 贴图整体失败。同名资源默认冲突；纹理键由模型命名空间及规范化来源生成，只复用来源、内容、采样选项均相符的资源。结果列出 added/reused。撤销只取消本次新增登记，仍有实体、材质、渲染上下文或音频消费者时返回 asset_in_use。重做使用保留结果，流式音乐保留所需内存源，因此不依赖原文件仍然存在。

导入后台准备，每帧分步上传后再校验原 revision 并提交；期间可以查询/编辑，发生内容变化会使导入返回 revision_conflict。每实例最多一个导入，额外请求返回 import_busy；开始前仍受队列 TTL 约束，开始后默认限时 60 秒。Server 等待超时返回 pending，继续查询原请求，不换 ID 重发。提交前成功取消保证无副作用；提交开始后的取消等待不能被当作取消写入。

输入文件合计最多 128 MiB，模型几何准备最多 128 MiB，单图片解码最多 128 MiB；每次最多 4096 个来源文件、4096 个网格部分、512 个纹理依赖，另受历史预算和 1 MiB 响应约束。高于边界时明确失败，不截断资源或稍后静默执行。

独立真实 EDITOR 验收（使用专门 QA 实例，不能与另一个桥接客户端同时连接）：

```powershell
& tools/engine_mcp/.venv/Scripts/python.exe -m engine_mcp.demo_editing --instance <QA实例> --output-dir tools/engine_mcp/qa-import-local
& './x64/Debug/AutomationTests.exe' --gl-tests
```

demo_editing 验证五类导入及“创建→物理→移动→删除→全部撤销→全部重做→清理”，检查源文件指纹并默认恢复进入前状态。--keep 留下本次唯一 QA 场景/资源，恢复并选中物理实体，供 Apply/Gizmo 人工界面检查。显式 output-dir 才保存图片与不含像素的工具证据。--gl-tests 使用真实隐藏 GL 上下文和 SDL dummy 音频后端，不代表用户已看到 UI 或听到声音。

受限 Codex 验收脚本增加 --editing（与 --visual 互斥），使用单次临时配置并校验工具链、原内容 ID 顺序和图片块。它仍会调用现有 Codex 模型，不修改全局配置。
