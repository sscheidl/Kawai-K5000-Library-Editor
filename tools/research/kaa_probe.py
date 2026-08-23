#!/usr/bin/env python3
"""Reproduce the Phase A KA1/KAA findings recorded in docs/FORMAT_NOTES.md.

This is *research tooling*, not production code and not a test. It exists so the
numbers in FORMAT_NOTES can be re-derived and challenged rather than taken on
trust (specification section 41: do not trust completion reports, inspect).

The reference corpus is READ-ONLY. This script opens files for reading only and
never writes to the corpus path.

Usage:
    python tools/research/kaa_probe.py <corpus-dir-or-zip> [...]

Accepts directories (scanned recursively) and .zip archives. Prints:
  - the KA1 size-formula census
  - the 7-bit census
  - the KAA container/pointer-table model applied to every bank
  - byte-identity between KAA patch payloads and KA1 files
"""

from __future__ import annotations

import collections
import pathlib
import struct
import sys
import zipfile

# Patch layout, docs/FORMAT_NOTES.md "Patch payload"
COMMON = 82
SOURCE = 86
ADD_KIT = 806

# KAA container, docs/FORMAT_NOTES.md "KAA - single bank"
SLOTS = 128
PTRS_PER_PATCH = 7
HEADER_BYTES = SLOTS * PTRS_PER_PATCH * 4 + 4  # 0x0E04
DATA_BYTES = 0x20000
KAA_SIZE = HEADER_BYTES + DATA_BYTES  # 134660

NAME_OFF = 0x28
NAME_LEN = 8
SOURCE_COUNT_OFF = 0x33


def patch_size(sources: int, adds: int) -> int:
    return COMMON + SOURCE * sources + ADD_KIT * adds


def ka1_is_structurally_valid(data: bytes) -> bool:
    if len(data) <= SOURCE_COUNT_OFF:
        return False
    s = data[SOURCE_COUNT_OFF]
    rest = len(data) - COMMON - SOURCE * s
    return 1 <= s <= 6 and rest >= 0 and rest % ADD_KIT == 0 and rest // ADD_KIT <= s


def kaa_patches(data: bytes):
    """Yield (slot, payload) for every occupied slot of a KAA bank."""
    if len(data) != KAA_SIZE:
        return
    table = [
        struct.unpack_from(">I", data, i * 4)[0]
        for i in range(SLOTS * PTRS_PER_PATCH + 1)
    ]
    rows = [(i, table[i * PTRS_PER_PATCH:(i + 1) * PTRS_PER_PATCH]) for i in range(SLOTS)]
    occupied = [(i, r) for i, r in rows if r[0]]
    if not occupied:
        return
    base = min(v for _, r in occupied for v in r if v)
    for slot, row in occupied:
        offset = HEADER_BYTES + row[0] - base
        if not 0 <= offset < len(data):
            continue
        sources = data[offset + SOURCE_COUNT_OFF]
        adds = sum(1 for v in row[1:] if v)
        if not 1 <= sources <= 6:
            continue
        size = patch_size(sources, adds)
        if offset + size > len(data):
            continue
        yield slot, data[offset:offset + size]


def collect(paths):
    """Yield (display_name, bytes) for every K5000 file under the given paths."""
    wanted = (".ka1", ".kaa")
    for raw in paths:
        p = pathlib.Path(raw)
        if p.is_dir():
            for f in p.rglob("*"):
                if f.is_file() and f.suffix.lower() in wanted:
                    yield f.name, f.read_bytes()
        elif p.suffix.lower() == ".zip":
            with zipfile.ZipFile(p) as z:  # read-only
                for e in z.infolist():
                    if e.file_size and pathlib.Path(e.filename).suffix.lower() in wanted:
                        yield pathlib.Path(e.filename).name, z.read(e)
        elif p.suffix.lower() in wanted:
            yield p.name, p.read_bytes()


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2

    ka1s, kaas = [], []
    for name, data in collect(argv[1:]):
        (kaas if name.lower().endswith(".kaa") else ka1s).append((name, data))

    print(f"corpus: {len(ka1s)} KA1, {len(kaas)} KAA\n")

    # --- KA1 size formula and 7-bit census -------------------------------
    valid, invalid, seven_bit = [], [], 0
    composition = collections.Counter()
    for name, data in ka1s:
        if ka1_is_structurally_valid(data):
            valid.append((name, data))
            s = data[SOURCE_COUNT_OFF]
            composition[(s, (len(data) - COMMON - SOURCE * s) // ADD_KIT)] += 1
            if max(data) < 0x80:
                seven_bit += 1
        else:
            invalid.append((name, len(data), data[SOURCE_COUNT_OFF] if len(data) > SOURCE_COUNT_OFF else None))

    print(f"KA1 matching size = {COMMON} + {SOURCE}*S + {ADD_KIT}*A : {len(valid)} of {len(ka1s)}")
    print(f"KA1 entirely 7-bit                                     : {seven_bit} of {len(valid)}")
    print(f"(sources, adds) combinations seen                      : {len(composition)}")
    if invalid:
        print(f"\nstructurally invalid ({len(invalid)}), first 10:")
        for name, size, s in invalid[:10]:
            print(f"  {name:20s} size={size:6d} byte[0x33]={s}")

    # --- KAA container model ---------------------------------------------
    print(f"\nKAA banks of exactly {KAA_SIZE} bytes: "
          f"{sum(1 for _, d in kaas if len(d) == KAA_SIZE)} of {len(kaas)}")

    index = collections.defaultdict(list)
    for name, data in valid:
        index[(data[NAME_OFF:NAME_OFF + NAME_LEN], len(data))].append(data)

    extracted = identical = differing = orphan = 0
    printable = 0
    for _, data in kaas:
        for _slot, payload in kaa_patches(data):
            extracted += 1
            name8 = payload[NAME_OFF:NAME_OFF + NAME_LEN]
            if all(0x20 <= c <= 0x7F for c in name8):
                printable += 1
            key = (name8, len(payload))
            if key not in index:
                orphan += 1
            elif any(candidate == payload for candidate in index[key]):
                identical += 1
            else:
                differing += 1

    print(f"patches extracted under the pointer-table model : {extracted}")
    print(f"  with a well-formed 8-char name                : {printable}")
    print(f"  byte-identical to a same-name same-size KA1   : {identical}")
    print(f"  same name and size but different content      : {differing}")
    print(f"  no counterpart KA1 in the corpus              : {orphan}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
