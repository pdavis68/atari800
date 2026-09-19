#ifndef ATARI_H_
#define ATARI_H_

#include "config.h"
#include <stdio.h> /* FILENAME_MAX */
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

/* Fundamental declarations ---------------------------------------------- */

#define Atari800_TITLE  "Atari 800 Emulator, Version " PACKAGE_VERSION

/* URL for automatic ROM download. */
#define ROM_URL "http://www.emulators.com/freefile/pcxf380.zip"

#ifndef FALSE
#define FALSE  0
#endif
#ifndef TRUE
#define TRUE   1
#endif

/* SBYTE and UBYTE must be exactly 1 byte long. */
/* SWORD and UWORD must be exactly 2 bytes long. */
/* SLONG and ULONG must be exactly 4 bytes long. */
#define SBYTE signed char
#define SWORD signed short
#define SLONG signed int
#define UBYTE unsigned char
#define UWORD unsigned short
#ifndef HAVE_WINDOWS_H
/* Windows headers typedef ULONG */
#define ULONG unsigned int
#endif
/* Note: in various parts of the emulator we assume that char is 1 byte
   and int is 4 bytes. */


/* Public interface ------------------------------------------------------ */

/* Machine type. */
enum {
	Atari800_MACHINE_800,
	Atari800_MACHINE_XLXE,
	Atari800_MACHINE_5200,
	/* Number of values in the emumerator */
	Atari800_MACHINE_SIZE
};

/* NOTE: instance.h includes this header, so this header cannot include
   instance.h (include cycle). The per-instance top-level state lives in
   Atari800_Instance (instance.h); the legacy global names below are aliased
   to the default instance. TUs that use these aliases need the complete
   struct type -- include "instance.h" (directly or transitively). */
struct Atari800_Instance;
extern struct Atari800_Instance *Atari800_default;

/* Don't change this variable directly; use Atari800_SetMachineType() instead. */
#define Atari800_machine_type (Atari800_default->machine_type)
void Atari800_SetMachineType_Ctx(struct Atari800_Instance *inst, int type);
#define Atari800_SetMachineType(type) \
	Atari800_SetMachineType_Ctx(Atari800_default, (type))

/* Always call Atari800_InitialiseMachine() after changing Atari800_machine_type
   or MEMORY_ram_size! */

/* Indicates if machine has BASIC built in. */
#define Atari800_builtin_basic (Atari800_default->builtin_basic)

/* Indicates existence of 1200XL's two keyboard LEDs.
   Used only for Atari800_MACHINE_XLXE. */
#define Atari800_keyboard_leds (Atari800_default->keyboard_leds)

/* Indicates existence of F1-F4 keys.
   Used only for Atari800_MACHINE_XLXE. */
#define Atari800_f_keys (Atari800_default->f_keys)

/* State of the J1 jumper on the 1200XL board.
   Used only for Atari800_MACHINE_XLXE. Always call
   Atari800_UpdateJumper() after changing this variable. */
#define Atari800_jumper (Atari800_default->jumper)
void Atari800_UpdateJumper_Ctx(struct Atari800_Instance *inst);
#define Atari800_UpdateJumper() \
	Atari800_UpdateJumper_Ctx(Atari800_default)

/* Indicates existence of XEGS' built-in game.
   Used only for Atari800_MACHINE_XLXE. */
#define Atari800_builtin_game (Atari800_default->builtin_game)

/* TRUE if the XEGS keyboard is detached.
   Used only for Atari800_MACHINE_XLXE. Always call
   Atari800_UpdateKeyboardDetached() after changing this variable. */
#define Atari800_keyboard_detached (Atari800_default->keyboard_detached)
void Atari800_UpdateKeyboardDetached_Ctx(struct Atari800_Instance *inst);
#define Atari800_UpdateKeyboardDetached() \
	Atari800_UpdateKeyboardDetached_Ctx(Atari800_default)

/* Video system. */
#define Atari800_TV_UNSET 0
#define Atari800_TV_PAL 312
#define Atari800_TV_NTSC 262
#define Atari800_FPS_PAL 49.8607597
/*49.8607597 = (4.43361875*(4/5)*1000000)/(312*228)*/
#define Atari800_FPS_NTSC 59.9227434
/*59.9227434 = (3.579545*1000000)/(262*228)*/

/* Video system / Number of scanlines per frame. Do not set this variable
   directly; instead use Atari800_SetTVMode(). */
#define Atari800_tv_mode (Atari800_default->tv_mode)

/* TRUE to disable Atari BASIC when booting Atari (hold Option in XL/XE). */
#define Atari800_disable_basic (Atari800_default->disable_basic)

/* OS ROM version currently used by the emulator. Can be -1 for missing ROM, or
   a value from the SYSROM enumerator. */
#define Atari800_os_version (Atari800_default->os_version)

/* If Atari800_Frame() sets it to TRUE, then the current contents
   of Screen_atari should be displayed. */
#define Atari800_display_screen (Atari800_default->display_screen)

/* Simply incremented by Atari800_Frame(). */
#define Atari800_nframes (Atari800_default->nframes)

/* How often the screen is updated (1 = every Atari frame). */
#define Atari800_refresh_rate (Atari800_default->refresh_rate)

/* If TRUE, will try to maintain the emulation speed to 100% */
#define Atari800_auto_frameskip (Atari800_default->auto_frameskip)

/* Set to TRUE for faster emulation with Atari800_refresh_rate > 1.
   Set to FALSE for accurate emulation with Atari800_refresh_rate > 1. */
