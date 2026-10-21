# Tasks

本清单仅用于后续 apply。所有任务初始未完成。验证禁止 computer-use、CUA、键鼠/窗口 UI 自动化及模拟拖拽；使用命令行、无窗口原生测试和现有 MCP 协议。真实画面、焦点和听感难以自动验证时，交付步骤并记录“待人工验证”，不要求代理绕过限制完成这些体验项。JSON 加载和格式迁移不在范围内。

## 1. 状态、时间与权限基础

- [x] 1.1 定义 Editing / Playing / Paused、运行会话存在性及统一编辑权限接口，提供状态到按钮可用性的纯数据映射；通过无窗口状态矩阵测试验证 Playing/Paused 均不可编辑，重复操作无副作用。
- [x] 1.2 实现可注入 dt 的 GameClock 与固定步累加规则，保留 0.25 秒有效帧上限；测试 30/60/144 FPS 等有效时长的步数、长暂停恢复、残余累加和新轮清零。
- [x] 1.3 为状态/时钟模块接入 AutomationTests.vcxproj，记录接口与编辑器时间/游戏时间边界；验证原生测试可不创建窗口运行，文档明确 Timer 的游戏与原生用途差异。

## 2. Scene 与同名 Lua 文件管理

- [x] 2.1 实现资产根注入、Windows Scene 名称校验和同名 Lua 路径解析；用临时目录测试中文名、设备保留名、非法字符、路径穿越、尾部字符及大小写冲突。
- [x] 2.2 实现 SceneScriptFiles 的模板生成、已有文件复用和文件/内存创建事务；故障注入测试目录冲突、不可写/发布失败及内存创建失败，验证无部分 Scene、已有字节不变和临时文件清理。
- [x] 2.3 实现场景与 Lua 协调重命名、内存删除保留文件；测试当前场景身份保留、目标冲突、仅大小写重命名、普通失败回滚和删除后脚本仍在。
- [x] 2.4 提供默认 default 创建/激活及编辑态绑定 helper，语法检查只编译不执行；测试首次生成、已有 default.lua 保留、空 Scene 没有脚本实体，以及切换不执行顶层副作用。
- [x] 2.5 在 assets/scripts 的说明文档记录创建/重命名/删除规则及文件名约束；核对示例和生成模板一致，明确 Scene 未自动持久化、旧 JSON 加载未改动。

## 3. 运行 Registry 构建与销毁

- [x] 3.1 重写 CopySceneToRuntime 或提供明确构建器，按两遍映射复制受支持组件、材质参数和层级；真实 ECS 测试验证编辑/运行实体隔离、ID/parent_id 重映射及运行增删不影响编辑数据。
- [x] 3.2 为运行副本新建物理世界、刚体、碰撞体和 user data，提取可复用物理构建核心；真实 RP3D 测试断言指针不共享、初始姿态一致、速度不跨轮保留、运行物理不改变编辑刚体。
- [x] 3.3 为未知组件、旧脚本闭包和无效层级提供构建前拒绝及部分失败清理；测试编辑内容、资源数量和原始对象身份保持不变，错误含目标/组件信息。
- [x] 3.4 为 Registry 增加完整运行重置并规定 context 清理顺序；重复构建/清理及中途故障测试验证不遗留物理世界、相机或旧上下文，不通过序列化恢复。
- [x] 3.5 在场景运行模块文档记录支持的复制组件、共享资产边界和清理顺序；核对复制表覆盖当前可编辑组件，无法复制项均有明确拒绝测试。

## 4. Scene 脚本宿主及运行绑定

