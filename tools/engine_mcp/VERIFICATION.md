# MCP 实现与验收记录

日期：2026-10-19。变更：`add-editor-mcp-integration`。

## 环境

| 项目 | 实测版本 |
| --- | --- |
| Windows | NT 10.0.19045，x64 |
| MSBuild / 编译器 | 17.11.2.32701 / MSVC 19.41.34120，v143 |
| Python | 3.12.7，独立 `.venv` |
| 官方 MCP Python SDK | 2.3.0 |
| jsonschema / pytest | 4.26.0 / 9.0.2 |
| 本地 Codex CLI | 0.159.3 |

依赖完整版本在 `requirements.lock`。Server 不调用模型 API，不读取模型密钥。

## 可复现的自动验证

从仓库根目录运行：

```powershell
python tools/engine_mcp/build.py tests
& './x64/Debug/AutomationTests.exe'
python tools/engine_mcp/build.py tests --configuration Release
& './x64/Release/AutomationTests.exe'
python tools/engine_mcp/build.py editor
python tools/engine_mcp/build.py editor --configuration Release
& tools/engine_mcp/.venv/Scripts/python.exe -m pytest tools/engine_mcp/tests -q
openspec validate add-editor-mcp-integration --strict
```

原生测试使用真实 SceneManager、ECS、资源管理接口和主线程队列。
覆盖身份与重命名、局部旋转/灯光规则、旧 revision/游标、物理目标拒绝、
批次回滚恢复原实体与删除新实体、撤销失效、恢复失败后只读、队列容量、过期与取消。

Python 套件实测 **38 passed**。其中集成测试启动独立的真实 C++ 管道宿主，
覆盖分片传输、多请求响应关联、1 MiB 边界、非法长度/JSON/UTF-8/重复键、
未知版本、同名实例冲突、双实例隔离、单客户端限制、断管重连、稍后上线、
编辑器重启、重复写去重、pending 查询、排队取消和不读取响应时的关闭。
另有 SDK stdio 工具调用、中文/空格路径配置、Draft-4 与公开 2020-12 Schema 等价性测试，
以及 MCP `2025-06-18` 协议的实际 initialize/tools/list 消息兼容检查。

这些测试不创建 OpenGL 窗口，不能证明视口显示或鼠标交互正确。
构建过程的完整日志写入本目录被忽略的 `build-*.log` 文件。

最终复核：完整 EDITOR x64 Debug/Release 均构建成功（0 个错误），
AutomationTests x64 Debug/Release 均构建并运行通过；`pip check` 通过，
OpenSpec strict 校验与 `git diff --check` 通过。

## 对真实 EDITOR 的 SDK 验收

已用真实 `x64/Debug/EDITOR.exe --enable-mcp-bridge --mcp-instance codex_qa`
运行官方 SDK 的 `engine_mcp.demo`，成功执行以下调用链：

`get_editor_status → list_assets → create_scene → apply_operations → get_scene_state → undo_operation → get_scene_state`

本机原始执行记录：`qa-demo-undo.log`（忽略文件，仅保留本地）。关键返回值：

| 观察项 | 实际返回 |
| --- | --- |
| 已加载模型 | capsule、cube、hud_quad、plane、quad、skybox、sphere |
| 新场景 / 批次 | `scene-11` / `operation-12`（只对当次 session 有效） |
| 模型 | cube，position `[0,0,0]`，rotation_degrees `[0,30,0]`，scale `[1,1,1]` |
| 灯光 | 启用的 point_light，position `[-3,4,2]`，color 约 `[1,.65,.35]`，intensity `250`，range `25` |
| 批次后 | 查询 total `2`，revision `4` |
| 撤销后 | 查询 total `0`，revision `5`；`scene-11` 仍存在并保持激活 |

浮点颜色返回 `.649999976` / `.349999994` 是引擎 float 精度。
查询中的 `visible=true` 是渲染组件状态，不代表已经观察到实际画面。
测试创建的临时编辑器进程已停止；没有保存或改写场景文件。

