# Atari Grid Design — Multi-Instance Grid Frontend

**Status:** Design
**Scope:** A grid/tiled frontend for the SDL port that runs multiple Atari800
instances side-by-side in one window, with selection, per-instance settings,
and routed audio/input.
**Audience:** Developers implementing the grid feature.
**Prerequisite:** The multi-instance refactor
([`aidocs/multi-instance-refactor.md`](multi-instance-refactor.md),
[`aidocs/refactor-checklist.md`](refactor-checklist.md)) — the emulation core
is now re-entrant via `Atari800_Instance` and exposes
`Atari800_NewInstance` / `Atari800_FreeInstance` / `Atari800_FrameInstance`.

---

## 1. Goal

Run **N Atari instances** simultaneously in a single SDL window, arranged in a
grid. One instance is **selected** at any time; it receives keyboard/joystick
input and is the only source of audio. The user can create/delete instances,
move the selection around the grid, and toggle between the grid view and a
"selected only" full-window view.

---

## 2. Key Bindings

| Key | Function | Notes |
|-----|----------|-------|
| Shift+F1 | Create a new instance | Appends to the grid; window grows if needed |
| Shift+F2 | Delete the current (selected) instance | Grid re-flows; last instance cannot be deleted |
| Shift+F3 | Toggle Grid mode ↔ Selected-screen mode | Selected screen fills the window; no outline in this mode |
| Shift+Ctrl+Arrow keys | Move the selection in the grid | Wraps/clamps per §5.3 |

These are implemented in the SDL port ([`src/sdl/input.c`](../src/sdl/input.c))
alongside the existing Shift+F5 (cold start) handling. They should also be
rebindable via new config options (§9): `SDL_GRID_NEW_KEY`,
`SDL_GRID_DELETE_KEY`, `SDL_GRID_TOGGLE_KEY`.

**Conflict check:** Shift+F1/F2/F3 are currently unused in the SDL port
(F1 = UI menu, F3 = SELECT; Shift+F5 = cold start is the only existing
Shift+Fn binding). Shift+Ctrl+arrows are unused (plain arrows and Shift+arrows
map to Atari cursor keys / F1–F4 when `F_KEYS=1`; the Shift+Ctrl combination
must be checked *before* the Atari key translation path and consumed).

---

## 3. Grid Layout Algorithm

### 3.1 Instance count → grid dimensions

Given `N` instances, the grid is `cols × rows` where `cols = ceil(sqrt(N))`
rounded so that `cols >= rows` (landscape bias), i.e.:

```
cols = ceil(sqrt(N))
rows = ceil(N / cols)
```

This yields exactly the requested table:

| N | Grid |
|---|------|
| 1 | 1×1 |
| 2 | 2×1 |
| 3 | 2×2 |
| 4 | 2×2 |
| 5 | 3×2 |
| 6 | 3×2 |
| 7 | 3×3 |
| 8 | 3×3 |
| 9 | 3×3 |
| 10 | 4×3 |
| 11 | 4×3 |
| 12 | 4×3 |
| 13 | 4×4 |
| ... | ... |

Instances fill the grid **row-major** (left→right, top→bottom). Empty cells
(bottom-right of the last row) are left blank.

### 3.2 Cell geometry

