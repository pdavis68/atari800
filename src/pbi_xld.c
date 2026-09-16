/*
 * pbi_xld.c - 1450XLD and 1400XL emulation
 *
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
#include "votrax.h"
#include "pbi_xld.h"
#include "pbi.h"
#include "util.h"
#include "sio.h"
#include "log.h"
#include "pokey.h"
#include "cpu.h"
#include "memory.h"
#include "statesav.h"
#include "votraxsnd.h"
#include <stdlib.h>

/* Transitional Option C bridge: the per-instance XLD state lives in
   XLD_state_t (instance.h). The *_Ctx entry points pin the file-scope
   context (X = &inst->xld, Xi = inst); inside this file the legacy
   names are #undef'd and re-pointed to X, so the _Ctx bodies operate
   on their own instance. */
#undef PBI_XLD_enabled
#undef PBI_XLD_v_enabled

static Atari800_Instance *Xi;
static XLD_state_t *X;

#define PBI_XLD_PIN_CTX(inst) do { \
	Xi = (inst); \
	X = &(inst)->xld; \
} while (0)

#define PBI_XLD_enabled   (X->enabled)
#define PBI_XLD_v_enabled (X->v_enabled)

#define DISK_PBI_NUM 0
#define MODEM_PBI_NUM 1
#define VOICE_PBI_NUM 7

#define DISK_MASK (1 << DISK_PBI_NUM)
#define MODEM_MASK (1 << MODEM_PBI_NUM)
#define VOICE_MASK (1 << VOICE_PBI_NUM)

/* Parallel Disk I/O emulation support */
#define PIO_NoFrame         (0x00)
#define PIO_CommandFrame    (0x01)
#define PIO_StatusRead      (0x02)
#define PIO_ReadFrame       (0x03)
#define PIO_WriteFrame      (0x04)
#define PIO_FinalStatus     (0x05)
#define PIO_FormatFrame     (0x06)

static void PIO_PutByte(int byte);
static int PIO_GetByte(void);
static UBYTE PIO_Command_Frame(void);

#ifdef PBI_DEBUG
#define D(a) a
#else
#define D(a) do{}while(0)
#endif

static void init_xld_v(void)
{
	free(X->voicerom);
	X->voicerom = (UBYTE *)Util_malloc(0x1000);
	if (!Atari800_LoadImage(X->v_rom_filename, X->voicerom, 0x1000)) {
		free(X->voicerom);
		PBI_XLD_v_enabled = FALSE;
	}
	else {
		printf("loaded XLD voice rom image\n");
		Xi->pbi.D6D7ram = TRUE;
	}
}

static void init_xld_d(void)
{
	free(X->diskrom);
	X->diskrom = (UBYTE *)Util_malloc(0x800);
	if (!Atari800_LoadImage(X->d_rom_filename, X->diskrom, 0x800)) {
		free(X->diskrom);
		X->d_enabled = FALSE;
	}
	else {
		D(printf("loaded 1450XLD D: device driver rom image\n"));
		Xi->pbi.D6D7ram = TRUE;
	}
}

int PBI_XLD_Initialise_Ctx(Atari800_Instance *inst, int *argc, char *argv[])
{
	int i, j;
	PBI_XLD_PIN_CTX(inst);
	for (i = j = 1; i < *argc; i++) {
		if (strcmp(argv[i], "-1400") == 0) {
			PBI_XLD_v_enabled = TRUE;
			PBI_XLD_enabled = TRUE;
		}else if (strcmp(argv[i], "-xld") == 0){
			PBI_XLD_v_enabled = TRUE;
			X->d_enabled = TRUE;
			PBI_XLD_enabled = TRUE;
		}
		else {
		 	if (strcmp(argv[i], "-help") == 0) {
				Log_print("\t-1400            Emulate the Atari 1400XL");
				Log_print("\t-xld             Emulate the Atari 1450XLD");
			}

			argv[j++] = argv[i];
		}

	}
	*argc = j;

	if (PBI_XLD_v_enabled) {
		init_xld_v();
	}

	/* If you set the drive to empty in the UI, the message is displayed */
	/* If you press select, I believe it tries to slow the I/O down */
	/* in order to increase compatibility. */
	/* dskcnt6 works. dskcnt10 does not */
	if (X->d_enabled) {
		init_xld_d();
	}

	return TRUE;
}

