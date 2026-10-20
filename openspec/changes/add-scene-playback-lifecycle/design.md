# Design

## Context

动机见 proposal.md。源码调查得到以下约束：

- GameDisplay::PlayGame / Update / RenderGame 当前都访问 GetRegistry()；Stop 只切换两个布尔值。SceneDisplay 也读相同编辑数据，且负责初始化相机、Lua 和物理，生命周期与窗口耦合。
- SceneObject 已有 m_Registry / m_RuntimeRegistry，但 CopySceneToRuntime 逐组件普通复制，没有实体重映射；PhysicsComponent 的 shared_ptr 会共享刚体等资源，ScriptComponent 实体被整体排除。Registry::ClearRegistry 只清实体，不清 context。
- ScriptingSystem 固定 safe_script_file("./assets/scripts/main.lua")，从全局 main 读取回调并创建 main_script 实体。现有 main.lua 顶层加载 GameDemo 和资源，不能在编辑态切场景时执行。
- Lua 的 Registry、Entity、Camera 和 FollowCamera 绑定依赖传入 Registry；输入直接读取全局 InputManager，ENGINE_GetTicks 使用 SDL 时间，Timer 使用 steady_clock，部分 ENGINE_* 开关写 CoreEngineData。资产和声音绑定直接访问全局 MainRegistry。
- Scene 创建、重命名、删除目前只管理内存；MCP 创建事务的失败回滚、undo 和 redo 也走这些入口。空 Scene 的历史检查包含所有实体，因此不能继续在编辑 Registry 中创建隐藏 main_script。
- src/main.cpp 已通过可执行文件向上定位 EDITOR/assets 并设置编辑器工作目录。沿用该定位结果作为资产根，不增加写死 D 盘的路径，也不依赖 MCP 进程 cwd。
- EDITOR/tests/AutomationTests.vcxproj 已有真实 ECS、物理、历史及管道测试，Python 集成通过无窗口 C++ 宿主运行；现有 --gl-tests 可创建隐藏 GL 上下文并使用 dummy 音频。没有必要增加窗口自动化或公共 MCP 播放接口。

## Goals / Non-Goals

**Goals:**

- 状态与运行资源只有一个权威所有者，模拟从窗口 Draw / Update 中移出。
- 复用双 Registry 的结构，保证编辑数据、活动物理资源、Lua 引用和运行选择互相隔离。
- 文件管理覆盖 UI、默认启动及 MCP 创建/历史路径，并保护已有脚本。
- 将可自动验证的行为拆到无窗口模块；真实视觉、焦点和听觉体验允许保留待人工验证。

**Non-Goals:**

- 不实现或修改 JSON 加载、JSON 格式迁移、场景自动保存，也不借 JSON 快照实现 Stop。现有菜单入口只接入播放期禁用保护。
- 不实现运行修改回写、暂停时编辑、运行切场景、脚本热重载、逐帧按钮、公共 MCP Play/Pause/Stop 工具或新项目系统。
- 不保留旧 main.lua 全局 main 回调契约；不自动重写用户 Lua 正文或 GameDemo。
- 不把任意 Lua 文件/网络 I/O 当成可回滚场景数据，不设计通用脚本沙箱；本次隔离的是提供的场景、时间、输入、物理、资产和音频接口。

## Decisions

### 1. PlaybackController 统一状态，RuntimeSession 统一所有权

在 EDITOR 的场景运行模块新增 PlaybackController，提供 Play、Pause、Stop、Tick、GetState、HasRuntimeSession、CanEdit、GetVisibleRegistry 等窄接口。Resume 由 Paused 下的 Play 处理。SceneManager 和 UI / Automation 依赖只读状态与权限接口，不再维护 m_bPlayGame 与 SceneObject::m_bPlay 两份事实；需要保留的 CheckPlay 兼容包装从会话推导，Paused 也返回 true。

RuntimeSession 由控制器唯一拥有，记录运行 Scene、游戏时钟/累加器、脚本宿主、输入门控和音频会话。创建过程使用内部候选会话；模块加载、复制、资源构建和 on_start 全成功后才发布 Playing，失败清理候选并维持 Editing。初始化中的绑定已经是 runtime 模式，不能因公开状态仍是 Editing 而允许写全局资源。

Pause、Resume、Stop 在主线程一致性边界提交。on_update 出错立即进入 Paused，跳过本帧余下物理步骤。重复/无效按钮操作为无副作用 no-op。正常 Stop 执行一次 protected on_stop；回调错误不阻止清理。候选初始化失败清理已建立的资源，不要求对未成功启动的脚本执行正常 on_stop。

