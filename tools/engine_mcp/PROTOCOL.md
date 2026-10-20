# Engine bridge v1

The committed `contracts/tools.json` is the tool argument contract for C++ and
Python. Regenerate it with `python tools/engine_mcp/generate_contract.py` and test
both consumers. It is NOT the MCP wire protocol; the SDK owns MCP negotiation.
The internal schema uses Draft-4 for RapidJSON. MCP discovery publishes equivalent
2020-12 schemas (numeric exclusiveMinimum); tests verify both dialects and bounds.

Local IPC: four-byte unsigned little-endian length followed by UTF-8 JSON,
1..1,048,576 bytes. Reject malformed UTF-8, duplicate keys, unknown fields,
unknown versions and non-finite/float-overflow numbers. Partial reads/writes
must be completed before decoding. Never log to the MCP server's stdout.

Request envelope: `protocol_version`, `correlation_id`, `method`, `params`.
`hello` has empty params and returns session and revision. Other methods use
the published tool arguments. Every write requires session_id, request_id,
expected_revision and (except create_scene) scene_id. Correlation IDs identify
transport exchanges; request IDs identify user writes and survive reconnection.
Fingerprint the method and canonical params, not correlation IDs or deadlines.

Response: correlation_id, request_id, session_id, editor_revision, status,
optional operation_id and data, or error {code,message,details,retryable}.
queued -> running -> succeeded/failed; queued -> cancelled. A wait timeout
returns status=pending and original request_id; poll get_operation_status.
Cancellation only guarantees no effect if it wins before execution begins.
IPC `cancel` takes session_id/request_id and never interrupts running work.

Defaults: queue 128, ledger 4096 accepted writes, queue TTL 5 seconds, response
wait 10 seconds, max 32 batch actions. Full queue: queue_full. Full ledger:
request_capacity_exceeded; never evict earlier entries within a session.
Same request/content returns original status/result BEFORE revision validation;
same request/different content: request_id_conflict. Restart changes session.
Unknown request IDs: request_not_found, not proof that a disconnected write
did not execute. No automatic replay following restart.

Common errors: invalid_arguments, unknown_method, unsupported_version,
editor_unavailable, editor_busy, stale_session, stale_target, revision_conflict,
play_mode, asset_not_found, unsupported_target, queue_full,
request_capacity_exceeded, request_id_conflict, request_not_found,
scene_name_conflict, history_conflict, recovery_required.

Transforms use LOCAL engine units and XYZ Euler DEGREES; positive scale only.
Physical root targets support pose updates but reject scale/world writes. Collider sizes are explicit engine units. Light position is independent scene-space
position, not Transform.position. Colors: linear RGB [0,1]; intensity [0,10000];
nonzero normalized direction; area half-sizes [.001,4.999]; constant (0,10],
linear/quadratic [0,1], range [0,10000]. Omitted fields stay unchanged, except
type switches reset type-specific defaults. Only type-applicable fields allowed.

A batch prevalidates all actions and back references, commits one revision and
one operation_id, or restores the original supported state. Ordered undo/redo restores typed before/after values, using logical targets across entity or scene restoration. Restored public IDs are fresh and returned in remap objects. New successful content writes clear redo; manual content/resource/context changes clear both stacks. Default history limits are 64 entries combined and 256 MiB of estimated retained payload. Failed requests preserve both stacks. Recovery failure disables
writes until editor restart. Scene JSON is never used as an undo snapshot.

Example hello:
```json
{"protocol_version":1,"correlation_id":"q1","method":"hello","params":{}}
```
Example write:
```json
{"protocol_version":1,"correlation_id":"q2","method":"create_scene","params":{"session_id":"SESSION_FROM_HELLO","request_id":"unique-write-1","expected_revision":0,"name":"Display"}}
```

## Material, selection and Scene images

The tool catalog contains 21 tools. New tools are `set_material`,
`get_selection`, `set_selection` and `capture_viewport`; framing remains bridge-v1.
Materials are queried through `get_scene_state.materials`, each with an explicit
zero-based material_index. No material slot is created by an update. Linear RGB
color[3], separate alpha, metallic, roughness and ao must be finite in [0,1].
use_texture is boolean. textures is a nonempty partial map of albedo, normal,
metallic, roughness or ao to a loaded 2D texture key, or null to clear. Clearing
returns an empty string in state queries and does not implicitly toggle use_texture.
shading_model is read-only. set_material is a content write, supports entity_id
or earlier target_ref in batches, and uses complete material inverses for undo.

get_selection has no arguments; data contains current scene_id (empty without an
active scene), nullable entity_id/name and monotonic selection_serial.
set_selection uses normal write session/request/revision/scene preconditions,
with entity_id=null to clear. It has a deduplicated operation receipt and can be
queried using get_operation_status, but its operation_id is not undoable.
Selection alone does not advance content revision or consume/replace/invalidate
content undo. Retrying a selection request returns the old receipt without
reapplying selection over a later manual choice. Play and recovery write guards apply.

Capture is read-only, has no write request_id, and requires session_id, current
scene_id and expected_revision. width/height default to 640/360 and accept
integers 64..1024. Dimensions are proportionally reduced to <=230400 pixels.
The 5-second deadline includes render admission and GPU fence polling. Only one
capture is in flight per instance; errors include capture_busy (retryable),
capture_timeout (retryable), capture_unavailable (retryable), capture_too_large,
invalid_response, and the existing session/revision/target/play errors.