- The host window is divided into `cols × rows` equal cells.
- Each cell is inset by a small border (e.g. 2 px) so instances do not touch.
- Each instance's framebuffer (`inst->screen.atari`, `Screen_WIDTH ×
  Screen_HEIGHT`) is **scaled** to fit its cell, preserving aspect ratio
  (letterbox within the cell if the aspect differs).
- Scaling is done with the existing SDL blit path (`SDL_SoftStretch` /
  renderer texture scaling in [`src/sdl/video_sw.c`](../src/sdl/video_sw.c) /
  [`src/sdl/video_gl.c`](../src/sdl/video_gl.c)).

### 3.3 Window sizing

When an instance is added, the required window size is
`cols × cellW × rows × cellH` where `cellW/cellH` are derived from the
instance framebuffer size and a minimum scale factor. If the current window is
smaller, it is resized (up to the desktop work area; clamp to it and let cells
shrink if the desktop is too small). The window is never shrunk when instances
are deleted (avoid jarring resizes); it shrinks only if the user resizes it.

---

## 4. View Modes

`GRID_MODE` (default) and `SELECTED_MODE`, toggled by Shift+F3.

- **GRID_MODE:** all instances are blitted into their cells; the selected
  instance gets a **light-green outline ~5 px thick** drawn around its cell
  (SDL_DrawRect ×4 or a renderer rect, colour e.g. RGB(0x40,0xC0,0x40)).
- **SELECTED_MODE:** only the selected instance is displayed, scaled to fill
  the whole window (same scaling path as the current single-instance display),
  **without** the outline. The other instances keep running (frames are still
  advanced for all instances every host frame — see §7) but are not rendered.

---

## 5. Selection

### 5.1 State

```c
typedef struct Grid_state {
    Atari800_Instance *instances[GRID_MAX_INSTANCES]; /* NULL = empty slot */
    int count;            /* number of live instances */
    int selected;         /* index into instances[] of the selected one */
    int mode;             /* GRID_MODE / SELECTED_MODE */
    int cols, rows;       /* current grid dimensions */
} Grid_state;
```

`GRID_MAX_INSTANCES` — soft cap (e.g. 16) to bound memory/CPU use; Shift+F1
beyond the cap is ignored (status message).

### 5.2 Selection rules

- Exactly one instance is always selected.
- Creating an instance selects the new instance.
- Deleting the selected instance selects the next live instance (same index if
  it still exists, else the previous one; if it was the last, select the last
  remaining).
- The selected instance is never deleted by Shift+F2 when it is the only
  instance (ignored with a status message).

### 5.3 Selection movement (Shift+Ctrl+arrows)

The selection moves in the **grid layout**, not the instance array:

- Compute the selected instance's `(col, row)` from its index `i`:
  `col = i % cols`, `row = i / cols`.
- Arrow moves the cursor one cell in that direction. If the target cell is
  empty (beyond `count`), the move **wraps** within the grid bounds:
  - Left from col 0 → last column of the same row (if occupied, else clamp).
  - Right from the last occupied column → col 0 of the same row.
  - Up/Down similarly wrap between rows; if the destination row has no
    instance in that column, move to the nearest occupied cell in that row
    (or clamp).
- Simple, predictable rule set; exact wrap/clamp behaviour is specified in the
  implementation checklist and refined during testing.

---

## 6. Per-Instance Settings

All per-machine settings already live in `Atari800_Instance` (drives,
cassette, cartridge, devices, machine type, TV mode, ...). The UI must operate
on the **selected instance**:

- **Disk management, cassette, cartridge menus** ([`src/ui.c`](../src/ui.c))
  currently operate on the default instance via the legacy forwarding macros.
  They must be converted to take the selected instance
  (`Atari800_GridSelected()`), e.g. by passing the instance into
  `UI_Run`/menu handlers or by temporarily re-pointing a "UI current instance"
  pointer (same pattern as `LIBATARI800_SetCurrentInstance`).
- **Config file:** the process-global config
  ([`src/cfg.c`](../src/cfg.c)) remains the *initial* template for new
  instances. Per-instance changes (mounted disks etc.) are runtime state; a
  follow-up may add per-instance config persistence (the §4.7 CFG follow-up in
  the refactor checklist).
- **New instances** are initialised from the current default-instance
  configuration (machine type, TV mode, RAM size), then
  `Atari800_InitialiseMachine`-equivalent per-instance init is run (see
  checklist item on full per-instance init — the Phase 5 follow-up).

---

## 7. Frame Loop and Audio/Input Routing

### 7.1 Frame loop (single-threaded, serialized)

The SDL main loop advances **every** live instance once per host frame:

```c
for (i = 0; i < grid.count; i++)
    Atari800_FrameInstance(grid.instances[i]);
