/*
 * instance.h - per-instance state for multi-instance Atari800 emulation
 *
 * This header defines the per-instance state that must be threaded through
 * the emulation core to support running multiple Atari instances side-by-side
 * (the "Option C" context-object refactor, see docs/multi-instance-refactor.md).
 *
 * Each module's global/static state is being migrated into a per-module
 * sub-struct, and the top-level Atari800_Instance aggregates them. Functions
 * take an Atari800_Instance * (or a per-module sub-context) as their first
 * parameter.
 *
 * This file is a work in progress: the core sub-structs (CPU, memory, ANTIC,
 * GTIA, POKEY, PIA) are defined concretely; the peripheral sub-structs are
 * forward-declared and referenced by pointer until their modules are migrated
 * in later phases.
 */

#ifndef INSTANCE_H_
#define INSTANCE_H_

#include "atari.h" /* UBYTE, UWORD, ULONG, TRUE/FALSE */
#include "screen.h" /* Screen_WIDTH */

/* ------------------------------------------------------------------ */
/* CPU state                                                          */
/* ------------------------------------------------------------------ */

#define CPU_REMEMBER_PC_STEPS 64
#define CPU_REMEMBER_JMP_STEPS 16

typedef struct CPU_state_t {
	UWORD regPC;
	UBYTE regA;
	UBYTE regP;
	UBYTE regS;
	UBYTE regY;
	UBYTE regX;
	UBYTE IRQ;
	UBYTE cim_encountered;
	UBYTE delayed_nmi;
	void (*rts_handler)(void);
#ifdef MONITOR_BREAK
	UWORD remember_PC[CPU_REMEMBER_PC_STEPS];
	UBYTE remember_op[CPU_REMEMBER_PC_STEPS][3];
	unsigned int remember_PC_curpos;
	int remember_xpos[CPU_REMEMBER_PC_STEPS];
	UWORD remember_JMP[CPU_REMEMBER_JMP_STEPS];
	unsigned int remember_jmp_curpos;
#endif
#ifdef MONITOR_PROFILE
	int instruction_count[256];
#endif
} CPU_state_t;

/* ------------------------------------------------------------------ */
/* Memory state                                                       */
/* ------------------------------------------------------------------ */

#define MEMORY_RAM       0
#define MEMORY_ROM       1
#define MEMORY_HARDWARE  2

#ifndef PAGED_ATTRIB
typedef UBYTE (*MEMORY_rdfunc)(UWORD addr, int no_side_effects);
typedef void (*MEMORY_wrfunc)(UWORD addr, UBYTE value);
#endif

typedef struct MEMORY_state_t {
	UBYTE mem[65536 + 2];
#ifndef PAGED_ATTRIB
	UBYTE attrib[65536];
#else
	MEMORY_rdfunc readmap[256];
	MEMORY_rdfunc safe_readmap[256];
	MEMORY_wrfunc writemap[256];
#endif
	int ram_size;
	int xe_bank;
	int selftest_enabled;
	int have_basic;
	int cartA0BF_enabled;
	int mosaic_num_banks;
	int axlon_0f_mirror;
	int axlon_num_banks;
	int enable_mapram;
	/* ROM image sources (shared/read-only, but kept here for convenience). */
	UBYTE os[16384];
	UBYTE basic[8192];
	UBYTE xegame[8192];
	/* Internal bank/expansion state (allocated buffers referenced by pointer). */
	UBYTE *atarixe_memory;
	ULONG atarixe_memory_size;
	UBYTE *axlon_ram;
	UBYTE *mosaic_ram;
	UBYTE *mapram_memory;
	int cart809F_enabled;
	UBYTE under_atarixl_os[16384];
	UBYTE under_cart809F[8192];
	UBYTE under_cartA0BF[8192];
	/* Axlon/Mosaic bank-switching state. */
	int axlon_current_bankmask;
	int axlon_curbank;
	int mosaic_current_num_banks;
	int mosaic_curbank;
	/* RAM shadowed by Self-Test in the XE bank seen by ANTIC, when
	   ANTIC/CPU separate XE access is active. */
	UBYTE antic_bank_under_selftest[0x800];
} MEMORY_state_t;

/* ------------------------------------------------------------------ */
/* ANTIC state                                                        */
/* ------------------------------------------------------------------ */

typedef struct ANTIC_state_t {
	UBYTE CHACTL;
	UBYTE CHBASE;
	UWORD dlist;
	UBYTE DMACTL;
	UBYTE HSCROL;
	UBYTE NMIEN;
	UBYTE NMIST;
	UBYTE PMBASE;
	UBYTE VSCROL;
	int break_ypos;
	int ypos;
	int wsync_halt;
	int xpos;
	int xpos_limit;
	unsigned int screenline_cpu_clock;
	int artif_mode;
	int artif_new;
	UBYTE PENH_input;
	UBYTE PENV_input;
	const UBYTE *xe_ptr;
	int player_dma_enabled;
	int missile_dma_enabled;
	int player_gra_enabled;
	int missile_gra_enabled;
	int player_flickering;
	int missile_flickering;
#ifdef NEW_CYCLE_EXACT
	int delayed_wsync;
	int cur_screen_pos;
#endif
#ifndef NO_SIMPLE_PAL_BLENDING
	int pal_blending;
#endif
} ANTIC_state_t;

/* ------------------------------------------------------------------ */
/* GTIA state                                                         */
/* ------------------------------------------------------------------ */

