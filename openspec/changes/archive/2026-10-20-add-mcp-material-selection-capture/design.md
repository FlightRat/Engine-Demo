# Design

## Context

动机和用户范围见 `proposal.md`。当前基础是 Python stdio MCP → bridge-v1 当前用户命名管道 → 主线程 `CommandQueue` → `EditorOperations`。

已核对的实现约束：

- `EditorOperations::EntityJson` 已返回完整 `materials` 数组，包括 RGB、独立 alpha、PBR 数值、开关及纹理键；缺少显式槽索引，因此不新增重复的材质查询工具。
- `MeshRender::GetMaterial(index)` 会在越界时扩展数组，不能用于验证外部索引。纹理槽定义在 `TextureRegistry`；`Texture` 目前没有公开类型查询，绑定前需增加只读类型访问以拒绝非二维纹理。
- `ComponentDrawer` 已通知颜色、数值和开关修改，但纹理绑定赋值没有通知，浏览面板还会通过 `map::operator[]` 插入空键。需要把实际提交和纯读取分开。
- 选择同时存于 `ToolContext::selectedEntity`、`Identification::selected` 和 `SceneHierarchyDisplay::m_pSelectedEntity`。现有 ResetSelection 只清工具指针并通过序号通知面板，直接改一个位置会导致不同步。
- 内容 Stamp 不含 selected 标志，也不含相机。选择可以保留为独立界面状态；当前所有写方法却默认进入内容提交和历史替换路径，必须明确分流。
- `SceneDisplay::Draw` 在 Scene 标签隐藏时直接返回；默认中央显示 Game。`SCENE_FXAA` 只有在 Scene 实际绘制时才更新，读取该纹理不足以证明新帧。
- 渲染管线已接受一组显式 framebuffer 和 camera；可以复用管线，但需隔离捕获资源和相机尺寸。OpenGL 4.5 上下文在主线程，`Application::Render` 的 AfterUI 位于 ImGui 渲染后、窗口交换前。
- 单消息上限为 1 MiB，写账本 4096 条不驱逐。Server 当前只输出文本和 structuredContent，不能把图像当普通 JSON 文本发送。

## Goals / Non-Goals

**Goals:**

- 为四个新增工具建立可直接实现的共享契约，同时保持旧工具参数、单消息上限和实例隔离不变。
- 内容事务、界面选择及只读图像分别进入适当执行路径，保持统一的会话与显式目标规则。
- 用真实 Scene 渲染产生图片；默认 Game 前台和 Scene 不可见均能完成捕获，不依赖桌面窗口截图权限。
- 保持材质变更可验证、可批处理、可撤销，选择和截图不消耗内容历史。

**Non-Goals:**

- 不增加通用组件写入或脚本执行，也不把物理变换限制扩大为本次工作；材质编辑不修改物理状态。
- 不承诺 alpha 参数会启用新的透明渲染路径，沿用当前渲染器行为。
- 不控制或自动聚焦相机，不新增 UI 自动切标签；截图可能包含视野之外的物体，返回相机信息供判断。
- 不扩展前一归档变更的完成记录，也不把无窗口宿主测试报告为真实画面验收。

## Decisions

### 1. 四个工具及明确的参数边界

| 工具 | 输入 | 结果和状态规则 |
| --- | --- | --- |
| `set_material` | session_id、request_id、expected_revision、scene_id、entity_id、material_index，以及至少一个修改字段 | 内容写入，返回实际实体/材质、operation_id 和新 revision，纳入账本与单层撤销 |
| `get_selection` | 空对象 | 返回当前 scene_id、entity_id/name（可空）、selection_serial，外层带 session/revision；无激活场景时 scene_id 为空、选择为空 |
| `set_selection` | session_id、request_id、expected_revision、scene_id、entity_id（字符串或 null） | 界面写入，返回选择回执和独立 operation_id；不推进内容 revision、不替换内容历史 |
| `capture_viewport` | session_id、scene_id、expected_revision；可选 width/height | 只读图像请求，无 request_id、无 operation_id，不入写账本；返回通用结果外壳中的元数据及 MCP 图片块 |

