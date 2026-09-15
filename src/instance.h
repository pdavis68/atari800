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

#include <stdio.h> /* FILE (Binload_state_t) */

#include <stdio.h> /* FILENAME_MAX, FILE */

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

/* ------------------------------------------------------------------ */
/* SIO state                                                          */
/* ------------------------------------------------------------------ */

#define SIO_MAX_DRIVES 8

typedef enum SIO_tagUnitStatus {
	SIO_OFF,
	SIO_NO_DISK,
	SIO_READ_ONLY,
	SIO_READ_WRITE
} SIO_UnitStatus;

typedef struct SIO_state_t {
	/* Public state (legacy globals). */
	char status[256];
	SIO_UnitStatus drive_status[SIO_MAX_DRIVES];
	char filename[SIO_MAX_DRIVES][FILENAME_MAX];
	int last_op;
	int last_op_time;
	int last_drive; /* 1 .. 8 */
	int last_sector;
	int format_sectorcount[SIO_MAX_DRIVES];
	int format_sectorsize[SIO_MAX_DRIVES];
	/* Internal per-drive state. */
	int boot_sectors_type[SIO_MAX_DRIVES];
	int image_type[SIO_MAX_DRIVES];
	FILE *disk[SIO_MAX_DRIVES];
	int sectorcount[SIO_MAX_DRIVES];
	int sectorsize[SIO_MAX_DRIVES];
	int io_success[SIO_MAX_DRIVES];
	void *additional_info[SIO_MAX_DRIVES];
	/* Serial frame state. */
	UBYTE CommandFrame[6];
	int CommandIndex;
	UBYTE DataBuffer[65535 + 3]; /* large buffer for FujiNet */
	int DataIndex;
	int TransferStatus;
	int ExpectedBytes;
	int delay_counter;
	int last_ypos;
} SIO_state_t;

/* ------------------------------------------------------------------ */
/* Devices (H:/P:/R:/B: patches) state                                */
/* ------------------------------------------------------------------ */

struct DEV_B {
	char url[512];
	int  pos;
	int  ready;
};

typedef struct Devices_state_t {
	/* Public state (legacy globals). */
	int enable_h_patch;
	int enable_p_patch;
	int enable_r_patch;
	int enable_b_patch;
	char atari_h_dir[4][FILENAME_MAX];
	int h_read_only;
	char h_exe_path[FILENAME_MAX];
	char h_device_name;
	char h_current_dir[4][FILENAME_MAX];
	char print_command[256];
	struct DEV_B dev_b_status;
	/* Internal H: device state. */
	FILE *h_fp[8];
	int h_textmode[8];
	int h_lastbyte[8];
	int h_wascr[8];
	char h_lastop[8];
	int h_iocb;
	int h_devnum;
	char atari_filename[FILENAME_MAX];
	char new_filename[FILENAME_MAX];
	char atari_path[FILENAME_MAX];
	char host_path[FILENAME_MAX];
} Devices_state_t;

/* ------------------------------------------------------------------ */
/* Cartridge state                                                    */
/* ------------------------------------------------------------------ */

/* Moved here from cartridge.h so Cartridge_state_t can embed it by value. */
typedef struct CARTRIDGE_image_t {
	int type;
	int state; /* Cartridge's state, such as selected bank or switch on/off. */
	int size; /* Size of the image, in kilobytes. */
	UBYTE *image;
	char filename[FILENAME_MAX];
	int raw; /* File contains RAW data (important for writeable cartridges). */
} CARTRIDGE_image_t;

typedef struct Cartridge_state_t {
	CARTRIDGE_image_t main;      /* Left/Right cartridge */
	CARTRIDGE_image_t piggyback; /* Pass through cartridge for SpartaDOSX */
	int autoreboot;
	/* Internal: currently active cartridge image (main or piggyback). */
	CARTRIDGE_image_t *active_cart;
} Cartridge_state_t;

/* ------------------------------------------------------------------ */
/* Cassette state                                                     */
/* ------------------------------------------------------------------ */

/* Moved here from cassette.h so Cassette_state_t can embed it by value. */
typedef enum {
	CASSETTE_STATUS_NONE,
	CASSETTE_STATUS_READ_ONLY,
	CASSETTE_STATUS_READ_WRITE
} CASSETTE_status_t;

#ifndef IMG_TAPE_T_DEFINED
#define IMG_TAPE_T_DEFINED
typedef struct IMG_TAPE_t IMG_TAPE_t;
#endif

typedef struct Cassette_state_t {
	/* Public state (legacy globals). */
	char filename[FILENAME_MAX];
	char description[256]; /* CASSETTE_DESCRIPTION_MAX */
	CASSETTE_status_t status;
	int hold_start;
	int hold_start_on_reboot; /* preserve hold_start after reboot */
	int press_space;
	int write_protect;
	int record;
	int readable;
	int writable;
	/* Internal tape state. */
	IMG_TAPE_t *cassette_file;
	SLONG event_time_left;
	int pending_serin;
	int passing_gap;
	UBYTE pending_serin_byte;
	UBYTE serin_byte;
	int cassette_gapdelay; /* in ms, includes leader and all gaps */
	int cassette_motor;
	int eof_of_tape;
} Cassette_state_t;

