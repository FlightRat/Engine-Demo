# Design

## Context

动机见 proposal.md。当前实现带来以下约束：

- Windows/MSVC v143、C++20、SDL2、OpenGL 4.5、ImGui；`Engine.Build.props` 管理 x64 构建与调试工作目录，`main.cpp` 另有资源目录自动定位及启动错误诊断，需要保留。
- `Application::Run()` 按事件、Update、Render 顺序执行；不存在可供外部线程使用的场景命令队列。
- `SceneManager` 以名称管理场景，`SceneObject` 分别持有编辑 Registry 和运行 Registry；实体 ID 来源于 EnTT，不是跨加载持久身份。
- `SceneHierarchyDisplay` 直接创建/删除实体，`ComponentDrawer` 直接修改组件。旋转需同步欧拉角和四元数，模型切换需更新刷新标记。灯光位置在 `LightComponent::pos` 中，与 `TransformComponent::position` 独立。
- `AssetManager` 可枚举、查询已加载模型与纹理；可复用模型网格材质初始化。创建可见对象不能只添加 Transform。
- `SceneLoader` 排除脚本实体，身份序列化只写名称和分组；已有 JSON 存取不能承担完整事务快照。
- Logger 会输出 stdout；方案将 MCP stdout 留在独立进程，不改变引擎日志用途。仓库尚无自动化测试目标。

## Goals / Non-Goals

**Goals:**

- 外部助手可以发现工具、读取准确快照、执行受支持操作，并按操作结果确认和撤销。
- 通信、模型响应等待不阻塞渲染；目标身份、执行边界、重试与失败语义明确。
- 共享属性更新规则，避免 UI 与 MCP 对相同属性产生不同结果。

**Non-Goals:**

- 不把 MCP 或模型 SDK 引入 CORE/RENDERING，不设计产品内聊天或模型服务。
- 不扩展场景持久化、物理对象编辑、父子关系编辑、世界空间变换或通用历史系统；完整范围见 proposal.md。

## Decisions

### 1. 独立 Python stdio Server，Windows 命名管道连接编辑器

采用官方 Python MCP SDK，包放在 `tools/engine_mcp/`，使用 `pyproject.toml` 与锁定依赖，建议 Python 3.11+。实施时锁定与目标 Codex 验证兼容的 SDK 版本，不手写 MCP 初始化和版本协商。无需模型 SDK、API Key、公网端点或 SDK 自动升级。

```text
Codex Host + Model
        |
        | MCP / stdio
        v
Python Engine MCP Server
        |
        | Named Pipe / UTF-8 JSON
        v
C++ EditorBridge -> Command Queue -> EditorOperations
                                         |
                                         v
                              SceneManager / ECS / Assets
```

编辑器以 `--enable-mcp-bridge --mcp-instance <name>` 显式启用；默认关闭。管道名由当前 Windows 用户标识与实例名构成；同名实例冲突必须报错，禁止自动切换目标。Server 使用 `--instance <name>` 连接同一实例，不自动启动编辑器。缺少实例、编辑器未打开或桥接关闭时，MCP 本身仍可启动并报告 `editor_unavailable`，允许编辑器稍后上线。

管道仅允许当前用户访问并拒绝远程客户端。桥接初始化失败只停用自动化并记录日志，保持手动编辑器可用。第一版每实例只允许一个桥接客户端，额外连接返回 `editor_busy`。

替代方案：原生 C++ MCP 会增加协议维护和 stdout 隔离负担；本地 HTTP 会增加监听、认证和网络配置；文件轮询不利于请求生命周期和及时反馈。命名管道匹配当前 Windows 单机范围，代价是后续跨平台需替换 IPC 适配器。

### 2. 编辑器操作与主线程调度边界

新增 `EDITOR/src/editor/automation/`，容纳操作、DTO、身份映射、revision、事务及历史；`editor/bridge/` 只处理 IPC、队列与结果传递。C++ 使用已有 RapidJSON。新增源文件进入 EDITOR 的 vcxproj/filters。

`Application::Update()` 在各 display 更新之前处理有限数量命令；队列每帧设命令数/时间预算，单批次上限 32 个动作且只操作已加载资源。通信线程不触碰 Registry、SceneManager、Logger 的可变 vector、ImGui 或 OpenGL；所有场景查询也在主线程生成独立 DTO。队列满时拒绝新请求，不无界分配。

初始化完成后启动桥接；清理时先停止接收、取消未开始命令、唤醒并结束 IPC worker，再清理场景与 GPU 资源。Python 使用异步或 worker 管理管道等待，不因一个等待阻断 MCP 调度。

受支持的 UI 物体创建、Transform/灯光编辑和 Gizmo 提交复用相同更新辅助函数。UI 删除、场景加载/切换/重命名、播放切换以及其他组件修改至少接入 revision/历史失效通知；不为它们新增远程工具。替代方案是 MCP 直接写组件，但会遗漏派生状态和编辑冲突。

### 3. 会话身份、快照和并发条件

