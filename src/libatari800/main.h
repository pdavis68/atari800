#ifndef LIBATARI800_MAIN_H_
#define LIBATARI800_MAIN_H_

#include <stdio.h>

#include "config.h"
#include "instance.h" /* Atari800_Instance, Atari800_default */

/* Advance the given instance by one video frame (previously
   LIBATARI800_Frame operating on file-scope globals). */
void LIBATARI800_Frame_Ctx(Atari800_Instance *inst);
#define LIBATARI800_Frame() LIBATARI800_Frame_Ctx(Atari800_default)

/* Pin the instance whose state the libatari800 PLATFORM_* callbacks
   (keyboard/joystick/triggers/sound) operate on. Called by
   libatari800_next_frame_Ctx / LIBATARI800_Frame_Ctx; defaults to
   Atari800_default. Implemented in libatari800/input.c. */
void LIBATARI800_SetCurrentInstance(Atari800_Instance *inst);
Atari800_Instance *LIBATARI800_CurrentInstance(void);

#endif /* LIBATARI800_MAIN_H_ */
