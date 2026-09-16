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

/* XEP80 geometry (moved here from xep80.h so XEP80_state_t can embed the
   display buffers by value). */
#define XEP80_WIDTH 256
#define XEP80_HEIGHT 25
#define XEP80_CHAR_WIDTH 7
#define XEP80_MAX_CHAR_HEIGHT 12
#define XEP80_GRAPH_WIDTH 320
#define XEP80_GRAPH_HEIGHT 200
#define XEP80_LINE_LEN 80
#define XEP80_SCRN_WIDTH (XEP80_LINE_LEN * XEP80_CHAR_WIDTH)
#define XEP80_MAX_SCRN_HEIGHT (XEP80_HEIGHT * XEP80_MAX_CHAR_HEIGHT)

typedef struct XEP80_state_t {
	/* Is XEP80 enabled? Don't change directly, use XEP80_SetEnabled(). */
	int enabled;
	int port;
	/* Current height of the XEP80 screen / of XEP80 characters. */
	int scrn_height;
	int char_height;
	/* Display buffers. */
	UBYTE screen_1[XEP80_SCRN_WIDTH*XEP80_MAX_SCRN_HEIGHT];
	UBYTE screen_2[XEP80_SCRN_WIDTH*XEP80_MAX_SCRN_HEIGHT];
	/* Path to the XEP80's charset ROM image. */
	char charset_filename[FILENAME_MAX];
	/* Serial-protocol state. */
	int output_word;
	UWORD input_queue[10]; /* IN_QUEUE_SIZE */
	int input_count;
	unsigned int start_trans_cpu_clock;
	int receiving;
	/* Internal NS405 RAM registers / rendering state. */
	int ypos;
	int xpos;
	UBYTE last_char;
	int lmargin;
	int rmargin;
	int xscroll;
	UBYTE *line_pointers[XEP80_HEIGHT];
	int old_ypos;
	int old_xpos;
	int list_mode;
	int escape_mode;
	int burst_mode;
	int screen_output;
	/* Attribute Latch 0. */
	UBYTE attrib_a;
	int font_a_index;
	int font_a_double;
	int font_a_blank;
	int font_a_blink;
	/* Attribute Latch 1. */
	UBYTE attrib_b;
	int font_b_index;
	int font_b_double;
	int font_b_blank;
	int font_b_blink;
	/* TCP. */
	int cursor_on;
	int graphics_mode;
	int pal_mode;
	/* VCR. */
	int blink_reverse;
	int cursor_blink;
	int cursor_overwrite;
	int inverse_mode;
	int char_set;
	/* CURS. */
	int cursor_x;
	int cursor_y;
	int curs;
	/* 8 KB of video RAM. */
	UBYTE video_ram[0x2000];
} XEP80_state_t;

typedef struct XLD_state_t {
	/* TRUE to emulate the 1450XLD / 1400XL (voice + optional parallel disk). */
	int enabled;
	int v_enabled;   /* voice box */
	int d_enabled;   /* parallel disk */
	/* ROM images (heap-allocated when enabled). */
	UBYTE *voicerom;
	UBYTE *diskrom;
	char d_rom_filename[FILENAME_MAX];
	char v_rom_filename[FILENAME_MAX];
	/* Voice/modem latches. */
	UBYTE votrax_latch;
	UBYTE modem_latch;
	/* Parallel Disk I/O (PIO) transfer state. */
	UBYTE CommandFrame[6];
	int CommandIndex;
	UBYTE DataBuffer[256 + 3];
	int DataIndex;
	int TransferStatus;
	int ExpectedBytes;
} XLD_state_t;

typedef struct AF80_state_t {
	/* TRUE to emulate the Austin Franklin 80 column board. */
	int enabled;
	/* ROM/charset images (0x1000 bytes each, heap-allocated when enabled). */
	UBYTE *rom;
	char rom_filename[FILENAME_MAX];
	UBYTE *charset;
	char charset_filename[FILENAME_MAX];
	/* 2 KB video RAM + attribute RAM (heap-allocated when enabled). */
	UBYTE *screen;
	UBYTE *attrib;
	/* Register/bank state. */
	int rom_bank_select;          /* bits 0-3 of d5f7, $0-$f 16 banks */
	int not_rom_output_enable;    /* bit 4 of d5f7 0 = Enable ROM 1 = Disable ROM */
	int not_right_cartridge_rd4_control; /* 0=$8000-$9fff cart ROM, 1=$8000-$9fff system RAM */
	int not_enable_2k_character_ram;
	int not_enable_2k_attribute_ram;
	int not_enable_crtc_registers;
	int not_enable_80_column_output;
	int video_bank_select;        /* bits 0-3 of d5f6, $0-$f 16 banks */
	int crtreg[0x40];
} AF80_state_t;

