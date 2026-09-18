/*
 * pbi_mio.c - ICD MIO board emulation
 *
 * Copyright (C) 2007-2008 Perry McFarlane
 * Copyright (C) 1998-2008 Atari800 development team (see DOC/CREDITS)
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

#include "atari.h"
#include "pbi_mio.h"
#include "util.h"
#include "log.h"
#include "memory.h"
#include "pia.h"
#include "pbi.h"
#include "cpu.h"
#include "stdlib.h"
#include "pbi_scsi.h"
#include "statesav.h"

#ifdef PBI_DEBUG
#define D(a) a
#else
#define D(a) do{}while(0)
#endif

/* Transitional Option C bridge: the per-instance MIO state lives in
   MIO_state_t (instance.h). The *_Ctx entry points pin the file-scope
   contexts (MIO = &inst->mio, MIOi = inst); inside this file the legacy
   state names route through MIO, so the _Ctx bodies operate on their own
   instance. The SCSI state is reached through the same instance. */
static Atari800_Instance *MIOi;
static MIO_state_t *MIO;

#define PBI_MIO_PIN_CTX(inst) do { \
	MIOi = (inst); \
	MIO = &MIOi->mio; \
} while (0)

/* Route the legacy state names through the pinned context. */
#undef PBI_SCSI_CD
#undef PBI_SCSI_MSG
#undef PBI_SCSI_IO
#undef PBI_SCSI_BSY
#undef PBI_SCSI_REQ
#undef PBI_SCSI_SEL
#undef PBI_SCSI_disk
#undef PBI_SCSI_GetByte
#undef PBI_SCSI_PutSEL
#undef PBI_SCSI_PutACK
#undef PBI_SCSI_PutByte
#undef PBI_MIO_enabled
#define PBI_MIO_enabled      (MIO->enabled)
#define mio_rom              (MIO->rom)
#define mio_rom_size         (MIO->rom_size)
#define mio_rom_bank         (MIO->rom_bank)
#define mio_rom_filename     (MIO->rom_filename)
#define mio_ram              (MIO->ram)
#define mio_ram_size         (MIO->ram_size)
#define mio_ram_bank_offset  (MIO->ram_bank_offset)
#define mio_ram_enabled      (MIO->ram_enabled)
#define mio_scsi_enabled     (MIO->scsi_enabled)
#define mio_scsi_disk_filename (MIO->scsi_disk_filename)
/* SCSI state and entry points of the same instance. */
#define PBI_SCSI_BSY  (MIOi->scsi.BSY)
#define PBI_SCSI_REQ  (MIOi->scsi.REQ)
#define PBI_SCSI_SEL  (MIOi->scsi.SEL)
#define PBI_SCSI_CD   (MIOi->scsi.CD)
#define PBI_SCSI_MSG  (MIOi->scsi.MSG)
#define PBI_SCSI_IO   (MIOi->scsi.IO)
#define PBI_SCSI_disk (MIOi->scsi.disk)
#define PBI_SCSI_GetByte()    PBI_SCSI_GetByte_Ctx(MIOi)
#define PBI_SCSI_PutSEL(sel)  PBI_SCSI_PutSEL_Ctx(MIOi, sel)
#define PBI_SCSI_PutACK(ack)  PBI_SCSI_PutACK_Ctx(MIOi, ack)
#define PBI_SCSI_PutByte(b)   PBI_SCSI_PutByte_Ctx(MIOi, b)

static void init_mio(void)
{
	free(mio_rom);
	mio_rom = (UBYTE *)Util_malloc(mio_rom_size);
	if (!Atari800_LoadImage(mio_rom_filename, mio_rom, mio_rom_size)) {
		free(mio_rom);
		mio_rom = NULL;
		return;
	}
	D(printf("Loaded mio rom image\n"));
	PBI_MIO_enabled = TRUE;
	if (PBI_SCSI_disk != NULL) fclose(PBI_SCSI_disk);
	if (!Util_filenamenotset(mio_scsi_disk_filename)) {
		PBI_SCSI_disk = fopen(mio_scsi_disk_filename, "rb+");
		if (PBI_SCSI_disk == NULL) {
			Log_print("Error opening SCSI disk image:%s", mio_scsi_disk_filename);
		}
		else {
			D(printf("Opened SCSI disk image\n"));
			mio_scsi_enabled = TRUE;
		}
	}
	if (!mio_scsi_enabled) {
		PBI_SCSI_BSY = TRUE; /* makes MIO give up easier */
	}
	free(mio_ram);
	mio_ram = (UBYTE *)Util_malloc(mio_ram_size);
	memset(mio_ram, 0, mio_ram_size);
}

