#!/usr/bin/env python3
"""Generate the WYD 748 client's ``serverlist.bin`` and ``sn.bin``.

Both files are read by the client during startup:

* ``serverlist.bin`` -- 10 groups x 11 entries x 64 bytes (7040 bytes).
  Entry 0 of each group is the user-count URL, entries 1..10 are the server
  addresses.  Every 64-byte record is "scrambled" with the fixed byte table
  from ``BASE_InitializeServerList()`` (Basedef.cpp).

* ``sn.bin`` -- 11 names x 9 bytes, followed by 11 little-endian int32 values
  (143 bytes).  The int32 list maps a visible group to a group in
  ``serverlist.bin`` (value = serverlist group index + 1) and the names are
  offset by one (the client displays ``names[i + 1]`` for mapping entry ``i``;
  ``names[0]`` is a non-empty sentinel such as "Reserved").

Defaults reproduce the layout shipped with the 748 client (one reserved group
plus MALECH / SEPHIRA / ALASTAR / NOVATO), but every address is 127.0.0.1.
Entry 0 (the user-count URL) defaults to the plain string "127.0.0.1": the
client only uses it to decide whether a group exists, and WinINet rejects it
instantly instead of waiting for an HTTP timeout.

Examples
--------
Write the default localhost pair into the client folder::

    python docs\\tools\\generate_serverlist.py

Write a different layout (2 servers per group, custom name) elsewhere::

    python docs\\tools\\generate_serverlist.py --out C:\\tmp\\srv \\
        --servers 2 --group-names "LOCAL"

Use a config file for full control (see ``--config`` / ``--dump``)::

    python docs\\tools\\generate_serverlist.py --config my_servers.json

    my_servers.json:
    {
      "groups": [
        {"name": "Reserved", "url": "127.0.0.1", "servers": ["127.0.0.1"]},
        {"name": "MALECH",   "url": "127.0.0.1", "servers": ["127.0.0.1"]}
      ]
    }

The first ``groups`` element is the hidden/reserved group 0.
"""

from __future__ import annotations

import argparse
import json
import shutil
import struct
import sys
from pathlib import Path

# --- Formats (must match Projects/TMProject/Basedef.h + Basedef.cpp) --------

MAX_SERVERGROUP = 10          # g_pServerList[10]
MAX_SERVERNUMBER = 11         # MAX_SERVER + 1, g_pServerList[.][11]
ENTRY_SIZE = 64               # g_pServerList[.][.][64]
SERVERLIST_SIZE = MAX_SERVERGROUP * MAX_SERVERNUMBER * ENTRY_SIZE  # 7040

NAME_COUNT = 11               # g_szServerNameList[11]
NAME_SIZE = 9                 # char[9]
SN_SIZE = NAME_COUNT * NAME_SIZE + NAME_COUNT * 4  # 143

# szList[] from BASE_InitializeServerList().  The client does:
#     plain[i] = file[i] - szList[63 - i]
SCRAMBLE = [
    0xA4, 0xA1, 0xA4, 0xA4, 0xA4, 0xA7, 0xA4, 0xA9, 0xA4, 0xB1, 0xA4,
    0xB2, 0xA4, 0xB5, 0xA4, 0xB7, 0xA4, 0xB8, 0xA4, 0xBA, 0xA4, 0xBB,
    0xA4, 0xBC, 0xA4, 0xBD, 0xA4, 0xBE, 0xA4, 0xBF, 0xA4, 0xC1, 0xA4,
    0xC3, 0xA4, 0xC5, 0xA4, 0xC7, 0xA4, 0xCB, 0xA4, 0xCC, 0xA4, 0xD0,
    0xA4, 0xD1, 0xA4, 0xD3, 0xA4, 0xBF, 0xA4, 0xC4, 0xA4, 0xD3, 0xA4,
    0xC7, 0xA4, 0xCC, 0xB0, 0xA1, 0xB3, 0xAA, 0xB4, 0xD9,
]
assert len(SCRAMBLE) == ENTRY_SIZE

DEFAULT_GROUP_NAMES = ["MALECH", "SEPHIRA", "ALASTAR", "NOVATO"]
DEFAULT_SERVERS_PER_GROUP = [6, 4, 4, 1]  # same layout as the shipped 748 file
DEFAULT_RESERVED_SERVERS = 2
DEFAULT_RESERVED_NAME = "Reserved"
DEFAULT_URL = "127.0.0.1"
DEFAULT_IP = "127.0.0.1"


class FormatError(ValueError):
    pass


# --- Encode / decode -------------------------------------------------------

