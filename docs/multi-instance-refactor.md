# Multi-Instance Refactor Design

**Status:** Assessment / Design
**Scope:** Enabling multiple Atari800 emulation instances to run side-by-side in a single process.
**Audience:** Developers working on the multi-instance feature.

---

## 1. Goal

The ultimate goal is to run **multiple Atari instances side-by-side** in one process
(e.g. a "multi-system" frontend, a grid of emulators, or a networked multi-player
setup where each player controls their own Atari).

Today the emulator is architected as a **single global instance**: every module keeps
its state in file-scope `extern` globals and `static` variables, and the CPU is
hard-wired to a single global 64 KB memory array. This document:

1. Confirms and inventories the global/static dependencies that block multi-instance.
2. Classifies each piece of state as *instance-specific* vs *shared/immutable*.
3. Proposes a refactor design to make the emulation core re-entrant.
4. Lays out a phased migration plan with minimal risk.

---

## 2. Current Architecture (Single Instance)

The emulation core lives in `src/`. The main driver is `Atari800_Frame()` in
[`src/atari.c`](src/atari.c), which advances one video frame by driving the
interlocked chips:

```
Atari800_Frame()
  -> CPU_GO(limit)          // 6502 executes until ANTIC_xpos_limit
  -> ANTIC_Frame()          // display list / DMA / scanline timing
  -> GTIA_Frame()           // playfield + player/missile graphics
  -> POKEY_Frame()          // audio + serial + keyboard
  -> PIA / SIO / devices    // peripherals
```

All of these modules communicate through **global variables** and **global function
pointers**. There is no "instance" object anywhere. The existing library wrapper
[`src/libatari800/api.c`](src/libatari800/api.c) exposes a single-instance API
(`libatari800_init`, `libatari800_next_frame`) but it simply calls the same globals —
it does **not** make the core re-entrant.

---

## 3. Inventory of Global / Static State

The following is a per-module inventory of the state that would need to become
per-instance. This is not exhaustive (each `.c` file also has many `static`
variables), but it captures the public surface and the most important internals.

### 3.1 CPU — [`src/cpu.h`](src/cpu.h), [`src/cpu.c`](src/cpu.c)

- Registers: `CPU_regPC`, `CPU_regA`, `CPU_regP`, `CPU_regS`, `CPU_regY`, `CPU_regX`
- `CPU_IRQ`, `CPU_cim_encountered`
- `CPU_rts_handler` (function pointer)
- Debug/remember buffers: `CPU_remember_PC[]`, `CPU_remember_op[][]`,
  `CPU_remember_PC_curpos`, `CPU_remember_xpos[]`, `CPU_remember_JMP[]`,
  `CPU_remember_jmp_curpos`
- `CPU_instruction_count[256]` (MONITOR_PROFILE)
- **Critical:** the entire instruction decoder reads/writes memory through macros
  (`MEMORY_dGetByte`, `MEMORY_GetByte`, `MEMORY_PutByte`, `MEMORY_dPutByte`) that
  reference the **global** `MEMORY_mem` array and global read/write maps. See §5.

### 3.2 Memory — [`src/memory.h`](src/memory.h), [`src/memory.c`](src/memory.c)

- `MEMORY_mem[65536 + 2]` — the 64 KB address space (the big one)
- `MEMORY_attrib[65536]` (non-paged) **or** `MEMORY_readmap[256]`,
  `MEMORY_safe_readmap[256]`, `MEMORY_writemap[256]` (PAGED_ATTRIB)
- `MEMORY_ram_size`, `MEMORY_xe_bank`, `MEMORY_selftest_enabled`
- `MEMORY_have_basic`, `MEMORY_cartA0BF_enabled`
- `MEMORY_basic[8192]`, `MEMORY_os[16384]`, `MEMORY_xegame[8192]`
- `MEMORY_mosaic_num_banks`, `MEMORY_axlon_0f_mirror`, `MEMORY_axlon_num_banks`,
  `MEMORY_enable_mapram`
