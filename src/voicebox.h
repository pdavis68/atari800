#ifndef VOICEBOX_H_
#define VOICEBOX_H_

#include "atari.h"
#include "instance.h" /* Atari800_Instance, Atari800_default (transitional) */

/* Transitional Option C bridge: the per-instance Voicebox state lives in
   Voicebox_state_t (instance.h). The legacy global names are aliased to the
   default instance for not-yet-migrated callers. */
#define VOICEBOX_enabled (Atari800_default->voicebox.enabled)
#define VOICEBOX_ii      (Atari800_default->voicebox.ii)

/* Context-aware entry points (Option C); the legacy names below are
   forwarding macros that pass the default instance. */
int VOICEBOX_Initialise_Ctx(Atari800_Instance *inst, int *argc, char *argv[]);
void VOICEBOX_SKCTLPutByte_Ctx(Atari800_Instance *inst, int byte);
void VOICEBOX_SEROUTPutByte_Ctx(Atari800_Instance *inst, int byte);

#define VOICEBOX_Initialise(argc, argv) \
	VOICEBOX_Initialise_Ctx(Atari800_default, argc, argv)
#define VOICEBOX_SKCTLPutByte(byte) \
	VOICEBOX_SKCTLPutByte_Ctx(Atari800_default, byte)
#define VOICEBOX_SEROUTPutByte(byte) \
	VOICEBOX_SEROUTPutByte_Ctx(Atari800_default, byte)

#define VOICEBOX_BASEAUDF 0xa0

#endif /* VOICEBOX_H_ */