## Codex 快速冒烟验收通过

2026-10-19，用户在已连接 `default` 的 Codex 会话执行一段式测试，反馈全部通过。
以下结果来源为用户实测反馈，不是本会话重新执行工具调用或观察画面所得：

| 项目 | 用户验收结果 |
| --- | --- |
| 查询状态与已加载资源，新建唯一测试场景 | 通过 |
| 单批次创建 Cube、设置原点位置和 Y 轴 30 度旋转、创建暖色点光源 | 通过 |
| 查询两个实体的属性，以及原请求的操作状态 | 通过 |
| 相同 request_id 和原参数重试，结果一致且不重复创建 | 通过 |
| 零缩放、旧 revision、缺失模型批次被拒绝，状态无残留变化 | 通过 |
| 撤销创建批次后实体数由 2 变为 0，测试场景保留 | 通过 |
| 新 request_id 再次撤销原批次被拒绝 | 通过 |

这轮验证确认 Codex 到真实编辑器的核心接口链路可用。它覆盖批次预校验拒绝，
执行中故障回滚仍以原生故障注入测试为证据。

`run_codex_acceptance.py` 可独立生成工具事件验收记录；早期的 `qa-codex-*.log`
包含未完成尝试，不作为上述用户验收的原始工具日志。

## 连接与构建注意事项

切换分支后曾复现 `editor_unavailable`：当前分支已包含 MCP 源码，但磁盘上的
Debug/Release EXE 是在不含 MCP 的分支编译的，均不含桥接启动参数和代码。
`x64/` 被 Git 忽略，切回分支不会恢复对应二进制。重新构建当前 MCP 分支两种配置后，
实际 `get_editor_status` 返回 connected=true、instance=default；`list_assets`
成功返回包括 cube、teapot 在内的 9 个模型。

当前用户专用管道还要求编辑器与 MCP 执行器使用同一 Windows 身份，
包括沙箱身份；宿主用户与沙箱用户不同会得到 `editor_unavailable`，不能自动改连其他实例。

## 完整界面与生命周期验收待完成

用户已恢复窗口访问授权；Computer Use 捕获窗口时仍遇到超时。
本轮快速冒烟不包含 Gizmo/属性面板的人工操作、模型与灯光画面一致性、
人工编辑后拒绝旧撤销、各 UI 结构/场景/资源入口的失效检查，
也不包含真实双编辑器隔离、桥接关闭/失败时的手动编辑与正常退出。
需要这些证据的 10 项 OpenSpec 任务继续保留未勾选。用户确认保留未完成项后，
变更于 2026-10-19 归档，两份能力规格已同步到主规格。
原始任务清单见 [归档 tasks.md](../../openspec/changes/archive/2026-10-19-add-editor-mcp-integration/tasks.md)。

继续验收时在 Scene 标签观察编辑结果；启动默认显示 Game，底部默认显示 Logs，
管理场景或资源需切换到 Asset。MCP 创建场景会自动激活，手动切换场景则需从
Asset 的 SCENE 分类拖入 Scene 视口。

## 材质、选择与 Scene 截图扩展（2026-10-19）

变更：`add-mcp-material-selection-capture`。下列记录属于本次扩展，前面已归档变更的
验收范围和未完成任务保持原记录，不追溯改勾。

### 构建和自动测试

- 完整 EDITOR x64 Release 最终构建成功，0 错误：`build-editor-Release.log`。
- 完整 EDITOR x64 Debug 最终标准产物构建成功，0 错误：`build-editor-Debug.log`，
  产物为 `x64/Debug/EDITOR.exe`；另有隔离复核日志 `build-editor-Debug-validation.log`。
  人工检查迁移至可见 Release QA 实例后，后台 Debug QA 已结束，标准 Debug/Release EXE 均已更新到最终代码。
- AutomationTests x64 Debug / Release 均构建成功并运行通过，包含原操作/队列、
  新材质/选择和捕获协调测试；原生测试不创建 OpenGL 窗口。