def scramble_record(record: bytes) -> bytes:
    """plain -> on-disk for one 64-byte record."""
    if len(record) != ENTRY_SIZE:
        raise FormatError(f"record must be {ENTRY_SIZE} bytes, got {len(record)}")
    return bytes((record[i] + SCRAMBLE[ENTRY_SIZE - 1 - i]) & 0xFF
                 for i in range(ENTRY_SIZE))


def descramble_record(record: bytes) -> bytes:
    """on-disk -> plain for one 64-byte record."""
    if len(record) != ENTRY_SIZE:
        raise FormatError(f"record must be {ENTRY_SIZE} bytes, got {len(record)}")
    return bytes((record[i] - SCRAMBLE[ENTRY_SIZE - 1 - i]) & 0xFF
                 for i in range(ENTRY_SIZE))


def _text_bytes(value: str, limit: int, what: str) -> bytes:
    try:
        raw = value.encode("latin-1")
    except UnicodeEncodeError as exc:
        raise FormatError(f"{what} {value!r} is not latin-1 encodable") from exc
    if b"\0" in raw:
        raise FormatError(f"{what} {value!r} must not contain NUL bytes")
    if len(raw) > limit:
        raise FormatError(
            f"{what} {value!r} is {len(raw)} bytes, max is {limit}")
    return raw


def build_serverlist(groups: list[dict]) -> bytes:
    """groups[i] = {"name", "url", "servers"} in serverlist order."""
    if len(groups) > MAX_SERVERGROUP:
        raise FormatError(
            f"at most {MAX_SERVERGROUP} groups fit in serverlist.bin")

    buf = bytearray(SERVERLIST_SIZE)
    for group_index in range(MAX_SERVERGROUP):
        group = groups[group_index] if group_index < len(groups) else {}
        if group.get("servers") and not group.get("url"):
            # The client only lists a group when entry 0 (the user-count URL)
            # is non-empty; see TMSelectServerScene::InitializeUI().
            raise FormatError(
                f"group {group_index} has servers but no 'url'; the client "
                "hides groups whose entry 0 is empty (use any placeholder, "
                "e.g. \"127.0.0.1\")")
        entries = [group.get("url", "")] + list(group.get("servers", []))
        if len(entries) > MAX_SERVERNUMBER:
            raise FormatError(
                f"group {group_index} has {len(entries)} entries; "
                f"max is {MAX_SERVERNUMBER} (URL + 10 servers)")
        # Every one of the 11 slots must be written: the client descrambles the
        # whole 7040-byte file, so an untouched zero byte would decode to a
        # non-zero garbage string and look like a real server entry.
        entries.extend([""] * (MAX_SERVERNUMBER - len(entries)))
        for entry_index, text in enumerate(entries):
            raw = _text_bytes(text, ENTRY_SIZE - 1,
                              f"group {group_index} entry {entry_index}")
            record = raw.ljust(ENTRY_SIZE, b"\0")
            offset = (group_index * MAX_SERVERNUMBER + entry_index) * ENTRY_SIZE
            buf[offset:offset + ENTRY_SIZE] = scramble_record(record)
    return bytes(buf)


def build_sn(groups: list[dict]) -> bytes:
    """Build sn.bin for the same ``groups`` list used by build_serverlist()."""
    names = [b""] * NAME_COUNT
    counts = [0] * NAME_COUNT

    # names[0] is only a sentinel: the client checks names[i][0] but displays
    # names[i + 1], so it must be non-empty for the first group row.
    reserved_name = (groups[0].get("name") if groups else "") or DEFAULT_RESERVED_NAME
    names[0] = _text_bytes(reserved_name, NAME_SIZE - 1, "reserved group name")

    # Only groups that the client will actually list get a mapping entry.
    # Entries are packed from index 0 upwards because the click handler counts
    # the non-zero entries; counts[i] - 1 is the serverlist group index.
    row = 0
    for group_index, group in enumerate(groups[1:], start=1):
        if not (group.get("url") or group.get("servers")):
            continue
        if row >= NAME_COUNT - 1:
            raise FormatError("too many visible groups for sn.bin")
        name = group.get("name") or f"GROUP{group_index}"
        names[row + 1] = _text_bytes(
            name, NAME_SIZE - 1, f"group {group_index} name")
        counts[row] = group_index + 1
        row += 1

    out = bytearray()
    for name in names:
        out += name.ljust(NAME_SIZE, b"\0")
    for count in counts:
        out += struct.pack("<i", count)
    assert len(out) == SN_SIZE
    return bytes(out)


