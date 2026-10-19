# Editor automation

`EditorOperations` owns session-scoped identities, coherent state queries, typed
mutation planning, one atomic transaction and a single undo slot. All entry
points run on its construction thread. `CommandQueue` owns thread-safe admission,
request receipts, deduplication, expiry and cancellation. The named-pipe worker
never touches ECS, ImGui, GPU resources or the Logger vector.

`Contract` loads `tools/engine_mcp/contracts/tools.json`, shared with Python.
`StateRules.h` contains the rotation/entity creation helpers shared by UI and
automation. UI scalar/vector edits notify the manual edit serial immediately;
structural/context/resource changes are also detected by the main-thread state
observer. Scene rename preserves the SceneObject and external scene identity.
Selection-reset notifications prevent stale editor selections following undo.

Runtime order: initialize editor -> enable bridge explicitly -> observe and drain
before display Update -> observe after UI Render -> stop/cancel queue and pipe
workers before scene/GPU cleanup. Without the startup flag no runtime/listener is
created. Existing log stdout does not share the Python MCP stdout.

For a query example, call get_editor_status with `{}`, then get_scene_state with
`{"page_size":50}`. A single transform write uses session_id/request_id,
expected_revision, scene_id, entity_id and `{"rotation_degrees":[0,30,0]}`.
Use set_light.position for lights: their position is not Transform.position.
The executable examples and Codex configuration are in
`tools/engine_mcp/README.md`; `engine_mcp.demo` prints real responses for every step.

Transactions prevalidate target/asset/type/reference constraints before applying
and retain inverse component values/created-object handles. They never serialize
the entire project. Failure injection hooks belong to native tests, not IPC.
Ordinary rollback leaves the prior undo record valid; unrecoverable rollback
disables writes until restart. Changing the scene, play state, assets or content
manually invalidates undo. A newer successful write replaces it; undo clears it.

The native tests verify these rules, including “AI rotates -> user moves ->
history_conflict without overwriting the user position”. To reproduce visually,
run the SDK demo with --keep, change a property/Gizmo in the editor and ask Codex
to undo the printed operation_id after refreshing the current revision.
