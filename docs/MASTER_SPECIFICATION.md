# K5000 Librarian & Gotek Builder V1

You are working on a desktop application for managing sounds, banks, Multi presets, SysEx files and Gotek/FlashFloppy disk images for the **Kawai K5000S/R**.

This project uses two coding agents:

- **Claude Code:** primary architecture and implementation
- **Codex:** independent review, adversarial testing and regression verification

Do not let both agents independently redesign the same subsystem at the same time.

The goal is a reliable **offline librarian, bank builder, format converter and disk-image manager**.

It is explicitly **not a synthesizer parameter editor**.

---

# 1. Target environment

Primary platform:

- Windows 11 x64

Technology:

- C++20
- CMake
- Qt 6 Widgets
- existing installed C++ compiler/toolchain must be preserved
- do not replace the existing C++ development environment with .NET, Java, Electron or another ecosystem

Before modifying the development environment:

1. inspect the existing compiler, CMake and IDE/toolchain,
2. reuse what is already available,
3. add only dependencies actually required by this application,
4. document any additional dependency.

Qt must be used for the desktop GUI unless a severe technical incompatibility with the existing environment is demonstrated.

Prefer Qt Widgets rather than QML.

The UI should look modern and polished but remain a conventional Windows desktop application.

---

# 2. Development phases

Do not begin by writing the complete application.

Proceed in this order:

## Phase A – Research

Verify:

- KA1
- KAA
- KC1
- KCA
- K5000 Single SysEx
- K5000 Multi SysEx
- relevant bank SysEx structures
- K5000 bank memory/capacity rules
- 1.44 MB FAT12 disk structure
- FlashFloppy compatibility requirements

Useful references may include:

- official Kawai documentation
- K5000 MIDI implementation documentation
- Edisyn K5000 implementation
- KSynthLib / k5ktool
- documented historical K5000 file-format research
- KA1toKAA / KAAtoKA1 / kaanalyz behavior
- FlashFloppy documentation

Never guess undocumented binary fields silently.

Record findings and uncertainties in:

`docs/FORMAT_NOTES.md`

---

## Phase B – GUI specification before implementation

Before substantial production code is written, define and document the GUI.

Create:

`docs/UI_SPEC.md`

Provide static mockups or an executable non-functional GUI prototype for review.

The main navigation must contain:

- Singles
- Multis
- Disk Images
- SysEx
- Settings

The main Singles screen should use:

- left: Library / imported presets
- center: Bank A and Bank D
- right: Inspector

Bank A and Bank D should be visible simultaneously at a normal 1920×1080 desktop resolution.

Do not imitate the K5000 LCD or a 1990s synthesizer editor.

Use a clean modern dark desktop design with readable typography and restrained visual decoration.

Only after this UI model is established should full implementation proceed.

---

# 3. Architecture

Separate binary formats, application state and GUI completely.

Suggested structure:

    src/
        core/
        formats/
            ka1/
            kaa/
            kc1/
            kca/
            sysex/
        disk/
            fat12/
        library/
        gui/

    tests/
        unit/
        integration/
        golden/

    testdata/

    docs/

The GUI must not contain binary parsing or serialization code.

Core libraries must be usable without Qt GUI objects.

Provide a small command-line test/diagnostic executable using the same core libraries.

---

# 4. Canonical internal data model

A Single preset may originate from:

- KA1
- an entry in a KAA file
- individual SysEx
- a SysEx bank

All of these must map to one canonical internal Single model while retaining enough source information for diagnostics.

Store at least:

- parsed semantic data
- original/raw payload where appropriate
- preset name
- source composition
- serialized size
- original source file
- source container
- original slot
- validation state

Do the same conceptually for Multi presets.

Unknown and reserved data must be preserved.

Do not normalize data merely because it is not understood.

---

# 5. Critical preservation rule

Parsing and reserialization must never silently alter musical data.

