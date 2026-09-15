#ifndef POKEY_H_
#define POKEY_H_

#ifdef ASAP /* external project, see http://asap.sf.net */
#include "asap_internal.h"
#else
#include "atari.h"
#endif

#define POKEY_OFFSET_AUDF1 0x00
#define POKEY_OFFSET_AUDC1 0x01
#define POKEY_OFFSET_AUDF2 0x02
#define POKEY_OFFSET_AUDC2 0x03
#define POKEY_OFFSET_AUDF3 0x04
#define POKEY_OFFSET_AUDC3 0x05
#define POKEY_OFFSET_AUDF4 0x06
#define POKEY_OFFSET_AUDC4 0x07
#define POKEY_OFFSET_AUDCTL 0x08
#define POKEY_OFFSET_STIMER 0x09
#define POKEY_OFFSET_SKRES 0x0a
#define POKEY_OFFSET_POTGO 0x0b
#define POKEY_OFFSET_SEROUT 0x0d
#define POKEY_OFFSET_IRQEN 0x0e
#define POKEY_OFFSET_SKCTL 0x0f

#define POKEY_OFFSET_POT0 0x00
#define POKEY_OFFSET_POT1 0x01
#define POKEY_OFFSET_POT2 0x02
#define POKEY_OFFSET_POT3 0x03
#define POKEY_OFFSET_POT4 0x04
#define POKEY_OFFSET_POT5 0x05
#define POKEY_OFFSET_POT6 0x06
#define POKEY_OFFSET_POT7 0x07
#define POKEY_OFFSET_ALLPOT 0x08
#define POKEY_OFFSET_KBCODE 0x09
#define POKEY_OFFSET_RANDOM 0x0a
#define POKEY_OFFSET_SERIN 0x0d
#define POKEY_OFFSET_IRQST 0x0e
#define POKEY_OFFSET_SKSTAT 0x0f

#define POKEY_OFFSET_POKEY2 0x10			/* offset to second pokey chip (STEREO expansion) */

#ifndef ASAP

#include "instance.h" /* Atari800_Instance (transitional default-instance aliases) */

/* Transitional Option C bridge: the per-instance POKEY state lives in
   POKEY_state_t (instance.h). Until all callers pass an instance explicitly,
   the legacy global names are aliased to the default instance. */
#define POKEY_KBCODE           (Atari800_default->pokey.KBCODE)
#define POKEY_IRQST            (Atari800_default->pokey.IRQST)
#define POKEY_IRQEN            (Atari800_default->pokey.IRQEN)
#define POKEY_SKSTAT           (Atari800_default->pokey.SKSTAT)
#define POKEY_SKCTL            (Atari800_default->pokey.SKCTL)
#define POKEY_DELAYED_SERIN_IRQ  (Atari800_default->pokey.DELAYED_SERIN_IRQ)
#define POKEY_DELAYED_SEROUT_IRQ (Atari800_default->pokey.DELAYED_SEROUT_IRQ)
#define POKEY_DELAYED_XMTDONE_IRQ (Atari800_default->pokey.DELAYED_XMTDONE_IRQ)
#ifdef NEW_CYCLE_EXACT
#define POKEY_irq_at_xpos      (Atari800_default->pokey.irq_at_xpos)
#define POKEY_irq_pending_mask (Atari800_default->pokey.irq_pending_mask)
#endif

#define POKEY_POT_input        (Atari800_default->pokey.POT_input)

/* Context-aware entry points (Option C). The legacy names below are
   forwarding macros that pass the default instance, so not-yet-migrated
   callers are unchanged. POKEY_GetByte/POKEY_PutByte remain real functions
   (registered in the per-instance MEMORY_readmap/MEMORY_writemap
   function-pointer tables, which have a fixed context-free signature) and
   pin the default instance. */
ULONG POKEY_GetRandomCounter_Ctx(Atari800_Instance *inst);
void POKEY_SetRandomCounter_Ctx(Atari800_Instance *inst, ULONG value);
UBYTE POKEY_GetByte_Ctx(Atari800_Instance *inst, UWORD addr, int no_side_effects);
void POKEY_PutByte_Ctx(Atari800_Instance *inst, UWORD addr, UBYTE byte);
int POKEY_Initialise_Ctx(Atari800_Instance *inst, int *argc, char *argv[]);
void POKEY_Frame_Ctx(Atari800_Instance *inst);
void POKEY_Scanline_Ctx(Atari800_Instance *inst);
void POKEY_StateSave_Ctx(Atari800_Instance *inst);
void POKEY_StateRead_Ctx(Atari800_Instance *inst);

