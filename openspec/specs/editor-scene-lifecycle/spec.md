# editor-scene-lifecycle Specification

## Purpose

为内存中的 Scene 建立一致的创建、激活、命名和主脚本绑定规则，使空场景启动即可编辑，场景切换不会提前执行游戏逻辑。脚本文件生成、复用和重命名遵守用户内容保护及明确的失败边界，运行生命周期与磁盘场景加载相互独立。

## Requirements

### Requirement: Default authored scene at editor startup

编辑器正常启动 SHALL 创建并激活名为 default 的编辑 Scene，绑定项目资产根目录下的 assets/scripts/default.lua。文件不存在 MUST 生成主脚本模板；已有文件 MUST 保留原内容。启动 MUST 不再预建 scene1/scene2；新建 default 时在候选编辑 Registry 调用可选 on_build 一次，不调用 on_start/on_update/on_stop，不生成隐藏脚本实体。

#### Scenario: First startup
- **WHEN** scripts 目录可写且没有 default.lua，编辑器启动
- **THEN** default 是当前 Scene，可编辑实体数量为零，生成 default.lua，未执行 on_start

#### Scenario: Startup with an existing script
- **WHEN** default.lua 已含用户逻辑，编辑器再次启动
- **THEN** 当前 Scene 为 default，脚本原字节不变，on_build 生成的初始对象在 Editing 可编辑，游戏行为等待 Play

### Requirement: Safe scene names and same named script resolution

Scene 名称 SHALL 非空且满足 Windows 文件名规则，拒绝保留设备名、路径分隔符、路径穿越、非法字符和末尾空格或句点。场景与文件名冲突 MUST 按大小写不敏感规则检查。主脚本路径 MUST 始终从项目资产根目录推导为 assets/scripts/<scene-name>.lua，不依赖调用工具的工作目录或写死 D 盘路径。

#### Scenario: Resolve a Unicode scene name
- **WHEN** 用户创建合法的中文 Scene 名称
- **THEN** 在项目 scripts 目录生成或绑定对应 UTF-8 路径脚本，切换和 Play 均能读取

#### Scenario: Reject unsafe or conflicting names
- **WHEN** 请求名称为路径、保留名，或与已有 Scene 仅大小写不同
- **THEN** 创建或重命名失败，原 Scene、激活状态和任何已有脚本均不变

### Requirement: Scene creation and script preservation

所有创建入口 SHALL 使用统一规则构建 Scene 并激活，自动生成缺失的同名 Lua；已有同名普通脚本文件 SHALL 原样复用。脚本文件或内存 Scene 创建失败 MUST 不发布新 Scene、不改变当前 Scene、不产生成功历史；新生成临时文件 MUST 清理，已有文件 MUST 不修改。语法、契约或 on_build 失败 MUST 给出路径/回调诊断并回滚候选内容，原 Scene 和历史不变。构建阶段 MUST 不播放声音、不推进游戏时间/物理、不注册共享资源。

#### Scenario: Create a scene with a reusable script
- **WHEN** 对应 Lua 已存在且名称未被内存 Scene 占用，用户创建 Scene
- **THEN** Scene 创建并激活，文件内容保持不变，独立构建环境调用 on_build 一次，构建对象立即可编辑；缺少该可选回调时保持空 Scene

#### Scenario: Fail to create the script file
- **WHEN** scripts 目录不可写或同名路径为目录，创建不能完成
- **THEN** 操作失败，原当前 Scene 和历史保持不变，不残留部分 Scene

### Requirement: Coordinated scene and script rename

Editing 中重命名 Scene SHALL 同步重命名其同名 Lua，保留脚本字节和 Scene 身份。目标 Scene 或 Lua 路径冲突、源文件缺失、权限或文件操作失败 MUST 拒绝整个重命名，保留原名称、绑定和激活状态。重命名 MUST 不自动改写 Lua 文件正文中的字符串或用户代码。

