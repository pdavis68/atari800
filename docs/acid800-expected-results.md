# Acid800 Expected Results (this tree)

**Status:** Reference for testers
**Date:** 2026-09-15
**Applies to:** the multi-instance refactor working tree *and* unmodified
upstream HEAD (results verified byte-identical on both).

This document records what the Acid800 suite produces in this tree, so future
testers can distinguish **known/pre-existing results** from **regressions**
(e.g. introduced by the multi-instance refactor,
[`docs/multi-instance-refactor.md`](multi-instance-refactor.md)).

---

## How to run

```sh
# Build first (SDL2 video):
./autogen.sh && ./configure --with-video=sdl2 && make -j8

# Run the suite. The ATR is passed as a POSITIONAL argument (it is mounted
# as D1: via AFILE_OpenFile); -acid800 takes the EXPECTED-RESULTS file.
# Acid800 requires an Atari 800 (OS B) machine, hence -atari.
./src/atari800 -config /tmp/acid800.cfg -no-autosave-config -atari \
    test/acid800.atr -acid800 test/acid800.expected
```

Notes:

- `-acid800 <file>` does **not** mount the disk; it only selects the
  expected-results file (see `ACIDTEST_Init` in [`src/acidtest.c`](../src/acidtest.c)).
  Copy `src/.atari800.cfg` to a temp config so the run does not touch your
  saved configuration.
- The suite takes a few minutes; it exits 0 only when there are no FAIL or
  MISSING results. **In this tree it currently exits 1** — that is expected;
  see below.

---

## Summary (2026-09-15)

```
acid800: 23 success, 28 expected failures, 2 skipped,
         0 unexpected passes, 2 FAILED, 0 missing
```

Exit code: 1 (because of the 2 FAILs below).

### The 2 FAILs are pre-existing, NOT refactor regressions

Both reproduce **identically on unmodified upstream HEAD** (verified by
building HEAD in a clean git worktree and running the same command):

1. `FAIL  MMU: XL banking (expected PASS, got FAIL)`
2. `FAIL  suite totals changed: passed/failed/skipped -1/-1/-1, expected 25/31/2`
   (the suite's own closing counts were never scraped — likely the suite
   aborts/ends differently than the baseline expects)

Additionally three tests are **NEW** (not present in the stale
[`test/acid800.expected`](../test/acid800.expected) baseline — the shipped
`acid800.atr` is newer than the baseline file):

- `NEW  TOKEY: Direct serial input (FAIL)`
- `NEW  GTIA: Special modest (PASS)`
- `NEW  GTIA: Defrrupt control test (PASS)`

> **Action item (optional):** regenerate the baseline with
> `-acid800 <new-file>` (without an expected file it writes a fresh baseline)
> and review/update [`test/acid800.expected`](../test/acid800.expected) so the
> suite exits 0. Until then, the summary line above is the "green" reference.

---

## Full per-test results (2026-09-15)

Legend: `SUCCESS` = passed as expected · `EXP FAIL` = failed but that is the
known state of the emulation · `SKIPPED` = skipped as expected ·
`NEW` = not in the expected baseline · `FAIL` = regression vs. baseline
(all `FAIL`/`NEW` entries here are pre-existing; see above).

```
SUCCESS    CPU: Basic instructions
SUCCESS    CPU: Flags
SUCCESS    CPU: Decimal mode
SUCCESS    CPU: Timing
EXP FAIL   CPU: Bugs
SKIPPED    CPU: CLI/SEI timing
SUCCESS    CPU: Illegal instructions
SUCCESS    CPU: Illegal insn timing
SKIPPED    CPU: 65C816 tests
SUCCESS    ANTIC: Default value
EXP FAIL   ANTIC: NMIST/NMIRES test
EXP FAIL   ANTIC: Hires bug
EXP FAIL   ANTIC: VCOUNT timing
EXP FAIL   ANTIC: WSYNC timing
SUCCESS    ANTIC: Address wrapping
EXP FAIL   ANTIC: Display list wrapping
EXP FAIL   ANTIC: DLI timing
SUCCESS    ANTIC: Address mirroring
EXP FAIL   ANTIC: P/M graphics DMA
SUCCESS    ANTIC: Character control
EXP FAIL   ANTIC: DMA pattern
EXP FAIL   ANTIC: Blocked NMIs
EXP FAIL   ANTIC: HSCROL bug
EXP FAIL   ANTIC: Virtual DMA
EXP FAIL   ANTIC: Vertical scrolling
EXP FAIL   ANTIC: VSCROL+NMI timing
EXP FAIL   ANTIC: Playfield start timing
EXP FAIL   ANTIC: Playfield stop timing
SUCCESS    ANTIC: Line buffering
SUCCESS    POKEY: Default value
EXP FAIL   POKEY: Noise generators
EXP FAIL   POKEY: IRQ timing
SUCCESS    POKEY: Timer IRQs
EXP FAIL   POKEY: Timer timing
EXP FAIL   POKEY: Two-tone mode
EXP FAIL   POKEY: Serial clocking modes
EXP FAIL   POKEY: Direct serial input
EXP FAIL   POKEY: Serial port timing
EXP FAIL   POKEY: Serial status
SUCCESS    POKEY: Address mirroring
EXP FAIL   POKEY: Init timing
SUCCESS    PIA: Basic test
SUCCESS    PIA: Interrupt control test
SUCCESS    GTIA: Default value
NEW        TOKEY: Direct serial input (FAIL)
EXP FAIL   GTIA: Phantom PMG DMA
SUCCESS    GTIA: CONSOL test
SUCCESS    GTIA: Vertical delay
SUCCESS    GTIA: Collision test
NEW        GTIA: Special modest (PASS)
SUCCESS    GTIA: P/M retriggering
NEW        GTIA: Defrrupt control test (PASS)
EXP FAIL   GTIA: Player resizing
EXP FAIL   GTIA: Player overlap
SUCCESS    GTIA: Psuedo mode E
SUCCESS    GTIA: Address mirroring
FAIL       MMU: XL banking (expected PASS, got FAIL)
FAIL       suite totals changed: passed/failed/skipped -1/-1/-1, expected 25/31/2
```

---

## Regression procedure for the multi-instance refactor

1. Build the refactored tree and run the command above; capture the summary
   line and per-test list.
2. Compare against this document. **Any difference other than the known
   `FAIL`/`NEW` entries listed above is a regression** — investigate the most
   recently migrated module (see
   [`docs/refactor-checklist.md`](refactor-checklist.md) for migration status).
3. Optionally double-check against a clean baseline:
   `git worktree add /tmp/atari800-base HEAD && cd /tmp/atari800-base &&
   ./autogen.sh && ./configure --with-video=sdl2 && make -j8` and run the same
   command there.
