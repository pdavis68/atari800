#ifndef LIBATARI800_INPUT_H_
#define LIBATARI800_INPUT_H_

#include "atari.h"
#include "instance.h" /* Atari800_Instance, Atari800_default */
#include "libatari800/libatari800.h"

#define LIBATARI800_FLAG_DELTA_MOUSE 0
#define LIBATARI800_FLAG_DIRECT_MOUSE 1

/* The caller-supplied input template for the current frame is per-instance
   (Atari800_Instance.libatari800.input_array); the legacy name is aliased to
   the default instance (transitional). */
#define LIBATARI800_Input_array (Atari800_default->libatari800.input_array)

int LIBATARI800_Input_Initialise(int *argc, char *argv[]);

/* Fill the instance's emulated input registers from its input template. */
void LIBATARI800_Mouse_Ctx(Atari800_Instance *inst);
#define LIBATARI800_Mouse() LIBATARI800_Mouse_Ctx(Atari800_default)

#endif /* LIBATARI800_INPUT_H_ */
