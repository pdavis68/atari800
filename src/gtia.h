#ifndef GTIA_H_
#define GTIA_H_

#include "atari.h"
#include "screen.h"
#include "instance.h" /* Atari800_Instance (transitional default-instance aliases) */

#define GTIA_OFFSET_HPOSP0 0x00
#define GTIA_OFFSET_M0PF 0x00
#define GTIA_OFFSET_HPOSP1 0x01
#define GTIA_OFFSET_M1PF 0x01
#define GTIA_OFFSET_HPOSP2 0x02
#define GTIA_OFFSET_M2PF 0x02
#define GTIA_OFFSET_HPOSP3 0x03
#define GTIA_OFFSET_M3PF 0x03
#define GTIA_OFFSET_HPOSM0 0x04
#define GTIA_OFFSET_P0PF 0x04
#define GTIA_OFFSET_HPOSM1 0x05
#define GTIA_OFFSET_P1PF 0x05
#define GTIA_OFFSET_HPOSM2 0x06
#define GTIA_OFFSET_P2PF 0x06
#define GTIA_OFFSET_HPOSM3 0x07
#define GTIA_OFFSET_P3PF 0x07
#define GTIA_OFFSET_SIZEP0 0x08
#define GTIA_OFFSET_M0PL 0x08
#define GTIA_OFFSET_SIZEP1 0x09
#define GTIA_OFFSET_M1PL 0x09
#define GTIA_OFFSET_SIZEP2 0x0a
#define GTIA_OFFSET_M2PL 0x0a
#define GTIA_OFFSET_SIZEP3 0x0b
#define GTIA_OFFSET_M3PL 0x0b
#define GTIA_OFFSET_SIZEM 0x0c
#define GTIA_OFFSET_P0PL 0x0c
#define GTIA_OFFSET_GRAFP0 0x0d
#define GTIA_OFFSET_P1PL 0x0d
#define GTIA_OFFSET_GRAFP1 0x0e
#define GTIA_OFFSET_P2PL 0x0e
#define GTIA_OFFSET_GRAFP2 0x0f
#define GTIA_OFFSET_P3PL 0x0f
#define GTIA_OFFSET_GRAFP3 0x10
#define GTIA_OFFSET_TRIG0 0x10
#define GTIA_OFFSET_GRAFM 0x11
#define GTIA_OFFSET_TRIG1 0x11
#define GTIA_OFFSET_COLPM0 0x12
#define GTIA_OFFSET_TRIG2 0x12
#define GTIA_OFFSET_COLPM1 0x13
#define GTIA_OFFSET_TRIG3 0x13
#define GTIA_OFFSET_COLPM2 0x14
#define GTIA_OFFSET_PAL 0x14
#define GTIA_OFFSET_COLPM3 0x15
#define GTIA_OFFSET_COLPF0 0x16
#define GTIA_OFFSET_COLPF1 0x17
#define GTIA_OFFSET_COLPF2 0x18
#define GTIA_OFFSET_COLPF3 0x19
#define GTIA_OFFSET_COLBK 0x1a
#define GTIA_OFFSET_PRIOR 0x1b
#define GTIA_OFFSET_VDELAY 0x1c
#define GTIA_OFFSET_GRACTL 0x1d
#define GTIA_OFFSET_HITCLR 0x1e
#define GTIA_OFFSET_CONSOL 0x1f

/* Transitional Option C bridge: the per-instance GTIA state lives in
   GTIA_state_t (instance.h). Until all callers pass an instance explicitly,
   the legacy global names are aliased to the default instance. */
#define GTIA_GRAFM  (Atari800_default->gtia.GRAFM)
#define GTIA_GRAFP0 (Atari800_default->gtia.GRAFP0)
#define GTIA_GRAFP1 (Atari800_default->gtia.GRAFP1)
#define GTIA_GRAFP2 (Atari800_default->gtia.GRAFP2)
#define GTIA_GRAFP3 (Atari800_default->gtia.GRAFP3)
#define GTIA_HPOSP0 (Atari800_default->gtia.HPOSP0)
#define GTIA_HPOSP1 (Atari800_default->gtia.HPOSP1)
#define GTIA_HPOSP2 (Atari800_default->gtia.HPOSP2)
#define GTIA_HPOSP3 (Atari800_default->gtia.HPOSP3)
#define GTIA_HPOSM0 (Atari800_default->gtia.HPOSM0)
#define GTIA_HPOSM1 (Atari800_default->gtia.HPOSM1)
#define GTIA_HPOSM2 (Atari800_default->gtia.HPOSM2)
#define GTIA_HPOSM3 (Atari800_default->gtia.HPOSM3)
#define GTIA_SIZEP0 (Atari800_default->gtia.SIZEP0)
#define GTIA_SIZEP1 (Atari800_default->gtia.SIZEP1)
#define GTIA_SIZEP2 (Atari800_default->gtia.SIZEP2)
#define GTIA_SIZEP3 (Atari800_default->gtia.SIZEP3)
#define GTIA_SIZEM  (Atari800_default->gtia.SIZEM)
#define GTIA_COLPM0 (Atari800_default->gtia.COLPM0)
#define GTIA_COLPM1 (Atari800_default->gtia.COLPM1)
#define GTIA_COLPM2 (Atari800_default->gtia.COLPM2)
#define GTIA_COLPM3 (Atari800_default->gtia.COLPM3)
#define GTIA_COLPF0 (Atari800_default->gtia.COLPF0)
#define GTIA_COLPF1 (Atari800_default->gtia.COLPF1)
#define GTIA_COLPF2 (Atari800_default->gtia.COLPF2)
#define GTIA_COLPF3 (Atari800_default->gtia.COLPF3)
#define GTIA_COLBK  (Atari800_default->gtia.COLBK)
#define GTIA_GRACTL (Atari800_default->gtia.GRACTL)
#define GTIA_M0PL   (Atari800_default->gtia.M0PL)
#define GTIA_M1PL   (Atari800_default->gtia.M1PL)
#define GTIA_M2PL   (Atari800_default->gtia.M2PL)
#define GTIA_M3PL   (Atari800_default->gtia.M3PL)
#define GTIA_P0PL   (Atari800_default->gtia.P0PL)
#define GTIA_P1PL   (Atari800_default->gtia.P1PL)
#define GTIA_P2PL   (Atari800_default->gtia.P2PL)
#define GTIA_P3PL   (Atari800_default->gtia.P3PL)
#define GTIA_PRIOR  (Atari800_default->gtia.PRIOR)
#define GTIA_VDELAY (Atari800_default->gtia.VDELAY)

