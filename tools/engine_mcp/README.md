# Engine-Demo MCP

本地 Codex 通过 stdio MCP Server 连接正在运行的 Windows 编辑器。
模型与对话由 Codex 管理；本包不调用模型 API，也不要求模型密钥。
首版支持非播放场景的已加载资源、局部变换、灯光、原子批次和一层撤销。

## 安装与构建

需要 Windows x64、Visual Studio v143/Windows SDK、Python 3.11+。
以下 PowerShell 命令从仓库根目录运行，不需要激活虚拟环境：

```powershell
python -m venv tools/engine_mcp/.venv
& tools/engine_mcp/.venv/Scripts/python.exe -m pip install -r tools/engine_mcp/requirements.lock
& tools/engine_mcp/.venv/Scripts/python.exe -m pip install --no-build-isolation --no-deps -e tools/engine_mcp
python tools/engine_mcp/build.py editor
python tools/engine_mcp/build.py tests
```

锁文件包含实测依赖及构建工具；不使用 `--no-deps` 安装未准备依赖的环境。
build.py 从开发者环境寻找 MSBuild，也可传 `--msbuild '完整路径/MSBuild.exe'`。
脚本为各项目传入正确 SolutionDir，并消除重复 Path/PATH 环境键；构建日志保存在本目录的忽略文件中。
`--configuration Release` 构建 Release。现有编辑器资源目录定位及启动诊断保持原样。

## 启用指定实例

编辑器与 Codex 的本地执行器必须使用同一 Windows 用户。桥接默认关闭：

```powershell
& './x64/Debug/EDITOR.exe' --enable-mcp-bridge --mcp-instance default
```

编辑器自己定位仓库内的 EDITOR/assets。MCP Server 的 cwd 不负责引擎资源目录。
同名实例冲突会停用新进程的桥接并记录错误，保留手动编辑器。
可以用另一个名称运行第二个实例，但单个实例仅接受一个桥接客户端。
实例名仅允许 1..64 个英文字母、数字、下划线或连字符。

## 连接 Codex

把 `codex-config.example.toml` 的表复制到你自己的 Codex 配置，修改其中的绝对路径。
本仓库不会自动写入用户全局配置。CLI 等效命令为：

```powershell
codex mcp add engine_demo -- 'D:/代码-CG/Engine-Demo/tools/engine_mcp/.venv/Scripts/python.exe' -m engine_mcp.server --instance default
codex mcp list
```