替代方案：继续给 GameDisplay 增加布尔值会让切场景、暂停和 MCP 保护产生组合歧义；从窗口驱动模拟会使隐藏窗口影响运行。因此采用会话控制器。

### 2. 重建运行 Registry，禁止物理浅拷贝

保留 SceneObject::m_Registry 与 m_RuntimeRegistry；GetRegistry 保持“编辑 Registry”含义以减少现有自动化歧义，新增明确的可见/运行访问接口，不把所有调用点无条件切成动态 Registry。

运行构建分两遍：先建立 edit entity -> runtime entity 映射，再复制 Identification、Transform、MeshFilter、MeshRender、Light、现有纯数据碰撞组件与物理属性，并重写 entity_id、parent_id 和物理 user data。材质参数及纹理键按值复制；模型、纹理、Shader 等 GPU 资源仍共享只读资产引用。

新建 PhysicsCommon / PhysicsWorld / ContactListener / PhysicsSystem，用属性重建每个刚体与碰撞体，从编辑姿态初始化 previous/current，速度和睡眠状态不继承旧轮次。提取或复用物理构建核心，不能使 CORE 依赖 MCP Protocol，也不为运行构建普通复制 PhysicsComponent。

主脚本由运行宿主持有，不向编辑 Registry 添加脚本实体。未知组件、不能重建的已有脚本闭包或无效层级必须预检查并拒绝 Play，报告具体对象/组件，不能静默遗漏实体。

Stop 先阻止更新/输入并解除运行选择，执行回调和音频收尾，再在 Registry 仍存活时释放 Lua 对象与回调引用，随后清实体/物理并重置整个 context。为 Registry 提供安全重置或 backing-registry 交换能力，不能只调用 clear() 而留下上一轮 context。清理顺序通过重复播放/失败路径测试验证。

替代方案：保存 JSON 再还原无法覆盖 Lua、运行上下文和全部组件，且 JSON 加载本次明确排除；使用现有 meta copy 会共享物理指针。

### 3. 一次模拟更新，两个渲染消费者

Application 每帧在编辑器时间更新及自动化一致性处理后调用控制器 Tick 一次，再由两个窗口 Draw 渲染。GameDisplay 不再负责脚本或物理 Update；SceneDisplay 的 LoadScnne 初始化拆到场景/运行服务，避免依赖窗口是否打开。

Editing 时两个窗口读编辑 Registry；Playing / Paused 时两个窗口读同一运行 Registry。编辑场景拥有用于 Game 预览的初始游戏相机配置，运行会话独立构造游戏相机；Scene 相机留在视图层，不被运行脚本访问。Stop 后恢复编辑预览相机，Scene 相机导航位置不随 Stop 回滚。

ToolContext 区分编辑选择和运行只读选择；成功进入会话清空编辑选择，运行 Inspector 只读，Gizmo 不提供写入。Stop 清空运行选择，所有 Registry 外部引用失效后才释放运行数据。MCP 的 get_selection 继续表示编辑选择，运行期间返回 null，不发布运行实体的编辑 ID。

### 4. 游戏时钟与固定物理步长

引入可注入帧 dt 的 GameClock。Playing 取有效 dt（保留现有单帧 0.25 秒上限），推进游戏时间一次、调用 on_update(dt) 一次，再以 1/60 秒步长消费累加器并同步物理结果；限制补算步数与帧上限匹配，避免无界循环。首版不用插值，PhysicsSystem 同步最后一步姿态。

Pause 保留不足一步的累加余量，但不加入暂停时长；Resume 后只处理新帧 dt。Stop/新会话重置时钟与余量。Lua ENGINE_DeltaTime、ENGINE_GetTicks 以及 Lua 创建的 Timer 注入会话游戏时钟；编辑器 UI、相机及原生通用计时用途继续使用真实时间，不全局修改所有 Timer 的语义。

Lua 暴露的物理启用和碰撞显示状态放入会话设置或运行世界，避免写 CoreEngineData 后污染下一轮。检查 Camera process_key 等隐式使用全局 dt 的路径，改用游戏时间。移除数字键 1 调试路径的 physicsWorld->update；调试显示变化不得推进模拟。

替代方案：每帧无条件推进 1/60 秒会依赖 FPS；只停 Update 而保留 SDL tick/steady_clock 会使恢复后的计时突跳。

