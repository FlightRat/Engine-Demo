# Spec Delta

## ADDED Requirements

### Requirement: Backward compatible playback state reporting

get_editor_status 与 get_scene_state 的 data SHALL 增加 playback_state 字符串，取值为 editing、playing、paused；既有 playing 布尔字段 MUST 保留，运行会话存在时（包括 Paused）为 true。工具说明和结果契约 MUST 明确场景查询仍返回编辑快照、运行会话禁止写入和截图。工具总数 SHALL 保持 21，不增加远程播放控制工具。

#### Scenario: Distinguish pause without changing old requests
- **WHEN** 旧客户端以原参数查询一个暂停的编辑器，新客户端读取新增状态字段
- **THEN** 旧请求仍合法且 playing 为 true，新客户端能从 playback_state=paused 区分暂停与播放

#### Scenario: Discover the unchanged tool set
- **WHEN** 客户端获取工具列表和结果 Schema
- **THEN** 仍发现 21 个工具，新增状态字段符合 Schema，说明包含 Paused 的写入与截图保护

### Requirement: Playback acceptance without computer use

本变更验收 SHALL 禁止 computer-use、CUA、浏览器/桌面键鼠及窗口 UI 自动化。验证 SHALL 使用构建、原生无窗口逻辑/真实物理/Lua 测试和 MCP 协议回归；图标外观、双视口真实画面、焦点体验和实际声音等难以自动验证的项目 MUST 标记待人工验证并提供步骤与预期。自动测试、真实后端证据和人工证据 MUST 分开记录，不能把桩或未执行项标记通过。

#### Scenario: Complete automated checks without UI automation
- **WHEN** apply 阶段执行本变更验证
- **THEN** 只用命令行、进程内测试或现有协议接口验证可自动化行为，不模拟点击按钮或拖拽，不为测试增加 MCP 播放工具

#### Scenario: Defer visual or auditory acceptance honestly
- **WHEN** 没有人工观察证据可证明图标、双窗口、焦点交互或音频体验正确
- **THEN** 验证记录列为待人工验证并说明步骤和预期，不能以构建或无窗口测试通过替代
