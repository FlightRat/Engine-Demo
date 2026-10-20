# editor-automation Specification

## Purpose

为本地编辑器提供可查询、可验证且具有明确目标的自动化操作，使外部助手能够使用已有资源搭建场景、修改物体与灯光，并在失败、手动编辑或状态变化时获得可靠结果，避免将 AI 指令直接转化为不受约束的组件写入。

## Requirements

### Requirement: Session scoped target identity

系统 SHALL 为编辑器会话、场景及实体提供不透明标识。目标删除、清空或重载后旧标识 MUST 失效；重命名场景 SHALL 保留其标识，删除后重建同名场景 MUST 使用新标识。标识不承诺跨进程重启持久有效。undo/redo 恢复已删除实体或场景时 MUST 分配新的公开身份并返回对应 remap；连续历史操作 MUST 仍指向恢复后的逻辑目标，不复活旧公开 ID。

#### Scenario: Deleted entity handle cannot address a replacement
- **WHEN** 实体删除后新实体复用了内部存储槽，助手使用旧 entity_id 修改对象
- **THEN** 系统返回 stale_target，且新实体保持不变

#### Scenario: Editor restart invalidates prior session
- **WHEN** 编辑器重启后收到携带旧 session_id 的写请求
- **THEN** 系统返回 stale_session，不解析为新会话中的目标或重放旧操作

#### Scenario: Continue history across entity restoration
- **WHEN** 先修改实体再删除它，连续撤销删除和先前修改
- **THEN** 删除撤销返回 entity_id_remap，先前修改在恢复后的实体上正确撤销，旧公开 ID 仍失效

#### Scenario: Recreate a scene through redo
- **WHEN** 撤销场景创建后重做该创建，再重做后续实体创建
- **THEN** 返回新的 scene_id_remap/实体身份，原历史逻辑目标映射到恢复场景，旧场景 ID 不可写入

### Requirement: Revision bound scene and asset queries

系统 SHALL 提供场景清单、当前场景、播放状态、带组件摘要及已应用物理配置的场景快照，以及已加载 model、texture、music、soundfx、shader 资源清单。场景内容、资源清单、当前场景或播放状态变化 SHALL 推进 editor_revision；分页游标 MUST 绑定会话和 revision。每页默认 50、最大 200 条。

#### Scenario: Query an editor without an active scene
- **WHEN** 助手查询未激活场景的编辑器
- **THEN** 返回会话、revision、场景清单和空实体列表，不隐式创建或激活场景

#### Scenario: Page cursor becomes stale
- **WHEN** 获取第一页后场景、资源或编辑上下文变化，助手继续使用旧游标
- **THEN** 系统返回需要重新查询的冲突结果，不拼接不同时刻的数据

#### Scenario: Resource discovery uses loaded assets
- **WHEN** 助手查询任一支持类型的资源
- **THEN** 返回已加载资源的键与类型，不将任意磁盘路径视为已加载资源，历史隐藏载荷不出现在列表中

### Requirement: Explicit write preconditions

所有自动化写操作 MUST 携带 session_id、request_id 和 expected_revision。场景实体写入 MUST 明确当前 scene_id；create_scene 和全局 import_asset 不要求 scene_id。undo/redo MUST 按目标记录作用域校验上下文：场景记录要求当前场景，场景生命周期记录校验其记录上下文，全局资源记录不要求激活场景。系统 MUST 校验 revision 并拒绝播放状态下写入。拒绝 MUST 不改变内容。

#### Scenario: Scene changes while a command waits
- **WHEN** 写操作等待执行时用户切换当前场景
- **THEN** 操作返回 revision_conflict 或 stale_target，不修改新当前场景或原目标场景

#### Scenario: Write during play
- **WHEN** 编辑器正在播放，助手请求创建场景、修改对象、导入、撤销或重做
- **THEN** 系统返回 play_mode，编辑状态与运行状态均不被该请求修改

#### Scenario: Traverse resource history without a scene
- **WHEN** 没有激活场景但有效的历史栈顶是资源导入，助手撤销或重做该项
- **THEN** 依据全局作用域执行，无需创建场景；旧 undo 请求如提供 scene_id 则仍校验其是否为当前场景

### Requirement: Create and activate a scene

系统 SHALL 允许以非空且唯一的名称创建并激活一个空编辑场景，返回 scene_id、operation_id 和最新 revision。名称冲突或参数无效时 MUST 不产生新场景，且不改变原激活场景。

#### Scenario: Create a named scene
- **WHEN** 助手以有效前置条件创建名称未使用的展示场景
- **THEN** 场景清单包含该空场景，当前场景指向它，结果返回新身份和 revision

#### Scenario: Duplicate scene name
- **WHEN** 同名场景已存在且收到新的创建请求
- **THEN** 返回 scene_name_conflict，不覆盖、重命名或切换已有场景

### Requirement: Create objects from available assets

系统 SHALL 支持创建 mesh 或 light 物体。mesh MUST 引用已加载模型并具有可渲染所需属性与材质；light SHALL 创建启用的默认点光源。结果 SHALL 返回实际 entity_id 和属性，创建失败 MUST 清除本次部分结果。物体名称允许重复。

