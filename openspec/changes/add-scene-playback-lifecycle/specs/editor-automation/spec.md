# Spec Delta

## MODIFIED Requirements

### Requirement: Revision bound scene and asset queries

系统 SHALL 提供场景清单、当前场景、Editing / Playing / Paused 播放状态、编辑场景的组件及已应用物理配置快照，以及已加载 model、texture、music、soundfx、shader 清单。get_editor_status 与 get_scene_state 的 data MUST 增加 playback_state（editing/playing/paused），保留 playing，且 Playing / Paused 的 playing 均为 true。编辑内容、资源、当前场景或播放状态变化 SHALL 推进 editor_revision；仅运行副本的逐帧变化 MUST 不推进该 revision。分页游标 MUST 绑定会话和 revision，每页默认 50、最大 200 条。

#### Scenario: Query an editor without an active scene
- **WHEN** 助手查询未激活场景的编辑器
- **THEN** 返回会话、revision、场景清单和空实体列表，不隐式创建或激活场景

#### Scenario: Page cursor becomes stale
- **WHEN** 获取第一页后场景、资源或编辑上下文变化，助手继续使用旧游标
- **THEN** 系统返回需要重新查询的冲突结果，不拼接不同时刻的数据

#### Scenario: Resource discovery uses loaded assets
- **WHEN** 助手查询任一支持类型的资源
- **THEN** 返回已加载资源的键与类型，不将任意磁盘路径视为已加载资源，历史隐藏载荷不出现在列表中

#### Scenario: Query a paused runtime session
- **WHEN** 运行实体已偏离初始位置后用户 Pause，助手查询状态和场景
- **THEN** playback_state 为 paused，playing 为 true，场景实体快照仍描述编辑数据，不混入运行实体或运行身份

#### Scenario: Keep runtime frames outside edit revisions
- **WHEN** Playing 中仅脚本、物理或运行实体选择发生变化
- **THEN** 不因这些运行变化持续推进 editor_revision，状态切换本身仍使旧游标失效

### Requirement: Explicit write preconditions

所有自动化写操作 MUST 携带 session_id、request_id 和 expected_revision。场景实体写入 MUST 明确当前 scene_id；create_scene 和全局 import_asset 不要求 scene_id。undo/redo MUST 按目标记录作用域校验上下文：场景记录要求当前场景，场景生命周期记录校验其记录上下文，全局资源记录不要求激活场景。系统 MUST 校验 revision，并在 Playing 和 Paused 下拒绝所有新的自动化写入，包括选择、场景创建、资源导入和 undo/redo。拒绝 MUST 不改变编辑或运行内容；异步操作在提交前 MUST 重新校验运行会话和 revision。已有成功请求的同内容重试仍 MUST 按既有去重规则返回原回执，不重新执行。

#### Scenario: Scene changes while a command waits
- **WHEN** 写操作等待执行时用户切换当前场景
- **THEN** 操作返回 revision_conflict 或 stale_target，不修改新当前场景或原目标场景

#### Scenario: Write during play
- **WHEN** 编辑器处于 Playing 或 Paused，助手请求创建场景、修改对象、导入、选择、撤销或重做
- **THEN** 系统返回 play_mode，编辑状态与运行状态均不被该请求修改

#### Scenario: Traverse resource history without a scene
- **WHEN** 没有激活场景但有效的历史栈顶是资源导入，助手撤销或重做该项
- **THEN** 依据全局作用域执行，无需创建场景；旧 undo 请求如提供 scene_id 则仍校验其是否为当前场景

#### Scenario: Import preparation crosses a playback transition
- **WHEN** 导入在 Editing 开始准备，提交前用户启动或暂停播放
- **THEN** 提交因 revision_conflict 或 play_mode 被拒绝，不向共享资源表发布部分结果

### Requirement: Create and activate a scene

系统 SHALL 允许以合法 Windows 文件名且大小写不敏感唯一的名称创建并激活一个空编辑场景，并生成或原样复用项目 assets/scripts/<name>.lua，返回 scene_id、operation_id 和最新 revision。创建 MUST 遵守 editor-scene-lifecycle 的文件与绑定规则，不执行脚本或向编辑场景生成脚本实体。名称冲突、参数无效或文件操作失败 MUST 不产生新场景、成功历史或原激活场景变化。

#### Scenario: Create a named scene
- **WHEN** 助手以有效前置条件创建名称未使用的展示场景
- **THEN** 场景清单包含该空场景，当前场景指向它，结果返回新身份和 revision

#### Scenario: Duplicate scene name
- **WHEN** 同名场景已存在且收到新的创建请求
- **THEN** 返回 scene_name_conflict，不覆盖、重命名或切换已有场景

#### Scenario: Create a scene with an existing Lua file
- **WHEN** 名称未被 Scene 占用但同名 Lua 已存在，助手以有效前置条件创建 Scene
- **THEN** 创建并激活空编辑 Scene，已有脚本字节保持不变，后续 Play 使用该文件

#### Scenario: Reject a script creation failure
- **WHEN** 创建同名脚本时遇到目录冲突或不可写路径
- **THEN** 返回明确错误，不新增 Scene、不改变历史和当前场景，不覆盖已有文件

