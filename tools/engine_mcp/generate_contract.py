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
    material = {
        "material_index": {"type": "integer", "minimum": 0},
        "color": vec(number(0, 1)), "alpha": number(0, 1),
        "metallic": number(0, 1), "roughness": number(0, 1), "ao": number(0, 1),
        "use_texture": {"type": "boolean"},
        "textures": {**obj({k: {"anyOf": [string(), {"type": "null"}]} for k in
                              ("albedo", "normal", "metallic", "roughness", "ao")}), "minProperties": 1},
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
    add("list_assets", "List loaded model/texture/music/soundfx/shader keys. Does not import files. Use a returned model key in create_object.", obj({"asset_type": {"type": "string", "enum": ["model", "texture", "music", "soundfx", "shader"]}, **paging}), True)
    add("create_scene", "Create and activate a uniquely named empty edit scene. Requires current session/revision and a new request_id. Writes are rejected during play.", obj({**common, "name": string()}, [*common, "name"]))

    for name, properties, description in [
        ("create_object", creation, "Create a visible mesh from a loaded model key, or an enabled default point light. Name is not identity; use returned entity_id."),
        ("set_transform", {**target, **transform}, "Update local position, XYZ Euler rotation_degrees, or positive scale. Engine units; no world space. Physical root objects support pose but reject scale edits. For light position use set_light instead."),
        ("set_light", {**target, **light}, "Update an existing light. Scene-space position is independent of Transform. Linear RGB 0..1, intensity 0..10000. Nonzero direction; area half-sizes .001..4.999. Only type-applicable fields allowed."),
        ("set_material", {**target, **material}, "Partially update an existing material slot. Query get_scene_state materials/material_index first. Linear RGB color[3], separate alpha, metallic/roughness/ao all 0..1. textures maps loaded 2D keys or null to clear; use_texture changes only when explicit. No slot creation or shading_model edit. Undoable; supports batches."),
    ]:
        schema = obj({**common, "scene_id": string(), **properties}, [*common, "scene_id", *( ["name", "kind"] if name == "create_object" else ["entity_id"])])
        if name == "create_object":
            schema["oneOf"] = [obj_condition("mesh", True), obj_condition("light", False)]
        else:
            fields = transform if name == "set_transform" else material if name == "set_material" else light
            schema["anyOf"] = [{"required": [k]} for k in fields if k not in ("space", "material_index")]
            if name == "set_material":
                schema["required"].append("material_index")
        add(name, description, schema)

    physics = {
        "body_type": {"enum": ["static", "dynamic", "kinematic"]}, "enable_gravity": {"type": "boolean"},
        "mass": number(0, exclusive=True), "linear_damping": number(0, 1), "angular_damping": number(0, 1),
        "linear_axis_factor": vec(number(0, 1)), "angular_axis_factor": vec(number(0, 1)),
        "shape": {"enum": ["box", "sphere", "capsule"]}, "half_extents": vec(number(0, exclusive=True)),
        "radius": number(0, exclusive=True), "half_height": number(0, exclusive=True), "trigger": {"type": "boolean"},
        "friction": number(0), "bounciness": number(0, 1), "mass_density": number(0, exclusive=True),
    }
    for method in ("add_physics", "set_physics", "remove_physics"):
        fields = {} if method == "remove_physics" else physics
        schema = obj({**common, "scene_id": string(), **target, **fields}, [*common, "scene_id", "entity_id"])
        if method == "set_physics": schema["anyOf"] = [{"required": [k]} for k in fields]
        add(method, "Edit Physics on a root entity with Transform in non-playing mode. Supports box/sphere/capsule, static/dynamic/kinematic. Pose uses set_transform; collider dimensions are engine units independent of visual scale. Values are applied immediately and undoable.", schema)
    add("delete_entity", "Delete a recoverable entity. cascade defaults false; children require explicit true. Unsupported components are rejected. Undo restores fresh public IDs; source assets remain loaded.", obj({**common, "scene_id": string(), **target, "cascade": {"type": "boolean"}}, [*common, "scene_id", "entity_id"]))
    actions = []
    for tool in (t for t in tools if t["name"] in ("create_object", "set_transform", "set_light", "set_material", "add_physics", "set_physics", "remove_physics", "delete_entity")):
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
    add("apply_operations", "Atomically apply 1..32 actions in the current scene. Each item has method and params. create_object params require name and kind; mesh also requires model (the loaded asset key). Example creation: {\"method\":\"create_object\",\"params\":{\"name\":\"Display Model\",\"kind\":\"mesh\",\"model\":\"cube\",\"client_ref\":\"model\"}}. For a light use name, kind=light and optional client_ref. Later set_transform/set_light params use entity_id OR an earlier creation's target_ref, plus the fields to update. One revision and one undo history entry; no scene creation inside a batch.", obj({**common, "scene_id": string(), "operations": {"type": "array", "minItems": 1, "maxItems": 32, "items": {"oneOf": actions}}}, [*common, "scene_id", "operations"]))
    add("undo_operation", "Undo the current undo stack top using its original content operation_id. Manual edits/context changes clear both stacks. Scene entries require the current scene_id; scene lifecycle/global entries may omit it. Use get_history before traversing.", obj({**common, "scene_id": string(), "operation_id": string()}, [*common, "operation_id"]))
    add("get_operation_status", "Look up queued/running/succeeded/failed/cancelled by original session_id/request_id after a timeout or reconnect. Never blindly resend as a NEW write.", obj({"session_id": string(), "request_id": string()}, ["session_id", "request_id"]), True)
    add("get_selection", "Read current edit-scene single selection: entity_id/name or null, scene_id and selection_serial. Use the returned explicit entity_id in content edits. Does not change content revision or undo.", obj({}), True)
    add("set_selection", "Select one entity in the current edit scene, or pass entity_id=null to clear. Synchronizes hierarchy, GO Details and Gizmo. Requires write preconditions and request dedup; does not change content revision or consume content undo. Its receipt operation_id is NOT undoable. No multi-select or camera focus.",
        obj({**common, "scene_id": string(), "entity_id": {"anyOf": [string(), {"type": "null"}]}}, [*common, "scene_id", "entity_id"]))
    tools[-1]["annotations"]["destructiveHint"] = False
    capture_target = {k: v for k, v in common.items() if k != "request_id"}
    add("capture_viewport", "Read a fresh Scene-camera PNG of the current non-playing edit scene, including when Game tab is active. No UI/Gizmo, file writes or camera changes. Default 640x360; integer width/height 64..1024, proportionally reduced to 230400 pixels. Returns image/png plus actual dimensions/frame_id/camera/session/revision. One in flight; capture_busy/timeout/unavailable are explicit. Does not consume undo.",
        obj({**capture_target, "scene_id": string(), "width": {"type": "integer", "minimum": 64, "maximum": 1024},
             "height": {"type": "integer", "minimum": 64, "maximum": 1024}}, [*capture_target, "scene_id"]), True)
    camera = obj({"position": vec(), "front": vec(), "up": vec(), "fov_degrees": number(0, 180, True),
                  "near": number(0, exclusive=True), "far": number(0, exclusive=True)},
                 ["position", "front", "up", "fov_degrees", "near", "far"])
    metadata = {"scene_id": string(), "frame_id": {"type": "integer", "minimum": 1},
                "width": {"type": "integer", "minimum": 1, "maximum": 1024},
                "height": {"type": "integer", "minimum": 1, "maximum": 1024}, "camera": camera}
    for key, data in (
        ("outputSchema", obj(metadata, list(metadata))),
        ("bridgeOutputSchema", obj({**metadata, "encoding": {"enum": ["rgb8"]}, "origin": {"enum": ["bottom_left"]},
                                   "pixels_base64": {"type": "string", "maxLength": 921600}},
                                  [*metadata, "encoding", "origin", "pixels_base64"])),
    ):
        schema = copy.deepcopy(response)
        schema["properties"]["data"] = data
        schema["anyOf"] = [{"properties": {"status": {"enum": ["succeeded"]}},
                            "required": ["data", "session_id", "editor_revision"]},
                           {"properties": {"status": {"enum": ["failed", "cancelled"]}}, "required": ["error"]}]
        tools[-1][key] = schema
    add("get_history", "Read ordered undo/redo entries, original content operation IDs, scope, counts and memory limits. Cursors expire on revision or history changes.", obj({"stack": {"enum": ["undo", "redo"]}, **paging}), True)
    add("redo_operation", "Redo the current redo stack top using its original content operation_id. Returns fresh IDs when recreating entities or scenes. Does not replay the original request.", obj({**common, "scene_id": string(), "operation_id": string()}, [*common, "operation_id"]))
    path = {"type": "string", "minLength": 1, "maxLength": 32767}
    imports = []
    for kind in ("model", "texture", "music", "soundfx", "shader"):
        fields = {**common, "asset_type": {"enum": [kind]}, "asset_key": string()}
        required = [*common, "asset_type", "asset_key"]
        if kind == "shader":
            fields.update(vertex_path=path, fragment_path=path); required += ["vertex_path", "fragment_path"]
        else:
            fields["path"] = path; required += ["path"]
            if kind == "texture": fields["pixel_art"] = {"type": "boolean"}
        imports.append(obj(fields, required))
    add("import_asset", "Import an absolute local model/2D texture/music/soundfx file, or vertex+fragment Shader files, under an explicit unused asset_key. Loads this session only; never copies or modifies source files. Global scope; no scene_id. Model dependencies commit atomically. May return pending; query the original request. Undo/redo retain original payloads.", {"type": "object", "oneOf": imports})
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
