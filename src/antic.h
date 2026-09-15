#ifndef ANTIC_H_
#define ANTIC_H_

#include "atari.h"
#include "instance.h" /* Atari800_Instance (transitional default-instance aliases) */

/*
 * Offset to registers in custom relative to start of antic memory addresses.
 */

#define ANTIC_OFFSET_DMACTL 0x00
#define ANTIC_OFFSET_CHACTL 0x01
#define ANTIC_OFFSET_DLISTL 0x02
#define ANTIC_OFFSET_DLISTH 0x03
#define ANTIC_OFFSET_HSCROL 0x04
#define ANTIC_OFFSET_VSCROL 0x05
#define ANTIC_OFFSET_PMBASE 0x07
#define ANTIC_OFFSET_CHBASE 0x09
#define ANTIC_OFFSET_WSYNC 0x0a
#define ANTIC_OFFSET_VCOUNT 0x0b
#define ANTIC_OFFSET_PENH 0x0c
#define ANTIC_OFFSET_PENV 0x0d
#define ANTIC_OFFSET_NMIEN 0x0e
#define ANTIC_OFFSET_NMIRES 0x0f
#define ANTIC_OFFSET_NMIST 0x0f

/* Transitional Option C bridge: the per-instance ANTIC state lives in
   ANTIC_state_t (instance.h). Until all callers pass an instance explicitly,
   the legacy global names are aliased to the default instance. */
#define ANTIC_CHACTL   (Atari800_default->antic.CHACTL)
#define ANTIC_CHBASE   (Atari800_default->antic.CHBASE)
#define ANTIC_dlist    (Atari800_default->antic.dlist)
#define ANTIC_DMACTL   (Atari800_default->antic.DMACTL)
#define ANTIC_HSCROL   (Atari800_default->antic.HSCROL)
#define ANTIC_NMIEN    (Atari800_default->antic.NMIEN)
#define ANTIC_NMIST    (Atari800_default->antic.NMIST)
#define ANTIC_PMBASE   (Atari800_default->antic.PMBASE)
#define ANTIC_VSCROL   (Atari800_default->antic.VSCROL)

#define ANTIC_break_ypos (Atari800_default->antic.break_ypos)
#define ANTIC_ypos     (Atari800_default->antic.ypos)
#define ANTIC_wsync_halt (Atari800_default->antic.wsync_halt)

/* Current clock cycle in a scanline.
   Normally 0 <= ANTIC_xpos && ANTIC_xpos < ANTIC_LINE_C, but in some cases ANTIC_xpos >= ANTIC_LINE_C,
   which means that we are already in line (ypos + 1). */
#define ANTIC_xpos     (Atari800_default->antic.xpos)

/* ANTIC_xpos limit for the currently running 6502 emulation. */
#define ANTIC_xpos_limit (Atari800_default->antic.xpos_limit)

/* Main clock value at the beginning of the current scanline. */
#define ANTIC_screenline_cpu_clock (Atari800_default->antic.screenline_cpu_clock)

/* Current main clock value. */
#define ANTIC_CPU_CLOCK (ANTIC_screenline_cpu_clock + ANTIC_XPOS)

#define ANTIC_NMIST_C	6
#define ANTIC_NMI_C	12

/* Number of cycles per scanline. */
#define ANTIC_LINE_C   114

/* STA WSYNC resumes here. */
#define ANTIC_WSYNC_C  106

/* Number of memory refresh cycles per scanline.
   In the first scanline of a font mode there are actually less than ANTIC_DMAR
   memory refresh cycles. */
#define ANTIC_DMAR     9

#define ANTIC_artif_mode (Atari800_default->antic.artif_mode)
#define ANTIC_artif_new  (Atari800_default->antic.artif_new)

#define ANTIC_PENH_input (Atari800_default->antic.PENH_input)
#define ANTIC_PENV_input (Atari800_default->antic.PENV_input)

/* Option C context-aware entry points. */
int ANTIC_Initialise_Ctx(Atari800_Instance *inst, int *argc, char *argv[]);
void ANTIC_Reset_Ctx(Atari800_Instance *inst);
void ANTIC_Frame_Ctx(Atari800_Instance *inst, int draw_display);
UBYTE ANTIC_GetByte_Ctx(Atari800_Instance *inst, UWORD addr, int no_side_effects);
void ANTIC_PutByte_Ctx(Atari800_Instance *inst, UWORD addr, UBYTE byte);

UBYTE ANTIC_GetDLByte_Ctx(Atari800_Instance *inst, UWORD *paddr);
UWORD ANTIC_GetDLWord_Ctx(Atari800_Instance *inst, UWORD *paddr);

/* always call ANTIC_UpdateArtifacting after changing ANTIC_artif_mode */
void ANTIC_UpdateArtifacting_Ctx(Atari800_Instance *inst);

/* Video memory access */
void ANTIC_VideoMemset_Ctx(Atari800_Instance *inst, UBYTE *ptr, UBYTE val, ULONG size);
void ANTIC_VideoPutByte_Ctx(Atari800_Instance *inst, UBYTE *ptr, UBYTE val);