### 5. 输入和音频通过会话适配器访问

Game 画面的焦点、文本捕获和工具栏命中转成可注入的输入许可，不直接修改编辑器 InputManager。Lua 读取 GameInputSnapshot；未获许可时返回无游戏输入，窗口失焦、Pause/Stop 时清边沿、位移和滚轮，重新获得焦点同步基线，不制造旧按键的新 just_pressed。UI 焦点体验留人工检查，许可到游戏状态的规则用数据驱动测试。

音频适配器追踪本会话的音乐和实际通道所有权，播放、停止和音量操作限定所属范围，不把 SoundFx.stop(-1) 或全局音量写入直接下发到所有声音。Pause 记录由会话暂停的声音集合，Resume 只恢复该集合。Stop/启动失败终止本会话音频并恢复本会话触及的音量状态，不操作未拥有声音；底层单音乐槽若已被非会话音乐占用，则拒绝抢占并报告诊断。

测试使用记录型后端核对调用与所有权；条件允许时使用 SDL dummy 音频验证真实通道状态。实际听感与设备效果仍列为待人工验证。

### 6. SceneScriptFiles 管理文件，ScriptingSystem 管理执行

新增 SceneScriptFiles 或等价模块，使用明确资产根与可替换文件系统接口。校验 Windows 名称、设备名、尾部字符和大小写不敏感冲突，脚本路径固定为 assets/scripts/<name>.lua。支持中文路径，不写死当前机器 D 盘。

创建先准备文件与 Scene 候选，再整体发布与激活；缺失文件使用同目录临时文件、排他创建和不覆盖发布，已有普通文件只绑定。已存在文件内容即使语法错误也保留，诊断后仍可编辑空 Scene，Play 再检查；文件 I/O 失败才使创建事务失败。失败回滚只清理本次拥有的临时/模板文件，不删除用户既有内容。

重命名在预校验目标与预分配内存状态后同步文件和内存名称，使用不覆盖文件操作；包含仅大小写变化的重命名可用同目录唯一临时名过渡，普通失败回滚原路径。删除和 undo 不删除 Lua；redo 复用当前文件字节，缺失才生成模板。脚本外部编辑不进入 MCP 内容历史，也不被场景 undo/redo 覆盖。scripts 目录中的 Lua 不代表自动恢复全部内存 Scene，启动仍只创建空 default。

编辑态语法检查使用 load_file/编译而不调用编译结果，不开放运行绑定。Play 创建新 Lua 状态、注册指向运行 Registry 的绑定、执行模块、检查三个函数并持有 protected 回调，再执行 on_start。新模板返回三个空回调。嵌套 ENGINE_RunScript/LoadScriptTable 等辅助加载也必须传播失败，不能吞掉错误后报告启动成功。

Lua AssetManager 新增类 API 在 runtime 模式明确拒绝；所需共享资源通过编辑器现有导入流程预加载。原 main.lua / GameDemo 文件保留为旧例子，不自动执行、不覆盖、不做自动迁移。

替代方案：把文件写入藏在低层 map AddScene/DeleteScene 中会破坏历史回滚语义；按 UI 单独创建文件会遗漏 MCP 和 redo 路径。

### 7. 所有入口共享编辑保护和版本规则

在实际操作入口统一 CanEdit 检查，UI 置灰只是提示。覆盖属性/物理 Apply、Gizmo、对象创建删除、Asset 导入/重命名、Scene 增删改切换、拖拽与菜单 Open/Save；Playing 和 Paused 都不可写。现有 JSON 读写实现保持原状，不为本次修改格式或加载行为。

EditorOperations 的状态保护与异步导入提交重校验使用运行会话状态。保持请求去重优先级：旧成功请求的相同重试返回原回执，不能被新的 Paused 状态误判为需要重执行。

get_editor_status 和 get_scene_state 的 data 新增 playback_state=editing|playing|paused；旧 playing 表示运行会话存在，Paused 为 true。场景列表条目的旧 playing 根据会话所属 Scene 推导。快照始终描述编辑 Registry，不能发布运行实体 ID。生成器和两份 tools.json 同步更新，21 个工具和既有合法请求保持。

成功 Play、Pause、Resume、Stop 及错误转入 Paused 均通过统一状态通知推进 revision 并清空 MCP 历史；失败 Play 没有已提交状态变化，不清空原历史。编辑 fingerprint 只包含编辑内容/资源/当前场景/播放状态，不 hash 运行实体、游戏时间或运行选择。截图只允许 Editing，排队捕获遇状态变化按 revision_conflict/play_mode 收尾。

