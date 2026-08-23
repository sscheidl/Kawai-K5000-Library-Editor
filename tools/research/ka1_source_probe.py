#!/usr/bin/env python3
"""Verify ADD versus PCM source identification against the real corpus.

Hypothesis under test: each 86-byte source descriptor carries a wave kit number
at source-relative offset +28/+29, encoded as two 7-bit bytes, high first. A
source is additive iff that number is 512 (`0x04 0x00`); any other value is PCM.

The independent check is structural: a KA1 file's total length already fixes the
number of ADD wave kits, because

    size = 82 + 86 * S + 806 * A

so A is known from the file size without looking at the descriptors at all. If
the wave-kit reading is right, the two must agree for every file.

Usage: python tools/research/ka1_source_probe.py <corpus-dir-or-zip> [...]
"""

from __future__ import annotations

import collections
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).parent))

import k5000_probe_lib as k5  # noqa: E402


def add_count_from_size(data: bytes, sources: int) -> int | None:
    rest = len(data) - k5.COMMON_SIZE - k5.SOURCE_SIZE * sources
    if rest < 0 or rest % k5.ADD_KIT_SIZE:
        return None
    adds = rest // k5.ADD_KIT_SIZE
    return adds if adds <= sources else None


def main(argv) -> int:
    if len(argv) < 2:
        return k5.usage("ka1_source_probe.py")

    ka1, _ = k5.split(k5.load(argv[1:], suffixes=(".ka1",)))

    analysed = 0
    total_sources = add_sources = pcm_sources = 0
    mixed = 0
    contradictions = []
    skipped = 0
    waves = collections.Counter()
    composition = collections.Counter()

    for item in ka1:
        data = item.data
        if len(data) <= k5.SOURCE_COUNT_OFF:
            skipped += 1
            continue
        s = data[k5.SOURCE_COUNT_OFF]
        if not 1 <= s <= k5.MAX_SOURCES:
            skipped += 1
            continue
        a_size = add_count_from_size(data, s)
        if a_size is None:
            skipped += 1
            continue

        analysed += 1
        w = [k5.wave_kit(data, i) for i in range(s)]
        a_wave = sum(1 for x in w if x == k5.ADD_WAVE_KIT)

        total_sources += s
        add_sources += a_wave
        pcm_sources += s - a_wave
        if 0 < a_wave < s:
            mixed += 1
        for x in w:
            waves[x] += 1
        composition[(s, a_wave)] += 1

        if a_wave != a_size:
            contradictions.append((item, s, a_size, a_wave, w))

    print("ADD/PCM classification by wave kit number at source offset +28/+29")
    print(f"  files analysed          : {analysed}")
    print(f"  files skipped           : {skipped}")
    print(f"  total sources inspected : {total_sources}")
    print(f"  ADD sources             : {add_sources}")
    print(f"  PCM sources             : {pcm_sources}")
    print(f"  mixed ADD/PCM patches   : {mixed}")
    print(f"  structural contradictions: {len(contradictions)}")

    non_add = {w: c for w, c in waves.items() if w != k5.ADD_WAVE_KIT}
    if non_add:
        print(f"\n  non-ADD wave numbers: {sum(non_add.values())} occurrences, "
              f"{len(non_add)} distinct, range {min(non_add)}..{max(non_add)}")
        out_of_range = sorted(w for w in non_add if w > k5.ADD_WAVE_KIT)
        if out_of_range:
            print(f"  values above {k5.ADD_WAVE_KIT} (implausible for a wave kit): {out_of_range}")

    print(f"\n  (S, A) combinations observed: {len(composition)}")

    if contradictions:
        print("\nContradictions - the file length and the descriptors disagree.")
        print("These are structurally ambiguous and must not be resolved by guessing:")
        for item, s, a_size, a_wave, w in contradictions:
            print(f"  fixture {item.fixture_id} size={len(item.data)} S={s} "
                  f"A_from_size={a_size} A_from_waves={a_wave} waves={w}")
            print(f"    sha256={item.sha256}")

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
