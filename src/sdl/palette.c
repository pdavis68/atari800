/*
 * sdl/palette.c - SDL library specific port code - table of display palettes
 *
 * Copyright (c) 2010 Tomasz Krasuski
 * Copyright (C) 2010 Atari800 development team (see DOC/CREDITS)
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

#include <SDL.h>

#include "sdl/palette.h"
#include "af80.h"
#include "bit3.h"
#include "config.h"
#include "colours.h"
#include "instance.h"
#include "videomode.h"

/* The palette pointers are resolved at runtime instead of at static-init
   time: the palettes are per-instance state (Colours_state_t.table,
   AF80_state_t.palette, BIT3_state_t.palette), and a static initialiser
   cannot dereference Atari800_default. The pointer targets never change,
   so later updates to the palette contents are picked up automatically. */
SDL_PALETTE_tab_t SDL_PALETTE_tab[VIDEOMODE_MODE_SIZE];

void SDL_PALETTE_Initialise(void)
{
	SDL_PALETTE_tab_t *tab = SDL_PALETTE_tab;
	tab->palette = Atari800_default->colours.table;
	tab->size = 256; /* Standard display */
#if NTSC_FILTER
	++tab;
	tab->palette = Atari800_default->colours.table;
	tab->size = 256; /* NTSC filter */
#endif
#ifdef XEP80_EMULATION
	++tab;
	tab->palette = Atari800_default->colours.table;
	tab->size = 256; /* XEP80 also uses the standard palette */
#endif
#ifdef PBI_PROTO80
	++tab;
	tab->palette = Atari800_default->colours.table;
	tab->size = 256; /* So does PBI Proto80 */
#endif
#ifdef AF80
	++tab;
	tab->palette = Atari800_default->af80.palette;
	tab->size = 16; /* AF80 */
#endif
#ifdef BIT3
	++tab;
	tab->palette = Atari800_default->bit3.palette;
	tab->size = 2; /* BIT3 */
#endif
}

SDL_PALETTE_buffer_t SDL_PALETTE_buffer;

void SDL_PALETTE_Calculate32_A8R8G8B8(void *dest, int const *palette, int size)
{
	int i;
	for (i = 0; i < size; i++) {
		((Uint32 *)dest)[i] = 0xff000000 | palette[i];
	}
}
void SDL_PALETTE_Calculate32_B8G8R8A8(void *dest, int const *palette, int size)
{
	int i;
	for (i = 0; i < size; i++) {
		int rgb = palette[i];
		((Uint32 *)dest)[i] = 0xff | ((rgb & 0x00ff0000) >> 8) | ((rgb & 0x0000ff00) << 8) | ((rgb & 0x000000ff) << 24);
	}
}

void SDL_PALETTE_Calculate16_R5G6B5(void *dest, int const *palette, int size)
{
	int i;
	for (i = 0; i < size; i++) {
		int rgb = palette[i];
		((Uint16 *)dest)[i] = ((rgb & 0x00f80000) >> 8) | ((rgb & 0x0000fc00) >> 5) | ((rgb & 0x000000f8) >> 3);
	}
}

void SDL_PALETTE_Calculate16_B5G6R5(void *dest, int const *palette, int size)
{
	int i;
	for (i = 0; i < size; i++) {
		int rgb = palette[i];
		((Uint16 *)dest)[i] = ((rgb & 0x00f80000) >> 19) | ((rgb & 0x0000fc00) >> 5) | ((rgb & 0x000000f8) << 8);
	}
}