- `static` internals: `under_atarixl_os[]`, `under_cart809F[]`, `under_cartA0BF[]`,
  `cart809F_enabled`, `atarixe_memory`, `axlon_ram`, `mosaic_ram`, `mapram_memory`,
  bank state, etc.

### 3.3 ANTIC — [`src/antic.h`](src/antic.h), [`src/antic.c`](src/antic.c)

- Registers: `ANTIC_CHACTL`, `ANTIC_CHBASE`, `ANTIC_dlist`, `ANTIC_DMACTL`,
  `ANTIC_HSCROL`, `ANTIC_NMIEN`, `ANTIC_NMIST`, `ANTIC_PMBASE`, `ANTIC_VSCROL`
- Timing: `ANTIC_break_ypos`, `ANTIC_ypos`, `ANTIC_wsync_halt`, `ANTIC_xpos`,
  `ANTIC_xpos_limit`, `ANTIC_screenline_cpu_clock`
- `ANTIC_artif_mode`, `ANTIC_artif_new`, `ANTIC_PENH_input`, `ANTIC_PENV_input`
- `ANTIC_xe_ptr`, PM DMA flags, colour lookup tables (`ANTIC_cl[]`, etc.)
- `NEW_CYCLE_EXACT` state: `ANTIC_delayed_wsync`, `ANTIC_cur_screen_pos`,
  `ANTIC_cpu2antic_ptr`, `ANTIC_antic2cpu_ptr`
- `ANTIC_pal_blending`

### 3.4 GTIA — [`src/gtia.h`](src/gtia.h), [`src/gtia.c`](src/gtia.c)

- All colour/position/graphics registers (`GTIA_GRAFP0..3`, `GTIA_HPOSP0..3`,
  `GTIA_HPOSM0..3`, `GTIA_SIZEP0..3`, `GTIA_SIZEM`, `GTIA_COLPM0..3`,
  `GTIA_COLPF0..3`, `GTIA_COLBK`, `GTIA_GRACTL`, `GTIA_PRIOR`, `GTIA_VDELAY`)
- Collision registers: `GTIA_M0PL..M3PL`, `GTIA_P0PL..P3PL`
- `GTIA_pm_scanline[]`, `GTIA_pm_dirty`
- Collision masks: `GTIA_collisions_mask_*`
- `GTIA_TRIG[4]`, `GTIA_TRIG_latch[4]`, `GTIA_consol_override`, `GTIA_speaker`
- `GTIA_colour_translation_table[256]` (USE_COLOUR_TRANSLATION_TABLE)

### 3.5 POKEY — [`src/pokey.h`](src/pokey.h), [`src/pokey.c`](src/pokey.c)

- `POKEY_KBCODE`, `POKEY_IRQST`, `POKEY_IRQEN`, `POKEY_SKSTAT`, `POKEY_SKCTL`
- `POKEY_DELAYED_SERIN_IRQ`, `POKEY_DELAYED_SEROUT_IRQ`, `POKEY_DELAYED_XMTDONE_IRQ`
- `NEW_CYCLE_EXACT`: `POKEY_irq_at_xpos`, `POKEY_irq_pending_mask`
- `POKEY_POT_input[8]`
- Audio state: `POKEY_AUDF[]`, `POKEY_AUDC[]`, `POKEY_AUDCTL[]`, `POKEY_DivNIRQ[]`,
  `POKEY_DivNMax[]`, `POKEY_Base_mult[]`
- Polynomial tables `POKEY_poly9_lookup[]`, `POKEY_poly17_lookup[]` — **immutable**,
  can be shared (see §4).

### 3.6 PIA — [`src/pia.h`](src/pia.h), [`src/pia.c`](src/pia.c)

- `PIA_PACTL`, `PIA_PBCTL`, `PIA_PORTA`, `PIA_PORTB`, `PIA_PORTA_mask`,
  `PIA_PORTB_mask`, `PIA_PORT_input[2]`