### Requirement: UI and automation preserve the same state rules

系统 SHALL 使受支持的 UI 与自动化编辑遵循相同的属性规则，包括材质、纹理及物理配置/姿态，并仅在 Editing 允许内容和资源写入。实际手动内容/资源修改、外部场景切换/加载/重命名，以及 Play、Pause、Resume、Stop 和错误触发的状态切换 MUST 更新 revision 并清空 MCP undo/redo；MCP 事务自身上下文变化不作为外部变化。单纯编辑选择/清空、运行只读选择和 Scene 相机导航 MUST 不推进内容 revision 或使历史失效。

#### Scenario: Manual edit after an AI edit
- **WHEN** AI 修改物体后，用户用属性面板或 Gizmo 再修改它
- **THEN** 新查询反映人工修改且 revision 推进，两栈清空，旧 AI 操作撤销返回 history_conflict，不覆盖人工结果

#### Scenario: Manual material or texture edit after an AI edit
- **WHEN** AI 修改材质后，用户在属性面板修改颜色、材质数值、纹理开关或绑定
- **THEN** 后续查询与渲染使用的值一致，revision 推进且两栈清空；仅展开或浏览材质面板不改变内容

#### Scenario: Manual change clears a redo branch
- **WHEN** 助手撤销了一项内容后用户实际应用物理配置或导入/重命名音乐、音效、Shader
- **THEN** 两栈清空且 revision 推进，redo 不覆盖人工内容或资源

#### Scenario: Pause is not an editing window
- **WHEN** 用户暂停后尝试 UI 属性修改、场景切换或 MCP 编辑
- **THEN** UI 禁止写入，MCP 返回 play_mode，两个场景的数据均不改变

#### Scenario: State transitions invalidate old history
- **WHEN** 用户依次 Play、Pause、Resume 和 Stop，或脚本错误使 Playing 进入 Paused
- **THEN** 每次已提交状态切换更新 revision 并清空内容历史，暂停不会恢复播放前的撤销栈

### Requirement: Ordered multi level operation history

系统 SHALL 为 MCP 内容操作提供全局有序 undo/redo，包括场景创建、实体/物理/材质编辑、删除及资源导入。仅栈顶可遍历，必须校验 revision、身份、作用域和依赖；成功推进一次 revision，将原内容项移至另一栈。普通失败 MUST 保留内容及两栈，不将 undo/redo 自身加入内容历史。Scene 创建的 undo MUST 保留同名 Lua 文件及外部编辑；redo MUST 按创建规则复用现有文件，缺失则生成模板，不恢复旧模板字节。

#### Scenario: Traverse a complete editing workflow
- **WHEN** 依次导入模型、创建场景/实体、添加物理、修改材质、删除实体，随后连续撤销并重做
- **THEN** 按相反/原顺序恢复各步，组件、依赖及逻辑目标正确，没有重复实体、刚体或资源

#### Scenario: Undo an empty scene creation
- **WHEN** 栈顶是创建空场景且上下文有效，助手撤销
- **THEN** 移除该场景并恢复原激活场景，原来无激活场景时恢复为空，同名 Lua 保留；重做重新绑定现有 Lua（缺失则生成模板）并返回新场景身份

#### Scenario: Reject non-top or invalid history
- **WHEN** 助手跳过栈顶操作、使用失效历史或依赖已不满足
- **THEN** 返回 history_conflict 或具体依赖错误，内容和历史顺序保持不变

#### Scenario: Inspect and select between undo and redo
- **WHEN** 内容修改后仅选择、清空选择、查询或截图，再撤销/重做
- **THEN** 内容历史仍有效，材质参数和纹理绑定完整恢复；选择回执 ID 不可作为内容撤销目标

#### Scenario: Traverse scene history after editing its script
- **WHEN** 空 Scene 创建后，其 Lua 在外部被编辑，再撤销和重做创建
- **THEN** 撤销不删除文件，重做不覆盖用户修改，编辑场景没有隐藏脚本实体阻止空场景撤销

## ADDED Requirements

### Requirement: Separate runtime inspection from edit selection

Playing / Paused 中的 UI 运行实体选择 SHALL 只用于只读检查，MUST 不作为 MCP 编辑实体身份公开。进入运行会话 SHALL 清空编辑选择；get_selection 在运行期间 MUST 返回编辑选择为空，运行 UI 选择 MUST 不改变编辑 selection_serial。Stop 和启动失败清理 MUST 清除运行选择，不保留指向已销毁 Registry 的引用。

#### Scenario: Select a runtime created entity
- **WHEN** 用户在 Playing 或 Paused 中选择脚本生成的实体，助手调用 get_selection
- **THEN** UI 可只读查看该实体，MCP 编辑选择为空，不返回可写入编辑场景的运行 entity_id

#### Scenario: Stop with an active runtime selection
- **WHEN** 用户选择运行实体后点击 Stop
- **THEN** GO Details 和工具上下文不再引用被销毁实体，编辑场景保持可用
