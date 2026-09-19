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
      `Screen_state_t`, `Colours_state_t`, `Sound_state_t`. Done so far
      (embedded by value in `Atari800_Instance`): `SIO_state_t`,
      `Devices_state_t`, `Cartridge_state_t`, `Cassette_state_t`,
      `PBI_state_t` (+ SCSI/BB/MIO/ESC/Binload/RTIME/Voicebox/Pokeyrec/IDE/
      AF80/BIT3/PROTO80/XLD/XEP80/RDevice/Input), `Screen_state_t`
      (2026-09-16), and `Colours_state_t` (2026-09-16), `Artifact_state_t`
      (2026-09-16), `Libatari800_state_t` (2026-09-18, Phase 5). Still
      forward-declared: `Sound_state_t` (Phase 4.3).
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
- [x] Implement `Atari800_FrameInstance` / `Atari800_ColdstartInstance` /
      `Atari800_WarmstartInstance`. Done 2026-09-18: thin wrappers over
      `Atari800_Frame_Ctx` / `Atari800_Coldstart_Ctx` / `Atari800_Warmstart_Ctx`
      in [`src/atari.c`](src/atari.c) (see §4.4).
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

> **Status: nearly complete.** SIO, Devices, Cartridge, Cassette, PBI, the
> PBI SCSI/Black Box/MIO sub-modules, the ESC/Binload handlers, RTIME, and
> Input (§3.9) done (2026-09-15); Rdevice done (2026-09-16). All of them now expose
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

> **Status:** SCSI, Black Box, MIO, PROTO80, and XLD done (2026-09-15). `PBI_SCSI_*` now
> exposes `*_Ctx(Atari800_Instance *inst, ...)` entry points (SCSI state lives
> in `SCSI_state_t`), and BB/MIO route their SCSI access through *their own*
> instance (`inst->scsi`), so the SCSI bus is fully per-instance. BB and MIO
> also re-point `PBI_IRQ` to their own instance's `pbi.IRQ` inside their `.c`
> files. pbi.c, atari.c, statesav.c and libatari800/main.c go through the
> forwarding macros (transitional: default instance). Build passes; Acid800
> results identical to pre-refactor baseline; 20 s no-disk smoke run clean.

- [x] **PBI_BB** — [`src/pbi_bb.h`](src/pbi_bb.h), [`src/pbi_bb.c`](src/pbi_bb.c):
      `PBI_BB_Menu`, `PBI_BB_Frame`, `PBI_BB_Initialise`, `PBI_BB_Exit`,
      `PBI_BB_D1GetByte`, `PBI_BB_D1PutByte`, `PBI_BB_D6GetByte`,
      `PBI_BB_D6PutByte`, `PBI_BB_StateSave`, `PBI_BB_StateRead`
      (also `PBI_BB_ReadConfig`/`PBI_BB_WriteConfig`).
      Done 2026-09-15: each is now `PBI_BB_*_Ctx(Atari800_Instance *inst, ...)`
      in `pbi_bb.c`, pinning the file-scope contexts (`BB = &inst->bb`,
      `BBi = inst`) via `PBI_BB_PIN_CTX(inst)`; all state aliases inside
      `pbi_bb.c` route through `BB`. In `pbi_bb.h` the legacy names are
      forwarding macros passing `Atari800_default`. Build passes; Acid800
      results identical to pre-refactor baseline.
- [x] Move state (PBI_BB): `PBI_BB_enabled` plus the previously file-scope
      internals (`bb_rom`, `bb_rom_size`, `bb_rom_high_bit`, `bb_rom_bank`,
      `bb_rom_filename`, `bb_ram`, `bb_ram_bank_offset`, `bb_PCR`,
      `bb_scsi_enabled`, `bb_scsi_disk_filename`, `buttondown`, and the
      frame counter from `PBI_BB_Frame`).
      Done 2026-09-15: all moved into `BB_state_t`
      ([`src/instance.h`](src/instance.h)), **embedded by value** in
      `Atari800_Instance` (`.bb`); `pbi_bb.c`/`pbi_bb.h` alias the legacy
      names to `Atari800_default->bb.*`. Default-instance init preserved in
      `atari.c` (`scsi_disk_filename = Util_FILENAME_NOT_SET`); the ROM/RAM
      buffers remain heap-allocated pointers filled by `init_bb()`.
- [x] **PBI_MIO** — [`src/pbi_mio.h`](src/pbi_mio.h), [`src/pbi_mio.c`](src/pbi_mio.c):
      `PBI_MIO_Initialise`, `PBI_MIO_Exit`, `PBI_MIO_D1GetByte`,
      `PBI_MIO_D1PutByte`, `PBI_MIO_D6GetByte`, `PBI_MIO_D6PutByte`,
      `PBI_MIO_StateSave`, `PBI_MIO_StateRead` (also
      `PBI_MIO_ReadConfig`/`PBI_MIO_WriteConfig`); state `PBI_MIO_enabled`.
      Done 2026-09-15: each is now `PBI_MIO_*_Ctx(Atari800_Instance *inst, ...)`
      in `pbi_mio.c`, pinning the file-scope contexts (`MIO = &inst->mio`,
      `MIOi = inst`) via `PBI_MIO_PIN_CTX(inst)`; all state aliases inside
      `pbi_mio.c` route through `MIO`. In `pbi_mio.h` the legacy names are
      forwarding macros passing `Atari800_default`. Build passes; Acid800
      results identical to pre-refactor baseline.
- [x] Move state (PBI_MIO): `PBI_MIO_enabled` plus the previously file-scope
      internals (`mio_rom`, `mio_rom_size`, `mio_rom_bank`,
      `mio_rom_filename`, `mio_ram`, `mio_ram_size`, `mio_ram_bank_offset`,
      `mio_ram_enabled`, `mio_scsi_enabled`, `mio_scsi_disk_filename`).
      Done 2026-09-15: all moved into `MIO_state_t`
      ([`src/instance.h`](src/instance.h)), **embedded by value** in
      `Atari800_Instance` (`.mio`); `pbi_mio.c`/`pbi_mio.h` alias the legacy
      names to `Atari800_default->mio.*`. Default-instance init preserved in
      `atari.c` (`rom_size = 0x4000`, `ram_size = 0x100000`,
      `scsi_disk_filename = Util_FILENAME_NOT_SET`); the ROM/RAM buffers
      remain heap-allocated pointers filled by `init_mio()`.
- [x] **PBI_SCSI** — [`src/pbi_scsi.h`](src/pbi_scsi.h), [`src/pbi_scsi.c`](src/pbi_scsi.c):
      `PBI_SCSI_PutByte`, `PBI_SCSI_GetByte`, `PBI_SCSI_PutSEL`,
      `PBI_SCSI_PutACK`; state `PBI_SCSI_CD/MSG/IO/BSY/REQ/SEL/ACK`,
      `PBI_SCSI_disk`.
      Done 2026-09-15: each is now `PBI_SCSI_*_Ctx(Atari800_Instance *inst, ...)`
      in `pbi_scsi.c`, pinning the file-scope context (`SCS = &inst->scsi`)
      via `PBI_SCSI_PIN_CTX(inst)`; the state aliases inside `pbi_scsi.c`
      route through `SCS` (including the internal transfer state
      `scsi_byte`, `scsi_phase`, `scsi_bufpos`, `scsi_buffer[256]`,
      `scsi_count`, and the `SCSI_PHASE_*` defines kept in `pbi_scsi.c`).
      In `pbi_scsi.h` the legacy names are forwarding macros passing
      `Atari800_default`, and the signal-line aliases (`PBI_SCSI_CD` etc.)
      read `Atari800_default->scsi.*`. `pbi_bb.c` and `pbi_mio.c` `#undef`
      these aliases and re-point them to their own instance, so the SCSI
      bus is per-instance end-to-end. Build passes; Acid800 results
      identical to pre-refactor baseline.
- [x] Move state (PBI_SCSI): `PBI_SCSI_CD/MSG/IO/BSY/REQ/SEL/ACK`,
      `PBI_SCSI_disk` and the internal transfer state.
      Done 2026-09-15: all moved into `SCSI_state_t`
      ([`src/instance.h`](src/instance.h)), **embedded by value** in
      `Atari800_Instance` (`.scsi`); defaults (all signals FALSE,
      `disk = NULL`) match the old static initialisers via zero-init.
- [x] **PBI_PROTO80** — [`src/pbi_proto80.h`](src/pbi_proto80.h),
      [`src/pbi_proto80.c`](src/pbi_proto80.c):
      `PBI_PROTO80_Initialise`, `PBI_PROTO80_Exit`, `PBI_PROTO80_D1GetByte`,
      `PBI_PROTO80_D1PutByte`, `PBI_PROTO80_D1ffPutByte`,
      `PBI_PROTO80_GetPixels` (also
      `PBI_PROTO80_ReadConfig`/`PBI_PROTO80_WriteConfig`).
      Done 2026-09-15: each is now
      `PBI_PROTO80_*_Ctx(Atari800_Instance *inst, ...)` in `pbi_proto80.c`,
      pinning the file-scope context (`PR = &inst->proto80`, `PRi = inst`)
      via `PBI_PROTO80_PIN_CTX(inst)`; the state aliases inside
      `pbi_proto80.c` route through `PR`. In `pbi_proto80.h` the legacy
      names are forwarding macros passing `Atari800_default`. The D1/D1ff
      Get/PutByte functions are dispatched from the
      `MEMORY_HwGetByte`/`MEMORY_HwPutByte` switch statements in `pbi.c`
      (transitional: default instance via the forwarding macros), and
      `sdl/video.c`'s `PBI_PROTO80_GetPixels` call likewise goes through the
      forwarding macro. Build passes; 20 s no-disk smoke run clean (no CIM).
- [x] Move state (PBI_PROTO80): `PBI_PROTO80_enabled` plus the previously
      file-scope `proto80rom` (heap-allocated 0x800-byte ROM buffer) and
      `proto80_rom_filename`.
      Done 2026-09-15: `PROTO80_state_t` defined in
      [`src/instance.h`](src/instance.h) and **embedded by value** in
      `Atari800_Instance` (`.proto80`); `pbi_proto80.c`/`pbi_proto80.h`
      alias `PBI_PROTO80_enabled` to `Atari800_default->proto80.enabled`.
      Defaults (`enabled = FALSE`, empty filename) match the old static
      initialisers via zero-init; the ROM buffer stays a heap pointer
      filled by `PBI_PROTO80_Initialise_Ctx`. `_Ctx` bodies touch
      `PRi->pbi.D6D7ram` and `PRi->memory.mem` directly (their own
      instance), so the module is per-instance end-to-end; only the
      transitional dispatch (pbi.c, sdl/video.c) still pins the default
      instance.