- `PIA_CA1`, `PIA_CB1`, `PIA_CA2`, `PIA_CB2`, `PIA_IRQ`

### 3.7 SIO / Disk drives — [`src/sio.h`](src/sio.h), [`src/sio.c`](src/sio.c)

- `SIO_status[256]`, `SIO_drive_status[8]`, `SIO_filename[8][FILENAME_MAX]`
- `SIO_last_op`, `SIO_last_op_time`, `SIO_last_drive`, `SIO_last_sector`
- `SIO_format_sectorcount[8]`, `SIO_format_sectorsize[8]`
- Per-drive file handles / buffers (in `sio.c`)

### 3.8 Devices (H:/P:/R:/B: patches) — [`src/devices.h`](src/devices.h)

- `Devices_enable_h_patch`, `Devices_enable_p_patch`, `Devices_enable_r_patch`,
  `Devices_enable_b_patch`
- `Devices_atari_h_dir[4][FILENAME_MAX]`, `Devices_h_read_only`,
  `Devices_h_exe_path`, `Devices_h_device_name`, `Devices_h_current_dir[4][]`
- `Devices_print_command[256]`, `dev_b_status`

### 3.9 Cartridge — [`src/cartridge.h`](src/cartridge.h)

- `CARTRIDGE_main`, `CARTRIDGE_piggyback` (`CARTRIDGE_image_t` structs holding
  type/state/size/`image` buffer/filename)
- `CARTRIDGE_autoreboot`

### 3.10 Cassette — [`src/cassette.h`](src/cassette.h)

- `CASSETTE_filename`, `CASSETTE_description`, `CASSETTE_status`
- `CASSETTE_hold_start`, `CASSETTE_hold_start_on_reboot`, `CASSETTE_press_space`
- `CASSETTE_write_protect`, `CASSETTE_record`, `CASSETTE_readable`, `CASSETTE_writable`
- Tape position / file state (in `cassette.c`)

### 3.11 PBI / Peripherals — [`src/pbi.h`](src/pbi.h), `pbi_*.c`, `rtime.c`, `xep80.c`, etc.

- `PBI_IRQ`, `PBI_D6D7ram`
- `RTIME_enabled` and R-Time state
- XEP80, AF80, BIT3, IDE, voicebox, etc. all carry their own globals.

### 3.12 Input — [`src/input.h`](src/input.h), [`src/input.c`](src/input.c)

- `INPUT_key_code`, `INPUT_key_shift`, `INPUT_key_consol`
- `INPUT_joy_autofire[4]`, `INPUT_joy_block_opposite_directions`, `INPUT_joy_multijoy`
- `INPUT_joy_5200_min/center/max`
- Mouse: `INPUT_mouse_mode`, `INPUT_mouse_port`, `INPUT_mouse_delta_x/y`,
  `INPUT_mouse_buttons`, `INPUT_mouse_speed`, `INPUT_mouse_pot_min/max`,
  `INPUT_mouse_pen_ofs_h/v`, `INPUT_mouse_joy_inertia`, `INPUT_direct_mouse`
- `INPUT_cx85`

### 3.13 Display / Screen — [`src/screen.h`](src/screen.h)

- `Screen_atari` (the framebuffer pointer), `Screen_atari_b/1/2` (BITPL_SCR)
- `Screen_dirty` (DIRTYRECT)
- `Screen_visible_x1/y1/x2/y2`, `Screen_show_atari_speed`, `Screen_show_disk_led`,
  `Screen_show_sector_counter`, `Screen_show_1200_leds`, `Screen_show_multimedia_stats`

### 3.14 Sound — [`src/sound.h`](src/sound.h)

- `Sound_desired`, `Sound_out`, `Sound_enabled`, `Sound_latency`
- The audio mixing buffer and POKEY sample state (in `sound.c` / `pokeysnd.c`)

