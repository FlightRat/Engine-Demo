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
Physics targets are unsupported. Light position is independent scene-space
position, not Transform.position. Colors: linear RGB [0,1]; intensity [0,10000];
nonzero normalized direction; area half-sizes [.001,4.999]; constant (0,10],
linear/quadratic [0,1], range [0,10000]. Omitted fields stay unchanged, except
type switches reset type-specific defaults. Only type-applicable fields allowed.

A batch prevalidates all actions and back references, commits one revision and
one operation_id, or restores the original supported state. Single-level undo
restores old components/removes created objects; newer successful writes replace
the undo slot. Manual content/resource/context changes invalidate the slot.
Undo clears it and does not reopen older history. Recovery failure disables
writes until editor restart. Scene JSON is never used as an undo snapshot.

Example hello:
```json
{"protocol_version":1,"correlation_id":"q1","method":"hello","params":{}}
```
Example write:
```json
{"protocol_version":1,"correlation_id":"q2","method":"create_scene","params":{"session_id":"SESSION_FROM_HELLO","request_id":"unique-write-1","expected_revision":0,"name":"Display"}}
```