- [x] 4.1 将固定 main.lua 加载改为传入同名路径，执行返回回调表并验证三个函数；用真实 Lua 测试模板、错误返回、缺失函数、语法错误和文件修复后重试。
- [x] 4.2 将 Registry/Entity/Camera/FollowCamera 绑定到运行 Registry，并将 delta/tick/Lua Timer 及物理/碰撞开关接入会话上下文；测试脚本修改仅影响运行数据、计时随注入游戏时间变化且不改编辑全局设置。
- [x] 4.3 将运行 AssetManager 新增类 API 改为明确拒绝，检查辅助脚本加载错误传播；测试顶层/on_start 资源新增失败无部分登记，嵌套脚本错误不能被吞掉并伪装成功。
- [x] 4.4 实现 protected on_start/on_update/on_stop 宿主与路径/回调错误诊断，不再创建编辑态 main_script；测试每轮回调计数、模块只在新 Play 重新读取、Resume 不重载，以及回调引用安全释放。
- [x] 4.5 补充新 Lua 回调示例和旧 main.lua/GameDemo 迁移说明；核对示例可由真实 Lua 测试启动，旧文件不被自动改写且资源要求预先导入。

## 5. 会话控制、输入与音频

- [x] 5.1 实现 PlaybackController 的候选运行会话、Play/Pause/Resume/Stop 和主线程 Tick；测试只有初始化全成功才发布 Playing、每帧 update 一次、双渲染消费者不重复模拟及 Stop 后编辑数据完整恢复。
- [x] 5.2 接入脚本失败状态和无条件清理；测试 on_start 失败保留 Editing/原历史，on_update 失败转 Paused 并停止余下模拟，on_stop 失败仍返回 Editing，以及 Playing/Paused 关闭流程清理。
- [x] 5.3 实现 GameInputSnapshot 和可注入焦点/文本捕获许可；用数据输入测试失焦不停止模拟、编辑器输入不透传、工具栏命中不透传，以及暂停/恢复/重新聚焦无旧边沿、鼠标位移或滚轮重放。
- [x] 5.4 实现会话音频所有权、音量边界及 Pause/Resume/Stop；记录型后端测试不误停非会话音频、不恢复脚本已暂停声音、stop(-1) 只限本会话、启动失败释放和新会话无音量污染；可用时补 SDL dummy 后端通道状态验证。
- [x] 5.5 更新运行模块说明中的时间、输入和音频规则，并交付声音/焦点人工验证步骤；核对文档将模拟测试、真实后端状态与待人工听感/焦点体验分开记录。

## 6. 编辑器和双视口接入

- [x] 6.1 将启动初始化改为创建并激活空 default，移除 scene1/scene2 预建；接入统一 Scene 创建/切换/重命名/删除服务，测试启动 helper 和所有操作路径采用相同文件规则，脚本不在编辑态执行。
- [x] 6.2 将模拟 Tick 移至 Application 每帧单次调用，拆除 GameDisplay::Update 与 SceneDisplay::LoadScnne 的运行所有权；通过控制器集成测试验证窗口显示状态不影响 Tick 次数，构建检查无两份播放状态残留。
- [x] 6.3 让 Game/Scene 按状态选择同一可见 Registry 并保留独立相机，区分编辑选择和运行只读选择；接口测试验证 Registry/相机选择和 Stop 后引用清空，真实双视口画面列为待人工验证。
- [x] 6.4 将用户 G 盘 pause.png 转为与现有图标一致的内嵌字节数组，注册 pause_button 并接入三按钮；测试按钮状态映射及嵌入 PNG 可解码，图标外观、高亮、禁用和 Resume 提示列为待人工验证。
- [x] 6.5 在属性、物理 Apply、Gizmo、层级对象操作、Asset/Scene 操作、拖拽和菜单 Open/Save 的实际入口接入 CanEdit；入口级测试验证 Playing/Paused 拒绝且数据不变，人工清单覆盖置灰与交互体验，不修改 JSON 实现。
- [x] 6.6 将 Game 画面焦点和文本/工具栏捕获传给输入许可，保留 Scene 相机真实时间导航；自动测试验证许可规则和独立相机数据，真实焦点切换列为待人工验证；移除数字键 1 额外物理步进并以物理步计数回归验证。
- [x] 6.7 更新编辑器使用说明及人工 UI 清单，包含三按钮、双视口、Pause 导航、拖入 Scene 和只读 Inspector/Gizmo；核对每项均有步骤、预期和“待人工验证”状态，不把清单交付等同人工通过。

