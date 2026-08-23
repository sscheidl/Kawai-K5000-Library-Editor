#!/usr/bin/env python3
"""Generate testdata/REFERENCE_MANIFEST.csv from the private corpus.

The manifest records what each corpus file is *expected* to parse as, keyed by
an anonymous fixture id. It carries metadata only - never payload bytes and
never original filenames - so it is safe to commit while the corpus itself is
not (docs/OPEN_QUESTIONS.md Q2).

The expectations come from the independent Python model in k5000_probe_lib, so
the C++ parser is checked against something that was not derived from it.

The corpus is opened READ-ONLY.

Usage:
    python tools/research/make_manifest.py <corpus-dir-or-zip> [...] \\
        --out testdata/REFERENCE_MANIFEST.csv
"""

from __future__ import annotations

import argparse
import csv
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).parent))

import k5000_probe_lib as k5  # noqa: E402

FIELDS = [
    "fixture_id",
    "format",
    "size",
    "sha256",
    "expected_state",
    "expected_source_count",
    "expected_add_sources",
    "expected_pcm_sources",
    "expected_structural_size",
    "expected_slot_count",
    "note",
]


def classify_ka1(data: bytes) -> dict:
    """Expected parse outcome for one KA1 file, in the parser's own vocabulary."""
    row = {
        "format": "KA1",
        "expected_source_count": "",
        "expected_add_sources": "",
        "expected_pcm_sources": "",
        "expected_structural_size": "",
        "expected_slot_count": "",
        "note": "",
    }

    st = k5.analyse(data)
    if not st.ok:
        row["expected_state"] = "Truncated" if "truncated" in st.reason else "Malformed"
        row["note"] = st.reason
        if st.sources:
            row["expected_source_count"] = st.sources
        return row

    row["expected_source_count"] = st.sources
    row["expected_add_sources"] = st.add_sources
    row["expected_pcm_sources"] = st.sources - st.add_sources
    row["expected_structural_size"] = st.size

    # The file length independently fixes the ADD count. When the two readings
    # disagree the file is ambiguous, and the parser must say so rather than
    # pick one.
    rest = len(data) - k5.COMMON_SIZE - k5.SOURCE_SIZE * st.sources
    length_adds = rest // k5.ADD_KIT_SIZE if rest >= 0 and rest % k5.ADD_KIT_SIZE == 0 else None

    if st.size != len(data):
        if length_adds is not None and length_adds <= st.sources:
            row["expected_state"] = "StructurallyAmbiguous"
            row["note"] = f"length implies {length_adds} ADD sources, descriptors imply {st.add_sources}"
        else:
            row["expected_state"] = "Malformed"
            row["note"] = f"{len(data) - st.size} trailing bytes"
        return row

    kit_start = k5.COMMON_SIZE + k5.SOURCE_SIZE * st.sources
    bad_kits = [
        k
        for k in range(st.add_sources)
        if data[kit_start + k * k5.ADD_KIT_SIZE]
        != k5.add_kit_checksum(data[kit_start + k * k5.ADD_KIT_SIZE:
                                    kit_start + (k + 1) * k5.ADD_KIT_SIZE])
    ]

    if not st.checksum_ok:
        row["expected_state"] = "ChecksumMismatch"
        row["note"] = (f"patch checksum stored 0x{st.checksum_stored:02X} "
                       f"calculated 0x{st.checksum_calculated:02X}")
    elif bad_kits:
        row["expected_state"] = "ChecksumMismatch"
        row["note"] = "ADD wave kit " + ",".join(str(k + 1) for k in bad_kits) + " checksum failed"
    elif any(w > k5.ADD_WAVE_KIT for w in st.waves):
        row["expected_state"] = "UnsupportedVariant"
        row["note"] = "wave kit number out of range"
    else:
        row["expected_state"] = "Valid"

    return row


