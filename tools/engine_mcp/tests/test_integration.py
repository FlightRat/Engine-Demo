import asyncio
import contextlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import time
import uuid
import pytest
from jsonschema import Draft4Validator
from engine_mcp.contract import TOOL_MAP
from engine_mcp.transport import PipeClient, pipe_name

ROOT = Path(__file__).resolve().parents[3]
EXE = ROOT / "x64/Debug/AutomationTests.exe"


@pytest.fixture
def editor():
    if sys.platform != "win32" or not EXE.is_file():
        pytest.skip("Build the native Windows automation tests first")
    instance = "test_" + uuid.uuid4().hex
    process = subprocess.Popen([str(EXE), "--serve", instance], cwd=ROOT, stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE, creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        line = process.stdout.readline()
        assert line.strip() == b"BRIDGE_READY", (line, process.poll())
        yield instance, process
    finally:
        process.terminate()
        process.communicate(timeout=10)


def ensure(result):
    assert result["status"] == "succeeded", result
    return result


def write_params(status, scene=None):
    result = {"session_id": status["session_id"], "request_id": uuid.uuid4().hex, "expected_revision": status["editor_revision"]}
    if scene:
        result["scene_id"] = scene
    return result


def test_native_roundtrip_retry_batch_and_undo(editor):
    async def run():
        client = PipeClient(editor[0])
        try:
            status = ensure(await client.rpc("get_editor_status", {}))
            created = ensure(await client.rpc("create_scene", {**write_params(status), "name": "展示场景"}))
            scene = created["data"]["scene_id"]
            arguments = {**write_params(created, scene), "operations": [
                {"method": "create_object", "params": {"name": "lamp", "kind": "light", "client_ref": "l"}},
                {"method": "set_light", "params": {"target_ref": "l", "color": [1, .6, .3], "position": [-3, 4, 2], "intensity": 10}}]}
            batch = ensure(await client.rpc("apply_operations", arguments))
            Draft4Validator(TOOL_MAP["apply_operations"]["outputSchema"]).validate(batch)
            await client.close()  # A new MCP process/connection must not clear the receipt.
            await asyncio.sleep(.05)
            client = PipeClient(editor[0])
            retry = ensure(await client.rpc("apply_operations", arguments))
            assert retry["operation_id"] == batch["operation_id"]
            state = ensure(await client.rpc("get_scene_state", {}))
            assert state["data"]["total"] == 1
            light = state["data"]["entities"][0]["light"]
            assert light["position"] == [-3, 4, 2]
            clash = await client.rpc("apply_operations", {**arguments, "expected_revision": state["editor_revision"]})
            assert clash["error"]["code"] == "request_id_conflict"
            undo = ensure(await client.rpc("undo_operation", {**write_params(state, scene), "operation_id": batch["operation_id"]}))
            assert undo["data"]["undone_operation_id"] == batch["operation_id"]
            state = ensure(await client.rpc("get_scene_state", {}))
            assert state["data"]["entities"] == []
            old = await client.rpc("set_light", {**write_params(state, scene), "entity_id": batch["data"]["references"]["l"], "intensity": 1})
            assert old["error"]["code"] == "stale_target"
        finally:
            await client.close()
    asyncio.run(run())


def test_busy_and_explicit_instance(editor):
    async def run():
        first, second, absent = PipeClient(editor[0]), PipeClient(editor[0]), PipeClient("missing_" + uuid.uuid4().hex)
        try:
            ensure(await first.rpc("get_editor_status", {}))
            from engine_mcp.contract import BridgeError
            with pytest.raises(BridgeError, match="Cannot connect") as error:
                await second.rpc("get_editor_status", {})
            assert error.value.code == "editor_busy"
            with pytest.raises(BridgeError) as error:
                await absent.rpc("get_editor_status", {})
            assert error.value.code == "editor_unavailable"
        finally:
            await first.close()
            await second.close()
            await absent.close()
    asyncio.run(run())


def test_fragmented_native_frames_and_unknown_version(editor):
    import win32con
    import win32file
    handle = win32file.CreateFile(pipe_name(editor[0]), win32con.GENERIC_READ | win32con.GENERIC_WRITE,
                                  0, None, win32con.OPEN_EXISTING, 0, None)
    def read_exact(n):
        output = bytearray()
        while len(output) < n:
            output.extend(win32file.ReadFile(handle, n - len(output))[1])
        return bytes(output)
    try:
        for version in (1, 999):
            payload = json.dumps({"protocol_version": version, "correlation_id": "q", "method": "hello", "params": {}}).encode()
            frame = struct.pack("<I", len(payload)) + payload
            for offset in range(0, len(frame), 3):
                win32file.WriteFile(handle, frame[offset:offset + 3])
            size = struct.unpack("<I", read_exact(4))[0]
            result = json.loads(read_exact(size))
            assert result["correlation_id"] == "q"
            assert result["status"] == ("succeeded" if version == 1 else "failed")
            if version != 1:
                assert result["error"]["code"] == "unsupported_version"
    finally:
        handle.Close()


def test_mcp_sdk_stdio_with_live_native_editor(editor):
    from mcp import Client
    from mcp.client.stdio import StdioServerParameters
    async def run():
        command = StdioServerParameters(command=sys.executable, args=["-m", "engine_mcp.server", "--instance", editor[0]])
        async with Client(command) as client:
            tools = await client.list_tools()
            assert {t.name for t in tools.tools} == set(TOOL_MAP)
            status = await client.call_tool("get_editor_status", {})
            assert not status.is_error, status
            result = status.structured_content
            ensure(result)
            create = await client.call_tool("create_scene", {**write_params(result), "name": "SDK scene"})
            assert not create.is_error
            invalid = await client.call_tool("get_scene_state", {"unexpected": True})
            assert invalid.is_error
            assert invalid.structured_content["error"]["code"] == "invalid_arguments"
    asyncio.run(run())


def test_mcp_stays_discoverable_without_editor():
    from mcp import Client
    from mcp.client.stdio import StdioServerParameters
    async def run():
        command = StdioServerParameters(command=sys.executable, args=["-m", "engine_mcp.server", "--instance", "missing_" + uuid.uuid4().hex])
        async with Client(command) as client:
            assert len((await client.list_tools()).tools) == 10
            status = await client.call_tool("get_editor_status", {})
            assert status.is_error
            assert status.structured_content["error"]["code"] == "editor_unavailable"
    asyncio.run(run())


def test_editor_restart_changes_session_and_does_not_replay(editor):
    instance, process = editor
    async def run():
        client = PipeClient(instance)
        replacement = None
        try:
            original = ensure(await client.rpc("get_editor_status", {}))
            args = {**write_params(original), "name": "Before restart"}
            ensure(await client.rpc("create_scene", args))
            process.terminate(); process.wait(timeout=10)
            await asyncio.sleep(.05)
            replacement = subprocess.Popen([str(EXE), "--serve", instance], cwd=ROOT, stdout=subprocess.PIPE,
                                           stderr=subprocess.PIPE, creationflags=subprocess.CREATE_NO_WINDOW)
            assert replacement.stdout.readline().strip() == b"BRIDGE_READY"
            current = ensure(await client.rpc("get_editor_status", {}))
            assert current["session_id"] != original["session_id"]
            old = await client.rpc("create_scene", args)
            assert old["error"]["code"] == "stale_session"
            empty = ensure(await client.rpc("get_scene_state", {}))
            assert empty["data"]["scenes"] == []
        finally:
            await client.close()
            if replacement:
                replacement.terminate(); replacement.communicate(timeout=10)
    asyncio.run(run())


def test_pending_result_can_be_queried_and_queued_request_cancelled():
    if not EXE.is_file():
        pytest.skip("Build native tests first")
    instance = "slow_" + uuid.uuid4().hex
    process = subprocess.Popen([str(EXE), "--serve", instance], cwd=ROOT, stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE, creationflags=subprocess.CREATE_NO_WINDOW)
    async def run():
        client = PipeClient(instance)
        try:
            state = ensure(await client.rpc("get_editor_status", {}))
            scene_result = ensure(await client.rpc("create_scene", {**write_params(state), "name": "Slow"}))
            scene = scene_result["data"]["scene_id"]
            args = {**write_params(scene_result, scene), "name": "slow lamp", "kind": "light"}
            client.wait_seconds = .05
            pending = await client.rpc("create_object", args)
            assert pending["status"] == "pending"
            client.wait_seconds = 2
            queued_args = {**write_params(scene_result, scene), "name": "cancel me", "kind": "light"}
            second = asyncio.create_task(client.rpc("create_object", queued_args))
            await asyncio.sleep(.01)
            cancelled = await client.rpc("cancel", {"session_id": queued_args["session_id"], "request_id": queued_args["request_id"]})
            assert cancelled["status"] == "cancelled"
            await second
            for _ in range(50):
                result = await client.rpc("get_operation_status", {"session_id": args["session_id"], "request_id": args["request_id"]})
                if result["status"] == "succeeded":
                    break
                await asyncio.sleep(.02)
            ensure(result)
            state = ensure(await client.rpc("get_scene_state", {}))
            assert state["data"]["total"] == 1
        finally:
            await client.close()
    try:
        assert process.stdout.readline().strip() == b"BRIDGE_READY"
        asyncio.run(run())
    finally:
        process.terminate(); process.communicate(timeout=10)


def test_idle_pipe_shutdown_does_not_hang():
    if not EXE.is_file():
        pytest.skip("Build native tests first")
    instance = "exit_" + uuid.uuid4().hex
    process = subprocess.Popen([str(EXE), "--serve", instance, "1"], cwd=ROOT, stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE, creationflags=subprocess.CREATE_NO_WINDOW)
    async def run():
        client = PipeClient(instance)
        try:
            ensure(await client.rpc("get_editor_status", {}))
            await asyncio.to_thread(process.wait, 5)
            assert process.returncode == 0
        finally:
            await client.close()
    try:
        assert process.stdout.readline().strip() == b"BRIDGE_READY"
        asyncio.run(run())
    finally:
        if process.poll() is None:
            process.terminate()
        process.communicate(timeout=10)


def test_config_paths_and_sdk_launch_from_space_directory(tmp_path):
    import tomllib
    from mcp import Client
    from mcp.client.stdio import StdioServerParameters
    config = tomllib.loads((ROOT / "tools/engine_mcp/codex-config.example.toml").read_text("utf-8"))["mcp_servers"]["engine_demo"]
    assert Path(config["command"]).is_file()
    cwd = tmp_path / "测试 with spaces"
    cwd.mkdir()
    async def run():
        async with Client(StdioServerParameters(command=config["command"], args=config["args"], cwd=str(cwd))) as client:
            assert len((await client.list_tools()).tools) == 10
    asyncio.run(run())


def raw_pipe(instance):
    import win32con
    import win32file
    return win32file.CreateFile(pipe_name(instance), win32con.GENERIC_READ | win32con.GENERIC_WRITE,
                                0, None, win32con.OPEN_EXISTING, 0, None)


def read_frame(handle):
    import win32file
    def exact(length):
        output = bytearray()
        while len(output) < length:
            output.extend(win32file.ReadFile(handle, length - len(output))[1])
        return output
    length = struct.unpack("<I", exact(4))[0]
    assert 0 < length <= 1024 * 1024
    return json.loads(exact(length))


@pytest.mark.parametrize("payload", [b'{', b'{"bad":"\xff"}', b'{"protocol_version":1,"protocol_version":1}'])
def test_malformed_json_does_not_mutate_and_connection_survives(editor, payload):
    import win32file
    handle = raw_pipe(editor[0])
    try:
        win32file.WriteFile(handle, struct.pack("<I", len(payload)) + payload)
        assert read_frame(handle)["error"]["code"] == "invalid_arguments"
        request = json.dumps({"protocol_version": 1, "correlation_id": "after-bad", "method": "get_scene_state", "params": {}}).encode()
        win32file.WriteFile(handle, struct.pack("<I", len(request)) + request)
        state = ensure(read_frame(handle))
        assert state["correlation_id"] == "after-bad"
        assert state["data"]["scenes"] == []
        assert state["editor_revision"] == 0
    finally:
        handle.Close()


@pytest.mark.parametrize("length", [0, 1024 * 1024 + 1, 0xffffffff])
def test_invalid_length_disconnects_and_allows_reconnect(editor, length):
    import pywintypes
    import win32file
    handle = raw_pipe(editor[0])
    try:
        win32file.WriteFile(handle, struct.pack("<I", length))
        with pytest.raises(pywintypes.error):
            win32file.ReadFile(handle, 4)
    finally:
        handle.Close()
    async def run():
        client = PipeClient(editor[0])
        try:
            assert ensure(await client.rpc("get_editor_status", {}))["editor_revision"] == 0
        finally:
            await client.close()
    asyncio.run(run())


def test_exact_message_limit_is_read_and_rejected_semantically(editor):
    import win32file
    request = {"protocol_version": 1, "correlation_id": "limit", "method": "hello", "params": {}, "padding": ""}
    payload = json.dumps(request).encode()
    request["padding"] = " " * (1024 * 1024 - len(payload))
    payload = json.dumps(request).encode()
    assert len(payload) == 1024 * 1024
    handle = raw_pipe(editor[0])
    try:
        win32file.WriteFile(handle, struct.pack("<I", len(payload)) + payload)
        result = read_frame(handle)
        assert result["correlation_id"] == "limit"
        assert result["error"]["code"] == "invalid_arguments"
    finally:
        handle.Close()


def test_simultaneous_calls_keep_response_correlation(editor):
    async def run():
        client = PipeClient(editor[0])
        try:
            methods = ["get_editor_status", "list_assets", "get_scene_state"] * 8
            results = await asyncio.gather(*(client.rpc(method, {}) for method in methods))
            assert len({r["correlation_id"] for r in results}) == len(results)
            for method, result in zip(methods, results):
                ensure(result)
                field = {"get_editor_status": "instance", "list_assets": "assets", "get_scene_state": "scenes"}[method]
                assert field in result["data"]
        finally:
            await client.close()
    asyncio.run(run())


def test_conflicting_instance_and_two_distinct_instances(editor):
    duplicate = subprocess.run([str(EXE), "--serve", editor[0]], cwd=ROOT, capture_output=True,
                               timeout=5, creationflags=subprocess.CREATE_NO_WINDOW)
    assert duplicate.returncode != 0
    assert b"instance may already be in use" in duplicate.stderr
    second_instance = "other_" + uuid.uuid4().hex
    second = subprocess.Popen([str(EXE), "--serve", second_instance], cwd=ROOT, stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE, creationflags=subprocess.CREATE_NO_WINDOW)
    async def run():
        first_client, second_client = PipeClient(editor[0]), PipeClient(second_instance)
        try:
            first = ensure(await first_client.rpc("get_editor_status", {}))
            other = ensure(await second_client.rpc("get_editor_status", {}))
            assert first["session_id"] != other["session_id"]
            assert first["data"]["instance"] == editor[0]
            assert other["data"]["instance"] == second_instance
            ensure(await first_client.rpc("create_scene", {**write_params(first), "name": "First only"}))
            isolated = ensure(await second_client.rpc("get_scene_state", {}))
            assert isolated["data"]["scenes"] == []
        finally:
            await first_client.close()
            await second_client.close()
    try:
        assert second.stdout.readline().strip() == b"BRIDGE_READY"
        asyncio.run(run())
    finally:
        second.terminate(); second.communicate(timeout=10)


def test_shutdown_with_client_not_reading_and_queued_requests():
    if not EXE.is_file():
        pytest.skip("Build native tests first")
    instance = "exit_" + uuid.uuid4().hex
    process = subprocess.Popen([str(EXE), "--serve", instance, "1"], cwd=ROOT, stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE, creationflags=subprocess.CREATE_NO_WINDOW)
    handle = None
    try:
        assert process.stdout.readline().strip() == b"BRIDGE_READY"
        handle = raw_pipe(instance)
        import win32file
        # Leave many large correlated responses unread, filling the OS output buffer.
        requests = []
        for index in range(160):
            payload = json.dumps({"protocol_version": 1, "correlation_id": str(index) + "x" * 240,
                                  "method": "get_editor_status", "params": {}}).encode()
            requests.append(struct.pack("<I", len(payload)) + payload)
        # Under 64 KiB: writing cannot depend on this client reading responses.
        assert sum(map(len, requests)) < 65536
        win32file.WriteFile(handle, b"".join(requests))
        process.wait(timeout=5)
        assert process.returncode == 0
    finally:
        if handle:
            handle.Close()
        if process.poll() is None:
            process.terminate()
        process.communicate(timeout=10)


def test_same_client_connects_when_editor_comes_online_later():
    if not EXE.is_file():
        pytest.skip("Build native tests first")
    from engine_mcp.contract import BridgeError
    instance = "late_" + uuid.uuid4().hex
    async def run():
        client = PipeClient(instance)
        process = None
        try:
            with pytest.raises(BridgeError) as error:
                await client.rpc("get_editor_status", {})
            assert error.value.code == "editor_unavailable"
            process = subprocess.Popen([str(EXE), "--serve", instance], cwd=ROOT, stdout=subprocess.PIPE,
                                       stderr=subprocess.PIPE, creationflags=subprocess.CREATE_NO_WINDOW)
            assert process.stdout.readline().strip() == b"BRIDGE_READY"
            state = ensure(await client.rpc("get_editor_status", {}))
            assert state["data"]["instance"] == instance
            assert state["editor_revision"] == 0
        finally:
            await client.close()
            if process:
                process.terminate(); process.communicate(timeout=10)
    asyncio.run(run())


def test_legacy_mcp_protocol_exposes_camel_case_tools():
    # Codex may negotiate an older MCP version than the installed SDK client.
    requests = [
        {"jsonrpc": "2.0", "id": 1, "method": "initialize", "params": {
            "protocolVersion": "2025-06-18", "capabilities": {}, "clientInfo": {"name": "compat-test", "version": "1"}}},
        {"jsonrpc": "2.0", "method": "notifications/initialized"},
        {"jsonrpc": "2.0", "id": 2, "method": "tools/list", "params": {}},
    ]
    process = subprocess.run([sys.executable, "-m", "engine_mcp.server", "--instance", "missing_compat"],
                             input="\n".join(json.dumps(r) for r in requests) + "\n", text=True, encoding="utf-8",
                             capture_output=True, timeout=15, creationflags=subprocess.CREATE_NO_WINDOW)
    assert process.returncode == 0, process.stderr
    responses = {r["id"]: r for r in map(json.loads, process.stdout.splitlines()) if "id" in r}
    assert responses[1]["result"]["protocolVersion"] == "2025-06-18"
    tools = responses[2]["result"]["tools"]
    assert {t["name"] for t in tools} == set(TOOL_MAP)
    assert all("inputSchema" in t and "outputSchema" in t for t in tools)
