# Refactor Checklist — Context-Pointer Migration (Option C)

This checklist enumerates every module and piece of code that must be refactored to
take an explicit instance/context pointer as part of the **Option C** multi-instance
refactor. It is the working companion to
[`docs/multi-instance-refactor.md`](multi-instance-refactor.md).

## Conventions

- **Context type:** `Atari800_Instance *` (the top-level instance) or a per-module
  sub-context (e.g. `ANTIC_state_t *`, `SIO_state_t *`) reachable from it.
- **Signature change:** each listed function gains the context as its first
  parameter, e.g. `void ANTIC_Frame(Atari800_Instance *inst, int draw_display)`.
- **State to move:** the `extern`/`static` globals listed for each module move into
  the instance struct (or a per-module sub-struct).
- **Shared/immutable:** items marked *(shared)* stay process-global and read-only.
- **Checkbox:** `[ ]` = not started, `[x]` = done, `[-]` = in progress.

---

## Phase 1 — Instance context + memory access layer

> **Status: nearly complete.** The instance struct, the context-aware memory
> access layer, the CPU decoder conversion, the CPU register state migration,
> the memory subsystem function conversion, and the memory *state* migration
> (all bank/expansion statics now live in `MEMORY_state_t`) are done and the
> tree builds; smoke run passes. `MEMORY_HwGetByte` / `MEMORY_HwPutByte` are
> deferred until the chip modules are migrated (they dispatch to
> ANTIC/GTIA/POKEY/PIA/cartridge/PBI entry points, which do not take a
> context yet).

### 1.1 New header `src/instance.h`

- [x] Define `Atari800_Instance` aggregating all per-instance state.
- [x] Define core sub-structs: `CPU_state_t`, `MEMORY_state_t`,
      `ANTIC_state_t`, `GTIA_state_t`, `POKEY_state_t`, `PIA_state_t`.
- [ ] Define peripheral sub-structs: `SIO_state_t`, `Devices_state_t`,
      `Cartridge_state_t`, `Cassette_state_t`, `PBI_state_t`, `Input_state_t`,
      `Screen_state_t`, `Sound_state_t` (currently forward-declared, referenced
      by pointer; to be defined in later phases).
- [x] Define the lifecycle API prototypes (`Atari800_NewInstance`, etc.).
- [x] Add the transitional `Atari800_default` instance pointer.

### 1.2 Memory access layer — [`src/memory.h`](src/memory.h), [`src/memory.c`](src/memory.c)

- [x] Replace global `MEMORY_mem[65536+2]` with a per-instance buffer
      (aliased to `Atari800_default->memory.mem`).
- [x] Replace `MEMORY_attrib[65536]` (non-paged) with a per-instance buffer
      (aliased to `Atari800_default->memory.attrib`).
- [x] Replace `MEMORY_readmap[256]` / `MEMORY_safe_readmap[256]` /
      `MEMORY_writemap[256]` (PAGED_ATTRIB) with per-instance maps
      (aliased to the default instance).
- [x] Add context-aware `static inline` accessors (`MEMORY_dGetByteCtx`,
      `MEMORY_dPutByteCtx`, `MEMORY_GetByteCtx`, `MEMORY_PutByteCtx`,
      `MEMORY_SafeGetByteCtx`, word/copy/fill variants). The RAM fast path is a
      direct array access.
- [x] Convert functions: `MEMORY_InitialiseMachine`, `MEMORY_StateSave`,
      `MEMORY_StateRead`, `MEMORY_CopyFromMem`, `MEMORY_CopyToMem`,
      `MEMORY_HandlePORTB`, `MEMORY_Cart809fDisable/Enable`,
      `MEMORY_CartA0bfDisable/Enable`, `MEMORY_GetCharset`. Each is now
      `*_Ctx(Atari800_Instance *inst, ...)` in `memory.c`, which sets a
      file-scope context pointer that the (redefined) memory accessor macros
      route through; the legacy names remain as forwarding macros in
      `memory.h` that pass `Atari800_default`, so unconverted callers are
      unchanged. `cpu.c` passes the instance directly from
      `CPU_StateSave`/`CPU_StateRead`.
- [x] `MEMORY_SizeValid` — stateless, signature unchanged.
- [x] `MEMORY_ROM_PutByte` — stateless PAGED_ATTRIB stub, signature unchanged
      (it is stored in the per-instance `writemap`).
- [ ] Convert `MEMORY_HwGetByte`/`MEMORY_HwPutByte` — deferred: they dispatch
      to chip/peripheral `*_GetByte`/`*_PutByte` functions that do not take a
      context yet. Convert together with the Phase 2 chip modules; for now they
      pin the file-scope context to the default instance.
- [x] Move state: `MEMORY_ram_size`, `MEMORY_xe_bank`, `MEMORY_selftest_enabled`,
      `MEMORY_have_basic`, `MEMORY_cartA0BF_enabled`, `MEMORY_mosaic_num_banks`,
      `MEMORY_axlon_0f_mirror`, `MEMORY_axlon_num_banks`, `MEMORY_enable_mapram`,
      and the `static` internals (`under_atarixl_os[]`, `under_cart809F[]`,
      `under_cartA0BF[]`, `cart809F_enabled`, `atarixe_memory`, `axlon_ram`,
      `mosaic_ram`, `mapram_memory`, bank state: `axlon_current_bankmask`,
      `axlon_curbank`, `mosaic_current_num_banks`, `mosaic_curbank`,
      `antic_bank_under_selftest[]`). Done 2026-09-14: all moved into
      `MEMORY_state_t` ([`src/instance.h`](src/instance.h)); `memory.c` aliases
      the legacy names to the file-scope `M` context, and `memory.h` aliases
      them to `Atari800_default->memory.*` for not-yet-migrated callers. The
      default instance is initialized with `ram_size = 64`,
      `mosaic_curbank = 0x3f` ([`src/atari.c`](src/atari.c)).
- [x] Keep shared: `MEMORY_os[16384]`, `MEMORY_basic[8192]`,
      `MEMORY_xegame[8192]` — kept as fields in `MEMORY_state_t` (so a future
      instance may embed its own ROM images) but aliased to the default
      instance via `memory.h` for now *(treated as shared, read-only ROM
      sources during migration)*.

### 1.3 CPU — [`src/cpu.h`](src/cpu.h), [`src/cpu.c`](src/cpu.c)

- [x] Convert the entire 6502 decoder to use the context-aware memory accessors
      (~140 macro call sites). Done by redefining the memory accessor macros in
      `cpu.c` to route through a local `MEMORY_state_t *mem` derived from the
      instance, so the decoder body is unchanged.
- [x] Convert `CPU_GO`, `CPU_Reset`, `CPU_NMI` to take `Atari800_Instance *`.
- [x] Convert `CPU_GetStatus`, `CPU_PutStatus`, `CPU_StateSave`,
      `CPU_StateRead` to take `Atari800_Instance *` (updated callers in
      `statesav.c` and `monitor.c`). `CPU_GenerateIRQ` remains a macro that
      sets `CPU_IRQ` (aliased to the default instance; transitional).
