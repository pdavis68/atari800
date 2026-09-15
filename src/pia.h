#ifndef PIA_H_
#define PIA_H_

#include "atari.h"
#include "instance.h" /* Atari800_Instance (transitional default-instance aliases) */

#define PIA_OFFSET_PORTA 0x00
#define PIA_OFFSET_PORTB 0x01
#define PIA_OFFSET_PACTL 0x02
#define PIA_OFFSET_PBCTL 0x03

/* Transitional Option C bridge: the per-instance PIA state lives in
   PIA_state_t (instance.h). Until all callers pass an instance explicitly,
   the legacy global names are aliased to the default instance. */
#define PIA_PACTL      (Atari800_default->pia.PACTL)
#define PIA_PBCTL      (Atari800_default->pia.PBCTL)
#define PIA_PORTA      (Atari800_default->pia.PORTA)
#define PIA_PORTB      (Atari800_default->pia.PORTB)
#define PIA_PORTA_mask (Atari800_default->pia.PORTA_mask)
#define PIA_PORTB_mask (Atari800_default->pia.PORTB_mask)
#define PIA_PORT_input (Atari800_default->pia.PORT_input)
/* PROCEED/INTERRUPT pin support (CA1/CB1) */
#define PIA_CA1        (Atari800_default->pia.CA1)
#define PIA_CB1        (Atari800_default->pia.CB1)
#define PIA_CA2        (Atari800_default->pia.CA2)
#define PIA_CB2        (Atari800_default->pia.CB2)
#define PIA_IRQ        (Atari800_default->pia.IRQ)

/* Context-aware entry points (Option C). The legacy names below are
   forwarding macros that pass the default instance, so not-yet-migrated
   callers are unchanged. PIA_GetByte/PIA_PutByte remain real functions
   (registered in the per-instance MEMORY_readmap/MEMORY_writemap
   function-pointer tables, which have a fixed context-free signature) and
   pin the default instance. */
int PIA_Initialise_Ctx(Atari800_Instance *inst, int *argc, char *argv[]);
void PIA_Reset_Ctx(Atari800_Instance *inst);
UBYTE PIA_GetByte_Ctx(Atari800_Instance *inst, UWORD addr, int no_side_effects);
void PIA_PutByte_Ctx(Atari800_Instance *inst, UWORD addr, UBYTE byte);
void PIA_StateSave_Ctx(Atari800_Instance *inst);
void PIA_StateRead_Ctx(Atari800_Instance *inst, UBYTE version);
/* Set PROCEED (CA1) and INTERRUPT (CB1) pin values */
void PIA_SetCA1_Ctx(Atari800_Instance *inst, int value);
void PIA_SetCB1_Ctx(Atari800_Instance *inst, int value);
void update_PIA_IRQ_Ctx(Atari800_Instance *inst);

#define PIA_Initialise(argc, argv) PIA_Initialise_Ctx(Atari800_default, argc, argv)
#define PIA_Reset()                PIA_Reset_Ctx(Atari800_default)
#define PIA_StateSave()            PIA_StateSave_Ctx(Atari800_default)
#define PIA_StateRead(version)     PIA_StateRead_Ctx(Atari800_default, version)
#define PIA_SetCA1(value)          PIA_SetCA1_Ctx(Atari800_default, value)
#define PIA_SetCB1(value)          PIA_SetCB1_Ctx(Atari800_default, value)
#define update_PIA_IRQ()           update_PIA_IRQ_Ctx(Atari800_default)

UBYTE PIA_GetByte(UWORD addr, int no_side_effects);
void PIA_PutByte(UWORD addr, UBYTE byte);

#endif /* PIA_H_ */