typedef struct GTIA_state_t {
	UBYTE GRAFM;
	UBYTE GRAFP0, GRAFP1, GRAFP2, GRAFP3;
	UBYTE HPOSP0, HPOSP1, HPOSP2, HPOSP3;
	UBYTE HPOSM0, HPOSM1, HPOSM2, HPOSM3;
	UBYTE SIZEP0, SIZEP1, SIZEP2, SIZEP3;
	UBYTE SIZEM;
	UBYTE COLPM0, COLPM1, COLPM2, COLPM3;
	UBYTE COLPF0, COLPF1, COLPF2, COLPF3;
	UBYTE COLBK;
	UBYTE GRACTL;
	UBYTE M0PL, M1PL, M2PL, M3PL;
	UBYTE P0PL, P1PL, P2PL, P3PL;
	UBYTE PRIOR;
	UBYTE VDELAY;
	UBYTE pm_scanline[Screen_WIDTH / 2 + 8];
	int pm_dirty;
	UBYTE collisions_mask_missile_playfield;
	UBYTE collisions_mask_player_playfield;
	UBYTE collisions_mask_missile_player;
	UBYTE collisions_mask_player_player;
	UBYTE TRIG[4];
	UBYTE TRIG_latch[4];
	int consol_override;
	int speaker;
} GTIA_state_t;

/* ------------------------------------------------------------------ */
/* POKEY state                                                        */
/* ------------------------------------------------------------------ */

#define POKEY_MAXPOKEYS 2

typedef struct POKEY_state_t {
	UBYTE KBCODE;
	UBYTE IRQST;
	UBYTE IRQEN;
	UBYTE SKSTAT;
	UBYTE SKCTL;
	int DELAYED_SERIN_IRQ;
	int DELAYED_SEROUT_IRQ;
	int DELAYED_XMTDONE_IRQ;
#ifdef NEW_CYCLE_EXACT
	int irq_at_xpos;
	UBYTE irq_pending_mask;
#endif
	UBYTE POT_input[8];
	UBYTE AUDF[4 * POKEY_MAXPOKEYS];
	UBYTE AUDC[4 * POKEY_MAXPOKEYS];
	UBYTE AUDCTL[POKEY_MAXPOKEYS];
	int DivNIRQ[4];
	int DivNMax[4];
	int Base_mult[POKEY_MAXPOKEYS];
} POKEY_state_t;

/* ------------------------------------------------------------------ */
/* PIA state                                                          */
/* ------------------------------------------------------------------ */

typedef struct PIA_state_t {
	UBYTE PACTL;
	UBYTE PBCTL;
	UBYTE PORTA;
	UBYTE PORTB;
	UBYTE PORTA_mask;
	UBYTE PORTB_mask;
	UBYTE PORT_input[2];
	int CA1;
	int CB1;
	int CA2;
	int CB2;
	int IRQ;
} PIA_state_t;

/* ------------------------------------------------------------------ */
/* Peripheral sub-structs (forward-declared; migrated in later phases) */
/* ------------------------------------------------------------------ */

typedef struct SIO_state_t SIO_state_t;
typedef struct Devices_state_t Devices_state_t;
typedef struct Cartridge_state_t Cartridge_state_t;
typedef struct Cassette_state_t Cassette_state_t;
typedef struct PBI_state_t PBI_state_t;
typedef struct Input_state_t Input_state_t;
typedef struct Screen_state_t Screen_state_t;
typedef struct Sound_state_t Sound_state_t;

/* ------------------------------------------------------------------ */
/* Top-level instance                                                 */
/* ------------------------------------------------------------------ */

typedef struct Atari800_Instance {
	/* Core chips */
	CPU_state_t cpu;
	MEMORY_state_t memory;
	ANTIC_state_t antic;
	GTIA_state_t gtia;
	POKEY_state_t pokey;
	PIA_state_t pia;

	/* Peripherals (allocated separately; migrated in later phases) */
	SIO_state_t *sio;
	Devices_state_t *devices;
	Cartridge_state_t *cartridge;
	Cassette_state_t *cassette;
	PBI_state_t *pbi;
	Input_state_t *input;
	Screen_state_t *screen;
	Sound_state_t *sound;

	/* Top-level configuration */
	int machine_type;
	int builtin_basic;
	int keyboard_leds;
	int f_keys;
	int jumper;
	int builtin_game;
	int keyboard_detached;
	int tv_mode;
	int disable_basic;
	int os_version;
	int display_screen;
	int nframes;
	int refresh_rate;
	int collisions_in_skipped_frames;
	int turbo;
	int turbo_speed;
	int start_in_monitor;
	int auto_frameskip;
} Atari800_Instance;

/* Lifecycle API (implemented in atari.c / a new instance.c) */
Atari800_Instance *Atari800_NewInstance(void);
void Atari800_FreeInstance(Atari800_Instance *inst);
void Atari800_FrameInstance(Atari800_Instance *inst);
void Atari800_ColdstartInstance(Atari800_Instance *inst);
void Atari800_WarmstartInstance(Atari800_Instance *inst);

/* Transitional bridge: a single default instance used by modules that have
   not yet been migrated to the instance context. The legacy memory globals
   (MEMORY_mem, MEMORY_attrib, ...) are aliased to this instance's memory so
   the tree stays buildable during the incremental migration. */
extern Atari800_Instance *Atari800_default;

#endif /* INSTANCE_H_ */