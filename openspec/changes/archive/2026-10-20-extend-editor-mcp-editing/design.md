# Design

## Context

动机见 proposal.md。现有 `EditorOperations.cpp` 同时负责场景查询、目标身份、动作预规划、执行和单个 `History`。历史使用捕获组件/实体的逆操作闭包，状态观察以完整快照与 `ManualEditSerial` 检测外部变化。`CommandQueue` 另行保留请求回执与去重账本。

`PhysicsComponent` 既包含 `PhysicsAttributes`，也持有刚体、形状、Collider 和 user data 的共享指针；属性面板当前会初始化它并通过 Apply 修改运行资源。`SceneDisplay::LoadScnne` 将物理上下文与 Lua 初始化放在一起，不能直接用于 MCP 的物理准备。`Entity::Kill` 直接销毁 ECS 实体；Identification 的 parent_id/objectData.entityID 是内部实体引用。

Asset 窗口直接调用 `AssetManager` 导入模型、纹理、音乐、音效和 Shader。模型加载器会返回关联纹理路径，并在构建 Mesh 时创建 GL 对象，因而不是纯 CPU 操作。资源表是进程级的；当前查询和状态指纹只覆盖模型和纹理。

## Goals / Non-Goals

**Goals:**
- 将编辑请求、批次、undo 和 redo 统一为经过预校验的可逆事务，拒绝部分成功。
- 区分公开生命周期 ID、历史逻辑身份、资源所有权与请求回执；删除恢复不复活旧公开 ID。
- 在现有线程、消息大小和单实例边界内完成编辑与异步导入。

**Non-Goals:**
- 不统一 UI 与 MCP 历史，不持久化历史、导入清单或源文件，不自动保存场景。
- 不开放播放期间写入、冲量/速度控制、任意 Lua、网络下载、通用组件反射写入。
- 不修复整个场景序列化格式，不把已有 Scene JSON 当作恢复快照。
- 首期不恢复 ScriptComponent 中的 Lua 函数/闭包，不扩展 Shader 绑定或音频播放工具。

## Decisions

### 1. 事务与历史拆为独立模块

建议新增 `OperationTransaction`、`OperationHistory`、`EntitySnapshot`、`PhysicsEditing`、`AssetImportCoordinator`，由 EditorOperations 协调查询和请求。模块名可随实现调整，职责不变。

每条内容记录包含原 operation_id、作用域、逻辑目标、前后数据、资源依赖及估算字节数。动作具有 prepare、apply、revert；redo 使用已记录的确定性后状态，不重新解释原始工具请求。新建场景、现有四类动作、物理、删除和导入都接入同一会话历史。undo/redo 请求本身仍有新的请求回执/operation_id，其目标始终是原内容记录 ID，不把遍历动作重新加入内容历史。

undo/redo 先检查栈顶、session、revision、当前作用域和依赖，再整体执行。执行失败可完整回滚时保持两栈及 revision；回滚失败沿用 recovery_required，只允许读取。事务在执行前预留历史空间和返回数据空间，销毁动作尽可能延后至所有可失败步骤完成。

替代方案：保留逆操作闭包并增加正操作闭包。放弃原因是实体销毁、物理资源重建和历史身份更新很难在闭包中统一审计。

### 2. 历史顺序、容量与外部变化

采用会话级全局顺序，undo 栈和 redo 栈合计最多 64 项。记录的组件快照、源字节及历史专属保留资源合计默认预算 256 MiB，按唯一资源计数，GPU 使用保守估算；预算作为内部配置并在状态中披露，测试可注入小值。新写入可从最旧 undo 端淘汰；成功后才清除 redo 分支和应用淘汰，失败不改变已有历史。单条记录或必要依赖无法容纳时返回 history_capacity_exceeded，不执行不可撤销的内容写入。仍被其他记录依赖的资源不得因淘汰释放。

人工内容/资源修改、外部场景切换/重命名/加载及播放切换清空两栈；选择、相机观察和截图不清空。MCP 自己创建/撤销场景导致的激活变化属于该事务，不能被误判为外部改动。执行后更新统一基线；比较逻辑内容和已应用物理配置，不拿旧 revision 或已更换的公开实体 ID 直接作为 redo 内容等价条件。人工物理编辑、音乐/Shader 等资源变化也必须进入观察范围。

历史淘汰绝不删除请求账本；重试旧请求返回旧回执（其实体 ID 可能已失效），不意味着当前内容被重放。

替代方案：按场景分开历史。首期放弃，因为全局导入及场景创建会形成跨作用域依赖，全局顺序可以自然保证先撤销使用者再撤销资源。

### 3. 身份与删除恢复