void PBI_XLD_Exit_Ctx(Atari800_Instance *inst)
{
	PBI_XLD_PIN_CTX(inst);
	if (X->d_enabled) {
		free(X->diskrom);
		X->d_enabled = FALSE;
	}
	if (PBI_XLD_v_enabled) {
		free(X->voicerom);
		PBI_XLD_v_enabled = FALSE;
	}
	PBI_XLD_enabled = FALSE;
}

int PBI_XLD_ReadConfig_Ctx(Atari800_Instance *inst, char *string, char *ptr)
{
	PBI_XLD_PIN_CTX(inst);
	if (strcmp(string, "XLD_D_ROM") == 0)
		Util_strlcpy(X->d_rom_filename, ptr, sizeof(X->d_rom_filename));
	else if (strcmp(string, "XLD_V_ROM") == 0)
		Util_strlcpy(X->v_rom_filename, ptr, sizeof(X->v_rom_filename));
	else return FALSE; /* no match */
	return TRUE; /* matched something */
}

void PBI_XLD_WriteConfig_Ctx(Atari800_Instance *inst, FILE *fp)
{
	PBI_XLD_PIN_CTX(inst);
	fprintf(fp, "XLD_D_ROM=%s\n", X->d_rom_filename);
	fprintf(fp, "XLD_V_ROM=%s\n", X->v_rom_filename);
}

void PBI_XLD_Reset_Ctx(Atari800_Instance *inst)
{
	PBI_XLD_PIN_CTX(inst);
	X->votrax_latch = 0;
}

int PBI_XLD_D1GetByte_Ctx(Atari800_Instance *inst, UWORD addr)
{
	int result = PBI_NOT_HANDLED;
	PBI_XLD_PIN_CTX(inst);
	if (X->d_enabled && addr == 0xd114) {
	/* XLD input from disk to atari byte latch */
		result = (int)PIO_GetByte();
		D(printf("d114: disk read byte:%2x\n",result));
	}
	return result;
}


/* D1FF: each bit indicates IRQ status of a device */
UBYTE PBI_XLD_D1ffGetByte_Ctx(Atari800_Instance *inst)
{
	UBYTE result = 0;
	PBI_XLD_PIN_CTX(inst);
	/* VOTRAX BUSY IRQ bit */
	/*if (!votraxsc01_status_r()) {*/
	if (!VOTRAXSND_busy) {
		result |= VOICE_MASK;
	}
	return result;
}

void PBI_XLD_D1PutByte_Ctx(Atari800_Instance *inst, UWORD addr, UBYTE byte)
{
	PBI_XLD_PIN_CTX(inst);
	if ((addr & ~3) == 0xd104)  {
		/* XLD disk strobe line */
		D(printf("votrax write:%4x\n",addr));
		if (VOTRAXSND_busy) {
			PBI_XLD_votrax_busy_callback_Ctx(inst, TRUE); /* idle -> busy */
		}
		VOTRAXSND_PutByte(X->votrax_latch & 0x3f);

	}
	else if ((addr & ~3) == 0xd100 )  {
		/* votrax phoneme+irq-enable latch */
		if ( !(X->votrax_latch & 0x80) && (byte & 0x80) && (!Votrax_GetStatus())) {
			/* IRQ disabled -> enabled, and votrax idle: generate IRQ */
			D(printf("votrax IRQ generated: IRQ enable changed and idle\n"));
			CPU_GenerateIRQ();
			Xi->pbi.IRQ |= VOICE_MASK;
		} else if ((X->votrax_latch & 0x80) && !(byte & 0x80) ){
			/* IRQ enabled -> disabled : stop IRQ */
			Xi->pbi.IRQ &= ~VOICE_MASK;
			/* update pokey IRQ status */
			POKEY_PutByte(POKEY_OFFSET_IRQEN, POKEY_IRQEN);
		}
		X->votrax_latch = byte;
	}
	else if (addr == 0xd108) {
	/* modem latch and XLD 8040 T1 input */
		D(printf("XLD 8040 T1:%d loop-back:%d modem+phone:%d offhook(modem relay):%d phaudio:%d DTMF:%d O/!A(originate/answer):%d SQT(squelch transmitter):%d\n",!!(byte&0x80),!!(byte&0x40),!!(byte&0x20),!!(byte&0x10),!!(byte&0x08),!!(byte&0x04),!!(byte&0x02),!!(byte&0x01)));
		X->modem_latch = byte;
	}
	else if (X->d_enabled && addr == 0xd110) {
	/* XLD byte output from atari to disk latch */
		D(printf("d110: disk output byte:%2x\n",byte));
		if (X->modem_latch & 0x80){
			/* 8040 T1=1 */
			X->CommandIndex = 0;
			X->DataIndex = 0;
			X->TransferStatus = PIO_CommandFrame;
			X->ExpectedBytes = 5;
			D(printf("command frame expected\n"));
		}
		else if (X->TransferStatus == PIO_StatusRead || X->TransferStatus == PIO_ReadFrame) {
			D(printf("read ack strobe\n"));
		}
		else {
			PIO_PutByte(byte);
		}
	}
}