- Python 套件最终 **62 passed**，`pip check` 无依赖冲突。
- 共享契约与公开 Schema 保持一致：十四工具、旧工具参数兼容、材质未知字段和数值边界、
  写入分类、选择回执/重连/去重、无渲染器时 capture_unavailable 均有覆盖。
- 选中对象改名后 get_selection 返回实际 Identification 名称，原生测试验证不会返回包装对象的缓存旧名。
- 捕获协调测试覆盖延迟完成不阻塞其他查询、单捕获忙、过期、取消、晚到编码结果不覆盖取消、
  场景切换、像素长度、最大消息预算、超大元数据以及停止时未完成请求取消。
- PNG 测试覆盖 RGB/行方向/CRC/随机像素最大尺寸、无图片字节的元数据、损坏像素拒绝、
  Server 取消后释放捕获槽和不把像素写入错误/日志。CLI 验收器拒绝无工具证据的“成功”。

### 真实 EDITOR 的 SDK 验收

对独立 `qa_material_capture`（Debug）及 `qa_material_release`（Release）运行：

```powershell
& tools/engine_mcp/.venv/Scripts/python.exe -m engine_mcp.demo_visual --instance <QA实例> --output-dir <明确指定的证据目录>
```

两种配置均实测通过 **18 次工具调用**：创建唯一 QA 场景；一个布局批次创建 Cube、暖光并以
target_ref 设置初始材质；选择及查询 Cube；材质批次改红色金属；连续两次截图；清空选择；
重试原选择不覆盖清空；非法槽/缺失纹理拒绝；撤销材质；查询及第三次截图确认恢复。

| Debug 返回项 | 真实结果 |
| --- | --- |
| 测试场景 / Cube | scene-4 / entity-8（仅该 session 有效） |
| 第一次图片 | frame_id=1，640×360，revision=3，约 0.438 秒 |
| 请求 1024×1024 | frame_id=2，实际 480×480，revision=3，约 0.422 秒 |
| 撤销后图片 | frame_id=3，640×360，revision=4，约 0.469 秒 |
| 相机 | position `[0,20,-20]`，前向约 `[0,-.7071,.7071]`，fov 43；三次元数据一致 |
| 真实图像检查 | 红色 Cube 可见；撤销后恢复白色；PNG 可正常解码且上下方向正常 |

本机证据（忽略文件，不作为代码提交）：
`qa-visual-debug/events.json`、`qa-visual-debug/material-red.png`、
`qa-visual-debug/material-large.png`、`qa-visual-debug/material-restored.png`；
Release 对应文件在 `qa-visual-release/`。工具记录不包含图片 Base64。

开发中真实连续捕获发现复用已 move 的 RapidJSON Document 会失去 allocator，导致第二次捕获失败；
已改为每次捕获创建新的相机元数据 Document。修复后 Debug/Release 连续截图和不同尺寸复核通过，
另外在真实 Release 中连续完成数百次捕获，无超时、错误帧或连接中断。

另以可见独立 Release 实例 `qa_visible_material` 再次运行同一 SDK 流程，三张图片与全部调用通过。
这些 PNG 是编辑器渲染结果；没有用 headless 合成像素替代真实画面。

### 真实本地 Codex 验收

运行 `run_codex_acceptance.py --visual`，只使用进程级 MCP 配置和既有模型登录。
原生 Codex CLI 返回 **exit 0**，且语义验收器核对工具调用后返回成功：

- 实例 qa_material_release，新 QA 场景 scene-17，Cube entity-21。
- 16 次成功工具调用，包含 get_selection / set_selection、材质批次、两次实际图片和撤销。
- 材质 operation-24；修改后 color 约 `[1,.02,.02]`、metallic `.6`、roughness `.25`、use_texture=false。
- 选择确认 Cube → 清空并查询 null；材质撤销保持有效，撤销后完整材质槽与原快照一致。
- 两张实际 MCP 图片分别为 frame_id 4、5，均 640×360；模型报告看到红色 Cube，随后看到恢复的棋盘外观。

