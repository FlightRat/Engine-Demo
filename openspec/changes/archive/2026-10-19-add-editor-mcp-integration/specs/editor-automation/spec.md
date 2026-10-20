# Spec Delta

## Purpose

为本地编辑器提供可查询、可验证且具有明确目标的自动化操作，使外部助手能够使用已有资源搭建场景、修改物体与灯光，并在失败、手动编辑或状态变化时获得可靠结果，避免将 AI 指令直接转化为不受约束的组件写入。

## ADDED Requirements

### Requirement: Session scoped target identity

系统 SHALL 为编辑器会话、场景及实体提供不透明标识。目标删除、清空或重载后旧标识 MUST 失效；重命名场景 SHALL 保留其标识，删除后重建同名场景 MUST 使用新标识。标识不承诺跨进程重启持久有效。

#### Scenario: Deleted entity handle cannot address a replacement
- **WHEN** 实体删除后新实体复用了内部存储槽，助手使用旧 entity_id 修改对象
- **THEN** 系统返回 stale_target，且新实体保持不变

#### Scenario: Editor restart invalidates prior session
- **WHEN** 编辑器重启后收到携带旧 session_id 的写请求
- **THEN** 系统返回 stale_session，不解析为新会话中的目标或重放旧操作

### Requirement: Revision bound scene and asset queries

系统 SHALL 提供场景清单、当前场景、播放状态、带组件摘要的场景快照，以及已加载模型和纹理清单。场景内容、资源清单、当前场景或播放状态变化 SHALL 推进 editor_revision；分页游标 MUST 绑定会话和 revision。每页默认 50、最大 200 条。

#### Scenario: Query an editor without an active scene
- **WHEN** 助手查询未激活场景的编辑器
- **THEN** 返回会话、revision、场景清单和空实体列表，不隐式创建或激活场景

#### Scenario: Page cursor becomes stale
- **WHEN** 获取第一页后场景、资源或编辑上下文变化，助手继续使用旧游标
- **THEN** 系统返回需要重新查询的冲突结果，不拼接不同时刻的数据

#### Scenario: Resource discovery uses loaded assets
- **WHEN** 助手查询模型或纹理
- **THEN** 返回已加载资源的键与类型，不将任意磁盘路径视为已加载资源

### Requirement: Explicit write preconditions

所有自动化写操作 MUST 携带 session_id、request_id 和 expected_revision；除创建场景外 MUST 明确 scene_id。系统 SHALL 在执行时校验 revision 和当前场景一致性，并拒绝播放状态下的写入。拒绝的请求 MUST 不改变场景内容。

#### Scenario: Scene changes while a command waits
- **WHEN** 写操作等待执行时用户切换当前场景
- **THEN** 操作返回 revision_conflict 或 stale_target，不修改新当前场景或原目标场景

#### Scenario: Write during play
- **WHEN** 编辑器正在播放，助手请求创建场景、修改对象或撤销
- **THEN** 系统返回 play_mode，编辑状态与运行状态均不被该请求修改

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

系统 SHALL 支持更新指定物体的位置、以度表示的 XYZ 欧拉角及缩放，至少提供一项。space MUST 为 local，位置和角度 MUST 为有限数，缩放 MUST 为正有限数；旋转派生状态 MUST 与输入一致。物理对象写入和 world 空间请求 MUST 被拒绝。

#### Scenario: Rotation remains consistent with rendering
- **WHEN** 助手设置旋转后，用户在属性面板查看并在视口观察对象
- **THEN** 面板角度、查询结果与可见旋转一致，未提供的变换字段保持不变

#### Scenario: Unsupported or invalid transform
- **WHEN** 请求包含非有限值、非正缩放、world 空间或带物理组件的目标
- **THEN** 系统返回明确错误，所有变换字段保持不变

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

系统 SHALL 使受支持的 UI 与自动化编辑遵循相同的属性更新规则。手动内容修改、资源修改、场景切换/加载/重命名及播放切换 MUST 更新 revision 并使先前自动化撤销记录失效。

#### Scenario: Manual edit after an AI edit
- **WHEN** AI 修改物体后，用户用属性面板或 Gizmo 再修改它
- **THEN** 新查询反映人工修改且 revision 推进，旧 AI 操作撤销返回 history_conflict，不覆盖人工结果

### Requirement: Atomic bounded operation batches

系统 SHALL 支持当前场景内 1–32 项物体创建、变换及灯光修改的原子批次。批内引用 MUST 唯一且只引用先前创建的对象。成功 SHALL 只产生一次 revision 更新和一个 operation_id；普通失败 MUST 无部分修改，返回失败动作位置。创建场景和撤销 MUST 不进入批次。

#### Scenario: Create and adjust objects in one batch
- **WHEN** 批次先创建模型与灯光，再通过 client_ref 调整它们
- **THEN** 所有变更同时提交，返回引用到 entity_id 的映射及单个可撤销 operation_id

#### Scenario: A later action fails
- **WHEN** 批内后续动作验证失败或可恢复的执行失败发生
- **THEN** 整个批次无持久部分结果，原实体属性恢复，返回失败索引，不生成成功撤销记录

#### Scenario: Invalid batch structure
- **WHEN** 批次超过 32 项、包含创建场景/撤销、重复引用名或向前引用
- **THEN** 系统在修改前拒绝整个批次

### Requirement: Single level operation undo

系统 SHALL 支持撤销最新一次尚未失效的自动化变更或批次。撤销 MUST 验证 revision、目标及依赖，恢复旧属性或移除本次创建的对象；空场景创建的撤销 SHALL 恢复原激活场景。后续修改及成功撤销后旧记录 MUST 不再可撤销；不提供 redo。

#### Scenario: Undo an object and lighting batch
- **WHEN** 没有后续修改，助手撤销最近一次创建模型和灯光的批次
- **THEN** 两个新对象被移除，之前场景内容保持一致，revision 推进且该记录不再可撤销

#### Scenario: Undo an empty scene creation
- **WHEN** 最新操作创建并激活空场景，且前置状态仍有效
- **THEN** 场景被移除并恢复原激活场景；原来没有激活场景时恢复为未激活

#### Scenario: Reject outdated undo
- **WHEN** 用户进行了后续修改，或待删除对象具有新增依赖，或请求撤销更早的操作
- **THEN** 返回 history_conflict，且当前场景内容不变

### Requirement: Faults cannot be reported as successful rollback

系统 MUST 在内部回滚不变量无法恢复时返回 recovery_required，标明需要重新检查状态并拒绝后续自动化写入，直到编辑器重新启动。系统 MUST 不将不确定或部分恢复结果报告为原子成功。

#### Scenario: Internal recovery failure
- **WHEN** 批次失败后的逆操作无法恢复必要状态
- **THEN** 结果为 recovery_required，后续读查询仍可用于检查，后续写请求被拒绝
