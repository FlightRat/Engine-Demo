# Spec Delta

## ADDED Requirements

### Requirement: Explicit material slots in scene queries

系统 SHALL 在 `get_scene_state` 的 mesh 材质列表中返回从零开始的 `material_index` 及实际颜色、alpha、metallic、roughness、ao、use_texture、纹理绑定和只读 shading_model。索引 MUST 对应当前实体的现有材质槽，不承诺跨模型或材质结构变化有效；查询 MUST 不创建或修改材质。

#### Scenario: Inspect a model with multiple materials
- **WHEN** 助手查询具有多个材质槽的 mesh
- **THEN** 每个材质返回唯一的现有槽索引和实际参数，未绑定纹理保留空字符串表示，查询不推进内容 revision

### Requirement: Bounded partial material updates

系统 SHALL 提供 `set_material`，按明确的 entity_id 和 material_index 部分更新现有材质。color 为三个线性 RGB 分量，alpha、metallic、roughness、ao 及颜色分量 MUST 为 [0,1] 有限数；use_texture 为布尔值。请求 MUST 至少包含一个有效修改字段，未提供字段保持不变。不存在的材质槽、无 MeshRender 的目标、未知字段及 shading_model 写入 MUST 被拒绝，不自动扩展材质列表。

#### Scenario: Make one material red and metallic
- **WHEN** 有效请求更新指定槽的 color、metallic 和 roughness
- **THEN** 仅该槽的对应字段改变，结果和后续场景查询返回实际参数，其他槽及其他字段不变，产生一次内容 revision 和可撤销操作

#### Scenario: Reject an invalid material update
- **WHEN** 请求使用越界材质索引、无效目标、非有限或越界数值、空修改或不支持的字段
- **THEN** 返回明确的参数或目标错误，材质数量、所有参数和已有撤销记录保持不变

### Requirement: Material texture bindings use loaded assets

材质纹理更新 SHALL 仅接受 albedo、normal、metallic、roughness、ao 槽；槽值 MUST 为已加载的二维纹理键或表示清空的 null。纹理字典 MUST 非空，未知槽及不兼容纹理 MUST 被拒绝。绑定或清空纹理 MUST 不隐式修改 use_texture，也不得导入文件；返回值 SHALL 使用实际存储的纹理键，空绑定表示为空字符串。

#### Scenario: Bind and clear loaded textures
- **WHEN** 请求把 albedo 绑定至已加载的二维纹理键，并以 null 清空 normal
- **THEN** 对应绑定一起更新，其他绑定及 use_texture 保持不变；查询返回 albedo 的实际键和 normal 的空字符串

#### Scenario: Reject unavailable or incompatible texture
- **WHEN** 请求包含未知槽、缺失纹理键、磁盘路径或立方体等非二维纹理资源
- **THEN** 整个材质请求被拒绝，不保留其他字段的部分更新

### Requirement: Single selection query and update

系统 SHALL 提供当前编辑场景的单选查询和选择/清空操作。`get_selection` MUST 返回 session、内容 revision、当前 scene_id、单调 selection_serial，以及选中实体的 entity_id/name 或 null。`set_selection` MUST 携带现有写入前置条件和当前 scene_id，以 entity_id 指定选择、以 null 清空；选中结果 MUST 同步层级高亮、GO Details、Gizmo 与编辑器工具上下文。

#### Scenario: Read a manual selection and select another object
- **WHEN** 用户在层级面板选中 A，助手查询后以返回的当前前置条件选中 B
- **THEN** 查询先返回 A 的真实身份；写结果返回 B，层级高亮、属性面板和 Gizmo 均指向 B，实际选择改变时 selection_serial 推进

#### Scenario: Clear the selection
- **WHEN** 助手向当前场景提交 entity_id=null
- **THEN** 结果中的选中实体为空，层级高亮、属性面板和 Gizmo 不再引用先前对象；已经为空时保持 selection_serial 不变

#### Scenario: Reject invalid selection without disturbing the current one
- **WHEN** 选择请求引用其他场景、已删除实体、旧会话、旧 revision，或编辑器处于播放状态
- **THEN** 返回对应冲突、目标或 play_mode 错误，原选择和场景内容不变

#### Scenario: Target disappears or active scene changes
- **WHEN** 已选实体被删除、撤销移除，或用户切换/加载场景
- **THEN** 系统清除失效选择并推进 selection_serial，不向新对象或新场景解释旧引用；没有激活场景时查询返回空选择

### Requirement: Selection receipts preserve content history

单纯选择或清空 SHALL 不推进 editor_revision、不改变内容分页游标、不产生或替换内容撤销记录。`set_selection` MUST 仍使用现有请求去重、结果状态和写入保护；成功回执包含独立 operation_id，但该 ID 不可撤销。重复原请求 MUST 返回原回执且不得再次覆盖之后的人工选择；恢复故障期间选择写入与其他写入一样被拒绝。

