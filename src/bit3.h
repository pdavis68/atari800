#ifndef BIT3_H_
#define BIT3_H_

#include "atari.h"
#include "instance.h" /* Atari800_Instance (transitional default-instance aliases) */
#include <stdio.h>

extern int BIT3_palette[2];

/* Transitional Option C bridge: the per-instance BIT3 state lives in
   BIT3_state_t (instance.h). Until all callers pass an instance
   explicitly, the legacy global names are aliased to the default
   instance. BIT3_palette is intentionally kept as a real file-scope
   global in bit3.c (referenced from a static initialiser in
   sdl/palette.c). VIDEOMODE_80_column / VIDEOMODE_Set80Column remain
   process-global (videomode is a Phase 4 module, transitional). */

/* Context-aware entry points (Option C). The legacy names below are
   forwarding macros that pass the default instance, so not-yet-migrated
   callers are unchanged. */
int BIT3_Initialise_Ctx(Atari800_Instance *inst, int *argc, char *argv[]);
void BIT3_Exit_Ctx(Atari800_Instance *inst);
void BIT3_InsertRightCartridge_Ctx(Atari800_Instance *inst);
int BIT3_ReadConfig_Ctx(Atari800_Instance *inst, char *string, char *ptr);
void BIT3_WriteConfig_Ctx(Atari800_Instance *inst, FILE *fp);
int BIT3_D5GetByte_Ctx(Atari800_Instance *inst, UWORD addr, int no_side_effects);
void BIT3_D5PutByte_Ctx(Atari800_Instance *inst, UWORD addr, UBYTE byte);
int BIT3_D6GetByte_Ctx(Atari800_Instance *inst, UWORD addr, int no_side_effects);
void BIT3_D6PutByte_Ctx(Atari800_Instance *inst, UWORD addr, UBYTE byte);
UBYTE BIT3_GetPixels_Ctx(Atari800_Instance *inst, int scanline, int column, int *colour, int blink);
void BIT3_Reset_Ctx(Atari800_Instance *inst);

#define BIT3_enabled                        (Atari800_default->bit3.enabled)
#define BIT3_Initialise(argc, argv)         BIT3_Initialise_Ctx(Atari800_default, argc, argv)
#define BIT3_Exit()                         BIT3_Exit_Ctx(Atari800_default)
#define BIT3_InsertRightCartridge()         BIT3_InsertRightCartridge_Ctx(Atari800_default)
#define BIT3_ReadConfig(str, ptr)           BIT3_ReadConfig_Ctx(Atari800_default, str, ptr)
#define BIT3_WriteConfig(fp)                BIT3_WriteConfig_Ctx(Atari800_default, fp)
#define BIT3_D5GetByte(addr, nse)           BIT3_D5GetByte_Ctx(Atari800_default, addr, nse)
#define BIT3_D5PutByte(addr, byte)          BIT3_D5PutByte_Ctx(Atari800_default, addr, byte)
#define BIT3_D6GetByte(addr, nse)           BIT3_D6GetByte_Ctx(Atari800_default, addr, nse)
#define BIT3_D6PutByte(addr, byte)          BIT3_D6PutByte_Ctx(Atari800_default, addr, byte)
#define BIT3_GetPixels(scan, col, c, blink) BIT3_GetPixels_Ctx(Atari800_default, scan, col, c, blink)
#define BIT3_Reset()                        BIT3_Reset_Ctx(Atari800_default)

#endif /* BIT3_H_ */