材质字段为 `color:[r,g,b]`、`alpha`、`metallic`、`roughness`、`ao`、`use_texture`、`textures`。所有颜色/标量为有限 [0,1] 数值；未知字段拒绝。textures 为非空的部分槽映射，允许键固定为 albedo/normal/metallic/roughness/ao，值为已加载二维纹理键或 null。null 规范化为内部空字符串；保持 use_texture，除非显式提供该字段。shading_model 只读，不编辑可见性或 flip_uv。

替代方案：增加 get_materials 会重复现有快照；用任意组件路径写入会绕过类型规则；默认把当前选择当所有写命令的隐式目标会引入用户切换选择时的误修改。保留显式 entity_id，助手通过 get_selection 获取它。

### 2. 材质复用内容事务和 UI 提交规则

- 在材质数组范围内先验证 entity/component/index，再复制该槽建立候选值；禁止通过 GetMaterial 自动扩容。完整预校验包括纹理存在性和可采样二维类型，失败不写任何字段。
- 给材料/纹理引入小型共享校验和提交辅助函数，供属性面板与 automation 使用；无需把 MCP 协议移入 CORE。
- 场景查询逐槽添加 material_index，保留已有字段名称和格式。纹理类型使用 Texture 的只读类型访问，不在 IPC 线程查询 GL 状态。
- 扩展批次虚拟实体保存必要材质列表，支持 set_material 的 entity_id 或向后 target_ref，创建 mesh 的预校验可计算其初始材质。记录完整旧材质作为逆操作，普通失败恢复内容并保留原历史；不可恢复错误继续进入 recovery_required。
- 属性面板使用 find 读取纹理绑定；只在选择实际改变时提交并通知。颜色和数值也经同一约束，取消/仅展开控件不产生修改。

替代方案：直接赋值后让快照观察器兜底容易遗漏无效 UI 值和浏览时写入；全场景序列化回滚会继承既有存取遗漏。继续采用最小属性逆操作。

### 3. 单选采用统一的主线程服务

新增编辑器侧 SelectionState/等价服务，保存“场景弱引用 + 带代数实体句柄”和单调 selection_serial，不保存跨会话身份。外部 ID 的解析/生成仍由 EditorOperations 负责。

统一入口维护 ToolContext、旧/新 Identification::selected，并让层级面板从共享状态同步本地包装，不让面板的旧指针把 MCP 选择清掉。人工点击、空白清空、MCP 选择、实体删除、场景加载/切换及撤销清理均走这个入口。选择相同目标为无变化，selection_serial 不增加。

内容 Stamp 和历史 after 值继续排除选择状态；选择和清空不发 NotifyManualEdit。场景切换本身仍推进内容 revision 并使历史失效，不能把清空选择例外误用为场景上下文例外。

set_selection 属于带请求回执的写操作，加入 C++ IsWrite 和 Python WRITES，使用原有容量、TTL、取消、去重先于 revision 的行为；在内容提交分支前单独处理。成功分配 operation_id 供回执关联，但不写入内容 history；undo_operation 对此 ID 返回 history_conflict 且保留真正内容历史。恢复故障和播放模式仍拒绝选择写入。重复回执不会重新选中目标，即使用户已手动改选。

替代方案：每次选择都推进 editor_revision 会使简单“选中再撤销材质”失效；绕过账本则会使断连重试覆盖后来的人工选择。选择序号负责可观察性，现有写回执负责去重。

### 4. 捕获在渲染阶段异步完成，隐藏 Scene 也生成新帧

在 automation 和 SceneDisplay 之间新增有界 CaptureCoordinator/等价接口。CommandQueue 的执行结果支持显式延迟完成，保留原有同步路径；不得在 Process 中等待 render future。

执行顺序：

