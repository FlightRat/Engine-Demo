# Editor automation

## Typed transactions and session history

`OperationTransaction.h` holds value-only before/after records. Durable history
addresses logical entities and scenes, not captured ECS handles. Restoring a
destroyed object assigns a new public ID and returns `entity_id_remap` or
`scene_id_remap`; subsequent history entries follow the logical binding. Old
request receipts remain historical and never make old public IDs valid again.

`OperationHistory` owns globally ordered undo/redo stacks (64 entries combined,
256 MiB estimated value storage by default). Admission happens before applying
the transaction. Failed validation or rollback preserves both stacks. A new
successful content edit clears redo; external content/context changes clear
both. Selection and capture do neither. The request deduplication ledger is
independent of history eviction.

Use `get_history` to obtain the top original content `operation_id`, then
`undo_operation` or `redo_operation` with a fresh request ID and current revision.
History traversal has its own receipt and does not add another content entry.
History queries are paginated and bound to revision and history serial.

`EditorOperations` owns session-scoped identities, coherent state queries, typed
mutation planning, atomic typed transactions and bounded undo/redo stacks. All entry
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
manually invalidates undo. A newer successful content write clears redo; undo moves the original record to redo.

The native tests verify these rules, including “AI rotates -> user moves ->
history_conflict without overwriting the user position”. To reproduce visually,
run the SDK demo with --keep, change a property/Gizmo in the editor and ask Codex
to undo the printed operation_id after refreshing the current revision.

Material updates use `MaterialRules.h`: validate an existing slot before indexing,
stage a complete candidate, check finite [0,1] values and loaded 2D texture keys,
then commit. UI and MCP share this rule; browsing a texture map does not insert
keys. The scene snapshot supplies material_index, RGB and separate alpha. For a
single write add entity_id/material_index/color/metallic/roughness to the usual
session/request/revision/scene envelope; batch set_material also accepts target_ref.

Selection is main-thread shared state in StateRules.h, with a weak scene owner,
the tool-context entity and selection serial. Hierarchy, properties and Gizmo
observe the same entity. get_selection has no arguments; set_selection uses the
normal write envelope and entity_id or null. Selection receipts are deduplicated
but never replace content history or increment editor_revision. Deleted targets
and scene changes clear selection; actual scene changes still invalidate history.

CaptureCoordinator owns one deferred queue ticket. SceneCapture renders using a
Scene camera copy and independent bounded targets, then polls a PBO fence with
zero wait. It runs even when the Scene tab is hidden. GL stays on the owner
thread; immutable RGB8 serialization and PNG encoding run off the editor loop.
Capture sources are injected in tests; the ordinary headless host has none and
reports capture_unavailable. Synthetic images only verify transport/encoding.
Successful images are real render evidence only when produced by EDITOR.

Captures expire after 5 seconds and never enter the write ledger. Shutdown makes
the main GL context current, stops queue admission, cancels capture, joins CPU
work and destroys GL capture resources before scene cleanup. No image is saved
unless the caller explicitly requests evidence output in the demo.

## Physics, snapshots and deferred imports

PhysicsEditing validates/applies candidate configurations without Lua or GUI
initialization. UI drafts are separate; Apply commits through the shared rules,
while manual pose edits synchronize the body and interpolation. Snapshots retain
PhysicsAttributes only. PhysicsComponent swaps complete resource sets and destroys
collider before shape/body, keeping user data alive until the body is gone.
CaptureEntity enumerates unsupported storages before reversible deletion.

AssetImportCoordinator owns one deferred ticket. PreparedAssets reads bounded
immutable source snapshots and parses models/images on a worker. ImportUpload
uploads one mesh/image per tick. AssetManager Publish/Withdraw prepare replacement
tables before swapping, so dependency publication is atomic. Provenance verifies
texture reuse; rename/delete align provenance lifetime with registrations.
ImportBundle retains loaded results and bytes needed for texture verification or
streaming, participates in history budgeting, and never writes source files.
Music retains its memory stream source until the decoder dies.

Queue cancellation can win until BeginImportCommit claims the ticket. Shutdown
stops admission, cancels/discards import, joins CPU work, then destroys resources
with their contexts valid. Native test hooks and headless test_mesh are neither
MCP arguments nor production resource/rendering backends.
