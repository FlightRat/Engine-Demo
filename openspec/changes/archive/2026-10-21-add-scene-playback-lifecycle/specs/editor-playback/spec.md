# Spec Delta

## Purpose

为编辑器提供有明确生命周期的场景播放，使用户能开始、暂停、继续并停止模拟，同时保留播放前编辑数据。两个视口共享一致运行画面，游戏时间、物理、音频和输入遵守同一状态边界，失败时仍可安全返回编辑。

## ADDED Requirements

### Requirement: Unified playback state transitions

系统 SHALL 提供 Editing、Playing、Paused 三种互斥状态。Editing 的 Play MUST 创建新的运行会话；Playing 的 Pause MUST 保留会话并暂停；Paused 的 Play MUST 恢复同一会话。Playing 和 Paused 的 Stop MUST 返回 Editing。不可用的重复操作 MUST 不重复初始化、销毁或推进模拟。

#### Scenario: Pause and resume the same session
- **WHEN** 用户从 Editing 点击 Play，运行后点击 Pause，再点击 Play
- **THEN** 状态依次为 Playing、Paused、Playing，暂停前的游戏状态和 Lua 变量保留，初始化只执行一次

#### Scenario: Stop and start a fresh session
- **WHEN** 用户从 Playing 或 Paused 点击 Stop 后再次 Play
- **THEN** 旧运行会话销毁，新会话从未被运行过程改变的编辑数据开始

### Requirement: Distinct playback toolbar controls

Game 工具栏 SHALL 提供 Play Game、Pause Game 和 Stop Game 图标按钮。Editing 仅 Play 可用；Playing 的 Play 高亮且禁用，Pause/Stop 可用；Paused 的 Pause 高亮且禁用，Play/Stop 可用，Play 提示为 Resume Game。Pause MUST 使用用户提供的 pause.png，按现有按钮方式随程序内嵌，不依赖外部 G 盘文件。

#### Scenario: Inspect controls in each state
- **WHEN** 用户依次进入 Editing、Playing 和 Paused 并查看按钮及提示
- **THEN** 可用性、高亮和提示符合状态，重复点击当前状态按钮不触发额外操作

### Requirement: Isolated runtime scene and complete stop restoration

播放 SHALL 使用与编辑数据隔离的运行场景。运行中的实体增删、层级、变换、灯光、实体材质、物理状态、游戏相机及 Lua 变量 MUST 不修改编辑数据。Stop MUST 丢弃运行修改及运行资源，不把模拟结果提交回编辑场景。再次 Play MUST 重建运行数据，不复用上轮速度、脚本变量或计时。

#### Scenario: Restore after runtime mutations
- **WHEN** 播放中改变实体属性、创建或删除实体并移动游戏相机，随后 Stop
- **THEN** 编辑实体及其身份、关系和属性与播放前一致，运行创建实体消失，游戏相机回到新会话的初始状态

#### Scenario: Reject an unsupported clone safely
- **WHEN** 编辑场景包含无法可靠隔离的组件或无效实体关系，或运行资源创建失败
- **THEN** Play 失败并报告目标和原因，保持 Editing，编辑场景不变，不留下部分运行会话

### Requirement: Shared scene content with independent viewport cameras

Game 与 Scene SHALL 在 Editing 显示编辑场景，在 Playing 和 Paused 显示同一运行场景。Game MUST 使用游戏相机，Scene MUST 使用独立编辑器相机。两个窗口的渲染 MUST 不各自推进模拟；任一窗口隐藏或折叠 MUST 不改变运行更新次数。

#### Scenario: Observe the same moving object in two viewports
- **WHEN** Playing 中实体发生运动且 Game 和 Scene 均可见
- **THEN** 两个窗口显示相同的运行实体状态，各自保持独立相机视角

#### Scenario: Move the Scene camera during pause
- **WHEN** Paused 中用户移动 Scene 相机
- **THEN** Scene 视角可以变化，实体、游戏相机和游戏时间均不因该操作推进

### Requirement: Frame independent simulation and pausable game time

系统 SHALL 在 Playing 每个编辑器帧至多调用一次主脚本 on_update(dt)，按有效经过时间累计固定 1/60 秒物理步进，并同步结果。Paused 和 Editing MUST 不推进运行物理、游戏时间或脚本更新。恢复 MUST 不补算暂停时长；游戏侧 delta、tick 和 Timer MUST 使用会话游戏时间。Scene 相机和 UI MUST 继续使用编辑器时间。

#### Scenario: Compare normal frame rates
- **WHEN** 相同初始条件分别以 30、60、144 FPS 的输入帧时序模拟相同有效时长
- **THEN** 物理步数和最终结果在数值容差内一致，更新次数不随可见窗口数倍增

