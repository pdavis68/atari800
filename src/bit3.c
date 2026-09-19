/*
 * bit3.c - Emulation of the Bit3 Full View 80 column card.
 *
 * Copyright (C) 2009 Perry McFarlane
 * Copyright (C) 2009 Atari800 development team (see DOC/CREDITS)
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

#include "bit3.h"
#include "atari.h"
#include "util.h"
#include "log.h"
#include "memory.h"
#include "cpu.h"
#include "videomode.h"
#include <stdlib.h>

/* Transitional Option C bridge: the per-instance BIT3 state lives in
   BIT3_state_t (instance.h). The *_Ctx entry points pin the file-scope
   context (B3 = &inst->bit3, B3i = inst); the _Ctx bodies operate on
   their own instance. BIT3_palette lives in BIT3_state_t.palette
   (initialised in the default-instance initializer in atari.c);
   VIDEOMODE_80_column / VIDEOMODE_Set80Column remain process-global
   until the videomode module is converted (Phase 4, transitional). */

static Atari800_Instance *B3i;
static BIT3_state_t *B3;

#define BIT3_PIN_CTX(inst) do { \
	B3i = (inst); \
	B3 = &(inst)->bit3; \
} while (0)

/* The display palette lives in BIT3_state_t.palette (instance.h). */
#undef BIT3_palette
#define BIT3_palette (B3->palette)

#ifdef BIT3_DEBUG
#define D(a) a
#else
#define D(a) do{}while(0)
#endif

static void update_d6(void)
{
	memcpy(B3i->memory.mem + 0xd600, B3->rom + (B3->rom_bank_select<<8), 0x100);
}

int BIT3_Initialise_Ctx(Atari800_Instance *inst, int *argc, char *argv[])
{
	int i, j;
	int help_only = FALSE;
	BIT3_PIN_CTX(inst);
	for (i = j = 1; i < *argc; i++) {
		if (strcmp(argv[i], "-bit3") == 0) {
			B3->enabled = TRUE;
		}
		else {
		 	if (strcmp(argv[i], "-help") == 0) {
		 		help_only = TRUE;
				Log_print("\t-bit3            Emulate the Bit3 Full View 80 column board");
			}
			argv[j++] = argv[i];
		}
	}
	*argc = j;

	if (help_only)
		return TRUE;

	if (B3->enabled) {
		Log_print("Bit 3 Full View enabled");
		B3->rom = (UBYTE *)Util_malloc(0x1000);
		if (!Atari800_LoadImage(B3->rom_filename, B3->rom, 0x1000)) {
			free(B3->rom);
			B3->rom = NULL;
			B3->enabled = FALSE;
			Log_print("Couldn't load Bit3 Full View ROM image");
			return FALSE;
		}
		else {
			Log_print("loaded Bit3 Full View ROM image");
		}
		B3->charset = (UBYTE *)Util_malloc(0x1000);
		if (!Atari800_LoadImage(B3->charset_filename, B3->charset, 0x1000)) {
			free(B3->charset);
			free(B3->rom);
			B3->charset = B3->rom = NULL;
			B3->enabled = FALSE;
			Log_print("Couldn't load Bit3 Full View charset image");
			return FALSE;
		}
		else {
			Log_print("loaded Bit3 Full View charset image");
		}
		B3->screen = (UBYTE *)Util_malloc(0x800);
		VIDEOMODE_80_column = 0; /* Disable 80 column mode if set in .cfg, Bit3 uses software control for this */
		BIT3_Reset_Ctx(inst); /* With VIDEOMODE_80_column = 0, VIDEOMODE_Set80Column(0) will not change modes */

	}

	return TRUE;
}

void BIT3_Exit_Ctx(Atari800_Instance *inst)
{
	BIT3_PIN_CTX(inst);
	free(B3->screen);
	free(B3->charset);
	free(B3->rom);
	B3->screen = B3->charset = B3->rom = NULL;
}

void BIT3_InsertRightCartridge_Ctx(Atari800_Instance *inst)
{
	/* Reserved: the Bit3 board is enabled via D5 registers, not a cart
	   ROM; the legacy declaration in bit3.h had no definition. */
}

int BIT3_ReadConfig_Ctx(Atari800_Instance *inst, char *string, char *ptr)
{
	BIT3_PIN_CTX(inst);
	if (strcmp(string, "BIT3_ROM") == 0)
		Util_strlcpy(B3->rom_filename, ptr, sizeof(B3->rom_filename));
	else if (strcmp(string, "BIT3_CHARSET") == 0)
		Util_strlcpy(B3->charset_filename, ptr, sizeof(B3->charset_filename));
	else return FALSE; /* no match */
	return TRUE; /* matched something */
}

void BIT3_WriteConfig_Ctx(Atari800_Instance *inst, FILE *fp)
{
	BIT3_PIN_CTX(inst);
	fprintf(fp, "BIT3_ROM=%s\n", B3->rom_filename);
	fprintf(fp, "BIT3_CHARSET=%s\n", B3->charset_filename);
}

