# Spec Delta

## MODIFIED Requirements

### Requirement: Explicit edit scene capture target

`capture_viewport` SHALL 仅在 Editing 捕获当前编辑场景，固定使用 Scene 相机及最终场景渲染输出，不包含编辑器窗口、菜单、GO Details 或 Gizmo 叠加。请求 MUST 携带 session_id、scene_id、expected_revision。无激活场景、旧会话、非当前场景、旧 revision 或处于 Playing / Paused MUST 返回明确错误，不改变编辑上下文。

#### Scenario: Capture the requested edit scene
- **WHEN** 助手以当前会话、激活场景和 revision 请求截图
- **THEN** 返回该场景的 Scene 相机渲染结果及匹配元数据，不使用 Game 相机或运行场景

#### Scenario: Reject an invalid capture context
- **WHEN** 没有激活场景或请求前置条件不符，或当前处于 Playing / Paused
- **THEN** 返回 capture_unavailable、stale_session、stale_target、revision_conflict 或 play_mode 中对应的错误，不返回旧图片

#### Scenario: Do not capture a paused runtime scene
- **WHEN** 用户 Pause 后助手请求 capture_viewport
- **THEN** 返回 play_mode，不把暂停运行画面当成编辑场景图片，也不临时切换相机或 Registry

#### Scenario: Start play while capture is pending
- **WHEN** 编辑态截图尚未交付时用户进入 Playing
- **THEN** 请求返回 revision_conflict 或 play_mode，不交付旧编辑上下文或运行副本图片