- [x] **PBI_XLD** — [`src/pbi_xld.h`](src/pbi_xld.h),
      [`src/pbi_xld.c`](src/pbi_xld.c): `PBI_XLD_Initialise`, `PBI_XLD_Exit`,
      `PBI_XLD_ReadConfig`, `PBI_XLD_WriteConfig`, `PBI_XLD_Reset`,
      `PBI_XLD_D1GetByte`, `PBI_XLD_D1ffGetByte`, `PBI_XLD_D1PutByte`,
      `PBI_XLD_D1ffPutByte`, `PBI_XLD_StateSave`, `PBI_XLD_StateRead`,
      `PBI_XLD_votrax_busy_callback`; state `PBI_XLD_enabled`,
      `PBI_XLD_v_enabled`.
      Done 2026-09-15: each is now `PBI_XLD_*_Ctx(Atari800_Instance *inst, ...)`
      in `pbi_xld.c`, pinning the file-scope context (`X = &inst->xld`,
      `Xi = inst`) via `PBI_XLD_PIN_CTX(inst)`; the state aliases inside
      `pbi_xld.c` route through `X`. In `pbi_xld.h` the legacy names are
      forwarding macros passing `Atari800_default`. The D1/D1ff Get/PutByte
      functions are dispatched from the `MEMORY_HwGetByte`/`MEMORY_HwPutByte`
      switch statements in `pbi.c` (transitional: default instance), and
      `votraxsnd.c`'s `PBI_XLD_votrax_busy_callback` call likewise goes
      through the forwarding macro. The internal `PIO_PutByte`/`PIO_GetByte`/
      `PIO_Command_Frame`/`WriteSectorBack` helpers operate on `X` directly;
      their SIO calls (`SIO_ReadSector`, `SIO_last_op`, ...) go through the
      forwarding macros (transitional: default instance). The never-defined
      sound-hook declarations (`PBI_XLD_VInit`/`VFrame`/`VProcess`) were kept
      as-is. Build passes; 20 s no-disk smoke run clean (no CIM).
- [x] Move state (PBI_XLD): `PBI_XLD_enabled`, `PBI_XLD_v_enabled` plus the
      previously file-scope `voicerom`/`diskrom` heap buffers,
      `xld_d_rom_filename`/`xld_v_rom_filename`, `xld_d_enabled`,
      `votrax_latch`, `modem_latch`, and the PIO transfer state
      (`CommandFrame[6]`, `CommandIndex`, `DataBuffer[256+3]`, `DataIndex`,
      `TransferStatus`, `ExpectedBytes`).
      Done 2026-09-15: `XLD_state_t` defined in
      [`src/instance.h`](src/instance.h) and **embedded by value** in
      `Atari800_Instance` (`.xld`); `pbi_xld.c`/`pbi_xld.h` alias
      `PBI_XLD_enabled`/`PBI_XLD_v_enabled` to
      `Atari800_default->xld.*`. Defaults (all FALSE, empty filenames,
      latches 0) match the old static initialisers via zero-init; the ROM
      buffers stay heap pointers filled by `init_xld_v`/`init_xld_d`.
      `_Ctx` bodies touch `Xi->pbi.IRQ`, `Xi->pbi.D6D7ram` and
      `Xi->memory.mem` directly (their own instance), so the module is
      per-instance end-to-end; only the transitional dispatch (pbi.c,
      votraxsnd.c, statesav.c) still pins the default instance.

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
> **Status:** RTIME, XEP80, AF80, BIT3, IDE, Voicebox, Pokeyrec, and Rdevice
> done (2026-09-16). Votrax deferred (see below; revisit together with
> Sound/Pokeysnd in §4.3). Phase 3 is now complete except Votrax.

- [x] **XEP80** — [`src/xep80.h`](src/xep80.h), [`src/xep80.c`](src/xep80.c):
      `XEP80_SetEnabled`, `XEP80_GetBit`, `XEP80_PutBit`, `XEP80_ChangeColors`,
      `XEP80_StateSave`, `XEP80_StateRead`, `XEP80_Initialise` (also
      `XEP80_ReadConfig`/`XEP80_WriteConfig`); state `XEP80_enabled`,
      `XEP80_port`, `XEP80_scrn_height`, `XEP80_char_height`,
      `XEP80_screen_1[]`, `XEP80_screen_2[]`.
      Done 2026-09-15: each is now `XEP80_*_Ctx(Atari800_Instance *inst, ...)`
      in `xep80.c`, pinning the file-scope context (`XE = &inst->xep80`,
      `XEI = inst`) via `XEP80_PIN_CTX(inst)`. All the previously file-scope
      statics (~45 variables: serial-protocol state, internal NS405 RAM
      registers, attribute latches, cursor/TV state, video_ram, and the two
      display buffers) are macros re-pointed to `XE->...` inside `xep80.c`,
      so the ~1900 lines of unchanged rendering/protocol bodies operate on
      their own instance. In `xep80.h` the legacy names are forwarding
      macros/aliases passing `Atari800_default` (pia.c, ui.c, videomode.c,
      cfg.c, atari.c, statesav.c, sdl/video_sw.c, sdl/video_gl.c are
      transitional). Note: the `xpos`/`ypos` state macros collide with the
      field-name tokens inside `antic.h`'s `ANTIC_xpos`/`ANTIC_ypos`
      expansions; fixed by re-routing those two to `static inline` helpers
      defined before the colliding macros (`XEP80_antic_xpos/ypos`).
      The shared `font` pointer (loaded by `XEP80_FONTS_InitFonts`,
      read-only afterwards) stays file-scope. `XEP80_Initialise_Ctx` and
      `XEP80_StateRead_Ctx` call `XEP80_SetEnabled_Ctx` with their own
      instance. Build passes; 20 s no-disk smoke run clean (no CIM).
- [x] Move state (XEP80): everything listed above plus the previously
      file-scope statics (`output_word`, `input_queue`, `input_count`,
      `start_trans_cpu_clock`, `receiving`, `ypos`, `xpos`, `last_char`,
      `lmargin`, `rmargin`, `xscroll`, `line_pointers[]`, `old_ypos`,
      `old_xpos`, `list_mode`, `escape_mode`, `burst_mode`, `screen_output`,
      `attrib_a/b` + font_a/b_* latches, `cursor_on`, `graphics_mode`,
      `pal_mode`, `blink_reverse`, `cursor_blink`, `cursor_overwrite`,
      `inverse_mode`, `char_set`, `cursor_x/y`, `curs`, `video_ram[0x2000]`,
      `charset_filename`).
      Done 2026-09-15: `XEP80_state_t` defined in
      [`src/instance.h`](src/instance.h) (the XEP80 geometry defines moved
      there from `xep80.h` so the display buffers can be embedded by value)
      and **embedded by value** in `Atari800_Instance` (`.xep80`).
      Non-zero defaults preserved in the `atari.c` initializer
      (`char_height = XEP80_CHAR_HEIGHT_NTSC`, `scrn_height = 250`,
      `rmargin = 0x4f`, `screen_output = TRUE`, `attrib_a/b = 0xff`,
      `cursor_on = TRUE`); the rest match the old static initialisers via
      zero-init.
- [x] **AF80** — [`src/af80.h`](src/af80.h): `AF80_Initialise`, `AF80_Exit`,
      `AF80_InsertRightCartridge`, `AF80_D5GetByte`, `AF80_D5PutByte`,
      `AF80_D6GetByte`, `AF80_D6PutByte`, `AF80_GetPixels`, `AF80_Reset` (also
      `AF80_ReadConfig`/`AF80_WriteConfig`); state `AF80_enabled`,
      `AF80_palette[16]`.
      Done 2026-09-15: each is now `AF80_*_Ctx(Atari800_Instance *inst, ...)`
      in `af80.c`, pinning the file-scope context (`AF = &inst->af80`,
      `AFI = inst`) via `AF80_PIN_CTX(inst)`; the state aliases inside
      `af80.c` route through `AF`. In `af80.h` the legacy names are
      forwarding macros passing `Atari800_default`. The D5/D6 Get/PutByte
      functions are dispatched from `cartridge.c` (D5) and `pbi.c` (D6)
      via the forwarding macros — transitional: default instance; they
      will be re-routed per-instance when those dispatchers pass their own
      instance. `_Ctx` bodies touch `AFI->memory.mem` directly and call
      `MEMORY_Cart809fEnableCtx/DisableCtx(inst)` (own instance), so the
      module is per-instance end-to-end. **Update (Phase 6, 2026-09-19):**
      `AF80_palette[16]` moved into `AF80_state_t.palette` (derived from
      the shared read-only `rgbi_palette` table by `AF80_Initialise_Ctx`);
      the former static-initialiser blocker in `sdl/palette.c` was replaced
      with the runtime `SDL_PALETTE_Initialise()` (see §6.1). The
      AF80_DEBUG `D()` prints still read `CPU_remember_PC` via the
      default-instance aliases in `cpu.h` (debug-only, transitional).
      Build passes; 20 s no-disk smoke run clean (no CIM); Acid800
      results identical to pre-refactor baseline.
- [x] Move state (AF80): `AF80_enabled` plus the previously file-scope
      statics (`af80_rom`, `af80_rom_filename`, `af80_charset`,
      `af80_charset_filename`, `af80_screen`, `af80_attrib`,
      `rom_bank_select`, `not_rom_output_enable`,
      `not_right_cartridge_rd4_control`, `not_enable_2k_character_ram`,
      `not_enable_2k_attribute_ram`, `not_enable_crtc_registers`,
      `not_enable_80_column_output`, `video_bank_select`, `crtreg[0x40]`).
      Done 2026-09-15: `AF80_state_t` defined in
      [`src/instance.h`](src/instance.h) and **embedded by value** in
      `Atari800_Instance` (`.af80`); `af80.c`/`af80.h` alias `AF80_enabled`
      to `Atari800_default->af80.enabled`. Defaults (all FALSE/0, NULL
      buffers) match the old static initialisers via zero-init; the
      ROM/charset/screen/attrib buffers stay heap pointers filled by
      `AF80_Initialise_Ctx`. The shared `rgbi_palette` table stays
      file-scope in `af80.c`; `AF80_palette` moved into
      `AF80_state_t.palette` in Phase 6 (see §6.1).
- [x] **BIT3** — [`src/bit3.h`](src/bit3.h): `BIT3_Initialise`, `BIT3_Exit`,
      `BIT3_InsertRightCartridge`, `BIT3_D5GetByte`, `BIT3_D5PutByte`,
      `BIT3_D6GetByte`, `BIT3_D6PutByte`, `BIT3_GetPixels`, `BIT3_Reset` (also
      `BIT3_ReadConfig`/`BIT3_WriteConfig`); state `BIT3_enabled`,
      `BIT3_palette[2]`.
      Done 2026-09-15: each is now `BIT3_*_Ctx(Atari800_Instance *inst, ...)`
      in `bit3.c`, pinning the file-scope context (`B3 = &inst->bit3`,
      `B3i = inst`) via `BIT3_PIN_CTX(inst)`; the state aliases inside
      `bit3.c` route through `B3`. In `bit3.h` the legacy names are
      forwarding macros passing `Atari800_default`. The D5 handlers are
      dispatched from `cartridge.c` and the D6 handlers from `pbi.c` via
      the forwarding macros — transitional: default instance; they will
      be re-routed per-instance when those dispatchers pass their own
      instance. `_Ctx` bodies touch `B3i->memory.mem` directly (own
      instance). **Update (Phase 6, 2026-09-19):** `BIT3_palette[2]` moved
      into `BIT3_state_t.palette` (its static-initialiser values are
      preserved in the default-instance initializer in `atari.c`; the
      former `sdl/palette.c` static-initialiser blocker was replaced with
      the runtime `SDL_PALETTE_Initialise()`, see §6.1). Remaining
      exception: `BIT3_Reset_Ctx`'s
      `VIDEOMODE_Set80Column` call and `BIT3_Initialise_Ctx`'s
      `VIDEOMODE_80_column` store remain process-global (videomode is a
      Phase 4 module, transitional). Note: `BIT3_InsertRightCartridge`
      was declared in `bit3.h` but never defined or called; a
      `BIT3_InsertRightCartridge_Ctx` stub was added for symmetry.
      Build passes; 20 s no-disk smoke run clean (no CIM); Acid800
      results identical to pre-refactor baseline.