int PBI_XLD_D1ffPutByte_Ctx(Atari800_Instance *inst, UBYTE byte)
{
	int result = 0; /* handled */
	PBI_XLD_PIN_CTX(inst);
	if (X->d_enabled && byte == DISK_MASK) {
		memcpy(Xi->memory.mem + 0xd800, X->diskrom, 0x800);
		D(printf("DISK rom activated\n"));
	}
	else if (byte == MODEM_MASK) {
		memcpy(Xi->memory.mem + 0xd800, X->voicerom + 0x800, 0x800);
		D(printf("MODEM rom activated\n"));
	}
	else if (byte == VOICE_MASK) {
		memcpy(Xi->memory.mem + 0xd800, X->voicerom, 0x800);
		D(printf("VOICE rom activated\n"));
	}
	else result = PBI_NOT_HANDLED;
	return result;
}

void PBI_XLD_votrax_busy_callback_Ctx(Atari800_Instance *inst, int busy_status)
{
	PBI_XLD_PIN_CTX(inst);
	if (!busy_status && (X->votrax_latch & 0x80)){
		/* busy->idle and IRQ enabled */
		D(printf("votrax IRQ generated\n"));
		CPU_GenerateIRQ();
		Xi->pbi.IRQ |= VOICE_MASK;
	}
	else if (busy_status && (Xi->pbi.IRQ & VOICE_MASK)) {
		/* idle->busy and PBI_IRQ set */
		Xi->pbi.IRQ &= ~VOICE_MASK;
		/* update pokey IRQ status */
		POKEY_PutByte(POKEY_OFFSET_IRQEN, POKEY_IRQEN);
	}
}

/* from sio.c */
static UBYTE WriteSectorBack(void)
{
	UWORD sector;
	UBYTE unit;

	sector = X->CommandFrame[2] + (X->CommandFrame[3] << 8);
	unit = X->CommandFrame[0] - '1';
	if (unit >= SIO_MAX_DRIVES)		/* UBYTE range ! */
		return 0;
	switch (X->CommandFrame[1]) {
	case 0x4f:				/* Write Status Block */
		return SIO_WriteStatusBlock(unit, X->DataBuffer);
	case 0x50:				/* Write */
	case 0x57:
	case 0xD0:				/* xf551 hispeed */
	case 0xD7:
		return SIO_WriteSector(unit, sector, X->DataBuffer);
	default:
		return 'E';
	}
}