#### Scenario: Rename the active scene
- **WHEN** 用户将 A 重命名为未占用的 B 且文件操作成功
- **THEN** Scene 身份不变，当前名称为 B，A.lua 成为内容相同的 B.lua，后续 Play 读取 B.lua

#### Scenario: Do not overwrite a destination script
- **WHEN** B.lua 已存在，用户请求将 A 重命名为 B
- **THEN** 整个重命名失败，A.lua、B.lua、Scene 名称和绑定不变

### Requirement: Retained scripts after scene removal and history traversal

删除 Scene 或撤销 Scene 创建 SHALL 只移除内存 Scene，保留脚本文件及其用户修改。重做创建 MUST 按原名称重新绑定现有脚本，不用历史模板覆盖文件；文件缺失则按创建规则重新生成模板。既有当前场景删除限制和历史身份 remap MUST 保留。

#### Scenario: Undo and redo after an external script edit
- **WHEN** 创建空 Scene 后用户在外部编辑 Lua，再撤销 Scene 创建并重做
- **THEN** 撤销保留文件，重做使用文件当前内容执行新实例的 on_build，新的 Scene 公开身份按现有规则返回，不复活旧 ID

### Requirement: Edit time binding without game side effects

激活或拖入 Scene SHALL 绑定同名主脚本并进行不执行代码的语法检查；切换 MUST 不调用脚本顶层代码、on_start、on_update 或 on_stop，不加载 GameDemo 公共逻辑、不播放声音、不改变实体或资源清单。语法问题 SHALL 显示可定位诊断，Play 时 MUST 重新读取和检查脚本。

#### Scenario: Switch between scenes
- **WHEN** Editing 中用户拖入另一个 Scene，其脚本顶层包含可观察副作用
- **THEN** 当前 Scene 和绑定切换，语法检查不产生这些副作用，两个 Scene 的编辑实体保持原样

#### Scenario: Fix a script before play
- **WHEN** 绑定时语法检查失败，用户在外部修复文件后点击 Play
- **THEN** 使用修复后的文件重新检查并启动，不需要重建 Scene 或重启编辑器

### Requirement: Scene main script callback contract

Scene 主 Lua SHALL 返回含 on_start、on_update 和 on_stop 函数的表。on_build SHALL 为可选构建回调，新文件 MUST 生成四个空回调模板，旧三回调脚本仍可创建空 Scene。on_build 仅在新建内存 Scene 时调用一次，切换、Play、Resume 和 Stop MUST 不重复调用。Play MUST 在新运行环境执行模块并调用 on_start 一次；Playing 每帧调用 on_update(dt)；Resume MUST 不重复模块执行或 on_start；正常 Stop MUST 在运行环境释放前调用 on_stop 一次。新一轮 Play MUST 重新读取磁盘文件。

#### Scenario: Execute one lifecycle per play session
- **WHEN** 用户 Play、Pause、Resume、Stop，再次 Play
- **THEN** 新建时模块及 on_build 各执行一次；第一轮运行环境的模块和 on_start 各执行一次，暂停期间无 update，Stop 调用 on_stop，新轮重新执行模块和 on_start，Lua 状态不跨轮保留

#### Scenario: Reject legacy main table scripts
- **WHEN** 主脚本未返回所需回调表，仍依赖旧全局 main 更新表
- **THEN** Play 报告契约错误并保持 Editing，不静默回退到公共 main.lua

#### Scenario: Preserve authored values across sessions
- **WHEN** on_build 生成初始对象，用户在 Editing 修改位置、材质和物理参数，然后多次 Play/Pause/Resume/Stop
- **THEN** Play 克隆修改后的对象，on_start 不重新生成初始对象，Stop 后原编辑对象身份及修改均保留

#### Scenario: Failed construction is atomic
- **WHEN** on_build 已创建部分实体后报错或试图播放声音/注册共享资源
- **THEN** 新 Scene 不发布，原当前 Scene、共享资源和历史不变，用户脚本不被覆盖
