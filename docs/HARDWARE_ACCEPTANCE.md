# Hardware acceptance log

Automated tests are necessary but not sufficient (specification §38). Every row
records an **actual observed result** on the real Kawai K5000S with the installed
Gotek/FlashFloppy. Nothing is marked passed on the basis of assumption.

## Environment

| Item | Value |
| --- | --- |
| Instrument | Kawai K5000S |
| Firmware version | _to record_ |
| Gotek firmware | FlashFloppy _version to record_ |
| Transfer medium | USB stick, _model to record_ |
| Application version | _to record per test session_ |

## Log

| Date | Item | Artefact | Expected | Observed | Result |
| --- | --- | --- | --- | --- | --- |
| — | — | — | — | — | not started |

## Required items (specification §38)

**KA1** — generated KA1 loads successfully on the K5000S.

**KAA** — generated KAA loads successfully; verify names, slot order, several ADD
presets, and mixed ADD/PCM presets.

**Multi** — generated Multi data loads correctly and referenced Singles resolve
correctly.

**IMG / Gotek** — the generated image:

1. is recognized by FlashFloppy
2. can be selected on the installed Gotek
3. is readable by the K5000S
4. lists the expected files
5. loads the generated KAA successfully
6. loads the generated KCA successfully where applicable
7. can receive a save operation from the K5000
8. can subsequently be reopened by this application
