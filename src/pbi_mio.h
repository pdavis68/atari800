#ifndef PBI_MIO_H_
#define PBI_MIO_H_

#include "atari.h"
#include <stdio.h>

#include "instance.h" /* Atari800_Instance (transitional default-instance aliases) */

/* Transitional Option C bridge: the per-instance MIO state lives in
   MIO_state_t (instance.h). Until all callers pass an instance explicitly,
   the legacy global names are aliased to the default instance. */
#define PBI_MIO_enabled (Atari800_default->mio.enabled)

/* Context-aware entry points (Option C). The legacy names below are
   forwarding macros that pass the default instance, so not-yet-migrated
   callers (pbi.c, statesav.c) are unchanged. */
int PBI_MIO_Initialise_Ctx(Atari800_Instance *inst, int *argc, char *argv[]);
void PBI_MIO_Exit_Ctx(Atari800_Instance *inst);
UBYTE PBI_MIO_D1GetByte_Ctx(Atari800_Instance *inst, UWORD addr, int no_side_effects);
void PBI_MIO_D1PutByte_Ctx(Atari800_Instance *inst, UWORD addr, UBYTE byte);
UBYTE PBI_MIO_D6GetByte_Ctx(Atari800_Instance *inst, UWORD addr, int no_side_effects);
void PBI_MIO_D6PutByte_Ctx(Atari800_Instance *inst, UWORD addr, UBYTE byte);
int PBI_MIO_ReadConfig_Ctx(Atari800_Instance *inst, char *string, char *ptr);
void PBI_MIO_WriteConfig_Ctx(Atari800_Instance *inst, FILE *fp);
void PBI_MIO_StateSave_Ctx(Atari800_Instance *inst);
void PBI_MIO_StateRead_Ctx(Atari800_Instance *inst);

#define PBI_MIO_Initialise(argc, argv) PBI_MIO_Initialise_Ctx(Atari800_default, argc, argv)
#define PBI_MIO_Exit()                 PBI_MIO_Exit_Ctx(Atari800_default)
#define PBI_MIO_D1GetByte(addr, nse)   PBI_MIO_D1GetByte_Ctx(Atari800_default, addr, nse)
#define PBI_MIO_D1PutByte(addr, byte)  PBI_MIO_D1PutByte_Ctx(Atari800_default, addr, byte)
#define PBI_MIO_D6GetByte(addr, nse)   PBI_MIO_D6GetByte_Ctx(Atari800_default, addr, nse)
#define PBI_MIO_D6PutByte(addr, byte)  PBI_MIO_D6PutByte_Ctx(Atari800_default, addr, byte)
#define PBI_MIO_ReadConfig(str, ptr)   PBI_MIO_ReadConfig_Ctx(Atari800_default, str, ptr)
#define PBI_MIO_WriteConfig(fp)        PBI_MIO_WriteConfig_Ctx(Atari800_default, fp)
#define PBI_MIO_StateSave()            PBI_MIO_StateSave_Ctx(Atari800_default)
#define PBI_MIO_StateRead()            PBI_MIO_StateRead_Ctx(Atari800_default)

#endif /* PBI_MIO_H_ */
