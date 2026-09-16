/*
 * af80.c - Emulation of the Austin Franklin 80 column card.
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

#include "af80.h"
#include "atari.h"
#include "util.h"
#include "log.h"
#include "memory.h"
#include "cpu.h"
#include <stdlib.h>

/* Transitional Option C bridge: the per-instance AF80 state lives in
   AF80_state_t (instance.h). The *_Ctx entry points pin the file-scope
   context (AF = &inst->af80, AFI = inst); the _Ctx bodies operate on
   their own instance. AF80_palette stays a real file-scope global: it
   is derived from the shared read-only RGBI table below and is
   referenced from static initialisers in sdl/palette.c. */

static Atari800_Instance *AFI;
static AF80_state_t *AF;

#define AF80_PIN_CTX(inst) do { \
	AFI = (inst); \
	AF = &(inst)->af80; \
} while (0)

/* Austin Franklin information from forum posts by warerat at Atariage */
static int const rgbi_palette[16] = {
	0x000000, /* black */
	0x0000AA, /* blue */
	0x00AA00, /* green */
	0x00AAAA, /* cyan */
	0xAA0000, /* red */
	0xAA00AA, /* magenta */
	0xAA5500, /* brown */
	0xAAAAAA, /* white */
	0x555555, /* grey */
	0x5555FF, /* light blue */
	0x55FF55, /* light green */
	0x55FFFF, /* light cyan */
	0xFF5555, /* light red */
	0xFF55FF, /* light magenta */
	0xFFFF55, /* yellow */
	0xFFFFFF  /* white (high intensity) */
};
int AF80_palette[16];

#ifdef AF80_DEBUG
#define D(a) a
#else
#define D(a) do{}while(0)
#endif

static void update_d6(void)
{
	if (!AF->not_enable_2k_character_ram) {
		memcpy(AFI->memory.mem + 0xd600, AF->screen + (AF->video_bank_select<<7), 0x80);
		memcpy(AFI->memory.mem + 0xd680, AF->screen + (AF->video_bank_select<<7), 0x80);
	}
	else if (!AF->not_enable_2k_attribute_ram) {
		memcpy(AFI->memory.mem + 0xd600, AF->attrib + (AF->video_bank_select<<7), 0x80);
		memcpy(AFI->memory.mem + 0xd680, AF->attrib + (AF->video_bank_select<<7), 0x80);
	}
	else if (AF->not_enable_crtc_registers) {
		memset(AFI->memory.mem + 0xd600, 0xff, 0x100);
	}
}

static void update_d5(void)
{
	if (AF->not_rom_output_enable) {
		memset(AFI->memory.mem + 0xd500, 0xff, 0x100);
	}
	else {
		memcpy(AFI->memory.mem + 0xd500, AF->rom + (AF->rom_bank_select<<8), 0x100);
	}
}

static void update_8000_9fff(void)
{
	if (AF->not_right_cartridge_rd4_control) return;
	if (AF->not_rom_output_enable) {
		memset(AFI->memory.mem + 0x8000, 0xff, 0x2000);
	}
	else {
		int i;
		for (i=0; i<32; i++) {
		memcpy(AFI->memory.mem + 0x8000 + (i<<8), AF->rom + (AF->rom_bank_select<<8), 0x100);
		}
	}
}

int AF80_Initialise_Ctx(Atari800_Instance *inst, int *argc, char *argv[])
{
	int i, j;
	int help_only = FALSE;
	AF80_PIN_CTX(inst);
	for (i = j = 1; i < *argc; i++) {
		if (strcmp(argv[i], "-af80") == 0) {
			AF->enabled = TRUE;
		}
		else {
		 	if (strcmp(argv[i], "-help") == 0) {
		 		help_only = TRUE;
				Log_print("\t-af80            Emulate the Austin Franklin 80 column board");
			}
			argv[j++] = argv[i];
		}
	}
	*argc = j;

	if (help_only)
		return TRUE;

	if (AF->enabled) {
		Log_print("Austin Franklin 80 enabled");
		AF->rom = (UBYTE *)Util_malloc(0x1000);
		if (!Atari800_LoadImage(AF->rom_filename, AF->rom, 0x1000)) {
			free(AF->rom);
			AF->rom = NULL;
			AF->enabled = FALSE;
			Log_print("Couldn't load Austin Franklin ROM image");
			return FALSE;
		}
		else {
			Log_print("loaded Austin Franklin rom image");
		}
		AF->charset = (UBYTE *)Util_malloc(0x1000);
		if (!Atari800_LoadImage(AF->charset_filename, AF->charset, 0x1000)) {
			free(AF->charset);
			free(AF->rom);
			AF->charset = AF->rom = NULL;
			AF->enabled = FALSE;
			Log_print("Couldn't load Austin Franklin charset image");
			return FALSE;
		}
		else {
			Log_print("loaded Austin Franklin charset image");
		}
		AF->screen = (UBYTE *)Util_malloc(0x800);
		AF->attrib = (UBYTE *)Util_malloc(0x800);
		AF80_Reset_Ctx(inst);

		/* swap palette */
		for (i=0; i<16; i++ ) {
			j=i;
			j = (j&0x0a) + ((j&0x01) << 2) + ((j&0x04) >> 2);
			AF80_palette[i] = rgbi_palette[j];
		}
	}

	return TRUE;
}