1. 管道受理截图，验证共享参数并预留唯一捕获槽。主线程校验 session/current scene/revision/play/recovery 下的只读规则，建立待渲染请求。没有渲染提供者的原生宿主返回 capture_unavailable。
2. AfterUI 先 ObserveExternalChanges，再处理捕获；此时主窗口 GL 上下文有效。重新核对场景和 revision，复制 Scene 相机的姿态与镜头参数，为输出尺寸调整副本的宽高。
3. 使用捕获专用 GBuffer、AO、AOBlur、Shade、FXAA framebuffer 运行同一 DeferredRenderPipeline。缓存一组有上限资源、按需分配/重建，不改 SCENE_* 的尺寸，不绘制 ImGui/Gizmo。
4. 在主线程以 RGB8、pack alignment=1 将最终 FXAA 输出读入 PBO，放置 fence；后续帧以零等待轮询 fence，完成后复制有界 CPU 像素。全程不等待模型、IPC 或编码完成；保存并恢复 framebuffer、viewport、pack buffer/alignment 及管线触及的 GL 状态。共享渲染缓冲按正常各 pass 准备规则更新，下一次可见渲染重新准备相机/灯光数据。
5. 成功捕获分配 frame_id，冻结对应 session/scene/revision/camera/尺寸元数据。像素已冻结后发生的编辑不使这张历史帧伪装为新状态；客户端使用返回 revision 判断。排队至实际读回前若检测内容或目标已改变，则失败并丢弃该次图像。
6. 工作线程只处理不可变 CPU 数据和内部 Base64/JSON，完成延迟 ticket；不得访问 ECS、ImGui、Logger 可变数据或 GL。Python 解码校验并编码 PNG，再转换为 MCP 图片结果。

CaptureCoordinator 截止期为服务端受理后 5 秒，包含等待开始和 fence 轮询；超时以 capture_timeout 结束，晚到结果丢弃。进程内停止标志/连接代数防止断连或 shutdown 后完成到错误连接。GPU 删除遵循有效上下文的 owner-thread 清理；shutdown 先停止受理、取消 ticket、清理捕获资源并结束 CPU worker，再释放场景与 GL。保证 make-current 发生在捕获资源清理之前。

替代方案：直接读取 SCENE_FXAA 会在 Game 前台返回旧帧；强制切到 Scene 会打断用户；另造简化渲染器会导致材质/灯光效果不一致。使用相同管线和独立有界目标，成本是一次额外场景渲染。

### 5. 图像传输保留 bridge-v1 与 1 MiB 上限

width/height 默认 640/360，输入为 64..1024 整数。超过 230400 像素时以统一比例缩小并向下取整，最大维度不超过请求值；元数据必须返回实际宽高。

- 读回 RGB8 最多 230400 × 3 = 691200 字节，Base64 最多 921600 字节；内部元数据/JSON 外壳预算最多 32 KiB，因此仍小于 1 MiB。发送前核对完整 UTF-8 消息长度，不能仅检查像素字段。
- 内部截图数据包含 encoding=rgb8、origin=bottom_left、width、height、pixels_base64 和冻结的 metadata。Python 严格验证 Base64、精确的 width×height×3 长度、像素上限和元数据预算；不接受任意文件路径或 URL。
- 标准库构造 PNG：翻转行、每行 filter=0、zlib 压缩、写 IHDR/IDAT/IEND 与 CRC；RGB、不带 alpha。压缩/编码放到有容量限制的工作线程，不阻塞 stdio 事件循环；PNG 上限 700 KiB，超限返回 capture_too_large。
- 契约生成器为截图区分内部 bridgeOutputSchema 和公开 outputSchema，public_tool 移除内部字段，Server 先校验桥接数据再构造公开结果。其他工具保持既有 outputSchema 路径。
- 公开结果 content 为短元数据 TextContent + 一个 ImageContent(image/png)，structuredContent 仅保留结果外壳和元数据，不复制 Base64。成功元数据包含 session_id、scene_id、editor_revision、frame_id、width、height，以及 position/front/up、fov_degrees、near/far 的 Scene 相机快照。错误不含图片块。
- C++ 捕获槽在内部响应交付/丢弃前占用；Python Server 的单捕获门闩覆盖到 PNG 输出完成，额外请求立即 capture_busy。保持单客户端实例限制，没有持续帧流和图片账本。