#### Scenario: Create a visible model object
- **WHEN** 助手使用已加载模型键创建 mesh 物体
- **THEN** 编辑器中出现具有默认变换、模型和材质的可渲染物体，查询可按返回 ID 找到它

#### Scenario: Model is not loaded
- **WHEN** 创建请求引用不存在的模型键或把文件路径当作模型键
- **THEN** 返回 asset_not_found，不创建空实体、不自动读取或导入该文件

#### Scenario: Create a light object
- **WHEN** 助手创建 kind=light 的物体
- **THEN** 新实体具有启用的 point_light，查询返回其颜色、强度和实际灯光位置

### Requirement: Consistent local transform updates

系统 SHALL 支持更新指定物体的位置、以度表示的 XYZ 欧拉角及缩放，至少提供一项。space MUST 为 local，位置和角度 MUST 为有限数，缩放 MUST 为正有限数；旋转派生状态 MUST 与输入一致。无父级物理对象的位置/角度写入 SHALL 同步实际刚体与插值状态；物理对象 scale 写入、带父级物理对象变换及 world 请求 MUST 被拒绝。

#### Scenario: Rotation remains consistent with rendering
- **WHEN** 助手设置旋转后，用户在属性面板查看并在视口观察对象
- **THEN** 面板角度、查询结果与可见旋转一致，未提供的变换字段保持不变

#### Scenario: Unsupported or invalid transform
- **WHEN** 请求包含非有限值、非正缩放、world 空间、物理对象 scale 或带父级物理对象变换
- **THEN** 系统返回明确错误，所有变换和物理字段保持不变

#### Scenario: Move a physical root object
- **WHEN** 对具有物理组件的根实体提交有效位置和角度
- **THEN** 编辑姿态、实际刚体及插值状态同步更新，操作可以撤销及重做

### Requirement: Type aware light updates

系统 SHALL 支持 point_light、direction_light 和 area_light 的适用属性，所有数值 MUST 有限。颜色 MUST 为线性 RGB [0,1]，强度 MUST 为 [0,10000]；位置为灯光实际场景空间位置。方向 MUST 非零并归一化，面积光半尺寸 MUST 为 [0.001,4.999]，旋转以度输入并更新派生状态。非法或不适用字段 MUST 被拒绝，类型切换 MUST 产生有效的类型专属默认值。

#### Scenario: Place a warm point light
- **WHEN** 助手设置点光源位置、暖色 RGB 和有效强度
- **THEN** 灯光实际照明位置和颜色随之变化，查询返回与渲染使用一致的属性

#### Scenario: Reject a type incompatible field
- **WHEN** 助手对点光源提交面积光半宽或对方向光提交零方向
- **THEN** 系统拒绝整个请求并保持原灯光属性

#### Scenario: Transform does not imply light position
- **WHEN** 助手查询灯光工具的字段说明并使用 set_light.position 调整位置
- **THEN** 该字段明确表示灯光位置，不要求同步设置物体 Transform 才能生效

### Requirement: UI and automation preserve the same state rules

系统 SHALL 使受支持的 UI 与自动化编辑遵循相同的属性更新规则，包括材质参数、纹理绑定及物理配置/姿态。实际手动内容修改、资源修改、外部场景切换/加载/重命名及播放切换 MUST 更新 revision 并清空 MCP undo/redo 历史；MCP 事务自身的上下文变化不作为外部变化。单纯选择/清空 MUST 不推进内容 revision 或使历史失效。

#### Scenario: Manual edit after an AI edit
- **WHEN** AI 修改物体后，用户用属性面板或 Gizmo 再修改它
- **THEN** 新查询反映人工修改且 revision 推进，两栈清空，旧 AI 操作撤销返回 history_conflict，不覆盖人工结果

#### Scenario: Manual material or texture edit after an AI edit
- **WHEN** AI 修改材质后，用户在属性面板修改颜色、材质数值、纹理开关或绑定
- **THEN** 后续查询与渲染使用的值一致，revision 推进且两栈清空；仅展开或浏览材质面板不改变内容

#### Scenario: Manual change clears a redo branch
- **WHEN** 助手撤销了一项内容后用户实际应用物理配置或导入/重命名音乐、音效、Shader
- **THEN** 两栈清空且 revision 推进，redo 不覆盖人工内容或资源

### Requirement: Atomic bounded operation batches

系统 SHALL 支持当前场景内 1–32 项物体创建、变换、灯光、材质、物理添加/修改/移除和实体删除的原子批次。批内引用 MUST 唯一且只引用先前创建对象；已删除目标的后续修改、重复/重叠删除 MUST 被拒绝。成功只产生一次 revision 和一个历史项；普通失败 MUST 无部分修改并返回失败位置。场景创建、导入、undo/redo、选择、截图和查询 MUST 不进入批次。

#### Scenario: Create and adjust objects in one batch
- **WHEN** 批次先创建模型与灯光，再通过 client_ref 调整它们
- **THEN** 所有变更同时提交，返回引用到 entity_id 的映射及单个可撤销 operation_id