/* GTIA calls it on a write to PRIOR */
void ANTIC_SetPrior_Ctx(Atari800_Instance *inst, UBYTE prior);

/* Saved states */
void ANTIC_StateSave_Ctx(Atari800_Instance *inst);
void ANTIC_StateRead_Ctx(Atari800_Instance *inst);

void ANTIC_UpdateScanline_Ctx(Atari800_Instance *inst);
void ANTIC_UpdateScanlinePrior_Ctx(Atari800_Instance *inst, UBYTE byte);
#define ANTIC_UpdateScanline()          ANTIC_UpdateScanline_Ctx(Atari800_default)
#define ANTIC_UpdateScanlinePrior(byte) ANTIC_UpdateScanlinePrior_Ctx(Atari800_default, byte)

/* Transitional forwarding macros: not-yet-migrated callers use the default
   instance. ANTIC_GetByte/ANTIC_PutByte remain real functions (they are
   registered in the per-instance memory map function-pointer tables, which
   have a fixed context-free signature) and pin the default instance. */
#define ANTIC_Initialise(argc, argv)      ANTIC_Initialise_Ctx(Atari800_default, argc, argv)
#define ANTIC_Reset()                     ANTIC_Reset_Ctx(Atari800_default)
#define ANTIC_Frame(draw_display)         ANTIC_Frame_Ctx(Atari800_default, draw_display)
#define ANTIC_GetDLByte(paddr)            ANTIC_GetDLByte_Ctx(Atari800_default, paddr)
#define ANTIC_GetDLWord(paddr)            ANTIC_GetDLWord_Ctx(Atari800_default, paddr)
#define ANTIC_UpdateArtifacting()         ANTIC_UpdateArtifacting_Ctx(Atari800_default)
#define ANTIC_VideoMemset(ptr, val, size) ANTIC_VideoMemset_Ctx(Atari800_default, ptr, val, size)
#define ANTIC_VideoPutByte(ptr, val)      ANTIC_VideoPutByte_Ctx(Atari800_default, ptr, val)
#define ANTIC_SetPrior(prior)             ANTIC_SetPrior_Ctx(Atari800_default, prior)
#define ANTIC_StateSave()                 ANTIC_StateSave_Ctx(Atari800_default)
#define ANTIC_StateRead()                 ANTIC_StateRead_Ctx(Atari800_default)

UBYTE ANTIC_GetByte(UWORD addr, int no_side_effects);
void ANTIC_PutByte(UWORD addr, UBYTE byte);

/* Pointer to 16 KB seen by ANTIC in 0x4000-0x7fff.
   If it's the same what the CPU sees (and what's in memory[0x4000..0x7fff],
   then NULL. */
#define ANTIC_xe_ptr   (Atari800_default->antic.xe_ptr)

/* PM graphics for GTIA */
#define ANTIC_player_dma_enabled   (Atari800_default->antic.player_dma_enabled)
#define ANTIC_missile_dma_enabled  (Atari800_default->antic.missile_dma_enabled)
#define ANTIC_player_gra_enabled   (Atari800_default->antic.player_gra_enabled)
#define ANTIC_missile_gra_enabled  (Atari800_default->antic.missile_gra_enabled)
#define ANTIC_player_flickering    (Atari800_default->antic.player_flickering)
#define ANTIC_missile_flickering   (Atari800_default->antic.missile_flickering)

/* ANTIC colour lookup tables, used by GTIA */
extern UWORD ANTIC_cl[128];
extern ULONG ANTIC_lookup_gtia9[16];
extern ULONG ANTIC_lookup_gtia11[16];
extern UWORD ANTIC_hires_lookup_l[128];

#ifdef NEW_CYCLE_EXACT
#define ANTIC_NOT_DRAWING -999
#define ANTIC_DRAWING_SCREEN (ANTIC_cur_screen_pos!=ANTIC_NOT_DRAWING)
#define ANTIC_delayed_wsync  (Atari800_default->antic.delayed_wsync)
#define ANTIC_cur_screen_pos (Atari800_default->antic.cur_screen_pos)
extern const int *ANTIC_cpu2antic_ptr;
extern const int *ANTIC_antic2cpu_ptr;

#define ANTIC_XPOS ( ANTIC_DRAWING_SCREEN ? ANTIC_cpu2antic_ptr[ANTIC_xpos] : ANTIC_xpos )
#else
#define ANTIC_XPOS ANTIC_xpos
#endif /* NEW_CYCLE_EXACT */

#ifndef NO_SIMPLE_PAL_BLENDING
/* Set to 1 to enable simplified emulation of PAL blending, that uses only
   the standard 8-bit palette. */
#define ANTIC_pal_blending (Atari800_default->antic.pal_blending)
#endif /* NO_SIMPLE_PAL_BLENDING */

#endif /* ANTIC_H_ */