typedef struct PBI_state_t {
	/* D1FF register latch: one bit per PBI device signals IRQ */
	UBYTE D1FF_LATCH;
	/* floating-point ROM reactivation state in PBI_D1PutByte */
	int fp_active;
	/* 1400XL/1450XLD and 1090 have ram here */
	int D6D7ram;
	/* So far as is currently implemented: PBI_IRQ can be generated by the
	 * 1400/1450 Votrax and the Black Box button. Each emulated PBI device
	 * sets a bit in this variable to indicate IRQ status. The actual
	 * hardware has only one common line; the device driver ROM has to
	 * figure it out. */
	int IRQ;
} PBI_state_t;

/* A function called to handle an escape sequence. The registered handlers
   are context-free thunks for now (they pin the default instance); they
   will take an instance once every handler module is converted. */
typedef void (*ESC_FunctionType)(void);

typedef struct ESC_state_t {
	/* Address of the patched instruction for each escape code, used to
	   verify the patch is still in place when the escape runs. */
	UWORD esc_address[256];
	/* Handler for each escape code (NULL = not installed). */
	ESC_FunctionType esc_function[256];
	/* TRUE to enable patched (fast) Serial I/O. */
	int enable_sio_patch;
} ESC_state_t;

typedef struct Binload_state_t {
	FILE *bin_file;
	int start_binloading;
	int loading_basic;
	int slow_xex_loading;
	/* Indicates that a DOS file is being currently slowly loaded. */
	int wait_active;
	/* Set to TRUE to pause the current loading of a DOS file. */
	int pause_loading;
	/* Internal slow-XEX-loading state. */
	unsigned int instr_elapsed; /* CPU instructions elapsed since last loaded byte */
	UWORD from; /* start address of the currently loaded segment */
	UWORD to;   /* end address of the currently loaded segment */
	int init2e3; /* next call to loader_cont will overwrite INITAD */
	int segfinished; /* currently not during loading of a segment */
} Binload_state_t;

typedef struct SCSI_state_t {
	/* SCSI bus signal lines (TRUE = asserted). */
	int CD;
	int MSG;
	int IO;
	int BSY;
	int REQ;
	int ACK;
	int SEL;
	/* Disk image backing the SCSI bus (opened by Black Box / MIO). */
	FILE *disk;
	/* Internal transfer state. */
	UBYTE byte;
	int phase;
	int bufpos;
	UBYTE buffer[256];
	int count;
} SCSI_state_t;

typedef struct BB_state_t {
	/* TRUE to emulate the CSS Black Box. */
	int enabled;
	/* ROM image. */
	UBYTE *rom;
	int rom_size;
	int rom_high_bit;
	UBYTE rom_bank;
	char rom_filename[FILENAME_MAX];
	/* 64 KB Black Box RAM. */
	UBYTE *ram;
	int ram_bank_offset;
	/* VIA Peripheral control register. */
	UBYTE PCR;
	/* SCSI disk state. */
	int scsi_enabled;
	char scsi_disk_filename[FILENAME_MAX];
	/* Menu-button IRQ state. */
	int buttondown;
	int frame_count;
} BB_state_t;

typedef struct MIO_state_t {
	/* TRUE to emulate the ICD MIO board. */
	int enabled;
	/* ROM image. */
	UBYTE *rom;
	int rom_size;
	UBYTE rom_bank;
	char rom_filename[FILENAME_MAX];
	/* MIO RAM. */
	UBYTE *ram;
	int ram_size;
	int ram_bank_offset;
	int ram_enabled;
	/* SCSI disk state. */
	int scsi_enabled;
	char scsi_disk_filename[FILENAME_MAX];
} MIO_state_t;

typedef struct PROTO80_state_t {
	/* TRUE to emulate a prototype 80 column board for the 1090. */
	int enabled;
	/* Proto80 ROM image (0x800 bytes, heap-allocated when enabled). */
	UBYTE *rom;
	char rom_filename[FILENAME_MAX];
} PROTO80_state_t;

typedef struct RTIME_state_t {
	/* TRUE to emulate the ICD R-Time 8 cartridge. */
	int enabled;
	/* 0 = waiting for register #, 1 = got register #, waiting for hi
	   nybble, 2 = got hi nybble, waiting for lo nybble. */
	int state;
	int tmp;
	int tmp2;
	UBYTE regset[16];
} RTIME_state_t;

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

	/* Peripherals (migrated in later phases) */
	SIO_state_t sio;
	Devices_state_t devices;
	Cartridge_state_t cartridge;
	Cassette_state_t cassette;
	PBI_state_t pbi;
	SCSI_state_t scsi;
	BB_state_t bb;
	MIO_state_t mio;
	ESC_state_t esc;
	Binload_state_t binload;
	RTIME_state_t rtime;
	PROTO80_state_t proto80;
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