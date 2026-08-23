# Contributing

## Two agents, one working tree

This project is developed by two coding agents sharing the same `Playground`
working directory on the same machine:

| Agent | Role |
| --- | --- |
| **Claude Code** | Primary architecture and implementation (specification §40) |
| **Codex (ChatGPT)** | Independent review, adversarial testing, regression verification (§41) |

Because both agents work in the same checkout, coordination is not optional:

- **Never let both agents redesign the same subsystem at the same time** (§0).
  One subsystem has one owner at a time.
- Before starting a subsystem, claim it in `docs/OPEN_QUESTIONS.md` or a commit
  message. Before touching a file, check `git status` — an unexpected dirty tree
  means the other agent is mid-edit.
- Commit at coherent checkpoints so the other agent has a stable base to review.
  Do not leave the tree broken between sessions.
- Review findings are recorded, not applied silently by the reviewer. Codex
  reports; Claude Code fixes; Codex re-verifies.

## Implementation loop (specification §40)

For each subsystem, in this order:

1. Document the understanding first (`docs/FORMAT_NOTES.md`).
2. Implement the smallest coherent unit.
3. Add tests immediately.
4. Run the tests.
5. Inspect failures — never weaken a test to make it pass.
6. Commit a clear checkpoint.

Do not implement multiple binary formats simultaneously. The order is:

```
project/toolchain → GUI prototype → KA1 → KAA → Bank A/D workspace
→ Single SysEx → KC1 → KCA → Multi workspace/references → FAT12
→ IMG GUI → Deep Extract → integration → release hardening
```

## Review priorities (specification §41)

Do not trust comments, README claims, completion reports or test names. Inspect
the implementation. Concentrate on: binary offsets, lengths, endian assumptions,
KAA pointer tables, reserved-byte preservation, SysEx framing, checksums, Multi
references, FAT12 12-bit cluster encoding, FAT chain corruption, root directory
handling, partial writes, accidental modification of source data, Undo/Redo
bypasses, inconsistent drag-and-drop versus keyboard behaviour, GUI-thread
blocking.

Findings are classified **P0** (data loss/corruption or fundamentally invalid
output), **P1** (major functional failure), **P2** (significant non-destructive
defect), **P3** (minor). Every P0/P1 needs a source location, trigger,
explanation, reproducible test and recommended fix — and a regression test before
it counts as fixed.

## Non-negotiables

- Imported files are read-only source material. Opening or importing must never
  modify them. This includes the external reference corpus.
- Unknown and reserved bytes are preserved, never normalized.
- Malformed input is reported, never auto-repaired.
- Output is written atomically: temp file → flush/close → validate → rename.
- No serializer ships without its parser and a round-trip test (§25).
- No format is described as supported below *Verified* confidence (§6, §30).

## Code style

`.clang-format` and `.editorconfig` are authoritative. C++20, MSVC `/W4
/permissive-`. Core and format libraries must not depend on Qt GUI types.

## Commits

Small, coherent, and describing the *why*. Reference the specification section
where relevant (`§8 capacity`, `§25 output validation`).