int PBI_MIO_Initialise_Ctx(Atari800_Instance *inst, int *argc, char *argv[])
{
	int i, j;
	PBI_MIO_PIN_CTX(inst);
	for (i = j = 1; i < *argc; i++) {
		if (strcmp(argv[i], "-mio") == 0) {
			init_mio();
		}
		else {
		 	if (strcmp(argv[i], "-help") == 0) {
				Log_print("\t-mio             Emulate the ICD MIO board");
			}
			argv[j++] = argv[i];
		}
	}
	*argc = j;

	return TRUE;
}

void PBI_MIO_Exit_Ctx(Atari800_Instance *inst)
{
	PBI_MIO_PIN_CTX(inst);
	if (PBI_SCSI_disk != NULL) {
		fclose(PBI_SCSI_disk);
		PBI_SCSI_disk = NULL;
	}
	free(mio_ram);
	free(mio_rom);
	mio_rom = mio_ram = NULL;
}

int PBI_MIO_ReadConfig_Ctx(Atari800_Instance *inst, char *string, char *ptr)
{
	PBI_MIO_PIN_CTX(inst);
	if (strcmp(string, "MIO_ROM") == 0)
		Util_strlcpy(mio_rom_filename, ptr, sizeof(mio_rom_filename));
	else if (strcmp(string, "MIO_SCSI_DISK") == 0)
		Util_strlcpy(mio_scsi_disk_filename, ptr, sizeof(mio_scsi_disk_filename));
	else return FALSE; /* no match */
	return TRUE; /* matched something */
}

void PBI_MIO_WriteConfig_Ctx(Atari800_Instance *inst, FILE *fp)
{
	PBI_MIO_PIN_CTX(inst);
	fprintf(fp, "MIO_ROM=%s\n", mio_rom_filename);
	if (!Util_filenamenotset(mio_scsi_disk_filename)) {
		fprintf(fp, "MIO_SCSI_DISK=%s\n", mio_scsi_disk_filename);
	}
}

/* $D1xx */
UBYTE PBI_MIO_D1GetByte_Ctx(Atari800_Instance *inst, UWORD addr, int no_side_effects)
{
	UBYTE result = 0x00;/*ff*/;
	PBI_MIO_PIN_CTX(inst);
	addr &= 0xffe3; /* 7 mirrors */
	D(printf("MIO Read:%4x  PC:%4x\n", addr, CPU_remember_PC[(CPU_remember_PC_curpos-1)%CPU_REMEMBER_PC_STEPS]));
	if (addr == 0xd1e2) {
		result = ((!PBI_SCSI_CD) | (!PBI_SCSI_MSG<<1) | (!PBI_SCSI_IO<<2) | (!PBI_SCSI_BSY<<5) | (!PBI_SCSI_REQ<<7));
	}
	else if (addr == 0xd1e1) {
		if (mio_scsi_enabled) {
			result = PBI_SCSI_GetByte()^0xff;
			if (!no_side_effects) {
				PBI_SCSI_PutACK(1);
				PBI_SCSI_PutACK(0);
			}
		}
	}
	return result;
}

