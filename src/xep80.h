#ifndef XEP80_H_
#define XEP80_H_

#include "config.h"
#include "atari.h"
#include "instance.h" /* XEP80 geometry defines + Atari800_Instance
                         (transitional default-instance aliases) */

/* XEP80 geometry defines now live in instance.h (used by XEP80_state_t). */

enum {
	XEP80_CHAR_HEIGHT_NTSC = 10,
	XEP80_CHAR_HEIGHT_PAL = 12
};

#define XEP80_ATARI_EOL			0x9b

/* Transitional Option C bridge: the per-instance XEP80 state lives in
   XEP80_state_t (instance.h). Until all callers pass an instance
   explicitly, the legacy global names are aliased to the default instance. */
#define XEP80_enabled     (Atari800_default->xep80.enabled)
#define XEP80_port        (Atari800_default->xep80.port)
#define XEP80_scrn_height (Atari800_default->xep80.scrn_height)
#define XEP80_char_height (Atari800_default->xep80.char_height)
#define XEP80_screen_1    (Atari800_default->xep80.screen_1)
#define XEP80_screen_2    (Atari800_default->xep80.screen_2)

/* Context-aware entry points (Option C). The legacy names below are
   forwarding macros that pass the default instance, so not-yet-migrated
   callers are unchanged. */
int XEP80_SetEnabled_Ctx(Atari800_Instance *inst, int value);
UBYTE XEP80_GetBit_Ctx(Atari800_Instance *inst);
void XEP80_PutBit_Ctx(Atari800_Instance *inst, UBYTE byte);
void XEP80_ChangeColors_Ctx(Atari800_Instance *inst);
void XEP80_StateSave_Ctx(Atari800_Instance *inst);
void XEP80_StateRead_Ctx(Atari800_Instance *inst);
int XEP80_ReadConfig_Ctx(Atari800_Instance *inst, char *string, char *ptr);
void XEP80_WriteConfig_Ctx(Atari800_Instance *inst, FILE *fp);
int XEP80_Initialise_Ctx(Atari800_Instance *inst, int *argc, char *argv[]);

#define XEP80_SetEnabled(value)        XEP80_SetEnabled_Ctx(Atari800_default, value)
#define XEP80_GetBit()                 XEP80_GetBit_Ctx(Atari800_default)
#define XEP80_PutBit(byte)             XEP80_PutBit_Ctx(Atari800_default, byte)
#define XEP80_ChangeColors()           XEP80_ChangeColors_Ctx(Atari800_default)
#define XEP80_StateSave()              XEP80_StateSave_Ctx(Atari800_default)
#define XEP80_StateRead()              XEP80_StateRead_Ctx(Atari800_default)
#define XEP80_ReadConfig(str, ptr)     XEP80_ReadConfig_Ctx(Atari800_default, str, ptr)
#define XEP80_WriteConfig(fp)          XEP80_WriteConfig_Ctx(Atari800_default, fp)
#define XEP80_Initialise(argc, argv)   XEP80_Initialise_Ctx(Atari800_default, argc, argv)

#endif /* XEP80_H_ */
