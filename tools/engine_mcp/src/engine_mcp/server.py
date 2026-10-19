from __future__ import annotations

import argparse
import json
import logging
import anyio
from jsonschema import Draft4Validator
from mcp.server.lowlevel import Server
from mcp.server.stdio import stdio_server
from mcp import types

from .contract import TOOLS, TOOL_MAP, BridgeError, failure, validate, public_tool
from .transport import PipeClient


def create_server(client: PipeClient):
    async def list_tools(context, params):
        return types.ListToolsResult(tools=[types.Tool.model_validate(public_tool(tool)) for tool in TOOLS])

    async def call_tool(context, params):
        arguments = params.arguments or {}
        try:
            validate(params.name, arguments)
            result = await client.rpc(params.name, arguments)
            Draft4Validator(TOOL_MAP[params.name]["outputSchema"]).validate(result)
        except BridgeError as error:
            logging.getLogger(__name__).warning("%s: %s", error.code, error)
            result = failure(error, arguments)
        return types.CallToolResult(content=[types.TextContent(type="text", text=json.dumps(result, ensure_ascii=False, allow_nan=False))],
                                    structured_content=result, is_error=result.get("status") == "failed")

    # Validation uses the exact Draft-4 schema shared with RapidJSON, rather than
    # interpreting exclusiveMinimum under a different JSON Schema draft.
    return Server("engine_demo", version="0.1.0", on_list_tools=list_tools, on_call_tool=call_tool,
                  instructions="Operate the explicitly configured local editor. Query state/revision first; reuse request_id only for identical retries. Use batches for one undoable edit. Never execute arbitrary scripts to work around rejected tools.")


async def serve(instance: str, wait_seconds: float):
    client = PipeClient(instance, wait_seconds)
    server = create_server(client)
    try:
        async with stdio_server() as (read, write):
            await server.run(read, write, server.create_initialization_options())
    finally:
        await client.close()


def main():
    import sys
    # Codex expects UTF-8 stderr even when Windows starts us under a legacy code page.
    if hasattr(sys.stderr, "reconfigure"):
        sys.stderr.reconfigure(encoding="utf-8", errors="backslashreplace")
    parser = argparse.ArgumentParser(description="Local MCP tools for Engine-Demo")
    parser.add_argument("--instance", required=True)
    parser.add_argument("--wait-seconds", type=float, default=12)
    args = parser.parse_args()
    import re
    if not re.fullmatch(r"[A-Za-z0-9_-]{1,64}", args.instance):
        parser.error("invalid instance name")
    if not 0 < args.wait_seconds <= 120:
        parser.error("wait-seconds must be in (0,120]")
    logging.basicConfig(level=logging.WARNING)
    anyio.run(serve, args.instance, args.wait_seconds)


if __name__ == "__main__":
    main()