- [x] Move state: `CPU_regPC/A/P/S/Y/X`, `CPU_IRQ`, `CPU_cim_encountered`,
      `CPU_rts_handler`, `CPU_remember_PC[]`, `CPU_remember_op[][]`,
      `CPU_remember_PC_curpos`, `CPU_remember_xpos[]`, `CPU_remember_JMP[]`,
      `CPU_remember_jmp_curpos`, `CPU_instruction_count[256]` (MONITOR_PROFILE)
      into `CPU_state_t`. The legacy global names are aliased to the default
      instance in `cpu.h`; `CPU_GO`/`CPU_NMI`/`CPU_Reset` redefine them to the
      instance they operate on.
- [ ] **FALCON_CPUASM follow-up:** the assembly `CPU_GO(int limit)` path
      (in `cpu.c`) still takes `int limit`; update it to match the new signature
      for Falcon builds.

### 1.4 Default-instance bridge (transitional)

- [x] Statically allocate a default `Atari800_Instance` in `atari.c` and expose
      it as `Atari800_default` so the legacy memory globals are always valid.
- [x] Implement `Atari800_NewInstance` / `Atari800_FreeInstance`.
- [ ] Implement `Atari800_FrameInstance` / `Atari800_ColdstartInstance` /
      `Atari800_WarmstartInstance` (multi-instance API, later phases).
- [x] Update callers in `atari.c` and `antic.c` to pass `Atari800_default` to
      `CPU_GO` / `CPU_NMI` / `CPU_Reset`.
- [x] Verify the tree builds (`make`) and the emulator starts (smoke run).

---

## Phase 2 — Chip modules

> **Status: in progress.** ANTIC, GTIA, POKEY, and PIA state migrations and
> function signature conversions are done (2026-09-15). All four chip modules
> now expose `*_Ctx(Atari800_Instance *inst, ...)` entry points; the legacy
> names are forwarding macros passing `Atari800_default` in each header, and
> `*_GetByte`/`*_PutByte` remain real functions (memory-map thunks) pinning
> the default instance. Build passes; 20 s smoke run (no-disk XL boot) clean,
> no CIM.

### 2.1 ANTIC — [`src/antic.h`](src/antic.h), [`src/antic.c`](src/antic.c)

- [x] Convert: `ANTIC_Initialise`, `ANTIC_Reset`, `ANTIC_Frame`,
      `ANTIC_GetByte`, `ANTIC_PutByte`, `ANTIC_GetDLByte`, `ANTIC_GetDLWord`,
      `ANTIC_UpdateArtifacting`, `ANTIC_VideoMemset`, `ANTIC_VideoPutByte`,
      `ANTIC_SetPrior`, `ANTIC_StateSave`, `ANTIC_StateRead`,
      `ANTIC_UpdateScanline`, `ANTIC_UpdateScanlinePrior`.
      Done 2026-09-15: each public function is now
      `ANTIC_*_Ctx(Atari800_Instance *inst, ...)` in `antic.c`, which pins the
      file-scope context (`A = &inst->antic`, `AI = inst`) via
      `ANTIC_PIN_CTX(inst)`; all legacy state aliases inside `antic.c` route
      through `A` and internal calls route through `AI`, so the `_Ctx` bodies
      operate on their own instance. In `antic.h` the legacy names are
      forwarding macros that pass `Atari800_default`, so not-yet-migrated
      callers are unchanged. `ANTIC_GetByte`/`ANTIC_PutByte` remain real
      functions (registered in the per-instance `MEMORY_readmap`/
      `MEMORY_writemap` function-pointer tables, which have a fixed
      context-free signature) that pin the default instance and forward to the
      `_Ctx` versions — they will become per-instance thunks when the memory
      map tables are converted. Build passes; smoke run (8 s, `pete.atr` boot)
      clean — the previously observed intermittent CIM did not reappear.
- [x] Move state: registers (`ANTIC_CHACTL`, `ANTIC_CHBASE`, `ANTIC_dlist`,
      `ANTIC_DMACTL`, `ANTIC_HSCROL`, `ANTIC_NMIEN`, `ANTIC_NMIST`,
      `ANTIC_PMBASE`, `ANTIC_VSCROL`), timing (`ANTIC_break_ypos`, `ANTIC_ypos`,
      `ANTIC_wsync_halt`, `ANTIC_xpos`, `ANTIC_xpos_limit`,
      `ANTIC_screenline_cpu_clock`), `ANTIC_artif_mode`, `ANTIC_artif_new`,
      `ANTIC_PENH_input`, `ANTIC_PENV_input`, `ANTIC_xe_ptr`, PM DMA flags,
      `ANTIC_delayed_wsync`, `ANTIC_cur_screen_pos`, `ANTIC_pal_blending`.
      Done 2026-09-15: all moved into `ANTIC_state_t`
      ([`src/instance.h`](src/instance.h)); `antic.c` aliases the legacy names
      to the default instance (via `Atari800_default->antic.*`), and `antic.h`
      replaces the `extern` declarations with the same aliases for
      not-yet-migrated callers (gtia.c, cpu.c, atari.c, memory.c, monitor.c,
      artifact.c, input.c, ui.c, statesav.c). Default-instance init values
      preserved in `atari.c` (`break_ypos = 999`, `PENV_input = 0xff`,
      `cur_screen_pos = ANTIC_NOT_DRAWING`). Remaining `static`s in `antic.c`
      (rendering state: `antic_memory`, `IR`, `anticmode`, `dctr`, scanline
      scratch, artifacting tables, etc.) are still file-scope — to be moved in
      the cross-cutting audit or a follow-up pass.
- [x] Keep shared: colour lookup tables `ANTIC_cl[]`, `ANTIC_lookup_gtia9/11`,
      `ANTIC_hires_lookup_l`, `ANTIC_cpu2antic_ptr`, `ANTIC_antic2cpu_ptr`
      *(shared, read-only)* — still file-scope globals in `antic.c`.
- [ ] **Watch:** one intermittent CIM crash (`Code $22 at $0000`, frame ~78,
      `pete.atr` boot) was observed once during smoke testing after this
      migration; not reproducible in 9 subsequent runs (8 s and 20 s). Re-test
      after the ANTIC function-signature conversion; if it reappears, suspect
      the transitional default-instance aliasing. Note: the `pete.atr` disk
      image was accidentally overwritten on 2026-09-15 by a mis-used
      `-config` flag during smoke testing (it was untracked/gitignored, so
      not recoverable from git); smoke runs now use a no-disk XL boot (and
      `valForth1.1.atr` when disk I/O coverage is needed). No CIM observed in
      the post-GTIA/POKEY/PIA-conversion 20 s run.

### 2.2 GTIA — [`src/gtia.h`](src/gtia.h), [`src/gtia.c`](src/gtia.c)

