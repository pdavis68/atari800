/*
 * pbi.c - Parallel bus emulation
 *
 * Copyright (C) 2002 Jason Duerstock <jason@cluephone.com>
 * Copyright (C) 2007-2008 Perry McFarlane
 * Copyright (C) 2002-2008 Atari800 development team (see DOC/CREDITS)
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
#include "pbi.h"
#include "memory.h"
#include "pia.h"
#include "cpu.h"
#include "log.h"
#include "util.h"
#include "statesav.h"
#include <stdlib.h>
#ifdef PBI_MIO
#include "pbi_mio.h"
#endif
#ifdef PBI_BB
#include "pbi_bb.h"
#endif
#ifdef PBI_XLD
#include "pbi_xld.h"
#endif
#ifdef PBI_PROTO80
#include "pbi_proto80.h"
#endif
#ifdef AF80
#include "af80.h"
#endif
#ifdef BIT3
#include "bit3.h"
#endif

/* Transitional Option C bridge: the per-instance PBI state lives in
   PBI_state_t (instance.h). The *_Ctx entry points pin the file-scope
   context (PB = &inst->pbi, PBIi = inst); inside this file the legacy
   state names route through PB, so the _Ctx bodies operate on their own
   instance. Calls into the not-yet-converted PBI sub-modules (MIO, BB,
   XLD, PROTO80, AF80, BIT3) still touch their own file-scope globals. */
static Atari800_Instance *PBIi;
static PBI_state_t *PB;

#define PBI_PIN_CTX(inst) do { \
	PBIi = (inst); \
	PB = &PBIi->pbi; \
} while (0)

/* Route the legacy state names through the pinned context. */
#undef PBI_IRQ
#undef PBI_D6D7ram
#define PBI_IRQ     (PB->IRQ)
#define PBI_D6D7ram (PB->D6D7ram)
#define D1FF_LATCH  (PB->D1FF_LATCH)

#ifdef PBI_DEBUG
#define D(a) a
#else
#define D(a) do{}while(0)
#endif

int PBI_Initialise_Ctx(Atari800_Instance *inst, int *argc, char *argv[])
{
	PBI_PIN_CTX(inst);
	return TRUE
#ifdef PBI_XLD
		&& PBI_XLD_Initialise(argc, argv)
#endif
#ifdef PBI_BB
		&& PBI_BB_Initialise(argc, argv)
#endif
#ifdef PBI_MIO
		&& PBI_MIO_Initialise(argc, argv)
#endif
#ifdef PBI_PROTO80
		&& PBI_PROTO80_Initialise(argc, argv)
#endif
	;
}

void PBI_Exit_Ctx(Atari800_Instance *inst)
{
	PBI_PIN_CTX(inst);
#ifdef PBI_PROTO80
	PBI_PROTO80_Exit();
#endif
#ifdef PBI_MIO
	PBI_MIO_Exit();
#endif
#ifdef PBI_BB
	PBI_BB_Exit();
#endif
#ifdef PBI_XLD
	PBI_XLD_Exit();
#endif
}

int PBI_ReadConfig_Ctx(Atari800_Instance *inst, char *string, char *ptr)
{
	PBI_PIN_CTX(inst);
	if (0) {
	}
#ifdef PBI_XLD
	else if (PBI_XLD_ReadConfig(string, ptr)) {
	}
#endif
#ifdef PBI_MIO
	else if (PBI_MIO_ReadConfig(string, ptr)) {
	}
#endif
#ifdef PBI_BB
	else if (PBI_BB_ReadConfig(string, ptr)) {
	}
#endif
#ifdef PBI_PROTO80
	else if (PBI_PROTO80_ReadConfig(string, ptr)) {
	}
#endif
	else return FALSE; /* no match */
	return TRUE; /* matched something */
}

void PBI_WriteConfig_Ctx(Atari800_Instance *inst, FILE *fp)
{
	PBI_PIN_CTX(inst);
#ifdef PBI_MIO
	PBI_MIO_WriteConfig(fp);
#endif
#ifdef PBI_BB
	PBI_BB_WriteConfig(fp);
#endif
#ifdef PBI_XLD
	PBI_XLD_WriteConfig(fp);
#endif
#ifdef PBI_PROTO80
	PBI_PROTO80_WriteConfig(fp);
#endif
}

