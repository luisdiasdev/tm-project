"""Local, read-only web viewer for the FieldScene Inspector."""

from __future__ import annotations

import json
import sys
import threading
import webbrowser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlparse

from .assets import AssetLibrary
from .audit import build_audit, load_known
from .controls import Constants
from .render import RenderOptions, render_scene, status_map
from .scene import Scene, load_ui_strings
from .source import scan_source

TOOL_DIR = Path(__file__).resolve().parent
REPO_DIR = TOOL_DIR.parents[1]
WEB_DIR = TOOL_DIR / "web"
TRANSPARENT_PNG = (
    b"\x89PNG\r\n\x1a\n\x00\x00\x00\rIHDR\x00\x00\x00\x01\x00\x00\x00\x01"
    b"\x08\x06\x00\x00\x00\x1f\x15\xc4\x89\x00\x00\x00\rIDATx\x9cc\x00\x01"
    b"\x00\x00\x05\x00\x01\r\n-\xb4\x00\x00\x00\x00IEND\xaeB`\x82"
)


class ViewerState:
    def __init__(self, assets_root, scene_path, header, source, ref_assets):
        self.assets_root = Path(assets_root)
        self.scene = Scene.load(scene_path)
        self.assets = AssetLibrary(self.assets_root)
        self.constants = Constants(header)
        self.scan = scan_source(source, self.constants)
        self.strings = load_ui_strings(self.assets_root / "UI" / "UIString.txt")

        ref_scene = None
        ref_assets_obj = None
        if ref_assets:
            ref_root = Path(ref_assets)
            if (ref_root / "UI" / "FieldScene2.bin").exists():
                try:
                    ref_scene = Scene.load(ref_root / "UI" / "FieldScene2.bin")
                    ref_assets_obj = AssetLibrary(ref_root)
                except Exception as exc:  # noqa: BLE001
                    print("warning: reference scene unavailable: %s" % exc, file=sys.stderr)

        known = load_known()
        self.known = known
        self.audit = build_audit(
            self.scene,
            self.assets,
            self.constants,
            self.scan,
            known=known,
            ref_scene=ref_scene,
            ref_assets=ref_assets_obj,
            with_candidates=True,
        )
        self.status = status_map(self.audit)
        self._state_json = None
        self._texture_cache: dict[tuple, bytes] = {}

    # -- API --------------------------------------------------------------
    def state(self):
        if self._state_json is None:
            self._state_json = json.dumps(self._build_state(), ensure_ascii=False)
        return self._state_json

    def _build_state(self):
        constants = self.constants
        unnamed = {item["id"]: item for item in self.audit.unnamed_controls}
        children = {}
        for c in self.scene.controls:
            children.setdefault(c.parent, []).append(c.id)

        controls = []
        for c in self.scene.controls:
            info = None if c.type == 15 else self.assets.frame_info(c.tex, 0, decode=False)
            names = constants.names_for(c.id)
            proposed = unnamed.get(c.id, {}).get("proposed_name")
            text = ""
            if c.string_index is not None:
                text = self.strings.get(int(c.string_index), "")
            sprite = None
            if info and info.get("solid") is None and info.get("rect"):
                sprite = {
                    "set": info["set"],
                    "frame": info["frame"],
                    "rect": info["rect"],
                    "dest": info["dest"],
                    "file": info["file"],
                    "name": info["name"],
                    "url": "/api/texture/%d/%d" % (c.tex, info["frame"]),
                }
            fields = dict(c.values)
            controls.append(
                {
                    "index": c.index,
                    "id": c.id,
                    "type": c.type_name,
                    "type_id": c.type,
                    "parent": c.parent,
                    "tex": c.tex,
                    "rect": [c.x, c.y, c.w, c.h],
                    "fields": fields,
                    "names": names,
                    "primary_name": names[0] if names else None,
                    "proposed_name": proposed,
                    "status": self.status.get(c.id, "orphan" if c.id else "orphan"),
                    "string_index": c.string_index,
                    "text": text,
                    "sprite": sprite,
                    "children": children.get(c.id, []),
                }
            )

        return {
            "meta": {
                "scene": str(self.scene.path),
                "control_count": len(self.scene.controls),
                "unique_ids": len(self.scene.ids),
                "bounds": list(self.scene.bounds()),
                "types": self.scene.type_counts(),
                "summary": self.audit.summary,
                "paths": self.audit.paths,
                "scene_warnings": self.scene.warnings,
                "asset_warnings": self.audit.asset_warnings,
                "source_warnings": self.audit.source_warnings,
                "duplicate_constants": self.audit.duplicate_constants,
                "texture_problems": self.audit.texture_problems,
            },
            "controls": controls,
            "dead_references": self.audit.dead_references,
            "type_mismatches": self.audit.type_mismatches,
            "computed_references": self.audit.computed_references,
            "unnamed_controls": self.audit.unnamed_controls,
            "orphan_controls": [
                {"id": item["id"], "type": item["type"], "rect": item["rect"]}
                for item in self.audit.orphan_controls
            ],
            "references": [a.to_dict() for a in self.audit.ref_audits],
        }

    def texture(self, tex: int, frame: int) -> bytes:
        key = (tex, frame)
        if key in self._texture_cache:
            return self._texture_cache[key]
        image = self.assets.frame_image(tex, frame)
        payload = image.to_png() if image is not None else TRANSPARENT_PNG
        self._texture_cache[key] = payload
        return payload

    def render_payload(self, width=800, height=600, frame=0):
        img = render_scene(
            self.scene,
            self.assets,
            self.constants,
            self.audit,
            self.strings,
            RenderOptions(width=width, height=height, labels="none", outline="none", frame=frame),
        )
        return img.to_png()


