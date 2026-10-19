import json
from pathlib import Path
import runpy
import pytest
from jsonschema import Draft4Validator, Draft202012Validator
from engine_mcp.contract import TOOLS, BridgeError, decode, validate, public_tool

ROOT = Path(__file__).resolve().parents[1]
BASE = {"session_id": "s", "request_id": "r", "expected_revision": 0, "scene_id": "scene", "entity_id": "entity"}


def test_generated_contract_is_current():
    expected = runpy.run_path(str(ROOT / "generate_contract.py"))["build"]()
    assert json.loads((ROOT / "contracts/tools.json").read_text("utf-8")) == expected
    assert json.loads((ROOT / "src/engine_mcp/contracts/tools.json").read_text("utf-8")) == expected
    assert len(TOOLS) == 10
    for tool in TOOLS:
        Draft4Validator.check_schema(tool["inputSchema"])
        Draft4Validator.check_schema(tool["outputSchema"])
        wire = public_tool(tool)
        Draft202012Validator.check_schema(wire["inputSchema"])
        Draft202012Validator.check_schema(wire["outputSchema"])


@pytest.mark.parametrize("method,args", [
    ("get_editor_status", {"unexpected": True}),
    ("get_scene_state", {"page_size": 201}),
    ("set_transform", {**BASE, "scale": [1, 0, 1]}),
    ("set_transform", {**BASE, "space": "world", "position": [0, 0, 0]}),
    ("set_transform", {**BASE, "position": [float("nan"), 0, 0]}),
    ("set_transform", {**BASE, "position": [1e100, 0, 0]}),
    ("set_transform", {**BASE, "scale": [1e-100, 1, 1]}),
    ("set_light", {**BASE, "intensity": 10**1000}),
    ("set_light", {**BASE, "intensity": 10001}),
    ("create_scene", {"name": "missing-preconditions"}),
])
def test_invalid_arguments(method, args):
    with pytest.raises(BridgeError):
        validate(method, args)


def test_batch_reference_validation():
    common = {k: v for k, v in BASE.items() if k != "entity_id"}
    bad = {**common, "operations": [{"method": "set_light", "params": {"target_ref": "future", "intensity": 1}}]}
    with pytest.raises(BridgeError):
        validate("apply_operations", bad)
    good = {**common, "operations": [{"method": "create_object", "params": {"name": "lamp", "kind": "light", "client_ref": "l"}},
                                    {"method": "set_light", "params": {"target_ref": "l", "intensity": 1}}]}
    validate("apply_operations", good)


def test_wire_and_native_bounds_are_equivalent():
    tool = next(t for t in TOOLS if t["name"] == "set_transform")
    old = Draft4Validator(tool["inputSchema"])
    new = Draft202012Validator(public_tool(tool)["inputSchema"])
    for scale in ([-1, 1, 1], [0, 1, 1], [.01, 1, 1], [1, 2, 3]):
        params = {**BASE, "scale": scale}
        assert old.is_valid(params) == new.is_valid(params)


@pytest.mark.parametrize("payload", [b'{"a":1,"a":2}', b'{"a":NaN}', b'{"a":"\xff"}', b'[]'])
def test_invalid_wire_json(payload):
    with pytest.raises(BridgeError):
        decode(payload)
