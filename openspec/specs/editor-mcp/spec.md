# editor-mcp Specification

## Purpose

使本地 AI 助手通过标准 MCP 工具连接明确选定的编辑器实例，发现和调用场景自动化能力，并在排队、超时、重连与编辑器重启时保持清晰的请求结果，使用户能够用现有 Codex 工作流可靠地操作运行中的编辑器。

## Requirements

### Requirement: Discoverable local MCP tools

系统 SHALL 提供本地 stdio MCP Server，在现有 get_editor_status、get_scene_state、list_assets、create_scene、create_object、set_transform、set_light、apply_operations、undo_operation、get_operation_status、set_material、get_selection、set_selection、capture_viewport 外新增 add_physics、set_physics、remove_physics、delete_entity、get_history、redo_operation、import_asset。工具 MUST 提供 Schema、字段约定及结构化结果，拒绝未知字段，保持现有 14 个工具的合法请求兼容。

#### Scenario: Codex discovers editor tools
- **WHEN** 支持的本地 Codex 会话加载 Server 配置并执行工具发现
- **THEN** 能发现全部 21 个工具及必填参数，知道坐标、角度、物理、删除恢复、资源作用域、历史及截图约定

#### Scenario: Invalid tool arguments
- **WHEN** 工具调用缺少必填字段或包含未知字段
- **THEN** 返回参数错误，不向编辑器执行部分或推测性修改

#### Scenario: Selection and capture annotations
- **WHEN** 客户端读取工具的说明与注解
- **THEN** get_history/get_selection/capture_viewport 标记只读；set_selection 只改变 UI 且无内容撤销；物理、删除、导入及 undo/redo 明确为写入，所有工具仅作用于指定实例

### Requirement: Explicit local editor instance connection

编辑器桥接 SHALL 默认关闭，仅显式启用后接受本机当前用户的连接。Server MUST 连接指定实例，不自动启动编辑器或静默改连其他实例；不接受远程客户端。同名实例冲突和已有客户端占用 MUST 明确报告。

#### Scenario: Bridge is disabled or editor is closed
- **WHEN** Server 已启动，但指定编辑器未启动或未启用桥接
- **THEN** 工具仍可被发现，状态查询明确返回 editor_unavailable，编辑请求不执行

#### Scenario: Multiple editors are running
- **WHEN** 同时运行不同实例名的编辑器，助手配置指定其中一个
- **THEN** 所有查询和修改仅到达该实例，实例断开时不改连其他编辑器

#### Scenario: Duplicate instance or connection
- **WHEN** 相同实例名已占用，或该实例已有桥接客户端
- **THEN** 新启用或连接请求收到明确冲突错误，原有连接不被替换

### Requirement: Versioned bounded local messages

本地桥接协议 SHALL 声明版本、消息边界和大小上限。系统 MUST 正确处理分段读写，拒绝未知版本、非法 JSON、无效长度或超过 1 MiB 的消息，且不得由非法消息触发引擎修改。

#### Scenario: Fragmented message delivery
- **WHEN** 一条合法消息分多次读取到达
- **THEN** 系统只在完整消息到达后派发一次请求并关联正确响应

#### Scenario: Malformed or incompatible peer
- **WHEN** 对端提交非法长度、超大消息或不支持的协议版本
- **THEN** 系统返回可识别错误或关闭该连接，场景不变且手动编辑器继续工作

### Requirement: Nonblocking editor execution

系统 SHALL 在不等待模型或 IPC 的情况下持续处理窗口和渲染。查询快照与写命令 MUST 在编辑器状态一致的执行点完成；等待队列 MUST 有限，容量不足时明确拒绝新请求。外部资源准备 SHALL 不占用编辑器主循环，发布及所属线程资源创建 MUST 有工作量边界；导入准备期间查询、选择和普通编辑 SHALL 可处理。

#### Scenario: Slow client does not freeze the editor
- **WHEN** MCP 客户端等待模型或暂停读取结果
- **THEN** 编辑器仍可交互和渲染，通信等待不占据主线程

#### Scenario: Queue is full
- **WHEN** 请求数量超过队列容量
- **THEN** 新请求返回 queue_full，未接收的请求不在稍后偷偷执行

#### Scenario: Inspect during resource preparation
- **WHEN** 模型导入正在后台读取和解析
- **THEN** get_editor_status、get_scene_state 和 get_history 可返回一致的已提交状态，渲染与交互继续

### Requirement: Request result and error contract

每个已接收写请求 SHALL 返回 request_id、session_id、状态及 editor_revision；成功变更 SHALL 包含 operation_id 和实际结果。错误 MUST 提供 code、message、details 和 retryable，区分连接不可用、目标失效、版本/参数错误、编辑冲突及执行失败。

#### Scenario: Write result can be verified
- **WHEN** 创建或修改成功
- **THEN** 助手可使用返回 ID 查询实际状态，不必根据自然语言成功描述猜测结果

#### Scenario: Domain error is preserved
- **WHEN** 编辑器拒绝不存在的资源或失效的实体
- **THEN** MCP 返回对应 asset_not_found 或 stale_target 及错误详情，不把它改成成功文本