### 3.15 Top-level config — [`src/atari.h`](src/atari.h), [`src/atari.c`](src/atari.c)

- `Atari800_machine_type`, `Atari800_builtin_basic`, `Atari800_keyboard_leds`,
  `Atari800_f_keys`, `Atari800_jumper`, `Atari800_builtin_game`,
  `Atari800_keyboard_detached`, `Atari800_tv_mode`, `Atari800_disable_basic`,
  `Atari800_os_version`, `Atari800_display_screen`, `Atari800_nframes`,
  `Atari800_refresh_rate`, `Atari800_collisions_in_skipped_frames`,
  `Atari800_turbo`, `Atari800_turbo_speed`, `Atari800_start_in_monitor`,
  `Atari800_auto_frameskip`
- `verbose`, `sigint_flag`, `dl_dir`, benchmark timers

---

## 4. Classification: Instance-Specific vs Shared

Not everything needs to be duplicated. The key distinction:

### 4.1 Instance-specific (must be per-instance)

Everything that represents the *state of one running machine*:

- CPU registers and internal state
- The 64 KB `MEMORY_mem` + attribute/read/write maps
- All chip registers (ANTIC, GTIA, POKEY, PIA)
- Cartridge/cassette/disk state and mounted images
- PBI/peripheral state
- Input state (per-instance key/joystick/mouse)
- The framebuffer `Screen_atari` and dirty-rect state
- Per-instance config (machine type, TV mode, RAM size, turbo, etc.)
- Per-instance counters (`Atari800_nframes`, `Atari800_display_screen`)

### 4.2 Shared / immutable (safe to keep global, read-only)

These are computed once and never mutated during emulation, so they can remain
global and be shared by all instances:

- **ROM images** (`MEMORY_os`, `MEMORY_basic`, `MEMORY_xegame`, altirra ROMs) —
  *provided* they are treated as read-only after load. (Currently some are copied
  into `MEMORY_mem`; the copy is per-instance, the source image can be shared.)
- **POKEY polynomial tables** (`POKEY_poly9_lookup`, `POKEY_poly17_lookup`)
- **Colour translation / lookup tables** (`GTIA_colour_translation_table`,
  `ANTIC_cl[]`, `ANTIC_lookup_gtia9/11`, `ANTIC_hires_lookup_l`) — if not mutated
  per-instance.
- **CPU cycle tables / timing constants** (`ANTIC_cpu2antic_ptr`,
  `ANTIC_antic2cpu_ptr`) — if truly constant.
- **Static configuration** that is process-wide (e.g. global ROM search paths,
  the `log` subsystem, the config file path).

> **Rule of thumb:** if a variable is written during `Atari800_Frame()` or during
> `Coldstart`/`Warmstart`, it is instance-specific. If it is only read, it can be
> shared.

---

## 5. The Core Problem: CPU ↔ Memory Coupling

The single hardest obstacle is the **macro-level coupling between the CPU decoder
and the global memory array**. In [`src/cpu.c`](src/cpu.c) the 6502 decoder is a
giant `switch`/`goto` that reads and writes memory through macros defined in
[`src/memory.h`](src/memory.h):

```c
#define MEMORY_dGetByte(x)  (MEMORY_mem[x])
#define MEMORY_GetByte(addr) (MEMORY_attrib[addr] == MEMORY_HARDWARE \
                              ? MEMORY_HwGetByte(addr, FALSE) : MEMORY_mem[addr])
#define MEMORY_PutByte(addr, byte) ...
```

These macros reference the **global** `MEMORY_mem` and the **global** read/write
maps. There are ~140 call sites in `cpu.c` alone, plus many more across `antic.c`,
`gtia.c`, `pokey.c`, `pia.c`, `devices.c`, `sio.c`, `cartridge.c`, `cassette.c`,
`pbi.c`, `monitor.c`, `statesav.c`, etc.

Because the decoder is written as macros over a global, you cannot simply pass a
"memory context" as a function argument without rewriting every instruction. This
is the central design decision for the whole refactor.