void AF80_Exit_Ctx(Atari800_Instance *inst)
{
	AF80_PIN_CTX(inst);
	free(AF->screen);
	free(AF->attrib);
	free(AF->charset);
	free(AF->rom);
	AF->screen = AF->attrib = AF->charset = AF->rom = NULL;
}

void AF80_InsertRightCartridge_Ctx(Atari800_Instance *inst)
{
		AF80_PIN_CTX(inst);
		MEMORY_Cart809fEnableCtx(inst);
		update_d5();
		update_8000_9fff();
}

int AF80_ReadConfig_Ctx(Atari800_Instance *inst, char *string, char *ptr)
{
	AF80_PIN_CTX(inst);
	if (strcmp(string, "AF80_ROM") == 0)
		Util_strlcpy(AF->rom_filename, ptr, sizeof(AF->rom_filename));
	else if (strcmp(string, "AF80_CHARSET") == 0)
		Util_strlcpy(AF->charset_filename, ptr, sizeof(AF->charset_filename));
	else return FALSE; /* no match */
	return TRUE; /* matched something */
}

void AF80_WriteConfig_Ctx(Atari800_Instance *inst, FILE *fp)
{
	AF80_PIN_CTX(inst);
	fprintf(fp, "AF80_ROM=%s\n", AF->rom_filename);
	fprintf(fp, "AF80_CHARSET=%s\n", AF->charset_filename);
}

int AF80_D6GetByte_Ctx(Atari800_Instance *inst, UWORD addr, int no_side_effects)
{
	int result = 0xff;
	AF80_PIN_CTX(inst);
	if (!AF->not_enable_2k_character_ram) {
		result = AFI->memory.mem[addr];
	}
	else if (!AF->not_enable_2k_attribute_ram) {
		result = AFI->memory.mem[addr];
	}
	else if (!AF->not_enable_crtc_registers) {
		if (AF->video_bank_select == 0 ) {
			if ((addr&0xff)<0x40) {
				result = AF->crtreg[addr&0xff];
				if ((addr&0xff) == 0x3a) {
					result = 0x01;
				}
			}
			D(printf("AF80 Read addr:%4x cpu:%4x\n", addr, CPU_remember_PC[(CPU_remember_PC_curpos-1)%CPU_REMEMBER_PC_STEPS]));
		}
	}
	return result;
}

void AF80_D6PutByte_Ctx(Atari800_Instance *inst, UWORD addr, UBYTE byte)
{
	AF80_PIN_CTX(inst);
	if (!AF->not_enable_2k_character_ram) {
		AFI->memory.mem[(addr&0xff7f)] = byte;
		AFI->memory.mem[(addr&0xff7f)+0x80] = byte;
		AF->screen[(addr&0x7f) + (AF->video_bank_select<<7)] = byte;
	}
	else if (!AF->not_enable_2k_attribute_ram) {
		AFI->memory.mem[(addr&0xff7f)] = byte;
		AFI->memory.mem[(addr&0xff7f)+0x80] = byte;
		AF->attrib[(addr&0x7f) + (AF->video_bank_select<<7)] = byte;
		D(printf("AF80 Write, attribute,  addr:%4x byte:%2x, cpu:%4x\n", addr, byte,CPU_remember_PC[(CPU_remember_PC_curpos-1)%CPU_REMEMBER_PC_STEPS]));
	}
	else if (!AF->not_enable_crtc_registers) {
		if (AF->video_bank_select == 0 ) {
			if ((addr&0xff)<0x40) {
				AF->crtreg[addr&0xff] = byte;
			}
			D(if (1 || (addr!=0xd618 && addr!=0xd619)) printf("AF80 Write addr:%4x byte:%2x, cpu:%4x\n", addr, byte,CPU_remember_PC[(CPU_remember_PC_curpos-1)%CPU_REMEMBER_PC_STEPS]));
		}
		else {
			D(printf("AF80 Write, video_bank_select!=0, addr:%4x byte:%2x, cpu:%4x\n", addr, byte,CPU_remember_PC[(CPU_remember_PC_curpos-1)%CPU_REMEMBER_PC_STEPS]));
		}
	}
}

int AF80_D5GetByte_Ctx(Atari800_Instance *inst, UWORD addr, int no_side_effects)
{
	int result;
	AF80_PIN_CTX(inst);
	result = AFI->memory.mem[addr];
	return result;
}