Every successful capture renders a fresh Scene-camera frame even with Game in
front; it does not switch tabs, move the camera, change selection/revision/undo,
save files or include ImGui/Gizmo. Rendering and PBO/fence polling run on the GL
owner thread; immutable CPU pixels are serialized on a worker. Pixel encoding
never waits in the editor loop for IPC or model response. Shutdown cancels
unfinished capture before scene and GL resource destruction; no replay occurs.

Internal data includes scene_id, frame_id, actual width/height, camera
(position/front/up/fov_degrees/near/far), encoding=rgb8, origin=bottom_left and
pixels_base64. Raw RGB is <=691200 bytes, Base64 <=921600 bytes; metadata/envelope
budget is 32 KiB. The complete UTF-8 frame must still be <=1 MiB. A headless host
without a render provider returns capture_unavailable, never a fake screenshot.

The generated contract distinguishes bridgeOutputSchema from the published
outputSchema for captures. Python strictly checks dimensions, byte length and
encoding, flips rows and emits an RGB PNG <=700 KiB using a bounded worker.
The MCP result contains short JSON metadata TextContent plus one image/png
ImageContent; structuredContent contains metadata only. Pixel/Base64 data is
never duplicated into text, metadata or logs. Other tools retain their output.

Example material write (query actual session/revision/IDs first):
```json
{"protocol_version":1,"correlation_id":"material-query-1","method":"set_material","params":{"session_id":"SESSION","request_id":"material-1","expected_revision":1,"scene_id":"scene-1","entity_id":"entity-1","material_index":0,"color":[1,0.1,0.1],"metallic":0.8,"roughness":0.3,"use_texture":false}}
```
Example selection write:
```json
{"protocol_version":1,"correlation_id":"selection-query-1","method":"set_selection","params":{"session_id":"SESSION","request_id":"selection-1","expected_revision":2,"scene_id":"scene-1","entity_id":null}}
```
Example image query:
```json
{"protocol_version":1,"correlation_id":"image-query-1","method":"capture_viewport","params":{"session_id":"SESSION","expected_revision":2,"scene_id":"scene-1","width":640,"height":360}}
```

## Physics, deletion, global import and ordered history

New writes: add_physics/set_physics/remove_physics/delete_entity/redo_operation/import_asset.
get_history is read-only. Public write classification derives from tool annotations;
C++ uses the corresponding explicit method set. Existing valid arguments remain accepted.

Scene writes require current scene_id. import_asset is global and needs no scene.
History scope is scene/scene-lifecycle/assets. Entity history requires scene_id;
lifecycle history verifies recorded context, and asset history works without a scene.
Explicit scene_id on legacy undo requests is still checked. Use the top original
content operation_id, not a receipt ID from undo/redo/selection. History cursors
bind session/revision/stack/history serial. Eviction never evicts the request ledger.

Physics queries return applied values. Collider dimensions are engine units;
pose uses Transform. Physical scale/world/parented-pose writes are rejected.
Deletion snapshots exclude transient selection and runtime resource pointers.
Restoration returns fresh entity_id_remap/scene_id_remap and reconnects supported
parent/physics user-data references. Unsupported deletion components are listed
in error.details.unsupported_components.

Imports keep one write receipt across preparing/uploading/committing. The queue
processes other requests while preparation runs. Committing claims the ticket
against cancellation and revalidates the original revision. A server wait timeout
returns pending. Before publication, cancellation/timeout/disconnect releases
candidates; a delivered pending receipt remains queryable after reconnect. One
import per instance, with a default started-import deadline of 60 seconds.
CPU preparation never touches GL/ECS/Logger. GPU creation and resource/history
release occur on the owner thread. Resource bytes never enter IPC JSON.

Errors include import_busy/import_timeout/import_unavailable/import_failed,
asset_name_conflict/asset_in_use/history_capacity_exceeded and existing context
errors. Failure messages are bounded; large result metadata rolls back before
destructive commit. PNG limits and observational Scene capture are unchanged.


## Playback state extension

Status and scene-query data include `playback_state` with values `editing`,
`playing`, `paused`. Existing `playing` means a runtime session exists, including
Paused. Scene entries derive their `playing` value from session ownership.
State fields are additive; existing request envelopes and the 21 tools remain.
Snapshots and public entity IDs always address edit data. Runtime UI selection
is separate and never becomes an editable MCP entity.

Playing and Paused reject new content/selection/import/history writes with
`play_mode`; capture is Editing-only. Async import/capture revalidate revision
and playback at commit/delivery. Deduplication still precedes execution guards,
so an identical completed request returns its original receipt. Successful
playback transitions (including error-driven pause) increment edit revision and
clear history; runtime frames and camera/inspection changes do not.

`create_scene` also creates or reuses the same-named Lua file. Names are valid
Windows filenames, case-insensitively unique. File failures cannot publish a
Scene or successful history. Undo/delete retain user script bytes; redo binds
the current file, generating a template only if absent. External Lua edits are
not rolled back by content history. Tests use an injected temporary asset root.

This change's verification forbids computer-use and UI input/window automation.
Visual, focus and auditory acceptance without human evidence stays explicitly
pending rather than being inferred from headless test results.
