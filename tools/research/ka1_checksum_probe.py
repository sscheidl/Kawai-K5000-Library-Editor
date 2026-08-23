#!/usr/bin/env python3
"""Verify the Kawai Single Tone checksum rule against the real corpus.

Hypothesis under test (from the Kawai MIDI implementation):

    checksum = ( sum(common excluding the checksum byte)
               + sum(active source descriptors)
               + 0xA5 ) & 0x7F

The important part is the *scope*: the ADD wave kits are excluded. They carry
their own checksum in their first byte, which this probe also checks.

Promotion criterion: every structurally valid corpus file must match. The probe
prints every mismatch with enough detail to reproduce it. Do not weaken the rule
to accommodate an unexplained exception - investigate it.

Usage: python tools/research/ka1_checksum_probe.py <corpus-dir-or-zip> [...]
"""

from __future__ import annotations

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).parent))

import k5000_probe_lib as k5  # noqa: E402


def main(argv) -> int:
    if len(argv) < 2:
        return k5.usage("ka1_checksum_probe.py")

    ka1, _ = k5.split(k5.load(argv[1:], suffixes=(".ka1",)))

    tested = matches = mismatches = skipped = 0
    kit_total = kit_ok = 0
    mismatch_rows = []
    kit_rows = []

    for item in ka1:
        st = k5.analyse(item.data)
        if not st.ok or st.size != len(item.data):
            skipped += 1
            continue

        tested += 1
        if st.checksum_ok:
            matches += 1
        else:
            mismatches += 1
            mismatch_rows.append((item, st))

        kit_start = k5.COMMON_SIZE + k5.SOURCE_SIZE * st.sources
        for k in range(st.add_sources):
            kit = item.data[kit_start + k * k5.ADD_KIT_SIZE:
                            kit_start + (k + 1) * k5.ADD_KIT_SIZE]
            kit_total += 1
            if kit[0] == k5.add_kit_checksum(kit):
                kit_ok += 1
            else:
                kit_rows.append((item, k, st))

    print("Patch checksum: (sum(common[1:82]) + sum(sources) + 0xA5) & 0x7F")
    print(f"  valid corpus files tested : {tested}")
    print(f"  checksum matches          : {matches}")
    print(f"  checksum mismatches       : {mismatches}")
    print(f"  malformed/skipped         : {skipped}")

    if mismatch_rows:
        print("\nMismatches:")
        for item, st in mismatch_rows:
            print(f"  fixture {item.fixture_id} size={len(item.data)} "
                  f"stored=0x{st.checksum_stored:02X} calculated=0x{st.checksum_calculated:02X} "
                  f"S={st.sources} A={st.add_sources} waves={st.waves}")
            print(f"    sha256={item.sha256}")

    print("\nADD wave kit checksum: kit[0] == (sum(kit[1:]) + 0xA5) & 0x7F")
    print(f"  kits tested : {kit_total}")
    print(f"  matches     : {kit_ok}")
    print(f"  mismatches  : {kit_total - kit_ok}")
    if kit_rows:
        print("\n  kits failing their own checksum (the patch checksum is unaffected,")
        print("  which is itself evidence that the patch checksum excludes the kits):")
        for item, k, st in kit_rows:
            print(f"    fixture {item.fixture_id} kit#{k} S={st.sources} A={st.add_sources} "
                  f"patch_checksum_ok={st.checksum_ok}")

    ok = mismatches == 0
    print("\nConclusion: patch checksum rule",
          "HOLDS for every structurally valid corpus file." if ok else "FAILED - do not promote.")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
