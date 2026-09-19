#ifndef STATESAV_H_
#define STATESAV_H_

#include "config.h"
#include "atari.h"
#include "instance.h" /* Atari800_Instance (transitional default-instance aliases) */

/* Option C refactor (docs/refactor-checklist.md §4.5): the state save/read
   stream is per-instance (Statesav_state_t, embedded in Atari800_Instance).
   Each public function takes the instance whose stream it operates on; the
   legacy un-suffixed names below are forwarding macros that route to
   Atari800_default, so not-yet-migrated callers keep working unchanged. */

int StateSav_SaveAtariState_Ctx(Atari800_Instance *inst, const char *filename, const char *mode, UBYTE SaveVerbose);
int StateSav_ReadAtariState_Ctx(Atari800_Instance *inst, const char *filename, const char *mode);
#define StateSav_SaveAtariState(filename, mode, SaveVerbose) \
	StateSav_SaveAtariState_Ctx(Atari800_default, (filename), (mode), (SaveVerbose))
#define StateSav_ReadAtariState(filename, mode) \
	StateSav_ReadAtariState_Ctx(Atari800_default, (filename), (mode))

void StateSav_SaveUBYTE_Ctx(Atari800_Instance *inst, const UBYTE *data, int num);
void StateSav_SaveUWORD_Ctx(Atari800_Instance *inst, const UWORD *data, int num);
void StateSav_SaveINT_Ctx(Atari800_Instance *inst, const int *data, int num);
void StateSav_SaveFNAME_Ctx(Atari800_Instance *inst, const char *filename);

void StateSav_ReadUBYTE_Ctx(Atari800_Instance *inst, UBYTE *data, int num);
void StateSav_ReadUWORD_Ctx(Atari800_Instance *inst, UWORD *data, int num);
void StateSav_ReadINT_Ctx(Atari800_Instance *inst, int *data, int num);
void StateSav_ReadFNAME_Ctx(Atari800_Instance *inst, char *filename);

#define StateSav_SaveUBYTE(data, num)   StateSav_SaveUBYTE_Ctx(Atari800_default, (data), (num))
#define StateSav_SaveUWORD(data, num)   StateSav_SaveUWORD_Ctx(Atari800_default, (data), (num))
#define StateSav_SaveINT(data, num)     StateSav_SaveINT_Ctx(Atari800_default, (data), (num))
#define StateSav_SaveFNAME(filename)    StateSav_SaveFNAME_Ctx(Atari800_default, (filename))

#define StateSav_ReadUBYTE(data, num)   StateSav_ReadUBYTE_Ctx(Atari800_default, (data), (num))
#define StateSav_ReadUWORD(data, num)   StateSav_ReadUWORD_Ctx(Atari800_default, (data), (num))
#define StateSav_ReadINT(data, num)     StateSav_ReadINT_Ctx(Atari800_default, (data), (num))
#define StateSav_ReadFNAME(filename)    StateSav_ReadFNAME_Ctx(Atari800_default, (filename))

#ifdef LIBATARI800
ULONG StateSav_Tell(void);
#include "libatari800/statesav.h"
/* STATESAV_MAX_SIZE defined in libatari800 include file */
/* The tag table is per-instance (inst->libatari800.statesav_tags); every
   use site is inside a *_Ctx function with `inst` in scope. */
#define STATESAV_TAG(a) do { if (inst->libatari800.statesav_tags) inst->libatari800.statesav_tags->a = StateSav_Tell(); } while (0)
#else /* LIBATARI800 */
#define STATESAV_MAX_SIZE 210000 /* max size of state save data */
#define STATESAV_TAG(a)
#endif /* LIBATARI800 */

#endif /* STATESAV_H_ */
