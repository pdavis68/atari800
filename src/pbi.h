#ifndef PBI_H_
#define PBI_H_

#include "atari.h"
#include "instance.h" /* Atari800_Instance (transitional default-instance aliases) */
#include <stdio.h>

/* Transitional Option C bridge: the per-instance PBI state lives in
   PBI_state_t (instance.h). Until all callers pass an instance explicitly,
   the legacy global names are aliased to the default instance. */
#define PBI_IRQ     (Atari800_default->pbi.IRQ)
#define PBI_D6D7ram (Atari800_default->pbi.D6D7ram)

/* Context-aware entry points (Option C). The legacy names below are
   forwarding macros that pass the default instance, so not-yet-migrated
   callers are unchanged. The D1/D6/D7 Get/PutByte functions are dispatched
   from MEMORY_HwGetByte/MEMORY_HwPutByte (which currently pin the default
   instance); they will be routed per-instance when the hardware dispatch
   layer is converted. */
int PBI_Initialise_Ctx(Atari800_Instance *inst, int *argc, char *argv[]);
void PBI_Exit_Ctx(Atari800_Instance *inst);
int PBI_ReadConfig_Ctx(Atari800_Instance *inst, char *string, char *ptr);
void PBI_WriteConfig_Ctx(Atari800_Instance *inst, FILE *fp);
void PBI_Reset_Ctx(Atari800_Instance *inst);
UBYTE PBI_D1GetByte_Ctx(Atari800_Instance *inst, UWORD addr, int no_side_effects);
void PBI_D1PutByte_Ctx(Atari800_Instance *inst, UWORD addr, UBYTE byte);
UBYTE PBI_D6GetByte_Ctx(Atari800_Instance *inst, UWORD addr, int no_side_effects);
void PBI_D6PutByte_Ctx(Atari800_Instance *inst, UWORD addr, UBYTE byte);
UBYTE PBI_D7GetByte_Ctx(Atari800_Instance *inst, UWORD addr, int no_side_effects);
void PBI_D7PutByte_Ctx(Atari800_Instance *inst, UWORD addr, UBYTE byte);
void PBI_StateSave_Ctx(Atari800_Instance *inst);
void PBI_StateRead_Ctx(Atari800_Instance *inst);

#define PBI_Initialise(argc, argv) PBI_Initialise_Ctx(Atari800_default, argc, argv)
#define PBI_Exit()                 PBI_Exit_Ctx(Atari800_default)
#define PBI_ReadConfig(str, ptr)   PBI_ReadConfig_Ctx(Atari800_default, str, ptr)
#define PBI_WriteConfig(fp)        PBI_WriteConfig_Ctx(Atari800_default, fp)
#define PBI_Reset()                PBI_Reset_Ctx(Atari800_default)
#define PBI_D1GetByte(addr, nse)   PBI_D1GetByte_Ctx(Atari800_default, addr, nse)
#define PBI_D1PutByte(addr, byte)  PBI_D1PutByte_Ctx(Atari800_default, addr, byte)
#define PBI_D6GetByte(addr, nse)   PBI_D6GetByte_Ctx(Atari800_default, addr, nse)
#define PBI_D6PutByte(addr, byte)  PBI_D6PutByte_Ctx(Atari800_default, addr, byte)
#define PBI_D7GetByte(addr, nse)   PBI_D7GetByte_Ctx(Atari800_default, addr, nse)
#define PBI_D7PutByte(addr, byte)  PBI_D7PutByte_Ctx(Atari800_default, addr, byte)
#define PBI_StateSave()            PBI_StateSave_Ctx(Atari800_default)
#define PBI_StateRead()            PBI_StateRead_Ctx(Atari800_default)

#define PBI_NOT_HANDLED -1
/* #define PBI_DEBUG */
#endif /* PBI_H_ */
