"""Real-editor material/selection/image smoke. Only creates a unique QA scene."""
from __future__ import annotations
import argparse
import asyncio
import base64
import json
from pathlib import Path
import sys
import time
import uuid
from mcp import Client
from mcp.client.stdio import StdioServerParameters


async def run(instance: str, output_dir: Path | None):
    events = []
    async with Client(StdioServerParameters(command=sys.executable, args=["-m", "engine_mcp.server", "--instance", instance])) as client:
        async def call(name, arguments, *, error=None):
            started = time.monotonic()
            answer = await client.call_tool(name, arguments)
            value = answer.structured_content
            events.append({"tool": name, "seconds": time.monotonic() - started, "result": value})
            for _ in range(100):
                if value["status"] not in {"pending", "queued", "running"}:
                    break
                await asyncio.sleep(.1)
                value = (await client.call_tool("get_operation_status", {"session_id": value["session_id"], "request_id": value["request_id"]})).structured_content
                events.append({"tool": "get_operation_status", "result": value})
            if output_dir:
                output_dir.mkdir(parents=True, exist_ok=True)
                (output_dir / "events.json").write_text(json.dumps(events, ensure_ascii=False, indent=2), "utf-8")
            if error:
                assert answer.is_error and value["error"]["code"] == error, value
            else:
                assert not answer.is_error and value["status"] == "succeeded", value
            return value, answer

        def write(value, scene=None):
            p = {"session_id": value["session_id"], "request_id": uuid.uuid4().hex, "expected_revision": value["editor_revision"]}
            if scene:
                p["scene_id"] = scene
            return p

        status, _ = await call("get_editor_status", {})
        assert status["data"]["instance"] == instance
        await call("list_assets", {"asset_type": "model"})
        created, _ = await call("create_scene", {**write(status), "name": "QA_visual_" + uuid.uuid4().hex[:10]})
        scene = created["data"]["scene_id"]
        layout, _ = await call("apply_operations", {**write(created, scene), "operations": [
            {"method": "create_object", "params": {"name": "Cube", "kind": "mesh", "model": "cube", "client_ref": "cube"}},
            {"method": "set_transform", "params": {"target_ref": "cube", "rotation_degrees": [0, 30, 0], "scale": [4, 4, 4]}},
            {"method": "create_object", "params": {"name": "Warm", "kind": "light", "client_ref": "lamp"}},
            {"method": "set_light", "params": {"target_ref": "lamp", "position": [-3, 5, -3], "color": [1, .65, .35], "intensity": 500, "range": 50}},
            {"method": "set_material", "params": {"target_ref": "cube", "material_index": 0, "use_texture": False, "metallic": 0, "roughness": .7}},
        ]})
        cube = layout["data"]["references"]["cube"]
        before, _ = await call("get_scene_state", {})
        original = next(e for e in before["data"]["entities"] if e["entity_id"] == cube)["materials"]
        selected_args = {**write(layout, scene), "entity_id": cube}
        selected, _ = await call("set_selection", selected_args)
        selection, _ = await call("get_selection", {})
        assert selection["data"]["entity_id"] == cube and selected["editor_revision"] == layout["editor_revision"]
        styled, _ = await call("apply_operations", {**write(selected, scene), "operations": [
            {"method": "set_material", "params": {"entity_id": cube, "material_index": 0, "color": [1, .02, .02], "metallic": .6, "roughness": .25}},
            {"method": "set_material", "params": {"entity_id": cube, "material_index": 0, "ao": .9}},
        ]})
        pictures = []

        async def capture(value, label, **dimensions):
            value, answer = await call("capture_viewport", {"session_id": value["session_id"], "scene_id": scene, "expected_revision": value["editor_revision"], **dimensions})
            image = next(item for item in answer.content if item.type == "image")
            assert image.mime_type == "image/png"
            png = base64.b64decode(image.data, validate=True)
            assert png.startswith(b"\x89PNG\r\n\x1a\n") and len(png) <= 700 * 1024
            assert "pixels_base64" not in json.dumps(value)
            pictures.append((value["data"], png))
            if output_dir:
                output_dir.mkdir(parents=True, exist_ok=True)
                (output_dir / (label + ".png")).write_bytes(png)
            return value

        first_image = await capture(styled, "material-red")
        again = await capture(styled, "material-large", width=1024, height=1024)
        assert again["data"]["frame_id"] > first_image["data"]["frame_id"]
        assert again["data"]["width"] * again["data"]["height"] <= 230400
        assert again["editor_revision"] == styled["editor_revision"]
        cleared, _ = await call("set_selection", {**write(styled, scene), "entity_id": None})
        await call("set_selection", selected_args)  # Exact original retry must not reselect.
        empty, _ = await call("get_selection", {})
        assert empty["data"]["entity_id"] is None
        await call("set_material", {**write(cleared, scene), "entity_id": cube, "material_index": 999, "metallic": 0}, error="invalid_arguments")
        await call("set_material", {**write(cleared, scene), "entity_id": cube, "material_index": 0, "textures": {"albedo": "missing_qa_texture"}}, error="asset_not_found")
        undo, _ = await call("undo_operation", {**write(cleared, scene), "operation_id": styled["operation_id"]})
        after, _ = await call("get_scene_state", {})
        assert next(e for e in after["data"]["entities"] if e["entity_id"] == cube)["materials"] == original
        restored = await capture(undo, "material-restored")
        assert restored["data"]["camera"] == first_image["data"]["camera"]
        assert pictures[0][1] != pictures[-1][1], "Material change did not produce different rendered pixels"
        if output_dir:
            (output_dir / "events.json").write_text(json.dumps(events, ensure_ascii=False, indent=2), "utf-8")
        print(json.dumps({"status": "passed", "instance": instance, "scene_id": scene, "calls": len(events),
                          "captures": [metadata for metadata, _ in pictures], "evidence": str(output_dir) if output_dir else None}, ensure_ascii=False))


def main():
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--instance", required=True)
    parser.add_argument("--output-dir", type=Path, help="Explicitly save PNG and tool evidence; otherwise no files are written")
    args = parser.parse_args()
    asyncio.run(run(args.instance, args.output_dir))


if __name__ == "__main__":
    main()