Scene 创建历史继续记录逻辑身份和旧场景，undo 保留脚本，redo 绑定当前文件并返回新 ID remap；脚本不进入实体数量判断。默认场景启动不写入 MCP 用户历史。

### 8. 验证禁止 computer-use，按证据类型分层

本变更不调用 computer-use、CUA、浏览器/桌面键鼠、模拟拖拽或 UI 窗口控制；不通过新 MCP 工具、脚本点击、按键注入等绕过约束。

| 层级 | 自动化方法 | 可证明内容 | 不能替代的人工项 |
| --- | --- | --- | --- |
| 构建 | 现有 build.py 的 editor/tests，x64 Debug/Release | 编译链接与项目依赖 | 按钮和画面 |
| 原生模块 | AutomationTests 增加 Playback/SceneScript 测试，临时资产根、可控 dt、真实 ECS/RP3D/Lua | 三状态、隔离、生命周期、物理、时钟、文件事务、错误清理、保护 | 实际窗口焦点、图标 |
| 后端 | 记录型音频后端；可用时 SDL dummy/隐藏 GL 的现有测试模式 | 通道所有权、暂停状态及后端资源数量 | 真实声音和双窗口显示 |
| 协议 | 现有原生操作测试与 Python pytest/管道宿主 | 新旧状态字段、21 工具、Paused 拒绝、历史/去重/捕获边界 | 真实按钮点击 |
| 人工 | 验证文档列步骤、预期、证据与待验证状态 | 真实交互和感官结果 | 不得由自动逻辑测试标记完成 |

原生测试从控制器公开 C++ 接口驱动状态，不向生产协议增加播放方法。测试所有文件写入使用临时 scripts 根，不在仓库真实 assets/scripts 批量留下 QA 文件。现有集成测试宿主需配置临时资产根，避免新增 create_scene 副作用污染工作区。

待人工验证至少包括：三按钮图标/高亮/提示；同时显示两个窗口观察相同运动；Pause 后 Scene 导航与恢复无突跳；Game 画面/Scene/文本框/工具栏之间焦点切换；真实音乐/音效暂停与停止；拖入 Scene、属性/Gizmo 禁用体验。apply 即使完成自动检查，也只将“提供人工清单”任务标记完成，记录中这些项目仍保持待人工验证直到有用户实测证据。

## Risks / Trade-offs

- [普通组件复制共享物理指针或旧实体 ID] → 显式复制表、两遍映射、真实物理所有权断言；未知组件先拒绝，禁止静默降级。
- [Registry context、sol 闭包和 UI Entity 引用跨 Stop 悬空] → 统一会话清理顺序及 backing-registry 重置，覆盖重复播放、清空实体、初始化/回调失败和关闭。
- [Lua 仍通过全局时间、物理开关、资源或声音接口写出运行边界] → 注入运行绑定上下文并审计已有 helper；全局资源写入明确失败，音频与时钟限定会话。
- [新 Scene 文件副作用破坏历史原子性] → 统一文件事务、临时资产根与故障注入；撤销/删除只移除内存 Scene，所有已有文件保留。
- [旧 GameDemo 不再自动运行] → 显式记录兼容性变化并提供新模板/迁移说明，资源先导入，不伪装旧脚本可直接兼容。
- [真实焦点和声音体验不能由无窗口测试证明] → 将体验项目独立标为待人工验证，不启用 computer-use、不宣称已通过。
- [极端卡顿触发帧上限而丢弃部分真实时间] → 明确有效 dt 上限，只保证正常帧率下等有效时长的模拟一致，暂停时长始终不进入游戏时间。

## Migration Plan

1. 先完成文件管理、运行构建和控制器的无窗口模块及测试，再接入 Application、两个视口和三按钮，避免中途同时保留两份状态真相。
2. 接入新 Lua 回调契约和会话绑定；启动改为空 default，首次运行生成缺失模板。旧脚本和 Scene JSON 文件不自动改写。
3. 同步 UI/自动化保护、状态契约及历史路径，运行构建和回归，更新验证记录及人工清单。
4. 回退通过撤回代码变更并重新构建，默认启动与旧 main.lua 行为恢复；新生成和用户编辑的 Lua 文件保留，回退不删除用户文件。
