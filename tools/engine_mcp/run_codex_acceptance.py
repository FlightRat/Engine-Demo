"""One bounded local Codex/MCP acceptance run; never edits global Codex config."""
import argparse
import json
import math
import os
from pathlib import Path
import subprocess


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


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--codex", required=True, help="Absolute path to the native codex executable")
    p.add_argument("--instance", required=True)
    args = p.parse_args()
    root = Path(__file__).resolve().parents[2]
    package = Path(__file__).resolve().parent
    python = package / ".venv/Scripts/python.exe"
    # TOML uses JSON-compatible quoted strings. These are process arguments,
    # never interpolated into a shell command.
    command = [args.codex, "exec", "--ephemeral", "--skip-git-repo-check", "--sandbox", "read-only", "--json",
               "-C", str(root), "-c", "mcp_servers.engine_demo.command=" + json.dumps(str(python).replace("\\", "/"), ensure_ascii=False),
               "-c", "mcp_servers.engine_demo.args=" + json.dumps(["-m", "engine_mcp.server", "--instance", args.instance]),
               "-c", "mcp_servers.engine_demo.cwd=" + json.dumps(str(package).replace("\\", "/"), ensure_ascii=False),
               "-c", "mcp_servers.engine_demo.required=true",
               "-c", "mcp_servers.engine_demo.startup_timeout_sec=20",
               "-o", str(package / "qa-codex-result.log"), "-"]
    prompt = (package / "tests/codex_acceptance.txt").read_text("utf-8")
    (package / "qa-codex-result.log").write_text("", encoding="utf-8")
    with (package / "qa-codex-events.log").open("w", encoding="utf-8") as output, (package / "qa-codex-stderr.log").open("w", encoding="utf-8") as errors:
        try:
            result = subprocess.run(command, input=prompt, text=True, encoding="utf-8", stdout=output, stderr=errors,
                                    cwd=root, env={k.upper(): v for k, v in os.environ.items()}, timeout=180)
        except subprocess.TimeoutExpired:
            print("Codex acceptance timed out; inspect the QA logs and original write status before retrying.")
            return 124
    print("Codex acceptance exit:", result.returncode)
    if result.returncode:
        return result.returncode
    events = [json.loads(line) for line in (package / "qa-codex-events.log").read_text("utf-8").splitlines() if line.startswith("{")]
    try:
        verify_events(events)
    except (ValueError, KeyError, IndexError, StopIteration, TypeError) as error:
        print("Acceptance not confirmed:", error)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