替代方案：提高全局消息限制会影响全部工具；图片分块和独立文件资源会增加路径/生命周期协议。首版以有界缩略渲染换取原桥接兼容和确定的内存占用。

### 6. 验证按实际证据分层

- 原生 ECS/队列：多材质槽部分更新、二维纹理约束、材质逆操作/故障回滚、target_ref、选择与内容 revision 分离、统一选择序号、请求重试不覆盖人工选择、取消及恢复故障写保护。
- Python：共享生成契约一致性、旧十工具兼容、十四工具发现、图片内部/公开 schema 分离、RGB/PNG 尺寸/方向/CRC/大小限制、错误时无 ImageContent、stdio 文本不泄漏图片。合成像素仅证明编码路径。
- 真实编辑器：Debug/Release 均构建；在 Scene 可见和默认 Game 前台两种情况下取图，检查新材质可见且不是旧帧；对比相机、选择、revision、frame_id 和撤销；人工属性面板/纹理修改后拒绝旧撤销；关闭和断连时无悬挂。
- 连续验收仅修改唯一 QA 场景。用已加载 cube 和 point light，人工选择与 MCP 改选/清空各验一次；材质批次 → 截图 → 查询 → 选择变化 → 撤销 → 再截图/查询。原始工具结果、PNG、人工观察分别记录，未完成项保持未勾选。

## Risks / Trade-offs

- [额外渲染和 GPU 读回影响帧耗时] → 单未完成请求、最多 230400 像素、PBO/fence 零等待轮询；记录真实耗时，不承诺每次捕获符合现有队列 2 ms 的普通命令预算。
- [共享渲染状态被捕获污染] → 使用相机副本/独立 framebuffer、GL 状态守卫及下一帧 Scene/Game 回归，覆盖 Game 前台和窗口尺寸变化。
- [截图分辨率不足以检查细节] → 明确返回缩小尺寸；本版用于外观和布局检查，高分辨率分块协议留待后续独立变更。
- [选择分支意外清除历史] → 独立控制流和回归测试覆盖材质修改后改选/清空/截图仍可撤销，UI 选择不调用内容通知。
- [多材质索引或纹理类型误判] → 每次执行验证当前向量长度和资源类型，使用 revision 防止模型切换后使用旧索引，不以文件扩展名或名称猜类型。
- [PNG 编码实现或数据边界错误] → 只支持 RGB/单种过滤的最小格式，以独立解码验证、CRC、随机像素上限和损坏输入测试约束；不借用当前仅含 stb_image 的解码依赖假定其可写 PNG。
- [部分 UI 交互缺少实测] → 验收任务单独保留，不能用图片桩或字段 visible=true 代替真实画面/面板同步证据。

## Migration Plan

1. 保留现有已归档规格，先扩展共享契约和旧接口回归，再依次接入材质、统一选择与捕获。仅在功能交付后同步本变更的 delta specs。
2. 重建 AutomationTests 与完整 EDITOR 的 x64 Debug/Release，部署匹配的 C++ EXE、Python 包和生成契约；工具集从十个增至十四个，客户端需重新加载工具目录。
3. 旧客户端仍可调用既有十个工具；新客户端连接旧 EXE 时新增工具明确报 unknown_method/参数错误，不做隐式降级。桥接 framing 仍为版本 1，不声称新增工具在旧 EXE 可用。
4. 回退时一起恢复本次代码、包和契约并重新构建；不需要迁移场景文件，也不会因截图产生用户文件。现有桥接默认关闭及显式实例选择保留。
