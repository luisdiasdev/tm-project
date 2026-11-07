"""Launcher: ``python tools/fieldscene/fieldscene.py <command>``.

The implementation lives in this directory as the ``fieldscene`` package; this
wrapper puts ``tools/`` on ``sys.path`` so the command works from any CWD.
"""

from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from fieldscene.cli import main  # noqa: E402

if __name__ == "__main__":
    raise SystemExit(main())
