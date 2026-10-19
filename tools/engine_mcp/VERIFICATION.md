# MCP 实现与验收记录

日期：2026-10-19。变更：`add-editor-mcp-integration`。

## 环境

| 项目 | 实测版本 |
| --- | --- |
| Windows | NT 10.0.19045，x64 |
| MSBuild / 编译器 | 17.11.2.32701 / MSVC 19.41.34120，v143 |
| Python | 3.12.7，独立 `.venv` |
| 官方 MCP Python SDK | 2.3.0 |
| jsonschema / pytest | 4.26.0 / 9.0.2 |
| 本地 Codex CLI | 0.159.3 |

依赖完整版本在 `requirements.lock`。Server 不调用模型 API，不读取模型密钥。

## 可复现的自动验证

从仓库根目录运行：

```powershell
python tools/engine_mcp/build.py tests
& './x64/Debug/AutomationTests.exe'
python tools/engine_mcp/build.py tests --configuration Release
& './x64/Release/AutomationTests.exe'
python tools/engine_mcp/build.py editor
python tools/engine_mcp/build.py editor --configuration Release
& tools/engine_mcp/.venv/Scripts/python.exe -m pytest tools/engine_mcp/tests -q
openspec validate add-editor-mcp-integration --strict
```

原生测试使用真实 SceneManager、ECS、资源管理接口和主线程队列。
覆盖身份与重命名、局部旋转/灯光规则、旧 revision/游标、物理目标拒绝、
批次回滚恢复原实体与删除新实体、撤销失效、恢复失败后只读、队列容量、过期与取消。

Python 套件实测 **38 passed**。其中集成测试启动独立的真实 C++ 管道宿主，
覆盖分片传输、多请求响应关联、1 MiB 边界、非法长度/JSON/UTF-8/重复键、
未知版本、同名实例冲突、双实例隔离、单客户端限制、断管重连、稍后上线、
编辑器重启、重复写去重、pending 查询、排队取消和不读取响应时的关闭。
另有 SDK stdio 工具调用、中文/空格路径配置、Draft-4 与公开 2020-12 Schema 等价性测试，
以及 MCP `2025-06-18` 协议的实际 initialize/tools/list 消息兼容检查。

这些测试不创建 OpenGL 窗口，不能证明视口显示或鼠标交互正确。
构建过程的完整日志写入本目录被忽略的 `build-*.log` 文件。

最终复核：完整 EDITOR x64 Debug/Release 均构建成功（0 个错误），
AutomationTests x64 Debug/Release 均构建并运行通过；`pip check` 通过，
OpenSpec strict 校验与 `git diff --check` 通过。

## 对真实 EDITOR 的 SDK 验收

已用真实 `x64/Debug/EDITOR.exe --enable-mcp-bridge --mcp-instance codex_qa`
运行官方 SDK 的 `engine_mcp.demo`，成功执行以下调用链：

`get_editor_status → list_assets → create_scene → apply_operations → get_scene_state → undo_operation → get_scene_state`

本机原始执行记录：`qa-demo-undo.log`（忽略文件，仅保留本地）。关键返回值：

| 观察项 | 实际返回 |
| --- | --- |
| 已加载模型 | capsule、cube、hud_quad、plane、quad、skybox、sphere |
| 新场景 / 批次 | `scene-11` / `operation-12`（只对当次 session 有效） |
| 模型 | cube，position `[0,0,0]`，rotation_degrees `[0,30,0]`，scale `[1,1,1]` |
| 灯光 | 启用的 point_light，position `[-3,4,2]`，color 约 `[1,.65,.35]`，intensity `250`，range `25` |
| 批次后 | 查询 total `2`，revision `4` |
| 撤销后 | 查询 total `0`，revision `5`；`scene-11` 仍存在并保持激活 |

浮点颜色返回 `.649999976` / `.349999994` 是引擎 float 精度。
查询中的 `visible=true` 是渲染组件状态，不代表已经观察到实际画面。
测试创建的临时编辑器进程已停止；没有保存或改写场景文件。

## Codex 与人工画面验收尚未完成

`run_codex_acceptance.py` 通过进程级 `-c mcp_servers.engine_demo.*` 覆盖提供配置，
没有修改用户的全局 Codex 配置。它只在确认所需工具调用成功、查询属性匹配，
且同一新场景的实体数从 2 变为 0、撤销指向原批次后才返回成功。
验收提示限制操作在新建场景，禁止触碰既有物体或改代码。

本机 CLI 尝试未完成验收：一次查询返回 `editor_unavailable`；后续会话虽然能启动
Server，但模型报告未发现 `engine_demo` 工具，没有执行创建/撤销调用。
最新辅助脚本据此返回失败，未将 CLI 退出码 0 视为验收通过。
现有证据不能确定此处的 Codex 工具暴露问题根因；SDK 与旧协议消息测试通过也不能替代它。
本地详情位于忽略的 `qa-codex-events.log`、`qa-codex-stderr.log`、`qa-codex-result.log`。

当前用户专用管道还要求编辑器与 MCP 执行器使用同一 Windows 身份，
包括沙箱身份；宿主用户与沙箱用户不同会得到 `editor_unavailable`，不能自动改连其他实例。

用户已明确暂缓画面验收。窗口授权超时后没有获取截图或执行鼠标验收，
因此仍需在真实编辑器完成模型/灯光显示、Gizmo/属性面板、场景切换、加载、
手动修改后拒绝旧撤销、桥接关闭/失败时的手动流程与正常退出检查。
相关 OpenSpec 任务保留未勾选，变更尚未归档。

继续验收时，先按 README 配置同一用户、同一实例，确认 Codex 实际调用
`get_editor_status`，再运行完整示例与人工修改冲突检查；不能只以 `codex mcp list` 作为成功依据。