def parse_serverlist(data: bytes) -> list[list[str]]:
    if len(data) != SERVERLIST_SIZE:
        raise FormatError(
            f"serverlist.bin is {len(data)} bytes, expected {SERVERLIST_SIZE}")
    groups: list[list[str]] = []
    for group_index in range(MAX_SERVERGROUP):
        entries = []
        for entry_index in range(MAX_SERVERNUMBER):
            offset = (group_index * MAX_SERVERNUMBER + entry_index) * ENTRY_SIZE
            record = descramble_record(data[offset:offset + ENTRY_SIZE])
            entries.append(record.split(b"\0", 1)[0].decode("latin-1"))
        groups.append(entries)
    return groups


def parse_sn(data: bytes) -> tuple[list[str], list[int]]:
    if len(data) != SN_SIZE:
        raise FormatError(f"sn.bin is {len(data)} bytes, expected {SN_SIZE}")
    names = [
        data[i * NAME_SIZE:(i + 1) * NAME_SIZE].split(b"\0", 1)[0]
        .decode("latin-1")
        for i in range(NAME_COUNT)
    ]
    counts = list(struct.unpack("<11i", data[NAME_COUNT * NAME_SIZE:SN_SIZE]))
    return names, counts


# --- Defaults / config -----------------------------------------------------

def default_groups(ips: list[str], servers_per_group: int | None,
                   group_names: list[str], url: str,
                   reserved_name: str,
                   reserved_servers: int = DEFAULT_RESERVED_SERVERS) -> list[dict]:
    addresses = ips or [DEFAULT_IP]

    def addresses_for(count: int) -> list[str]:
        return [addresses[i % len(addresses)] for i in range(count)]

    groups = [{
        "name": reserved_name or DEFAULT_RESERVED_NAME,
        "url": url,
        "servers": addresses_for(reserved_servers),
    }]
    for index, name in enumerate(group_names, start=1):
        if servers_per_group is None:
            # The shipped layout has different counts per group; the top group
            # aggregates one server from every other group, so it expects the
            # lower groups to have more than one entry.
            count_index = (index - 1) % len(DEFAULT_SERVERS_PER_GROUP)
            count = DEFAULT_SERVERS_PER_GROUP[count_index]
        else:
            count = servers_per_group
        groups.append({"name": name, "url": url,
                       "servers": addresses_for(count)})
    return groups


def load_config(path: Path) -> list[dict]:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except OSError as exc:
        raise SystemExit(f"cannot read config {path}: {exc}") from exc
    except json.JSONDecodeError as exc:
        raise SystemExit(f"invalid JSON in {path}: {exc}") from exc

    groups = data.get("groups") if isinstance(data, dict) else data
    if not isinstance(groups, list) or not groups:
        raise SystemExit(f"{path}: expected a non-empty 'groups' list")

    normalised = []
    for index, group in enumerate(groups):
        if not isinstance(group, dict):
            raise SystemExit(f"{path}: groups[{index}] must be an object")
        servers = group.get("servers", [])
        if isinstance(servers, str):
            servers = [servers]
        normalised.append({
            "name": str(group.get("name", "")),
            "url": str(group.get("url", "")),
            "servers": [str(s) for s in servers],
        })
    return normalised


def find_default_out_dir() -> Path:
    # repo/tmproject/docs/tools/this_file -> repo, then repo/../client_748
    repo = Path(__file__).resolve().parents[2]
    client = repo.parent / "client_748"
    if client.is_dir():
        return client
    return Path.cwd()


# --- Main ------------------------------------------------------------------

def write_file(path: Path, data: bytes, backup: bool) -> None:
    if backup and path.exists():
        shutil.copy2(path, path.with_name(path.name + ".bak"))
    path.write_bytes(data)


