"""One bounded local Codex/MCP acceptance run; never edits global Codex config."""
import argparse
import json
import math
import os
from pathlib import Path
import subprocess
import threading


def redact_images(value):
    if isinstance(value, list):
        return [redact_images(v) for v in value]
    if isinstance(value, dict):
        return {k: ("<image bytes omitted>" if k == "pixels_base64" or
                     (value.get("type") == "image" and k in {"data", "image_url"}) else redact_images(v))
                for k, v in value.items()}
    if isinstance(value, str) and value.startswith("data:image/"):
        return "<image bytes omitted>"
    return value


def completed_tools(events):
    results = []
    for event in events:
        item = event.get("item", {})
        if event.get("type") != "item.completed" or item.get("type") != "mcp_tool_call" or item.get("server") != "engine_demo":
            continue
        wire = item.get("result") or {}
        result = wire.get("structured_content") or wire.get("structuredContent")
        if not result:
            texts = [c["text"] for c in wire.get("content", []) if c.get("type") == "text"]
            result = json.loads(texts[0]) if texts else {}
        if item.get("status") != "completed" or item.get("error") or wire.get("isError") or wire.get("is_error") or result.get("status") != "succeeded":
            raise ValueError("Editor tool failed or outcome is unresolved")
        results.append((item["tool"], result, wire))
    return results


def verify_visual_events(events):
    calls = completed_tools(events)
    names = {name for name, _, _ in calls}
    required = {"get_editor_status", "list_assets", "create_scene", "apply_operations", "get_scene_state", "get_selection", "set_selection", "capture_viewport", "undo_operation"}
    if not required <= names:
        raise ValueError("Missing visual workflow tools: " + str(sorted(required - names)))
    scene = next(result["data"]["scene_id"] for name, result, _ in calls if name == "create_scene")
    snapshots = [r["data"] for n, r, _ in calls if n == "get_scene_state" and r["data"].get("scene_id") == scene]
    if len(snapshots) < 3 or any(s.get("total") != 2 for s in snapshots):
        raise ValueError("Missing original, modified and restored QA snapshots")
    def cube(state):
        return next(e for e in state["entities"] if e.get("model") == "cube")
    before, final = cube(snapshots[0]), cube(snapshots[-1])
    if before["materials"] != final["materials"]:
        raise ValueError("Material did not restore exactly")
    changed = next((cube(s)["materials"][0] for s in snapshots[1:-1]
                    if math.isclose(cube(s)["materials"][0]["metallic"], .6, abs_tol=1e-5)
                    and math.isclose(cube(s)["materials"][0]["roughness"], .25, abs_tol=1e-5)
                    and not cube(s)["materials"][0]["use_texture"]), None)
    if not changed or any(not math.isclose(a,b,abs_tol=1e-5) for a,b in zip(changed["color"],[1,.02,.02])):
        raise ValueError("Modified material was not confirmed")
    selections = [r["data"]["entity_id"] for n,r,_ in calls if n == "get_selection"]
    if before["entity_id"] not in selections or selections[-1] is not None:
        raise ValueError("Selection and clear were not confirmed")
    images = [(r,w) for n,r,w in calls if n == "capture_viewport"]
    if len(images) < 2 or images[-1][0]["data"]["frame_id"] <= images[0][0]["data"]["frame_id"]:
        raise ValueError("Two fresh capture frames were not confirmed")
    for result, wire in images:
        if result["data"]["scene_id"] != scene or not any(c.get("type") == "image" for c in wire.get("content", [])):
            raise ValueError("Capture is missing its actual MCP image block")
    batches = [r for n,r,_ in calls if n == "apply_operations"]
    undo = [r for n,r,_ in calls if n == "undo_operation"][-1]
    if undo["data"]["undone_operation_id"] != batches[-1]["operation_id"]:
        raise ValueError("Undo did not target the material batch")