每次编辑器启动创建 `session_id`；每个场景和实体分配不透明外部 ID，映射到该次会话中的实际对象/带代数的 EnTT 句柄。删除、清空或重载后不复用旧 ID；场景重命名保持该场景 ID，删除重建生成新 ID。ID 不写入场景文件。

维护单调递增的 `editor_revision`：所有场景内容修改、当前场景切换、资源目录变化和播放切换都递增。查询返回 revision；所有写请求携带 `session_id`、`request_id`、`expected_revision`，除 create_scene 外还必须携带当前 `scene_id`。执行前再次校验，当前场景切换或 revision 不符返回冲突而不写入。批次只核对一次 revision，成功后整体递增一次。

`get_scene_state` 返回场景清单、目标场景快照、播放状态、实体组件摘要及下一页游标；实体列表稳定排序。游标绑定 session/scene/revision，revision 变化要求重读第一页，避免拼接不同时刻快照。默认每页 50、最大 200。`list_assets` 只列已加载模型/纹理的键和类型，不暴露任意磁盘读取；分页同样绑定 revision。

替代方案是直接接受名字或整数实体 ID，但重名、句柄回收和多场景会导致误操作。会话 ID 不承诺跨编辑器重启有效，避免本次迁移场景格式。

### 4. 工具契约与组件语义

| 工具 | 输入与行为 |
| --- | --- |
| `get_editor_status` | 返回连接、实例、session、revision、当前场景、播放状态及桥接协议版本 |
| `get_scene_state` | scene_id 可省略；省略时读取当前场景，无当前场景仍返回清单与空实体列表；支持名称/组件过滤与分页 |
| `list_assets` | 按 model/texture 类型查询已加载资源，支持分页 |
| `create_scene` | 非空唯一名称；创建并激活场景，记录之前的激活场景 |
| `create_object` | 当前 scene_id、名称、kind=mesh/light；mesh 必须指定已加载模型键，light 创建默认启用的点光源 |
| `set_transform` | 明确实体；position、rotation_degrees、scale 为可选完整 vec3，至少提供一个；只接受 space=local |
| `set_light` | 明确已有灯光实体；修改类型、颜色、强度、位置及类型专属参数，至少提供一个字段 |
| `apply_operations` | 当前场景内 1–32 个 create_object/set_transform/set_light；允许用唯一 client_ref 引用批内先前创建的对象 |
| `undo_operation` | 指定最新可撤销 operation_id；不支持跳过中间操作或 redo |
| `get_operation_status` | 按 session_id/request_id 查询写请求状态与已完成结果 |

写工具必须要求 request_id 和 expected_revision；工具参数采用明确 JSON Schema，拒绝未知字段。输入、输出、错误示例由 Python 工具 Schema 与 C++ bridge contract 共享契约测试验证。

Transform 使用引擎单位和本地 XYZ 坐标，根实体的 local 即 world；角度为度，派生四元数按现有 GLM 规则更新。只允许有限位置/角度和正的有限缩放。带 PhysicsComponent 的对象不接受 MCP 变换写入，返回 unsupported_target，避免物理状态不同步。父子层级保持原样，不提供 world 转换。

灯光类型沿用 point_light/direction_light/area_light；所有数值必须有限。颜色为线性 RGB [0,1]、强度 [0,10000]，位置是 LightComponent 的场景空间 pos，方向必须非零且归一化，面积光旋转以度输入并派生四元数。常数衰减 (0,10]，线性/二次衰减 [0,1]，range [0,10000]，半宽/半高 [0.001,4.999]；拒绝与选定类型不适用的字段。切换类型时保留颜色、强度和启用状态，未指定的类型专属字段初始化为有效默认值，面积光半宽/高默认 1。`set_transform` 不移动灯光，工具描述要求使用 `set_light.position`。light kind 不自动添加无效的 Transform。

模型物体一次性创建 Identification、Transform、MeshFilter、MeshRender，复用已加载模型的网格/材质初始化，成功后才暴露 entity_id。失败清除本次新实体，不自动导入文件或生成模型。物体名称允许重复，所有后续操作使用 entity_id；场景名称冲突返回 scene_name_conflict。

### 5. 本地协议与请求生命周期

MCP 与 IPC 是两个边界。内部 bridge-v1 用带 4 字节 little-endian 长度前缀的 UTF-8 JSON，单消息不超过 1 MiB；正确处理部分读写、非法长度、断管及未知版本。普通命令包含 protocol_version、session_id、request_id、method、params；握手独立获取当前会话和版本，版本不匹配拒绝派发。

结果包含 request_id、status、session_id、editor_revision、operation_id、data 或 error。error 具有 code、message、details、retryable；常见 code 包括 editor_unavailable、editor_busy、invalid_arguments、stale_session、stale_target、revision_conflict、play_mode、asset_not_found、unsupported_target、queue_full、request_id_conflict、history_conflict。

写请求状态为 queued -> running -> succeeded/failed，或 queued -> cancelled。默认排队有效期 5 秒、Server 等待结果 10 秒，可配置。开始前超时/取消必须保证不写入；running 不做非事务中断。等待超时返回 pending 标识和 request_id，客户端通过 get_operation_status 确认，不能把通信超时解释为执行失败。