本机记录：`qa-codex-visual-result.log`、`qa-codex-visual-events.log`、`qa-codex-visual-stderr.log`。
日志在写盘前移除图片字节，仅保留图片内容块标记和元数据；图片由模型从实际工具响应查看。
该记录是 Codex 模型观察，不是人工面板观察。

### 界面与真实生命周期仍待补证

Computer Use 对隐藏及可见 QA 窗口均遇到 `FrameArrived timed out` / `window capture timed out`；
无图像的可访问性树只包含窗口系统控件。关闭按钮输入报 `coordinate input geometry is unavailable`，
键盘关闭未观察到进程退出。因此没有把这些尝试记为正常退出成功，也没有使用盲点坐标或其他 UI 自动化绕过。
已结束不再使用的隐藏 Debug/Release QA 进程；这些清理不作为正常关闭验收证据。

2026-10-19，用户明确选择“暂不做人工界面验收”。本轮停止继续窗口交互检查，
层级/GO Details/Gizmo、手动选择与粗糙度修改，以及人工修改后旧撤销返回 history_conflict 的联验均暂缓。
此前准备的可见 Release 实例为 `qa_visible_material`，已选中 Cube；检查点记录在
`qa-visual-visible/manual-checkpoint.json`，原材质 operation-17 / revision 5。
此检查点仅在原进程与 session 仍有效时可用；恢复验收前须重新查询状态，不能直接复用旧身份与版本。
此前后台 Debug 检查点已随该临时进程结束失效，不得跨 session 使用。

本次任务 6.2–6.5 保留未勾选：Scene 前台与 Game 前台的完整界面对比、人工编辑保护、
真实 UI 场景/播放/缩放/关闭矩阵及汇总人工证据仍未齐备。SDK 和 Codex 主链路已通过的部分如上，
不以它们代替缺失的人工界面/正常退出证据。

## Scene 播放生命周期验收（2026-10-20）

变更：`add-scene-playback-lifecycle`。本节记录本变更的证据，前文历史验收记录不代表新增播放交互已由人工确认。

### 自动化检查

| 项目 | 结果与证据范围 |
| --- | --- |
| EDITOR x64 Debug / Release | 构建通过，0 个错误；完整构建仍有编译器/模板转换警告 |
| AutomationTests x64 Debug / Release | 构建及运行通过，未创建编辑器窗口 |
| 新增原生测试 | 状态/按钮矩阵、游戏时钟、30/60/144 FPS 真实 RP3D 一致性、双 Registry/层级/材质/相机隔离、反复运行与故障清理、默认空 Scene、文件生成/复用/冲突/回滚、Lua 回调及文档示例、组件元数据独立注册、运行输入数据与音频所有权 |
| SDL dummy 真实音频后端 | 通道/音乐 Pause、Resume、Stop、脚本已暂停声音、外部声音和音量还原通过；没有实际听感证据 |
| MCP 原生回归 | 三状态及旧 playing、Paused 写入/截图拒绝、Scene 创建历史与脚本保留、失败 Play 保留历史、跨状态导入/截图失效、运行帧不推进 revision、多个状态切换分别计数、已完成请求回执复用通过 |
| Python SDK / 协议回归 | 68 passed；真实无窗口 C++ 管道宿主及 SDK 调用，21 个工具保持 |
| Pause 图标 | 内嵌字节与用户 pause.png 一致；Pillow 解码为 200 × 200 RGBA，1749 字节；只证明资源有效，未验证按钮实际外观 |
| OpenSpec / 差异检查 | strict 校验及 git diff --check 通过；新文件另行检查尾部空白与项目源文件条目 |
| JSON 加载 | SceneLoader.cpp 未修改；未改变 JSON 格式、加载或引入 JSON 运行快照 |

可复现的构建及原生检查（仓库根目录）：

