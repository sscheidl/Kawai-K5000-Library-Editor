#!/usr/bin/env python3
"""Determine and verify the KAA pointer-to-file-position mapping.

Candidate mapping:

    base        = minimum non-zero pointer in the whole table
    file_offset = 0x0E04 + (pointer - base)

A base chosen to make one bank work proves nothing, so this probe validates the
mapping the hard way: it parses the patch at every computed offset and checks
its Kawai checksum. A wrong base shifts every patch and destroys every checksum,
so a full pass across independent banks is strong evidence.

The probe also tests whether the ADD-kit pointers in the table can be trusted to
determine how many ADD wave kits a patch has. They cannot - see the "stale
pointer" section of the output - which is why patch length must be derived from
the patch's own structure.

Usage: python tools/research/kaa_pointer_probe.py <corpus-dir-or-zip> [...]
"""

from __future__ import annotations

import collections
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).parent))

import k5000_probe_lib as k5  # noqa: E402


def main(argv) -> int:
    if len(argv) < 2:
        return k5.usage("kaa_pointer_probe.py")

    _, banks = k5.split(k5.load(argv[1:], suffixes=(".kaa",)))

    wrong_size = [b for b in banks if len(b.data) != k5.KAA_FILE_SIZE]
    banks = [b for b in banks if len(b.data) == k5.KAA_FILE_SIZE]

    print(f"banks loaded              : {len(banks) + len(wrong_size)}")
    print(f"  exactly {k5.KAA_FILE_SIZE} bytes : {len(banks)}")
    print(f"  other sizes             : {len(wrong_size)}")

    slots = ck_ok = ck_bad = struct_bad = oob = 0
    base_is_patch_ptr = 0
    banks_clean = 0
    stale_beyond_sources = 0
    ptr_wave_disagree = 0
    used_pct = []
    fragmented_banks = collections.Counter()

    for item in banks:
        data = item.data
        rows = k5.kaa_pointer_table(data)
        occupied = [(i, r) for i, r in enumerate(rows) if r[0]]
        if not occupied:
            continue
        base = k5.kaa_base(rows)
        tail = k5.kaa_end_of_data(data)

        # Does the overall minimum coincide with the lowest patch pointer?
        if min(r[0] for _, r in occupied) == base:
            base_is_patch_ptr += 1

        used_pct.append((tail - base) / k5.KAA_DATA_SIZE * 100.0)

        bank_ok = True
        spans = []
        for slot, row in occupied:
            slots += 1
            offset = k5.kaa_offset(row[0], base)
            if not k5.KAA_HEADER_SIZE <= offset < k5.KAA_HEADER_SIZE + k5.KAA_DATA_SIZE:
                oob += 1
                bank_ok = False
                continue

            limit = len(data) - offset
            st = k5.analyse(data[offset:offset + 8192], limit=limit)
            if not st.ok:
                struct_bad += 1
                bank_ok = False
                continue

            if st.checksum_ok:
                ck_ok += 1
            else:
                ck_bad += 1
                bank_ok = False

            spans.append((offset, st.size))

            if any(row[1 + i] for i in range(st.sources, 6)):
                stale_beyond_sources += 1
            if sum(1 for v in row[1:] if v) != st.add_sources:
                ptr_wave_disagree += 1

        if bank_ok:
            banks_clean += 1

        spans.sort()
        gaps = sum(1 for a, b in zip(spans, spans[1:]) if a[0] + a[1] != b[0])
        if gaps:
            fragmented_banks[item.fixture_id] = gaps

    print("\nMapping: file_offset = 0x0E04 + (pointer - min_non_zero_pointer)")
    print(f"  banks where the base equals the lowest patch pointer : {base_is_patch_ptr}/{len(banks)}")
    print(f"  occupied slots                                       : {slots}")
    print(f"    patch checksum OK                                  : {ck_ok}")
    print(f"    patch checksum FAILED                              : {ck_bad}")
    print(f"    structurally rejected                              : {struct_bad}")
    print(f"    offset out of the data region                      : {oob}")
    print(f"  banks fully clean                                    : {banks_clean}/{len(banks)}")
    if used_pct:
        print(f"  data-region usage: min {min(used_pct):.1f}%  max {max(used_pct):.1f}%")

    print("\nAre the ADD-kit pointers usable for sizing?  No.")
    print(f"  slots with an ADD-kit pointer set beyond the source count : {stale_beyond_sources}")
    print(f"  slots where the pointer count disagrees with the wave-kit")
    print(f"  classification                                            : {ptr_wave_disagree}")
    print("  These are stale entries left by a larger patch that previously")
    print("  occupied the slot. Patch length must come from the patch structure.")

    print(f"\nFragmented banks (a patch does not abut its neighbour): {len(fragmented_banks)}")
    for fid, gaps in fragmented_banks.most_common(8):
        print(f"  fixture {fid}: {gaps} gap(s)")
    print("  This is why `size = next_pointer - pointer` is forbidden.")

    return 0 if (ck_bad == 0 and struct_bad == 0 and oob == 0) else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