An imported preset that is exported without semantic modification must preserve its payload exactly wherever the source and destination format allow exact preservation.

Only bytes directly required by an explicit operation such as:

- rename
- relocation
- changed Multi reference

may be changed intentionally.

Do not automatically repair malformed files.

Report malformed input instead.

---

# 6. Single preset formats

Version 1 must support:

- KA1 import
- KA1 export
- KAA import
- KAA export
- K5000 Single SysEx import
- K5000 Single SysEx export

Required conversion paths:

    KA1 -> internal Single
    internal Single -> KA1

    KAA -> individual Singles
    Singles -> KAA

    Single SysEx -> internal Single
    internal Single -> Single SysEx

    KAA -> individual KA1 files
    KAA -> individual SysEx files

Support bank SysEx where its exact structure can be verified.

Do not claim support for an unverified SysEx variant.

---

# 7. Bank A and Bank D

Provide two independent Single-bank workspaces:

- Bank A
- Bank D

Each contains up to 128 slots.

Use a visual card/matrix representation.

A card should display at minimum:

- A001 / D001 style slot identifier
- preset name
- compact source information
- compact size/capacity information where useful

Required operations:

- select
- multi-select
- copy
- cut
- paste
- move
- swap
- duplicate
- clear
- rename
- manual reordering
- sort
- drag-and-drop

Moving and swapping must have deterministic semantics.

Never silently overwrite an occupied destination.

When an operation requires replacement, make the action explicit.

---

# 8. Bank capacity

K5000 memory limits must be modeled from verified K5000 behavior.

Do not treat 128 slots as the only capacity constraint.

Research and validate:

- dynamic Single memory use
- additive-source/waveset limitations
- relevant K5000 bank limits

Display capacity status clearly.

Prevent export as "valid K5000 bank" if verified capacity limits are exceeded.

Capacity calculations must be tested against real reference banks and known analysis results where available.

---

# 9. Mass import

Mass import is a mandatory Version 1 feature.

Support:

- selecting many KA1 files
- selecting many SysEx files
- selecting directories
- recursive directory import
- dropping files/directories from Windows Explorer

When multiple Singles are inserted into a bank, support sensible ordering based on:

- explicit source order
- filename
- encoded slot prefix such as A001_
- manual order

Do not silently discard excess presets if a bank cannot accept all items.

Report the items that could not be inserted.

---

# 10. Mass export

Mass export is a mandatory Version 1 feature.

For a KAA bank support:

- export all occupied presets as individual KA1
- export selected presets as individual KA1
- export all occupied presets as individual SysEx
- export selected presets as individual SysEx

Default filename scheme:

    A001_Preset_Name.KA1
    A002_Other_Name.KA1

and equivalent `.syx` names.

Slot prefixes should prevent accidental filename collisions.

Sanitize characters invalid in Windows filenames.

Never overwrite an existing file silently.

---

# 11. Multi support

Version 1 must support:

- KC1
- KCA
- K5000 Multi SysEx where verified

Provide a workspace for up to 64 Multi presets.

Required operations:

- import
- export
- rename
- copy
- cut
- paste
- move
- swap
- clear
- sort
- drag-and-drop
- Undo/Redo

Mass export:

    KCA -> individual KC1 files

and, where validated:

    KCA -> individual Multi SysEx files

Parse and display references from Multi zones/parts to Single-bank slots.

Provide a dependency view conceptually equivalent to:

    Multi -> Part/Zone -> Bank/Slot -> Single

When a referenced Single changes location, detect affected Multis.

Offer to update their references.

Never silently leave references pointing to the wrong sound as the result of an application-generated bank edit.

---

# 12. Windows Explorer drag-and-drop

Explorer integration is a core Version 1 feature.

Support dragging from Windows Explorer into:

## Library

- KA1
- KAA
- KC1
- KCA
- supported SysEx
- IMG
- directories

## Bank A / Bank D