void PBI_Reset_Ctx(Atari800_Instance *inst)
{
	/* Reset all PBI ROMs */
	PBI_D1PutByte_Ctx(inst, 0xd1ff, 0);
#ifdef PBI_XLD
	if (PBI_XLD_enabled) PBI_XLD_Reset();
#endif
	PBI_IRQ = 0;
}

UBYTE PBI_D1GetByte_Ctx(Atari800_Instance *inst, UWORD addr, int no_side_effects)
{
	int result = 0xff;
	PBI_PIN_CTX(inst);
	/* MIO and BB do not follow the spec, they take over the bus: */
#ifdef PBI_MIO
	if (PBI_MIO_enabled) return PBI_MIO_D1GetByte(addr, no_side_effects);
#endif
#ifdef PBI_BB
	if (PBI_BB_enabled) return PBI_BB_D1GetByte(addr, no_side_effects);
#endif
	/* Remaining PBI devices cooperate, following spec */
#ifdef PBI_XLD
	if (PBI_XLD_enabled && !no_side_effects) result = PBI_XLD_D1GetByte(addr);
#endif
#ifdef PBI_PROTO80
	if (result == PBI_NOT_HANDLED && PBI_PROTO80_enabled) result = PBI_PROTO80_D1GetByte(addr, no_side_effects);
#endif
	if(result != PBI_NOT_HANDLED) return (UBYTE)result;
	/* Each bit of D1FF is set by one of the 8 PBI devices to signal IRQ */
	/* The XLD devices have been combined into a single handler */
	if (addr == 0xd1ff) {
	/* D1FF IRQ status: */
		result = 0;
#ifdef PBI_XLD
		if (PBI_XLD_enabled && !no_side_effects) result |= PBI_XLD_D1ffGetByte();
#endif
		/* add more devices here... */
		return result;
	}
	/* addr was not handled: */
	D(printf("PBI_GetByte:%4x:%2x PC:%4x IRQ:%d\n",addr,result,CPU_regPC,CPU_IRQ));
	return result; /* 0xff */
}

void PBI_D1PutByte_Ctx(Atari800_Instance *inst, UWORD addr, UBYTE byte)
{
	int fp_active;
	PBI_PIN_CTX(inst);
	fp_active = PB->fp_active;
#ifdef PBI_MIO
	if (PBI_MIO_enabled) {
		PBI_MIO_D1PutByte(addr, byte);
		return;
	}
#endif
#ifdef PBI_BB
	if (PBI_BB_enabled) {
		PBI_BB_D1PutByte(addr, byte);
		return;
	}
#endif
	/* Remaining PBI devices cooperate, following spec */
	if (addr != 0xd1ff) {
		D(printf("PBI_PutByte:%4x <- %2x\n", addr, byte));
#ifdef PBI_XLD
		if (PBI_XLD_enabled) PBI_XLD_D1PutByte(addr, byte);
#endif
#ifdef PBI_PROTO_80
		if (PBI_PROTO80_enabled) PBI_PROTO80_D1PutByte(addr, byte);
#endif
		/* add more devices here... */
	}
	else if (addr == 0xd1ff) {
		/* D1FF pbi rom bank select */
		D(printf("D1FF write:%x\n", byte));
		if (D1FF_LATCH != byte) {
			/* if it's not valid, ignore it */
			if (byte != 0 && byte != 1 && byte != 2 && byte != 4 && byte != 8 && byte != 0x10 && byte !=0x20 && byte != 0x40 && byte != 0x80){
				D(printf("*****INVALID d1ff write:%2x********\n",byte));
				return;
			}
			/* otherwise, update the latch */
			D1FF_LATCH = byte;
#ifdef PBI_XLD
			if (PBI_XLD_enabled && PBI_XLD_D1ffPutByte(byte) != PBI_NOT_HANDLED) {
				/* handled */
				fp_active = FALSE;
				return;
			}
#endif
#ifdef PBI_PROTO80
			if (PBI_PROTO80_enabled && PBI_PROTO80_D1ffPutByte(byte) != PBI_NOT_HANDLED) {
				/* handled */
				fp_active = FALSE;
				return;
			}
#endif
		    /* add more devices here... */
			/* reactivate the floating point rom */
			if (!fp_active) {
				memcpy(MEMORY_mem + 0xd800, MEMORY_os + 0x1800, 0x800);
				D(printf("Floating point rom activated\n"));
				fp_active = TRUE;
			}
		}
	}
	PB->fp_active = fp_active;
}

