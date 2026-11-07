"""Offline layout rendering and screenshot overlays."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from .assets import AssetLibrary
from .audit import Audit
from .controls import Constants
from .png import Image, draw_text, draw_text_shadow, read_png, text_width
from .scene import CTRL_3DOBJ, CTRL_GRID, CTRL_PANEL, CTRL_PROGRESS, CTRL_TEXT, Scene

STATUS_COLORS = {
    "resolved": (80, 200, 120, 230),
    "orphan": (70, 150, 255, 220),
    "unnamed": (255, 170, 40, 235),
    "mismatch": (240, 70, 70, 245),
    "selected": (255, 235, 60, 255),
}


@dataclass
class RenderOptions:
    width: int = 800
    height: int = 600
    labels: str = "none"       # none | ids | names
    outline: str = "status"    # none | all | status
    frame: int = 0
    draw_text: bool = True
    fit_bounds: bool = False
    background: str | None = "checker"
    dim_orphans: bool = False


def argb(value: int):
    v = int(value) & 0xFFFFFFFF
    return ((v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF, (v >> 24) & 0xFF)


def checkerboard(img: Image, cell: int = 16):
    dark = (28, 30, 34, 255)
    light = (38, 40, 46, 255)
    for y in range(0, img.height, cell):
        for x in range(0, img.width, cell):
            color = dark if ((x // cell) + (y // cell)) % 2 == 0 else light
            img.fill_rect(x, y, min(cell, img.width - x), min(cell, img.height - y), color)


def status_map(audit: Audit) -> dict:
    """Map control id -> coarse status for outlines."""
    status = {}
    mismatch_ids = {item["value"] for item in audit.type_mismatches}
    unnamed_ids = {item["id"] for item in audit.unnamed_controls}
    referenced = getattr(audit, "referenced_ids", set())
    for item in audit.orphan_controls:
        status[item["id"]] = "orphan"
    for control_id in referenced:
        status[control_id] = "resolved"
    for control_id in unnamed_ids:
        status[control_id] = "unnamed"
    for control_id in mismatch_ids:
        status[control_id] = "mismatch"
    return status


def _text_for(scene_strings: dict, index):
    if index is None:
        return ""
    text = scene_strings.get(int(index), "")
    return text


def _blit_frame(img: Image, frame: Image | None, x, y, w, h, dest=(0, 0)):
    if frame is None:
        return
    dx, dy = dest
    img.blit_scaled(frame, x + dx, y + dy, w, h)


def _tile(img: Image, frame: Image | None, x, y, w, h):
    if frame is None or frame.width <= 0 or frame.height <= 0:
        return
    for yy in range(y, y + h, frame.height):
        for xx in range(x, x + w, frame.width):
            img.blit(frame, xx, yy)


def _draw_grid(img: Image, c, frame, info):
    rows = max(1, int(c.get("rows", 1) or 1))
    cols = max(1, int(c.get("columns", 1) or 1))
    cell_w = max(1, c.w // cols)
    cell_h = max(1, c.h // rows)
    if frame is None:
        img.outline_rect(c.x, c.y, c.w, c.h, (90, 90, 90, 160), 1)
        return
    for row in range(rows):
        for col in range(cols):
            img.blit_scaled(
                frame,
                c.x + col * cell_w,
                c.y + row * cell_h,
                cell_w,
                cell_h,
                230,
            )


def draw_control(img: Image, c, assets: AssetLibrary, strings: dict, opts: RenderOptions):
    if c.x >= img.width or c.y >= img.height or c.x + c.w < 0 or c.y + c.h < 0:
        return
    info = assets.frame_info(c.tex, opts.frame, decode=True)
    frame = info.get("image") if info else None
    dest = tuple(info.get("dest") or (0, 0)) if info else (0, 0)
    fill_type = c.get("fill_type")

    if c.type == CTRL_PANEL:
        if c.tex == -2:
            color = argb(c.get("color", 0))
            if color[3]:
                img.fill_rect(c.x, c.y, c.w, c.h, color)
            return
        if frame is None:
            return
        if fill_type == 4:
            _blit_frame(img, frame, c.x, c.y, c.w, c.h, dest)
        elif fill_type == 2:
            img.blit(frame, c.x + dest[0], c.y + dest[1])
        elif fill_type == 3:
            _tile(img, frame, c.x, c.y, c.w, c.h)
        else:
            _blit_frame(img, frame, c.x, c.y, c.w, c.h, dest)
        return

    if c.type == CTRL_GRID:
        _draw_grid(img, c, frame, info)
        return

    if c.type == 3:  # CHECKBOX (avoid importing every constant)
        _blit_frame(img, frame, c.x, c.y, c.w, c.h, dest)
        return

    if c.type == CTRL_3DOBJ:
        color = argb(c.get("color", 0x33000000))
        img.fill_rect(c.x, c.y, c.w, c.h, (color[0], color[1], color[2], 40))
        img.outline_rect(c.x, c.y, c.w, c.h, (120, 120, 120, 100), 1)
        draw_text_shadow(img, c.x + 3, c.y + 3, "3D", (200, 200, 200, 220), 1)
        return

    # Buttons, text, edits, list boxes, progress and 3D objects all use
    # RENDER_IMAGE_STRETCH in the client.
    _blit_frame(img, frame, c.x, c.y, c.w, c.h, dest)

    if c.type == CTRL_PROGRESS:
        current = c.get("current", 0) or 0
        maximum = c.get("max_value", 0) or 0
        if maximum > 0:
            frac = max(0.0, min(1.0, current / float(maximum)))
            bar = argb(c.get("progress_color", 0xFF00FF00))
            img.fill_rect(c.x + 2, c.y + 2, int((c.w - 4) * frac), max(1, c.h - 4), bar)

    if c.type == CTRL_TEXT or c.string_index is not None:
        text = _text_for(strings, c.string_index)
        if opts.draw_text and text:
            color = argb(c.get("font_color", 0xFFFFFFFF))
            scale = 2 if c.h >= 18 else 1
            tx = c.x + 2
            ty = c.y + max(1, (c.h - 7 * scale) // 2)
            draw_text_shadow(img, tx, ty, text, color, scale)

    if c.type == 13:  # EDIT
        value = str(c.get("value", "") or "")
        if opts.draw_text and value:
            color = argb(c.get("font_color", 0xFFFFFFFF))
            draw_text_shadow(img, c.x + 2, c.y + 2, value, color, 1)


def render_scene(
    scene: Scene,
    assets: AssetLibrary,
    constants: Constants,
    audit: Audit,
    strings: dict,
    opts: RenderOptions | None = None,
) -> Image:
    opts = opts or RenderOptions()
    width, height = opts.width, opts.height
    if opts.fit_bounds:
        x0, y0, x1, y1 = scene.bounds()
        width = max(1, x1 - x0 + 8)
        height = max(1, y1 - y0 + 8)
    img = Image(width, height)
    if opts.background == "checker":
        checkerboard(img)
    elif opts.background:
        img.fill_rect(0, 0, width, height, argb(0xFF000000))

    status = status_map(audit)
    name_by_id = {}
    for item in audit.orphan_controls:
        name_by_id[item["id"]] = item.get("name")
    for item in audit.unnamed_controls:
        name_by_id[item["id"]] = item.get("proposed_name") or item.get("name")

    for c in scene.controls:
        draw_control(img, c, assets, strings, opts)
        if opts.outline == "all":
            img.outline_rect(c.x, c.y, c.w, c.h, (100, 100, 100, 120), 1)
        elif opts.outline == "status":
            st = status.get(c.id, "resolved" if c.id else "orphan")
            img.outline_rect(c.x, c.y, c.w, c.h, STATUS_COLORS.get(st, (120, 120, 120, 120)), 1)
        if opts.labels == "ids":
            label = "#%d" % c.id
        elif opts.labels == "names":
            label = constants.primary_name(c.id) or name_by_id.get(c.id) or "#%d" % c.id
        else:
            label = ""
        if label:
            tw = text_width(label, 1)
            img.fill_rect(c.x, c.y, tw + 4, 11, (0, 0, 0, 170))
            draw_text(img, c.x + 2, c.y + 2, label, (255, 255, 255, 230), 1)
    return img


def render_overlay(
    screenshot_path,
    scene: Scene,
    assets: AssetLibrary,
    constants: Constants,
    audit: Audit,
    strings: dict,
    opts: RenderOptions | None = None,
    origin=(8, 31),
    size=(800, 600),
    alpha: int = 140,
    mode: str = "rects",
):
    """Overlay controls on a real client screenshot.

    ``origin``/``size`` select the client area inside the captured window.
    """
    opts = opts or RenderOptions()
    shot = read_png(screenshot_path)
    ox, oy = origin
    sw, sh = size
    client = Image(sw, sh)
    client.blit(shot, -ox, -oy)

    if mode in ("art", "both"):
        layer = render_scene(
            scene,
            assets,
            constants,
            audit,
            strings,
            RenderOptions(
                width=sw,
                height=sh,
                labels="none",
                outline="none",
                frame=opts.frame,
                draw_text=opts.draw_text,
                background=None,
            ),
        )
        client.blit(layer, 0, 0, opacity=alpha)

    if mode in ("rects", "both"):
        status = status_map(audit)
        for c in scene.controls:
            st = status.get(c.id, "resolved" if c.id else "orphan")
            color = STATUS_COLORS.get(st, (120, 120, 120, 200))
            client.outline_rect(c.x, c.y, c.w, c.h, color, 1)
            if opts.labels == "ids":
                label = "#%d" % c.id
            elif opts.labels == "names":
                label = constants.primary_name(c.id) or "#%d" % c.id
            else:
                label = ""
            if label:
                tw = text_width(label, 1)
                client.fill_rect(c.x, c.y, tw + 4, 11, (0, 0, 0, 180))
                draw_text(client, c.x + 2, c.y + 2, label, color, 1)

    # Small legend
    legend = "controls=%d  resolved=%d  orphans=%d  dead=%d" % (
        len(scene.controls),
        audit.summary.get("resolved_references", 0),
        audit.summary.get("orphan_controls", 0),
        audit.summary.get("dead_values", 0),
    )
    client.fill_rect(4, 4, text_width(legend, 1) + 8, 13, (0, 0, 0, 190))
    draw_text(client, 8, 7, legend, (240, 240, 240, 240), 1)
    return client


def save_layout(path, *args, **kwargs):
    img = render_scene(*args, **kwargs)
    Path(path).parent.mkdir(parents=True, exist_ok=True)
    img.save(path)
    return img, path
