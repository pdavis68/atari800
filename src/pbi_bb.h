#ifndef PBI_BB_H_
#define PBI_BB_H_

#include "atari.h"
#include <stdio.h>

#include "instance.h" /* Atari800_Instance (transitional default-instance aliases) */

/* Transitional Option C bridge: the per-instance Black Box state lives in
   BB_state_t (instance.h). Until all callers pass an instance explicitly,
   the legacy global names are aliased to the default instance. */
#define PBI_BB_enabled (Atari800_default->bb.enabled)

/* Context-aware entry points (Option C). The legacy names below are
   forwarding macros that pass the default instance, so not-yet-migrated
   callers (pbi.c, atari.c, statesav.c, libatari800/main.c) are unchanged. */
int PBI_BB_Initialise_Ctx(Atari800_Instance *inst, int *argc, char *argv[]);
void PBI_BB_Exit_Ctx(Atari800_Instance *inst);
UBYTE PBI_BB_D1GetByte_Ctx(Atari800_Instance *inst, UWORD addr, int no_side_effects);
void PBI_BB_D1PutByte_Ctx(Atari800_Instance *inst, UWORD addr, UBYTE byte);
UBYTE PBI_BB_D6GetByte_Ctx(Atari800_Instance *inst, UWORD addr, int no_side_effects);
void PBI_BB_D6PutByte_Ctx(Atari800_Instance *inst, UWORD addr, UBYTE byte);
int PBI_BB_ReadConfig_Ctx(Atari800_Instance *inst, char *string, char *ptr);
void PBI_BB_WriteConfig_Ctx(Atari800_Instance *inst, FILE *fp);
void PBI_BB_Menu_Ctx(Atari800_Instance *inst);
void PBI_BB_Frame_Ctx(Atari800_Instance *inst);
void PBI_BB_StateSave_Ctx(Atari800_Instance *inst);
void PBI_BB_StateRead_Ctx(Atari800_Instance *inst);

#define PBI_BB_Initialise(argc, argv) PBI_BB_Initialise_Ctx(Atari800_default, argc, argv)
#define PBI_BB_Exit()                 PBI_BB_Exit_Ctx(Atari800_default)
#define PBI_BB_D1GetByte(addr, nse)   PBI_BB_D1GetByte_Ctx(Atari800_default, addr, nse)
#define PBI_BB_D1PutByte(addr, byte)  PBI_BB_D1PutByte_Ctx(Atari800_default, addr, byte)
#define PBI_BB_D6GetByte(addr, nse)   PBI_BB_D6GetByte_Ctx(Atari800_default, addr, nse)
#define PBI_BB_D6PutByte(addr, byte)  PBI_BB_D6PutByte_Ctx(Atari800_default, addr, byte)
#define PBI_BB_ReadConfig(str, ptr)   PBI_BB_ReadConfig_Ctx(Atari800_default, str, ptr)
#define PBI_BB_WriteConfig(fp)        PBI_BB_WriteConfig_Ctx(Atari800_default, fp)
#define PBI_BB_Menu()                 PBI_BB_Menu_Ctx(Atari800_default)
#define PBI_BB_Frame()                PBI_BB_Frame_Ctx(Atari800_default)
#define PBI_BB_StateSave()            PBI_BB_StateSave_Ctx(Atari800_default)
#define PBI_BB_StateRead()            PBI_BB_StateRead_Ctx(Atari800_default)

#endif /* PBI_BB_H_ */
