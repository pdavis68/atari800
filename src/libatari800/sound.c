/*
 * libatari800/sound.c - Atari800 as a library - sound output
 *
 * Copyright (c) 2001-2002 Jacek Poplawski
 * Copyright (C) 2001-2013 Atari800 development team (see DOC/CREDITS)
 * Copyright (c) 2016-2019 Rob McMullen
 *
 * This file is part of the Atari800 emulator project which emulates
 * the Atari 400, 800, 800XL, 130XE, and 5200 8-bit computers.
 *
 * Atari800 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Atari800 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Atari800; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "atari.h"
#include "log.h"
#include "platform.h"
#include "init.h"
#include "sound.h"
#include "util.h"
#include "libatari800/main.h"

/* The sound buffer state (LIBATARI800_Sound_array, sound_array_fill,
   sound_hw_buffer_size, sample_diff, sample_residual) is per-instance
   (Atari800_Instance.libatari800.*, see instance.h). Within this file the
   legacy names are re-pointed to the current instance's fields, since the
   PLATFORM_Sound* callbacks below are invoked for whichever instance is
   currently being driven (pinned by libatari800_next_frame_Ctx). */

int PLATFORM_SoundSetup(Sound_setup_t *setup)
{
	Atari800_Instance *LS = LIBATARI800_CurrentInstance();
	double refresh_rate;
	double samples_per_video_frame;

#undef LIBATARI800_Sound_array
#undef sound_hw_buffer_size
#undef sample_diff
#undef sample_residual
#define LIBATARI800_Sound_array  (LS->libatari800.sound_array)
#define sound_hw_buffer_size     (LS->libatari800.sound_hw_buffer_size)
#define sample_diff              (LS->libatari800.sample_diff)
#define sample_residual          (LS->libatari800.sample_residual)

	refresh_rate = Atari800_tv_mode == Atari800_TV_PAL ? Atari800_FPS_PAL : Atari800_FPS_NTSC;
	samples_per_video_frame = setup->freq / refresh_rate;
	setup->buffer_frames = (int)(ceil(samples_per_video_frame));

	sound_hw_buffer_size = setup->buffer_frames * setup->sample_size * setup->channels;
	if (sound_hw_buffer_size == 0)
	        return FALSE;

	LIBATARI800_Sound_array = Util_malloc(sound_hw_buffer_size);

	sample_diff = (double)setup->buffer_frames - samples_per_video_frame;
	sample_residual = 0;

#undef LIBATARI800_Sound_array
#undef sound_hw_buffer_size
#undef sample_diff
#undef sample_residual
#define LIBATARI800_Sound_array  (Atari800_default->libatari800.sound_array)
#define sound_hw_buffer_size     (Atari800_default->libatari800.sound_hw_buffer_size)
#define sample_diff              (Atari800_default->libatari800.sample_diff)
#define sample_residual          (Atari800_default->libatari800.sample_residual)

	return TRUE;
}

void PLATFORM_SoundExit(void)
{
	Atari800_Instance *LS = LIBATARI800_CurrentInstance();

	free(LS->libatari800.sound_array);
	LS->libatari800.sound_array = NULL;
}

void PLATFORM_SoundPause(void)
{
}

void PLATFORM_SoundContinue(void)
{
}

/* Called just before audio buffer is filled; used to initialize sound parameters */
unsigned int PLATFORM_SoundAvailable(void)
{
	Atari800_Instance *LS = LIBATARI800_CurrentInstance();
	int buf_size;

#undef sound_array_fill
#undef sound_hw_buffer_size
#define sound_array_fill     (LS->libatari800.sound_array_fill)
#define sound_hw_buffer_size (LS->libatari800.sound_hw_buffer_size)

	buf_size = sound_hw_buffer_size;

#undef sample_diff
#undef sample_residual
#define sample_diff     (LS->libatari800.sample_diff)
#define sample_residual (LS->libatari800.sample_residual)

	/* Because the frame rate is not an integer (59.92 NTSC, 49.86 PAL), the sample
	   rate will not be constant. For example, on NTSC with a sample rate of 44100Hz,
	   there should be 735.9476... samples per second. But, obviously there can't be
	   a fraction of a sample, so sample sizes will mostly be 736 samples with the
	   occasional 735 thrown in to make the rate average to 44100Hz. The sample_residual
	   keeps track of the fractional samples to see when we must shorten the sample
	   buffer. */
	sample_residual += sample_diff;
	if (sample_residual > 1.0) {
		sample_residual -= 1.0;
		buf_size -= Sound_out.sample_size * Sound_out.channels;
	}

	sound_array_fill = 0;

#undef sound_array_fill
#undef sound_hw_buffer_size
#define sound_array_fill     (Atari800_default->libatari800.sound_array_fill)
#define sound_hw_buffer_size (Atari800_default->libatari800.sound_hw_buffer_size)

#undef sample_diff
#undef sample_residual
#define sample_diff     (Atari800_default->libatari800.sample_diff)
#define sample_residual (Atari800_default->libatari800.sample_residual)

	return buf_size;
}

void PLATFORM_SoundWrite(UBYTE const *buffer, unsigned int size)
{
	Atari800_Instance *LS = LIBATARI800_CurrentInstance();

	memcpy(LS->libatari800.sound_array, buffer, size);
#undef sound_array_fill
#define sound_array_fill (LS->libatari800.sound_array_fill)
	sound_array_fill = size;
#undef sound_array_fill
#define sound_array_fill (Atari800_default->libatari800.sound_array_fill)
}

/*
vim:ts=4:sw=4:
*/