def dump(out_dir: Path) -> int:
    serverlist = out_dir / "serverlist.bin"
    sn = out_dir / "sn.bin"
    try:
        groups = parse_serverlist(serverlist.read_bytes())
        names, counts = parse_sn(sn.read_bytes())
    except OSError as exc:
        print(f"cannot read files: {exc}", file=sys.stderr)
        return 1
    except FormatError as exc:
        print(f"invalid data: {exc}", file=sys.stderr)
        return 1

    print(f"names : {names}")
    print(f"counts: {counts}")

    referenced: set[int] = set()
    for i, count in enumerate(counts):
        if count <= 0:
            continue
        group_index = count - 1
        if not 0 <= group_index < MAX_SERVERGROUP:
            print(f"  mapping[{i}] -> invalid group {group_index}")
            continue
        referenced.add(group_index)
        display = names[i + 1] if i + 1 < NAME_COUNT else ""
        entries = groups[group_index]
        server_count = sum(1 for value in entries[1:] if value)
        print(f"  row {i}: name={display!r} -> serverlist group "
              f"{group_index} ({server_count} server(s))")
        print(f"         entry0={entries[0]!r} servers={entries[1:]}")
        if not entries[0]:
            print("         WARNING: entry 0 is empty; the client hides "
                  "this group")

    reserved = groups[0]
    print(f"  hidden group 0: entry0={reserved[0]!r} "
          f"servers={reserved[1:]}")

    for group_index, entries in enumerate(groups):
        if group_index and any(entries) and group_index not in referenced:
            print(f"  WARNING: group {group_index} has data but no sn.bin "
                  "entry references it (it will not be shown)")
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Generate WYD 748 serverlist.bin and sn.bin",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("--out", type=Path, default=None,
                        help="output directory (default: ../client_748 if it "
                             "exists, else the current directory)")
    parser.add_argument("--config", type=Path, default=None,
                        help="JSON config with a 'groups' list (group 0 = "
                             "reserved/hidden group)")
    parser.add_argument("--ip", action="append", default=None,
                        help=f"server address, repeatable (default {DEFAULT_IP})")
    parser.add_argument("--servers", type=int, default=None, metavar="N",
                        help="servers per visible group (default: the shipped "
                             "748 layout, "
                             f"{DEFAULT_SERVERS_PER_GROUP})")
    parser.add_argument("--group-names", default=",".join(DEFAULT_GROUP_NAMES),
                        help="comma-separated visible group names "
                             f"(default: {','.join(DEFAULT_GROUP_NAMES)})")
    parser.add_argument("--url", default=DEFAULT_URL,
                        help="value stored in entry 0 of each group (user-"
                             "count URL; only needs to be non-empty, plain "
                             f"host names fail instantly) (default: {DEFAULT_URL})")
    parser.add_argument("--reserved-name", default=DEFAULT_RESERVED_NAME,
                        help="name of the hidden group 0 (default: "
                             f"{DEFAULT_RESERVED_NAME})")
    parser.add_argument("--no-backup", action="store_true",
                        help="do not keep a .bak copy of files being replaced")
    parser.add_argument("--dump", action="store_true",
                        help="decode and print the files in --out instead of "
                             "writing")
    args = parser.parse_args(argv)

    out_dir = args.out or find_default_out_dir()

    if args.dump:
        return dump(out_dir)

    if args.servers is not None and args.servers < 1:
        parser.error("--servers must be >= 1")

    if args.config:
        groups = load_config(args.config)
    else:
        group_names = [n.strip() for n in args.group_names.split(",") if n.strip()]
        if not group_names:
            group_names = list(DEFAULT_GROUP_NAMES)
        if len(group_names) > MAX_SERVERGROUP - 1:
            parser.error(f"at most {MAX_SERVERGROUP - 1} visible groups fit "
                         "in sn.bin/serverlist.bin")
        groups = default_groups(
            args.ip or [DEFAULT_IP], args.servers, group_names,
            args.url, args.reserved_name)

    try:
        serverlist_data = build_serverlist(groups)
        sn_data = build_sn(groups)
    except FormatError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    out_dir.mkdir(parents=True, exist_ok=True)
    write_file(out_dir / "serverlist.bin", serverlist_data, not args.no_backup)
    write_file(out_dir / "sn.bin", sn_data, not args.no_backup)

    print(f"serverlist.bin: {len(serverlist_data)} bytes -> "
          f"{(out_dir / 'serverlist.bin').resolve()}")
    print(f"sn.bin        : {len(sn_data)} bytes -> "
          f"{(out_dir / 'sn.bin').resolve()}")
    if not args.no_backup:
        print("previous files (if any) saved as *.bak")

    print("\nGroups the client will show (top to bottom):")
    for group_index, group in reversed(list(enumerate(groups[1:], start=1))):
        name = group.get("name") or f"GROUP{group_index}"
        entries = group.get("servers", [])
        unique = list(dict.fromkeys(entries))
        print(f"  {name:<9} group {group_index}: {len(entries)} server(s) "
              f"-> {', '.join(unique)}")
    hidden = groups[0]
    print(f"  (hidden)  group 0       : "
          f"{len(hidden.get('servers', []))} server(s)")
    print("\nNote: the top row is the client's aggregate group -- it borrows")
    print("one server from each lower group. All rows connect to the address")
    print("stored for them in serverlist.bin.")
    print("Use --dump to decode the files again.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
