"""Repeatable SDK -> stdio MCP -> real editor acceptance workflow."""
import argparse
import asyncio
import json
import sys
import uuid
from mcp import Client
from mcp.client.stdio import StdioServerParameters


async def demo(instance, model, keep, pause):
    command = StdioServerParameters(command=sys.executable, args=["-m", "engine_mcp.server", "--instance", instance])
    async with Client(command) as client:
        async def call(name, arguments):
            reply = await client.call_tool(name, arguments)
            result = reply.structured_content
            for _ in range(100):
                if result["status"] not in {"pending", "queued", "running"}:
                    break
                await asyncio.sleep(.1)
                result = (await client.call_tool("get_operation_status", {
                    "session_id": result["session_id"], "request_id": result["request_id"]})).structured_content
            if result["status"] != "succeeded":
                raise RuntimeError(json.dumps(result, ensure_ascii=False))
            print(json.dumps({"tool": name, "result": result}, ensure_ascii=False), flush=True)
            return result

        def params(state, scene=None):
            data = {"session_id": state["session_id"], "request_id": uuid.uuid4().hex, "expected_revision": state["editor_revision"]}
            if scene:
                data["scene_id"] = scene
            return data

        status = await call("get_editor_status", {})
        assets = await call("list_assets", {"asset_type": "model", "page_size": 200})
        if model not in {a["key"] for a in assets["data"]["assets"]}:
            raise RuntimeError(f"Model {model!r} is not loaded. Import it through the editor or use --model cube.")
        created = await call("create_scene", {**params(status), "name": "MCP Display " + uuid.uuid4().hex[:8]})
        scene = created["data"]["scene_id"]
        batch = await call("apply_operations", {**params(created, scene), "operations": [
            {"method": "create_object", "params": {"name": "Display Model", "kind": "mesh", "model": model, "client_ref": "model"}},
            {"method": "set_transform", "params": {"target_ref": "model", "position": [0, 0, 0], "rotation_degrees": [0, 30, 0]}},
            {"method": "create_object", "params": {"name": "Warm Key Light", "kind": "light", "client_ref": "light"}},
            {"method": "set_light", "params": {"target_ref": "light", "position": [-3, 4, 2], "color": [1, .65, .35], "intensity": 250, "range": 25}},
        ]})
        state = await call("get_scene_state", {"scene_id": scene})
        assert state["data"]["total"] == 2, state
        if pause:
            await asyncio.sleep(pause)
        if not keep:
            await call("undo_operation", {**params(state, scene), "operation_id": batch["operation_id"]})
            final = await call("get_scene_state", {"scene_id": scene})
            assert final["data"]["entities"] == [], final
        return {"scene_id": scene, "operation_id": batch["operation_id"], "session_id": state["session_id"],
                "editor_revision": state["editor_revision"], "references": batch["data"]["references"], "kept": keep}


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--instance", required=True)
    p.add_argument("--model", default="cube")
    p.add_argument("--keep", action="store_true", help="Keep demo objects for visual inspection")
    p.add_argument("--pause-seconds", type=float, default=0)
    args = p.parse_args()
    if not 0 <= args.pause_seconds <= 60:
        p.error("pause-seconds must be in [0,60]")
    result = asyncio.run(demo(args.instance, args.model, args.keep, args.pause_seconds))
    print(json.dumps({"demo": result}, ensure_ascii=False), flush=True)


if __name__ == "__main__":
    main()