/* Put a byte that comes from the parallel bus */
static void PIO_PutByte(int byte)
{
	D(printf("TransferStatus:%d\n",X->TransferStatus));
	switch (X->TransferStatus) {
	case PIO_CommandFrame:
		D(printf("CommandIndex:%d ExpectedBytes:%d\n",X->CommandIndex,X->ExpectedBytes));
		if (X->CommandIndex < X->ExpectedBytes) {
			X->CommandFrame[X->CommandIndex++] = byte;
			if (X->CommandIndex >= X->ExpectedBytes) {
				if (X->CommandFrame[0] >= 0x31 && X->CommandFrame[0] <= 0x38) {
					X->TransferStatus = PIO_StatusRead;
					/*DELAYED_SERIN_IRQ = SERIN_INTERVAL + ACK_INTERVAL;*/
					D(printf("TransferStatus = PIO_StatusRead\n"));
				}
				else{
					X->TransferStatus = PIO_NoFrame;
					D(printf("TransferStatus = PIO_NoFrame\n"));
				}
			}
		}
		else {
			Log_print("Invalid command frame!");
			X->TransferStatus = PIO_NoFrame;
		}
		break;
	case PIO_WriteFrame:		/* Expect data */
		if (X->DataIndex < X->ExpectedBytes) {
			X->DataBuffer[X->DataIndex++] = byte;
			if (X->DataIndex >= X->ExpectedBytes) {
				UBYTE sum = SIO_ChkSum(X->DataBuffer, X->ExpectedBytes - 1);
				if (sum == X->DataBuffer[X->ExpectedBytes - 1]) {
					UBYTE result = WriteSectorBack();
					if (result != 0) {
						X->DataBuffer[0] = 'A';
						X->DataBuffer[1] = result;
						X->DataIndex = 0;
						X->ExpectedBytes = 2;
						/*DELAYED_SERIN_IRQ = SERIN_INTERVAL + ACK_INTERVAL;*/
						X->TransferStatus = PIO_FinalStatus;
					}
					else
						X->TransferStatus = PIO_NoFrame;
				}
				else {
					X->DataBuffer[0] = 'E';
					X->DataIndex = 0;
					X->ExpectedBytes = 1;
					/*DELAYED_SERIN_IRQ = SERIN_INTERVAL + ACK_INTERVAL;*/
					X->TransferStatus = PIO_FinalStatus;
				}
			}
		}
		else {
			Log_print("Invalid data frame!");
		}
		break;
	}
	/*DELAYED_SEROUT_IRQ = SEROUT_INTERVAL;*/
}

/* Get a byte from the floppy to the parallel bus. */
static int PIO_GetByte(void)
{
	int byte = 0;
	D(printf("PIO_GetByte TransferStatus:%d\n",X->TransferStatus));

	switch (X->TransferStatus) {
	case PIO_StatusRead:
		byte = PIO_Command_Frame();		/* Handle now the command */
		break;
	case PIO_FormatFrame:
		X->TransferStatus = PIO_ReadFrame;
		/*DELAYED_SERIN_IRQ = SERIN_INTERVAL << 3;*/
		/* FALL THROUGH */
	case PIO_ReadFrame:
		D(printf("ReadFrame: DataIndex:%d ExpectedBytes:%d\n",X->DataIndex,X->ExpectedBytes));
		if (X->DataIndex < X->ExpectedBytes) {
			byte = X->DataBuffer[X->DataIndex++];
			if (X->DataIndex >= X->ExpectedBytes) {
				X->TransferStatus = PIO_NoFrame;
			}
			/*else {*/
				/* set delay using the expected transfer speed */
				/*DELAYED_SERIN_IRQ = (DataIndex == 1) ? SERIN_INTERVAL*/
					/*: ((SERIN_INTERVAL * AUDF[CHAN3] - 1) / 0x28 + 1);*/
			/*}*/
		}
		else {
			Log_print("Invalid read frame!");
			X->TransferStatus = PIO_NoFrame;
		}
		break;
	case PIO_FinalStatus:
		if (X->DataIndex < X->ExpectedBytes) {
			byte = X->DataBuffer[X->DataIndex++];
			if (X->DataIndex >= X->ExpectedBytes) {
				X->TransferStatus = PIO_NoFrame;
			}
			/*else {
				if (DataIndex == 0)
					DELAYED_SERIN_IRQ = SERIN_INTERVAL + ACK_INTERVAL;
				else
					DELAYED_SERIN_IRQ = SERIN_INTERVAL;
			}*/
		}
		else {
			Log_print("Invalid read frame!");
			X->TransferStatus = PIO_NoFrame;
		}
		break;
	default:
		break;
	}
	return byte;
}