int BIT3_D6GetByte_Ctx(Atari800_Instance *inst, UWORD addr, int no_side_effects)
{
	int result;
	BIT3_PIN_CTX(inst);
	result = B3i->memory.mem[addr];
	return result;
}

void BIT3_D6PutByte_Ctx(Atari800_Instance *inst, UWORD addr, UBYTE byte)
{
	return;
}

int BIT3_D5GetByte_Ctx(Atari800_Instance *inst, UWORD addr, int no_side_effects)
{
	int result=0xff;
	BIT3_PIN_CTX(inst);
	if (addr == 0xd508) {
	}
	else if (addr == 0xd580) {
		/* crtc status TODO */
	}
	else if (addr == 0xd581) {
		result = B3->crtreg[B3->crtreg[0x00]&0x3f];
	}
	else if (addr == 0xd583 || addr == 0xd585) {
		/* d583 is used for reading screen ram, d585 for writing, in the ROM.
		 * This code supports both since the manual only mentions using
		 * d583 for read/write */
		result = B3->screen[(((B3->crtreg[0x12]&0x07)<<8)|B3->crtreg[0x13])];
		if(B3->crtreg[0x13] == 0) {
			B3->crtreg[0x12] = ((B3->crtreg[0x12]+1)&0x3f);
		}
	}
	else {
   		result = B3i->memory.mem[addr];
	}
	return result;
}

void BIT3_D5PutByte_Ctx(Atari800_Instance *inst, UWORD addr, UBYTE byte)
{
	BIT3_PIN_CTX(inst);
	if (addr == 0xd508) {
		/* ROM bank bits 0-2 and bit 5 */
		/* The manual says bit 3 unblanks the 80x24 display and bit 4 turns the video switch to 80 x 24 (from 40 col)*/
			if (B3->rom_bank_select != (((byte & 0x20)>>2)|(byte & 0x07))) {
				B3->rom_bank_select = (((byte & 0x20)>>2)|(byte & 0x07));
				update_d6();
			};
			if (B3->video_latch != !!(byte & 0x10)){
				B3->video_latch = !!(byte & 0x10);
				VIDEOMODE_Set80Column(B3->video_latch);
			}
	}
	else if (addr == 0xd580) {
		/* select crtc register */
		B3->crtreg[0] = byte;
	}
	else if (addr == 0xd581) {
		/* write selected crtc register */
		B3->crtreg[B3->crtreg[0]&0x3f] = byte;
	}
	else if (addr == 0xd583 || addr == 0xd585) {
		/* d583 is used for reading screen ram, d585 for writing, in the ROM.
		 * This code supports both since the manual only mentions using
		 * d583 for read/write */
		B3->screen[(((B3->crtreg[0x12]&0x07)<<8)|B3->crtreg[0x13])] = byte;
		B3->crtreg[0x13]++;
		if(B3->crtreg[0x13] == 0) {
			B3->crtreg[0x12] = ((B3->crtreg[0x12]+1)&0x3f);
		}
	}
}

UBYTE BIT3_GetPixels_Ctx(Atari800_Instance *inst, int scanline, int column, int *colour, int blink)
{
#define BIT3_ROWS 24
#define BIT3_CELL_HEIGHT 10
	UBYTE character;
	UBYTE font_data;
	int table_start;
	int row;
	int line;
	int screen_pos;
	BIT3_PIN_CTX(inst);
	table_start = B3->crtreg[0x0d] + ((B3->crtreg[0x0c]&0x3f)<<8);
	row = scanline / BIT3_CELL_HEIGHT;
	line = scanline % BIT3_CELL_HEIGHT;

	if (row  >= BIT3_ROWS) {
		return 0;
	}
	screen_pos = ((row*80+column + table_start)&0x3fff);
	character = B3->screen[screen_pos&0x7ff];
	font_data = B3->charset[(character&0x7f)*16 + line];
	if (character & 0x80) {
		font_data ^= 0xff; /* invert */
	}
	if (screen_pos == (((B3->crtreg[0x0e]&0x3f)<<8)|B3->crtreg[0x0f]) && !blink) {
		if (line >= (B3->crtreg[0x0a]&0x1f) && line <= (B3->crtreg[0x0b]&0x1f)){
			if ((B3->crtreg[0x0a]&0x60) == 0x00 ||
			((B3->crtreg[0x0a]&0x60) == 0x40 && !blink) ||
			((B3->crtreg[0x0a]&0x60) == 0x60 && !blink)) {
					/* 0x00: no blinking */
					/* 0x20: no cursor */
					/* 0x40: blink at 1/16 field rate */
					/* 0x60: blink at 1/32 field rate TODO */
				font_data ^= 0xff; /* cursor */
			}
		}
	}
	*colour = 1; /* set number of palette entry for foreground pixels */
	return font_data;
}

void BIT3_Reset_Ctx(Atari800_Instance *inst)
{
	BIT3_PIN_CTX(inst);
	memset(B3->screen, 0, 0x800);
	B3->rom_bank_select = 0;
	memset(B3->crtreg, 0, sizeof(B3->crtreg));
	update_d6();
	B3->video_latch = 0;
	VIDEOMODE_Set80Column(B3->video_latch);
}

/*
vim:ts=4:sw=4:
*/