- [x] Move state (BIT3): `BIT3_enabled` plus the previously file-scope
      statics (`bit3_rom`, `bit3_rom_filename`, `bit3_charset`,
      `bit3_charset_filename`, `bit3_screen`, `video_latch`,
      `rom_bank_select`, `crtreg[0x40]`).
      Done 2026-09-15: `BIT3_state_t` defined in
      [`src/instance.h`](src/instance.h) and **embedded by value** in
      `Atari800_Instance` (`.bit3`); `bit3.c`/`bit3.h` alias `BIT3_enabled`
      to `Atari800_default->bit3.enabled`. Defaults (all FALSE/0, NULL
      buffers) match the old static initialisers via zero-init; the
      ROM/charset/screen buffers stay heap pointers filled by
      `BIT3_Initialise_Ctx`. `BIT3_palette` moved into
      `BIT3_state_t.palette` in Phase 6 (see §6.1); the shared
      `rgbi_palette`-equivalent constants are now in the default-instance
      initializer in `atari.c`.
- [x] **IDE** — [`src/ide.h`](src/ide.h): `IDE_Initialise`, `IDE_Exit`,
      `IDE_GetByte`, `IDE_PutByte`; state `IDE_enabled` and IDE disk state.
      Done 2026-09-15: each is now `IDE_*_Ctx(Atari800_Instance *inst, ...)`
      in `ide.c`, pinning the file-scope context (`IDEs = &inst->ide`,
      `IDEi = inst`) via `IDE_PIN_CTX(inst)`; the state aliases inside
      `ide.c` route through `IDEs` (`IDE_enabled`, `IDE_debug`, `count`,
      and `device`). In `ide.h` the legacy names are forwarding macros
      passing `Atari800_default`; `cartridge.c`'s D5 dispatch and `atari.c`
      go through the forwarding macros (transitional). The legacy
      zero-initialised file-scope `struct ide_device` became a lazily
      allocated, zeroed heap buffer (`IDE_state_t.dev`, named `dev` because
      ide.c aliases the token `device`); `IDE_Exit_Ctx` frees it. Build
      passes; Acid800 results identical to pre-refactor baseline.
- [x] Move state (IDE): `IDE_enabled`, `IDE_debug`, the debug `count`
      counter, and the `struct ide_device` device state.
      Done 2026-09-15: `IDE_state_t` defined in
      [`src/instance.h`](src/instance.h) and **embedded by value** in
      `Atari800_Instance` (`.ide`); defaults (all zero) match the old
      static initialisers via zero-init.
- [x] **Voicebox** — [`src/voicebox.h`](src/voicebox.h): `VOICEBOX_Initialise`,
      `VOICEBOX_SKCTLPutByte`, `VOICEBOX_SEROUTPutByte`; state `VOICEBOX_enabled`,
      `VOICEBOX_ii`.
      Done 2026-09-15: each is now
      `VOICEBOX_*_Ctx(Atari800_Instance *inst, ...)` in `voicebox.c`,
      pinning the file-scope context (`VB = &inst->voicebox`) via
      `VOICEBOX_PIN_CTX(inst)`; the state aliases inside `voicebox.c`
      route through `VB`. In `voicebox.h` the legacy names are forwarding
      macros passing `Atari800_default`. `pokey.c`'s SKCTL/SEROUT hooks and
      `atari.c` go through the forwarding macros (transitional). The
      function-local `static`s in `VOICEBOX_SKCTLPutByte` (`prev_byte`,
      `prev_prev_byte`, `voice_box_byte`, `voice_box_bit`) moved into
      `Voicebox_state_t`. Build passes; Acid800 results identical to
      pre-refactor baseline.
- [x] Move state (Voicebox): `VOICEBOX_enabled`, `VOICEBOX_ii` plus the
      serial-decode statics listed above.
      Done 2026-09-15: `Voicebox_state_t` defined in
      [`src/instance.h`](src/instance.h) and **embedded by value** in
      `Atari800_Instance` (`.voicebox`); defaults (all FALSE/0) match the
      old static initialisers via zero-init.
- [ ] **Votrax** — [`src/votraxsnd.h`](src/votraxsnd.h): all `VOTRAXSND_*`
      functions and state. **Deferred to Phase 4 (sound):** `votraxsnd.c`
      is tightly coupled to the sound subsystem (`POKEYSND_volume`, the
      mixing buffers, `num_pokeys`, `POKEYSND_Init` call ordering) and to
      the shared `Votrax_*` synth in `votrax.c`; converting it before the
      Sound module would produce a misleading per-instance split. Revisit
      together with Sound/Pokeysnd (§4.3).
- [x] **Pokeyrec** — [`src/pokeyrec.h`](src/pokeyrec.h): `POKEYREC_Recorder`,
      `POKEYREC_Initialise`, `POKEYREC_Exit`.
      Done 2026-09-15: each is now
      `POKEYREC_*_Ctx(Atari800_Instance *inst, ...)` in `pokeyrec.c`,
      pinning the file-scope context (`RECC = &inst->pokeyrec`) via
      `POKEYREC_PIN_CTX(inst)`; the state aliases inside `pokeyrec.c`
      route through `RECC`. In `pokeyrec.h` the legacy names are
      forwarding macros passing `Atari800_default`. `pokeysnd.c`'s
      `POKEYREC_Recorder` call and `atari.c`'s Initialise/Exit go through
      the forwarding macros (transitional). The old static defaults
      (`filename = "pokeyrec.dat"`, `fmt = "%c"`) are applied at the top
      of `POKEYREC_Initialise_Ctx` (the instance state is
      zero-initialised). Build passes; Acid800 results identical to
      pre-refactor baseline.
- [x] Move state (Pokeyrec): `enabled`, `counter`, `interval`, `filename`,
      `fmt`, `fp` (and `stereo` under STEREO_SOUND).
      Done 2026-09-15: `Pokeyrec_state_t` defined in
      [`src/instance.h`](src/instance.h) and **embedded by value** in
      `Atari800_Instance` (`.pokeyrec`).
- [x] **Rdevice** — [`src/rdevice.h`](src/rdevice.h), [`src/rdevice.c`](src/rdevice.c):
      `RDevice_OPEN`, `RDevice_CLOS`, `RDevice_READ`, `RDevice_WRIT`,
      `RDevice_STAT`, `RDevice_SPEC`, `RDevice_INIT`, `RDevice_Exit`; state
      `RDevice_serial_enabled`, `RDevice_serial_device`.
      Done 2026-09-16: each is now
      `RDevice_*_Ctx(Atari800_Instance *inst, ...)` in `rdevice.c`, pinning
      the file-scope context (`RD = &inst->rdevice`, `RDI = inst`) via
      `RDEV_PIN_CTX(inst)`; all state aliases inside `rdevice.c` route
      through `RD` (including the internal helpers `xio_34/36/38/40_Ctx`,
      `open_connection_Ctx`, `open_connection_serial_Ctx`, and
      `RDevice_GetInetAddress_Ctx`, which take the instance). In `rdevice.h`
      the legacy state names are forwarding macros passing
      `Atari800_default`. The `RDevice_OPEN..INIT` functions remain real
      functions (registered as context-free escape handlers via
      `ESC_AddEscRts` in `devices.c`) that pin the default instance and
      forward to the `_Ctx` versions — same deferred ESC-handler
      consideration as `SIO_Handler`/`loader_cont`. `RDevice_Exit` likewise
      forwards (its `WSACleanup` is process-global). The two
      `struct sockaddr_in` blobs are stored opaquely in `RDevice_state_t`
      (`in_storage`/`peer_in_storage`, cast in `rdevice.c`) so
      [`src/instance.h`](src/instance.h) does not need the network headers.
      `catch_disconnect` (the SIGPIPE/SIGHUP handler) stays a context-free
      function operating on the pinned context. Build passes; 20 s no-disk
      smoke run clean (no CIM); Acid800 results identical to pre-refactor
      baseline.
- [x] Move state (Rdevice): `RDevice_serial_enabled`,
      `RDevice_serial_device` plus the previously file-scope statics
      (`connected`, `do_once`, `rdev_fd`, `in`/`peer_in` sockaddr storage,
      `sock`, `portnum`, `inetaddress[256]`, `CONNECT_STRING[40]`,
      `retval`, `MESSAGE[256]`, `command_buf[256]`, `bufout[256]`,
      `concurrent`, `command_end`, `translation`, `trans_cr`, `linefeeds`,
      `bufend`).
      Done 2026-09-16: all moved into `RDevice_state_t`
      ([`src/instance.h`](src/instance.h)) and **embedded by value** in
      `Atari800_Instance` (`.rdevice`); `rdevice.c`/`rdevice.h` alias the
      legacy names to `Atari800_default->rdevice.*`. Defaults preserved in
      the `atari.c` initializer (`portnum = 9000`,
      `CONNECT_STRING = "\r\n_CONNECT 2400\r\n"`, `translation = 1`,
      `linefeeds = 1`, `serial_enabled` = 0 with R_NETWORK / 1 without);
      the rest match the old static initialisers via zero-init. Remaining
      file-scope: `ioctlsocket_non_block` (Windows-only constant) and the
      Windows-only `winsock_started` function-local static in
      `open_connection_Ctx` (WSAStartup is process-global).

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

- [x] Convert: `INPUT_Initialise`, `INPUT_Exit`, `INPUT_Frame`, `INPUT_Scanline`,
      `INPUT_SelectMultiJoy`, `INPUT_CenterMousePointer`, `INPUT_DrawMousePointer`,
      `INPUT_Recording`, `INPUT_Playingback`, `INPUT_RecordInt`, `INPUT_PlaybackInt`.
      Done 2026-09-15: each is now `INPUT_*_Ctx(Atari800_Instance *inst, ...)`
      in `input.c`, pinning the file-scope context (`IN = &inst->input`,
      `INI = inst`) via `INPUT_PIN_CTX(inst)`; all state aliases inside
      `input.c` route through `IN`. The registers written/read by
      `INPUT_Frame_Ctx` (`POKEY_KBCODE/IRQST/IRQEN/SKSTAT/POT_input`,
      `GTIA_TRIG`, `PIA_PORT_input`, `ANTIC_PENH_input/PENV_input`,
      `CASSETTE_press_space`) are `#undef`'d and re-pointed to `INI->...`
      inside `input.c`, so the module is per-instance end-to-end for those.
      In `input.h` the legacy names are forwarding macros passing
      `Atari800_default`. Transitional (still default-instance via header
      aliases in `input.c`): `CPU_GenerateIRQ` (cpu.h macro over the default
      `CPU_IRQ`), and the top-level config reads
      (`Atari800_machine_type`, `Atari800_nframes`, `Atari800_tv_mode`,
      `Atari800_keyboard_detached`). The EVENT_RECORDING statics
      (`recordfp`, `playbackfp`, `recording`, `playingback`,
      `playingback_exit_after`, `recording_version`, `gzbuf`) remain
      file-scope (EVENT_RECORDING is a non-default build option; move them
      with a future pass if that build is needed per-instance). Build
      passes; Acid800 results identical to pre-refactor baseline; 25 s
      no-disk smoke run clean.
