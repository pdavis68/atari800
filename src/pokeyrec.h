#ifndef POKEYREC_H_
#define POKEYREC_H_

#include "atari.h"
#include "instance.h" /* Atari800_Instance, Atari800_default (transitional) */

/* Transitional Option C bridge: the per-instance Pokeyrec state lives in
   Pokeyrec_state_t (instance.h). The legacy names below are forwarding
   macros that pass the default instance, so not-yet-migrated callers are
   unchanged. */
void POKEYREC_Recorder_Ctx(Atari800_Instance *inst);
int  POKEYREC_Initialise_Ctx(Atari800_Instance *inst, int *argc, char *argv[]);
void POKEYREC_Exit_Ctx(Atari800_Instance *inst);

#define POKEYREC_Recorder() \
	POKEYREC_Recorder_Ctx(Atari800_default)
#define POKEYREC_Initialise(argc, argv) \
	POKEYREC_Initialise_Ctx(Atari800_default, argc, argv)
#define POKEYREC_Exit() \
	POKEYREC_Exit_Ctx(Atari800_default)

#endif