Dropping compatible Singles should insert them into the selected/available bank position.

Dropping a KAA must offer sensible actions such as:

- open as bank
- insert contained presets
- add contained presets to Library

## Multi workspace

Accept compatible Multi files.

## Disk Image workspace

Allow files to be dropped directly into the image.

Directories dropped into the application should be scanned recursively where appropriate.

If practical with Qt/Windows shell integration, also support dragging an internal preset to Windows Explorer and materializing it as an exported KA1 file.

This reverse Explorer drag is desirable but may be classified as SHOULD rather than blocking Version 1 release if Windows shell implementation proves disproportionately complex.

---

# 13. Keyboard shortcuts

The program must behave like a conventional Windows desktop application.

Mandatory shortcuts:

    Ctrl+C        Copy
    Ctrl+X        Cut
    Ctrl+V        Paste
    Ctrl+Z        Undo
    Ctrl+Y        Redo
    Ctrl+Shift+Z  alternate Redo
    Ctrl+A        Select all
    Ctrl+O        Open
    Ctrl+S        Save
    Ctrl+Shift+S  Save As
    Ctrl+F        Find/Search
    F2            Rename
    Delete        Clear/Remove selected workspace entries
    Esc           Cancel current transient operation

Use normal:

- Ctrl-click for discontinuous selection
- Shift-click for range selection

Cut must be safe.

The source must not be destroyed merely by pressing Ctrl+X.

Removal of the source occurs only as part of a successful paste/move transaction.

---

# 14. Mouse interaction

Mouse-wheel behavior must remain predictable.

Default behavior:

- Wheel: normal scrolling
- Shift + Wheel: horizontal scrolling where applicable
- Ctrl + Wheel: adjust Bank card size/information density

Provide several card-density levels, for example:

- Compact
- Normal
- Detailed

If Windows exposes mouse Back/Forward buttons:

- Mouse 4 = navigation history Back
- Mouse 5 = navigation history Forward

Do not assign potentially destructive actions to auxiliary mouse buttons.

Do not use ordinary mouse-wheel movement to modify stored data.

Middle mouse button remains unassigned in Version 1.

Reserve it for possible Audition functionality in Version 1.5.

---

# 15. Undo / Redo

Workspace modifications must use a transactional command system.

Undoable actions include at minimum:

- rename
- move
- swap
- copy/paste
- cut/paste
- clear
- sorting
- bulk insertion
- bulk removal
- Multi-reference updates

Undo/Redo must work consistently regardless of whether the original action came from:

- keyboard
- context menu
- toolbar
- drag-and-drop

---

# 16. Disk image / Gotek support

Disk-image functionality is a core feature.

Version 1 must support standard raw:

**1.44 MB IBM-PC FAT12 `.IMG`**

suitable for use with FlashFloppy/Gotek and the Kawai K5000.

Required operations:

- create blank image
- open image
- parse BPB/FAT/directory structure
- display files
- display used/free space
- add file
- replace file
- rename file
- remove file
- extract file
- extract all
- save image
- Save As
- validate image

Support appropriate DOS/K5000 filename restrictions.

Never alter a K5000 payload simply because it is inserted into an image.

---

# 17. Build IMG from workspace

Provide a high-level operation:

**Create Disk Image from Current Workspace**

It should allow the user to add current:

- Bank A KAA
- Bank D KAA
- Multi KCA
- existing KRA files
- other compatible K5000 files

and create a ready-to-use IMG.

This workflow should require as few manual intermediate steps as practical.

---

# 18. IMG extraction

Two extraction modes are mandatory.

## Extract All Files

Perform byte-identical extraction of all files stored in the FAT12 image.

Example:

    IMAGE.IMG
        -> BANKA.KAA
        -> BANKD.KAA
        -> MULTI.KCA
        -> ARPS.KRA

## Deep Extract

Recognize supported K5000 container formats inside the image and optionally continue extracting them.

