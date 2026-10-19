# Proposal

## Why

编辑器已有场景、ECS、资源和灯光能力，但这些能力主要由 ImGui 回调直接调用，外部 AI 无法查询或可靠地修改运行中的场景。提供本地 MCP 接口后，用户可以在 Codex 中用自然语言完成场景搭建，并通过实际状态反馈和撤销验证结果。

## What Changes

- 增加 C++ 编辑器操作层，封装查询、创建场景、创建物体、设置变换、设置灯光及有限范围的操作撤销；相关 UI 修改复用一致的更新规则。
- 增加 Windows 本地桥接与主线程命令队列，隔离通信线程和 ECS/OpenGL 状态，提供场景及实体有效性校验、请求去重、执行状态查询和异常关闭处理。
- 增加独立 Python MCP Server，通过 stdio 接入本地 Codex，通过 Windows 命名管道访问明确选定的编辑器实例。
- 提供已有资源枚举、分页场景快照、受限原子批量操作，以及结构化执行结果和错误。
- 提供依赖声明、Codex 配置示例、Windows 启动说明和端到端验收步骤。
- 首版只编辑非播放状态的场景，只使用已加载资源。内置聊天、模型 API、NPC、任意代码执行、远程 HTTP 服务、素材生成、截图、场景存取格式修复、父子层级编辑及通用 Undo/Redo 不在本次范围。

## Capabilities

### New Capabilities

- `editor-automation`: 编辑器状态查询、资源发现、具有明确目标和更新规则的场景编辑、有限撤销与批量事务。
- `editor-mcp`: 本地 MCP 工具契约、编辑器实例连接、请求生命周期与 Codex 接入。

### Modified Capabilities

无。当前主规格目录没有已有能力规格。

## Impact

- `EDITOR/src/main.cpp`、`Application.*`：保留现有资源目录定位与启动诊断，增加桥接选项、生命周期、主线程队列处理及关闭顺序。
- `EDITOR/src/editor/scene/*`、`displays/SceneHierarchyDisplay.cpp`、`SceneDisplay.cpp`、`AssetDisplay.cpp`、`MenuDisplay.cpp`、`GameDisplay.cpp`、`utilities/ComponentDrawer.cpp`、`ImGuiUtils.cpp`：场景身份、操作入口、选择、上下文变化通知与受支持属性修改的一致性。
- 复用 `CORE/Core/ECS/*`、`CORE/Core/Resources/AssetManager.*`、现有组件和 RapidJSON；按需增加最小的共享更新辅助函数，不引入模型运行时。
- 新增编辑器 automation/bridge 模块及 `tools/engine_mcp/` 下的 Python 包、协议说明、测试和配置样例；更新 EDITOR MSBuild 项目及必要的测试目标。
- Python 运行环境增加官方 MCP SDK 依赖。C++ 引擎不保存模型密钥，模型及对话由 Codex 管理；部署范围为本机 Windows x64。
- 现有场景 JSON 文件格式保持兼容，本次不依赖其实现完整场景回滚。