---

## 6. Refactor Design Options

There are three viable strategies, in increasing order of invasiveness. They are
**not mutually exclusive**, but the project has **decided to commit to Option C**
(the full context-object refactor) as the target architecture. Options A and B are
documented here for context and as possible stepping stones, but the plan below
assumes Option C.

### Option A — "Context struct + current-instance pointer" (re-entrant, single-threaded)

Introduce a single `Atari800_Instance` struct that aggregates all per-instance
state, and a **module-level "current instance" pointer** that the frame loop sets
before driving the chips.

```c
typedef struct Atari800_Instance {
    /* CPU */
    UWORD cpu_regPC; UBYTE cpu_regA, cpu_regP, cpu_regS, cpu_regY, cpu_regX;
    UBYTE cpu_IRQ; ...
    /* Memory */
    UBYTE mem[65536 + 2];
    UBYTE attrib[65536];            /* or readmap/writemap */
    int ram_size; ...
    /* Chips */
    ANTIC_state_t antic; GTIA_state_t gtia; POKEY_state_t pokey; PIA_state_t pia;
    /* Peripherals */
    SIO_state_t sio; Devices_state_t devices; Cartridge_state_t cart; ...
    /* Config */
    int machine_type; int tv_mode; ...
    /* Output */
    ULONG *screen_atari; ...
} Atari800_Instance;

/* The "active" instance for the duration of a frame. */
extern Atari800_Instance *Atari800_current;
```

The frame loop becomes:

```c
void Atari800_FrameInstance(Atari800_Instance *inst) {
    Atari800_current = inst;
    CPU_GO(limit);
    ANTIC_Frame();
    GTIA_Frame();
    POKEY_Frame();
    ...
}
```

**How the macros get fixed:** the memory macros are re-pointed at the current
instance's memory:

```c
#define MEMORY_mem  (Atari800_current->mem)
#define MEMORY_attrib (Atari800_current->attrib)
```

Because `MEMORY_mem` is currently a global *array* (not a pointer), this requires
changing its declaration to a macro or a pointer. The cleanest minimal change is to
turn `MEMORY_mem` into a macro that dereferences `Atari800_current->mem`, and keep
the rest of the code untouched. The CPU decoder then works unchanged.

**Pros:**
- Minimal, mechanical change; the huge CPU decoder is untouched.
- Enables multiple instances to be created, run, and switched between.
- Works with the existing `libatari800` API (add an instance handle).

**Cons:**
- **Not thread-safe.** Only one instance runs at a time; you must serialize frames
  (e.g. run each instance on its own thread but guard with a global mutex, or
  time-slice them on one thread). "Side-by-side" in the sense of *simultaneous
  parallel execution* is not achieved, only *coexistence*.

### Option B — "Thread-local current instance" (per-thread instances)

Same as Option A, but the "current instance" pointer is `_Thread_local` (C11) /
`__thread` (GCC/Clang) instead of a plain global.

```c
extern _Thread_local Atari800_Instance *Atari800_current;
```

Each OS thread gets its own `Atari800_current`, so **each thread can run its own
instance in parallel** without a global lock. The macros still work unchanged.

**Pros:**
- True parallelism: one thread per instance, no shared mutable state.
- Still minimal change to the CPU decoder.

**Cons:**
- Requires a threading model where each instance lives on its own thread.
- The `static` variables inside each `.c` file (e.g. `sio.c` file handles,
  `cassette.c` tape position) are **not** thread-local and would still collide.
  These must be moved into the instance struct (or made thread-local too).
- Platform ports (SDL, X11, etc.) and the sound/display backends are not
  thread-safe and need per-instance handling.

### Option C — "Full context object + function pointers" (cleanest, most invasive)

Refactor every module to take an explicit context pointer, and replace the memory
macros with function-pointer-based access (or pass the memory context through the
CPU). This is the "textbook" re-entrant design.