- [x] Convert: `GTIA_Initialise`, `GTIA_Frame`, `GTIA_NewPmScanline`,
      `GTIA_GetByte`, `GTIA_PutByte`, `GTIA_StateSave`, `GTIA_StateRead`,
      `GTIA_UpdatePmplColls`.
      Done 2026-09-15: each public function is now
      `GTIA_*_Ctx(Atari800_Instance *inst, ...)` in `gtia.c`, which pins the
      file-scope context (`G = &inst->gtia`, `GI = inst`) via
      `GTIA_PIN_CTX(inst)`; all state aliases inside `gtia.c` route through
      `G`. In `gtia.h` the legacy names are forwarding macros that pass
      `Atari800_default` (`GTIA_StateRead(version)` forwards the version
      argument). `GTIA_GetByte`/`GTIA_PutByte` remain real functions
      (registered in the per-instance memory map tables, fixed context-free
      signature) that pin the default instance and forward to the `_Ctx`
      versions — they will become per-instance thunks when the memory map
      tables are converted. Build passes; 20 s smoke run clean.
- [x] Move state: all colour/position/graphics registers (`GTIA_GRAFP0..3`,
      `GTIA_HPOSP0..3`, `GTIA_HPOSM0..3`, `GTIA_SIZEP0..3`, `GTIA_SIZEM`,
      `GTIA_COLPM0..3`, `GTIA_COLPF0..3`, `GTIA_COLBK`, `GTIA_GRACTL`,
      `GTIA_PRIOR`, `GTIA_VDELAY`), collision registers (`GTIA_M0PL..M3PL`,
      `GTIA_P0PL..P3PL`), `GTIA_pm_scanline[]`, `GTIA_pm_dirty`,
      `GTIA_collisions_mask_*`, `GTIA_TRIG[4]`, `GTIA_TRIG_latch[4]`,
      `GTIA_consol_override`, `GTIA_speaker`.
      Done 2026-09-15: all moved into `GTIA_state_t`
      ([`src/instance.h`](src/instance.h)); `gtia.c` and `gtia.h` alias the
      legacy names to `Atari800_default->gtia.*`. Default-instance init values
      preserved in `atari.c` (`pm_dirty = TRUE`, collision masks `0x0f`).
      `sizeof(GTIA_pm_scanline)` uses replaced with new
      `GTIA_PM_SCANLINE_SIZE` macro. Remaining file-scope statics (`consol`,
      `consol_mask`, `hposp_ptr`, `grafp_lookup`, NEW_CYCLE_EXACT temporary
      collision registers `P1PL_T` etc.) to be moved in a follow-up pass.
- [x] Keep shared: `GTIA_colour_translation_table[256]` *(shared, read-only)* —
      still a file-scope global in `gtia.c` (USE_COLOUR_TRANSLATION_TABLE).

### 2.3 POKEY — [`src/pokey.h`](src/pokey.h), [`src/pokey.c`](src/pokey.c)

- [x] Convert: `POKEY_Initialise`, `POKEY_Frame`, `POKEY_Scanline`,
      `POKEY_GetByte`, `POKEY_PutByte`, `POKEY_StateSave`, `POKEY_StateRead`,
      `POKEY_GetRandomCounter`, `POKEY_SetRandomCounter`.
      Done 2026-09-15: each public function is now
      `POKEY_*_Ctx(Atari800_Instance *inst, ...)` in `pokey.c`, which pins
      the file-scope context (`PK = &inst->pokey`, `PKI = inst`) via
      `POKEY_PIN_CTX(inst)`; all state aliases inside `pokey.c` route through
      `PK`. In `pokey.h` the legacy names are forwarding macros that pass
      `Atari800_default`. `POKEY_GetByte`/`POKEY_PutByte` remain real
      functions (registered in the per-instance memory map tables, fixed
      context-free signature) that pin the default instance and forward to
      the `_Ctx` versions — same thunk consideration as ANTIC/GTIA. Build
      passes; 20 s smoke run clean.
- [x] Move state: `POKEY_KBCODE`, `POKEY_IRQST`, `POKEY_IRQEN`, `POKEY_SKSTAT`,
      `POKEY_SKCTL`, `POKEY_DELAYED_SERIN_IRQ`, `POKEY_DELAYED_SEROUT_IRQ`,
      `POKEY_DELAYED_XMTDONE_IRQ`, `POKEY_irq_at_xpos`, `POKEY_irq_pending_mask`,
      `POKEY_POT_input[8]`, `POKEY_AUDF[]`, `POKEY_AUDC[]`, `POKEY_AUDCTL[]`,
      `POKEY_DivNIRQ[]`, `POKEY_DivNMax[]`, `POKEY_Base_mult[]`.
      Done 2026-09-15: all moved into `POKEY_state_t`
      ([`src/instance.h`](src/instance.h)); `pokey.c` and `pokey.h` alias the
      legacy names to `Atari800_default->pokey.*`. Default-instance init
      preserved in `atari.c` (`POT_input` = 228 × 8). Remaining file-scope
      state in `pokey.c` (`POKEY_SERIN`, `irq_15khz_phase`, `pot_scanline`,
      timer/divisor internals) to be moved in a follow-up pass.
- [x] Keep shared: `POKEY_poly9_lookup[]`, `POKEY_poly17_lookup[]`
      *(shared, read-only)* — still file-scope globals in `pokey.c`.

### 2.4 PIA — [`src/pia.h`](src/pia.h), [`src/pia.c`](src/pia.c)

- [x] Convert: `PIA_Initialise`, `PIA_Reset`, `PIA_GetByte`, `PIA_PutByte`,
      `PIA_StateSave`, `PIA_StateRead`, `PIA_SetCA1`, `PIA_SetCB1`,
      `update_PIA_IRQ`.
      Done 2026-09-15: each public function is now
      `PIA_*_Ctx(Atari800_Instance *inst, ...)` (and
      `update_PIA_IRQ_Ctx`) in `pia.c`, which pins the file-scope context
      (`PI = &inst->pia`, `PINST = inst`) via `PIA_PIN_CTX(inst)`; all state
      aliases inside `pia.c` route through `PI`. In `pia.h` the legacy names
      are forwarding macros that pass `Atari800_default`
      (`PIA_StateRead(version)` forwards the version argument).
      `PIA_GetByte`/`PIA_PutByte` remain real functions (registered in the
      per-instance memory map tables, fixed context-free signature) that pin
      the default instance and forward to the `_Ctx` versions — same thunk
      consideration as ANTIC/GTIA/POKEY. Build passes; 20 s smoke run clean.
- [x] Move state: `PIA_PACTL`, `PIA_PBCTL`, `PIA_PORTA`, `PIA_PORTB`,
      `PIA_PORTA_mask`, `PIA_PORTB_mask`, `PIA_PORT_input[2]`, `PIA_CA1`,
      `PIA_CB1`, `PIA_CA2`, `PIA_CB2`, `PIA_IRQ`.
      Done 2026-09-15: all moved into `PIA_state_t`
      ([`src/instance.h`](src/instance.h)); `pia.c` and `pia.h` alias the
      legacy names to `Atari800_default->pia.*`. Default-instance init
      preserved in `atari.c` (`CA1/CA2/CB1/CB2 = 1`). The internal
      `*_negpending`/`*_pospending` edge-detect flags were made `static` in
      `pia.c` (not yet in `PIA_state_t`; follow-up pass).

---

