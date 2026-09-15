#ifndef RTIME_H_
#define RTIME_H_
/* Emulate ICD R-Time 8 cartridge
   Copyright 2000 Jason Duerstock <jason@cluephone.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#include "atari.h"
#include "instance.h" /* Atari800_Instance (transitional default-instance aliases) */

/* Transitional Option C bridge: the per-instance RTIME state lives in
   RTIME_state_t (instance.h). Until all callers pass an instance
   explicitly, the legacy global names are aliased to the default instance. */
#define RTIME_enabled (Atari800_default->rtime.enabled)

/* Context-aware entry points (Option C). The legacy names below are
   forwarding macros that pass the default instance, so not-yet-migrated
   callers are unchanged. cartridge.c's _Ctx bodies pass their own
   instance. */
int RTIME_ReadConfig_Ctx(Atari800_Instance *inst, char *string, char *ptr);
void RTIME_WriteConfig_Ctx(Atari800_Instance *inst, FILE *fp);
int RTIME_Initialise_Ctx(Atari800_Instance *inst, int *argc, char *argv[]);
UBYTE RTIME_GetByte_Ctx(Atari800_Instance *inst);
void RTIME_PutByte_Ctx(Atari800_Instance *inst, UBYTE byte);

#define RTIME_ReadConfig(str, ptr) RTIME_ReadConfig_Ctx(Atari800_default, str, ptr)
#define RTIME_WriteConfig(fp)      RTIME_WriteConfig_Ctx(Atari800_default, fp)
#define RTIME_Initialise(argc, argv) RTIME_Initialise_Ctx(Atari800_default, argc, argv)
#define RTIME_GetByte()            RTIME_GetByte_Ctx(Atari800_default)
#define RTIME_PutByte(byte)        RTIME_PutByte_Ctx(Atari800_default, byte)

#endif /* RTIME_H_ */
