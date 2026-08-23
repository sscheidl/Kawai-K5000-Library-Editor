"""Shared model and corpus loading for the Phase A research probes.

This is *research tooling*, not production code and not a test. The production
parser lives in `src/formats/`. Both are kept deliberately independent so that
one can be used to check the other (specification section 41).

The reference corpus is READ-ONLY. Nothing here opens a corpus path for writing.

Structural model, as recorded in docs/FORMAT_NOTES.md:

    patch = common(82) + source_descriptor(86) * S + add_wave_kit(806) * A

    S  comes from common offset 0x33
    A  comes from the source descriptors: a source is ADD iff its wave kit
       number is 512. The KAA pointer table must NOT be used for this - it
       carries stale entries (see kaa_pointer_probe.py).
"""

from __future__ import annotations

import hashlib
import pathlib
import struct
import zipfile
from dataclasses import dataclass, field

# --- Patch layout -----------------------------------------------------------
COMMON_SIZE = 82
SOURCE_SIZE = 86
ADD_KIT_SIZE = 806

CHECKSUM_OFF = 0x00
NAME_OFF = 0x28
NAME_LEN = 8
SOURCE_COUNT_OFF = 0x33

# Wave kit number inside a source descriptor, two 7-bit bytes, high first.
WAVE_KIT_OFF = 28
ADD_WAVE_KIT = 512

CHECKSUM_SEED = 0xA5
MAX_SOURCES = 6

# --- KAA container ----------------------------------------------------------
KAA_SLOTS = 128
KAA_PTRS_PER_SLOT = 7
KAA_HEADER_SIZE = KAA_SLOTS * KAA_PTRS_PER_SLOT * 4 + 4  # 0x0E04
KAA_DATA_SIZE = 0x20000
KAA_FILE_SIZE = KAA_HEADER_SIZE + KAA_DATA_SIZE  # 134660


def wave_kit(payload: bytes, source_index: int) -> int:
    """Wave kit number of one source descriptor."""
    base = COMMON_SIZE + SOURCE_SIZE * source_index + WAVE_KIT_OFF
    return (payload[base] << 7) | payload[base + 1]


def expected_checksum(payload: bytes, sources: int) -> int:
    """Kawai Single Tone checksum.

    Covers the common block excluding the checksum byte itself, plus the active
    source descriptors. It does NOT cover the ADD wave kits, which carry their
    own checksum in their first byte.
    """
    end = COMMON_SIZE + SOURCE_SIZE * sources
    return (sum(payload[1:end]) + CHECKSUM_SEED) & 0x7F


def add_kit_checksum(kit: bytes) -> int:
    return (sum(kit[1:]) + CHECKSUM_SEED) & 0x7F


@dataclass
class Structure:
    """Result of a structural traversal of a patch payload."""

    ok: bool
    reason: str = ""
    sources: int = 0
    add_sources: int = 0
    size: int = 0
    waves: list[int] = field(default_factory=list)
    checksum_stored: int = 0
    checksum_calculated: int = 0

    @property
    def checksum_ok(self) -> bool:
        return self.checksum_stored == self.checksum_calculated


def analyse(payload: bytes, limit: int | None = None) -> Structure:
    """Traverse a patch payload structurally. Never reads past `limit`.

    `limit` defaults to len(payload); pass the remaining bytes of a container
    when the payload is embedded.
    """
    avail = len(payload) if limit is None else min(limit, len(payload))

    if avail < COMMON_SIZE:
        return Structure(False, "truncated common block")

    sources = payload[SOURCE_COUNT_OFF]
    if not 1 <= sources <= MAX_SOURCES:
        return Structure(False, f"source count {sources} out of range")

    need_sources = COMMON_SIZE + SOURCE_SIZE * sources
    if avail < need_sources:
        return Structure(False, "truncated source descriptors", sources=sources)

    waves = [wave_kit(payload, i) for i in range(sources)]
    adds = sum(1 for w in waves if w == ADD_WAVE_KIT)
    size = need_sources + ADD_KIT_SIZE * adds
    if avail < size:
        return Structure(False, "truncated ADD wave kits", sources=sources,
                         add_sources=adds, waves=waves)

    return Structure(
        True,
        sources=sources,
        add_sources=adds,
        size=size,
        waves=waves,
        checksum_stored=payload[CHECKSUM_OFF],
        checksum_calculated=expected_checksum(payload, sources),
    )


def kaa_pointer_table(data: bytes) -> list[list[int]]:
    """The 128 x 7 pointer rows. Requires an exactly sized bank."""
    count = KAA_SLOTS * KAA_PTRS_PER_SLOT + 1
    table = [struct.unpack_from(">I", data, i * 4)[0] for i in range(count)]
    return [table[i * KAA_PTRS_PER_SLOT:(i + 1) * KAA_PTRS_PER_SLOT] for i in range(KAA_SLOTS)]


def kaa_end_of_data(data: bytes) -> int:
    return struct.unpack_from(">I", data, KAA_SLOTS * KAA_PTRS_PER_SLOT * 4)[0]


def kaa_base(rows: list[list[int]]) -> int | None:
    values = [v for row in rows for v in row if v]
    return min(values) if values else None


def kaa_offset(pointer: int, base: int) -> int:
    return KAA_HEADER_SIZE + pointer - base


@dataclass
class Item:
    name: str
    data: bytes

    @property
    def sha256(self) -> str:
        return hashlib.sha256(self.data).hexdigest()

    @property
    def fixture_id(self) -> str:
        """Anonymous, stable identifier. Never exposes the original filename."""
        return self.sha256[:16]


def load(paths, suffixes=(".ka1", ".kaa")):
    """Yield Item for every matching file under the given paths. Read-only.

    Accepts directories (scanned recursively), .zip archives, and single files.
    """
    suffixes = tuple(s.lower() for s in suffixes)
    for raw in paths:
        p = pathlib.Path(raw)
        if p.is_dir():
            for f in sorted(p.rglob("*")):
                if f.is_file() and f.suffix.lower() in suffixes:
                    yield Item(f.name, f.read_bytes())
        elif p.suffix.lower() == ".zip":
            with zipfile.ZipFile(p) as z:  # read-only
                for e in z.infolist():
                    if e.file_size and pathlib.Path(e.filename).suffix.lower() in suffixes:
                        yield Item(pathlib.PurePath(e.filename).name, z.read(e))
        elif p.suffix.lower() in suffixes:
            yield Item(p.name, p.read_bytes())


def split(items):
    """Partition into (KA1, KAA). Materializes, so a generator is safe here."""
    items = list(items)
    ka1 = [i for i in items if not i.name.lower().endswith(".kaa")]
    kaa = [i for i in items if i.name.lower().endswith(".kaa")]
    return ka1, kaa


def usage(script: str) -> int:
    print(f"usage: python tools/research/{script} <corpus-dir-or-zip> [...]")
    print("The corpus is opened read-only.")
    return 2