```

Instances run time-sliced on one thread (Option A semantics — the refactor's
Option C context design permits this without locks). If the host cannot keep
up with N instances at full speed, apply a global frameskip or reduce per-
instance refresh (e.g. run non-selected instances every 2nd frame) — tunable
via a config option (`GRID_BACKGROUND_REFRESH`, default 1 = full rate).

### 7.2 Input routing

- Host keyboard/joystick events are delivered **only** to the selected
  instance: the SDL input handlers write into
  `selected->input.*` (`INPUT_key_code`, `POKEY_POT_input`, ...) instead of
  the default-instance aliases.
- Grid-control keys (§2) are intercepted before Atari key translation and are
  never forwarded to any instance.
- Mouse: routed to the selected instance (existing mouse emulation path,
  re-pointed at the selected instance).

### 7.3 Audio routing

- **Only the selected instance produces audio.** The SDL sound callback mixes
  from the selected instance's POKEY output only.
- Implementation: the sound module ([`src/sound.c`](../src/sound.c),
  [`src/pokeysnd.c`](../src/pokeysnd.c)) is still process-global (Phase 4.3 of
  the refactor). Two options:
  1. **Preferred (short term):** keep the global sound pipeline but point it
     at the selected instance — before generating samples each frame, pin the
     sound/POKEY-sound context to the selected instance (same
     "current instance" pattern as `libatari800`). On selection change,
     re-init the POKEY sound state from the new instance's POKEY registers.
  2. **Later (Phase 4.3 conversion):** per-instance sound buffers with a
     mixer that sums all instances but mutes non-selected ones. Not needed for
     the initial feature since only one instance is audible.
- On selection change: `POKEYSND_Init`-equivalent re-init for the new
  instance to avoid stale audio state.

---

## 8. Rendering Pipeline (SDL)

Per host frame in `GRID_MODE`:

1. Clear the window surface/renderer to black.
2. For each live instance `i`:
   - Compute cell rect from `(i % cols, i / cols)`.
   - Blit `instances[i]->screen.atari` scaled into the cell
     (aspect-preserving, centred).
3. Draw the 5 px light-green outline around the selected instance's cell.
4. Present.

In `SELECTED_MODE`: blit only the selected instance scaled to the full window
(this is essentially the existing single-instance display path) and present.

The existing SDL video modules (`video_sw.c` / `video_gl.c`) own the window
and present path; the grid renderer is a new layer (`src/sdl/grid.c` +
`src/sdl/grid.h`) that sits above them and either (a) reuses their blit
helpers per cell, or (b) takes over presentation entirely while grid mode is
active. Option (b) is cleaner: `SDL_VIDEO_DisplayScreen` checks the grid mode
and delegates to `GRID_Display()`.

Status overlays (fps, disk LED) are drawn only for the selected instance.

---

## 9. Config Options

New `.atari800.cfg` options (written/read by the SDL port's config section):

```
GRID_ENABLED = 0            # start in grid mode with 1 instance (default single-instance behaviour unchanged)
GRID_MAX_INSTANCES = 16
GRID_BACKGROUND_REFRESH = 1 # frames per background-instance update
GRID_OUTLINE_COLOUR = 40C040
SDL_GRID_NEW_KEY = ...      # default Shift+F1
SDL_GRID_DELETE_KEY = ...   # default Shift+F2
SDL_GRID_TOGGLE_KEY = ...   # default Shift+F3
```

With `GRID_ENABLED = 0` (default), the emulator behaves exactly as today
(single default instance, no grid keys active) — zero regression risk.

---

## 10. Lifecycle Details

### 10.1 Create (Shift+F1)

1. If `count == GRID_MAX_INSTANCES`, show status message, return.
2. `inst = Atari800_NewInstance()`.
3. Initialise `inst` from the current config template (machine type, TV mode,
   OS ROM selection) and run the per-instance machine init.
4. Append to `instances[]`, `count++`, recompute `cols/rows` (§3.1).
5. Resize the window if required (§3.3).
6. Select the new instance.

### 10.2 Delete (Shift+F2)

1. If `count == 1`, show status message, return.
2. `Atari800_FreeInstance(instances[selected])` (frees mounted images,
   cartridges, buffers).
3. Compact the array (shift the tail left), `count--`, recompute grid dims.
4. Fix up selection (§5.2). Window is not resized (§3.3).

### 10.3 Toggle (Shift+F3)

Flip `mode` between `GRID_MODE` and `SELECTED_MODE`. No instance state
changes; only rendering differs.

---

## 11. Risks and Considerations

1. **Performance:** N instances × full speed on one thread. Mitigation:
   `GRID_BACKGROUND_REFRESH` frameskip for non-selected instances; measure
   with the existing benchmark hooks.
2. **Sound module is still global** (refactor Phase 4.3 pending): the
   "pin to selected instance" approach (§7.3) is a transitional hack; it must
   be revisited when Sound is converted.
3. **UI menus are default-instance-bound:** the `ui.c` conversion to operate
   on the selected instance is the largest non-rendering work item.
4. **Window resize events:** user-driven resizes must recompute cell geometry;
   fullscreen toggling must preserve the grid layout.
5. **Memory:** each instance is ~1 MB+ (64 KB RAM + expansion + buffers);
   16 instances is fine on desktop, but the cap should be configurable.
6. **Monitor/debugger** attaches to the selected instance (already
   instance-aware post-refactor).
7. **State save/load** is per-instance post-refactor; the UI Save/Load state
   menu should target the selected instance.
8. **Screenshot** (F10) captures the selected instance's framebuffer in
   SELECTED_MODE; in GRID_MODE it captures the composed window (or the
   selected instance — decide during implementation; default: selected
   instance).

---

## 12. Summary

The grid frontend is a new SDL-layer module (`src/sdl/grid.c`) plus
conversions of the input and UI paths to be selection-aware. The emulation
core already supports everything needed via the instance API. The design
keeps single-instance behaviour as the default (`GRID_ENABLED = 0`) so the
feature is fully opt-in, and defers per-instance audio mixing until the Sound
module conversion (refactor Phase 4.3).
