#!/usr/bin/env python3
"""Restore raw non-UTF-8 bytes that were replaced by U+FFFD.

Some decompiled sources (notably TMFieldScene.cpp) contain raw single-byte
characters such as a `'\\xA8'` character literal and Latin-1 comments. Tools
that assume UTF-8 replace those bytes with EF BF BD (U+FFFD), which changes
program semantics.

This script compares the working file with a git revision and restores any
line that is exactly the UTF-8-replacement-mangled form of a HEAD line.

Usage (from the repo root):

    python docs/tools/restore_non_utf8.py --file Projects/TMProject/TMFieldScene.cpp
    python docs/tools/restore_non_utf8.py --file ... --rev HEAD --dry-run
"""

import argparse
import subprocess
import sys


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--file", required=True, help="path relative to repo root")
    ap.add_argument("--rev", default="HEAD")
    ap.add_argument("--repo", default=".")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    head = subprocess.check_output(
        ["git", "-C", args.repo, "show", "%s:%s" % (args.rev, args.file)])
    path = args.file if args.repo == "." else args.repo + "/" + args.file
    work = open(path, "rb").read()

    suspects = [line for line in head.split(b"\n")
                if any(byte > 127 for byte in line)]
    print("HEAD lines with raw non-ASCII bytes:", len(suspects))

    restored = 0
    for original in suspects:
        mangled = original.decode("utf-8", errors="replace").encode("utf-8")
        if mangled == original:
            continue
        count = work.count(mangled)
        if count:
            print("restore x%d: %r" % (count, original[:100]))
            if not args.dry_run:
                work = work.replace(mangled, original)
            restored += count
        else:
            print("already intact (mangled form not present): %r"
                  % (original[:100],))

    if not args.dry_run:
        open(path, "wb").write(work)
    print("restored:", restored, "(dry-run)" if args.dry_run else "")
    print("remaining U+FFFD:", work.count(b"\xef\xbf\xbd"))


if __name__ == "__main__":
    main()