/* $D6xx */
UBYTE PBI_D6GetByte_Ctx(Atari800_Instance *inst, UWORD addr, int no_side_effects)
{
	PBI_PIN_CTX(inst);
#ifdef AF80
	if (AF80_enabled) return AF80_D6GetByte(addr, no_side_effects);
#endif
#ifdef BIT3
	if (BIT3_enabled) return BIT3_D6GetByte(addr, no_side_effects);
#endif
#ifdef PBI_MIO
	if (PBI_MIO_enabled) return PBI_MIO_D6GetByte(addr, no_side_effects);
#endif
#ifdef PBI_BB
	if(PBI_BB_enabled) return PBI_BB_D6GetByte(addr, no_side_effects);
#endif
	/* XLD/1090 has ram here */
	if (PBI_D6D7ram) return MEMORY_mem[addr];
	else return 0xff;
}

/* $D6xx */
void PBI_D6PutByte_Ctx(Atari800_Instance *inst, UWORD addr, UBYTE byte)
{
	PBI_PIN_CTX(inst);
#ifdef AF80
	if (AF80_enabled) {
		AF80_D6PutByte(addr,byte);
		return;
	}
#endif
#ifdef BIT3
	if (BIT3_enabled) {
		BIT3_D6PutByte(addr,byte);
		return;
	}
#endif
#ifdef PBI_MIO
	if (PBI_MIO_enabled) {
		PBI_MIO_D6PutByte(addr,byte);
		return;
	}
#endif
#ifdef PBI_BB
	if(PBI_BB_enabled) {
		PBI_BB_D6PutByte(addr,byte);
		return;
	}
#endif
	/* XLD/1090 has ram here */
	if (PBI_D6D7ram) MEMORY_mem[addr]=byte;
}

/* read page $D7xx */
/* XLD/1090 has ram here */
UBYTE PBI_D7GetByte_Ctx(Atari800_Instance *inst, UWORD addr, int no_side_effects)
{
	PBI_PIN_CTX(inst);
	D(printf("PBI_D7GetByte:%4x\n",addr));
	if (PBI_D6D7ram) return MEMORY_mem[addr];
	else return 0xff;
}

/* write page $D7xx */
/* XLD/1090 has ram here */
void PBI_D7PutByte_Ctx(Atari800_Instance *inst, UWORD addr, UBYTE byte)
{
	PBI_PIN_CTX(inst);
	D(printf("PBI_D7PutByte:%4x <- %2x\n",addr,byte));
	if (PBI_D6D7ram) MEMORY_mem[addr]=byte;
}

#ifndef BASIC

void PBI_StateSave_Ctx(Atari800_Instance *inst)
{
	PBI_PIN_CTX(inst);
	StateSav_SaveUBYTE_Ctx(inst, &D1FF_LATCH, 1);
	StateSav_SaveINT_Ctx(inst, &PBI_D6D7ram, 1);
	StateSav_SaveINT_Ctx(inst, &PBI_IRQ, 1);
}

void PBI_StateRead_Ctx(Atari800_Instance *inst)
{
	PBI_PIN_CTX(inst);
	StateSav_ReadUBYTE_Ctx(inst, &D1FF_LATCH, 1);
	StateSav_ReadINT_Ctx(inst, &PBI_D6D7ram, 1);
	StateSav_ReadINT_Ctx(inst, &PBI_IRQ, 1);
}

#endif /* #ifndef BASIC */

/*
vim:ts=4:sw=4:
*/