在新建或重新加载配置的本地 Codex 会话中先调用 get_editor_status，再调用 get_scene_state。
`mcp list` 只验证配置注册；实际查询到 session、实例和场景才证明连通。
使用既有 Codex 登录和模型配置；编辑器桥接不提供模型，也不管理模型费用。
官方配置文档：[Codex MCP](https://learn.chatgpt.com/docs/extend/mcp?surface=cli)。

## 工具与使用约定

| 工具 | 作用 |
| --- | --- |
| get_editor_status | 实例、会话、revision、播放状态及可撤销 operation_id |
| get_scene_state | 场景清单、当前或指定场景快照、名称/组件过滤与分页 |
| list_assets | 已加载 model/texture 的键，默认页长 50，最大 200 |
| create_scene | 创建并激活空场景；名称唯一 |
| create_object | 从已加载模型创建可见 mesh，或创建启用的点光源 |
| set_transform | 局部位置、XYZ 欧拉角（度）和正缩放 |
| set_light | 灯光类型、线性 RGB、强度及适用的类型专属参数 |
| apply_operations | 当前场景内最多 32 个创建/变换/灯光动作，失败整体恢复 |
| undo_operation | 撤销最近一次仍有效的自动化操作或批次 |
| get_operation_status | 根据原 session_id/request_id 查询排队或最终结果 |

先查询，再把返回的 session_id/editor_revision 与新的 request_id 用于写入。
除 create_scene 外，写操作必须指向当前 scene_id。重名物体用 entity_id 区分。
不要自行猜测实体 ID；删除、加载或编辑器重启后旧 ID 失效。
场景/资源/播放状态变化会使旧分页游标和写入前置条件失效，需重新查询。

Transform 使用引擎单位和 local 空间，根对象 local 即 world；不支持 world 转换及物理对象变换。
灯光位置使用 set_light.position，与 Transform 独立。点光衰减常数必须 >0，方向光方向不得为零。
颜色为线性 RGB [0,1]，强度 [0,10000]，面积光半宽/高 [.001,4.999]，完整边界见 PROTOCOL.md。
切换灯光类型保留颜色、强度和启用状态，其他类型字段恢复有效默认值。

使用 apply_operations 将一次布局组合为一个撤销单元。client_ref 唯一，target_ref 只能引用批内更早创建的对象。
场景创建单独调用，不放入批次。撤销只有一层：新成功写入会替换记录；人工编辑、资源/场景切换、加载及播放切换会使旧记录失效。
撤销成功后不会恢复更早的撤销记录，不提供 redo。

通信超时不等于操作失败。pending/queued/running 时应查询 get_operation_status；
响应丢失时只有相同 request_id、原参数与原 expected_revision 的重试才会返回同一结果。
不要换一个 ID 重发可能已执行的创建操作。编辑器重启会产生新 session，不能跨会话自动重放。

## 可重复验收

编辑器已加载 cube 等默认模型，可先以它验收；茶壶需要事先通过编辑器导入并查询实际模型键。

```powershell
& tools/engine_mcp/.venv/Scripts/python.exe -m engine_mcp.demo --instance default --model cube --pause-seconds 5
```

该命令使用官方 SDK 启动 stdio Server，依次查询状态/资源、创建唯一展示场景、
在一个批次中创建模型与暖色点光源、查询真实属性、等待 5 秒，再撤销该批次。
预期：视口出现旋转的模型和暖色照明，最后两个物体消失，空展示场景保留。
用 `--keep` 可保留对象供检查，输出包含 operation_id 和实体引用。
要验证人工修改冲突，先 `--keep`，用属性面板或 Gizmo 改动物体，重新查询后请求撤销原 operation_id，
预期 history_conflict 且人工结果不变。

在 Codex 中可输入：

> 使用 engine_demo 工具查询资源和当前状态，创建展示场景，在一个批次中把 cube 放在中央并旋转 30 度，
> 在左上方创建暖色点光源，查询确认后撤销该批次。不要修改代码或执行 Lua。

场景不会自动保存；关闭编辑器会丢失未保存内容。本次没有修复现有 JSON 序列化覆盖范围，不用它实现撤销。

## 测试与故障排查

```powershell
& './x64/Debug/AutomationTests.exe'
& tools/engine_mcp/.venv/Scripts/python.exe -m pytest tools/engine_mcp/tests -q
```

原生测试覆盖真实 ECS、身份、旋转、分页、事务、撤销及队列。
Python 集成测试使用 `AutomationTests.exe --serve <唯一实例>` 提供真实 C++ 桥接，不创建 OpenGL 窗口；
SDK stdio 测试与无编辑器测试也在其中。画面验收必须另外在实际 EDITOR 中运行上面的 demo，不能以这些测试代替。

实际版本、测试范围、真实 EDITOR 的查询结果，以及尚未完成的 Codex/画面验收见
[VERIFICATION.md](VERIFICATION.md)。当前自动测试通过不表示全部接入验收已经完成。

`run_codex_acceptance.py --codex <本地原生codex.exe绝对路径> --instance <实例>`
可用进程级配置运行一次受限验收，不写用户全局配置；它沿用既有 Codex 登录和模型设置，
会产生模型调用。只有实际完成必要的 engine_demo 工具调用才返回成功，输出保存在本目录的 `qa-codex-*.log`。

- editor_unavailable：检查编辑器已打开、启用桥接、实例与 Windows 用户匹配；Server 可先于编辑器启动。
- editor_busy：该实例已被另一个客户端连接；关闭旧连接再试，不要改连其他实例。
- revision_conflict/stale_target：重新查询状态，确认目标后生成新的明确修改请求。
- history_conflict：旧撤销已失效，不会覆盖人工内容；先查看当前状态。
- request_capacity_exceeded：本会话写账本已满；保留旧请求可查询性，正常保存后重开编辑器建立新会话。
- recovery_required：停止自动化写入、检查当前场景，重开编辑器后再继续。
- Server 日志写 stderr；不要把调试打印写 stdout。CLI 参数错误退出码为 2，正常关闭为 0。

回退：关闭带桥接开关的编辑器，以不带该开关的方式重开，并从自己的 Codex 配置移除 engine_demo 表。
手动编辑流程和现有场景文件格式无需迁移。