typedef struct PROTO80_state_t {
	/* TRUE to emulate a prototype 80 column board for the 1090. */
	int enabled;
	/* Proto80 ROM image (0x800 bytes, heap-allocated when enabled). */
	UBYTE *rom;
	char rom_filename[FILENAME_MAX];
} PROTO80_state_t;

typedef struct BIT3_state_t {
	/* TRUE to emulate the Bit3 Full View 80 column board. */
	int enabled;
	/* ROM/charset images (0x1000 bytes each, heap-allocated when enabled). */
	UBYTE *rom;
	char rom_filename[FILENAME_MAX];
	UBYTE *charset;
	char charset_filename[FILENAME_MAX];
	/* 2 KB video RAM (heap-allocated when enabled). */
	UBYTE *screen;
	/* Register/bank state. */
	int video_latch;
	int rom_bank_select;          /* bits 5 and 0-2 of d508, $0-$f 16 banks */
	UBYTE crtreg[0x40];
} BIT3_state_t;

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

typedef struct Voicebox_state_t {
	/* TRUE to emulate the Alien Group Voice Box I / II. */
	int enabled;
	/* TRUE for Voice Box II (serial), FALSE for Voice Box I (SKCTL). */
	int ii;
	/* Serial-protocol decode state (Voice Box I). */
	int prev_byte;
	int prev_prev_byte;
	int voice_box_byte;
	int voice_box_bit;
} Voicebox_state_t;

typedef struct Pokeyrec_state_t {
	int enabled;
	int counter;
	int interval;
	char *filename; /* points to a literal or a Util_strdup'ed string */
	char *fmt;      /* points to a string literal */
	FILE *fp;
#ifdef STEREO_SOUND
	int stereo;
#endif
} Pokeyrec_state_t;

struct ide_device; /* defined in ide_internal.h (IDE build only) */

typedef struct IDE_state_t {
	int enabled;
	int debug;
	int count; /* debug counter */
	/* Heap-allocated device state (lazily created by IDE_PIN_CTX;
	   the legacy module used a zero-initialised file-scope struct).
	   Named `dev` because ide.c aliases the token `device`. */
	struct ide_device *dev;
} IDE_state_t;

typedef struct Input_state_t {
	/* Keyboard */
	int key_code;   /* regular Atari key code */
	int key_shift;  /* Shift key pressed */
	int key_consol; /* Start, Select and Option keys (INPUT_CONSOL_*) */
	/* Joysticks */
	int joy_autofire[4]; /* autofire mode for each Atari port */
	int joy_block_opposite_directions; /* can't move left and right simultaneously */
	int joy_multijoy;  /* emulate MultiJoy4 interface */
	/* 5200 joystick values */
	int joy_5200_min;
	int joy_5200_center;
	int joy_5200_max;
	/* Mouse */
	int mouse_mode;      /* device emulated with mouse (INPUT_MOUSE_*) */
	int mouse_port;      /* Atari port the emulated device is attached to */
	int mouse_delta_x;   /* x motion since last frame */
	int mouse_delta_y;   /* y motion since last frame */
	int mouse_buttons;   /* buttons pressed (b0: left, b1: right, b2: middle) */
	int mouse_speed;     /* how fast the mouse pointer moves */
	int mouse_pot_min;   /* min. value of POKEY's POT register */
	int mouse_pot_max;   /* max. value of POKEY's POT register */
	int mouse_pen_ofs_h; /* light pen/gun horizontal offset (calibration) */
	int mouse_pen_ofs_v; /* light pen/gun vertical offset (calibration) */
	int mouse_joy_inertia; /* how long the mouse pointer can move (in Atari frames) */
	int direct_mouse;    /* convert mouse pointer position directly into POT values */
	/* CX85 numeric keypad */
	int cx85;
	/* Internal state (previously file-scope statics in input.c) */
	int cx85_port;
	int mouse_x;
	int mouse_y;
	int mouse_move_x;
	int mouse_move_y;
	int mouse_step_e;    /* Bresenham error term used by mouse_step() */
	int mouse_pen_show_pointer;
	int mouse_last_right;
	int mouse_last_down;
	UBYTE STICK[4];
	UBYTE TRIG_input[4];
	int joy_multijoy_no; /* number of selected joy */
	int max_scanline_counter;
	int scanline_counter;
	int last_key_code;
	int last_key_break;
	UBYTE last_stick[4];
	int last_mouse_buttons;
	int bit5_5200;
} Input_state_t;

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
	Voicebox_state_t voicebox;
	Pokeyrec_state_t pokeyrec;
	IDE_state_t ide;
	AF80_state_t af80;
	BIT3_state_t bit3;
	PROTO80_state_t proto80;
	Input_state_t input;
	XLD_state_t xld;
	XEP80_state_t xep80;
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