void AF80_D5PutByte_Ctx(Atari800_Instance *inst, UWORD addr, UBYTE byte)
{
	AF80_PIN_CTX(inst);
	if (addr == 0xd5f6) {
		int need_update_d6 = FALSE;
		if ((byte&0x10) != AF->not_enable_2k_character_ram) {
			AF->not_enable_2k_character_ram = (byte & 0x10);
			need_update_d6 = TRUE;
		}
		if ((byte&0x20) != AF->not_enable_2k_attribute_ram) {
			AF->not_enable_2k_attribute_ram = (byte & 0x20);
			need_update_d6 = TRUE;
		}
		if ((byte&0x40) != AF->not_enable_crtc_registers) {
			AF->not_enable_crtc_registers = (byte & 0x40);
			need_update_d6 = TRUE;
		}
		if ((byte&0x80) != AF->not_enable_80_column_output) {
			AF->not_enable_80_column_output = (byte & 0x80);
		}
		if ((byte&0x0f) != AF->video_bank_select) {
			AF->video_bank_select = (byte & 0x0f);
			need_update_d6 = TRUE;
		}
		if (need_update_d6) {
			update_d6();
		}
	}
	else if (addr == 0xd5f7) {
		int need_update_d5 = FALSE;
		int need_update_8000_9fff = FALSE;
		if ((byte&0x10) != AF->not_rom_output_enable) {
			AF->not_rom_output_enable = (byte & 0x10);
			need_update_d5 = TRUE;
			if (byte&0x20) {
				need_update_8000_9fff = TRUE;
			}
		}
		if ((byte&0x20) != AF->not_right_cartridge_rd4_control) {
			AF->not_right_cartridge_rd4_control = (byte & 0x20);
			if (AF->not_right_cartridge_rd4_control) {
				MEMORY_Cart809fDisableCtx(inst);
			}
			else {
				MEMORY_Cart809fEnableCtx(inst);
				need_update_8000_9fff = TRUE;
			}
		}
		if ((byte&0x0f) != AF->rom_bank_select) {
			AF->rom_bank_select = (byte & 0x0f);
			if (!AF->not_rom_output_enable) {
				need_update_d5 = TRUE;
				if (!AF->not_right_cartridge_rd4_control) {
					need_update_8000_9fff = TRUE;
				}
			}
		}
		if (need_update_d5) {
			update_d5();
		}
		if (need_update_8000_9fff) {
			update_8000_9fff();
		}
	}
	D(if (addr!=0xd5f7 && addr!=0xd5f6) printf("AF80 Write addr:%4x byte:%2x, cpu:%4x\n", addr, byte,CPU_remember_PC[(CPU_remember_PC_curpos-1)%CPU_REMEMBER_PC_STEPS]));
}

UBYTE AF80_GetPixels_Ctx(Atari800_Instance *inst, int scanline, int column, int *colour, int blink)
{
#define AF80_ROWS 25
#define AF80_CELL_HEIGHT 10
	UBYTE character;
	int attrib;
	UBYTE font_data;
	int table_start;
	int row;
	int line;
	int screen_pos;
	AF80_PIN_CTX(inst);
	table_start = AF->crtreg[0x0c] + ((AF->crtreg[0x0d]&0x3f)<<8);
	row = scanline / AF80_CELL_HEIGHT;
	line = scanline % AF80_CELL_HEIGHT;
	if (row  >= AF80_ROWS) {
		return 0;
	}

	if (row >= AF->crtreg[0x10]) {
		screen_pos = (row-AF->crtreg[0x10])*80 + column + AF->crtreg[0x0e] + ((AF->crtreg[0x0f]&0x3f)<<8);
	}
	else {
		screen_pos = row*80+column + table_start;
	}
	screen_pos &= 0x7ff;
	character = AF->screen[screen_pos];
	attrib = AF->attrib[screen_pos];
	font_data = AF->charset[character*16 + line];
	if (attrib & 0x01) {
	   	font_data ^= 0xff; /* invert */
	}
	if ((attrib & 0x02) && blink) {
	   	font_data = 0x00; /* blink */
	}
	if (line+1 == AF80_CELL_HEIGHT && (attrib & 0x04)) {
		font_data = 0xff; /* underline */
	}
	if (row == AF->crtreg[0x18] && column == AF->crtreg[0x19] && !blink) {
		font_data = 0xff; /* cursor */
	}
	*colour = attrib>>4; /* set number of palette entry */
	return font_data;
}

void AF80_Reset_Ctx(Atari800_Instance *inst)
{
	AF80_PIN_CTX(inst);
	memset(AF->screen, 0, 0x800);
	memset(AF->attrib, 0, 0x800);
	AF->rom_bank_select = 0;
	AF->not_rom_output_enable = 0;
	AF->not_right_cartridge_rd4_control = 0;
	AF->not_enable_2k_character_ram = 0;
	AF->not_enable_2k_attribute_ram = 0;
	AF->not_enable_crtc_registers = 0;
	AF->not_enable_80_column_output = 0;
	AF->video_bank_select = 0;
	memset(AF->crtreg, 0, sizeof(AF->crtreg));
}

/*
vim:ts=4:sw=4:
*/