#### Scenario: A later action fails
- **WHEN** 批内后续动作验证失败或可恢复的执行失败发生
- **THEN** 整个批次无持久部分结果，原实体属性、身份、选择与历史恢复，返回失败索引，不生成成功历史项

#### Scenario: Invalid batch structure
- **WHEN** 批次超过 32 项、包含创建场景/导入/undo/redo/选择/截图/查询、重复引用名或向前引用
- **THEN** 系统在修改前拒绝整个批次

#### Scenario: Create and style a mesh in one batch
- **WHEN** 批次创建 mesh 后通过 target_ref 修改其现有材质槽
- **THEN** 材质与创建一起提交为一个可撤销操作；后续动作失败时，新物体移除且对原实体的材质修改完整恢复

#### Scenario: Create a physical entity and delete another
- **WHEN** 批次创建 mesh、通过 target_ref 添加物理并删除另一个可恢复实体
- **THEN** 成功为一个历史项，撤销完整恢复原实体并移除新物体及其刚体

### Requirement: Faults cannot be reported as successful rollback

系统 MUST 在内部回滚不变量无法恢复时返回 recovery_required，标明需要重新检查状态并拒绝后续自动化写入，直到编辑器重新启动。系统 MUST 不将不确定或部分恢复结果报告为原子成功。

#### Scenario: Internal recovery failure
- **WHEN** 批次失败后的逆操作无法恢复必要状态
- **THEN** 结果为 recovery_required，后续读查询仍可用于检查，后续写请求被拒绝

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
### Requirement: Recoverable entity deletion

系统 SHALL 提供 delete_entity，默认 cascade=false；存在子实体时 MUST 返回 entity_has_children，显式 true 才删除子树。删除 MUST 保存全部受支持组件与层级、资源引用；无法完整恢复的组件或引用 MUST 在删除前拒绝并说明原因。成功 MUST 返回删除目标、推进 revision、进入历史并清空受影响选择；undo/redo 不得覆盖其他 UI 选择。

#### Scenario: Delete and restore a selected subtree
- **WHEN** 显式级联删除选中的父实体及子树，随后撤销
- **THEN** 子树消失且选择清空；撤销恢复全部支持组件、父子关系和资源绑定，返回新身份映射，不自动恢复旧选择

#### Scenario: Reject a nonrecoverable entity
- **WHEN** 删除对象含 ScriptComponent 或其他尚无完整恢复能力的组件，或层级有循环/不支持的外部引用
- **THEN** 返回 unsupported_component 或明确依赖错误及详情，不删除任何实体，已有历史不变

### Requirement: Ordered multi level operation history

系统 SHALL 为 MCP 内容操作提供全局有序 undo/redo，包括场景创建、实体/物理/材质编辑、删除及资源导入。仅栈顶可遍历，必须校验 revision、身份、作用域和依赖；成功推进一次 revision，将原内容项移至另一栈。普通失败 MUST 保留内容及两栈，不将 undo/redo 自身加入内容历史。

#### Scenario: Traverse a complete editing workflow
- **WHEN** 依次导入模型、创建场景/实体、添加物理、修改材质、删除实体，随后连续撤销并重做
- **THEN** 按相反/原顺序恢复各步，组件、依赖及逻辑目标正确，没有重复实体、刚体或资源

#### Scenario: Undo an empty scene creation
- **WHEN** 栈顶是创建空场景且上下文有效，助手撤销
- **THEN** 移除该场景并恢复原激活场景，原来无激活场景时恢复为空；重做可再次创建并返回新场景身份

#### Scenario: Reject non-top or invalid history
- **WHEN** 助手跳过栈顶操作、使用失效历史或依赖已不满足
- **THEN** 返回 history_conflict 或具体依赖错误，内容和历史顺序保持不变

#### Scenario: Inspect and select between undo and redo
- **WHEN** 内容修改后仅选择、清空选择、查询或截图，再撤销/重做
- **THEN** 内容历史仍有效，材质参数和纹理绑定完整恢复；选择回执 ID 不可作为内容撤销目标

### Requirement: Observable bounded history and branches

系统 SHALL 默认限制 undo/redo 合计 64 项、唯一历史快照/专属保留载荷估算 256 MiB，披露实际限制与使用量。新成功内容编辑 MUST 清空 redo；失败不清空。容量压力可淘汰最旧 undo 项，但 MUST 保留存留历史依赖，单项无法容纳时返回 history_capacity_exceeded。历史淘汰 MUST 不删除请求去重账本。

#### Scenario: Start a new branch
- **WHEN** 撤销后执行新的成功内容编辑
- **THEN** redo 分支清空，新操作成为 undo 栈顶，旧分支不能再次重做

#### Scenario: Reject an oversized operation
- **WHEN** 一个删除快照或导入载荷无法在历史预算内保留
- **THEN** 返回 history_capacity_exceeded，不执行不可撤销修改，不丢失原历史

#### Scenario: Query history and retry an evicted request
- **WHEN** get_history 查询分页历史，再重试已被历史淘汰但仍在账本中的原请求
- **THEN** 查询返回作用域、栈顶原操作 ID、数量和预算；重试返回原回执而非重新执行；游标绑定 revision 及历史序号