- [x] Move state: `INPUT_key_code`, `INPUT_key_shift`, `INPUT_key_consol`,
      `INPUT_joy_autofire[4]`, `INPUT_joy_block_opposite_directions`,
      `INPUT_joy_multijoy`, `INPUT_joy_5200_min/center/max`, mouse state
      (`INPUT_mouse_mode`, `INPUT_mouse_port`, `INPUT_mouse_delta_x/y`,
      `INPUT_mouse_buttons`, `INPUT_mouse_speed`, `INPUT_mouse_pot_min/max`,
      `INPUT_mouse_pen_ofs_h/v`, `INPUT_mouse_joy_inertia`, `INPUT_direct_mouse`),
      `INPUT_cx85`.
      Done 2026-09-15: `Input_state_t` defined concretely in
      [`src/instance.h`](src/instance.h) (replacing the forward declaration;
      the old `Input_state_t *input` pointer member was removed) and
      **embedded by value** in `Atari800_Instance` (`.input`). The
      previously file-scope statics (`mouse_x/y`, `mouse_move_x/y`,
      `mouse_pen_show_pointer`, `mouse_last_right/down`, `STICK[4]`,
      `TRIG_input[4]`, `joy_multijoy_no`, `cx85_port`,
      `max_scanline_counter`, `scanline_counter`) and the function-local
      statics in `INPUT_Frame` (`last_key_code`, `last_key_break`,
      `last_stick[4]`, `last_mouse_buttons`, `bit5_5200`) and `mouse_step`
      (`e` → `mouse_step_e`) all moved into the struct; `input.c` aliases
      the legacy tokens to `IN->...`. Non-zero defaults preserved in the
      `atari.c` initializer (`key_code = AKEY_NONE`,
      `key_consol = INPUT_CONSOL_NONE`,
      `joy_block_opposite_directions = 1`, `joy_5200_min/center/max =
      6/114/220`, `mouse_speed = 3`, `mouse_pot_min/max = 1/228`,
      `mouse_pen_ofs_h/v = 42/2`, `mouse_joy_inertia = 10`,
      `last_stick[4] = INPUT_STICK_CENTRE`); the rest match the old static
      initialisers via zero-init. The `mouse_amiga_codes`/`mouse_st_codes`
      tables stay file-scope `static const` *(shared, read-only)*.

---

## Phase 4 — Output / input / platform layer

> **Status: in progress.** Screen (§4.1) done (2026-09-16); Colours
> (§4.2, including the COLOURS_NTSC/COLOURS_PAL sub-modules), Artifact,
> Pal_blending, and Filter_ntsc (§4.2) done (2026-09-16); the top-level
> driver (§4.4) done (2026-09-18); State save/load (§4.5) and the
> Monitor/debugger (§4.6) done (2026-09-18).
> File_export (§4.2) and Videomode (§4.2) are **deferred** (host-output
> state; see their entries for rationale).
> All public Screen/Colours functions now expose
> `*_Ctx(Atari800_Instance *inst, ...)` entry points with legacy-name
> forwarding macros in their headers; `Screen_state_t` and `Colours_state_t`
> are defined concretely in [`src/instance.h`](src/instance.h) and **embedded
> by value** in `Atari800_Instance` (`.screen`, `.colours`). Build passes;
> 20 s no-disk smoke run clean (no CIM); Acid800 results identical to the
> pre-refactor baseline (23 success / 28 expected failures / 2 skipped; the
> 2 FAILs — "MMU: XL banking" and "suite totals changed" — are pre-existing,
> see [`docs/acid800-expected-results.md`](acid800-expected-results.md)).

### 4.1 Screen — [`src/screen.h`](src/screen.h), [`src/screen.c`](src/screen.c)

- [x] Convert: `Screen_Initialise`, `Screen_ReadConfig`, `Screen_WriteConfig`,
      `Screen_DrawAtariSpeed`, `Screen_DrawDiskLED`, `Screen_Draw1200LED`,
      `Screen_DrawMultimediaStats`, `Screen_SaveScreenshot`,
      `Screen_SaveNextScreenshot`, `Screen_EntireDirty`, `Screen_SetStatusText`,
      `Screen_DrawStatusText`.
      Done 2026-09-16: each is now
      `Screen_*_Ctx(Atari800_Instance *inst, ...)` in `screen.c`, pinning the
      file-scope context (`SC = &inst->screen`, `SCI = inst`) via
      `SCREEN_PIN_CTX(inst)`; all state aliases inside `screen.c` route
      through `SC`. In `screen.h` the legacy names are forwarding macros
      passing `Atari800_default`. Include-cycle note: `instance.h` includes
      `screen.h` (for the ANTIC `pm_scanline` buffer size), so `screen.h`
      defines `Screen_WIDTH`/`Screen_HEIGHT` and forward-declares
      `struct Atari800_Instance` for the `_Ctx` prototypes *before*
      including `instance.h`; the aliases/forwarding macros come after.
      Per-instance end-to-end: the `_Ctx` bodies re-point `SIO_last_*` to
      `SCI->sio.*`, `CASSETTE_readable/record/writable` to `SCI->cassette.*`
      (calling `CASSETTE_GetSize_Ctx`/`GetPosition_Ctx(SCI)`), `PIA_PORTB`/
      `PIA_PORTB_mask` to `SCI->pia.*`, and call
      `ANTIC_VideoPutByte_Ctx(SCI, ...)` / `ANTIC_Frame_Ctx(SCI, TRUE)`.
      Transitional (still default-instance): `Atari800_nframes`,
      `Atari800_tv_mode`, `Atari800_keyboard_leds` (top-level config, §4.4)
      and the `File_Export_*` calls (§4.2). The stale, never-defined
      `Screen_FindScreenshotFilename` declaration was dropped from
      `screen.h`. Build passes; 20 s no-disk smoke run clean (no CIM);
      Acid800 results identical to the pre-refactor baseline.
- [x] Move state: `Screen_atari` (framebuffer pointer, heap-allocated by
      `Screen_Initialise_Ctx`), `Screen_atari_b/1/2` (BITPL_SCR),
      `Screen_dirty` (DIRTYRECT), `Screen_visible_x1/y1/x2/y2`,
      `Screen_show_atari_speed`, `Screen_show_disk_led`,
      `Screen_show_sector_counter`, `Screen_show_1200_leds`,
      `Screen_show_multimedia_stats`, plus the previously file-scope
      statics (`screenshot_filename_format`, `screenshot_no_last`,
      `screenshot_no_max` under SCREENSHOTS, `status_text[60]`,
      `status_text_duration`) and the function-local statics in
      `Screen_DrawAtariSpeed` (`percent_display`, `last_updated`,
      `last_time`).
      Done 2026-09-16: all moved into `Screen_state_t`
      ([`src/instance.h`](src/instance.h)); `screen.c`/`screen.h` alias the
      legacy names to `Atari800_default->screen.*`. Default-instance init
      preserved in `atari.c` (`visible_x1 = 24`, `visible_x2 = 360`,
      `visible_y2 = Screen_HEIGHT`, `show_disk_led = TRUE`,
      `show_1200_leds = TRUE`, `show_multimedia_stats = TRUE` under
      AUDIO/VIDEO_RECORDING, `screenshot_no_last = -1` under SCREENSHOTS,
      `percent_display = 100`); the rest match the old static initialisers
      via zero-init. Platform ports that read/assign `Screen_atari`
      (falcon, sdl, dc, ps2, gles2, android, javanvm, amiga, dos, x11, rpi)
      keep working via the header aliases (transitional; Phase 6). Note:
      `wince/port/main.c` self-declares `extern UBYTE *Screen_dirty;` /
      `extern void Screen_EntireDirty(void);` without including `screen.h`
      — it will need updating in Phase 6 (not part of the Linux build).

### 4.2 Colours / video filters

- [x] **Colours** — [`src/colours.h`](src/colours.h): `Colours_*` functions; state
      `Colours_table[256]`, `Colours_setup`, `Colours_NTSC_setup`, `Colours_PAL_setup`.
      Done 2026-09-16 (including the COLOURS_NTSC/COLOURS_PAL sub-modules,
      which are part of the same state cluster): each public function is now
      `Colours_*_Ctx(Atari800_Instance *inst, ...)` in `colours.c`, pinning
      the file-scope context (`CO = &inst->colours`, `COI = inst`) via
      `COLOURS_PIN_CTX(inst)`; `Colours_setup`/`Colours_external` are
      `#undef`'d and re-pointed to `CO->setup`/`CO->external` inside
      `colours.c`. In `colours.h` the legacy names are forwarding macros
      passing `Atari800_default`. Include-cycle note: `instance.h` includes
      `colours.h` (for the `Colours_setup_t`/`COLOURS_EXTERNAL_t` types used
      by `Colours_state_t`), so `colours.h` forward-declares
      `struct Atari800_Instance` for the `_Ctx` prototypes *before*
      including `instance.h`; the aliases/forwarding macros come after.
      The stateless helpers (`Colours_RGB2YUV`, `Colours_YUV2RGB`,
      `Colours_Gamma2Linear`, `Colours_Linear2sRGB`) keep their signatures.
      `COLOURS_NTSC_*`/`COLOURS_PAL_*` are likewise `*_Ctx(inst, ...)` in
      `colours_ntsc.c`/`colours_pal.c` (pinning `NT`/`PAL` contexts via
      `COLOURS_NTSC_PIN_CTX`/`COLOURS_PAL_PIN_CTX`), with forwarding macros
      and state aliases in `colours_ntsc.h`/`colours_pal.h`. Per-instance
      end-to-end: `colours.c`'s `_Ctx` bodies call the NTSC/PAL `_Ctx`
      functions with their own instance. Transitional (still default-instance
      via header aliases): `filter_ntsc.c` (`COLOURS_NTSC_GetYIQ`/`_setup`/
      `_external`), `pal_blending.c` (`COLOURS_PAL_GetYUV`/`_setup`/
      `_external`), `ui.c`, `sdl/input.c`, `atari.c`, `cfg.c`, the codecs
      (`Colours_GetR/G/B` over `Colours_table`), `atari_ntsc/atari_ntsc.c`,
      and `atari_x11.c`. Build passes; 20 s no-disk smoke run clean (no
      CIM); Acid800 results identical to the pre-refactor baseline.
- [x] Move state (Colours): `Colours_setup`/`Colours_external` (the
      TV-system-selected pointers), `COLOURS_NTSC_setup`,
      `COLOURS_NTSC_external`, `COLOURS_PAL_setup`, `COLOURS_PAL_external`.
      Done 2026-09-16: `Colours_state_t` defined in
      [`src/instance.h`](src/instance.h) (ntsc/pal setup + external palette
      structs, plus the `setup`/`external` pointers into them) and
      **embedded by value** in `Atari800_Instance` (`.colours`); the header
      aliases point at `Atari800_default->colours.*`. Defaults match the
      old static initialisers via zero-init (`filename = ""`,
      `loaded/adjust = FALSE`); the non-zero defaults (NTSC
      `color_delay = 26.8`, PAL `color_delay = 23.2`) are applied by
      `Colours_PreInitialise_Ctx` at startup, as before.
      **Update (Phase 6, 2026-09-19):** `Colours_table[256]` now also lives
      in `Colours_state_t` (`Colours_state_t.table`); `colours.c` aliases
      it to the pinned context and `colours.h` to
      `Atari800_default->colours.table`. The former blocker —
      `sdl/palette.c`'s static initialiser — was replaced with the runtime
      `SDL_PALETTE_Initialise()` (see §6.1).
- [ ] **Videomode** — [`src/videomode.h`](src/videomode.h): `VIDEOMODE_*` functions
      and state. **Deferred (host-display state):** `videomode.c` (1271 lines,
      ~300 external call sites) is the host display's geometry/resolution
      manager — it owns the fullscreen resolution list (from
      `PLATFORM_AvailableResolutions`), the window size, and drives
      `PLATFORM_SetVideoMode`. These are single-host-device concerns, not
      per-machine state; converting it before the platform ports (Phase 6)
      would produce a misleading per-instance split. Revisit together with the
      SDL/platform layer, where the `Screen_atari` static-init references
      must also be resolved (the `Colours_table` static-init blocker was
      resolved in Phase 6, see §6.1).