```c
void CPU_GO(Atari800_Instance *inst, int limit);
void ANTIC_Frame(Atari800_Instance *inst, int draw_display);
UBYTE MEMORY_GetByte(Atari800_Instance *inst, UWORD addr, int safe);
```

**Pros:**
- Clean, explicit, no hidden global state; fully thread-safe and testable.
- Best long-term architecture.

**Cons:**
- **Very invasive.** The CPU decoder's ~140 macro call sites and the hot-path
  memory access would need rewriting, risking performance regressions (the current
  macros inline to direct array access; function pointers add indirection).
- Large diff, high regression risk across all the platform ports.

---

## 7. Recommended Approach (Decision: Option C)

**The project has committed to Option C — the full context-object refactor.** Every
emulation module will take an explicit `Atari800_Instance *` (or a per-module
sub-context) as its first parameter, and the global memory macros will be replaced
with explicit context-aware access. This is the most invasive option but yields the
cleanest, fully re-entrant, thread-safe architecture and the most flexibility for
future features (per-instance config, independent save states, parallel frames).

The migration is deliberately phased so the tree stays buildable at each step. The
detailed per-module work items live in
[`docs/refactor-checklist.md`](refactor-checklist.md); the phases below are the
high-level sequence.

### Phase 1 — Define the instance context and the memory access layer

1. Define `Atari800_Instance` in a new header (`src/instance.h`) aggregating the
   per-instance state from §3, plus per-module sub-structs (`ANTIC_state_t`,
   `GTIA_state_t`, `POKEY_state_t`, `PIA_state_t`, `SIO_state_t`, etc.).
2. Replace the global memory macros with context-aware accessors. The CPU hot path
   is the priority: introduce a `MEMORY_State` (or pass the instance) and rewrite
   the ~140 macro call sites in `cpu.c` to use it. Keep the accessors `static
   inline` so the compiler can still optimize the common RAM case to a direct
   array access (avoiding function-pointer indirection in the hot loop).
3. Convert the CPU register globals into `CPU_state_t` and thread it through the
   decoder.

### Phase 2 — Convert each chip module to take the context

Convert ANTIC, GTIA, POKEY, PIA, and the memory subsystem one at a time. Each
module's `*_GetByte`/`*_PutByte`/`*_Frame`/`*_Reset`/`*_StateSave`/`*_StateRead`
functions take the instance (or sub-context) as their first argument. Update the
callers in `atari.c`, `memory.c`, and `cpu.c` accordingly. Build and test after
each module.

### Phase 3 — Convert peripheral modules

Convert SIO, devices, cartridge, cassette, PBI (and all `pbi_*` sub-modules),
RTIME, XEP80, AF80, BIT3, IDE, voicebox, input, and the ESC/binload handlers to
take the context. This is the bulk of the mechanical work.

### Phase 4 — Convert output/input and the platform layer

Make the framebuffer (`Screen_atari`), dirty-rect state, sound buffers, and input
state per-instance. Update the platform ports (`sdl/`, `atari_x11.c`,
`atari_rpi.c`, `macosx/`, `android/`, etc.) to drive instances through the new API.
The `log` subsystem and config-file handling remain process-global.

### Phase 5 — Add the multi-instance lifecycle API

```c
Atari800_Instance *Atari800_NewInstance(void);
void Atari800_FreeInstance(Atari800_Instance *inst);
void Atari800_FrameInstance(Atari800_Instance *inst);
void Atari800_ColdstartInstance(Atari800_Instance *inst);
void Atari800_WarmstartInstance(Atari800_Instance *inst);
```

Extend `libatari800` so callers can create and drive multiple instances. The
existing single-instance API is re-implemented on top of a default instance.

### Phase 6 — Parallelism (optional, later)

Because Option C removes all shared mutable state, instances can be driven from
separate threads with no global lock. Each instance owns its framebuffer, sound
buffer, and input source; the frontend composites video and mixes audio.