```powershell
python tools/engine_mcp/build.py tests --configuration Debug --msbuild 'D:/Microsoft Visual Studio/IDE/MSBuild/Current/Bin/MSBuild.exe'
& './x64/Debug/AutomationTests.exe'
python tools/engine_mcp/build.py editor --configuration Debug --msbuild 'D:/Microsoft Visual Studio/IDE/MSBuild/Current/Bin/MSBuild.exe'
python tools/engine_mcp/build.py tests --configuration Release --msbuild 'D:/Microsoft Visual Studio/IDE/MSBuild/Current/Bin/MSBuild.exe'
& './x64/Release/AutomationTests.exe'
python tools/engine_mcp/build.py editor --configuration Release --msbuild 'D:/Microsoft Visual Studio/IDE/MSBuild/Current/Bin/MSBuild.exe'
```

SDK 检查：

```powershell
$qaRoot = Join-Path (Join-Path (Get-Location).Path 'x64') ('qa-playback-' + [guid]::NewGuid().ToString('N'))
& tools/engine_mcp/.venv/Scripts/python.exe -m pytest tools/engine_mcp/tests -q -p no:cacheprovider --basetemp $qaRoot
openspec validate add-scene-playback-lifecycle --strict
git diff --check
```

环境说明：默认 pytest 缓存和 pytest-of 临时目录存在访问权限限制，因此使用新的 x64 隔离临时目录并关闭 pytest 缓存；该方式下完整套件通过，没有修改用户全局配置。原生宿主使用独立临时资产根，QA 不向真实 assets/scripts 写入脚本。Debug 的资源新增禁止/缺失辅助脚本样例会输出预期 sol2 异常消息，随后测试通过。

### 验证方式约束

本次没有使用 computer-use、CUA、浏览器/桌面键鼠、窗口 UI 自动化或模拟拖拽。状态切换由测试直接调用 C++ 控制器，输入验证使用纯数据；没有增加 MCP Play/Pause/Stop 工具来绕过约束。

原生 CaptureTests 使用合成像素，仅证明既有协调器/协议规则；本轮没有以它们宣称真实 Game/Scene 画面正确。真实声音使用 dummy 设备核对后端状态，不能据此确认听感。

### 待人工验证

以下各项均为**待人工验证**。提供清单即可完成交付任务；没有人工证据前不得把这些条目改为通过。

| 状态 | 人工步骤 | 预期 |
| --- | --- | --- |
| 待人工验证 | 启动 EDITOR，查看空 default；依次 Play、Pause、Resume、Stop 并悬停三按钮 | 图标正确、当前状态高亮、不可用操作置灰，暂停后的 Play 提示 Resume Game |
| 待人工验证 | 编辑态创建可见动态刚体，同时显示 Game 与 Scene 后 Play | 两个窗口显示同一运行对象及运动，使用独立相机 |
| 待人工验证 | Pause 后等待并导航 Scene 相机，再 Resume | 场景模拟与游戏相机冻结，Scene 仍可导航，恢复不补算暂停时间 |
| 待人工验证 | 在 Game 画面操作，再切到 Scene、工具栏及可输入文本框（若有） | 游戏只接收 Game 画面的输入，编辑器导航、文本和按钮点击不透传；失焦仍模拟 |
| 待人工验证 | 预加载音乐/音效，通过 Scene 脚本播放，依次 Pause/Resume/Stop | 实际声音暂停、恢复和停止，音量不污染下一轮 |
| 待人工验证 | Playing/Paused 时尝试属性、物理 Apply、Gizmo、对象/资源/Scene 修改、拖入其他 Scene、Open/Save | 内容写入被阻止，仍可选择运行对象并只读查看 |
| 待人工验证 | 运行脚本改变位置、创建/删除对象，Stop 后再次 Play | 编辑数据恢复，下一轮从编辑初始状态启动；没有重复对象或继承速度 |
| 待人工验证 | 通过 Asset UI 创建、重命名、删除 Scene，并检查同名 Lua | 创建激活空 Scene、复用不覆盖、重命名同步文件、冲突不覆盖、删除保留脚本 |
