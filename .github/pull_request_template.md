## What changed

<!-- One paragraph. Reference the specification section, e.g. "§8 capacity". -->

## Subsystem

<!-- ka1 / kaa / kc1 / kca / sysex / fat12 / core / library / gui / cli / docs / build -->

## Data-safety checklist

- [ ] No source file is modified, moved or deleted by this change
- [ ] Unknown and reserved bytes are preserved verbatim
- [ ] Malformed input is reported, not auto-repaired
- [ ] Output is written atomically (temp -> validate -> rename)
- [ ] Every new serializer has a matching parser and a round-trip test (§25)
- [ ] No format is claimed as supported above its verified confidence level (§6, §30)

## Tests

- [ ] Unit tests added or updated
- [ ] Golden/reference test added or updated, or explicitly not applicable
- [ ] `ctest` passes locally
- [ ] `docs/TEST_MATRIX.md` updated

## Docs

- [ ] `docs/FORMAT_NOTES.md` updated with new findings and confidence levels
- [ ] `docs/KNOWN_LIMITATIONS.md` still accurate

## Hardware

- [ ] Not applicable
- [ ] Requires hardware verification — recorded in `docs/HARDWARE_ACCEPTANCE.md`