#define Atari800_collisions_in_skipped_frames \
	(Atari800_default->collisions_in_skipped_frames)

/* Set to TRUE to run emulated Atari as fast as possible */
#define Atari800_turbo (Atari800_default->turbo)
/* Percentage speed or 0 for max turbo */
#define Atari800_turbo_speed (Atari800_default->turbo_speed)

/* Set to TRUE to start in the monitor. It's up to each port's
	main.c to implement this (initially only SDL supports it). */
#define Atari800_start_in_monitor (Atari800_default->start_in_monitor)

/* Initializes Atari800 emulation core. */
int Atari800_Initialise(int *argc, char *argv[]);

/* Emulates one frame (1/50sec for PAL, 1/60sec for NTSC). */
void Atari800_Frame_Ctx(struct Atari800_Instance *inst);
#define Atari800_Frame() Atari800_Frame_Ctx(Atari800_default)

/* Reboots the emulated Atari. */
void Atari800_Coldstart_Ctx(struct Atari800_Instance *inst);
#define Atari800_Coldstart() Atari800_Coldstart_Ctx(Atari800_default)

/* Presses the Reset key in the emulated Atari. */
void Atari800_Warmstart_Ctx(struct Atari800_Instance *inst);
#define Atari800_Warmstart() Atari800_Warmstart_Ctx(Atari800_default)

/* Reinitializes after Atari800_machine_type or ram_size change.
   You should call Atari800_Coldstart() after it. */
int Atari800_InitialiseMachine_Ctx(struct Atari800_Instance *inst);
#define Atari800_InitialiseMachine() \
	Atari800_InitialiseMachine_Ctx(Atari800_default)

/* Fully initialise a new instance from the current configuration template
   (machine type, TV mode, RAM size, ROM selection, machine init +
   coldstart). Implemented in atari.c; used by the grid frontend. */
int Atari800_InitInstanceFromTemplate(struct Atari800_Instance *inst);

/* Shuts down Atari800 emulation core and saves the config file if needed.
 * Use it when a user requested exiting/entering a monitor. */
int Atari800_Exit(int run_monitor);

/* Shuts down Atari800 emulation core. Use it for emergency-exiting
   such as on failure. */
void Atari800_ErrExit(void);


/* Private interface ----------------------------------------------------- */
/* Don't use outside the emulation core! */

/* STAT_UNALIGNED_WORDS is solely for benchmarking purposes.
   8-element arrays (stat_arr) represent number of accesses with the given
   value of 3 least significant bits of address. This gives us information
   about the ratio of aligned vs unaligned accesses. */
#ifdef STAT_UNALIGNED_WORDS
#define UNALIGNED_STAT_DEF(stat_arr)             unsigned int stat_arr[8];
#define UNALIGNED_STAT_DECL(stat_arr)            extern unsigned int stat_arr[8];
#define UNALIGNED_GET_WORD(ptr, stat_arr)        (stat_arr[(unsigned int) (ptr) & 7]++, *(const UWORD *) (ptr))
#define UNALIGNED_PUT_WORD(ptr, value, stat_arr) (stat_arr[(unsigned int) (ptr) & 7]++, *(UWORD *) (ptr) = (value))
#define UNALIGNED_GET_LONG(ptr, stat_arr)        (stat_arr[(unsigned int) (ptr) & 7]++, *(const ULONG *) (ptr))
#define UNALIGNED_PUT_LONG(ptr, value, stat_arr) (stat_arr[(unsigned int) (ptr) & 7]++, *(ULONG *) (ptr) = (value))
UNALIGNED_STAT_DECL(Screen_atari_write_long_stat)
UNALIGNED_STAT_DECL(pm_scanline_read_long_stat)
UNALIGNED_STAT_DECL(memory_read_word_stat)
UNALIGNED_STAT_DECL(memory_write_word_stat)
UNALIGNED_STAT_DECL(memory_read_aligned_word_stat)
UNALIGNED_STAT_DECL(memory_write_aligned_word_stat)
#else
#define UNALIGNED_STAT_DEF(stat_arr)
#define UNALIGNED_STAT_DECL(stat_arr)
#define UNALIGNED_GET_WORD(ptr, stat_arr)        (*(const UWORD *) (ptr))
#define UNALIGNED_PUT_WORD(ptr, value, stat_arr) (*(UWORD *) (ptr) = (value))
#define UNALIGNED_GET_LONG(ptr, stat_arr)        (*(const ULONG *) (ptr))
#define UNALIGNED_PUT_LONG(ptr, value, stat_arr) (*(ULONG *) (ptr) = (value))
#endif

/* Sleeps until it's time to emulate next Atari frame. */
void Atari800_Sync_Ctx(struct Atari800_Instance *inst);
#define Atari800_Sync() Atari800_Sync_Ctx(Atari800_default)

/* Load a ROM image filename of size nbytes into buffer */
int Atari800_LoadImage(const char *filename, UBYTE *buffer, int nbytes);

/* Save State */
void Atari800_StateSave_Ctx(struct Atari800_Instance *inst);
#define Atari800_StateSave() Atari800_StateSave_Ctx(Atari800_default)

/* Read State */
void Atari800_StateRead_Ctx(struct Atari800_Instance *inst, UBYTE version);
#define Atari800_StateRead(version) \
	Atari800_StateRead_Ctx(Atari800_default, (version))

/* Change TV mode. */
void Atari800_SetTVMode_Ctx(struct Atari800_Instance *inst, int mode);
#define Atari800_SetTVMode(mode) Atari800_SetTVMode_Ctx(Atari800_default, (mode))

#endif /* ATARI_H_ */