外部 entity_id/scene_id 沿用生命周期约束；内部增加会话级逻辑标识，历史记录不依赖原 entt 句柄。恢复实体或重做创建时分配新公开 ID，返回旧生命周期到新生命周期的 `entity_id_remap`；恢复场景时相应返回 `scene_id_remap`。内部历史更新逻辑目标到当前实例的绑定，旧外部 ID 始终 stale_target。

`EntitySnapshot` 首期完整覆盖 Identification（排除 selected）、Transform、MeshFilter、MeshRender、Light、Physics 配置、CubeCollider 和 SphereCollider 的值。Script 或其他未注册快照适配器的组件在删除前返回 unsupported_component，details 列出类型，不静默丢失。父级和子树内引用先存成逻辑引用，恢复时两遍创建/重连；重算 Identification.entity_id 和物理 objectData.entityID，清理瞬态接触记录，不复制失效实体引用。

`delete_entity` 的 cascade 默认为 false：有子实体则 entity_has_children；true 删除可验证的整棵子树。层级循环、无法解释的外部引用或依赖拒绝提交。批内删除后继续修改该目标、子树重叠删除或重复删除都预校验失败。批内已创建对象可被后续删除，结果只列存活对象引用和明确的 deleted 标记。

删除采用准备好的快照和延迟销毁；普通事务失败不能改变原实体公开身份、选择和历史。成功删除清空受影响选择；undo 不自动恢复历史选择，避免覆盖期间的 UI 改选。恢复物理对象需重建运行资源；历史中不保留仍挂在世界中的已删除刚体。

替代方案：恢复相同公开 ID。放弃原因是现有契约明确要求删除后旧 ID 永久失效；复用会让延迟请求误命中新对象。

### 4. 物理配置与姿态的一致来源

`add_physics` 要求当前实体已有 Transform 且无 Physics；`set_physics`/`remove_physics` 要求已有 Physics；分别返回 component_exists/component_not_found。首期只编辑无父级的物理对象，父级对象返回 unsupported_target，避免把局部坐标直接当作刚体世界坐标。

查询返回已应用的 body_type、enable_gravity、mass、linear_damping、angular_damping、linear_axis_factor、angular_axis_factor、shape、trigger、bounciness、friction、mass_density 和形状专属尺寸。位置/角度不作为 set_physics 的重复输入；Transform 是编辑姿态来源。所有数值有限，质量/密度/尺寸为正，摩擦非负，弹性/阻尼/轴因子在 [0,1]；形状类型字段必须匹配，切换形状给出有效默认尺寸。

碰撞尺寸明确使用引擎单位，不自动乘 Transform.scale。保留已有视觉缩放；对已带 Physics 的实体，set_transform 首期只接受位置/角度，携带 scale 返回 unsupported_target。要改碰撞大小使用 set_physics；不偷偷改变渲染缩放或碰撞尺寸。

共享 PhysicsEditing 保证无面板、无 Lua、无渲染上下文也可创建所需 PhysicsCommon/World。运行资源采用可失败的候选构建和有序替换，释放顺序为 collider -> shape/body -> world/common，并维护 user data。位置/角度修改同步属性、刚体姿态、previous/current 插值状态；add/remove/set 与其逆操作遵循同一规则。

UI 保留 Apply 交互，但草稿从已应用配置中分离；查询与历史基线只读已应用配置，Apply 校验并调用同一服务，成功后标记人工内容变化。初始化本身不被当作内容编辑。Gizmo 的非播放物理姿态修改复用共享同步入口，属于人工修改而清空历史。

替代方案：仅写 PhysicsAttributes，等属性面板/播放时应用。放弃原因是查询成功与实际刚体状态不一致，也会使撤销依赖面板是否打开。

### 5. 导入沿用 Asset 能力，增加准备与原子登记

`import_asset` 接收 asset_type、asset_key 和绝对本地文件路径；Shader 使用 vertex_path/fragment_path，texture 可传 pixel_art（默认 false）。类型为 model/texture/music/soundfx/shader；仅使用当前加载器可处理的格式，模型继续遵守 ModelRegistry。路径做 UTF-8/存在性/普通文件校验，资源键冲突返回 asset_name_conflict；不覆盖、不复制、不修改源文件。create_scene 仍承担 Asset 的 Add Scene 功能。

把模型处理拆成纯 CPU 数据准备和主线程 GL 上传，修复不能处理的缺失 tangent/依赖情况，防止合法失败变成解引用崩溃。普通二维图片、音效和 Shader 源读取也在后台准备，GL 创建/释放和音频后端要求的步骤放在所属线程。embedded 纹理或其他不能可靠解码的依赖返回明确失败，不能以扩展名受支持承诺所有特性都支持。

