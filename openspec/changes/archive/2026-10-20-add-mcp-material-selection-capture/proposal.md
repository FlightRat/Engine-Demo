# Proposal

## Why

现有 MCP 已能搭建场景和调整灯光，但无法修改材质、理解用户当前选择，也无法把实际 Scene 画面返回给助手。增加这三类能力，使用户能对选中物体描述外观修改，并通过真实渲染结果检查修改效果。

## What Changes

- 新增 `set_material`，按实体和现有材质槽修改线性 RGB、alpha、metallic、roughness、ao、纹理启用状态及已加载纹理绑定；材质查询复用 `get_scene_state`，返回明确的 `material_index`。
- 将材质修改纳入现有 1–32 项原子批次、请求去重和单层撤销；属性面板与自动化遵循相同校验和修改通知规则。
- 新增单选 `get_selection` / `set_selection`，支持查询、选中和清空，统一层级高亮、GO Details、Gizmo 与工具上下文。选择变更不推进内容 revision、不替换或失效已有内容撤销，但仍校验会话、当前场景及写入前置条件并支持请求去重。
- 新增只读 `capture_viewport`，固定捕获当前编辑场景的 Scene 相机画面，以 PNG MCP 图片内容和结构化元数据返回；即使默认 Game 标签处于前台，也按需生成新画面，不返回陈旧帧、不切换标签或修改相机。
- 保持现有十个工具、bridge-v1、1 MiB 单消息限制及本地实例隔离规则；为图片设置分辨率、压缩后大小和并发上限。
- 本次不增加相机控制/聚焦、多选、显示开关、复制/删除、保存加载、磁盘资源导入、着色模型切换或连续视频。

## Capabilities

### New Capabilities

- `editor-scene-capture`: 有界、与场景版本一致的 Scene 渲染捕获，包含隐藏标签下的新帧生成、图片元数据、失败和生命周期规则。

### Modified Capabilities

- `editor-automation`: 材质编辑及查询、包含材质的原子批次与单层撤销、UI 一致的单选状态，以及选择操作对内容历史的明确例外。
- `editor-mcp`: 四个新增工具的发现与契约、选择操作回执，以及 PNG 图片内容和结构化元数据的输出。

## Impact

- 编辑器：`editor/automation/`、`editor/bridge/CommandQueue.*`、`SceneHierarchyDisplay.*`、`ComponentDrawer.cpp`、`ToolContext.h`、`SceneDisplay.*` 和 `Application.cpp`。
- 复用 `MeshRender::materials`、`TextureRegistry`、现有材质快照、Scene 相机及 `RenderSystem::DeferredRenderPipeline`；新增编辑器侧捕获协调和有界像素读回，不在 IPC 线程调用 OpenGL。
- Python：扩展契约生成器、两份生成的共享契约、写请求分类、截图响应校验及 `server.py` 的 MCP 图片输出；默认不增加运行时依赖，PNG 编码采用标准库实现的有界 RGB 编码路径。
- 测试与交付：扩展原生 ECS/队列测试、Python 契约/stdio 测试，并增加真实 OpenGL 编辑器截图和选择交互验收；保留前一变更未完成验收记录的原始状态。
- 本变更只产生规划文档；实现和主规格同步分别在后续 apply / 归档阶段执行。