同一会话中保留全部已接收写请求的 payload 指纹和结果；指纹按 method 和用户语义参数规范化，包含 expected_revision，不包含传输关联 ID、等待超时等元数据。相同 request_id/相同内容返回原状态或结果，去重检查先于 revision 检查；相同 ID/不同内容拒绝。账本默认上限 4096，满后拒绝新写入，保留已知请求状态而不驱逐去重记录。Server 重连或重启不清空编辑器账本；编辑器重启生成新会话，旧请求返回 stale_session，不自动重放。参数在工具边界就被拒绝的请求不入账，未知 request_id 查询返回 request_not_found，不能由此推断一个断连前的写操作肯定未执行。

替代方案是超时自动重试或短 TTL 缓存，会在响应丢失时重复创建物体。当前选择以内存账本和显式状态查询换取确定性，不承诺进程崩溃后的持久恢复。

### 6. 受限批次、有限撤销与手动编辑冲突

apply_operations 在同一主线程调度单元执行，预先验证整个批次、资源和 client_ref 依赖，按顺序应用并记录逆操作。只能引用该批次先前创建的对象。成功只生成一个 operation_id；失败按逆序恢复，返回失败索引且不保留部分对象或属性。批次不含 create_scene、undo、物理或任意组件操作，控制事务复杂度。

操作日志保存变换/灯光旧值及写后值，或本次新建对象/场景；不调用 SceneLoader 做全场景快照。支持最新一次 MCP 变更的撤销：创建物体反向删除；属性写入恢复完整相关组件；创建场景只在仍为空、仍处于预期状态时撤销，并恢复原激活场景。SceneManager 不允许删除当前场景，撤销时先切回仍有效的原场景，无原场景则清空激活状态。

只保留一层可撤销记录：新的成功写入替换上一条记录，普通失败不清除之前仍有效的记录。手动内容修改、资源修改、播放切换会使旧 MCP 撤销历史失效；当前场景切换也使其失效。撤销前再次核验目标、revision、写后值和对象依赖；发现后续脚本/物理/父子依赖时拒绝删除并返回 history_conflict。撤销成功递增 revision，清空可撤销槽并标记原操作已撤销，不重新开放更早记录；重复 request_id 返回原撤销结果，不再次撤销。

这不是完整编辑器 Undo/Redo。保守失效会减少可撤销范围，但能防止覆盖人工修改。若发生内部回滚不变量故障，明确返回 recovery_required、禁止后续写入并要求重开编辑器，不宣称原子成功或继续修改。

### 7. Codex 接入、打包与验证

提交 `tools/engine_mcp/README.md`、依赖锁定、MCP 启动入口、bridge-v1 协议说明及 `codex-config.example.toml`，不自动修改用户全局配置。示例使用虚拟环境 Python 的绝对路径和带引号的 Windows 路径，明确 MCP cwd 与 EDITOR 资源工作目录不同。stdout 只输出 MCP 消息，Server 日志写 stderr。

先用 SDK 客户端验证 list_tools 和工具调用，再在新建/重新加载配置的本地 Codex 会话中执行真实调用。Codex 文档只用于接入说明，不向引擎添加模型调用：

- https://learn.chatgpt.com/docs/extend/mcp?surface=cli
- https://modelcontextprotocol.io/docs/sdk

新增轻量 C++ 测试目标验证身份、调度、事务与更新规则；Python 测试验证 MCP Schema、IPC 分帧、错误和重连。GPU 可见性与场景搭建通过实际 Windows 编辑器端到端验收；纯 headless 测试不替代画面验收。

## Risks / Trade-offs

- [UI 直接写组件绕过 revision] -> 审计当前写入口，接入修改/失效通知，并测试人工修改后拒绝撤销；未适配的写操作不暴露为工具。
- [长命令影响帧率] -> 已加载资源、有限页长/队列/批次、每帧调度预算；预算不能抢占已经开始的事务，不承诺固定帧率。
- [位置与灯光字段不同造成无效调整] -> 独立灯光位置契约及画面验收，不强行统一现有数据模型。
- [SDK 与 Codex 协议演进] -> 锁定实测版本、使用 SDK 协商，接入文档记录验证环境和失败信息。
- [Windows 专用 IPC 与额外 Python 环境] -> 封装传输并提供安装说明；跨平台和打包为独立可执行程序留待后续变更。
- [会话内存账本与撤销有限] -> 明确容量、满额拒绝和历史冲突；不承诺跨重启恢复或完整项目持久化。

## Migration Plan

1. 先增加操作/身份/版本契约及测试，保持桥接默认关闭，验证现有 UI 行为。
2. 接入只读管道与 MCP，再开放带校验、事务和有限撤销的写操作。
3. 验证 x64 Debug/Release，提供独立依赖安装与配置示例，在本地 Codex 完成场景搭建和撤销验收。
4. 回退时关闭桥接开关并移除 Codex 的 engine_demo 配置；现有编辑器手动流程与 JSON 文件无需迁移。正常退出仍不自动保存未持久化场景，启动说明明确这一点。
