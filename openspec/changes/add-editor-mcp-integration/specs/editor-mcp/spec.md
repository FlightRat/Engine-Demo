# Spec Delta

## Purpose

使本地 AI 助手通过标准 MCP 工具连接明确选定的编辑器实例，发现和调用场景自动化能力，并在排队、超时、重连与编辑器重启时保持清晰的请求结果，使用户能够用现有 Codex 工作流可靠地操作运行中的编辑器。

## ADDED Requirements

### Requirement: Discoverable local MCP tools

系统 SHALL 提供本地 stdio MCP Server，公开 get_editor_status、get_scene_state、list_assets、create_scene、create_object、set_transform、set_light、apply_operations、undo_operation 和 get_operation_status。每个工具 MUST 提供参数 Schema、字段约定及结构化结果；未知字段 MUST 被拒绝。

#### Scenario: Codex discovers editor tools
- **WHEN** 支持的本地 Codex 会话加载 Server 配置并执行工具发现
- **THEN** 能发现全部首版工具及必填参数，知道坐标、角度、灯光和撤销约定

#### Scenario: Invalid tool arguments
- **WHEN** 工具调用缺少必填字段或包含未知字段
- **THEN** 返回参数错误，不向编辑器执行部分或推测性修改

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

系统 SHALL 在不等待模型或 IPC 的情况下持续处理窗口和渲染。查询快照与写命令 MUST 在编辑器状态一致的执行点完成；等待队列 MUST 有限，容量不足时明确拒绝新请求。

#### Scenario: Slow client does not freeze the editor
- **WHEN** MCP 客户端等待模型或暂停读取结果
- **THEN** 编辑器仍可交互和渲染，通信等待不占据主线程

#### Scenario: Queue is full
- **WHEN** 请求数量超过队列容量
- **THEN** 新请求返回 queue_full，未接收的请求不在稍后偷偷执行

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

系统 SHALL 区分 queued、running、succeeded、failed 和 cancelled，并通过 get_operation_status 返回状态与最终结果。开始前过期或成功取消的请求 MUST 永不写入；等待超时 MUST 不被视为执行失败，运行中的请求不得盲目重试为新写操作。

#### Scenario: Command expires before execution
- **WHEN** 排队命令超过有效期且尚未开始执行
- **THEN** 最终状态为 cancelled，主线程恢复处理后也不执行它

#### Scenario: Result wait times out during execution
- **WHEN** 写请求已 running 但 Server 等待结果超时
- **THEN** 返回 pending 状态、原 request_id 和查询方式；后续状态查询得到真实完成结果

#### Scenario: Cancellation after execution starts
- **WHEN** 客户端在操作开始后取消等待
- **THEN** 系统不宣称操作已取消，保留可查询的最终事务结果

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