- [x] **Pal_blending** — [`src/pal_blending.h`](src/pal_blending.h):
      `PAL_BLENDING_UpdateLookup`, `PAL_BLENDING_Blit16/32`,
      `PAL_BLENDING_BlitScaled16/32`.
      Done 2026-09-16: each is now
      `PAL_BLENDING_*_Ctx(Atari800_Instance *inst, ...)` in `pal_blending.c`,
      pinning the file-scope context (`PL = &inst->pal_blending`,
      `PLi = inst`) via `PAL_BLENDING_PIN_CTX(inst)`; the state aliases
      (`palette`, `shift_mask`) inside `pal_blending.c` route through `PL`.
      In `pal_blending.h` the legacy names are forwarding macros passing
      `Atari800_default`. Per-instance end-to-end: the `_Ctx` bodies
      `#undef` and re-point `ARTIFACT_mode` to `PLi->artifact.mode`,
      `COLOURS_PAL_setup`/`COLOURS_PAL_external` to `PLi->colours.pal_*`,
      and call `COLOURS_PAL_GetYUV_Ctx(PLi, ...)`. Transitional (still
      default-instance): the `PLATFORM_*` pixel-format calls (host device,
      Phase 6). Build passes; 20 s no-disk smoke run clean; Acid800 results
      identical to the pre-refactor baseline.
- [x] Move state (Pal_blending): the `palette` union (16/32-BPP lookup
      tables, 2×256 entries) and `shift_mask`.
      Done 2026-09-16: `Pal_blending_state_t` (with `Pal_blending_palette_t`)
      defined in [`src/instance.h`](src/instance.h) and **embedded by value**
      in `Atari800_Instance` (`.pal_blending`); defaults (all zero) match the
      old static initialisers via zero-init.
- [x] **Filter_ntsc** — [`src/filter_ntsc.h`](src/filter_ntsc.h):
      `FILTER_NTSC_Update`, `FILTER_NTSC_RestoreDefaults`,
      `FILTER_NTSC_SetPreset`, `FILTER_NTSC_GetPreset`,
      `FILTER_NTSC_NextPreset`, `FILTER_NTSC_PreInitialise`,
      `FILTER_NTSC_ReadConfig`, `FILTER_NTSC_WriteConfig`,
      `FILTER_NTSC_Initialise`; state `FILTER_NTSC_setup`, `FILTER_NTSC_emu`.
      Done 2026-09-16: each is now
      `FILTER_NTSC_*_Ctx(Atari800_Instance *inst, ...)` in `filter_ntsc.c`,
      pinning the file-scope context (`FN = &inst->filter_ntsc`,
      `FNi = inst`) via `FILTER_NTSC_PIN_CTX(inst)`; the state aliases inside
      `filter_ntsc.c` route through `FN`. In `filter_ntsc.h` the legacy names
      are forwarding macros passing `Atari800_default`. Include-cycle note:
      `instance.h` includes `filter_ntsc.h` (for the `atari_ntsc_setup_t`/
      `atari_ntsc_t` types used by `Filter_ntsc_state_t`), so `filter_ntsc.h`
      forward-declares `struct Atari800_Instance` before including
      `instance.h`. Exceptions: `FILTER_NTSC_New`/`FILTER_NTSC_Delete` are
      stateless (malloc/free) and keep their signatures. Per-instance
      end-to-end: the `_Ctx` bodies `#undef` and re-point
      `COLOURS_NTSC_setup`/`COLOURS_NTSC_external` to
      `FNi->colours.ntsc_*` and call `COLOURS_NTSC_GetYIQ_Ctx(FNi, ...)`.
      Transitional (still default-instance via header aliases): `atari.c`,
      `cfg.c`, `ui.c`, `sdl/video.c`, `sdl/video_sw.c`, `sdl/video_gl.c`,
      `sdl/input.c`. **Note:** `atari_rpi.c` self-declares its own
      `FILTER_NTSC_emu`/`FILTER_NTSC_setup` globals and no-op
      `FILTER_NTSC_Update`/`NextPreset` stubs without including
      `filter_ntsc.h` — it will need updating in Phase 6 (not part of the
      Linux build). Build passes; 20 s no-disk smoke run clean; Acid800
      results identical to the pre-refactor baseline.
- [x] Move state (Filter_ntsc): `FILTER_NTSC_setup` (the
      `atari_ntsc_setup_t` controls) and `FILTER_NTSC_emu` (the allocated
      filter pointer).
      Done 2026-09-16: `Filter_ntsc_state_t` defined in
      [`src/instance.h`](src/instance.h) and **embedded by value** in
      `Atari800_Instance` (`.filter_ntsc`); defaults (zeroed setup, NULL
      emu) match the old static initialisers via zero-init
      (`FILTER_NTSC_PreInitialise_Ctx` applies the composite preset at
      startup, as before). The `presets[]` and `preset_cfg_strings[]`
      tables stay file-scope `static const` *(shared, read-only)*.
- [x] **Artifact** — [`src/artifact.h`](src/artifact.h): `ARTIFACT_Set`,
      `ARTIFACT_SetTVMode`, `ARTIFACT_WriteConfig`, `ARTIFACT_ReadConfig`,
      `ARTIFACT_Initialise`; state `ARTIFACT_mode` (+ per-TV-system
      `mode_ntsc`/`mode_pal` internals).
      Done 2026-09-16: each is now
      `ARTIFACT_*_Ctx(Atari800_Instance *inst, ...)` in `artifact.c`, pinning
      the file-scope context (`AR = &inst->artifact`, `ARi = inst`) via
      `ARTIFACT_PIN_CTX(inst)`; the state aliases inside `artifact.c` route
      through `AR`. Include-cycle note: `instance.h` includes `artifact.h`
      (for the `ARTIFACT_t` enum used by `Artifact_state_t`), so `artifact.h`
      forward-declares `struct Atari800_Instance` for the `_Ctx` prototypes
      *before* including `instance.h`; the `ARTIFACT_mode` alias and
      forwarding macros come after. Per-instance end-to-end: the `_Ctx`
      bodies `#undef` the ANTIC aliases (`ANTIC_artif_mode`, `ANTIC_artif_new`,
      `ANTIC_pal_blending`, `ANTIC_UpdateArtifacting`) and re-point them to
      `ARi->antic.*` / `ANTIC_UpdateArtifacting_Ctx(ARi)`. Transitional
      (still default-instance): `Atari800_tv_mode` (top-level config, §4.4)
      and the `VIDEOMODE_Update()` call under NTSC_FILTER &&
      SUPPORTS_CHANGE_VIDEOMODE (Videomode deferred above). Callers in
      `atari.c`, `cfg.c`, `ui.c`, `pal_blending.c` and `sdl/video*.c` go
      through the forwarding macros (transitional). Build passes; 20 s no-disk
      smoke run clean; Acid800 results identical to the pre-refactor baseline.
- [x] Move state (Artifact): `ARTIFACT_mode` plus the previously file-scope
      statics `mode_ntsc`/`mode_pal`.
      Done 2026-09-16: `Artifact_state_t` defined in
      [`src/instance.h`](src/instance.h) and **embedded by value** in
      `Atari800_Instance` (`.artifact`); `artifact.h` aliases `ARTIFACT_mode`
      to `Atari800_default->artifact.mode`. Defaults (all `ARTIFACT_NONE`,
      i.e. 0) match the old static initialisers via zero-init. The
      `mode_cfg_strings[]` table stays file-scope `static const`
      *(shared, read-only)*.
- [ ] **File_export** — [`src/file_export.h`](src/file_export.h): `FILE_EXPORT_*`
      functions and state. **Deferred (host-output state):** `file_export.c` is
      tightly coupled to the `codecs/` subsystem — `container`,
      `audio_codec`, `video_codec`, `image_codec`, `video_frame_count`, `fps`,
      `byteswritten`, and `description` are file-scope globals in
      `src/codecs/container.c`/`audio.c`/`video.c`/`image.c`, none of which are
      instance-aware yet. File_export's own state is small (`error_msg`,
      `FILE_EXPORT_compression_level`, the sound/video filename patterns and
      counters under AUDIO_RECORDING/VIDEO_RECORDING/SCREENSHOTS), but
      converting it before the codecs would produce a misleading per-instance
      split (recording is a host-output concern: one encoder pipeline, one
      output file). Revisit together with Sound/Pokeysnd (§4.3) and the
      codecs subsystem (a new §4.8-style work item), or with the platform
      layer (Phase 6). Note: `Screen_SaveScreenshot_Ctx`/`SaveNextScreenshot_Ctx`
      call `File_Export_*` via the legacy names (transitional: default
      instance), and `File_Export_WriteAudio`/`WriteVideo` are called from the
      sound/video paths (`pokeysnd.c`, `sdl/video*.c`) — all transitional.

### 4.3 Sound

- [ ] **Sound** — [`src/sound.h`](src/sound.h): `Sound_Initialise`, `Sound_Exit`,
      `Sound_Update`, `Sound_Pause`, `Sound_Continue`, `Sound_Setup`,
      `Sound_Callback`, `Sound_SetLatency`, `Sound_AdjustSpeed`; state
      `Sound_desired`, `Sound_out`, `Sound_enabled`, `Sound_latency`.
- [ ] **Pokeysnd** — [`src/pokeysnd.h`](src/pokeysnd.h): `POKEYSND_*` functions
      and state.
- [ ] **Mzpokeysnd** — [`src/mzpokeysnd.h`](src/mzpokeysnd.h): `MZPOKEYSND_*`
      functions and state.

> **Phase 6 audit note (2026-09-19):** `src/android/jni/sound.c` defines its
> own `Sound_out`/`Sound_enabled`/`Sound_latency` (the Android port ships its
> own `sound.c` in place of the core one), so converting the Sound module
> must account for that port-local duplicate (or the port must be re-pointed
> at the core module). Recorded during the Phase 6 port audit.

### 4.4 Top-level driver — [`src/atari.h`](src/atari.h), [`src/atari.c`](src/atari.c)

> **Status: done (2026-09-18).** All per-instance driver state now lives in
> `Atari800_Instance` (the top-level config fields were already declared there;
> the driver internals `refresh_counter`, `sync_lasttime`,
> `last_display_screen_time`, `afs_lastframe/afs_discard/afs_lasttime/
> afs_sleeptime`, and `benchmark_start_time` (BENCHMARK) were added). Build
> passes; 20 s no-disk smoke run clean (no CIM); Acid800 results identical to
> the pre-refactor baseline (23 success / 28 expected failures / 2 skipped;
> the 2 FAILs — "MMU: XL banking" and "suite totals changed" — are
> pre-existing, see [`docs/acid800-expected-results.md`](acid800-expected-results.md)).