#define POKEY_GetRandomCounter()  POKEY_GetRandomCounter_Ctx(Atari800_default)
#define POKEY_SetRandomCounter(v) POKEY_SetRandomCounter_Ctx(Atari800_default, v)
#define POKEY_Initialise(argc, argv) POKEY_Initialise_Ctx(Atari800_default, argc, argv)
#define POKEY_Frame()                POKEY_Frame_Ctx(Atari800_default)
#define POKEY_Scanline()             POKEY_Scanline_Ctx(Atari800_default)
#define POKEY_StateSave()            POKEY_StateSave_Ctx(Atari800_default)
#define POKEY_StateRead()            POKEY_StateRead_Ctx(Atari800_default)

UBYTE POKEY_GetByte(UWORD addr, int no_side_effects);
void POKEY_PutByte(UWORD addr, UBYTE byte);

#endif

/* CONSTANT DEFINITIONS */

/* definitions for AUDCx (D201, D203, D205, D207) */
#define POKEY_NOTPOLY5    0x80		/* selects POLY5 or direct CLOCK */
#define POKEY_POLY4       0x40		/* selects POLY4 or POLY17 */
#define POKEY_PURETONE    0x20		/* selects POLY4/17 or PURE tone */
#define POKEY_VOL_ONLY    0x10		/* selects VOLUME OUTPUT ONLY */
#define POKEY_VOLUME_MASK 0x0f		/* volume mask */

/* definitions for AUDCTL (D208) */
#define POKEY_POLY9       0x80		/* selects POLY9 or POLY17 */
#define POKEY_CH1_179     0x40		/* selects 1.78979 MHz for Ch 1 */
#define POKEY_CH3_179     0x20		/* selects 1.78979 MHz for Ch 3 */
#define POKEY_CH1_CH2     0x10		/* clocks channel 1 w/channel 2 */
#define POKEY_CH3_CH4     0x08		/* clocks channel 3 w/channel 4 */
#define POKEY_CH1_FILTER  0x04		/* selects channel 1 high pass filter */
#define POKEY_CH2_FILTER  0x02		/* selects channel 2 high pass filter */
#define POKEY_CLOCK_15    0x01		/* selects 15.6999kHz or 63.9210kHz */

/* for accuracy, the 64kHz and 15kHz clocks are exact divisions of
   the 1.79MHz clock */
#define POKEY_DIV_64      28			/* divisor for 1.79MHz clock to 64 kHz */
#define POKEY_DIV_15      114			/* divisor for 1.79MHz clock to 15 kHz */

/* the size (in entries) of the 4 polynomial tables */
#define POKEY_POLY4_SIZE  0x000f
#define POKEY_POLY5_SIZE  0x001f
#define POKEY_POLY9_SIZE  0x01ff
#define POKEY_POLY17_SIZE 0x0001ffff

#define POKEY_MAXPOKEYS         2		/* max number of emulated chips */

/* channel/chip definitions */
#define POKEY_CHAN1       0
#define POKEY_CHAN2       1
#define POKEY_CHAN3       2
#define POKEY_CHAN4       3
#define POKEY_CHIP1       0
#define POKEY_CHIP2       4
#define POKEY_CHIP3       8
#define POKEY_CHIP4      12
#define POKEY_SAMPLE    127

/* structures to hold the 9 pokey control bytes */
#define POKEY_AUDF   (Atari800_default->pokey.AUDF)	/* AUDFx (D200, D202, D204, D206) */
#define POKEY_AUDC   (Atari800_default->pokey.AUDC)	/* AUDCx (D201, D203, D205, D207) */
#define POKEY_AUDCTL (Atari800_default->pokey.AUDCTL)		/* AUDCTL (D208) */

#define POKEY_DivNIRQ (Atari800_default->pokey.DivNIRQ)
#define POKEY_DivNMax (Atari800_default->pokey.DivNMax)
#define POKEY_Base_mult (Atari800_default->pokey.Base_mult)	/* selects either 64Khz or 15Khz clock mult */

extern UBYTE POKEY_poly9_lookup[POKEY_POLY9_SIZE];
extern UBYTE POKEY_poly17_lookup[16385];

#endif /* POKEY_H_ */
