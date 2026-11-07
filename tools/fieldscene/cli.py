"""Command line entry point for the FieldScene Inspector."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

from .assets import AssetLibrary
from .audit import (
    build_audit,
    compare_baseline,
    load_known,
    read_baseline,
    write_baseline,
)
from .controls import Constants
from .render import (
    RenderOptions,
    render_overlay,
    render_scene,
    save_layout,
)
from .scene import Scene, load_ui_strings, summarize
from .source import scan_source

TOOL_DIR = Path(__file__).resolve().parent
REPO = TOOL_DIR.parents[1]
WORKSPACE = REPO.parent
DEFAULT_ASSETS = WORKSPACE / "client_748"
DEFAULT_REF_ASSETS = WORKSPACE / "client"
DEFAULT_HEADER = REPO / "Projects" / "TMProject" / "ResourceControl.h"
DEFAULT_SOURCE = REPO / "Projects" / "TMProject" / "TMFieldScene.cpp"
DEFAULT_OUT = TOOL_DIR / "out"
DEFAULT_BASELINE = TOOL_DIR / "data" / "baseline.json"


def _paths(args):
    assets = Path(args.assets)
    scene_path = Path(args.scene) if args.scene else assets / "UI" / "FieldScene2.bin"
    ref_assets = None
    if not args.no_reference:
        ref_assets = Path(args.ref_assets)
    return assets, scene_path, ref_assets


def _load(args, candidates: bool = True):
    assets_root, scene_path, ref_root = _paths(args)
    scene = Scene.load(scene_path)
    assets = AssetLibrary(assets_root)
    constants = Constants(args.header)
    scan = scan_source(args.source, constants)

    ref_scene = None
    ref_assets = None
    if ref_root is not None and (ref_root / "UI" / "FieldScene2.bin").exists():
        try:
            ref_scene = Scene.load(ref_root / "UI" / "FieldScene2.bin")
            ref_assets = AssetLibrary(ref_root)
        except Exception as exc:  # noqa: BLE001 - reference data is optional
            print("warning: cannot read reference scene: %s" % exc, file=sys.stderr)

    known = load_known()
    audit = build_audit(
        scene,
        assets,
        constants,
        scan,
        known=known,
        ref_scene=ref_scene,
        ref_assets=ref_assets,
        with_candidates=candidates,
    )
    strings = load_ui_strings(assets_root / "UI" / "UIString.txt")
    return scene, assets, constants, scan, audit, strings


def _common_parser(sub):
    sub.add_argument("--assets", default=str(DEFAULT_ASSETS),
                     help="748 client directory (default: ../client_748)")
    sub.add_argument("--ref-assets", default=str(DEFAULT_REF_ASSETS),
                     help="755 client directory used for candidate matching (default: ../client)")
    sub.add_argument("--no-reference", action="store_true",
                     help="do not read the 755 scene")
    sub.add_argument("--scene", default=None,
                     help="scene binary (default: <assets>/UI/FieldScene2.bin)")
    sub.add_argument("--header", default=str(DEFAULT_HEADER),
                     help="ResourceControl.h")
    sub.add_argument("--source", default=str(DEFAULT_SOURCE),
                     help="TMFieldScene.cpp")


def _print_summary(audit):
    s = audit.summary
    print("scene: %d controls, %d unique ids" % (
        audit.scene_summary["controls"], audit.scene_summary["unique_ids"]))
    print("references: %d (%d resolved)" % (s["references"], s["resolved_references"]))
    print("dead: %d values / %d references, %d with candidates" % (
        s["dead_values"], s["dead_references"], s["dead_values_with_candidates"]))
    print("type mismatches: %d, unknown constants: %d, computed: %d" % (
        s["type_mismatches"], s["unknown_constants"], s["computed_references"]))
    print("orphan controls: %d, unnamed scene ids: %d, duplicate values: %d" % (
        s["orphan_controls"], s["unnamed_controls"], s["duplicate_values"]))
    if audit.asset_warnings:
        print("asset warnings: %d" % len(audit.asset_warnings))


def cmd_inspect(args):
    scene = Scene.load(args.scene or (Path(args.assets) / "UI" / "FieldScene2.bin"))
    info = summarize(scene)
    print(json.dumps(info, indent=2, ensure_ascii=False))
    return 0


def cmd_audit(args):
    _, _, _, _, audit, _ = _load(args, candidates=not args.no_candidates)
    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)
    md_path = out_dir / "fieldscene-audit.md"
    json_path = out_dir / "fieldscene-audit.json"
    md_path.write_text(audit.to_markdown(), encoding="utf-8")
    json_path.write_text(
        json.dumps(audit.to_dict(), indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )
    _print_summary(audit)
    print("wrote %s" % md_path)
    print("wrote %s" % json_path)

    baseline_path = Path(args.baseline) if args.baseline else DEFAULT_BASELINE
    if args.write_baseline:
        write_baseline(baseline_path, audit)
        print("wrote baseline %s" % baseline_path)
        return 0

    baseline_keys = read_baseline(baseline_path)
    if baseline_keys is None:
        print("baseline: none (use --write-baseline to create %s)" % baseline_path)
        return 0
    new, fixed = compare_baseline(audit, baseline_keys)
    print("baseline: %d new, %d fixed" % (len(new), len(fixed)))
    for key in new[:40]:
        print("  NEW   %s" % key)
    for key in fixed[:40]:
        print("  FIXED %s" % key)
    if args.fail_on_new and new:
        return 1
    return 0


def cmd_render(args):
    scene, assets, constants, _, audit, strings = _load(args, candidates=False)
    opts = RenderOptions(
        width=args.width,
        height=args.height,
        labels=args.labels,
        outline=args.outline,
        frame=args.frame,
        draw_text=not args.no_text,
        fit_bounds=args.fit,
        background=None if args.transparent else "checker",
    )
    png = Path(args.png)
    img, _ = save_layout(png, scene, assets, constants, audit, strings, opts)
    print("wrote %s (%dx%d)" % (png, img.width, img.height))
    return 0


def _parse_pair(value, name):
    parts = str(value).split(",")
    if len(parts) != 2:
        raise SystemExit("%s must be X,Y (got %r)" % (name, value))
    return int(parts[0]), int(parts[1])


def cmd_overlay(args):
    scene, assets, constants, _, audit, strings = _load(args, candidates=False)
    origin = _parse_pair(args.origin, "--origin")
    size = _parse_pair(args.size, "--size")
    opts = RenderOptions(
        width=size[0],
        height=size[1],
        labels=args.labels,
        outline=args.outline,
        frame=args.frame,
        draw_text=not args.no_text,
    )
    img = render_overlay(
        args.screenshot,
        scene,
        assets,
        constants,
        audit,
        strings,
        opts,
        origin=origin,
        size=size,
        alpha=args.alpha,
        mode=args.mode,
    )
    png = Path(args.png)
    png.parent.mkdir(parents=True, exist_ok=True)
    img.save(png)
    print("wrote %s (%dx%d)" % (png, img.width, img.height))
    return 0


def cmd_serve(args):
    from .server import serve

    return serve(
        assets=args.assets,
        scene=args.scene,
        header=args.header,
        source=args.source,
        ref_assets=None if args.no_reference else args.ref_assets,
        port=args.port,
        open_browser=not args.no_browser,
        verbose=args.verbose,
    )


def build_parser():
    parser = argparse.ArgumentParser(
        prog="fieldscene",
        description="Inspect, audit and visualise WYD 748 FieldScene2.bin",
    )
    sub = parser.add_subparsers(dest="command", required=True)

    p = sub.add_parser("audit", help="write a binding audit (markdown + json)")
    _common_parser(p)
    p.add_argument("--out", default=str(DEFAULT_OUT))
    p.add_argument("--baseline", default=None,
                   help="baseline json (default: tools/fieldscene/data/baseline.json)")
    p.add_argument("--write-baseline", action="store_true")
    p.add_argument("--fail-on-new", action="store_true",
                   help="exit 1 when the audit has new problems")
    p.add_argument("--no-candidates", action="store_true")
    p.set_defaults(func=cmd_audit)

    p = sub.add_parser("render", help="render the scene layout to a PNG")
    _common_parser(p)
    p.add_argument("--png", default=str(DEFAULT_OUT / "fieldscene-layout.png"))
    p.add_argument("--width", type=int, default=800)
    p.add_argument("--height", type=int, default=600)
    p.add_argument("--labels", choices=("none", "ids", "names"), default="none")
    p.add_argument("--outline", choices=("none", "all", "status"), default="status")
    p.add_argument("--frame", type=int, default=0)
    p.add_argument("--no-text", action="store_true")
    p.add_argument("--fit", action="store_true", help="size the canvas to the scene bounds")
    p.add_argument("--transparent", action="store_true")
    p.set_defaults(func=cmd_render)

    p = sub.add_parser("overlay", help="overlay the scene on a client screenshot")
    _common_parser(p)
    p.add_argument("--screenshot", required=True)
    p.add_argument("--png", default=str(DEFAULT_OUT / "fieldscene-overlay.png"))
    p.add_argument("--origin", default="8,31", help="client-area origin in the screenshot")
    p.add_argument("--size", default="800,600")
    p.add_argument("--alpha", type=int, default=140)
    p.add_argument("--mode", choices=("rects", "art", "both"), default="rects")
    p.add_argument("--labels", choices=("none", "ids", "names"), default="ids")
    p.add_argument("--outline", choices=("none", "all", "status"), default="status")
    p.add_argument("--frame", type=int, default=0)
    p.add_argument("--no-text", action="store_true")
    p.set_defaults(func=cmd_overlay)

    p = sub.add_parser("serve", help="start the local interactive viewer")
    _common_parser(p)
    p.add_argument("--port", type=int, default=0)
    p.add_argument("--no-browser", action="store_true")
    p.add_argument("--verbose", action="store_true")
    p.set_defaults(func=cmd_serve)

    p = sub.add_parser("inspect", help="print the scene summary only")
    p.add_argument("--assets", default=str(DEFAULT_ASSETS))
    p.add_argument("--scene", default=None)
    p.set_defaults(func=cmd_inspect)

    return parser


def main(argv=None):
    parser = build_parser()
    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