- [x] Convert: `Atari800_Frame`, `Atari800_Coldstart`, `Atari800_Warmstart`,
      `Atari800_InitialiseMachine`, `Atari800_Sync`, `Atari800_StateSave`,
      `Atari800_StateRead`, `Atari800_SetTVMode`, `Atari800_SetMachineType`,
      `Atari800_UpdateJumper`, `Atari800_UpdateKeyboardDetached`.
      Done 2026-09-18: each is now
      `Atari800_*_Ctx(Atari800_Instance *inst, ...)` in `atari.c`, pinning the
      file-scope context (`AI = inst`) via `ATARI_PIN_CTX(inst)`; the legacy
      names — this module's own state and the chip/peripheral aliases pulled
      in from the module headers (`MEMORY_*`, `GTIA_*`, `ANTIC_*`, `POKEY_*`,
      `PIA_Reset`, `PBI_Reset`, `CARTRIDGE_ColdStart`, `ESC_ClearAll`,
      `Devices_*`, `Colours_*`, `ARTIFACT_*`, `AF80_*`, `BIT3_*`, `INPUT_*`,
      `Screen_*`, `PBI_BB_Frame`) — are `#undef`'ed and re-pointed to `AI`
      inside `atari.c`, so the `_Ctx` bodies operate on their own instance.
      In `atari.h` the legacy names are forwarding macros passing
      `Atari800_default`. **Include-cycle note:** `instance.h` includes
      `atari.h`, so `atari.h` cannot include `instance.h`; instead `atari.h`
      forward-declares `struct Atari800_Instance` and declares
      `extern struct Atari800_Instance *Atari800_default` (the canonical
      declaration — `instance.h` no longer redeclares it, avoiding
      `-Wredundant-decls`), and the alias macros expand to
      `Atari800_default->...`. TUs that use the aliases need the complete
      struct type, i.e. `instance.h` transitively (all in-tree TUs get it).
      Exceptions: `Atari800_Initialise`, `Atari800_Exit`, `Atari800_ErrExit`
      remain process-level functions (they drive config parsing, the platform
      layer, and the still-transitional module init/exit calls); they pin `AI`
      to the default instance at entry. `Atari800_LoadImage` is stateless
      (signature unchanged). The BASIC/VERY_SLOW/CURSES_BASIC-only
      `basic_frame`/`basic_antic_scanline` helpers still use the
      default-instance aliases (not part of the Linux build; revisit if those
      builds are needed per-instance). The multi-instance lifecycle wrappers
      (`Atari800_FrameInstance`/`ColdstartInstance`/`WarmstartInstance`,
      §1.4) are implemented as thin wrappers over the `_Ctx` functions.
- [x] Move state: `Atari800_machine_type`, `Atari800_builtin_basic`,
      `Atari800_keyboard_leds`, `Atari800_f_keys`, `Atari800_jumper`,
      `Atari800_builtin_game`, `Atari800_keyboard_detached`, `Atari800_tv_mode`,
      `Atari800_disable_basic`, `Atari800_os_version`, `Atari800_display_screen`,
      `Atari800_nframes`, `Atari800_refresh_rate`,
      `Atari800_collisions_in_skipped_frames`, `Atari800_turbo`,
      `Atari800_turbo_speed`, `Atari800_start_in_monitor`,
      `Atari800_auto_frameskip`, plus the driver internals listed in the
      status note above.
      Done 2026-09-18: all moved into `Atari800_Instance`
      ([`src/instance.h`](src/instance.h)); `atari.c`/`atari.h` alias the
      legacy names to `Atari800_default->...`. Non-zero defaults preserved in
      the `default_instance_storage` initializer in `atari.c`
      (`machine_type = Atari800_MACHINE_XLXE`, `builtin_basic = TRUE`,
      `tv_mode = Atari800_TV_PAL`, `disable_basic = TRUE`,
      `os_version = -1`, `refresh_rate = 1`); the rest match the old static
      initialisers via zero-init.
- [x] Keep shared: `verbose`, `sigint_flag`, `dl_dir` (process-global) —
      still file-scope globals in `atari.c`.

### 4.5 State save/load — [`src/statesav.h`](src/statesav.h), [`src/statesav.c`](src/statesav.c)

> **Status: done (2026-09-18).** The save/read stream is per-instance
> (`Statesav_state_t`, embedded by value in `Atari800_Instance`), and every
> module's `_Ctx` StateSave/StateRead body now routes its `StateSav_*` calls
> through its own instance, so a state save of instance X writes only X's
> state to X's stream. Build passes; 20 s no-disk smoke run clean (no CIM);
> a monitor-driven `savestate`/`loadstate` round-trip succeeds; Acid800
> results identical to the pre-refactor baseline (23 success / 28 expected
> failures / 2 skipped; the 2 FAILs — "MMU: XL banking" and "suite totals
> changed" — are pre-existing, see
> [`docs/acid800-expected-results.md`](acid800-expected-results.md)).

- [x] Convert: `StateSav_SaveAtariState`, `StateSav_ReadAtariState`,
      `StateSav_SaveUBYTE`, `StateSav_SaveUWORD`, `StateSav_SaveINT`,
      `StateSav_SaveFNAME`, `StateSav_ReadUBYTE`, `StateSav_ReadUWORD`,
      `StateSav_ReadINT`, `StateSav_ReadFNAME`, `StateSav_Tell`.
      Done 2026-09-18: each is now
      `StateSav_*_Ctx(Atari800_Instance *inst, ...)` in `statesav.c`, pinning
      the file-scope context (`SS = &inst->statesav`) via
      `STATESAV_PIN_CTX(inst)`; the stream is accessed through
      `StateFile`/`nFileError` macros routed through `SS` (the stream is
      stored opaquely as `void *` in `Statesav_state_t`, since it is a
      `gzFile`, `FILE *` or `char *` depending on the build configuration).
      In `statesav.h` the legacy names are forwarding macros passing
      `Atari800_default`. `StateSav_Tell` (LIBATARI800 only) keeps its
      signature — it reads the shared in-memory `plainmemoff` cursor of the
      libatari800 save buffer (single-stream; revisit in Phase 5).
      `SaveAtariState_Ctx`/`ReadAtariState_Ctx` dispatch to the module
      `_Ctx` StateSave/StateRead functions with their own instance
      (`Atari800_StateSave_Ctx(inst)`, `CARTRIDGE_StateSave_Ctx(inst)`,
      `SIO_StateSave_Ctx(inst)`, `ANTIC_StateSave_Ctx(inst)`,
      `CPU_StateSave(inst, ...)`, `GTIA_StateSave_Ctx(inst)`,
      `PIA_StateSave_Ctx(inst)`, `POKEY_StateSave_Ctx(inst)`,
      `XEP80_StateSave_Ctx(inst)`, `PBI_StateSave_Ctx(inst)`,
      `PBI_MIO_StateSave_Ctx(inst)`, `PBI_BB_StateSave_Ctx(inst)`,
      `PBI_XLD_StateSave_Ctx(inst)`). Build passes; monitor
      `savestate`/`loadstate` round-trip OK; Acid800 results identical to
      the pre-refactor baseline.
- [x] Make the save/read stream per-instance.
      Done 2026-09-18: `Statesav_state_t` (the opaque `StateFile` stream
      pointer and `nFileError`) defined in
      [`src/instance.h`](src/instance.h) and **embedded by value** in
      `Atari800_Instance` (`.statesav`); defaults (NULL stream, no error)
      match the old static initialisers via zero-init. The module
      `_Ctx` StateSave/StateRead bodies (antic.c, gtia.c, pokey.c, pia.c,
      memory.c, cartridge.c, sio.c, pbi.c, pbi_bb.c, pbi_mio.c, pbi_xld.c,
      xep80.c, atari.c, cpu.c) were re-routed to call
      `StateSav_*_Ctx(inst, ...)`, so the whole save/load path is
      per-instance end-to-end. Transitional (still default-instance via the
      forwarding macros): the platform ports (`dc/atari_dc.c`'s config
      save, `amiga/amiga.c`, `android/jni/jni.c`, `libatari800/statesav.c`)
      and `atari.c`'s `-state`-file read in `Atari800_Initialise`
      (process-level, pins the default instance).

### 4.6 Monitor / debugger — [`src/monitor.h`](src/monitor.h), [`src/monitor.c`](src/monitor.c)

- [x] Convert: `MONITOR_Run`, `MONITOR_Exit`, `MONITOR_ShowState`, `MONITOR_BBRK_on`,
      `MONITOR_BPC`.
      Done 2026-09-18: each is now
      `MONITOR_*_Ctx(Atari800_Instance *inst, ...)` in `monitor.c`, pinning
      the file-scope contexts (`MO = &inst->monitor`, `MI = inst`) via
      `MONITOR_PIN_CTX(inst)`; the monitor-state aliases and the
      emulator-state macros pulled in from the module headers (`CPU_reg*`,
      `CPU_remember_*`, the `MEMORY_*` accessors, `ANTIC_*`, `GTIA_*`,
      `POKEY_*`, `PIA_*`, `CARTRIDGE_main/piggyback`,
      `Atari800_machine_type`) are `#undef`'ed and re-pointed to `MO`/`MI`
      inside `monitor.c`, so the ~4000 lines of command bodies are unchanged
      and operate on their own instance. In `monitor.h` the legacy names are
      forwarding macros passing `Atari800_default`. `MONITOR_Run_Ctx` calls
      `CPU_GetStatus(MI)`, `Atari800_Coldstart_Ctx(MI)`/`Warmstart_Ctx(MI)`,
      and `StateSav_SaveAtariState_Ctx(MI, ...)`/`ReadAtariState_Ctx(MI, ...)`
      (own instance). `cpu.c`'s `CPU_GO` re-points the monitor break/
      breakpoint/coverage aliases to its own instance (so breakpoints fire
      per instance) and passes its instance to `MONITOR_ShowState_Ctx`
      (MONITOR_TRACE). Exceptions: `MONITOR_PreloadLabelFile` stays
      context-free (the label/symtable state is process-global debug
      tooling; it pins the default instance so its `Atari800_machine_type`
      alias is valid). `cpu.c`'s `DO_BREAK` -> `ENTER_MONITOR` still enters
      the process-level monitor loop (`Atari800_Exit`), which is inherently
      single-instance (transitional). Build passes; monitor smoke run
      (savestate/loadstate round-trip) OK; Acid800 results identical to the
      pre-refactor baseline.
- [x] Move state: `MONITOR_break_addr`, `MONITOR_break_step`, `MONITOR_break_ret`,
      `MONITOR_break_brk`, `MONITOR_ret_nesting`, `MONITOR_breakpoint_table[]`,
      `MONITOR_breakpoint_table_size`, `MONITOR_breakpoints_enabled`,
      `MONITOR_coverage[]`, `MONITOR_coverage_insns`, `MONITOR_coverage_cycles`.
      Done 2026-09-18: all moved into `Monitor_state_t`
      ([`src/instance.h`](src/instance.h), which also now defines the
      `MONITOR_breakpoint_cond`/`MONITOR_coverage_rec` typedefs and
      `MONITOR_BREAKPOINT_TABLE_MAX`, moved there from `monitor.h` so the
      table can be embedded by value) and **embedded by value** in
      `Atari800_Instance` (`.monitor`); `monitor.c`/`monitor.h` alias the
      legacy names to `Atari800_default->monitor.*`, and `cpu.c` re-points
      them to its own instance inside `CPU_GO`. Default-instance init
      preserved in the `default_instance_storage` initializer in `atari.c`
      (`break_addr = 0xd000`, `breakpoints_enabled = TRUE`); the rest match
      the old static initialisers via zero-init. Note:
      `MONITOR_coverage[0x10000]` is ~1 MB per instance (MONITOR_PROFILE is
      a non-default debug build option). Remaining file-scope in
      `monitor.c` (debug tooling, treated as process-global): the
      `symtable_*` label tables (MONITOR_HINTS), `MONITOR_trace_file`
      (MONITOR_TRACE host output), `trainer_memory`/`trainer_flags`
      (trainer scratch), the readline completion statics, and the
      "repeat last command" function-local statics in `coverage()`/
      `string_search()`/`print_graphics()`.
