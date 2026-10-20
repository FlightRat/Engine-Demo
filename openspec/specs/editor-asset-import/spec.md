# editor-asset-import Specification

## Purpose

将 Asset 窗口已有的本地外部资源导入能力提供给 MCP，使助手能在当前会话内按明确资源键加载模型、纹理、音频和着色器，处理依赖与失败，并通过同一内容历史撤销和重做资源登记。

## Requirements

### Requirement: Explicit local session resource import

系统 SHALL 提供 import_asset，支持 model、texture、music、soundfx 和 shader。请求 MUST 提供 asset_key 及绝对本地源路径；Shader MUST 提供 vertex_path/fragment_path，texture SHALL 接受 pixel_art（默认 false）。导入 MUST 遵守现有加载器可处理的格式，检查源文件存在、类型和可读性；不得复制/修改源文件、远程下载或承诺重启恢复。

#### Scenario: Import each Asset window resource category
- **WHEN** 助手分别导入有效模型、二维图片、音乐、音效和顶点/片元 Shader
- **THEN** 每类资源登记到当前实例，list_assets 可查询实际类型/键；模型键可用于创建 mesh，纹理键可用于材质绑定

#### Scenario: Reject an invalid source
- **WHEN** 路径不存在、是目录、不可读取，或内容/特性不能被加载器可靠处理
- **THEN** 返回明确结构化错误，不登记空资源、不改变历史，不写源文件

### Requirement: Deterministic resource keys and conflicts

资源键 MUST 显式指定，同类型已存在的键 MUST 返回 asset_name_conflict，不覆盖或静默改名。模型依赖键 SHALL 与导入模型及依赖身份关联，结果 MUST 列出主资源、实际依赖键、本次新增项及复用项。复用 MUST 验证来源、内容及加载选项一致，不得仅因文件名相同而绑定已有纹理。

#### Scenario: Import models with identically named texture files
- **WHEN** 两个模型引用同名但内容不同的贴图
- **THEN** 各模型得到各自可区分的依赖绑定，或返回明确冲突，不把第二个模型绑定到错误纹理

#### Scenario: Request an existing resource key
- **WHEN** 使用新的 request_id 导入到已占用的同类型资源键
- **THEN** 返回 asset_name_conflict，已有资源、实体和历史保持不变

### Requirement: Atomic import dependency publication

主资源与本次新增的必要依赖 SHALL 整体发布为一个内容操作和一次 revision 变化。任一必要依赖失败 MUST 无可见部分登记；不能解码的 embedded 纹理 MUST 明确失败，不静默丢弃。无依赖模型 SHALL 可正常导入。已有复用资源 MUST 不因失败回滚或后续撤销被移除。

#### Scenario: A required texture fails to load
- **WHEN** 模型解析成功，但必要贴图缺失或损坏
- **THEN** 整体失败，模型及本次其他新增纹理均不可查询，已有资源和历史保留

#### Scenario: Import a complete model package
- **WHEN** 模型及其全部必要依赖准备成功且提交前置条件仍有效
- **THEN** 所有新增资源一起可用，查询返回实际绑定，产生单个可撤销 operation_id

### Requirement: Observationally safe asynchronous import

导入 SHALL 后台准备并在提交前重新校验原 session、revision、资源冲突、播放状态及预算。准备期间场景、历史和资源表 MUST 不被部分修改，普通查询和编辑 SHALL 可继续。每实例最多一个导入，额外导入返回可重试 import_busy。导入 MUST 独立于 apply_operations，且无需激活场景。

#### Scenario: User edits while import is preparing
- **WHEN** 导入尚在准备时用户修改内容或助手完成其他内容编辑
- **THEN** 后续导入提交返回 revision_conflict，候选资源回收，已发生的内容修改保留

#### Scenario: Import with no current scene
- **WHEN** 没有激活场景，助手以有效会话和 revision 导入资源
- **THEN** 导入成功且可查询，不隐式创建或激活场景

### Requirement: Queryable import completion and cancellation

导入 SHALL 使用原请求账本和 get_operation_status；等待超时 MUST 返回 pending 而不是执行失败，重复原请求不得重复导入。准备开始前沿用队列 TTL，开始后总期限默认 60 秒。成功取消只在提交前保证无副作用；关机或会话结束 MUST 不在之后发布资源或自动重放。

#### Scenario: Wait times out while import continues
- **WHEN** Server 等待结束但导入仍在准备或提交
- **THEN** 返回原 request_id 的 pending，后续查询得到唯一最终结果，重复原请求不产生第二份资源

#### Scenario: Import expires or editor closes before publication
- **WHEN** 导入超出开始后的期限或编辑器在提交前关闭
- **THEN** 未发布资源被取消/清理，不在新会话延迟登记，查询得到明确终态或连接关闭错误

### Requirement: Reversible asset registration with retained payloads

导入 SHALL 进入全局内容历史。undo MUST 只移除本次新增登记，检查当前全部使用者，存在依赖时返回 asset_in_use；复用项保持可用。redo MUST 恢复原导入结果，不能因外部文件改动产生不同资源。历史专属载荷 MUST 纳入有界预算；源文件不得因撤销而被删除。

#### Scenario: Undo and redo an import after source removal
- **WHEN** 导入后删除或修改外部源文件，撤销导入再重做
- **THEN** 撤销后新增项不可查询，重做恢复原内容及键，包括音频所需源数据，不重新读取变化后的路径

#### Scenario: Undo a resource that still has a consumer
- **WHEN** 请求撤销的导入资源仍被当前实体、渲染上下文或音频播放使用
- **THEN** 返回 asset_in_use，资源、使用者、revision 和历史顺序保持不变

