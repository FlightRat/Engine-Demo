from __future__ import annotations

import json
import math
import struct
import copy
from importlib.resources import files
from jsonschema import Draft4Validator

MAX_MESSAGE = 1024 * 1024
TOOLS = json.loads(files("engine_mcp").joinpath("contracts/tools.json").read_text(encoding="utf-8"))["tools"]
TOOL_MAP = {tool["name"]: tool for tool in TOOLS}
WRITES = {tool["name"] for tool in TOOLS if not tool.get("annotations", {}).get("readOnlyHint", False)}


def public_tool(tool):
    """Publish 2020-12 bounds, preserving the RapidJSON Draft-4 contract semantics."""
    def modern(value):
        if isinstance(value, list):
            return [modern(x) for x in value]
        if not isinstance(value, dict):
            return value
        result = {key: modern(item) for key, item in value.items()}
        if result.get("exclusiveMinimum") is True:
            result["exclusiveMinimum"] = result.pop("minimum")
        elif result.get("exclusiveMinimum") is False:
            del result["exclusiveMinimum"]
        return result
    result = copy.deepcopy(tool)
    result.pop("bridgeOutputSchema", None)
    for key in ("inputSchema", "outputSchema"):
        result[key] = modern(result[key])
        result[key]["$schema"] = "https://json-schema.org/draft/2020-12/schema"
    return result


class BridgeError(Exception):
    def __init__(self, code: str, message: str, *, retryable=False):
        super().__init__(message)
        self.code = code
        self.retryable = retryable


def failure(error: BridgeError, arguments=None):
    arguments = arguments or {}
    return {"status": "failed", "session_id": arguments.get("session_id"), "request_id": arguments.get("request_id", ""),
            "error": {"code": error.code, "message": str(error), "details": {}, "retryable": error.retryable}}


def finite(value):
    if isinstance(value, (int, float)) and not isinstance(value, bool):
        return abs(value) <= 3.4028234e38 and math.isfinite(value) and (value == 0 or struct.unpack("f", struct.pack("f", value))[0] != 0)
    if isinstance(value, dict):
        return all(finite(x) for x in value.values())
    if isinstance(value, list):
        return all(finite(x) for x in value)
    return True


def validate(name: str, arguments: dict):
    tool = TOOL_MAP.get(name)
    if tool is None:
        raise BridgeError("unknown_method", f"Unknown tool: {name}")
    if not finite(arguments):
        raise BridgeError("invalid_arguments", "Expected finite float-compatible values")
    errors = list(Draft4Validator(tool["inputSchema"]).iter_errors(arguments))
    if errors:
        raise BridgeError("invalid_arguments", errors[0].message)
    if name == "apply_operations":
        refs = set()
        for action in arguments["operations"]:
            params = action["params"]
            if "target_ref" in params and params["target_ref"] not in refs:
                raise BridgeError("invalid_arguments", "target_ref must reference an earlier creation")
            if "client_ref" in params:
                if params["client_ref"] in refs:
                    raise BridgeError("invalid_arguments", "Duplicate client_ref")
                refs.add(params["client_ref"])


def decode(payload: bytes):
    def unique(pairs):
        result = {}
        for key, value in pairs:
            if key in result:
                raise ValueError("Duplicate JSON field")
            result[key] = value
        return result
    try:
        result = json.loads(payload.decode("utf-8"), object_pairs_hook=unique,
                            parse_constant=lambda s: (_ for _ in ()).throw(ValueError(s)))
        if not isinstance(result, dict) or not finite(result):
            raise ValueError("Expected a finite JSON object")
        return result
    except (ValueError, UnicodeError) as error:
        raise BridgeError("invalid_message", str(error)) from error