- [x] Keep shared: `MONITOR_optype6502[256]` *(shared, read-only)* —
      still a file-scope const global in `monitor.c`.

### 4.7 Config / ROM / util / log

> **Status: complete (2026-09-18).** This phase is an audit: these four modules
> are intentionally **process-global** and stay that way for now. No signature
> changes were needed. Build passes; 20 s no-disk XL smoke run clean (no CIM).

- [x] **Cfg** — [`src/cfg.h`](src/cfg.h): `CFG_*` functions — **stays
      process-global**. Audited 2026-09-18:
      - `CFG_LoadConfig` / `CFG_WriteConfig` operate on the process-global
        `rtconfig_filename` (file-static) and write per-instance emulator
        options through the legacy-name aliases, which currently resolve to
        `Atari800_default->...` (e.g. `MEMORY_ram_size`,
        `Atari800_machine_type`, `Devices_enable_h_patch`). Since only the
        default instance exists, this routing is correct today; when
        multi-instance config support is added (Phase 5+), `CFG_LoadConfig` /
        `CFG_WriteConfig` must gain an `Atari800_Instance *` parameter (or a
        "config target instance" pointer) so per-instance options are written
        to the right instance. Recorded as follow-up.
      - `CFG_save_on_exit` and `CFG_data_dir` stay process-global
        (config-file bookkeeping, not machine state).
      - `CFG_MatchTextParameter` is stateless; signature unchanged.
- [x] **Sysrom** — [`src/sysrom.h`](src/sysrom.h): `SYSROM_*` functions —
      **stays process-global**. Audited 2026-09-18:
      - `SYSROM_roms[]`, `SYSROM_os_versions[]`, `SYSROM_basic_version`,
        `SYSROM_xegame_version` are configuration/ROM-selection state set once
        at startup from the config file; they are consumed by
        `MEMORY_InitialiseMachine` as *(shared, read-only)* ROM sources. Kept
        global; when per-instance ROM selection is needed, these move into a
        `SYSROM_state_t` reachable from the instance.
      - The file-static `*_filename[FILENAME_MAX]` path buffers, `num_unset_roms`,
        and the `cfg_strings*` / `autochoose_order_*` tables are
        config-parsing-only state — process-global is correct.
- [x] **Util** — [`src/util.h`](src/util.h): `Util_*` helpers — audited
      2026-09-18: no hidden global/mutable state. The only `static` in
      [`src/util.c`](src/util.c) is the helper function `parse_hashes`
      (line 366) — a static *function*, not static *data*. All `Util_*`
      functions are stateless/re-entrant; no changes needed.
- [x] **Log** — [`src/log.h`](src/log.h): `Log_*` functions — **stays
      process-global** (shared). Audited 2026-09-18: `Log_print` /
      `Log_flushlog` only touch `Log_buffer` (present only with
      `BUFFERED_LOG`), which is diagnostic output, not machine state. No
      changes needed.

---

## Phase 5 — libatari800 library

> **Status: done (2026-09-18).** The library wrapper is re-implemented on top
> of the instance API: every per-machine entry point now has a
> `libatari800_*_Ctx(Atari800_Instance *inst, ...)` form, the legacy
> un-suffixed names are forwarding macros over `Atari800_default` (declared
> in [`src/libatari800/libatari800.h`](src/libatari800/libatari800.h)), and
> the per-instance wrapper state (input template, in-memory state-save
> buffer/tags, sound buffer) lives in the new `Libatari800_state_t`,
> **embedded by value** in `Atari800_Instance` (`.libatari800`). The
> PLATFORM_* callbacks (keyboard/joystick/triggers/sound) route through a
> file-scope "current instance" pointer pinned by
> `libatari800_next_frame_Ctx` / `LIBATARI800_Frame_Ctx`
> (`LIBATARI800_SetCurrentInstance`/`CurrentInstance` in
> [`src/libatari800/input.c`](src/libatari800/input.c)), so frames are
> per-instance end-to-end under serialized (single-threaded) driving.
> Multi-instance entry points `libatari800_new_instance` /
> `libatari800_free_instance` were added. Verified: the libatari800 target
> builds (`./configure --target=libatari800`), `libatari800_test` passes
> (200 frames incl. a state save/load round-trip), an ad-hoc smoke test
> creates a second instance alongside the default and runs 50 frames, the
> default target still builds, Acid800 results identical to the pre-refactor
> baseline (23 success / 28 expected failures / 2 skipped; the 2 FAILs —
> "MMU: XL banking" and "suite totals changed" — are pre-existing, see
> [`docs/acid800-expected-results.md`](acid800-expected-results.md)), and a
> 20 s no-disk XL smoke run is clean (no CIM). Build fixes en route: the
> default-instance initializer's `.xep80` block in
> [`src/atari.c`](src/atari.c) is now guarded by `#ifdef XEP80_EMULATION`
> (it referenced `XEP80_CHAR_HEIGHT_NTSC`, which does not exist in
> non-XEP80 builds such as the libatari800 target), and
> `libatari800.h`'s `ULONG` now matches `atari.h`'s (`unsigned int`) to
> avoid a `-Wredefinition` under `-Werror`.

- [x] **api.c** — [`src/libatari800/api.c`](src/libatari800/api.c):
      `libatari800_init`, `libatari800_next_frame`, and the
      `libatari800_get_*` accessors re-implemented on top of the instance
      API. Done 2026-09-18: each per-machine function is now
      `libatari800_*_Ctx(Atari800_Instance *inst, ...)`; the bodies operate
      on `inst` directly (`inst->memory.mem`, `inst->screen.atari`,
      `inst->libatari800.*`, `inst->nframes`, `inst->tv_mode`,
      `inst->cpu.cim_encountered`, `inst->antic.dlist`,
      `inst->input.key_code`, `inst->esc.enable_sio_patch`,
      `inst->memory.selftest_enabled`) and call the module `_Ctx` functions
      with their own instance (`SIO_Mount_Ctx`, `Atari800_Coldstart_Ctx`,
      `LIBATARI800_Mouse_Ctx`, `LIBATARI800_Frame_Ctx`,
      `LIBATARI800_StateSave_Ctx`/`StateLoad_Ctx`). In
      [`src/libatari800/libatari800.h`](src/libatari800/libatari800.h) the
      legacy names are forwarding macros passing `Atari800_default` (the
      header forward-declares `struct Atari800_Instance` and the
      `Atari800_default` extern, and typedefs it as
      `libatari800_instance_t`). Exceptions (process-level, unchanged):
      `libatari800_init` (drives the process-level `Atari800_Initialise`,
      which sets up the default instance), `libatari800_exit`,
      `libatari800_error_message`,
      `libatari800_continue_emulation_on_brk`,
      `libatari800_set_disk_activity_callback`, and the
      `libatari800_cpu_crash` jmp_buf / `libatari800_error_code` /
      `libatari800_continue_on_brk` globals (per-frame flow control, shared;
      the jmp_buf is only live within one `next_frame` call). Transitional
      (still default-instance): `AFILE_OpenFile` in
      `libatari800_reboot_with_file_Ctx` (AFILE is not yet instance-aware)
      and the `Sound_out` reads in the sound accessors (Sound module,
      Phase 4.3).
- [x] **init.c** — [`src/libatari800/init.c`](src/libatari800/init.c):
      `LIBATARI800_Initialise`/`ReadConfig`/`Exit` audited — they only touch
      the process-global `libatari800_continue_on_brk` flag (config option
      `LIBATARI800_CONTINUE_ON_BRK`), which is shared flow control, not
      machine state. No signature changes needed.
- [x] **input.c** — [`src/libatari800/input.c`](src/libatari800/input.c):
      per-instance input array. Done 2026-09-18: the input template pointer
      moved into `Libatari800_state_t.input_array`
      ([`src/instance.h`](src/instance.h)); `input.h` aliases
      `LIBATARI800_Input_array` to
      `Atari800_default->libatari800.input_array`. `PLATFORM_Keyboard`,
      `PLATFORM_PORT`, `PLATFORM_TRIG` and `LIBATARI800_Mouse_Ctx(inst)`
      read the current instance's template and `#undef`/re-point the
      emulator-state aliases they write (`INPUT_key_shift/key_consol`,
      `BINLOAD_pause_loading`, `INPUT_mouse_delta_x/y`, `INPUT_mouse_buttons`,
      `POKEY_POT_input`, `INPUT_mouse_port`) to that instance, so input is
      per-instance end-to-end. The file-scope `lastkey`/`key_control` scratch
      statics remain file-scope (per-call transient). `LIBATARI800_Mouse`
      legacy name is a forwarding macro in
      [`src/libatari800/input.h`](src/libatari800/input.h).
- [x] **video.c** — [`src/libatari800/video.c`](src/libatari800/video.c):
      audited — `PLATFORM_DisplayScreen` is a no-op stub and
      `LIBATARI800_Video_Initialise`/`Exit` are stateless; the framebuffer
      access is per-instance via `libatari800_get_screen_ptr_Ctx`
      (`inst->screen.atari`). No changes needed.
- [x] **sound.c** — [`src/libatari800/sound.c`](src/libatari800/sound.c):
      per-instance audio. Done 2026-09-18: the sound buffer, fill level,
      hardware buffer size, `sample_diff` and `sample_residual` moved into
      `Libatari800_state_t`; `sound.h` aliases the legacy names to
      `Atari800_default->libatari800.*`. `PLATFORM_SoundSetup`/`Exit`/
      `Available`/`Write` operate on the current instance (pinned via
      `LIBATARI800_CurrentInstance()`), so each instance owns its sound
      buffer. Transitional (still process-global): `Sound_out` (Sound
      module, Phase 4.3) and `Atari800_tv_mode` in `PLATFORM_SoundSetup`
      (read through the default-instance alias; sound setup happens at
      process init).
- [x] **statesav.c** — [`src/libatari800/statesav.c`](src/libatari800/statesav.c):
      per-instance state save/load. Done 2026-09-18: `LIBATARI800_StateSave`
      /`StateLoad` are now `*_Ctx(inst, ...)`; the in-memory buffer and tag
      table live in `inst->libatari800.statesav_buffer`/`statesav_tags`
      (legacy names aliased to the default instance in
      [`src/libatari800/statesav.h`](src/libatari800/statesav.h)). In
      [`src/statesav.c`](src/statesav.c) the LIBATARI800 in-memory stream is
      now selected per save/load: `StateSav_SaveAtariState_Ctx`/
      `ReadAtariState_Ctx` copy `inst->libatari800.statesav_buffer` into a
      file-scope pointer consumed by `mem_open()`, and `STATESAV_TAG`
      ([`src/statesav.h`](src/statesav.h)) writes
      `inst->libatari800.statesav_tags` (all use sites are inside `_Ctx`
      functions with `inst` in scope). The `plainmemoff` cursor behind
      `StateSav_Tell` remains a single shared stream (saves are serialized;
      the pre-existing §4.5 note stands).
- [x] **main.c**, **exit.c**, **guess_settings.c** — update to instance API.
      Done 2026-09-18: `LIBATARI800_Frame` is now
      `LIBATARI800_Frame_Ctx(Atari800_Instance *inst)`
      ([`src/libatari800/main.c`](src/libatari800/main.c)), pinning the
      current instance and calling the module `_Ctx` functions with it
      (`Atari800_Coldstart/Warmstart_Ctx`, `PBI_BB_Frame_Ctx`,
      `Devices_Frame_Ctx`, `INPUT_Frame_Ctx`, `GTIA_Frame_Ctx`,
      `ANTIC_Frame_Ctx`, `INPUT_DrawMousePointer_Ctx`,
      `Screen_Draw*_Ctx`, `POKEY_Frame_Ctx`); `inst->nframes++` replaces the
      global. Transitional (still default-instance): `VOTRAXSND_Frame`
      (Votrax deferred, §3.7) and `Sound_Update` (Phase 4.3).
      `PLATFORM_Initialise`/`PLATFORM_Configure`/`PLATFORM_Exit`
      ([`src/libatari800/exit.c`](src/libatari800/exit.c)) are process-level
      and unchanged. `guess_settings.c` and `libatari800_test.c` use the
      legacy forwarding macros and compile unchanged.