## Phase 3 — Peripheral modules

> **Status: in progress.** SIO, Devices, Cartridge, Cassette, PBI, the
> ESC/Binload handlers, and RTIME done (2026-09-15). All of them now expose
> `*_Ctx(Atari800_Instance *inst, ...)` entry points with legacy-name
> forwarding macros in their headers. Verified with the Acid800 suite
> (`-atari test/acid800.atr -acid800 test/acid800.expected`): results
> identical to the pre-refactor baseline (23 success / 28 expected failures /
> 2 skipped; the 2 FAILs — "MMU: XL banking" and "suite totals changed" — and
> the NEW "GTIA: Defrrupt control test" reproduce identically on unmodified
> HEAD, so they are pre-existing, not refactor regressions). See
> [`docs/acid800-expected-results.md`](acid800-expected-results.md) for the
> full expected-results reference and how to run the suite.

### 3.1 SIO / Disk drives — [`src/sio.h`](src/sio.h), [`src/sio.c`](src/sio.c)

- [x] Convert: `SIO_Handler`, `SIO_Mount`, `SIO_Dismount`, `SIO_DisableDrive`,
      `SIO_RotateDisks`, `SIO_ChkSum`, `SIO_SwitchCommandFrame`, `SIO_PutByte`,
      `SIO_GetByte`, `SIO_Initialise`, `SIO_Exit`, `SIO_ReadStatusBlock`,
      `SIO_FormatDisk`, `SIO_SizeOfSector`, `SIO_ReadSector`, `SIO_DriveStatus`,
      `SIO_WriteStatusBlock`, `SIO_WriteSector`, `SIO_StateSave`, `SIO_StateRead`.
      Done 2026-09-15: each is now `SIO_*_Ctx(Atari800_Instance *inst, ...)`
      in `sio.c`, pinning the file-scope context (`SIOp = &inst->sio`,
      `SIOi = inst`) via `SIO_PIN_CTX(inst)`; all state aliases inside `sio.c`
      route through `SIOp`. In `sio.h` the legacy names are forwarding macros
      passing `Atari800_default`. Exceptions: `SIO_ChkSum` is stateless
      (signature unchanged); `SIO_Handler` remains a real function (it is
      registered as a function pointer by `ESC_AddEscRts` in `esc.c`) that
      pins the default instance and forwards to `SIO_Handler_Ctx`. Build
      passes; Acid800 results identical to pre-refactor baseline.
- [x] Move state: `SIO_status[256]`, `SIO_drive_status[8]`,
      `SIO_filename[8][FILENAME_MAX]`, `SIO_last_op`, `SIO_last_op_time`,
      `SIO_last_drive`, `SIO_last_sector`, `SIO_format_sectorcount[8]`,
      `SIO_format_sectorsize[8]`, and per-drive file handles/buffers.
      Done 2026-09-15: `SIO_state_t` is now defined concretely in
      [`src/instance.h`](src/instance.h) (moved the `SIO_MAX_DRIVES` define and
      `SIO_UnitStatus` enum there; `sio.h` includes `instance.h`) and is
      **embedded by value** in `Atari800_Instance` (`.sio`), so the default
      instance's state is valid before any allocation. All public state plus the
      per-drive internals (`boot_sectors_type`, `image_type`, `disk` FILE*s,
      `sectorcount`, `sectorsize`, `io_success`, `additional_info`) and the
      serial-frame state (`CommandFrame`, `CommandIndex`, `DataBuffer`,
      `DataIndex`, `TransferStatus`, `ExpectedBytes`, `delay_counter`,
      `last_ypos`) moved in; `sio.c`/`sio.h` alias the legacy names to
      `Atari800_default->sio.*`. `ui.c`'s `DiskManagement()` menu array made
      non-static (it was initialised from `SIO_filename` at static-init time).
      Remaining file-scope: `sio_tmpbuf` scratch buffers (per-call transient),
      `ignore_header_writeprotect` (config-ish).

### 3.2 Devices (H:/P:/R:/B: patches) — [`src/devices.h`](src/devices.h), [`src/devices.c`](src/devices.c)

- [x] Convert: `Devices_Initialise`, `Devices_Exit`, `Devices_PatchOS`,
      `Devices_Frame`, `Devices_UpdatePatches`, `Devices_SkipDeviceName`,
      `Devices_H_CountOpen`, `Devices_H_CloseAll`, `Devices_SetPrintCommand`,
      `Devices_UpdateHATABSEntry`, `Devices_RemoveHATABSEntry`.
      Done 2026-09-15: each is now `Devices_*_Ctx(Atari800_Instance *inst, ...)`
      in `devices.c`, pinning the file-scope context (`DEV = &inst->devices`,
      `DEVi = inst`) via `DEVICES_PIN_CTX(inst)`; all state aliases inside
      `devices.c` route through `DEV`. In `devices.h` the legacy names are
      forwarding macros passing `Atari800_default`. Build passes; Acid800
      results identical to pre-refactor baseline.
- [x] Move state: `Devices_enable_h_patch`, `Devices_enable_p_patch`,
      `Devices_enable_r_patch`, `Devices_enable_b_patch`,
      `Devices_atari_h_dir[4][FILENAME_MAX]`, `Devices_h_read_only`,
      `Devices_h_exe_path`, `Devices_h_device_name`,
      `Devices_h_current_dir[4][]`, `Devices_print_command[256]`, `dev_b_status`.
      Done 2026-09-15: `Devices_state_t` defined concretely in
      [`src/instance.h`](src/instance.h) (including `struct DEV_B`, moved there
      from `devices.h`) and **embedded by value** in `Atari800_Instance`
      (`.devices`). The H:-device internals (`h_fp[8]`, `h_textmode`,
      `h_lastbyte`, `h_wascr`, `h_lastop`, `h_iocb`, `h_devnum`,
      `atari_filename`, `new_filename`, `atari_path`, `host_path`) moved too;
      `devices.c`/`devices.h` alias the legacy names to
      `Atari800_default->devices.*`. Default-instance init preserved in
      `atari.c` (`enable_h/p_patch = TRUE`, `h_read_only = TRUE`,
      `h_exe_path = "H1:>DOS;>DOS"`, `h_device_name = 'H'`,
      `print_command = "lpr %s"`). `ui.c`'s `HDeviceStatus()` and `Settings()`
      menu arrays made non-static (static-init from Devices state). Remaining
      file-scope: `devbug` (debug flag), `h_tmpbuf` scratch, platform
      directory-enumeration statics (`dir_path`, `dp`, ...).

### 3.3 Cartridge — [`src/cartridge.h`](src/cartridge.h), [`src/cartridge.c`](src/cartridge.c)

