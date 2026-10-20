# Spec Delta

## MODIFIED Requirements

### Requirement: Discoverable local MCP tools

系统 SHALL 提供本地 stdio MCP Server，公开 get_editor_status、get_scene_state、list_assets、create_scene、create_object、set_transform、set_light、apply_operations、undo_operation、get_operation_status、set_material、get_selection、set_selection 和 capture_viewport。每个工具 MUST 提供参数 Schema、字段约定及结构化结果；未知字段 MUST 被拒绝。现有十个工具的调用方式 MUST 保持兼容。

#### Scenario: Codex discovers editor tools
- **WHEN** 支持的本地 Codex 会话加载 Server 配置并执行工具发现
- **THEN** 能发现全部十四个工具及必填参数，知道坐标、角度、灯光、材质槽、选择、截图和撤销约定

#### Scenario: Invalid tool arguments
- **WHEN** 工具调用缺少必填字段或包含未知字段
- **THEN** 返回参数错误，不向编辑器执行部分或推测性修改

#### Scenario: Selection and capture annotations
- **WHEN** 客户端读取新增工具的说明与注解
- **THEN** get_selection 和 capture_viewport 标记为只读；set_selection 标记为改变界面状态且不提供内容撤销；set_material 标记为内容修改，说明查询复用 get_scene_state，所有工具均仅作用于指定本地实例

## ADDED Requirements

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