def verify_events(events):
    calls = [e.get("item", {}) for e in events if e.get("type") == "item.completed"
             and e.get("item", {}).get("type") == "mcp_tool_call" and e["item"].get("server") == "engine_demo"]
    results = {}
    for call in calls:
        if call.get("status") != "completed" or call.get("error"):
            raise ValueError("An editor tool did not complete successfully")
        wire = call.get("result") or {}
        data = wire.get("structured_content") or wire.get("structuredContent")
        if not data:
            texts = [c["text"] for c in wire.get("content", []) if c.get("type") == "text"]
            data = json.loads(texts[0]) if texts else {}
        if wire.get("isError") or wire.get("is_error") or data.get("status") != "succeeded":
            raise ValueError("Editor tool returned an error or an unresolved outcome")
        results.setdefault(call["tool"], []).append(data)
    required = {"get_editor_status", "list_assets", "create_scene", "apply_operations", "get_scene_state", "undo_operation"}
    if not required.issubset(results):
        raise ValueError("Missing successful engine_demo tools: " + str(sorted(required - results.keys())))
    scene = results["create_scene"][-1]["data"]["scene_id"]
    operation = results["apply_operations"][-1]["operation_id"]
    states = [r["data"] for r in results["get_scene_state"] if r["data"].get("scene_id") == scene]
    populated = next((s for s in states if s.get("total") == 2), None)
    if not populated or states[-1].get("total") != 0 or states[-1].get("entities") != []:
        raise ValueError("Scene queries did not confirm two entities followed by an empty scene")
    if not any(s["scene_id"] == scene for s in states[-1]["scenes"]):
        raise ValueError("The empty display scene was not preserved")
    mesh = next(e for e in populated["entities"] if e.get("model") == "cube")
    light = next(e["light"] for e in populated["entities"] if "light" in e)
    def equal(actual, expected):
        return len(actual) == len(expected) and all(math.isclose(a, b, abs_tol=1e-5) for a, b in zip(actual, expected))
    if not (equal(mesh["transform"]["position"], [0, 0, 0])
            and equal(mesh["transform"]["rotation_degrees"], [0, 30, 0])
            and equal(light["position"], [-3, 4, 2]) and equal(light["color"], [1, .65, .35])
            and light["type"] == "point_light" and light["render"]
            and light["intensity"] == 250 and light["range"] == 25):
        raise ValueError("Queried model or light values did not match the acceptance prompt")
    if results["undo_operation"][-1]["data"]["undone_operation_id"] != operation:
        raise ValueError("Undo did not target the observed batch")