#### Scenario: Resume after a long pause
- **WHEN** 游戏计时器运行后暂停较长时间再继续
- **THEN** 暂停期间计时器和物理不推进，恢复首帧不包含暂停时长，不产生补算突跳

#### Scenario: Toggle collision debugging outside play
- **WHEN** Editing 或 Paused 中用户切换碰撞调试显示
- **THEN** 只改变调试显示状态，不额外推进物理世界

### Requirement: Session scoped audio lifecycle

运行会话发起的音乐和音效 SHALL 随 Pause 暂停、Resume 恢复、Stop 停止并释放会话占用。Resume MUST 只恢复被本次会话级 Pause 暂停的声音，不能复活此前已被脚本暂停或停止的声音。声音控制和音量 MUST 不遗留对后续会话的全局修改，清理 MUST 不误停非本会话音频。

#### Scenario: Pause and stop gameplay sound
- **WHEN** 运行脚本播放音乐和音效后用户 Pause、Resume、Stop
- **THEN** 本轮声音依次暂停、恢复和停止；新 Play 不继承上一轮声音或音量修改

#### Scenario: Preserve a script paused sound
- **WHEN** 脚本已暂停某段声音，之后用户 Pause 再 Resume
- **THEN** 该声音继续保持脚本暂停状态，不被会话恢复操作错误启动

### Requirement: Focus scoped gameplay input

游戏 SHALL 仅在 Playing 且 Game 画面获得焦点时接收键鼠输入；工具栏、其他窗口和文本输入 MUST 不透传为游戏操作。失去 Game 焦点 MUST 不停止模拟。Pause、Stop 和焦点切换 MUST 清除暂存边沿事件和鼠标位移，避免恢复后误触发；重新获得焦点不得把旧按键解释成新的按下事件。

#### Scenario: Navigate the Scene camera during play
- **WHEN** Playing 中用户在 Scene 操作编辑器相机或在其他窗口输入文本
- **THEN** 运行模拟继续，游戏脚本不收到这些键鼠操作，Scene 相机控制仍正常

#### Scenario: Resume without stale input
- **WHEN** 按键或鼠标事件发生后游戏暂停或失去焦点，随后恢复并重新聚焦
- **THEN** 不重放旧的 just_pressed、just_released、位移或滚轮事件

### Requirement: Read only inspection during runtime sessions

Playing 和 Paused SHALL 禁止 UI 与自动化修改编辑内容、资源清单或切换、创建、加载、重命名和删除 Scene，也禁止保存运行内容。用户 SHALL 仍可选择运行实体、只读查看属性并移动 Scene 相机；Gizmo 和属性写入 MUST 禁用。运行实体选择 MUST 不成为编辑实体的可写引用，Stop 后失效引用 MUST 清除。

#### Scenario: Attempt editing while paused
- **WHEN** Paused 中用户尝试修改属性、应用物理草稿、操作 Gizmo、导入资源或拖入另一 Scene
- **THEN** 操作被阻止，编辑和运行场景均不被修改，当前运行会话保留

### Requirement: Runtime scripts cannot mutate shared asset registration

运行脚本 SHALL 只使用已加载共享资源，MUST 拒绝新增、覆盖或移除全局资源登记。运行实体的组件参数仍 SHALL 可由脚本修改，且 Stop 后丢弃。资源写入拒绝 MUST 不产生部分模型、纹理、声音或 Shader 登记。

#### Scenario: Reject resource creation from scene initialization
- **WHEN** on_start 调用全局资源新增 API
- **THEN** 调用明确失败，不登记资源；若产生未处理脚本错误则按初始化失败清理，编辑资源清单不变

### Requirement: Recoverable script failures and unconditional cleanup

脚本语法、回调契约或 on_start 失败 MUST 取消启动并保持 Editing。on_update 报错 MUST 自动进入 Paused，停止本帧后续模拟并记录文件路径、回调和可获得的行号。on_stop 报错 MUST 记录错误并继续释放会话，确保返回 Editing；关闭编辑器 MUST 同样安全结束运行会话。

#### Scenario: Fail during start
- **WHEN** 脚本有语法错误、返回值不符合契约或 on_start 执行失败
- **THEN** 保持 Editing，部分实体、Lua、物理及本轮音频被清理，编辑内容不变

#### Scenario: Fail during update then stop
- **WHEN** on_update 抛出错误，用户随后点击 Stop 且 on_stop 也抛出错误
- **THEN** 首个错误使会话 Paused，两个错误均可定位，Stop 仍清理并返回 Editing