- [x] Convert: `CARTRIDGE_Initialise`, `CARTRIDGE_Exit`, `CARTRIDGE_Insert`,
      `CARTRIDGE_InsertAutoReboot`, `CARTRIDGE_Insert_Second`, `CARTRIDGE_SetType`,
      `CARTRIDGE_SetTypeAutoReboot`, `CARTRIDGE_Remove`, `CARTRIDGE_RemoveAutoReboot`,
      `CARTRIDGE_Remove_Second`, `CARTRIDGE_ColdStart`, `CARTRIDGE_GetByte`,
      `CARTRIDGE_PutByte`, `CARTRIDGE_StateSave`, `CARTRIDGE_StateRead`,
      `CARTRIDGE_BountyBob1GetByte`.
      Done 2026-09-15: each is now
      `CARTRIDGE_*_Ctx(Atari800_Instance *inst, ...)` in `cartridge.c`
      (including `CARTRIDGE_ReadConfig`/`WriteConfig`/`UpdateState`); the
      existing `CARTRIDGE_PIN_CTX()` was extended to take the instance
      (`CARTp = &inst->cartridge`, `CARTi = inst`) and still lazily pins
      `active_cart` to `&main` (the instance is zero-initialised). All state
      aliases inside `cartridge.c` route through `CARTp`. In `cartridge.h`
      the legacy names are forwarding macros passing `Atari800_default`.
      `CARTRIDGE_GetByte`/`CARTRIDGE_PutByte` and the BountyBob1/2 and 5200
      SuperCart handlers remain real functions (registered in the per-instance
      memory map tables, fixed context-free signature) that pin the default
      instance and forward to the `_Ctx` versions. Build passes; Acid800
      results identical to pre-refactor baseline.
- [x] Move state: `CARTRIDGE_main`, `CARTRIDGE_piggyback` (including the
      `image` buffers), `CARTRIDGE_autoreboot`.
      Done 2026-09-15: `CARTRIDGE_image_t` typedef moved from `cartridge.h`
      into [`src/instance.h`](src/instance.h); `Cartridge_state_t` (main,
      piggyback, autoreboot, and the internal `active_cart` pointer) is
      **embedded by value** in `Atari800_Instance` (`.cartridge`);
      `cartridge.c`/`cartridge.h` alias the legacy names to
      `Atari800_default->cartridge.*`. `active_cart` is lazily pinned to
      `&...main` via `CARTRIDGE_PIN_CTX()` at the entry points that can reach
      it (UpdateState, Get/PutByte, BountyBob/5200 handlers, ColdStart,
      Initialise, StateSave/Read) because the default instance is
      zero-initialised. Defaults (`autoreboot = TRUE`, both carts
      `CARTRIDGE_NONE`) match the old static initialisers via zero-init plus
      the `atari.c` initializer.

### 3.4 Cassette — [`src/cassette.h`](src/cassette.h), [`src/cassette.c`](src/cassette.c)

- [x] Convert: `CASSETTE_Initialise`, `CASSETTE_Exit`, `CASSETTE_Insert`,
      `CASSETTE_Remove`, `CASSETTE_CreateCAS`, `CASSETTE_ToggleWriteProtect`,
      `CASSETTE_ToggleRecord`, `CASSETTE_Seek`, `CASSETTE_IOLineStatus`,
      `CASSETTE_GetByte`, `CASSETTE_PutByte`, `CASSETTE_TapeMotor`,
      `CASSETTE_AddScanLine`, `CASSETTE_ResetPOKEY`, `CASSETTE_GetSize`,
      `CASSETTE_GetPosition`, `CASSETTE_AddGap`, `CASSETTE_ReadToMemory`,
      `CASSETTE_WriteFromMemory`, `CASSETTE_LeaderLoad`, `CASSETTE_LeaderSave`.
      Done 2026-09-15: each is now
      `CASSETTE_*_Ctx(Atari800_Instance *inst, ...)` in `cassette.c` (also
      `CASSETTE_ReadConfig`/`WriteConfig`), pinning the file-scope context
      (`CAS = &inst->cassette`, `CASi = inst`) via `CASSETTE_PIN_CTX(inst)`;
      all state aliases inside `cassette.c` route through `CAS`. In
      `cassette.h` the legacy names are forwarding macros passing
      `Atari800_default`. Build passes; Acid800 results identical to
      pre-refactor baseline.
- [x] Move state: `CASSETTE_filename`, `CASSETTE_description`, `CASSETTE_status`,
      `CASSETTE_hold_start`, `CASSETTE_hold_start_on_reboot`,
      `CASSETTE_press_space`, `CASSETTE_write_protect`, `CASSETTE_record`,
      `CASSETTE_readable`, `CASSETTE_writable`, and tape position/file state.
      Done 2026-09-15: `CASSETTE_status_t` enum moved from `cassette.h` into
      [`src/instance.h`](src/instance.h) (the `IMG_TAPE_t` forward typedef is
      now guarded by `IMG_TAPE_T_DEFINED` in both `instance.h` and
      `img_tape.h` to avoid a pedantic redefinition); `Cassette_state_t`
      (public state plus internals `cassette_file`, `event_time_left`,
      `pending_serin`, `passing_gap`, `pending_serin_byte`, `serin_byte`,
      `cassette_gapdelay`, `cassette_motor`, `eof_of_tape`) is **embedded by
      value** in `Atari800_Instance` (`.cassette`); `cassette.c`/`cassette.h`
      alias the legacy names to `Atari800_default->cassette.*`. `ui.c`'s
      `TapeManagement()` menu array made non-static (static-init from
      `CASSETTE_description`).

### 3.5 PBI — [`src/pbi.h`](src/pbi.h), [`src/pbi.c`](src/pbi.c)