/* $D1xx */
void PBI_MIO_D1PutByte_Ctx(Atari800_Instance *inst, UWORD addr, UBYTE byte)
{
	int old_mio_ram_bank_offset;
	int old_mio_ram_enabled;
	int offset_changed;
	int ram_enabled_changed;
	PBI_MIO_PIN_CTX(inst);
	old_mio_ram_bank_offset = mio_ram_bank_offset;
	old_mio_ram_enabled = mio_ram_enabled;
	addr &= 0xffe3; /* 7 mirrors */
	if (addr == 0xd1e0) {
		/* ram bank A15-A8 */
		mio_ram_bank_offset &= 0xf0000;
		mio_ram_bank_offset |= (byte << 8);
	}
	else if (addr == 0xd1e1) {
		if (mio_scsi_enabled) {
			PBI_SCSI_PutByte(byte^0xff);
			PBI_SCSI_PutACK(1);
			PBI_SCSI_PutACK(0);
		}
	}
	else if (addr == 0xd1e2) {
		/* ram bank A19-A16, ram enable, other stuff */
		mio_ram_bank_offset &= 0x0ffff;
		mio_ram_bank_offset |= ( (byte & 0x0f) <<  16);
		mio_ram_enabled = (byte & 0x20);
		if (mio_scsi_enabled) PBI_SCSI_PutSEL(!!(byte & 0x10));
	}
	else if (addr == 0xd1e3) {
		/* or 0xd1ff. rom bank. */
		if (mio_rom_bank != byte){
			int offset = -1;
			if (byte == 4) offset = 0x2000;
			else if (byte == 8) offset = 0x2800;
			else if (byte == 0x10) offset = 0x3000;
			else if (byte == 0x20) offset = 0x3800;
			if (offset != -1) {
				memcpy(MEMORY_mem + 0xd800, mio_rom+offset, 0x800);
				D(printf("mio bank:%2x activated\n", byte));
			}else{
				memcpy(MEMORY_mem + 0xd800, MEMORY_os + 0x1800, 0x800);
				D(printf("Floating point rom activated\n"));

			}
			mio_rom_bank = byte;
		}

	}
	offset_changed = (old_mio_ram_bank_offset != mio_ram_bank_offset);
	ram_enabled_changed = (old_mio_ram_enabled != mio_ram_enabled);
	if (mio_ram_enabled && ram_enabled_changed) {
		/* Copy new page from buffer, overwrite ff page */
		memcpy(MEMORY_mem + 0xd600, mio_ram + mio_ram_bank_offset, 0x100);
	} else if (mio_ram_enabled && offset_changed) {
		/* Copy old page to buffer, copy new page from buffer */
		memcpy(mio_ram + old_mio_ram_bank_offset,MEMORY_mem + 0xd600, 0x100);
		memcpy(MEMORY_mem + 0xd600, mio_ram + mio_ram_bank_offset, 0x100);
	} else if (!mio_ram_enabled && ram_enabled_changed) {
		/* Copy old page to buffer, set new page to ff */
		memcpy(mio_ram + old_mio_ram_bank_offset, MEMORY_mem + 0xd600, 0x100);
		memset(MEMORY_mem + 0xd600, 0xff, 0x100);
	}
	D(printf("MIO Write addr:%4x byte:%2x, cpu:%4x\n", addr, byte,CPU_remember_PC[(CPU_remember_PC_curpos-1)%CPU_REMEMBER_PC_STEPS]));
}

/* MIO RAM page at D600-D6ff */
/* Possible to put code in this ram, so we can't avoid using MEMORY_mem[] */
/* because opcode fetch doesn't call this function */
UBYTE PBI_MIO_D6GetByte_Ctx(Atari800_Instance *inst, UWORD addr, int no_side_effects)
{
	PBI_MIO_PIN_CTX(inst);
	if (!mio_ram_enabled) return 0xff;
	return MEMORY_mem[addr];
}

/* $D6xx */
void PBI_MIO_D6PutByte_Ctx(Atari800_Instance *inst, UWORD addr, UBYTE byte)
{
	PBI_MIO_PIN_CTX(inst);
	if (!mio_ram_enabled) return;
	MEMORY_mem[addr]=byte;
}

#ifndef BASIC

void PBI_MIO_StateSave_Ctx(Atari800_Instance *inst)
{
	PBI_MIO_PIN_CTX(inst);
	StateSav_SaveINT_Ctx(inst, &PBI_MIO_enabled, 1);
	if (PBI_MIO_enabled) {
		StateSav_SaveFNAME_Ctx(inst, mio_scsi_disk_filename);
		StateSav_SaveFNAME_Ctx(inst, mio_rom_filename);
		StateSav_SaveINT_Ctx(inst, &mio_ram_size, 1);

		StateSav_SaveINT_Ctx(inst, &mio_ram_bank_offset, 1);
		StateSav_SaveUBYTE_Ctx(inst, mio_ram, mio_ram_size);
		StateSav_SaveUBYTE_Ctx(inst, &mio_rom_bank, 1);
		StateSav_SaveINT_Ctx(inst, &mio_ram_enabled, 1);
	}
}

void PBI_MIO_StateRead_Ctx(Atari800_Instance *inst)
{
	PBI_MIO_PIN_CTX(inst);
	StateSav_ReadINT_Ctx(inst, &PBI_MIO_enabled, 1);
	if (PBI_MIO_enabled) {
		StateSav_ReadFNAME_Ctx(inst, mio_scsi_disk_filename);
		StateSav_ReadFNAME_Ctx(inst, mio_rom_filename);
		StateSav_ReadINT_Ctx(inst, &mio_ram_size, 1);
		init_mio();
		StateSav_ReadINT_Ctx(inst, &mio_ram_bank_offset, 1);
		StateSav_ReadUBYTE_Ctx(inst, mio_ram, mio_ram_size);
		StateSav_ReadUBYTE_Ctx(inst, &mio_rom_bank, 1);
		StateSav_ReadINT_Ctx(inst, &mio_ram_enabled, 1);
	}
}

#endif /* #ifndef BASIC */

/*
vim:ts=4:sw=4:
*/
