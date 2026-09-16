#ifndef AF80_H_
#define AF80_H_

#include "atari.h"
#include "instance.h" /* Atari800_Instance (transitional default-instance aliases) */
#include <stdio.h>

extern int AF80_palette[16];

/* Transitional Option C bridge: the per-instance AF80 state lives in
   AF80_state_t (instance.h). Until all callers pass an instance
   explicitly, the legacy global names are aliased to the default
   instance. AF80_palette is intentionally kept as a real file-scope
   global in af80.c (it is derived from the shared read-only RGBI table
   and referenced from static initialisers in sdl/palette.c). */

/* Context-aware entry points (Option C). The legacy names below are
   forwarding macros that pass the default instance, so not-yet-migrated
   callers are unchanged. */
int AF80_Initialise_Ctx(Atari800_Instance *inst, int *argc, char *argv[]);
void AF80_Exit_Ctx(Atari800_Instance *inst);
void AF80_InsertRightCartridge_Ctx(Atari800_Instance *inst);
int AF80_ReadConfig_Ctx(Atari800_Instance *inst, char *string, char *ptr);
void AF80_WriteConfig_Ctx(Atari800_Instance *inst, FILE *fp);
int AF80_D5GetByte_Ctx(Atari800_Instance *inst, UWORD addr, int no_side_effects);
void AF80_D5PutByte_Ctx(Atari800_Instance *inst, UWORD addr, UBYTE byte);
int AF80_D6GetByte_Ctx(Atari800_Instance *inst, UWORD addr, int no_side_effects);
void AF80_D6PutByte_Ctx(Atari800_Instance *inst, UWORD addr, UBYTE byte);
UBYTE AF80_GetPixels_Ctx(Atari800_Instance *inst, int scanline, int column, int *colour, int blink);
void AF80_Reset_Ctx(Atari800_Instance *inst);

#define AF80_enabled                        (Atari800_default->af80.enabled)
#define AF80_Initialise(argc, argv)         AF80_Initialise_Ctx(Atari800_default, argc, argv)
#define AF80_Exit()                         AF80_Exit_Ctx(Atari800_default)
#define AF80_InsertRightCartridge()         AF80_InsertRightCartridge_Ctx(Atari800_default)
#define AF80_ReadConfig(str, ptr)           AF80_ReadConfig_Ctx(Atari800_default, str, ptr)
#define AF80_WriteConfig(fp)                AF80_WriteConfig_Ctx(Atari800_default, fp)
#define AF80_D5GetByte(addr, nse)           AF80_D5GetByte_Ctx(Atari800_default, addr, nse)
#define AF80_D5PutByte(addr, byte)          AF80_D5PutByte_Ctx(Atari800_default, addr, byte)
#define AF80_D6GetByte(addr, nse)           AF80_D6GetByte_Ctx(Atari800_default, addr, nse)
#define AF80_D6PutByte(addr, byte)          AF80_D6PutByte_Ctx(Atari800_default, addr, byte)
#define AF80_GetPixels(scan, col, c, blink) AF80_GetPixels_Ctx(Atari800_default, scan, col, c, blink)
#define AF80_Reset()                        AF80_Reset_Ctx(Atari800_default)

#endif /* AF80_H_ */
