#ifndef IDE_H_
#define IDE_H_

#include "atari.h"
#include "instance.h" /* Atari800_Instance, Atari800_default (transitional) */
#ifdef HAVE_INTTYPES_H
#  include <inttypes.h>
#else
   typedef signed char  int8_t;
   typedef unsigned char  uint8_t;
   typedef short int16_t;
   typedef unsigned short uint16_t;
   typedef int  int32_t;
   typedef unsigned uint32_t;
   typedef long long int64_t;
   typedef unsigned long long uint64_t;
#endif

/* Transitional Option C bridge: the per-instance IDE state lives in
   IDE_state_t (instance.h). The legacy global names are aliased to the
   default instance for not-yet-migrated callers. */
#define IDE_enabled (Atari800_default->ide.enabled)
#define IDE_debug   (Atari800_default->ide.debug)

/* Context-aware entry points (Option C); the legacy names below are
   forwarding macros that pass the default instance. */
int IDE_Initialise_Ctx(Atari800_Instance *inst, int *argc, char *argv[]);
void IDE_Exit_Ctx(Atari800_Instance *inst);
uint8_t IDE_GetByte_Ctx(Atari800_Instance *inst, uint16_t addr, int no_side_effects);
void    IDE_PutByte_Ctx(Atari800_Instance *inst, uint16_t addr, uint8_t byte);

#define IDE_Initialise(argc, argv) \
	IDE_Initialise_Ctx(Atari800_default, argc, argv)
#define IDE_Exit() \
	IDE_Exit_Ctx(Atari800_default)
#define IDE_GetByte(addr, no_side_effects) \
	IDE_GetByte_Ctx(Atari800_default, addr, no_side_effects)
#define IDE_PutByte(addr, byte) \
	IDE_PutByte_Ctx(Atari800_default, addr, byte)

#endif