def verify_editing_events(events):
    calls = []
    resolved = {}
    for event in events:
        item = event.get("item", {})
        if event.get("type") != "item.completed" or item.get("type") != "mcp_tool_call" or item.get("server") != "engine_demo":
            continue
        wire = item.get("result") or {}
        value = wire.get("structured_content") or wire.get("structuredContent")
        if not value:
            texts = [c["text"] for c in wire.get("content", []) if c.get("type") == "text"]
            value = json.loads(texts[0]) if texts else {}
        if item.get("status") != "completed" or item.get("error") or wire.get("isError") or wire.get("is_error"):
            raise ValueError("Editing workflow tool failed")
        key = (value.get("session_id"), value.get("request_id"))
        if value.get("status") == "succeeded" and key[1]:
            resolved[key] = value
        elif value.get("status") not in {"succeeded", "pending", "queued", "running"}:
            raise ValueError("Editing workflow contains a failed or cancelled outcome")
        calls.append((item["tool"], value, wire))
    normalized = []
    for name, value, wire in calls:
        if value.get("status") != "succeeded":
            value = resolved.get((value.get("session_id"), value.get("request_id")))
            if not value:
                raise ValueError("Unresolved editing request")
        normalized.append((name, value, wire))
    names = {n for n, _, _ in normalized}
    required = {"get_editor_status", "list_assets", "import_asset", "create_scene", "apply_operations", "add_physics", "set_physics", "set_transform", "delete_entity", "get_history", "undo_operation", "redo_operation", "get_scene_state", "capture_viewport"}
    if not required <= names:
        raise ValueError("Missing editing workflow tools: " + str(sorted(required - names)))
    kinds = {v["data"].get("type") for n, v, _ in normalized if n == "import_asset"}
    if kinds != {"model", "texture", "music", "soundfx", "shader"}:
        raise ValueError("Five Asset import categories were not verified")
    content = {"import_asset", "create_scene", "apply_operations", "add_physics", "set_physics", "set_transform", "delete_entity"}
    operations = [v["operation_id"] for n, v, _ in normalized if n in content]
    if len(set(operations)) != len(operations):
        raise ValueError("Repeated content requests cannot substitute for a complete workflow")
    undo = [v["data"]["undone_operation_id"] for n, v, _ in normalized if n == "undo_operation"]
    redo = [v["data"]["redone_operation_id"] for n, v, _ in normalized if n == "redo_operation"]
    if undo[:len(operations)] != operations[::-1] or redo[:len(operations)] != operations:
        raise ValueError("The complete ordered undo/redo workflow was not observed")
    snapshots = [v["data"] for n, v, _ in normalized if n == "get_scene_state"]
    if not any(e.get("physics", {}).get("shape") == "sphere" and e["physics"].get("mass") == 3 for state in snapshots for e in state.get("entities", [])):
        raise ValueError("Applied physics configuration was not confirmed")
    if not any(v["data"].get("entity_id_remap") for n, v, _ in normalized if n == "undo_operation"):
        raise ValueError("Deletion restoration identity was not confirmed")
    if not any(v["data"].get("scene_id_remap") for n, v, _ in normalized if n == "redo_operation"):
        raise ValueError("Scene recreation identity was not confirmed")
    pictures = [(v, w) for n, v, w in normalized if n == "capture_viewport"]
    if len(pictures) < 2 or pictures[-1][0]["data"]["frame_id"] <= pictures[0][0]["data"]["frame_id"]:
        raise ValueError("Fresh real editor captures were not confirmed")
    if any(not any(c.get("type") == "image" for c in wire.get("content", [])) for _, wire in pictures):
        raise ValueError("Capture metadata without an image is not render evidence")


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--codex", required=True, help="Absolute path to the native codex executable")
    p.add_argument("--instance", required=True)
    modes = p.add_mutually_exclusive_group()
    modes.add_argument("--visual", action="store_true", help="Verify material, selection and real Scene image workflow")
    modes.add_argument("--editing", action="store_true", help="Verify five imports, physics, deletion and full ordered undo/redo")
    args = p.parse_args()
    root = Path(__file__).resolve().parents[2]
    package = Path(__file__).resolve().parent
    python = package / ".venv/Scripts/python.exe"
    prefix = "qa-codex-editing" if args.editing else "qa-codex-visual" if args.visual else "qa-codex"
    result_log, event_log, error_log = [package / f"{prefix}-{suffix}.log" for suffix in ("result", "events", "stderr")]
    # TOML uses JSON-compatible quoted strings. These are process arguments,
    # never interpolated into a shell command.
    command = [args.codex, "exec", "--ephemeral", "--skip-git-repo-check", "--sandbox", "read-only", "--json",
               "-C", str(root), "-c", "mcp_servers.engine_demo.command=" + json.dumps(str(python).replace("\\", "/"), ensure_ascii=False),
               "-c", "mcp_servers.engine_demo.args=" + json.dumps(["-m", "engine_mcp.server", "--instance", args.instance]),
               "-c", "mcp_servers.engine_demo.cwd=" + json.dumps(str(package).replace("\\", "/"), ensure_ascii=False),
               "-c", "mcp_servers.engine_demo.required=true",
               "-c", "mcp_servers.engine_demo.startup_timeout_sec=20",
               "-o", str(result_log), "-"]
    prompt = (package / ("tests/codex_editing_acceptance.txt" if args.editing else "tests/codex_visual_acceptance.txt" if args.visual else "tests/codex_acceptance.txt")).read_text("utf-8")
    prompt = prompt.replace("__FIXTURES__", str(package / "tests/fixtures/imports").replace("\\", "/"))
    result_log.write_text("", encoding="utf-8")
    with event_log.open("w", encoding="utf-8") as output, error_log.open("w", encoding="utf-8") as errors:
        process = subprocess.Popen(command, text=True, encoding="utf-8", stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=errors,
                                   cwd=root, env={k.upper(): v for k, v in os.environ.items()})
        timed_out = threading.Event()
        def expire():
            timed_out.set()
            process.kill()
        timer = threading.Timer(180, expire)
        timer.start()
        try:
            process.stdin.write(prompt); process.stdin.close()
            for line in process.stdout:
                if line.startswith("{"):
                    output.write(json.dumps(redact_images(json.loads(line)), ensure_ascii=False) + "\n")
                else:
                    output.write("<non-JSON diagnostic omitted>\n")
                output.flush()
            process.wait()
        finally:
            timer.cancel()
        if timed_out.is_set():
            print("Codex acceptance timed out; inspect the QA logs and original write status before retrying.")
            return 124
    print("Codex acceptance exit:", process.returncode)
    if process.returncode:
        return process.returncode
    events = [json.loads(line) for line in event_log.read_text("utf-8").splitlines() if line.startswith("{")]
    try:
        (verify_editing_events if args.editing else verify_visual_events if args.visual else verify_events)(events)
    except (ValueError, KeyError, IndexError, StopIteration, TypeError) as error:
        print("Acceptance not confirmed:", error)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
