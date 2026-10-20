from pathlib import Path
import runpy
import pytest

RUNNER = runpy.run_path(str(Path(__file__).resolve().parents[1] / "run_codex_acceptance.py"))


def test_acceptance_redacts_images_before_logging():
    original = {"content": [{"type": "image", "data": "SECRETPIXELS", "mimeType": "image/png"}],
                "metadata": {"pixels_base64": "RAWPIXELS"}, "uri": "data:image/png;base64,OTHERPIXELS"}
    redacted = RUNNER["redact_images"](original)
    assert "SECRETPIXELS" not in str(redacted)
    assert "RAWPIXELS" not in str(redacted) and "OTHERPIXELS" not in str(redacted)
    assert original["content"][0]["data"] == "SECRETPIXELS"


def test_cli_success_without_tool_evidence_is_rejected():
    with pytest.raises(ValueError):
        RUNNER["verify_visual_events"]([{"type": "turn.completed"}])


def test_editing_acceptance_requires_real_tool_evidence():
    with pytest.raises(ValueError, match="Missing editing workflow"):
        RUNNER["verify_editing_events"]([{"type": "turn.completed"}])


def test_pending_import_without_terminal_receipt_is_rejected():
    event = {"type": "item.completed", "item": {"type": "mcp_tool_call", "server": "engine_demo", "tool": "import_asset", "status": "completed", "result": {"structuredContent": {"status": "pending", "session_id": "s", "request_id": "r"}}}}
    with pytest.raises(ValueError, match="Unresolved"):
        RUNNER["verify_editing_events"]([event])
