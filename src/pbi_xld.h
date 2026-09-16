#ifndef PBI_XLD_H_
#define PBI_XLD_H_

#include "atari.h"
#include "instance.h" /* Atari800_Instance (transitional default-instance aliases) */

/* Transitional Option C bridge: the per-instance XLD state lives in
   XLD_state_t (instance.h). Until all callers pass an instance
   explicitly, the legacy global names are aliased to the default instance. */
#define PBI_XLD_enabled   (Atari800_default->xld.enabled)
#define PBI_XLD_v_enabled (Atari800_default->xld.v_enabled)

/* Context-aware entry points (Option C). The legacy names below are
   forwarding macros that pass the default instance, so not-yet-migrated
   callers are unchanged. */
int PBI_XLD_Initialise_Ctx(Atari800_Instance *inst, int *argc, char *argv[]);
void PBI_XLD_Exit_Ctx(Atari800_Instance *inst);
int PBI_XLD_ReadConfig_Ctx(Atari800_Instance *inst, char *string, char *ptr);
void PBI_XLD_WriteConfig_Ctx(Atari800_Instance *inst, FILE *fp);
void PBI_XLD_Reset_Ctx(Atari800_Instance *inst);
int PBI_XLD_D1GetByte_Ctx(Atari800_Instance *inst, UWORD addr);
UBYTE PBI_XLD_D1ffGetByte_Ctx(Atari800_Instance *inst);
void PBI_XLD_D1PutByte_Ctx(Atari800_Instance *inst, UWORD addr, UBYTE byte);
int PBI_XLD_D1ffPutByte_Ctx(Atari800_Instance *inst, UBYTE byte);
void PBI_XLD_StateSave_Ctx(Atari800_Instance *inst);
void PBI_XLD_StateRead_Ctx(Atari800_Instance *inst);
void PBI_XLD_votrax_busy_callback_Ctx(Atari800_Instance *inst, int busy_status);

#define PBI_XLD_Initialise(argc, argv)     PBI_XLD_Initialise_Ctx(Atari800_default, argc, argv)
#define PBI_XLD_Exit()                     PBI_XLD_Exit_Ctx(Atari800_default)
#define PBI_XLD_ReadConfig(str, ptr)       PBI_XLD_ReadConfig_Ctx(Atari800_default, str, ptr)
#define PBI_XLD_WriteConfig(fp)            PBI_XLD_WriteConfig_Ctx(Atari800_default, fp)
#define PBI_XLD_Reset()                    PBI_XLD_Reset_Ctx(Atari800_default)
#define PBI_XLD_D1GetByte(addr)            PBI_XLD_D1GetByte_Ctx(Atari800_default, addr)
#define PBI_XLD_D1ffGetByte()              PBI_XLD_D1ffGetByte_Ctx(Atari800_default)
#define PBI_XLD_D1PutByte(addr, byte)      PBI_XLD_D1PutByte_Ctx(Atari800_default, addr, byte)
#define PBI_XLD_D1ffPutByte(byte)          PBI_XLD_D1ffPutByte_Ctx(Atari800_default, byte)
#define PBI_XLD_StateSave()                PBI_XLD_StateSave_Ctx(Atari800_default)
#define PBI_XLD_StateRead()                PBI_XLD_StateRead_Ctx(Atari800_default)
#define PBI_XLD_votrax_busy_callback(st)   PBI_XLD_votrax_busy_callback_Ctx(Atari800_default, st)

/* Never defined nor used (leftover sound hook declarations kept for
   compatibility; the voice box is handled by VOTRAXSND_*). */
void PBI_XLD_VInit(int playback_freq, int num_pokeys, int bit16);
void PBI_XLD_VFrame(void);
void PBI_XLD_VProcess(void *sndbuffer, int sndn);

#endif /* PBI_XLD_H_ */
