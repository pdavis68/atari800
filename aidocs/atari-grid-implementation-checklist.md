# Atari Grid Implementation Checklist

Working checklist for the grid frontend described in
[`aidocs/atari-grid-design.md`](atari-grid-design.md). Conventions:
`[ ]` = not started, `[x]` = done, `[-]` = in progress.

---

## Phase G1 — Grid state module (core, SDL-independent)

- [ ] Create `src/grid.h` / `src/grid.c` (or `src/sdl/grid.*` if kept
      SDL-only; prefer core so other ports can adopt it later).
- [ ] Define `Grid_state_t`: `instances[GRID_MAX_INSTANCES]`, `count`,
      `selected`, `mode` (`GRID_MODE`/`SELECTED_MODE`), `cols`, `rows`.
- [ ] Implement `GRID_Initialise()` — start with one instance (the default
      instance) selected, 1×1 grid, `GRID_MODE`.
- [ ] Implement `GRID_LayoutFor(count, &cols, &rows)` —
      `cols = ceil(sqrt(count))`, `rows = ceil(count / cols)`; unit-test
      against the design table (N=1..13+).
- [ ] Implement `GRID_AddInstance()` — `Atari800_NewInstance()`, per-instance
      init from the config template, append, recompute layout, select it.
- [ ] Implement `GRID_RemoveInstance()` — refuse when `count == 1`;
      `Atari800_FreeInstance()`, compact array, recompute layout, fix
      selection.
- [ ] Implement `GRID_ToggleMode()` — flip `GRID_MODE`/`SELECTED_MODE`.
- [ ] Implement `GRID_Selected()` accessor returning the selected
      `Atari800_Instance *`.
- [ ] Implement `GRID_MoveSelection(direction)` — grid-coordinate movement
      with wrap/clamp per design §5.3; unit-test the wrap/clamp rules.
- [ ] Implement `GRID_FrameAll()` — call `Atari800_FrameInstance()` for every
      live instance, honouring `GRID_BACKGROUND_REFRESH` for non-selected
      instances.

## Phase G2 — Full per-instance initialisation

- [ ] Resolve the Phase 5 follow-up from
      [`aidocs/refactor-checklist.md`](refactor-checklist.md) §5: a new
      instance must be fully initialised (config template, ROM selection,
      `Atari800_InitialiseMachine` equivalent) — currently only the default
      instance is fully initialized.
- [ ] Add `Atari800_InitInstanceFromTemplate(Atari800_Instance *inst)` (or
      extend `Atari800_NewInstance`) that applies machine type, TV mode, RAM
      size, OS/BASIC ROM selection, and runs machine init + coldstart.
- [ ] Verify a second instance boots to the READY prompt independently
      (smoke test with two instances, both rendering).

## Phase G3 — Key bindings (SDL input)

- [ ] In [`src/sdl/input.c`](../src/sdl/input.c), add grid key handling:
      Shift+F1 = add instance, Shift+F2 = delete selected, Shift+F3 = toggle
      mode, Shift+Ctrl+arrows = move selection.
- [ ] Intercept these keys **before** the Atari key-translation path so they
      are never forwarded to any instance (check Shift+Ctrl+arrow before the
      plain/Shift arrow handling).
- [ ] Add config options `SDL_GRID_NEW_KEY`, `SDL_GRID_DELETE_KEY`,
      `SDL_GRID_TOGGLE_KEY` (defaults Shift+F1/F2/F3) in the SDL config
      section ([`src/sdl/input.c`](../src/sdl/input.c) keymap + `cfg.c`
      strings).
- [ ] Add `GRID_ENABLED` (default 0), `GRID_MAX_INSTANCES` (default 16),
      `GRID_BACKGROUND_REFRESH` (default 1), `GRID_OUTLINE_COLOUR` config
      options; with `GRID_ENABLED = 0` the grid keys are inert and behaviour
      is unchanged.
- [ ] Update [`aidocs/keybindings.md`](keybindings.md) with the new bindings.

## Phase G4 — Input routing to the selected instance

- [ ] Re-point the SDL keyboard handler's writes (`INPUT_key_code`,
      `INPUT_key_shift`, `INPUT_key_consol`, console keys) to
      `GRID_Selected()->input.*` instead of the default-instance aliases.
- [ ] Re-point joystick handling (sticks, triggers, `POKEY_POT_input`) to the
      selected instance.
- [ ] Re-point mouse emulation (deltas, buttons, paddle pots) to the selected
      instance.
