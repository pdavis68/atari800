#ifndef LIBATARI800_SOUND_H_
#define LIBATARI800_SOUND_H_

#include <stdio.h>

#include "atari.h"
#include "instance.h" /* Atari800_Instance, Atari800_default */

/* The sound output buffer and fill level are per-instance
   (Atari800_Instance.libatari800.*); the legacy names are aliased to the
   default instance (transitional). */
#define LIBATARI800_Sound_array  (Atari800_default->libatari800.sound_array)

#define sound_array_fill     (Atari800_default->libatari800.sound_array_fill)

#define sound_hw_buffer_size (Atari800_default->libatari800.sound_hw_buffer_size)

#define sample_residual      (Atari800_default->libatari800.sample_residual)

#endif /* LIBATARI800_SOUND_H_ */
