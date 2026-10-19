"""Regenerate the language-neutral contract consumed by both bridge peers."""
from __future__ import annotations

import copy
import json
from pathlib import Path


def obj(properties: dict, required: list[str] = ()) -> dict:
    return {"type": "object", "properties": properties, **({"required": list(required)} if required else {}), "additionalProperties": False}


def string() -> dict:
    return {"type": "string", "minLength": 1, "maxLength": 256}


def number(minimum=None, maximum=None, exclusive=False) -> dict:
    result = {"type": "number"}
    if minimum is not None:
        result["minimum"] = minimum
    if maximum is not None:
        result["maximum"] = maximum
    if exclusive:
        result["exclusiveMinimum"] = True
    return result


def vec(item=None) -> dict:
    return {"type": "array", "minItems": 3, "maxItems": 3, "items": item or number()}


def build() -> dict:
    common = {"session_id": string(), "request_id": string(), "expected_revision": {"type": "integer", "minimum": 0}}
    target = {"entity_id": string()}
    transform = {"space": {"type": "string", "enum": ["local"]}, "position": vec(), "rotation_degrees": vec(), "scale": vec(number(0, exclusive=True))}
    light = {
        "type": {"type": "string", "enum": ["point_light", "direction_light", "area_light"]},
        "color": vec(number(0, 1)), "intensity": number(0, 10000), "position": vec(),
        "direction": vec(), "rotation_degrees": vec(), "render": {"type": "boolean"},
        "constant": number(0, 10, True), "linear": number(0, 1), "quadratic": number(0, 1),
        "range": number(0, 10000), "half_width": number(.001, 4.999), "half_height": number(.001, 4.999),
    }
    creation = {
        "name": {**string(), "description": "Required display name for each new object; names may repeat."},
        "kind": {"type": "string", "enum": ["mesh", "light"], "description": "Required object kind."},
        "model": {**string(), "description": "Required for mesh: an exact loaded model key from list_assets. Omit for light."},
    }
    tools = []
    response = obj({
        "status": {"type": "string", "enum": ["queued", "running", "succeeded", "failed", "cancelled", "pending"]},
        "session_id": {"type": ["string", "null"]}, "request_id": {"type": "string"},
        "editor_revision": {"type": "integer", "minimum": 0}, "correlation_id": {"type": "string"},
        "operation_id": {"type": "string"}, "execution_status": {"type": "string"},
        "data": {"type": "object"},
        "error": obj({"code": {"type": "string"}, "message": {"type": "string"}, "details": {"type": "object"}, "retryable": {"type": "boolean"}}, ["code", "message", "details", "retryable"]),
    }, ["status", "request_id"])

    def add(name, description, schema, read=False):
        tools.append({"name": name, "description": description, "inputSchema": schema, "outputSchema": response,
                      "annotations": {"readOnlyHint": read, "destructiveHint": not read, "openWorldHint": False}})

    paging = {"page_size": {"type": "integer", "minimum": 1, "maximum": 200}, "cursor": {"type": "string", "minLength": 1, "maxLength": 4096}}
    add("get_editor_status", "Read connection, instance, session_id, editor_revision, current_scene_id, play state and protocol version. Start here.", obj({}), True)
    add("get_scene_state", "Read edit scene list and stable entity snapshots. Default page=50 (max=200); cursors expire when revision changes. Transforms are local engine units/degrees; lights expose their own scene-space position.", obj({"scene_id": string(), "name_filter": {"type": "string", "maxLength": 256}, "component_filter": {"type": "string", "enum": ["transform", "light", "mesh", "physics", "script"]}, **paging}), True)
    add("list_assets", "List only loaded model/texture keys. Does not import files. Use a returned model key in create_object.", obj({"asset_type": {"type": "string", "enum": ["model", "texture"]}, **paging}), True)
    add("create_scene", "Create and activate a uniquely named empty edit scene. Requires current session/revision and a new request_id. Writes are rejected during play.", obj({**common, "name": string()}, [*common, "name"]))

    for name, properties, description in [
        ("create_object", creation, "Create a visible mesh from a loaded model key, or an enabled default point light. Name is not identity; use returned entity_id."),
        ("set_transform", {**target, **transform}, "Update local position, XYZ Euler rotation_degrees, or positive scale. Engine units; no world space or physics targets. For light position use set_light instead."),
        ("set_light", {**target, **light}, "Update an existing light. Scene-space position is independent of Transform. Linear RGB 0..1, intensity 0..10000. Nonzero direction; area half-sizes .001..4.999. Only type-applicable fields allowed."),
    ]:
        schema = obj({**common, "scene_id": string(), **properties}, [*common, "scene_id", *( ["name", "kind"] if name == "create_object" else ["entity_id"])])
        if name == "create_object":
            schema["oneOf"] = [obj_condition("mesh", True), obj_condition("light", False)]
        else:
            fields = transform if name == "set_transform" else light
            schema["anyOf"] = [{"required": [k]} for k in fields if k != "space"]
        add(name, description, schema)

    actions = []
    for tool in tools[4:7]:
        name = tool["name"]
        schema = copy.deepcopy(tool["inputSchema"])
        for k in [*common, "scene_id"]:
            del schema["properties"][k]
            schema["required"].remove(k)
        if name == "create_object":
            schema["properties"]["client_ref"] = string()
        else:
            schema["required"].remove("entity_id")
            schema["properties"]["target_ref"] = string()
            schema["oneOf"] = [{"required": ["entity_id"], "not": {"required": ["target_ref"]}}, {"required": ["target_ref"], "not": {"required": ["entity_id"]}}]
        if not schema["required"]:
            del schema["required"]
        actions.append(obj({"method": {"type": "string", "enum": [name]}, "params": schema}, ["method", "params"]))
    add("apply_operations", "Atomically apply 1..32 actions in the current scene. Each item has method and params. create_object params require name and kind; mesh also requires model (the loaded asset key). Example creation: {\"method\":\"create_object\",\"params\":{\"name\":\"Display Model\",\"kind\":\"mesh\",\"model\":\"cube\",\"client_ref\":\"model\"}}. For a light use name, kind=light and optional client_ref. Later set_transform/set_light params use entity_id OR an earlier creation's target_ref, plus the fields to update. One revision and one single-level undo record; no scene creation inside a batch.", obj({**common, "scene_id": string(), "operations": {"type": "array", "minItems": 1, "maxItems": 32, "items": {"oneOf": actions}}}, [*common, "scene_id", "operations"]))
    add("undo_operation", "Undo only the latest still-valid operation_id. Manual edits/context changes invalidate it. No redo or older-history undo; successful undo clears the slot.", obj({**common, "scene_id": string(), "operation_id": string()}, [*common, "scene_id", "operation_id"]))
    add("get_operation_status", "Look up queued/running/succeeded/failed/cancelled by original session_id/request_id after a timeout or reconnect. Never blindly resend as a NEW write.", obj({"session_id": string(), "request_id": string()}, ["session_id", "request_id"]), True)
    return {"protocol_version": 1, "tools": tools}


def obj_condition(kind: str, model: bool) -> dict:
    return {"properties": {"kind": {"enum": [kind]}}, **({"required": ["model"]} if model else {"not": {"required": ["model"]}})}


if __name__ == "__main__":
    output = Path(__file__).parent / "contracts" / "tools.json"
    output.parent.mkdir(parents=True, exist_ok=True)
    content = json.dumps(build(), ensure_ascii=False, indent=2) + "\n"
    output.write_text(content, encoding="utf-8")
    packaged = Path(__file__).parent / "src/engine_mcp/contracts/tools.json"
    packaged.parent.mkdir(parents=True, exist_ok=True)
    packaged.write_text(content, encoding="utf-8")
