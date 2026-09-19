#ifndef LIBATARI800_STATESAV_H_
#define LIBATARI800_STATESAV_H_

#include <stdio.h>
#include <stdint.h>

#include "config.h"
#include "atari.h"
#include "instance.h" /* Atari800_Instance, Atari800_default */
#include "../statesav.h"
#include "libatari800/libatari800.h"

/* The in-memory state buffer and the tag table are per-instance
   (Atari800_Instance.libatari800.*); the legacy names are aliased to the
   default instance (transitional). */
#define LIBATARI800_StateSav_buffer (Atari800_default->libatari800.statesav_buffer)
#define LIBATARI800_StateSav_tags   (Atari800_default->libatari800.statesav_tags)

/* Save/read the given instance's state into/from the caller-supplied
   in-memory buffer. */
void LIBATARI800_StateSave_Ctx(Atari800_Instance *inst, UBYTE *buffer, statesav_tags_t *tags);
void LIBATARI800_StateLoad_Ctx(Atari800_Instance *inst, UBYTE *buffer);

#define LIBATARI800_StateSave(buffer, tags) \
	LIBATARI800_StateSave_Ctx(Atari800_default, (buffer), (tags))
#define LIBATARI800_StateLoad(buffer) \
	LIBATARI800_StateLoad_Ctx(Atari800_default, (buffer))

#endif /* LIBATARI800_STATESAV_H_ */