#ifdef USE_COLOUR_TRANSLATION_TABLE

extern UWORD GTIA_colour_translation_table[256];
#define GTIA_COLOUR_BLACK GTIA_colour_translation_table[0]
#define GTIA_COLOUR_TO_WORD(dest,src) dest = GTIA_colour_translation_table[src];

#else

#define GTIA_COLOUR_BLACK 0
#define GTIA_COLOUR_TO_WORD(dest,src) dest = (((UWORD) (src)) << 8) | (src);

#endif /* USE_COLOUR_TRANSLATION_TABLE */

#define GTIA_PM_SCANLINE_SIZE (Screen_WIDTH / 2 + 8)
#define GTIA_pm_scanline (Atari800_default->gtia.pm_scanline)	/* there's a byte for every *pair* of pixels */
#define GTIA_pm_dirty    (Atari800_default->gtia.pm_dirty)

#define GTIA_collisions_mask_missile_playfield (Atari800_default->gtia.collisions_mask_missile_playfield)
#define GTIA_collisions_mask_player_playfield  (Atari800_default->gtia.collisions_mask_player_playfield)
#define GTIA_collisions_mask_missile_player    (Atari800_default->gtia.collisions_mask_missile_player)
#define GTIA_collisions_mask_player_player     (Atari800_default->gtia.collisions_mask_player_player)

#define GTIA_TRIG        (Atari800_default->gtia.TRIG)
#define GTIA_TRIG_latch  (Atari800_default->gtia.TRIG_latch)

#define GTIA_consol_override (Atari800_default->gtia.consol_override)
#define GTIA_speaker         (Atari800_default->gtia.speaker)

/* Context-aware entry points (Option C). The legacy names below are
   forwarding macros that pass the default instance, so not-yet-migrated
   callers are unchanged. GTIA_GetByte/GTIA_PutByte remain real functions
   (registered in the per-instance MEMORY_readmap/MEMORY_writemap
   function-pointer tables, which have a fixed context-free signature) and
   pin the default instance. */
int GTIA_Initialise_Ctx(Atari800_Instance *inst, int *argc, char *argv[]);
void GTIA_Frame_Ctx(Atari800_Instance *inst);
void GTIA_NewPmScanline_Ctx(Atari800_Instance *inst);
UBYTE GTIA_GetByte_Ctx(Atari800_Instance *inst, UWORD addr, int no_side_effects);
void GTIA_PutByte_Ctx(Atari800_Instance *inst, UWORD addr, UBYTE byte);
void GTIA_StateSave_Ctx(Atari800_Instance *inst);
void GTIA_StateRead_Ctx(Atari800_Instance *inst, UBYTE version);

#ifdef NEW_CYCLE_EXACT
void GTIA_UpdatePmplColls_Ctx(Atari800_Instance *inst);
#endif

#define GTIA_Initialise(argc, argv) GTIA_Initialise_Ctx(Atari800_default, argc, argv)
#define GTIA_Frame()                GTIA_Frame_Ctx(Atari800_default)
#define GTIA_NewPmScanline()        GTIA_NewPmScanline_Ctx(Atari800_default)
#define GTIA_StateSave()            GTIA_StateSave_Ctx(Atari800_default)
#define GTIA_StateRead(version)     GTIA_StateRead_Ctx(Atari800_default, version)

UBYTE GTIA_GetByte(UWORD addr, int no_side_effects);
void GTIA_PutByte(UWORD addr, UBYTE byte);
#ifdef NEW_CYCLE_EXACT
#define GTIA_UpdatePmplColls()      GTIA_UpdatePmplColls_Ctx(Atari800_default)
#endif
#endif /* GTIA_H_ */
