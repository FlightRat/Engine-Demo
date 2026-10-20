"""Bounded bridge RGB8 to PNG conversion. No file or GPU access."""
from __future__ import annotations

import base64
import binascii
import copy
import struct
import zlib

from .contract import BridgeError, MAX_MESSAGE

MAX_PIXELS = 230400
MAX_PNG = 700 * 1024


def encode_capture(result: dict) -> tuple[dict, str]:
    data = result["data"]
    width, height = data["width"], data["height"]
    if (type(width) is not int or type(height) is not int or not 0 < width <= 1024 or
            not 0 < height <= 1024 or width * height > MAX_PIXELS or
            data["encoding"] != "rgb8" or data["origin"] != "bottom_left"):
        raise BridgeError("invalid_response", "Invalid capture pixel format or dimensions")
    encoded = data["pixels_base64"]
    if len(encoded) > 921600:
        raise BridgeError("capture_too_large", "Capture exceeds pixel budget")
    try:
        pixels = base64.b64decode(encoded, validate=True)
    except (ValueError, binascii.Error):
        raise BridgeError("invalid_response", "Invalid capture pixel encoding") from None
    if len(pixels) != width * height * 3:
        raise BridgeError("invalid_response", "Capture pixel length does not match dimensions")

    stride = width * 3
    scanlines = b"".join(b"\0" + pixels[row * stride:(row + 1) * stride] for row in range(height - 1, -1, -1))

    def chunk(kind: bytes, payload: bytes) -> bytes:
        return struct.pack(">I", len(payload)) + kind + payload + struct.pack(">I", zlib.crc32(kind + payload))

    png = (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)) +
           chunk(b"IDAT", zlib.compress(scanlines)) + chunk(b"IEND", b""))
    if len(png) > MAX_PNG:
        raise BridgeError("capture_too_large", "PNG exceeds capture output budget")
    public = {key: copy.deepcopy(value) for key, value in result.items() if key != "data"}
    public["data"] = {key: copy.deepcopy(value) for key, value in data.items()
                      if key not in ("pixels_base64", "encoding", "origin")}
    import json
    if len(json.dumps(public, ensure_ascii=False).encode("utf-8")) > 32768:
        raise BridgeError("capture_too_large", "Capture metadata exceeds budget")
    png_data = base64.b64encode(png).decode("ascii")
    if len(png_data) + 32768 > MAX_MESSAGE:
        raise BridgeError("capture_too_large", "Capture response exceeds message budget")
    return public, png_data