- [x] Convert: `PBI_Initialise`, `PBI_Exit`, `PBI_Reset`, `PBI_D1GetByte`,
      `PBI_D1PutByte`, `PBI_D6GetByte`, `PBI_D6PutByte`, `PBI_D7GetByte`,
      `PBI_D7PutByte`, `PBI_StateSave`, `PBI_StateRead`
      (also `PBI_ReadConfig`/`PBI_WriteConfig`).
      Done 2026-09-15: each is now `PBI_*_Ctx(Atari800_Instance *inst, ...)`
      in `pbi.c`, pinning the file-scope context (`PB = &inst->pbi`,
      `PBIi = inst`) via `PBI_PIN_CTX(inst)`; all state aliases inside
      `pbi.c` route through `PB` (the header's `PBI_IRQ`/`PBI_D6D7ram`
      aliases are `#undef`'d and re-pointed to `PB` inside `pbi.c`).
      In `pbi.h` the legacy names are forwarding macros passing
      `Atari800_default`. Unlike the chip modules, the D1/D6/D7
      Get/PutByte functions are dispatched from the
      `MEMORY_HwGetByte`/`MEMORY_HwPutByte` switch statements (not
      function-pointer tables), so no context-free real-function thunks
      were needed — memory.c's calls go through the forwarding macros and
      will be re-routed per-instance when the hardware dispatch layer is
      converted. `PBI_Reset_Ctx` calls `PBI_D1PutByte_Ctx(inst, ...)`
      directly (not the forwarding macro). Note: `fp_active` — the
      floating-point-ROM reactivation flag that was a function-local
      `static` in `PBI_D1PutByte` — moved into `PBI_state_t` (initialised
      `TRUE` in the default-instance initializer in `atari.c`) so the
      TRUE-on-first-call semantics are preserved per instance. Calls into
      the not-yet-converted PBI sub-modules (MIO, BB, XLD, PROTO80, AF80,
      BIT3) still touch those modules' own file-scope globals.
      Build passes; Acid800 results identical to pre-refactor baseline;
      20 s no-disk smoke run clean (no CIM).
- [x] Move state: `PBI_IRQ`, `PBI_D6D7ram` (plus the previously file-scope
      `D1FF_LATCH` and `fp_active` internals).
      Done 2026-09-15: `PBI_state_t` defined concretely in
      [`src/instance.h`](src/instance.h) and **embedded by value** in
      `Atari800_Instance` (`.pbi`, replacing the earlier forward-declared
      pointer member); `pbi.h` aliases `PBI_IRQ`/`PBI_D6D7ram` to
      `Atari800_default->pbi.*`. Default-instance init preserved in
      `atari.c` (`fp_active = TRUE`). Sub-modules that write
      `PBI_D6D7ram`/`PBI_IRQ` directly (pbi_xld.c, pbi_bb.c,
      pbi_proto80.c) continue to work unchanged via the header aliases
      (transitional: they hit the default instance).

### 3.6 PBI sub-modules

- [ ] **PBI_BB** — [`src/pbi_bb.h`](src/pbi_bb.h): `PBI_BB_Menu`, `PBI_BB_Frame`,
      `PBI_BB_Initialise`, `PBI_BB_Exit`, `PBI_BB_D1GetByte`, `PBI_BB_D1PutByte`,
      `PBI_BB_D6GetByte`, `PBI_BB_D6PutByte`, `PBI_BB_StateSave`,
      `PBI_BB_StateRead`; state `PBI_BB_enabled`.
- [ ] **PBI_MIO** — [`src/pbi_mio.h`](src/pbi_mio.h): `PBI_MIO_Initialise`,
      `PBI_MIO_Exit`, `PBI_MIO_D1GetByte`, `PBI_MIO_D1PutByte`,
      `PBI_MIO_D6GetByte`, `PBI_MIO_D6PutByte`, `PBI_MIO_StateSave`,
      `PBI_MIO_StateRead`; state `PBI_MIO_enabled`.
- [ ] **PBI_PROTO80** — [`src/pbi_proto80.h`](src/pbi_proto80.h):
      `PBI_PROTO80_Initialise`, `PBI_PROTO80_Exit`, `PBI_PROTO80_D1GetByte`,
      `PBI_PROTO80_D1PutByte`, `PBI_PROTO80_D1ffPutByte`,
      `PBI_PROTO80_GetPixels`; state `PBI_PROTO80_enabled`.
- [ ] **PBI_SCSI** — [`src/pbi_scsi.h`](src/pbi_scsi.h): `PBI_SCSI_PutByte`,
      `PBI_SCSI_GetByte`, `PBI_SCSI_PutSEL`, `PBI_SCSI_PutACK`; state
      `PBI_SCSI_CD/MSG/IO/BSY/REQ/SEL/ACK`, `PBI_SCSI_disk`.
- [ ] **PBI_XLD** — [`src/pbi_xld.h`](src/pbi_xld.h): all `PBI_XLD_*` functions
      and state.

### 3.7 Other peripherals

- [x] **RTIME** — [`src/rtime.h`](src/rtime.h), [`src/rtime.c`](src/rtime.c):
      `RTIME_Initialise`, `RTIME_GetByte`, `RTIME_PutByte`
      (also `RTIME_ReadConfig`/`RTIME_WriteConfig`); state `RTIME_enabled`.
      Done 2026-09-15: each is now `RTIME_*_Ctx(Atari800_Instance *inst, ...)`
      in `rtime.c`, pinning the file-scope context (`RT = &inst->rtime`,
      `RTI = inst`) via `RTIME_PIN_CTX(inst)`; the state aliases inside
      `rtime.c` route through `RT`. In `rtime.h` the legacy names are
      forwarding macros passing `Atari800_default`. `cartridge.c`'s
      `CARTRIDGE_*_Ctx` D5-page dispatch passes its own instance (`CARTi`)
      to `RTIME_GetByte_Ctx`/`RTIME_PutByte_Ctx`; `cfg.c`, `atari.c` and
      `ui.c` go through the forwarding macros (transitional). State
      `RTIME_enabled` plus the internals (`rtime_state`, `rtime_tmp`,
      `rtime_tmp2`, `regset[16]`) moved into `RTIME_state_t`
      ([`src/instance.h`](src/instance.h)), **embedded by value** in
      `Atari800_Instance` (`.rtime`); `RTIME_enabled = 1` is preserved in
      the default-instance initializer in `atari.c`. Build passes;
      Acid800 results identical to pre-refactor baseline; 20 s smoke run
      clean.
- [ ] **XEP80** — [`src/xep80.h`](src/xep80.h): `XEP80_SetEnabled`, `XEP80_GetBit`,
      `XEP80_PutBit`, `XEP80_ChangeColors`, `XEP80_StateSave`, `XEP80_StateRead`,
      `XEP80_Initialise`; state `XEP80_enabled`, `XEP80_port`, `XEP80_scrn_height`,
      `XEP80_char_height`, `XEP80_screen_1[]`, `XEP80_screen_2[]`.
- [ ] **AF80** — [`src/af80.h`](src/af80.h): `AF80_Initialise`, `AF80_Exit`,
      `AF80_InsertRightCartridge`, `AF80_D5GetByte`, `AF80_D5PutByte`,
      `AF80_D6GetByte`, `AF80_D6PutByte`, `AF80_GetPixels`, `AF80_Reset`; state
      `AF80_enabled`, `AF80_palette[16]`.
- [ ] **BIT3** — [`src/bit3.h`](src/bit3.h): `BIT3_Initialise`, `BIT3_Exit`,
      `BIT3_InsertRightCartridge`, `BIT3_D5GetByte`, `BIT3_D5PutByte`,
      `BIT3_D6GetByte`, `BIT3_D6PutByte`, `BIT3_GetPixels`, `BIT3_Reset`; state
      `BIT3_enabled`, `BIT3_palette[2]`.
- [ ] **IDE** — [`src/ide.h`](src/ide.h): `IDE_Initialise`, `IDE_Exit`,
      `IDE_GetByte`, `IDE_PutByte`; state `IDE_enabled` and IDE disk state.
- [ ] **Voicebox** — [`src/voicebox.h`](src/voicebox.h): `VOICEBOX_Initialise`,
      `VOICEBOX_SKCTLPutByte`, `VOICEBOX_SEROUTPutByte`; state `VOICEBOX_enabled`,
      `VOICEBOX_ii`.
- [ ] **Votrax** — [`src/votraxsnd.h`](src/votraxsnd.h): all `VOTRAXSND_*`
      functions and state.
- [ ] **Pokeyrec** — [`src/pokeyrec.h`](src/pokeyrec.h): `POKEYREC_Recorder`,
      `POKEYREC_Initialise`, `POKEYREC_Exit`.
- [ ] **Rdevice** — [`src/rdevice.h`](src/rdevice.h): all `RDevice_*` functions
      and state.

### 3.8 ESC / Binload handlers

- [x] **ESC** — [`src/esc.h`](src/esc.h), [`src/esc.c`](src/esc.c):
      `ESC_Add`, `ESC_AddEscRts`, `ESC_AddEscRts2`, `ESC_Remove`, `ESC_Run`,
      `ESC_PatchOS`, `ESC_ClearAll`, `ESC_UpdatePatches`; state
      `ESC_enable_sio_patch` and the escape tables.
      Done 2026-09-15: each is now `ESC_*_Ctx(Atari800_Instance *inst, ...)`
      in `esc.c`, pinning the file-scope context (`E = &inst->esc`,
      `ESCi = inst`) via `ESC_PIN_CTX(inst)`; the state aliases inside
      `esc.c` route through `E`. In `esc.h` the legacy names are forwarding
      macros passing `Atari800_default`. `cpu.c` passes its own instance to
      `ESC_Run_Ctx` (the escape machinery runs inside the CPU decoder's
      context); all other callers (memory.c, devices.c, cassette.c, sio.c,
      atari.c, ui.c, binload.c) go through the forwarding macros
      (transitional: they hit the default instance) and will be re-routed as
      their enclosing modules become instance-aware. The `esc_address[256]`/
      `esc_function[256]` tables moved into `ESC_state_t`
      ([`src/instance.h`](src/instance.h)) along with the
      `ESC_FunctionType` typedef (moved there from `esc.h` so instance.h
      can embed the tables); `ESC_enable_sio_patch = TRUE` is preserved in
      the default-instance initializer in `atari.c`. **Deferred:** the
      registered handler functions (`SIO_Handler`, `CassetteLeaderLoad`,
      `loader_cont`, the Devices/P: handlers, ...) remain context-free
      thunks pinning the default instance — converting them requires the
      ESC dispatch to carry a context (or per-instance registrations) and
      touches every handler module at once; revisit in the cross-cutting
      audit. Build passes; Acid800 results identical to pre-refactor
      baseline; 20 s smoke run clean.
- [x] **Binload** — [`src/binload.h`](src/binload.h), [`src/binload.c`](src/binload.c):
      `BINLOAD_Loader`, `BINLOAD_LoaderStart`.
      Done 2026-09-15: both are now
      `BINLOAD_*_Ctx(Atari800_Instance *inst, ...)` in `binload.c`, pinning
      the file-scope context (`BL = &inst->binload`, `BLi = inst`) via
      `BL_PIN_CTX(inst)`; the state aliases inside `binload.c` route through
      `BL`. In `binload.h` the legacy names are forwarding macros passing
      `Atari800_default`. `sio.c`'s `SIO_ReadSector_Ctx` boot-sector path
      passes its own instance to `BINLOAD_LoaderStart_Ctx`. State
      `BINLOAD_bin_file`, `BINLOAD_start_binloading`, `BINLOAD_loading_basic`,
      `BINLOAD_slow_xex_loading`, `BINLOAD_wait_active`,
      `BINLOAD_pause_loading` plus the internal slow-XEX state
      (`instr_elapsed`, `from`, `to`, `init2e3`, `segfinished`) moved into
      `Binload_state_t` ([`src/instance.h`](src/instance.h)) and is
      **embedded by value** in `Atari800_Instance` (`.binload`); defaults
      (all zero/FALSE, `segfinished = TRUE` in the `atari.c` initializer)
      match the old static initialisers. The internal `loader_cont` escape
      handler is registered via `ESC_Add` as a context-free callback
      (`static void loader_cont(void)` thunk pinning the default instance
      over `loader_cont_Ctx`) — same deferred ESC-handler consideration as
      above. Build passes; Acid800 results identical to pre-refactor
      baseline; 20 s smoke run clean.

### 3.9 Input — [`src/input.h`](src/input.h), [`src/input.c`](src/input.c)

- [ ] Convert: `INPUT_Initialise`, `INPUT_Exit`, `INPUT_Frame`, `INPUT_Scanline`,
      `INPUT_SelectMultiJoy`, `INPUT_CenterMousePointer`, `INPUT_DrawMousePointer`,
      `INPUT_Recording`, `INPUT_Playingback`, `INPUT_RecordInt`, `INPUT_PlaybackInt`.
- [ ] Move state: `INPUT_key_code`, `INPUT_key_shift`, `INPUT_key_consol`,
      `INPUT_joy_autofire[4]`, `INPUT_joy_block_opposite_directions`,
      `INPUT_joy_multijoy`, `INPUT_joy_5200_min/center/max`, mouse state
      (`INPUT_mouse_mode`, `INPUT_mouse_port`, `INPUT_mouse_delta_x/y`,
      `INPUT_mouse_buttons`, `INPUT_mouse_speed`, `INPUT_mouse_pot_min/max`,
      `INPUT_mouse_pen_ofs_h/v`, `INPUT_mouse_joy_inertia`, `INPUT_direct_mouse`),
      `INPUT_cx85`.

---

## Phase 4 — Output / input / platform layer

### 4.1 Screen — [`src/screen.h`](src/screen.h), [`src/screen.c`](src/screen.c)

- [ ] Convert: `Screen_Initialise`, `Screen_DrawAtariSpeed`, `Screen_DrawDiskLED`,
      `Screen_Draw1200LED`, `Screen_DrawMultimediaStats`,
      `Screen_FindScreenshotFilename`, `Screen_SaveScreenshot`,
      `Screen_SaveNextScreenshot`, `Screen_EntireDirty`, `Screen_SetStatusText`,
      `Screen_DrawStatusText`.
- [ ] Move state: `Screen_atari` (framebuffer), `Screen_atari_b/1/2` (BITPL_SCR),
      `Screen_dirty` (DIRTYRECT), `Screen_visible_x1/y1/x2/y2`,
      `Screen_show_atari_speed`, `Screen_show_disk_led`,
      `Screen_show_sector_counter`, `Screen_show_1200_leds`,
      `Screen_show_multimedia_stats`.

### 4.2 Colours / video filters

- [ ] **Colours** — [`src/colours.h`](src/colours.h): `Colours_*` functions; state
      `Colours_table[256]`, `Colours_setup`, `Colours_NTSC_setup`, `Colours_PAL_setup`.
- [ ] **Videomode** — [`src/videomode.h`](src/videomode.h): `VIDEOMODE_*` functions
      and state.
- [ ] **Pal_blending** — [`src/pal_blending.h`](src/pal_blending.h):
      `PAL_BLENDING_*` functions.
- [ ] **Filter_ntsc** — [`src/filter_ntsc.h`](src/filter_ntsc.h): `FILTER_NTSC_*`
      functions; state `FILTER_NTSC_setup`, `FILTER_NTSC_emu`.
- [ ] **Artifact** — [`src/artifact.h`](src/artifact.h): `ARTIFACT_*` functions
      and state.
- [ ] **File_export** — [`src/file_export.h`](src/file_export.h): `FILE_EXPORT_*`
      functions and state.

### 4.3 Sound

- [ ] **Sound** — [`src/sound.h`](src/sound.h): `Sound_Initialise`, `Sound_Exit`,
      `Sound_Update`, `Sound_Pause`, `Sound_Continue`, `Sound_Setup`,
      `Sound_Callback`, `Sound_SetLatency`, `Sound_AdjustSpeed`; state
      `Sound_desired`, `Sound_out`, `Sound_enabled`, `Sound_latency`.
- [ ] **Pokeysnd** — [`src/pokeysnd.h`](src/pokeysnd.h): `POKEYSND_*` functions
      and state.
- [ ] **Mzpokeysnd** — [`src/mzpokeysnd.h`](src/mzpokeysnd.h): `MZPOKEYSND_*`
      functions and state.

### 4.4 Top-level driver — [`src/atari.h`](src/atari.h), [`src/atari.c`](src/atari.c)

- [ ] Convert: `Atari800_Initialise`, `Atari800_Frame`, `Atari800_Coldstart`,
      `Atari800_Warmstart`, `Atari800_InitialiseMachine`, `Atari800_Exit`,
      `Atari800_ErrExit`, `Atari800_Sync`, `Atari800_LoadImage`,
      `Atari800_StateSave`, `Atari800_StateRead`, `Atari800_SetTVMode`,
      `Atari800_SetMachineType`, `Atari800_UpdateJumper`,
      `Atari800_UpdateKeyboardDetached`.
- [ ] Move state: `Atari800_machine_type`, `Atari800_builtin_basic`,
      `Atari800_keyboard_leds`, `Atari800_f_keys`, `Atari800_jumper`,
      `Atari800_builtin_game`, `Atari800_keyboard_detached`, `Atari800_tv_mode`,
      `Atari800_disable_basic`, `Atari800_os_version`, `Atari800_display_screen`,
      `Atari800_nframes`, `Atari800_refresh_rate`,
      `Atari800_collisions_in_skipped_frames`, `Atari800_turbo`,
      `Atari800_turbo_speed`, `Atari800_start_in_monitor`,
      `Atari800_auto_frameskip`.
- [ ] Keep shared: `verbose`, `sigint_flag`, `dl_dir` (process-global).

### 4.5 State save/load — [`src/statesav.h`](src/statesav.h), [`src/statesav.c`](src/statesav.c)

- [ ] Convert: `StateSav_SaveAtariState`, `StateSav_ReadAtariState`,
      `StateSav_SaveUBYTE`, `StateSav_SaveUWORD`, `StateSav_SaveINT`,
      `StateSav_SaveFNAME`, `StateSav_ReadUBYTE`, `StateSav_ReadUWORD`,
      `StateSav_ReadINT`, `StateSav_ReadFNAME`, `StateSav_Tell`.
- [ ] Make the save/read stream per-instance.

### 4.6 Monitor / debugger — [`src/monitor.h`](src/monitor.h), [`src/monitor.c`](src/monitor.c)

- [ ] Convert: `MONITOR_Run`, `MONITOR_Exit`, `MONITOR_ShowState`, `MONITOR_BBRK_on`,
      `MONITOR_BPC`.
- [ ] Move state: `MONITOR_break_addr`, `MONITOR_break_step`, `MONITOR_break_ret`,
      `MONITOR_break_brk`, `MONITOR_ret_nesting`, `MONITOR_breakpoint_table[]`,
      `MONITOR_breakpoint_table_size`, `MONITOR_breakpoints_enabled`,
      `MONITOR_coverage[]`, `MONITOR_coverage_insns`, `MONITOR_coverage_cycles`.
- [ ] Keep shared: `MONITOR_optype6502[256]` *(shared, read-only)*.

### 4.7 Config / ROM / util / log

- [ ] **Cfg** — [`src/cfg.h`](src/cfg.h): `CFG_*` functions (config file handling
      stays process-global, but per-instance options must be routed to the right
      instance).
- [ ] **Sysrom** — [`src/sysrom.h`](src/sysrom.h): `SYSROM_*` functions; ROM image
      buffers *(shared, read-only)*.
- [ ] **Util** — [`src/util.h`](src/util.h): `Util_*` helpers (mostly stateless;
      audit for any hidden global state).
- [ ] **Log** — [`src/log.h`](src/log.h): `Log_*` functions — **stays
      process-global** (shared).

---

## Phase 5 — libatari800 library

- [ ] **api.c** — `libatari800_init`, `libatari800_next_frame`, and the
      `libatari800_get_*` accessors: re-implement on top of the instance API.
- [ ] **init.c** — instance-aware initialization.
- [ ] **input.c** — per-instance input array (`LIBATARI800_Input_array`).
- [ ] **video.c** — per-instance framebuffer access.
- [ ] **sound.c** — per-instance audio.
- [ ] **statesav.c** — per-instance state save/load.
- [ ] **main.c**, **exit.c**, **guess_settings.c** — update to instance API.
- [ ] Add multi-instance entry points (`libatari800_new_instance`, etc.).

---

## Phase 6 — Platform ports

- [ ] **SDL** — [`src/sdl/`](src/sdl/): `main.c`, `init.c`, `input.c`, `sound.c`,
      `video.c`, `video_gl.c`, `video_sw.c`, `palette.c`.
- [ ] **X11** — [`src/atari_x11.c`](src/atari_x11.c).
- [ ] **Raspberry Pi** — [`src/atari_rpi.c`](src/atari_rpi.c).
- [ ] **Curses** — [`src/atari_curses.c`](src/atari_curses.c).
- [ ] **BASIC** — [`src/atari_basic.c`](src/atari_basic.c).
- [ ] **PS2** — [`src/atari_ps2.c`](src/atari_ps2.c).
- [ ] **Amiga** — [`src/amiga/`](src/amiga/).
- [ ] **Android** — [`src/android/`](src/android/).
- [ ] **macOS** — [`src/macosx/`](src/macosx/).
- [ ] **Falcon** — [`src/falcon/`](src/falcon/).
- [ ] **Dreamcast** — [`src/dc/`](src/dc/).
- [ ] **DOS** — [`src/dos/`](src/dos/).
- [ ] **WinCE** — [`src/wince/`](src/wince/).
- [ ] **GLES2** — [`src/gles2/`](src/gles2/).
- [ ] **Java NVM** — [`src/javanvm/`](src/javanvm/).

> **Note:** The standalone ports can initially keep using a single default
> instance while the multi-instance API is exposed through `libatari800`.

---

## Cross-cutting audit

- [ ] Grep for `^static` in every `src/*.c` and move each per-instance `static`
      into the relevant sub-struct.
- [ ] Grep for `extern` declarations in every `src/*.h` and classify each as
      instance-specific vs shared.
- [ ] Audit all `*_GetByte` / `*_PutByte` / `*_Frame` / `*_Reset` /
      `*_StateSave` / `*_StateRead` functions for the context parameter.
- [ ] Audit the memory read/write function-pointer tables (`MEMORY_readmap`,
      `MEMORY_writemap`) and the PBI dispatch tables.
- [ ] Verify the CPU hot loop still compiles to direct array access (benchmark).
- [ ] Keep the tree buildable after each module migration; run the test suite
      (`test/`, `acidtest`, `libatari800_test.c`).