# Proposal

## Why

当前 Play 直接更新编辑 Registry，Stop 只关闭更新开关，无法恢复播放前的场景；Lua 入口固定为 main.lua，Scene 切换和脚本生命周期没有明确边界。需要把编辑数据与游戏运行数据隔离，提供可暂停、可恢复、可重新开始的场景播放，并建立 Scene 与同名主脚本的绑定。

## What Changes

- 增加统一 Editing / Playing / Paused 状态控制器；Play 从编辑数据创建独立运行场景，Pause 冻结更新，Resume 继续，Stop 销毁运行数据并恢复编辑视图。
- 沿用 SceneObject 双 Registry，重建实体关系、Lua、物理及游戏相机；Game 与 Scene 在播放和暂停期间渲染同一运行 Registry，模拟每帧只更新一次。
- 增加 Pause Game 按钮，按现有 Play/Stop 内嵌 PNG 字节数组方式集成 `G:\Microsoft Edge下载\pause.png`；明确三按钮高亮、禁用和 Resume 提示。
- 统一可暂停的游戏时间、固定步长物理、音频及 Game 视口焦点输入；Playing / Paused 禁止编辑内容及切换场景，允许只读查看和 Scene 相机导航。
- 启动创建并激活 default Scene，并在编辑态执行一次可选 on_build 构建初始对象；创建 Scene 自动生成或复用 `EDITOR/assets/scripts/<scene-name>.lua`，重命名同步重命名脚本，删除或撤销创建保留脚本，重做绑定已有文件。
- **BREAKING**：Scene 不再自动执行公共 main.lua / GameDemo 资源加载，改用返回可选 `on_build` 及必需 `on_start / on_update(dt) / on_stop` 回调表的同名 Lua；新建 Scene 时在独立构建环境执行 on_build 一次，切换只绑定和编译检查，Play 时在全新运行环境重新读取并只执行游戏生命周期。运行脚本不得新增或替换全局共享资源。
- UI 和 MCP 使用一致的播放保护、状态查询及历史失效规则；保留现有工具集，不新增远程播放控制工具。
- JSON 加载、格式迁移和通过 JSON 实现播放快照不在本次范围；不引入自动保存或脚本热重载。

## Capabilities

### New Capabilities

- `editor-playback`：统一播放状态、运行副本隔离、双视口渲染、游戏时间/物理/音频/输入、运行失败恢复及播放期编辑保护。
- `editor-scene-lifecycle`：默认可编辑初始场景、Scene 命名与同名脚本文件管理、语法检查、场景脚本生命周期和文件操作失败处理。

### Modified Capabilities

- `editor-automation`：明确暂停仍属于运行会话；查询、创建 Scene、编辑权限、revision 和撤销重做适配运行隔离及脚本文件保留规则。
- `editor-mcp`：在现有状态结果中增加可区分三状态的字段，保留旧布尔字段和 21 个工具；同步工具说明与无 computer-use 的验收约定。
- `editor-scene-capture`：明确截图只允许 Editing；Playing 和 Paused 均拒绝，避免将运行副本误作编辑场景图片。

## Impact

- 涉及 EDITOR 的 Application、SceneObject/SceneManager、GameDisplay/SceneDisplay、层级/属性/资源/菜单操作、ToolContext、自动化操作及原生测试项目；新增统一播放与脚本文件管理模块。
- 涉及 CORE 的 ScriptingSystem、Registry/组件复制、Lua 输入/时间/资源/声音绑定及 PhysicsSystem；运行音频按会话追踪，运行材质与编辑材质分离。
- 扩展原生无窗口测试与 Python MCP 契约/集成回归，更新工具契约、协议和验证文档。验证禁止 computer-use、CUA 及其他键鼠/窗口 UI 自动化；视觉、焦点和听觉体验难以自动验证的项目列为待人工验证，不以逻辑测试冒充完成。
- 本阶段只生成规划文档，不修改项目代码；实现与测试执行留给后续 apply。
