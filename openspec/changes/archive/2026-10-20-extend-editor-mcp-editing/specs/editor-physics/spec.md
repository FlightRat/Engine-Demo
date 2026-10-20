# Spec Delta

## Purpose

让本地助手在非播放编辑场景中查询并配置物理刚体和碰撞形状，使属性、实际物理资源与可见姿态保持一致，并能够在组件移除、实体删除及多级撤销重做时可靠恢复。

## ADDED Requirements

### Requirement: Applied physics configuration queries

系统 SHALL 在 get_scene_state 中返回物理组件的已应用配置，包括刚体类型、重力、质量、阻尼、轴因子、碰撞形状及尺寸、trigger、摩擦、弹性和密度。查询 MUST 不初始化或修改组件；UI 未应用草稿 MUST 不冒充已应用配置。

#### Scenario: Inspect a configured rigid body
- **WHEN** 查询具有物理组件的实体
- **THEN** 返回其已应用配置和适用形状尺寸，物理过滤仍可找到该实体，revision 和历史不变

#### Scenario: Inspect before applying UI drafts
- **WHEN** 用户在物理属性面板编辑但尚未应用草稿，助手查询实体
- **THEN** 返回上次已应用配置，草稿与实际生效配置在界面中可区分

### Requirement: Explicit physics component lifecycle

系统 SHALL 提供 add_physics、set_physics 和 remove_physics，分别添加、部分修改和移除物理组件。添加 MUST 要求已有 Transform、无父级且不存在 Physics；修改和移除 MUST 要求已有 Physics。成功 MUST 返回实际结果并进入内容历史。操作 MUST 无需打开属性面板或执行 Lua。

#### Scenario: Add and update physics without opening details
- **WHEN** 助手为根 mesh 添加动态刚体，随后修改质量和重力
- **THEN** 实际物理组件与查询反映配置，其他组件保留，无需 UI 初始化或脚本执行

#### Scenario: Reject invalid lifecycle targets
- **WHEN** 对已有物理的实体再次添加、对无物理实体修改/移除，或给无 Transform/带父级实体添加物理
- **THEN** 分别返回 component_exists、component_not_found 或 unsupported_target，组件和历史均不变

#### Scenario: Remove and restore physics
- **WHEN** 移除物理组件后撤销，再重做
- **THEN** 依次移除、完整恢复、再次移除物理配置及必要运行资源，实体和其他组件保留

### Requirement: Validated type aware physics updates

物理数值 MUST 有限；质量、密度和形状尺寸 MUST 为正，摩擦 MUST 非负，弹性、阻尼及轴因子 MUST 在 [0,1]。body_type SHALL 为 static/dynamic/kinematic，shape SHALL 为 box/sphere/capsule。未知、空修改或不适用形状字段 MUST 被拒绝，形状切换 MUST 提供有效默认尺寸，未指定的适用公共字段保持不变。

#### Scenario: Change collider shape
- **WHEN** 助手把 box 切换为 sphere 并指定有效 radius
- **THEN** 实际碰撞形状与查询成为 sphere，公共刚体参数保持不变

#### Scenario: Reject an invalid candidate atomically
- **WHEN** 一次修改同时包含有效质量和非法尺寸、非有限值或不匹配的形状字段
- **THEN** 返回 invalid_arguments，不保留有效字段的部分修改，不创建无效刚体或 Collider

### Requirement: Coherent edit pose for physics objects

非播放根物理对象 SHALL 通过 set_transform 修改位置/角度，并同步查询、刚体姿态及插值状态。set_physics MUST 不接受独立位置/角度。碰撞尺寸 SHALL 使用引擎单位且不自动乘视觉缩放；已有视觉缩放保留。带物理对象的 scale 写入及带父级对象的物理姿态写入 MUST 返回 unsupported_target。

#### Scenario: Move and undo a physical mesh
- **WHEN** 助手改变根物理实体位置/角度后撤销
- **THEN** Transform、实际刚体及 previous/current 插值状态同步恢复，查询与可见位置一致

#### Scenario: Reject implicit collider scaling
- **WHEN** set_transform 给物理实体提交 scale，即使还包含有效位置
- **THEN** 整个请求拒绝，视觉缩放、碰撞尺寸及姿态均保持不变

### Requirement: Recoverable physics resource ownership

物理编辑、删除恢复、批次回滚及 undo/redo SHALL 恢复配置与所需物理资源，MUST 不共享已删除对象的活动刚体或保留失效实体 user data。失败 MUST 恢复原配置及资源数量；无法完整恢复时 MUST 返回 recovery_required 并拒绝后续写入。

#### Scenario: Delete and restore a physical entity repeatedly
- **WHEN** 对物理实体重复执行删除、撤销与重做
- **THEN** 场景中的刚体/碰撞体数量与存活实体匹配，恢复后的 user data 指向当前实体，无重复刚体或失效引用

#### Scenario: Apply a manual physics edit
- **WHEN** 用户通过 Apply 提交有效物理草稿或使用 Gizmo 改变物理姿态
- **THEN** 使用与 MCP 相同的校验和同步规则，revision 更新且 MCP 两个历史栈清空；仅展开面板不清空历史