## 7. MCP 状态、保护与历史兼容

- [x] 7.1 在 get_editor_status/get_scene_state 增加 playback_state，保留 playing（Paused=true）及编辑快照；更新 generate_contract.py 与两份 tools.json，通过原生结果校验和 Python 契约测试确认旧参数合法、字段一致且仍为 21 个工具。
- [x] 7.2 将写入、set_selection、异步导入提交及 capture_viewport 统一按运行会话保护；原生测试直接驱动控制器，验证 Playing/Paused 的 play_mode、导入跨状态拒绝和排队截图失效，不新增协议播放方法。
- [x] 7.3 将已提交状态切换纳入 revision/历史失效，排除运行帧/运行选择/游戏时间 fingerprint；测试 Play/Pause/Resume/Stop、错误暂停、旧游标失效、运行帧不推进 revision，以及旧成功请求重试仍返回原回执。
- [x] 7.4 更新 create_scene、失败回滚、undo/redo 以复用统一文件服务；使用临时 scripts 根测试创建后外部修改脚本、撤销保留、重做复用/缺失生成及新 Scene ID remap，不产生隐藏实体阻碍空 Scene 撤销。
- [x] 7.5 将原生管道宿主及 Python 集成 fixture 配置为临时资产根，更新默认空 Scene 的相关预期；运行集成测试验证不会向真实 EDITOR/assets/scripts 写入 QA 文件，保留无场景查询和既有场景/资源历史回归。
- [x] 7.6 更新 PROTOCOL.md、README.md 和验证说明中的 Paused 语义、查询作用域、文件副作用及无 computer-use 约束；核对说明与生成工具 Schema 及新增回归一致。

## 8. 跨模块集成验证与交付

- [x] 8.1 使用现有 build.py 分别构建 editor/tests 的 x64 Debug 和 Release，并运行对应 AutomationTests.exe；记录实际命令、结果及失败/环境限制，验证完整模块能够链接且无窗口核心回归通过。
- [x] 8.2 使用 tools/engine_mcp/.venv/Scripts/python.exe -m pytest tools/engine_mcp/tests -q 执行协议集成回归，运行 openspec validate add-scene-playback-lifecycle --strict 和 git diff --check；记录实际结果，不把构建成功当作渲染/音频/界面验收。
- [x] 8.3 在 VERIFICATION.md 汇总自动检查及各组已交付的人工清单，注明 computer-use 未使用、JSON 加载未变；核对真实视觉/焦点/听感无人工证据的项目仍为“待人工验证”，该任务完成只代表记录交付，不代表人工项通过。


## 9. 人工反馈修正：编辑态初始场景构建

本组依据 2026-10-21 用户确认覆盖所有 Scene 的统一 on_build 方案，替代原任务中“创建一律为空/编辑态不执行模块”的假设；切换仍只编译，运行期只读及 Stop 回滚规则不变。

- [x] 9.1 新增可选 on_build 和四回调模板，在 Scene 创建候选中执行一次，成功才发布；测试语法/部分构建/声音/资源写入失败的回滚、切换与重命名不重建、临时 Lua/context 释放。
- [x] 9.2 将 default 构建移至 on_build，增加 Entity.find 与玩家/平台 Bind；真实 Lua/ECS/物理回归先修改编辑态位置、材质、质量，再验证多轮 Play/Pause/Resume/Stop 保留修改及身份、不重复生成对象。
- [x] 9.3 适配 MCP 非空构建 Scene 的创建撤销与重做、逻辑实体重绑及历史预算；回归创建 Scene→修改构建实体→全部撤销→全部重做，并验证脚本保留及旧公开身份失效。
- [x] 9.4 同步方案、规格、使用文档和工具说明，完成 Debug/Release 原生与编辑器构建、SDK/协议回归、严格规格验证及差异检查；记录编辑对象真实 UI 操作仍待人工验证，全程不使用 computer-use。