#### Scenario: Select an object after editing its material
- **WHEN** 材质编辑成功后发生人工或 MCP 选择变化，并查询选择或截图
- **THEN** 材质操作仍是最新可撤销内容操作，editor_revision 不因这些选择或只读请求改变，撤销可恢复原材质

#### Scenario: Retry a selection request after a manual selection
- **WHEN** MCP 先选中 A，用户随后选中 B，客户端重发相同 request_id 和原参数
- **THEN** 返回原选择 A 的操作回执，实际选择仍是 B；修改参数重用该 ID 返回 request_id_conflict

## MODIFIED Requirements

### Requirement: UI and automation preserve the same state rules

系统 SHALL 使受支持的 UI 与自动化编辑遵循相同的属性更新规则，包括材质参数与纹理绑定。手动内容修改、资源修改、场景切换/加载/重命名及播放切换 MUST 更新 revision 并使先前自动化撤销记录失效。单纯选择/清空属于界面状态变化，MUST 不推进内容 revision 或使内容撤销失效。

#### Scenario: Manual edit after an AI edit
- **WHEN** AI 修改物体后，用户用属性面板或 Gizmo 再修改它
- **THEN** 新查询反映人工修改且 revision 推进，旧 AI 操作撤销返回 history_conflict，不覆盖人工结果

#### Scenario: Manual material or texture edit after an AI edit
- **WHEN** AI 修改材质后，用户在属性面板修改颜色、材质数值、纹理开关或绑定
- **THEN** 后续查询与渲染使用的值一致，内容 revision 推进，原 AI 撤销失效且不覆盖人工结果；仅展开或浏览材质面板不改变内容

### Requirement: Atomic bounded operation batches

系统 SHALL 支持当前场景内 1–32 项物体创建、变换、灯光及材质修改的原子批次。批内引用 MUST 唯一且只引用先前创建的对象。成功 SHALL 只产生一次 revision 更新和一个 operation_id；普通失败 MUST 无部分修改，返回失败动作位置。创建场景、撤销、选择和截图 MUST 不进入批次。

#### Scenario: Create and adjust objects in one batch
- **WHEN** 批次先创建模型与灯光，再通过 client_ref 调整它们
- **THEN** 所有变更同时提交，返回引用到 entity_id 的映射及单个可撤销 operation_id

#### Scenario: A later action fails
- **WHEN** 批内后续动作验证失败或可恢复的执行失败发生
- **THEN** 整个批次无持久部分结果，原实体属性恢复，返回失败索引，不生成成功撤销记录

#### Scenario: Invalid batch structure
- **WHEN** 批次超过 32 项、包含创建场景/撤销/选择/截图、重复引用名或向前引用
- **THEN** 系统在修改前拒绝整个批次

#### Scenario: Create and style a mesh in one batch
- **WHEN** 批次创建 mesh 后通过 target_ref 修改其现有材质槽
- **THEN** 材质与创建一起提交为一个可撤销操作；后续动作失败时，新物体移除且对原实体的材质修改完整恢复

### Requirement: Single level operation undo

系统 SHALL 支持撤销最新一次尚未失效的自动化内容变更或批次，包括材质参数与纹理绑定。撤销 MUST 验证 revision、目标及依赖，恢复旧属性或移除本次创建的对象；空场景创建的撤销 SHALL 恢复原激活场景。后续内容修改及成功撤销后旧记录 MUST 不再可撤销；不提供 redo。选择、清空选择及只读查询/截图 MUST 不替换或失效内容历史。

#### Scenario: Undo an object and lighting batch
- **WHEN** 没有后续修改，助手撤销最近一次创建模型和灯光的批次
- **THEN** 两个新对象被移除，之前场景内容保持一致，revision 推进且该记录不再可撤销

#### Scenario: Undo an empty scene creation
- **WHEN** 最新操作创建并激活空场景，且前置状态仍有效
- **THEN** 场景被移除并恢复原激活场景；原来没有激活场景时恢复为未激活

#### Scenario: Reject outdated undo
- **WHEN** 用户进行了后续修改，或待删除对象具有新增依赖，或请求撤销更早的操作
- **THEN** 返回 history_conflict，且当前场景内容不变

#### Scenario: Undo material changes after selecting and inspecting
- **WHEN** 材质变更后仅发生选择和截图，助手以当前前置条件撤销原材质操作
- **THEN** 所有受影响材质参数、纹理开关和绑定恢复，其他槽不变；尝试撤销选择操作的 ID 返回 history_conflict 且不影响材质历史