### Requirement: Retry safe write requests

系统 MUST 在同一编辑器会话中按 request_id 和完整请求内容去重，先查询去重记录再验证旧 revision。相同内容返回原状态或结果，不同内容返回 request_id_conflict。去重记录 MUST 跨 Server 重连保留；达到有界账本容量时 MUST 拒绝新写入，不驱逐旧记录使其可重复执行。

#### Scenario: Response is lost after object creation
- **WHEN** 对象已创建但响应丢失，Server 重连后提交相同 session、request_id 和内容
- **THEN** 返回原结果与相同 entity_id、operation_id，不创建第二个物体

#### Scenario: Request ID is reused for another action
- **WHEN** 同一 request_id 被用于不同参数或 expected_revision
- **THEN** 返回 request_id_conflict，不执行新动作

#### Scenario: Deduplication ledger is full
- **WHEN** 账本达到配置上限并收到未知写 request_id
- **THEN** 返回 request_capacity_exceeded，已知请求仍可查询和安全重试

### Requirement: Observable timeout and cancellation states

系统 SHALL 区分 queued、running、succeeded、failed 和 cancelled，并通过 get_operation_status 返回状态与最终结果。开始前过期或成功取消的请求 MUST 永不写入；等待超时 MUST 不被视为执行失败。导入准备阶段 SHALL 保持同一 running 请求，提交前成功取消才保证无副作用；运行中不得盲目重试为新写操作。

#### Scenario: Command expires before execution
- **WHEN** 排队命令超过有效期且尚未开始执行
- **THEN** 最终状态为 cancelled，主线程恢复处理后也不执行它

#### Scenario: Result wait times out during execution
- **WHEN** 写请求已 running 但 Server 等待结果超时
- **THEN** 返回 pending 状态、原 request_id 和查询方式；后续状态查询得到真实完成结果

#### Scenario: Cancellation after execution starts
- **WHEN** 客户端在同步操作或导入提交开始后取消等待
- **THEN** 系统不宣称操作已取消，保留可查询的最终事务结果

#### Scenario: Cancel import preparation
- **WHEN** 导入尚未发布资源且成功取消
- **THEN** 请求变为 cancelled，候选被回收，不登记资源、不推进 revision、不产生历史

### Requirement: Predictable restart and shutdown behavior

Server SHALL 能重连同一编辑器会话；编辑器重启后 MUST 暴露新 session 并拒绝旧写请求。编辑器关闭时 MUST 停止接收并取消未开始命令，在释放场景资源前结束桥接状态访问；断连 MUST 不触发新写入或自动重放。

#### Scenario: Editor restarts while Codex stays open
- **WHEN** 编辑器进程退出并以同一实例名重新启动
- **THEN** 状态查询显示新 session，旧实体、游标和请求不能修改新会话

#### Scenario: Shutdown with queued requests
- **WHEN** 编辑器在请求仍排队时关闭
- **THEN** 未开始请求不再执行，Server 获得关闭或连接不可用结果，编辑器关闭不挂起

### Requirement: Codex setup without an engine model credential

交付物 SHALL 包含可复现的 Windows 依赖安装、编辑器桥接启用、Server 启动及 Codex 配置示例。模型调用 SHALL 由 Codex 承担，引擎和 Server 不要求模型 API Key。配置示例 MUST 不自动修改用户全局配置，Server stdout MUST 仅包含 MCP 消息。

#### Scenario: Follow setup on a local Windows project
- **WHEN** 用户安装声明依赖、启用指定编辑器实例并在本地 Codex 配置 Server
- **THEN** 可发现工具并查询真实场景，无需另行配置引擎模型密钥或公网服务

#### Scenario: Logs do not corrupt stdio
- **WHEN** Server 报告连接或执行诊断日志
- **THEN** 日志使用 stderr，MCP 消息仍能被客户端正确解析

### Requirement: End to end scene editing acceptance

系统 SHALL 提供可重复验收步骤，在本地 Codex 查询资源后创建并激活展示场景、放置已有模型、创建暖色点光源、查询确认并撤销最新批次。验收 MUST 记录工具执行与编辑器可见结果，不能仅以配置注册成功作为连接成功。

#### Scenario: Assemble and undo a display scene
- **WHEN** 用户预加载茶壶或另一个测试模型并按说明执行自然语言搭建请求
- **THEN** 视口显示模型与有效暖色灯光，查询值匹配；撤销创建物体和灯光的批次后两者消失，场景保留

### Requirement: Typed image results with bounded metadata

成功截图 SHALL 同时返回一个 image/png 类型的 MCP 图片内容块、简短文本元数据和不含图片字节的 structuredContent。元数据 MUST 包含实际会话、场景、内容 revision、frame_id、图片尺寸与 Scene 相机信息。图片字节 MUST 不混入文本 JSON、结构化元数据或日志；失败 MUST 沿用结构化错误且不返回图片块。其他工具 SHALL 保持原有文本与结构化结果。

#### Scenario: SDK and Codex receive a screenshot
- **WHEN** capture_viewport 成功
- **THEN** 客户端可解码图片块为 PNG，解码尺寸与元数据相符，文本和 structuredContent 均不含原始像素或 Base64 图片字符串