static UBYTE PIO_Command_Frame(void)
{
	int unit;
	int sector;
	int realsize;

	sector = X->CommandFrame[2] | (((UWORD) X->CommandFrame[3]) << 8);
	unit = X->CommandFrame[0] - '1';

	if (unit < 0 || unit >= SIO_MAX_DRIVES) {
		/* Unknown device */
		Log_print("Unknown command frame: %02x %02x %02x %02x %02x",
			   X->CommandFrame[0], X->CommandFrame[1], X->CommandFrame[2],
			   X->CommandFrame[3], X->CommandFrame[4]);
		X->TransferStatus = PIO_NoFrame;
		return 0;
	}
	switch (X->CommandFrame[1]) {
	case 0x01:
		Log_print("PIO DISK: Set large mode (unimplemented)");
		return 'E';
	case 0x02:
		Log_print("PIO DISK: Set small mode (unimplemented)");
		return 'E';
	case 0x23:
		Log_print("PIO DISK: Drive Diagnostic In (unimplemented)");
		return 'E';
	case 0x24:
		Log_print("PIO DISK: Drive Diagnostic Out (unimplemented)");
		return 'E';
	case 0x4e:				/* Read Status */
#ifdef PBI_DEBUG
		Log_print("PIO DISK: Read-status frame: %02x %02x %02x %02x %02x",
			X->CommandFrame[0], X->CommandFrame[1], X->CommandFrame[2],
			X->CommandFrame[3], X->CommandFrame[4]);
#endif
		X->DataBuffer[0] = SIO_ReadStatusBlock(unit, X->DataBuffer + 1);
		X->DataBuffer[13] = SIO_ChkSum(X->DataBuffer + 1, 12);
		X->DataIndex = 0;
		X->ExpectedBytes = 14;
		X->TransferStatus = PIO_ReadFrame;
		/*DELAYED_SERIN_IRQ = SERIN_INTERVAL;*/
		return 'A';
	case 0x4f:				/* Write status */
#ifdef PBI_DEBUG
		Log_print("PIO DISK: Write-status frame: %02x %02x %02x %02x %02x",
			X->CommandFrame[0], X->CommandFrame[1], X->CommandFrame[2],
			X->CommandFrame[3], X->CommandFrame[4]);
#endif
		X->ExpectedBytes = 13;
		X->DataIndex = 0;
		X->TransferStatus = PIO_WriteFrame;
		return 'A';
	case 0x50:				/* Write */
	case 0x57:
#ifdef PBI_DEBUG
		Log_print("PIO DISK: Write-sector frame: %02x %02x %02x %02x %02x",
			X->CommandFrame[0], X->CommandFrame[1], X->CommandFrame[2],
			X->CommandFrame[3], X->CommandFrame[4]);
#endif
		SIO_SizeOfSector((UBYTE) unit, sector, &realsize, NULL);
		X->ExpectedBytes = realsize + 1;
		X->DataIndex = 0;
		X->TransferStatus = PIO_WriteFrame;
		SIO_last_op = SIO_LAST_WRITE;
		SIO_last_op_time = 10;
		SIO_last_drive = unit + 1;
		return 'A';
	case 0x52:				/* Read */
#ifdef PBI_DEBUG
		Log_print("PIO DISK: Read-sector frame: %02x %02x %02x %02x %02x",
			X->CommandFrame[0], X->CommandFrame[1], X->CommandFrame[2],
			X->CommandFrame[3], X->CommandFrame[4]);
#endif
		SIO_SizeOfSector((UBYTE) unit, sector, &realsize, NULL);
		X->DataBuffer[0] = SIO_ReadSector(unit, sector, X->DataBuffer + 1);
		X->DataBuffer[1 + realsize] = SIO_ChkSum(X->DataBuffer + 1, realsize);
		X->DataIndex = 0;
		X->ExpectedBytes = 2 + realsize;
		X->TransferStatus = PIO_ReadFrame;
		/* wait longer before confirmation because bytes could be lost */
		/* before the buffer was set (see $E9FB & $EA37 in XL-OS) */
		/*DELAYED_SERIN_IRQ = SERIN_INTERVAL << 2;*/
		/*
#ifndef NO_SECTOR_DELAY
		if (sector == 1) {
			//DELAYED_SERIN_IRQ += delay_counter;
			delay_counter = SECTOR_DELAY;
		}
		else {
			delay_counter = 0;
		}
#endif*/
		SIO_last_op = SIO_LAST_READ;
		SIO_last_op_time = 10;
		SIO_last_drive = unit + 1;
		return 'A';
	case 0x53:				/* Status */
		/*
		from spec doc:
		BYTE 1 - DISK STATUS

		BIT 0 = 1 indicates an invalid
		command frame was receiv-
		ed.
		BIT 1 = 1 indicates an invalid
		data frame was received.
		BIT 2 = 1 indicates an opera-
		tion was unsuccessful.
		BIT 3 = 1 indicates the disk-
		ette is write protected.
		BIT 4 = 1 indicates drive is
		active.
		BITS 5-7 = 100 indicates single
		density format.
		BITS 5-7 = 101 indicates double
		density format.

		BYTE 2 - DISK CONTROLLER HARDWARE
		STATUS

		This byte shall contain the in-
		verted value of the disk con-
		troller hardware status regis-
		ter as of the last operation.
		The hardware status value for
		no errors shall be $FF. A zero
		in any bit position shall indi-
		cate an error. The definition
		of the bit positions shall be:

		BIT 0 = 0 indicates device busy
		BIT 1 = 0 indicates data re-
		quest is full on a read
		operation.
		BIT 2 = 0 indicates data lost
		BIT 3 = 0 indicates CRC error
		BIT 4 = 0 indicates desired
		track and sector not found
		BIT 5 = 0 indicates record
		type/write fault
		BIT 6 NOT USED
		*BIT 7 = 0 indicates device not
		ready (door open)

		BYTES 3 & 4 - TIMEOUT

		These bytes shall contain a
		disk controller provided maxi-
		mum timeout value, in seconds,
		for the worst case command. The
		worst case operation is for a
		disk format command (time TBD
		seconds). Byte 4 is not used,
		currently.*/
		/*****Compare with:******/
/*
   Status Request from Atari 400/800 Technical Reference Notes

   DVSTAT + 0   Command Status
   DVSTAT + 1   Hardware Status
   DVSTAT + 2   Timeout
   DVSTAT + 3   Unused

   Command Status Bits

   Bit 0 = 1 indicates an invalid command frame was received(same)
   Bit 1 = 1 indicates an invalid data frame was received(same)
   Bit 2 = 1 indicates that last read/write operation was unsuccessful(same)
   Bit 3 = 1 indicates that the diskette is write protected(same)
   Bit 4 = 1 indicates active/standby(same)

   plus

   Bit 5 = 1 indicates double density
   Bit 7 = 1 indicates dual density disk (1050 format)
 */

#ifdef PBI_DEBUG
		Log_print("PIO DISK: Status frame: %02x %02x %02x %02x %02x",
			X->CommandFrame[0], X->CommandFrame[1], X->CommandFrame[2],
			X->CommandFrame[3], X->CommandFrame[4]);
#endif
		/*if (SIO_drive_status[unit]==SIO_OFF) SIO_drive_status[unit]=SIO_NO_DISK;*/
		/*need to modify the line below also for  SIO_OFF==SIO_NO_DISK*/
		X->DataBuffer[0] = SIO_DriveStatus(unit, X->DataBuffer + 1);
		X->DataBuffer[2] = 0xff;/*/1;//SIO_DriveStatus(unit, DataBuffer + 1);*/
		if (SIO_drive_status[unit]==SIO_NO_DISK || SIO_drive_status[unit]==SIO_OFF){
		/*Can't turn 1450XLD drives off, so make SIO_OFF==SIO_NO_DISK*/
			X->DataBuffer[2]=0x7f;
		}
		X->DataBuffer[1 + 4] = SIO_ChkSum(X->DataBuffer + 1, 4);
		X->DataIndex = 0;
		X->ExpectedBytes = 6;
		X->TransferStatus = PIO_ReadFrame;
		/*DELAYED_SERIN_IRQ = SERIN_INTERVAL;*/
		return 'A';
	case 0x21:				/* Format Disk */
#ifdef PBI_DEBUG
		Log_print("PIO DISK: Format-disk frame: %02x %02x %02x %02x %02x",
			X->CommandFrame[0], X->CommandFrame[1], X->CommandFrame[2],
			X->CommandFrame[3], X->CommandFrame[4]);
#endif
		realsize = SIO_format_sectorsize[unit];
		X->DataBuffer[0] = SIO_FormatDisk(unit, X->DataBuffer + 1, realsize, SIO_format_sectorcount[unit]);
		X->DataBuffer[1 + realsize] = SIO_ChkSum(X->DataBuffer + 1, realsize);
		X->DataIndex = 0;
		X->ExpectedBytes = 2 + realsize;
		X->TransferStatus = PIO_FormatFrame;
		/*DELAYED_SERIN_IRQ = SERIN_INTERVAL;*/
		return 'A';
	case 0x22:				/* Dual Density Format */
#ifdef PBI_DEBUG
		Log_print("PIO DISK: Format-Medium frame: %02x %02x %02x %02x %02x",
			X->CommandFrame[0], X->CommandFrame[1], X->CommandFrame[2],
			X->CommandFrame[3], X->CommandFrame[4]);
#endif
		X->DataBuffer[0] = SIO_FormatDisk(unit, X->DataBuffer + 1, 128, 1040);
		X->DataBuffer[1 + 128] = SIO_ChkSum(X->DataBuffer + 1, 128);
		X->DataIndex = 0;
		X->ExpectedBytes = 2 + 128;
		X->TransferStatus = PIO_FormatFrame;
		/*DELAYED_SERIN_IRQ = SERIN_INTERVAL;*/
		return 'A';

		/*
		The Integral Disk Drive uses COMMAND BYTE $B1 and
			$B2 for internal use. These COMMAND BYTES may not
			be used by any other drivers.*/
	case 0xb1:
		Log_print("PIO DISK: Internal Command 0xb1 (unimplemented)");
		return 'E';
	case 0xb2:
		Log_print("PIO DISK: Internal Command 0xb2 (unimplemented)");
		return 'E';


	default:
		/* Unknown command for a disk drive */
#ifdef PBI_DEBUG
		Log_print("PIO DISK: Unknown Command frame: %02x %02x %02x %02x %02x",
			X->CommandFrame[0], X->CommandFrame[1], X->CommandFrame[2],
			X->CommandFrame[3], X->CommandFrame[4]);
#endif
		X->TransferStatus = PIO_NoFrame;
		return 'E';
	}
}