模型依赖采用由模型键和规范化依赖标识生成的纹理键；映射写入模型默认材质，并返回依赖清单。只在规范化来源、内容指纹及采样选项一致时复用已有纹理；同名不同内容拒绝。所有必需依赖成功才原子发布模型和新增纹理，资源可见前不改 revision。

导入结果记录本次新增项和复用项，undo 只取消新增项登记。预检查整个编辑器的 Mesh、材质、渲染上下文及音频播放等使用者；仍有用户时返回 asset_in_use。隐藏到 redo 历史的资源不再可由 list_assets/新对象查找，但保持有效载荷与必要依赖。redo 复用保留结果，不重读外部文件；对流式音频还需保留有界源字节/内存流，不把原路径继续有效作为重做前提。释放必须在正确线程完成，播放中的音频不能因历史清理悬空。

替代方案：直接暴露 AddModel/AddTexture。放弃原因是当前 UI 逐项导入可能部分成功，且整个解析/上传会占用编辑线程，无法满足原子撤销和非阻塞要求。

### 6. 导入状态机与作用域

保持桥接 v1 和 1 MiB 消息限制。新增工具只扩展 Schema，二进制资源不进入 IPC。导入在现有请求账本中保持 running，内部阶段为 preparing/ready/committing，不新增必须由老客户端识别的顶层状态。一个实例最多一个导入；额外导入返回 import_busy，场景查询、编辑及截图继续处理。准备阶段读源字节后据此解析，避免同一次导入混用文件变化前后的内容。

保留队列开始前 5 秒 TTL；已开始导入默认总期限 60 秒，提交前检查 session/revision、资源冲突、播放/恢复状态及预算，发生冲突丢弃候选。服务器等待超时返回 pending，不另建请求重试。取消只在提交前成功时保证无副作用；提交后取消等待仍可查询最终结果。关机先停止接收/提交，再取消准备任务、回收候选，最后销毁 GL/音频/物理上下文；启动新会话不自动重放。

场景动作要求 scene_id；导入是全局动作，不要求存在当前场景。undo/redo 按栈顶记录区分 scene、scene-lifecycle、assets 作用域；scene 记录要求当前 scene_id，场景创建记录通过其存活侧和记录的上下文校验，assets 不要求 scene_id。旧 undo 请求所带的当前 scene_id 仍接受并在提供时校验。get_history 返回作用域、分页历史、栈顶 ID、数量、预算及历史变更序号；游标同时绑定 revision 与历史序号，选择不使其失效。get_editor_status 保留 undo_operation_id 并增补 redo/计数摘要。

## Risks / Trade-offs

- [恢复对象改变外部 ID] -> 返回 remap，演示和客户端每次使用返回身份；历史内部以逻辑目标关联，旧回执不改写。
- [物理资源浅复制、销毁顺序或未应用 UI 草稿] -> 值快照、候选重建、独立物理上下文和共享应用服务；原生测试检查世界中的 body/collider 数量。
- [异步导入与人工编辑竞争] -> 仅准备后台运行，提交前重验 revision，冲突候选回收，不把全局状态锁在漫长解析期间。
- [GL 上传/Shader 编译仍占用所属线程] -> 上传分帧并设输入/候选预算，事务最终发布保持短小；不可拆的后端步骤有边界并用真实编辑器测响应性，不承诺任意资源零卡顿。
- [历史保留 GPU/音频内存] -> 合计条数/内存上限、唯一载荷计费、正确线程释放、超预算明确拒绝；不提供不受限的 redo 缓存。
- [快照覆盖不足或不透明脚本引用] -> 未注册组件/引用删除前拒绝；不宣称可恢复任意实体。
- [在已修改工作区实施] -> 保留现有材质/选择/截图代码和未提交改动，基于当前文件演进，不能回滚到旧分支实现。

## Migration Plan

1. 先建立事务、逻辑身份和历史，迁移现有四类动作与场景创建，保留原工具合法请求和回执语义。
2. 加入物理服务、值查询与 UI/Gizmo 共享路径；再加入快照删除与子树恢复。
3. 拆分加载器准备/提交，加入五类导入及全局历史，扩充状态观察和关机流程。
4. 从单一生成器生成两份工具契约，更新 Python/C++ 消费端、注解、文档及 VS 工程；重新构建并重启指定桥接实例、重新加载工具目录。
5. 执行原生 Debug/Release、Python 契约/管道/SDK 测试及真实 EDITOR 验收后交付。回退需成套恢复代码与契约并重启编辑器；本变更无磁盘资源迁移，退出会丢失当前会话内容及历史。
