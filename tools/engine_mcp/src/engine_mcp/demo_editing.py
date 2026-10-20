"""Exercise imports, physics, deletion and ordered history in a dedicated real editor instance."""
from __future__ import annotations
import argparse
import asyncio
import base64
import hashlib
import json
from pathlib import Path
import sys
import time
import uuid
from mcp import Client
from mcp.client.stdio import StdioServerParameters


async def run(instance: str, output_dir: Path | None = None, keep: bool = False):
    fixtures = Path(__file__).resolve().parents[2] / "tests/fixtures/imports"
    fingerprints = {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in fixtures.iterdir() if p.is_file()}
    events = []
    token = uuid.uuid4().hex[:10]
    operations = []
    async with Client(StdioServerParameters(command=sys.executable, args=["-m", "engine_mcp.server", "--instance", instance])) as client:
        assert len((await client.list_tools()).tools) == 21

        async def call(name, arguments):
            started = time.monotonic()
            answer = await client.call_tool(name, arguments)
            value = answer.structured_content
            for _ in range(650):
                if value["status"] not in {"pending", "queued", "running"}:
                    break
                await asyncio.sleep(.1)
                value = (await client.call_tool("get_operation_status", {"session_id": value["session_id"], "request_id": value["request_id"]})).structured_content
            events.append({"tool": name, "result": value, "seconds": time.monotonic() - started,
                           "has_image": any(item.type == "image" for item in answer.content)})
            if output_dir:
                output_dir.mkdir(parents=True, exist_ok=True)
                (output_dir / "events.json").write_text(json.dumps(events, ensure_ascii=False, indent=2), "utf-8")
            assert not answer.is_error and value["status"] == "succeeded", value
            return value, answer

        def write(value, scene=None):
            args = {"session_id": value["session_id"], "request_id": uuid.uuid4().hex, "expected_revision": value["editor_revision"]}
            if scene:
                args["scene_id"] = scene
            return args

        status, _ = await call("get_editor_status", {})
        assert status["data"]["instance"] == instance and not status["data"]["playing"]
        baseline_scene = status["data"]["current_scene_id"]
        keys = {}
        for kind, source in [("model", "textured.obj"), ("texture", "color.png"), ("shader", "fragment.frag"), ("music", "tone.wav"), ("soundfx", "tone.wav")]:
            key = keys[kind] = f"qa_edit_{token}_{kind}"
            args = {**write(status), "asset_type": kind, "asset_key": key}
            if kind == "shader":
                args.update(vertex_path=str(fixtures / "vertex.vert"), fragment_path=str(fixtures / source))
            else:
                args["path"] = str(fixtures / source)
            status, _ = await call("import_asset", args)
            operations.append(status["operation_id"])
            assets, _ = await call("list_assets", {"asset_type": kind})
            assert any(asset["key"] == key for asset in assets["data"]["assets"])
        status, _ = await call("create_scene", {**write(status), "name": "QA_editing_" + token})
        operations.append(status["operation_id"])
        scene = status["data"]["scene_id"]
        status, _ = await call("apply_operations", {**write(status, scene), "operations": [
            {"method": "create_object", "params": {"kind": "mesh", "model": keys["model"], "name": "QA Body", "client_ref": "body"}},
            {"method": "set_transform", "params": {"target_ref": "body", "position": [0, 2, 0], "scale": [6, 6, 6]}},
            {"method": "set_material", "params": {"target_ref": "body", "material_index": 0, "use_texture": False, "color": [1, .08, .04], "metallic": .25, "roughness": .45}},
            {"method": "create_object", "params": {"kind": "light", "name": "QA Warm", "client_ref": "lamp"}},
            {"method": "set_light", "params": {"target_ref": "lamp", "position": [-3, 5, 5], "intensity": 500, "color": [1, .75, .5], "range": 50}},
        ]})
        operations.append(status["operation_id"])
        body = status["data"]["references"]["body"]
        status, _ = await call("add_physics", {**write(status, scene), "entity_id": body, "body_type": "dynamic", "mass": 3, "half_extents": [6, 3, 1]})
        operations.append(status["operation_id"])
        status, _ = await call("set_physics", {**write(status, scene), "entity_id": body, "shape": "sphere", "radius": 3, "enable_gravity": False})
        operations.append(status["operation_id"])
        status, _ = await call("set_transform", {**write(status, scene), "entity_id": body, "position": [1, 3, 0], "rotation_degrees": [0, 15, 0]})
        operations.append(status["operation_id"])

        async def capture(label):
            current, _ = await call("get_editor_status", {})
            picture, answer = await call("capture_viewport", {"session_id": current["session_id"], "scene_id": current["data"]["current_scene_id"], "expected_revision": current["editor_revision"]})
            png = base64.b64decode(next(item.data for item in answer.content if item.type == "image"), validate=True)
            if output_dir:
                (output_dir / (label + ".png")).write_bytes(png)
            return picture

        before, _ = await call("get_scene_state", {})
        physical = next(e for e in before["data"]["entities"] if e["name"] == "QA Body")
        assert physical["physics"]["mass"] == 3 and physical["physics"]["shape"] == "sphere"
        first = await capture("before-delete")
        status, _ = await call("delete_entity", {**write(before, scene), "entity_id": body})
        operations.append(status["operation_id"])
        deleted = await capture("after-delete")
        assert deleted["data"]["frame_id"] > first["data"]["frame_id"]

        async def traverse(method, sequence):
            current = None
            for operation in sequence:
                current, _ = await call("get_editor_status", {})
                history, _ = await call("get_history", {"stack": "undo" if method == "undo_operation" else "redo", "page_size": 1})
                row = history["data"]["entries"][0]
                assert row["operation_id"] == operation
                args = {**write(current), "operation_id": operation}
                if row["scope"] != "assets" and current["data"]["current_scene_id"]:
                    args["scene_id"] = current["data"]["current_scene_id"]
                current, _ = await call(method, args)
            return current

        await traverse("undo_operation", reversed(operations))
        empty, _ = await call("get_editor_status", {})
        assert empty["data"]["current_scene_id"] == baseline_scene
        for kind, key in keys.items():
            assets, _ = await call("list_assets", {"asset_type": kind})
            assert not any(asset["key"] == key for asset in assets["data"]["assets"])
        await traverse("redo_operation", operations)
        redo_state, _ = await call("get_scene_state", {})
        assert redo_state["data"]["total"] == 1  # The final content operation deletes the body, preserving its light.
        summary = {"status": "passed", "instance": instance, "operations": len(operations), "calls": len(events), "kept": keep}
        if keep:
            await traverse("undo_operation", [operations[-1]])
            current, _ = await call("get_scene_state", {})
            body = next(e for e in current["data"]["entities"] if e.get("name") == "QA Body")
            await call("set_selection", {**write(current, current["data"]["scene_id"]), "entity_id": body["entity_id"]})
            await capture("restored-for-ui")
            summary.update(session_id=current["session_id"], scene_id=current["data"]["scene_id"], entity_id=body["entity_id"])
        else:
            await traverse("undo_operation", reversed(operations))
        assert {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in fixtures.iterdir() if p.is_file()} == fingerprints
        if output_dir:
            (output_dir / "summary.json").write_text(json.dumps(summary, ensure_ascii=False, indent=2), "utf-8")
        print(json.dumps(summary, ensure_ascii=False))
        return summary


def main():
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--instance", required=True)
    parser.add_argument("--output-dir", type=Path)
    parser.add_argument("--keep", action="store_true", help="Leave only the dedicated QA scene/resources for manual UI checks")
    args = parser.parse_args()
    asyncio.run(run(args.instance, args.output_dir, args.keep))


if __name__ == "__main__":
    main()