def classify_kaa(data: bytes) -> dict:
    row = {
        "format": "KAA",
        "expected_source_count": "",
        "expected_add_sources": "",
        "expected_pcm_sources": "",
        "expected_structural_size": "",
        "expected_slot_count": "",
        "note": "",
    }

    if len(data) != k5.KAA_FILE_SIZE:
        row["expected_state"] = "Malformed"
        row["note"] = f"expected {k5.KAA_FILE_SIZE} bytes"
        return row

    rows = k5.kaa_pointer_table(data)
    occupied = [(i, r) for i, r in enumerate(rows) if r[0]]
    base = k5.kaa_base(rows)
    if base is None:
        row["expected_state"] = "Malformed"
        row["note"] = "no occupied slot"
        return row

    # The bank's state is the worst of everything observed while reading it,
    # including the per-patch diagnostics. This must mirror the parser's own
    # aggregation exactly, or the manifest cross-check compares two different
    # questions. Severity order, worst first:
    #   Malformed > Truncated > StructurallyAmbiguous > UnsupportedVariant
    #   > ChecksumMismatch > Valid
    stale = 0
    patch_checksum_failures = 0
    kit_checksum_failures = 0
    wave_out_of_range = 0
    unreadable = 0

    for _slot, ptr_row in occupied:
        offset = k5.kaa_offset(ptr_row[0], base)
        st = k5.analyse(data[offset:offset + k5.ADD_KIT_SIZE * 7], limit=len(data) - offset)
        if not st.ok:
            unreadable += 1
            continue
        if not st.checksum_ok:
            patch_checksum_failures += 1
        kit_start = offset + k5.COMMON_SIZE + k5.SOURCE_SIZE * st.sources
        for k in range(st.add_sources):
            kit = data[kit_start + k * k5.ADD_KIT_SIZE:kit_start + (k + 1) * k5.ADD_KIT_SIZE]
            if kit[0] != k5.add_kit_checksum(kit):
                kit_checksum_failures += 1
        if any(w > k5.ADD_WAVE_KIT for w in st.waves):
            wave_out_of_range += 1
        if any(ptr_row[1 + i] for i in range(st.sources, 6)):
            stale += 1

    row["expected_slot_count"] = len(occupied)
    notes = []
    if stale:
        notes.append(f"{stale} slot(s) with a stale ADD kit pointer")
    if wave_out_of_range:
        notes.append(f"{wave_out_of_range} slot(s) with a wave kit number out of range")
    if patch_checksum_failures:
        notes.append(f"{patch_checksum_failures} slot(s) failing the patch checksum")
    if kit_checksum_failures:
        notes.append(f"{kit_checksum_failures} ADD wave kit checksum failure(s)")
    if unreadable:
        notes.append(f"{unreadable} unreadable slot(s)")
    row["note"] = "; ".join(notes)

    if unreadable:
        row["expected_state"] = "Malformed"
    elif stale or wave_out_of_range:
        row["expected_state"] = "UnsupportedVariant"
    elif patch_checksum_failures or kit_checksum_failures:
        row["expected_state"] = "ChecksumMismatch"
    else:
        row["expected_state"] = "Valid"
    return row


def main(argv) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("paths", nargs="+")
    parser.add_argument("--out", default="testdata/REFERENCE_MANIFEST.csv")
    args = parser.parse_args(argv[1:])

    rows = []
    for item in k5.load(args.paths):
        row = classify_kaa(item.data) if item.name.lower().endswith(".kaa") \
            else classify_ka1(item.data)
        row["fixture_id"] = item.fixture_id
        row["size"] = len(item.data)
        row["sha256"] = item.sha256
        rows.append(row)

    rows.sort(key=lambda r: (r["format"], r["fixture_id"]))

    out = pathlib.Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=FIELDS, lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)

    states = {}
    for row in rows:
        key = (row["format"], row["expected_state"])
        states[key] = states.get(key, 0) + 1

    print(f"wrote {out} with {len(rows)} rows")
    for (fmt, state), count in sorted(states.items()):
        print(f"  {fmt} {state:24s} {count}")
    print("\nNo payload bytes and no original filenames are recorded.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
