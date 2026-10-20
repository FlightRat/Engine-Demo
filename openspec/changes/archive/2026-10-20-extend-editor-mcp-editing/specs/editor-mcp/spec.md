# Spec Delta

## MODIFIED Requirements

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

## ADDED Requirements

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
