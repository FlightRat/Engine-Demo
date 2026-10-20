# editor-scene-capture Specification

## Purpose

为本地助手提供来自当前编辑场景的真实渲染画面，使材质和灯光修改可以通过图像与版本元数据共同核对。截图具有明确的场景、相机、时间顺序和资源边界，在标签隐藏、编辑上下文变化或渲染不可用时不会把陈旧画面报告为当前结果。

## Requirements

### Requirement: Explicit edit scene capture target

`capture_viewport` SHALL 仅捕获当前非播放编辑场景，固定使用 Scene 相机及最终场景渲染输出，不包含编辑器窗口、菜单、GO Details 或 Gizmo 叠加。请求 MUST 携带 session_id、scene_id、expected_revision。无激活场景、旧会话、非当前场景、旧 revision 或播放状态 MUST 返回明确错误，不改变编辑上下文。

#### Scenario: Capture the requested edit scene
- **WHEN** 助手以当前会话、激活场景和 revision 请求截图
- **THEN** 返回该场景的 Scene 相机渲染结果及匹配元数据，不使用 Game 相机或运行场景

#### Scenario: Reject an invalid capture context
- **WHEN** 没有激活场景或请求前置条件不符，或当前处于播放模式
- **THEN** 返回 capture_unavailable、stale_session、stale_target、revision_conflict 或 play_mode 中对应的错误，不返回旧图片

### Requirement: Fresh images when Scene is hidden

系统 SHALL 为每个成功捕获生成请求接收后渲染的新画面，即使 Game 标签处于前台或 Scene 标签被折叠。截图 MUST 使用捕获时的一致场景状态与相机快照；等待期间场景或内容 revision 改变时 MUST 拒绝请求，不静默捕获新上下文。元数据中的 frame_id SHALL 在同一会话内对成功捕获单调递增。

#### Scenario: Default Game tab is active
- **WHEN** 用户保持默认 Game 标签前台，助手修改材质后请求截图
- **THEN** 系统返回含最新材质的 Scene 渲染画面，保持 Game 标签前台，不使用先前缓存图片

#### Scenario: Scene changes while capture is pending
- **WHEN** 截图尚未读回时用户修改内容、切换或加载场景
- **THEN** 请求返回 revision_conflict 或 stale_target，不把变化后画面标记为原请求的成功结果

### Requirement: Capture is observational

截图 SHALL 不改变 editor_revision、选择、内容撤销、Scene 相机位置/方向/镜头参数、可见视口尺寸、标签布局或场景存储。图片默认不落盘、不自动保存场景。仅为输出图片产生的渲染资源变化 MUST 不计为内容修改。

#### Scenario: Inspect without consuming undo
- **WHEN** 材质变更成功后连续捕获两次，再查询状态并撤销材质操作
- **THEN** 两次捕获有不同 frame_id，内容 revision、选择和相机保持不变，材质操作仍可撤销

### Requirement: Bounded image size and orientation

截图 SHALL 以不带透明通道的 RGB PNG 返回，方向与 Scene 正常显示一致。请求 width/height 默认 640/360，均 MUST 为 [64,1024] 的整数；实际图片 MUST 不超过请求尺寸及 230400 像素，并按相同比例缩小至边界内，实际尺寸明确返回。PNG MUST 不超过 700 KiB，图片传输 MUST 遵守原有 1 MiB 单消息限制；超过限制时明确失败，不截断数据。

#### Scenario: Request a large image
- **WHEN** 请求 1024×1024 的合法截图
- **THEN** 返回保持正方形比例且不超过 230400 像素的实际图片，元数据报告缩小后的尺寸，PNG 和消息均未超过上限

#### Scenario: Verify image orientation and content
- **WHEN** 测试场景上下部分具有可区分的已知颜色并请求截图
- **THEN** 解码图片上下方向正确，尺寸、颜色通道及 Scene 渲染内容一致，不返回全黑占位图冒充结果

### Requirement: Bounded capture work and shutdown

每个实例 SHALL 最多保留一个未完成捕获；额外请求返回可重试的 capture_busy。等待渲染、编码或客户端读取 MUST 不阻塞编辑器主循环，像素读取工作 MUST 有上述尺寸边界。捕获 SHALL 在服务端受理后最多等待 5 秒进入渲染阶段，超时返回 capture_timeout；断连、过期或关闭后 MUST 释放请求和图像资源，不自动重放。

#### Scenario: Capture unavailable or overloaded
- **WHEN** 没有有效渲染上下文、已有捕获进行中，或捕获未在有效期内开始
- **THEN** 分别返回 capture_unavailable、capture_busy 或 capture_timeout，不返回陈旧图片，正常编辑和其他工具仍可继续

#### Scenario: Close or disconnect during capture
- **WHEN** 捕获排队、读回或编码期间客户端断开或编辑器关闭
- **THEN** 未开始工作取消，已开始工作安全收尾并丢弃不可交付结果，在场景和 OpenGL 上下文销毁前结束资源访问，不挂起退出或自动发起后续捕获