#### Scenario: Invalid image transport data
- **WHEN** 图片响应的格式、尺寸、长度或元数据不符合契约
- **THEN** Server 返回结构化错误，不展示伪造或不完整图片，不把内部数据异常当作成功

### Requirement: Extended workflow acceptance

交付物 SHALL 提供一次可重复的本地 Codex 验收：查询材质与人工选择、选择测试物体、批次修改材质、获取 Scene 图片、查询确认及撤销。验收 MUST 区分自动测试、真实渲染证据和人工界面证据，并覆盖默认 Game 前台时的截图；不得以 headless 图片桩替代真实编辑器截图结果。

#### Scenario: Style and inspect a selected mesh
- **WHEN** 用户按说明在真实编辑器中选中测试 mesh，并让 Codex 修改材质、截图和撤销
- **THEN** 选择身份正确，查询值与画面匹配，返回的是 Scene 图片；选择和截图不消耗材质撤销，撤销后材质恢复，记录对应工具结果与实际图片
### Requirement: Explicit history and global asset envelopes

import_asset MUST 接受会话/request/revision 及类型专属路径而无需 scene_id。undo_operation/redo_operation MUST 接受原内容 operation_id，根据历史作用域要求或允许省略 scene_id；旧 undo 请求提供的 scene_id 仍校验。get_history SHALL 返回分页、作用域、顺序及预算；get_editor_status SHALL 保留 undo_operation_id 并提供 redo 栈顶及两栈计数。

#### Scenario: A legacy client undoes its latest scene edit
- **WHEN** 旧客户端以原有 session_id/request_id/expected_revision/scene_id/operation_id 调用 undo_operation
- **THEN** 栈顶和上下文有效时成功，查询/选择/截图原契约保留

#### Scenario: Inspect and traverse an asset-only history
- **WHEN** 无当前场景时导入资源，再通过历史返回的原内容 ID 撤销及重做
- **THEN** 所有请求可通过 Schema，资源操作不隐式创建场景，回执和 revision 可查询

### Requirement: Physics deletion import and history acceptance

交付物 SHALL 提供可重复的真实 EDITOR 验收，覆盖五类导入以及模型导入、创建、物理配置、删除、连续撤销/重做，并验证人工内容修改清空两栈。验收 MUST 区分自动化测试、真实物理/渲染/音频资源证据及人工界面证据，保留现有材质/选择/截图验收。

#### Scenario: Complete a reversible physical scene workflow
- **WHEN** 在独立 QA 实例导入本地模型，创建 mesh、添加/修改物理、移动、删除，再逐项撤销到导入前并重做
- **THEN** 每步状态、历史和实际资源一致，恢复后的 ID 可用于后续请求，没有重复刚体，源文件未修改

#### Scenario: Verify Asset category parity and manual conflict
- **WHEN** 分别导入二维纹理、音乐、音效和 Shader，并在产生 redo 后人工修改内容
- **THEN** Asset 窗口和资源查询显示各类型，导入可撤销/重做；人工修改清空两栈，旧历史无法覆盖人工结果

### Requirement: Backward compatible playback state reporting

get_editor_status 与 get_scene_state 的 data SHALL 增加 playback_state 字符串，取值为 editing、playing、paused；既有 playing 布尔字段 MUST 保留，运行会话存在时（包括 Paused）为 true。工具说明和结果契约 MUST 明确场景查询仍返回编辑快照、运行会话禁止写入和截图。工具总数 SHALL 保持 21，不增加远程播放控制工具。

#### Scenario: Distinguish pause without changing old requests
- **WHEN** 旧客户端以原参数查询一个暂停的编辑器，新客户端读取新增状态字段
- **THEN** 旧请求仍合法且 playing 为 true，新客户端能从 playback_state=paused 区分暂停与播放

#### Scenario: Discover the unchanged tool set
- **WHEN** 客户端获取工具列表和结果 Schema
- **THEN** 仍发现 21 个工具，新增状态字段符合 Schema，说明包含 Paused 的写入与截图保护

### Requirement: Playback acceptance without computer use

本变更验收 SHALL 禁止 computer-use、CUA、浏览器/桌面键鼠及窗口 UI 自动化。验证 SHALL 使用构建、原生无窗口逻辑/真实物理/Lua 测试和 MCP 协议回归；图标外观、双视口真实画面、焦点体验和实际声音等难以自动验证的项目 MUST 标记待人工验证并提供步骤与预期。自动测试、真实后端证据和人工证据 MUST 分开记录，不能把桩或未执行项标记通过。

#### Scenario: Complete automated checks without UI automation
- **WHEN** apply 阶段执行本变更验证
- **THEN** 只用命令行、进程内测试或现有协议接口验证可自动化行为，不模拟点击按钮或拖拽，不为测试增加 MCP 播放工具

#### Scenario: Defer visual or auditory acceptance honestly
- **WHEN** 没有人工观察证据可证明图标、双窗口、焦点交互或音频体验正确
- **THEN** 验证记录列为待人工验证并说明步骤和预期，不能以构建或无窗口测试通过替代