- [x] Add multi-instance entry points (`libatari800_new_instance`, etc.).
      Done 2026-09-18: `libatari800_new_instance()` (wraps
      `Atari800_NewInstance`) and `libatari800_free_instance()` (wraps
      `Atari800_FreeInstance`) declared in
      [`src/libatari800/libatari800.h`](src/libatari800/libatari800.h);
      verified with an ad-hoc smoke test (second instance created and freed
      alongside 50 frames on the default instance). **Follow-up (Phase 5+ /
      cross-cutting):** a newly created instance has only the built-in
      defaults — full per-instance initialization (config parsing, ROM
      selection, `Atari800_InitialiseMachine`) still routes through the
      process-level `Atari800_Initialise`; per-instance config support is
      the recorded §4.7 CFG follow-up. Until then, only the default instance
      is fully initialized.

---

## Phase 6 — Platform ports

> **Status: complete (2026-09-19).** The Phase 6 blockers recorded in
> earlier phases are resolved: the per-instance palettes (`Colours_table`,
> `AF80_palette`, `BIT3_palette`) now live in the instance state and
> [`src/sdl/palette.c`](src/sdl/palette.c) resolves them at runtime instead
> of from a static initialiser; the two flagged port fixes
> (`wince/port/main.c` self-declared externs, `atari_rpi.c` self-declared
> NTSC-filter globals) are done. The remaining ports (§6.3) were audited
> (2026-09-19): every port touches per-instance emulator state only at
> runtime through the header aliases/forwarding macros — no port has a
> static initialiser dereferencing per-instance state (the only such
> blocker, `sdl/palette.c`, was resolved in §6.1) and no port defines a
> global that shadows per-instance emulator state. Per the phase note
> below, the standalone ports keep driving the single default instance
> through the aliases/forwarding macros, which is correct while the
> multi-instance API is exposed only through `libatari800`; converting the
> port files to explicit `_Ctx` calls is deferred until a port actually
> needs to drive a non-default instance. Build passes; 20 s no-disk XL
> smoke run clean (no CIM); Acid800 results identical to the pre-refactor
> baseline (23 success / 28 expected failures / 2 skipped; the 2 FAILs —
> "MMU: XL banking" and "suite totals changed" — are pre-existing, see
> [`docs/acid800-expected-results.md`](acid800-expected-results.md)).
> **Audit note (2026-09-19):** `src/android/jni/sound.c` defines its own
> `Sound_out`/`Sound_enabled`/`Sound_latency` (the Android port ships its
> own `sound.c` in place of the core one) — that is a Phase 4.3 (Sound)
> concern, not a Phase 6 blocker; recorded there.

### 6.1 SDL — [`src/sdl/`](src/sdl/)

- [x] **Per-instance palettes end-to-end** (the recorded Phase 6 blocker).
      Done 2026-09-19: `Colours_table[256]` moved into `Colours_state_t`
      (`Colours_state_t.table`,
      [`src/instance.h`](src/instance.h)); `colours.c` aliases the legacy
      name to the pinned context (`CO->table`) and `colours.h` aliases it to
      `Atari800_default->colours.table` for the not-yet-migrated callers
      (`image_pcx.c`, `atari_x11.c`, and the amiga/dos/ps2/javanvm ports —
      transitional). `AF80_palette[16]` moved into `AF80_state_t.palette`
      (still derived from the shared read-only `rgbi_palette` table by
      `AF80_Initialise_Ctx`); `BIT3_palette[2]` moved into
      `BIT3_state_t.palette` (the old static initialiser values —
      `0x000000`/`0xFFFFFF` — are preserved in the default-instance
      initializer in `atari.c`). `sdl/palette.c`'s `SDL_PALETTE_tab` static
      initialiser (which could not dereference `Atari800_default`) was
      replaced with a runtime `SDL_PALETTE_Initialise()` that resolves the
      tab pointers from `Atari800_default` (called at the top of
      `SDL_VIDEO_Initialise` in `sdl/video.c`); the pointer targets never
      change, so later palette-content updates are picked up automatically.
      The SDL display path (video.c/video_sw.c/video_gl.c) consumes the
      palettes through the unchanged `SDL_PALETTE_tab` interface. Build
      passes; smoke run + Acid800 as above.
- [x] Remaining SDL files (`main.c`, `init.c`, `input.c`, `sound.c`,
      `video.c`, `video_gl.c`, `video_sw.c`): keep using the default
      instance via the header aliases/forwarding macros — correct per the
      phase note (single default instance; per-instance input/screen/sound
      already exists in the core). Explicit `_Ctx` conversion deferred until
      a port drives a non-default instance.

### 6.2 Flagged port fixes

- [x] **WinCE** — [`src/wince/port/main.c`](src/wince/port/main.c): the
      self-declared `extern UBYTE *Screen_dirty;` /
      `extern void Screen_EntireDirty(void);` were replaced with
      `#include "screen.h"` (Screen_dirty is per-instance now, aliased to
      the default instance via the header). Not part of the Linux build
      (Windows CE headers unavailable), so compile-verified by inspection
      only.
- [x] **Raspberry Pi** — [`src/atari_rpi.c`](src/atari_rpi.c): the
      self-declared `FILTER_NTSC_emu`/`FILTER_NTSC_setup` dummies now
      `#include "filter_ntsc.h"` and `#undef` the header aliases before
      defining the port-local stubs, so the file no longer silently
      diverges from `Filter_ntsc_state_t` when the RPi build starts using
      the real filter module. Not part of the Linux build
      (`bcm_host.h` unavailable), so compile-verified by inspection only.

### 6.3 Remaining ports

> **Status: audited (2026-09-19).** Each port below was audited for
> compatibility with the per-instance refactor: all uses of per-instance
> emulator state (`Screen_atari`, `Colours_table`, `POKEY_POT_input`, ...)
> are runtime accesses through the header aliases (default instance), no
> port has a static initialiser dereferencing per-instance state, and no
> port defines a global that shadows per-instance emulator state (the
> port-local globals found — `amiga.c`'s `fps`/directory strings,
> `atari_dc.c`'s `db_mode`/`screen_tv_mode`/`emulate_paddles`, the
> `gles2/video.c` `op_filtering`/`op_zoom` options, the `wince`/`android`
> port-local UI flags — are all port-private). The ports keep driving the
> single default instance via the aliases/forwarding macros, which is
> correct per the phase note; explicit `_Ctx` conversion is deferred until
> a port needs to drive a non-default instance. Ports not in the Linux
> build (all except X11) were compile-verified by inspection only.

- [x] **X11** — [`src/atari_x11.c`](src/atari_x11.c). Audited 2026-09-19:
      `Colours_table` (palette copy loop) and `POKEY_POT_input` (mouse
      paddle emulation) are runtime uses via the `colours.h`/`pokey.h`
      aliases (transitional: default instance); the `static int
      window_height = Screen_HEIGHT` etc. initialisers use macro constants
      only. Builds as part of the tree.
- [x] **Raspberry Pi** — [`src/atari_rpi.c`](src/atari_rpi.c). Audited
      2026-09-19: the flagged NTSC-filter fix is done (§6.2); the
      port-local `op_filtering`/`op_zoom` globals are port-private (shared
      with `gles2/video.c`, not emulator state). Not in the Linux build;
      verified by inspection.
- [x] **Curses** — [`src/atari_curses.c`](src/atari_curses.c). Audited
      2026-09-19: framebuffer access via the `screen.h` aliases only
      (runtime). Not in the Linux build; verified by inspection.
- [x] **BASIC** — [`src/atari_basic.c`](src/atari_basic.c). Audited
      2026-09-19: uses `ui_basic.c`'s `Screen_atari` rendering helpers
      (runtime, via the `screen.h` aliases). Not in the Linux build;
      verified by inspection.
- [x] **PS2** — [`src/atari_ps2.c`](src/atari_ps2.c). Audited 2026-09-19:
      `Colours_table` (CLUT build) and `Screen_atari` (texture memory) are
      runtime uses via the aliases. Not in the Linux build; verified by
      inspection.
- [x] **Amiga** — [`src/amiga/`](src/amiga/). Audited 2026-09-19:
      `Colours_table`/`Screen_atari` runtime uses via the aliases; the
      port-local globals (`fps`, `atari_disk_dirs`, `program_name`, ...)
      are port-private. Not in the Linux build; verified by inspection.
- [x] **Android** — [`src/android/`](src/android/). Audited 2026-09-19:
      the JNI layer's globals are port-private; `jni.c`'s `extern void
      Sound_Exit/Pause/Continue` declarations match the still
      process-global Sound module (Phase 4.3). Note:
      `android/jni/sound.c` defines its own `Sound_out`/`Sound_enabled`/
      `Sound_latency` (the port ships its own `sound.c` replacing the
      core one) — a Phase 4.3 concern, recorded there. Not in the Linux
      build; verified by inspection.
- [x] **macOS** — [`src/macosx/`](src/macosx/). Shipped as a tarball
      (`macosx.tar.gz`); no in-tree sources to audit.
- [x] **Falcon** — [`src/falcon/`](src/falcon/). Audited 2026-09-19:
      `Screen_atari` runtime use via the `screen.h` alias; the port-local
      keyboard/palette globals are port-private. Not in the Linux build;
      verified by inspection.
- [x] **Dreamcast** — [`src/dc/`](src/dc/). Audited 2026-09-19: the
      port-local globals (`db_mode`, `screen_tv_mode`, `emulate_paddles`,
      declared `extern` by `ui.c` under `DREAMCAST`) are port-private;
      the config save path uses the `StateSav_*` forwarding macros
      (transitional: default instance, per §4.5). Not in the Linux build;
      verified by inspection.
- [x] **DOS** — [`src/dos/`](src/dos/). Audited 2026-09-19:
      `atari_vga.c`'s `static int scr_ptr_inc = Screen_WIDTH` initialiser
      uses a macro constant only; `Screen_atari` accesses are runtime via
      the alias. Not in the Linux build; verified by inspection.
- [x] **WinCE** — [`src/wince/`](src/wince/). Audited 2026-09-19: the
      flagged `port/main.c` fix is done (§6.2); the remaining port-local
      globals (`smooth_filter`, `virtual_joystick`, ..., declared `extern`
      by `ui.c` under `_WIN32_WCE`) are port-private. Not in the Linux
      build; verified by inspection.
- [x] **GLES2** — [`src/gles2/`](src/gles2/). Audited 2026-09-19: the
      port-local `op_filtering`/`op_zoom` globals are port-private (same
      names as the RPi port's). Not in the Linux build; verified by
      inspection.
- [x] **Java NVM** — [`src/javanvm/`](src/javanvm/). Audited 2026-09-19:
      no per-instance emulator state referenced outside the header
      aliases. Not in the Linux build; verified by inspection.

> **Note:** The standalone ports can initially keep using a single default
> instance while the multi-instance API is exposed through `libatari800`.
> Explicit `_Ctx` conversion of the port files is deferred until a port
> actually needs to drive a non-default instance.

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