---

## 8. What Stays Global (Shared)

Keep these process-global and read-only:

- ROM image sources (loaded once, copied into each instance's `MEMORY_mem`)
- POKEY polynomial tables
- Colour/lookup tables (if not mutated)
- The `log` subsystem
- Config file path / global ROM search paths
- The `libatari800` error-message table

---

## 9. Risks and Considerations

1. **Performance (the big one):** The memory macros currently inline to direct
   array access. Replacing them with context-aware accessors risks adding
   indirection in the CPU hot loop. Mitigation: keep the accessors `static inline`
   and structure the memory state so the common RAM case compiles to a direct
   array access (e.g. cache the `mem` pointer in a local at the top of `CPU_GO`).
   Benchmark before/after each phase.
2. **Large diff / regression risk:** Option C touches every module and every
   platform port. Mitigation: migrate module-by-module, keeping the tree buildable
   after each step, and lean on the existing test suite (`test/`, `acidtest`,
   `libatari800_test.c`).
3. **Platform ports:** `sdl/`, `atari_x11.c`, `atari_rpi.c`, `macosx/`, `android/`,
   etc. all touch the globals directly. They must be updated to use the instance
   API, or the multi-instance feature should initially be exposed only through
   `libatari800` while the standalone ports keep using a default instance.
4. **Sound/display backends** are inherently single-device. Multiple instances
   need either per-instance backends or a frontend that mixes audio and tiles
   video.
5. **State save/load** (`statesav.c`) reads/writes the globals; it must be made
   instance-aware so each instance can save/load independently.
6. **Monitor/debugger** (`monitor.c`) is single-instance today; decide whether it
   attaches to a specific instance.
7. **`static` state is the hidden trap:** the public `extern` globals are easy to
   find, but every `static` variable in every `.c` file is also per-instance state
   and must be audited. A grep for `^static` in `src/*.c` is a good starting point.
8. **Shared/immutable data** (ROM images, POKEY polynomial tables, colour lookup
   tables) must be kept read-only so they can be safely shared across instances.

---

## 10. Suggested First Steps

1. Create `src/instance.h` with the `Atari800_Instance` struct skeleton and the
   per-module sub-structs.
2. Build the context-aware memory access layer and convert the CPU decoder
   (`cpu.c`) to use it, keeping the accessors `static inline`. Confirm the existing
   build and tests still pass.
3. Convert the memory subsystem (`memory.c`) and the chip modules (ANTIC, GTIA,
   POKEY, PIA) one at a time, updating callers in `atari.c` after each.
4. Convert the peripheral modules (SIO, devices, cartridge, cassette, PBI, etc.).
5. Add `Atari800_NewInstance` / `Atari800_FrameInstance` and a small test that
   creates two instances, runs each for a few frames, and verifies their memory
   and CPU state are independent.
6. Extend `libatari800` to expose the multi-instance API.

> The detailed, per-module work items are enumerated in
> [`docs/refactor-checklist.md`](refactor-checklist.md).

---

## 11. Summary

The emulator is a **single global instance** today. The main blockers are:

- The **global 64 KB memory array** and the **macro-based CPU↔memory coupling**.
- **Per-module `extern` and `static` state** for every chip and peripheral.
- **Single-instance output** (framebuffer, sound) and **single-instance input**.

**Decision: the project is committing to Option C — the full context-object
refactor.** Every module will take an explicit instance/context pointer, and the
global memory macros will be replaced with context-aware accessors. This is the
most invasive option, but it removes all shared mutable state, making the core
fully re-entrant, thread-safe, and flexible for future features.

The migration is phased so the tree stays buildable at each step: define the
instance context and memory access layer → convert the CPU and chip modules →
convert the peripheral modules → convert output/input and the platform layer →
add the multi-instance lifecycle API → (optionally) enable parallel frames. The
detailed per-module work items are enumerated in
[`docs/refactor-checklist.md`](refactor-checklist.md).