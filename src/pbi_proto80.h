#ifndef PBI_PROTO80_H_
#define PBI_PROTO80_H_

#include "atari.h"
#include "instance.h" /* Atari800_Instance (transitional default-instance aliases) */

/* Transitional Option C bridge: the per-instance PROTO80 state lives in
   PROTO80_state_t (instance.h). Until all callers pass an instance
   explicitly, the legacy global names are aliased to the default instance. */
#define PBI_PROTO80_enabled (Atari800_default->proto80.enabled)

/* Context-aware entry points (Option C). The legacy names below are
   forwarding macros that pass the default instance, so not-yet-migrated
   callers are unchanged. */
int PBI_PROTO80_Initialise_Ctx(Atari800_Instance *inst, int *argc, char *argv[]);
void PBI_PROTO80_Exit_Ctx(Atari800_Instance *inst);
int PBI_PROTO80_ReadConfig_Ctx(Atari800_Instance *inst, char *string, char *ptr);
void PBI_PROTO80_WriteConfig_Ctx(Atari800_Instance *inst, FILE *fp);
int PBI_PROTO80_D1GetByte_Ctx(Atari800_Instance *inst, UWORD addr, int no_side_effects);
void PBI_PROTO80_D1PutByte_Ctx(Atari800_Instance *inst, UWORD addr, UBYTE byte);
int PBI_PROTO80_D1ffPutByte_Ctx(Atari800_Instance *inst, UBYTE byte);
UBYTE PBI_PROTO80_GetPixels_Ctx(Atari800_Instance *inst, int scanline, int column);

#define PBI_PROTO80_Initialise(argc, argv) PBI_PROTO80_Initialise_Ctx(Atari800_default, argc, argv)
#define PBI_PROTO80_Exit()                 PBI_PROTO80_Exit_Ctx(Atari800_default)
#define PBI_PROTO80_ReadConfig(str, ptr)   PBI_PROTO80_ReadConfig_Ctx(Atari800_default, str, ptr)
#define PBI_PROTO80_WriteConfig(fp)        PBI_PROTO80_WriteConfig_Ctx(Atari800_default, fp)
#define PBI_PROTO80_D1GetByte(addr, nse)   PBI_PROTO80_D1GetByte_Ctx(Atari800_default, addr, nse)
#define PBI_PROTO80_D1PutByte(addr, byte)  PBI_PROTO80_D1PutByte_Ctx(Atari800_default, addr, byte)
#define PBI_PROTO80_D1ffPutByte(byte)      PBI_PROTO80_D1ffPutByte_Ctx(Atari800_default, byte)
#define PBI_PROTO80_GetPixels(scan, col)   PBI_PROTO80_GetPixels_Ctx(Atari800_default, scan, col)

#endif /* PBI_PROTO80_H_ */