Examples:

    IMG
      -> KAA
          -> individual KA1

    IMG
      -> KAA
          -> individual Single SYX

    IMG
      -> KCA
          -> individual KC1

    IMG
      -> KCA
          -> individual Multi SYX
             if verified

The user must be able to choose which generated representations are wanted.

Original container files may optionally be retained alongside extracted presets.

---

# 19. IMG round-trip

IMG support must be bidirectional.

The application must be able to:

    open IMG
    extract K5000 files
    modify/rebuild content
    save IMG
    reopen saved IMG

without filesystem corruption.

Inserted files must extract byte-identically unless they were explicitly modified.

---

# 20. Workspace/project persistence

Allow the user to save and reopen application workspace state.

At minimum preserve:

- Bank A
- Bank D
- Multi workspace
- Library references where practical
- currently configured disk-image project state

Do not embed or duplicate huge source libraries unnecessarily.

Handle missing external source files gracefully.

---

# 21. No deduplication subsystem in Version 1

Do NOT build a large duplicate-management feature.

It is outside Version 1 scope.

Do not:

- automatically delete duplicate sounds
- rename duplicate sounds
- merge duplicate sounds
- attempt acoustic/similarity matching

A trivial exact-match warning may be implemented if it falls out naturally from internal comparison, but it must not become a separate subsystem and is not a release criterion.

---

# 22. Explicitly out of scope for Version 1

Do not implement:

- full K5000 synthesis editor
- harmonic/additive parameter editor
- automatic sound categorization
- AI classification
- cloud services
- online database
- KRA graphical editor
- ME-1 E/F support
- automatic duplicate cleanup
- direct MIDI device communication
- live preset audition
- automatic K5000 hardware discovery

These features must not delay Version 1.

---

# 23. Version 1.5 architectural preparation

Although MIDI is not implemented in V1, do not design the internal model in a way that prevents it.

Version 1.5 may add:

- MIDI input/output device selection
- receive Single
- send Single
- receive Bank A/D
- send Bank A/D
- receive/send Multi
- SysEx transfer queue
- configurable transfer timing
- temporary preset audition
- Space = Audition
- Middle Mouse = Audition
- optional Auto Audition while browsing

No Version 1 acceptance criterion depends on this functionality.

---

# 24. File safety

Imported files are source material.

Opening or importing a file must never modify it.

"Remove from workspace" and "delete physical file" are different actions.

Version 1 does not need physical file deletion from the source library.

Prefer not to implement source-file deletion at all unless there is a compelling reason.

Output files must be written atomically where practical:

1. write temporary output
2. flush/close
3. validate
4. rename/replace final destination

Never leave a partially written output presented as successful.

---

# 25. Mandatory output validation

Every serializer must have a corresponding parser.

For generated KA1/KAA/KC1/KCA/SysEx:

    model
    -> serialize
    -> parse generated result
    -> validate
    -> compare expected semantic contents

For generated IMG:

    image model
    -> serialize
    -> parse image again
    -> validate FAT12
    -> enumerate files
    -> verify file contents

Do not display "Export successful" before this validation has completed.

---

# 26. Testing

Use automated unit, integration and golden-file tests.

Do not rely only on synthetic fixtures.

Maintain reference material under `testdata/`.

Reference files supplied by the user must never be modified by tests.

---

# 27. KA1 tests

Test at minimum:

- valid files
- truncated files
- malformed files
- parse/export round trip
- rename
- KA1 -> SYX -> KA1 semantic preservation

---

# 28. KAA tests

Test:

- valid bank parsing
- slot table/pointer parsing
- extract every occupied preset
- mass export
- rebuild bank
- slot order
- empty slots
- maximum-slot scenarios
- capacity calculation
- corrupt offsets
- overlapping offsets
- out-of-range offsets
- generated KAA reparses successfully

---

# 29. Multi tests

Test:

- KC1 parse/export
- KCA parse
- KCA -> individual KC1
- KCA rebuild
- slot order
- references
- reference updates after Single moves
- Undo of reference updates
- malformed input

---

# 30. SysEx tests

Test every implemented SysEx variant.

Verify:

- manufacturer/model identifiers
- message framing
- length
- bank/slot metadata
- checksums if applicable
- malformed/truncated messages
- round trips
- KA1/SysEx semantic equivalence

Never claim support for an untested variant.

---

# 31. FAT12 / IMG tests

Test:

- exact raw image size
- valid BPB
- both FAT copies
- root-directory handling
- FAT12 cluster encoding
- cluster-chain traversal
- empty files if applicable
- multi-cluster files
- fragmented files
- file replacement
- deletion
- rename
- free-space calculation
- full disk behavior
- invalid cluster chains
- malformed FAT
- malformed directory entries

Generated images must also be validated using an independent FAT implementation/tool where feasible.

---

# 32. Deep Extract tests

Create tests equivalent to:

    IMG -> KAA -> 128/occupied KA1
    IMG -> KAA -> individual SYX
    IMG -> KCA -> individual KC1

Compare resulting logical contents against direct extraction of the same original containers.

The extra IMG layer must not alter preset payloads.

---

# 33. UI acceptance tests

Manually and, where feasible, automatically verify:

- Explorer file drop
- Explorer directory drop
- internal drag-and-drop
- multi-selection
- Ctrl+C/V/X
- Ctrl+Z/Y
- F2
- Delete
- search focus
- card zoom via Ctrl+Wheel
- Mouse Back/Forward
- Bank A/D simultaneous display
- long operations do not freeze the UI

Keyboard operations and drag-and-drop must call the same underlying command logic.

Do not create separate inconsistent implementations.

---

# 34. Performance

The application should remain responsive with large preset directories.

Directory scanning, parsing and other long operations must not freeze the GUI thread.

Provide:

- progress indication
- cancellation where appropriate
- safe completion behavior

Do not introduce an unnecessary heavyweight database for Version 1.

---

# 35. Error handling

Errors must be understandable.

For malformed input report information such as:

- source filename
- detected format
- offset/location
- expected condition
- actual condition

Do not expose raw C++ exception text as the only user-facing diagnostic.

Do not crash because one bad file is present in a directory containing thousands of valid files.

---

# 36. Logging

Provide diagnostic logging useful during development and hardware testing.

Log relevant operations such as:

- parsing
- validation failures
- conversion
- image writing
- capacity rejection

Do not create enormous logs during normal use.

---

# 37. Documentation

Maintain:

- `README.md`
- `docs/ARCHITECTURE.md`
- `docs/FORMAT_NOTES.md`
- `docs/UI_SPEC.md`
- `docs/TEST_MATRIX.md`
- `docs/KNOWN_LIMITATIONS.md`
- `docs/THIRD_PARTY_NOTICES.md`
- `docs/HARDWARE_ACCEPTANCE.md`

---

# 38. Hardware acceptance

Automated tests are necessary but not sufficient.

Final K5000S hardware tests must include:

## KA1

Generated KA1 loads successfully on the real K5000S.

## KAA

Generated KAA loads successfully.

Check:

- names
- slot order
- several ADD presets
- mixed ADD/PCM presets

## Multi

Generated Multi data loads correctly.

Referenced Singles resolve correctly.

## IMG / Gotek

Generated IMG:

1. is recognized by FlashFloppy,
2. can be selected on the installed Gotek,
3. is readable by the K5000S,
4. lists expected files,
5. loads generated KAA successfully,
6. loads generated KCA successfully where applicable,
7. can receive a save operation from the K5000,
8. can subsequently be reopened by this application.

Record actual results.

Do not mark an item passed based on assumption.

---

# 39. Definition of Done

Version 1 is complete only when:

