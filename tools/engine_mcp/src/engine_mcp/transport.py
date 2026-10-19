from __future__ import annotations

import asyncio
import contextlib
import json
import re
import struct
import sys
import uuid

from .contract import BridgeError, MAX_MESSAGE, WRITES, decode, failure


def pipe_name(instance: str) -> str:
    if not re.fullmatch(r"[A-Za-z0-9_-]{1,64}", instance):
        raise ValueError("Instance must be 1..64 ASCII letters, digits, _ or -")
    if sys.platform != "win32":
        raise BridgeError("editor_unavailable", "Engine bridge currently requires Windows")
    import win32api
    import win32con
    import win32security
    token = win32security.OpenProcessToken(win32api.GetCurrentProcess(), win32con.TOKEN_QUERY)
    try:
        sid = win32security.ConvertSidToStringSid(win32security.GetTokenInformation(token, win32security.TokenUser)[0])
    finally:
        token.Close()
    return rf"\\.\pipe\EngineDemo.{sid}.{instance}"


class PipeClient:
    def __init__(self, instance: str, wait_seconds: float = 12):
        self.instance = instance
        self.wait_seconds = wait_seconds
        self._handle = None
        self._reader = None
        self._pending = {}
        self._connect_lock = asyncio.Lock()
        self._write_lock = asyncio.Lock()

    async def _connect(self):
        async with self._connect_lock:
            if self._handle is not None:
                return self._handle
            import pywintypes
            import win32con
            import win32file
            try:
                handle = win32file.CreateFile(pipe_name(self.instance), win32con.GENERIC_READ | win32con.GENERIC_WRITE,
                                             0, None, win32con.OPEN_EXISTING, win32con.FILE_FLAG_OVERLAPPED, None)
            except pywintypes.error as error:
                code = "editor_busy" if error.winerror == 231 else "editor_unavailable"
                raise BridgeError(code, f"Cannot connect to editor instance {self.instance}: {error.strerror}. Editor and MCP executor must use the same Windows user, including sandbox identity.", retryable=True) from error
            self._handle = handle
            self._reader = asyncio.create_task(self._receive(handle))
            try:
                hello = await self._exchange(handle, "hello", {})
                if hello.get("status") != "succeeded" or hello.get("data", {}).get("protocol_version") != 1:
                    raise BridgeError("unsupported_version", "Editor did not confirm bridge protocol version 1")
                if hello.get("data", {}).get("instance") != self.instance:
                    raise BridgeError("editor_unavailable", "Editor handshake returned a different instance")
            except BaseException:
                await self.close()
                raise
            return handle

    async def _io(self, handle, value, *, write=False):
        import pywintypes
        import win32event
        import win32file
        over = pywintypes.OVERLAPPED()
        over.hEvent = win32event.CreateEvent(None, True, False, None)
        try:
            error, data = win32file.WriteFile(handle, value, over) if write else win32file.ReadFile(handle, value, over)
            if error not in (0, 997):
                raise OSError(f"Pipe IO error {error}")
            while win32event.WaitForSingleObject(over.hEvent, 0) == win32event.WAIT_TIMEOUT:
                await asyncio.sleep(.002)
            count = win32file.GetOverlappedResult(handle, over, False)
            if count == 0:
                raise OSError("Pipe closed")
            return count if write else bytes(data[:count])
        except BaseException:
            with contextlib.suppress(Exception):
                win32file.CancelIoEx(handle, over)
                win32file.GetOverlappedResult(handle, over, True)
            raise
        finally:
            over.hEvent.Close()

    async def _read(self, handle, length):
        chunks = bytearray()
        while len(chunks) < length:
            chunks.extend(await self._io(handle, min(length - len(chunks), 65536)))
        return bytes(chunks)

    async def _receive(self, handle):
        try:
            while True:
                size = struct.unpack("<I", await self._read(handle, 4))[0]
                if not 0 < size <= MAX_MESSAGE:
                    raise BridgeError("invalid_message", "Response exceeds bridge size limit")
                result = decode(await self._read(handle, size))
                future = self._pending.pop(result.get("correlation_id"), None)
                if future is not None and not future.done():
                    future.set_result(result)
        except asyncio.CancelledError:
            raise
        except Exception as error:
            if self._handle is handle:
                self._handle = None
                for future in self._pending.values():
                    if not future.done():
                        future.set_exception(BridgeError("editor_unavailable", f"Editor disconnected: {error}. Query original request status; do not replay as a new write.", retryable=True))
                self._pending.clear()
        finally:
            with contextlib.suppress(Exception):
                handle.Close()

    async def rpc(self, method: str, arguments: dict):
        if sys.platform != "win32":
            raise BridgeError("editor_unavailable", "Engine bridge currently requires Windows")
        handle = await self._connect()
        return await self._exchange(handle, method, arguments)

    async def _exchange(self, handle, method: str, arguments: dict):
        if len(self._pending) >= 128:
            raise BridgeError("queue_full", "Too many outstanding bridge calls", retryable=True)
        correlation = uuid.uuid4().hex
        payload = json.dumps({"protocol_version": 1, "correlation_id": correlation,
                              "method": method, "params": arguments}, ensure_ascii=False, allow_nan=False).encode("utf-8")
        if len(payload) > MAX_MESSAGE:
            raise BridgeError("invalid_arguments", "Request exceeds bridge size limit")
        future = asyncio.get_running_loop().create_future()
        # A timed-out caller can stop awaiting this future; consume late exceptions.
        future.add_done_callback(lambda f: f.exception() if not f.cancelled() else None)
        self._pending[correlation] = future
        try:
            async with self._write_lock:
                frame = struct.pack("<I", len(payload)) + payload
                offset = 0
                while offset < len(frame):
                    offset += await self._io(handle, frame[offset:offset + 65536], write=True)
            return await asyncio.wait_for(asyncio.shield(future), timeout=self.wait_seconds)
        except asyncio.TimeoutError:
            if method in WRITES:
                return {"status": "pending", "session_id": arguments["session_id"], "request_id": arguments["request_id"],
                        "data": {"next_tool": "get_operation_status", "message": "Execution outcome is not yet known"}}
            raise BridgeError("editor_unavailable", "Editor response timed out", retryable=True)
        except asyncio.CancelledError:
            if method in WRITES:
                import anyio
                with anyio.CancelScope(shield=True):
                    with contextlib.suppress(Exception):
                        await asyncio.wait_for(self.rpc("cancel", {"session_id": arguments["session_id"], "request_id": arguments["request_id"]}), .5)
            raise
        except Exception as error:
            self._pending.pop(correlation, None)
            if isinstance(error, BridgeError):
                raise
            raise BridgeError("editor_unavailable", f"Pipe operation failed: {error}; query the original request status", retryable=True) from error

    async def close(self):
        if self._reader is not None:
            self._reader.cancel()
            with contextlib.suppress(asyncio.CancelledError):
                await self._reader
            self._reader = None
        self._handle = None
        for future in self._pending.values():
            if not future.done():
                future.cancel()
        self._pending.clear()