- [ ] Ensure UI-menu key (F1) and other emulator-level keys still work while
      grid mode is active (they are host-level, not per-instance).
- [ ] Verify: with two instances, typing affects only the selected one.

## Phase G5 — Audio routing

- [ ] Pin the sound pipeline to the selected instance each frame before
      sample generation (transitional approach per design §7.3, since Sound
      is still process-global — refactor Phase 4.3).
- [ ] On selection change, re-initialise the POKEY sound state from the new
      selected instance's POKEY registers (avoid stale audio state).
- [ ] Verify: audio follows the selection; non-selected instances are silent.
- [ ] Record a follow-up: replace the pinning hack with per-instance sound
      buffers + mixer when the Sound module conversion (§4.3) lands.

## Phase G6 — Grid rendering (SDL video)

- [ ] Create the grid renderer: per-frame composition of all live instances
      into their cells (aspect-preserving scale, centred, 2 px cell inset).
- [ ] Draw the ~5 px light-green outline around the selected instance's cell
      in `GRID_MODE` only.
- [ ] Implement `SELECTED_MODE`: selected instance scaled to the full window,
      no outline (reuse the existing single-instance display path).
- [ ] Hook into the SDL present path: `SDL_VIDEO_DisplayScreen` (or
      equivalent) delegates to `GRID_Display()` when grid mode is active.
- [ ] Handle window sizing on instance add: grow the window to fit the new
      grid (clamped to the desktop work area); never auto-shrink on delete.
- [ ] Handle user window resizes and fullscreen toggles: recompute cell
      geometry.
- [ ] Draw status overlays (fps, disk LED) only for the selected instance.
- [ ] Verify rendering at N = 1, 2, 3, 4, 5, 9 instances (layout table
      coverage).

## Phase G7 — UI / settings per instance

- [ ] Convert the disk-management UI ([`src/ui.c`](../src/ui.c)
      `DiskManagement`) to mount/dismount on the **selected** instance
      (`SIO_Mount_Ctx(selected, ...)` etc.).
- [ ] Convert the cassette UI (`TapeManagement`) to the selected instance.
- [ ] Convert the cartridge UI to the selected instance.
- [ ] Convert the remaining per-machine UI menus (controller, sound, video
      settings that are per-instance) to the selected instance; host-level
      menus (fullscreen, recording) stay global.
- [ ] Route Save state / Load state menu actions to the selected instance
      (per-instance `StateSav_*_Ctx` already exists).
- [ ] Decide and implement screenshot behaviour: selected instance's
      framebuffer (default) vs composed window.
- [ ] Verify: mount a disk in instance 2 only; instance 1 is unaffected.

## Phase G8 — Config integration

- [ ] `CFG_LoadConfig`/`CFG_WriteConfig`: read/write the new `GRID_*` and
      `SDL_GRID_*` options.
- [ ] Optional startup: `GRID_ENABLED = 1` starts in grid mode with the
      configured instance count (load per-instance disk settings later —
      per-instance config persistence is the recorded §4.7 CFG follow-up;
      out of scope for the initial feature).

## Phase G9 — Testing and validation

- [ ] Unit-test `GRID_LayoutFor` against the design table.
- [ ] Unit-test selection movement (wrap/clamp) at grid edges and with empty
      cells (e.g. N=5 in a 3×2 grid).
- [ ] Smoke test: create up to 4 instances, run 60 s, no crashes/CIM; delete
      back to 1; repeat.
- [ ] Verify input isolation: keyboard/joystick only affects the selected
      instance.
- [ ] Verify audio isolation: only the selected instance is audible; audio
      re-inits correctly on selection change.
- [ ] Verify per-instance settings isolation (disks, cassette, cartridge).
- [ ] Verify Shift+F3 toggling preserves instance state (no resets) and
      rendering switches cleanly both ways.
- [ ] Verify `GRID_ENABLED = 0` default build behaves identically to
      pre-grid (regression check: Acid800 suite results unchanged, 20 s
      no-disk smoke run clean).
- [ ] Performance check: 4 instances at full refresh — measure fps; confirm
      `GRID_BACKGROUND_REFRESH` frameskip works for non-selected instances.
- [ ] Memory check: 16 instances allocate/free cleanly (create/delete cycle,
      no leaks — valgrind if available).

## Phase G10 — Documentation

- [ ] Update [`aidocs/keybindings.md`](keybindings.md) with the grid keys.
- [ ] Update `DOC/USAGE` / readme with the grid feature and config options.
- [ ] Record any deviations from
      [`aidocs/atari-grid-design.md`](atari-grid-design.md) in that document.