- clean checkout builds successfully
- Windows x64 application launches
- Qt deployment dependencies are correctly packaged
- automated tests pass
- KA1 works
- KAA works
- KA1/KAA mass import/export works
- implemented SysEx conversions work
- KC1 works
- KCA works
- Multi references work
- A/D editing works
- Explorer drag-and-drop works
- keyboard shortcuts work
- Undo/Redo works
- FAT12 IMG creation works
- IMG editing works
- Extract All works
- Deep Extract works
- generated output reparses successfully
- source files are not modified
- no known P0/P1 data-corruption issue remains
- documentation is current

Features explicitly assigned to V1.5 are not blockers.

---

# 40. Claude Code implementation role

Claude Code is the primary implementation agent.

For each subsystem:

1. document understanding,
2. implement the smallest coherent unit,
3. add tests immediately,
4. run tests,
5. inspect failures rather than weakening tests,
6. commit/record a clear checkpoint.

Do not implement all binary formats simultaneously.

Recommended order:

    project/toolchain
    -> GUI prototype
    -> KA1
    -> KAA
    -> Bank A/D workspace
    -> Single SysEx
    -> KC1
    -> KCA
    -> Multi workspace/references
    -> FAT12
    -> IMG GUI
    -> Deep Extract
    -> integration
    -> release hardening

---

# 41. Codex independent audit role

Codex acts as an independent reviewer after coherent milestones.

Do not trust:

- comments
- README claims
- completion reports
- test names

Inspect the implementation itself.

Concentrate particularly on:

- binary offsets
- lengths
- endian assumptions
- KAA pointer tables
- reserved-byte preservation
- SysEx framing
- checksums
- Multi references
- FAT12 12-bit cluster encoding
- FAT chain corruption
- root directory handling
- partial writes
- accidental modification/deletion of source data
- Undo/Redo bypasses
- inconsistent drag/drop versus keyboard behavior
- GUI-thread blocking

Classify findings:

- P0 — data loss/corruption or fundamentally invalid output
- P1 — major functional failure
- P2 — significant non-destructive defect
- P3 — minor defect

For every P0/P1 provide:

- source file/function
- trigger
- explanation
- reproducible test
- recommended fix

Add a regression test before considering a confirmed P0/P1 fixed.

---

# 42. Final adversarial review

Before declaring Version 1 finished, Codex must perform an adversarial repository-wide audit.

Claude Code then fixes confirmed issues.

Codex reruns:

- relevant regression tests
- complete automated test suite
- build
- output validators

Final verdict must be exactly one of:

- PASS
- PASS WITH LIMITATIONS
- FAIL

A PASS is forbidden while a mandatory V1 format or operation is supported only by assumption.

---

# 43. Final acceptance matrix

The final report must explicitly contain rows for:

- KA1 import
- KA1 export
- KAA import
- KAA export
- KAA -> KA1 mass export
- KAA -> SYX mass export
- KA1/SYX mass import
- Bank A editing
- Bank D editing
- bank capacity validation
- KC1
- KCA
- KCA mass export
- Multi reference management
- Single SysEx
- implemented bank SysEx
- implemented Multi SysEx
- Windows Explorer drag-in
- internal drag-and-drop
- drag-out to Explorer if implemented
- keyboard shortcuts
- mouse-wheel behavior
- Mouse Back/Forward
- Undo/Redo
- FAT12 reading
- FAT12 writing
- IMG opening
- IMG creation
- IMG modification
- IMG Extract All
- IMG Deep Extract
- workspace persistence
- source-file safety
- hardware KA1 validation
- hardware KAA validation
- hardware KCA validation
- Gotek IMG validation

For every row report:

- Implemented
- Automated test
- Golden/reference-file test
- Real-hardware test
- Remaining risk

Do not hide incomplete validation behind a general "works" statement.

The overriding priority of this project is:

**Preserve user data, generate genuinely valid K5000 files and images, and make bank management fast and pleasant. Reliability is more important than adding additional features.**