def _make_handler(viewer: ViewerState, verbose: bool):
    class Handler(BaseHTTPRequestHandler):
        server_version = "FieldSceneViewer/0.1"

        def log_message(self, fmt, *args):  # noqa: A003
            if verbose:
                super().log_message(fmt, *args)

        def _send(self, status, content_type, payload: bytes):
            self.send_response(status)
            self.send_header("Content-Type", content_type)
            self.send_header("Content-Length", str(len(payload)))
            self.send_header("Cache-Control", "no-store")
            self.end_headers()
            if self.command != "HEAD":
                self.wfile.write(payload)

        def do_HEAD(self):  # noqa: N802
            self.do_GET()

        def do_GET(self):  # noqa: N802
            parsed = urlparse(self.path)
            path = parsed.path
            try:
                if path in ("/", "/index.html"):
                    return self._file("index.html", "text/html; charset=utf-8")
                if path == "/app.js":
                    return self._file("app.js", "application/javascript; charset=utf-8")
                if path == "/style.css":
                    return self._file("style.css", "text/css; charset=utf-8")
                if path == "/api/state":
                    return self._send(200, "application/json; charset=utf-8", viewer.state().encode("utf-8"))
                if path == "/api/health":
                    return self._send(200, "application/json", b'{"ok":true}')
                if path.startswith("/api/texture/"):
                    parts = path.strip("/").split("/")
                    if len(parts) != 4:
                        return self._send(400, "text/plain", b"bad texture path")
                    tex = int(parts[2])
                    frame = int(parts[3])
                    return self._send(200, "image/png", viewer.texture(tex, frame))
                if path.startswith("/api/render"):
                    return self._send(200, "image/png", viewer.render_payload())
                return self._send(404, "text/plain", b"not found")
            except Exception as exc:  # noqa: BLE001
                if verbose:
                    import traceback

                    traceback.print_exc()
                return self._send(500, "text/plain", ("error: %s" % exc).encode("utf-8"))

        def _file(self, name, content_type):
            p = WEB_DIR / name
            if not p.exists():
                return self._send(404, "text/plain", b"missing web asset")
            return self._send(200, content_type, p.read_bytes())

    return Handler


def serve(
    assets,
    scene=None,
    header=None,
    source=None,
    ref_assets=None,
    port=0,
    open_browser=True,
    verbose=False,
):
    assets = Path(assets)
    scene_path = Path(scene) if scene else assets / "UI" / "FieldScene2.bin"
    header = Path(header) if header else REPO_DIR / "Projects" / "TMProject" / "ResourceControl.h"
    source = Path(source) if source else REPO_DIR / "Projects" / "TMProject" / "TMFieldScene.cpp"

    print("loading scene and assets ...")
    viewer = ViewerState(assets, scene_path, header, source, ref_assets)
    print("controls=%d refs=%d dead=%d" % (
        len(viewer.scene.controls),
        viewer.audit.summary["references"],
        viewer.audit.summary["dead_values"],
    ))

    handler = _make_handler(viewer, verbose)
    httpd = ThreadingHTTPServer(("127.0.0.1", port), handler)
    actual_port = httpd.server_address[1]
    url = "http://127.0.0.1:%d/" % actual_port
    print("serving %s" % url)
    if open_browser:
        threading.Timer(0.4, lambda: webbrowser.open(url)).start()
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nstopped")
    finally:
        httpd.server_close()
    return 0
