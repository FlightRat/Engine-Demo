import asyncio
import base64
import copy
import json
import os
import struct
import zlib

import pytest
from jsonschema import Draft4Validator
from engine_mcp.contract import BridgeError, TOOL_MAP
from engine_mcp.image import encode_capture


def capture(width=64, height=64, pixels=None):
    return {"status": "succeeded", "request_id": "", "session_id": "s", "editor_revision": 2,
            "data": {"scene_id": "scene", "frame_id": 1, "width": width, "height": height,
                     "camera": {"position": [0, 1, 2], "front": [0, 0, -1], "up": [0, 1, 0], "fov_degrees": 43, "near": .1, "far": 200},
                     "encoding": "rgb8", "origin": "bottom_left",
                     "pixels_base64": base64.b64encode(pixels if pixels is not None else bytes(width * height * 3)).decode()}}


def decode_png(encoded):
    png = base64.b64decode(encoded)
    assert png[:8] == b"\x89PNG\r\n\x1a\n"
    pos, compressed, dims = 8, b"", None
    while pos < len(png):
        length = struct.unpack_from(">I", png, pos)[0]
        kind, payload = png[pos + 4:pos + 8], png[pos + 8:pos + 8 + length]
        assert zlib.crc32(kind + payload) == struct.unpack_from(">I", png, pos + 8 + length)[0]
        if kind == b"IHDR":
            w, h, depth, color, compression, filter_method, interlace = struct.unpack(">IIBBBBB", payload)
            assert (depth, color, compression, filter_method, interlace) == (8, 2, 0, 0, 0)
            dims = w, h
        elif kind == b"IDAT":
            compressed += payload
        elif kind == b"IEND":
            assert pos + 12 == len(png)
        pos += 12 + length
    rows = zlib.decompress(compressed)
    w, h = dims
    assert len(rows) == (w * 3 + 1) * h
    assert all(rows[i * (w * 3 + 1)] == 0 for i in range(h))
    return dims, b"".join(rows[i * (w * 3 + 1) + 1:(i + 1) * (w * 3 + 1)] for i in range(h))


def test_capture_png_orientation_and_public_result():
    bottom = b"\xff\0\0" * (64 * 32)
    top = b"\0\0\xff" * (64 * 32)
    original = capture(pixels=bottom + top)
    frozen = copy.deepcopy(original)
    public, png = encode_capture(original)
    assert original == frozen
    dims, pixels = decode_png(png)
    assert dims == (64, 64) and pixels == top + bottom
    assert "pixels_base64" not in json.dumps(public) and "encoding" not in public["data"]
    Draft4Validator(TOOL_MAP["capture_viewport"]["bridgeOutputSchema"]).validate(original)
    Draft4Validator(TOOL_MAP["capture_viewport"]["outputSchema"]).validate(public)


def test_random_maximum_capture_is_bounded():
    original = capture(640, 360, os.urandom(640 * 360 * 3))
    public, png = encode_capture(original)
    assert len(base64.b64decode(png)) <= 700 * 1024
    assert len(json.dumps(original).encode()) < 1024 * 1024
    assert decode_png(png)[0] == (640, 360)


@pytest.mark.parametrize("mutation", [
    {"width": 1025}, {"width": True}, {"height": 0}, {"width": 1024, "height": 1024},
    {"encoding": "rgba8"}, {"origin": "unknown"}, {"pixels_base64": "invalid!"}, {"pixels_base64": "YQ=="},
])
def test_invalid_image_never_becomes_png(mutation):
    original = capture(); original["data"].update(mutation)
    with pytest.raises(BridgeError):
        encode_capture(original)


def test_server_image_content_and_busy():
    from mcp import types
    from mcp.server.lowlevel import Server
    from engine_mcp.server import create_server
    # Exercise the registered callback through the SDK's actual Server object.
    class Client:
        async def rpc(self, name, args):
            await asyncio.sleep(.03)
            return capture()

    async def run():
        server = create_server(Client())
        assert isinstance(server, Server)
        request = types.CallToolRequestParams(name="capture_viewport", arguments={"session_id": "s", "scene_id": "scene", "expected_revision": 2})
        callback = server.get_request_handler("tools/call").handler
        first = asyncio.create_task(callback(None, request))
        await asyncio.sleep(.005)
        second = await callback(None, request)
        result = await first
        assert second.is_error and second.structured_content["error"]["code"] == "capture_busy"
        assert not result.is_error
        images = [item for item in result.content if item.type == "image"]
        assert len(images) == 1 and images[0].mime_type == "image/png"
        assert decode_png(images[0].data)[0] == (64, 64)
        assert "pixels_base64" not in result.content[0].text
        assert "pixels_base64" not in json.dumps(result.structured_content)
    asyncio.run(run())


def test_server_rejects_corrupt_pixels_without_leaking_image(caplog):
    from mcp import types
    from engine_mcp.server import create_server
    class Client:
        async def rpc(self, name, args):
            bad = capture(); bad["data"]["pixels_base64"] = "SECRET_CORRUPT_PIXELS"
            return bad
    async def run():
        server = create_server(Client())
        request = types.CallToolRequestParams(name="capture_viewport", arguments={"session_id": "s", "scene_id": "scene", "expected_revision": 2})
        result = await server.get_request_handler("tools/call").handler(None, request)
        assert result.is_error and result.structured_content["error"]["code"] == "invalid_response"
        assert not any(item.type == "image" for item in result.content)
        assert "SECRET_CORRUPT_PIXELS" not in result.content[0].text + caplog.text
    asyncio.run(run())


def test_cancelled_capture_releases_server_slot():
    from mcp import types
    from engine_mcp.server import create_server
    class Client:
        slow = True
        async def rpc(self, name, args):
            if self.slow:
                await asyncio.sleep(10)
            return capture()
    async def run():
        client = Client(); server = create_server(client)
        callback = server.get_request_handler("tools/call").handler
        request = types.CallToolRequestParams(name="capture_viewport", arguments={"session_id": "s", "scene_id": "scene", "expected_revision": 2})
        task = asyncio.create_task(callback(None, request)); await asyncio.sleep(.005); task.cancel()
        with pytest.raises(asyncio.CancelledError):
            await task
        client.slow = False
        result = await callback(None, request)
        assert not result.is_error and any(c.type == "image" for c in result.content)
    asyncio.run(run())