#ifndef BASIC

void PBI_XLD_StateSave_Ctx(Atari800_Instance *inst)
{
	PBI_XLD_PIN_CTX(inst);
	StateSav_SaveINT(&PBI_XLD_enabled, 1);
	if (PBI_XLD_enabled) {
		StateSav_SaveINT(&PBI_XLD_v_enabled, 1);
		StateSav_SaveINT(&X->d_enabled, 1);
		StateSav_SaveFNAME(X->d_rom_filename);
		StateSav_SaveFNAME(X->v_rom_filename);

		StateSav_SaveUBYTE(&X->votrax_latch, 1);
		StateSav_SaveUBYTE(&X->modem_latch, 1);
		StateSav_SaveUBYTE(X->CommandFrame, sizeof(X->CommandFrame));
		StateSav_SaveINT(&X->CommandIndex, 1);
		StateSav_SaveUBYTE(X->DataBuffer, sizeof(X->DataBuffer));
		StateSav_SaveINT(&X->DataIndex, 1);
		StateSav_SaveINT(&X->TransferStatus, 1);
		StateSav_SaveINT(&X->ExpectedBytes, 1);
		StateSav_SaveINT(&VOTRAXSND_busy, 1);
	}
}

void PBI_XLD_StateRead_Ctx(Atari800_Instance *inst)
{
	PBI_XLD_PIN_CTX(inst);
	StateSav_ReadINT(&PBI_XLD_enabled, 1);
	if (PBI_XLD_enabled) {
		/* UI should have paused sound while doing this */
		StateSav_ReadINT(&PBI_XLD_v_enabled, 1);
		StateSav_ReadINT(&X->d_enabled, 1);
		StateSav_ReadFNAME(X->d_rom_filename);
		StateSav_ReadFNAME(X->v_rom_filename);
		if (PBI_XLD_v_enabled) {
			init_xld_v();
			VOTRAXSND_Reinit();
		}
		if (X->d_enabled) init_xld_d();
		StateSav_ReadUBYTE(&X->votrax_latch, 1);
		StateSav_ReadUBYTE(&X->modem_latch, 1);
		StateSav_ReadUBYTE(X->CommandFrame, sizeof(X->CommandFrame));
		StateSav_ReadINT(&X->CommandIndex, 1);
		StateSav_ReadUBYTE(X->DataBuffer, sizeof(X->DataBuffer));
		StateSav_ReadINT(&X->DataIndex, 1);
		StateSav_ReadINT(&X->TransferStatus, 1);
		StateSav_ReadINT(&X->ExpectedBytes, 1);
		StateSav_ReadINT(&VOTRAXSND_busy, 1);
	}
	else {
		PBI_XLD_v_enabled = FALSE;
		X->d_enabled = FALSE;
	}
}

#endif /* #ifndef BASIC */

/*
vim:ts=4:sw=4:
*/
