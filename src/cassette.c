/*
 * cassette.c - cassette emulation
 *
 * Copyright (C) 2001 Piotr Fusik
 * Copyright (C) 2001-2011 Atari800 development team (see DOC/CREDITS)
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "atari.h"
#include "cpu.h"
#include "cassette.h"
#include "esc.h"
#include "img_tape.h"
#include "log.h"
#include "util.h"
#include "pokey.h"

/* Transitional Option C bridge: the per-instance cassette state lives in
   Cassette_state_t (instance.h). Within cassette.c the legacy global/static
   names are aliases into the file-scope context pointer `CAS`, which is
   pinned to the default instance until callers pass an instance
   names are aliases into the default instance (via
   Atari800_default->cassette.*). */
#undef CASSETTE_filename
#undef CASSETTE_description
#undef CASSETTE_status
#undef CASSETTE_hold_start
#undef CASSETTE_hold_start_on_reboot
#undef CASSETTE_press_space
#undef CASSETTE_write_protect
#undef CASSETTE_record
#undef CASSETTE_readable
#undef CASSETTE_writable
static Cassette_state_t *CAS;
/* Pin the context to the given instance (set from the *_Ctx() argument). */
static Atari800_Instance *CASi;
#define CASSETTE_PIN_CTX(inst) ((void) (CASi = (inst), CAS = &(inst)->cassette))
#define CASSETTE_filename    (CAS->filename)
#define CASSETTE_description (CAS->description)
#define CASSETTE_status      (CAS->status)
#define CASSETTE_hold_start           (CAS->hold_start)
#define CASSETTE_hold_start_on_reboot (CAS->hold_start_on_reboot)
#define CASSETTE_press_space          (CAS->press_space)
#define CASSETTE_write_protect        (CAS->write_protect)
#define CASSETTE_record               (CAS->record)
#define CASSETTE_readable             (CAS->readable)
#define CASSETTE_writable             (CAS->writable)
#define cassette_file      (CAS->cassette_file)
#define event_time_left    (CAS->event_time_left)
#define pending_serin      (CAS->pending_serin)
#define passing_gap        (CAS->passing_gap)
#define pending_serin_byte (CAS->pending_serin_byte)
#define serin_byte         (CAS->serin_byte)
#define cassette_gapdelay  (CAS->cassette_gapdelay)
#define cassette_motor     (CAS->cassette_motor)
#define eof_of_tape        (CAS->eof_of_tape)

/* The cassette.h forwarding macros route legacy names to the default
   instance; inside cassette.c they are redefined to route to the instance
   pinned by CASSETTE_PIN_CTX() so the *_Ctx() bodies operate on their own
   instance. */
#undef CASSETTE_Initialise
#undef CASSETTE_Exit
#undef CASSETTE_ReadConfig
#undef CASSETTE_WriteConfig
#undef CASSETTE_Insert
#undef CASSETTE_Remove
#undef CASSETTE_CreateCAS
#undef CASSETTE_GetPosition
#undef CASSETTE_GetSize
#undef CASSETTE_Seek
#undef CASSETTE_GetByte
#undef CASSETTE_IOLineStatus
#undef CASSETTE_PutByte
#undef CASSETTE_TapeMotor
#undef CASSETTE_ToggleWriteProtect
#undef CASSETTE_ToggleRecord
#undef CASSETTE_AddScanLine
#undef CASSETTE_ResetPOKEY
#undef CASSETTE_AddGap
#undef CASSETTE_LeaderLoad
#undef CASSETTE_LeaderSave
#undef CASSETTE_ReadToMemory
#undef CASSETTE_WriteFromMemory
#define CASSETTE_Initialise(argc, argv)  CASSETTE_Initialise_Ctx(CASi, argc, argv)
#define CASSETTE_Exit()                  CASSETTE_Exit_Ctx(CASi)
#define CASSETTE_ReadConfig(str, ptr)    CASSETTE_ReadConfig_Ctx(CASi, str, ptr)
#define CASSETTE_WriteConfig(fp)         CASSETTE_WriteConfig_Ctx(CASi, fp)
#define CASSETTE_Insert(fn)              CASSETTE_Insert_Ctx(CASi, fn)
#define CASSETTE_Remove()                CASSETTE_Remove_Ctx(CASi)
#define CASSETTE_CreateCAS(fn, desc)     CASSETTE_CreateCAS_Ctx(CASi, fn, desc)
#define CASSETTE_GetPosition()           CASSETTE_GetPosition_Ctx(CASi)
#define CASSETTE_GetSize()               CASSETTE_GetSize_Ctx(CASi)
#define CASSETTE_Seek(pos)               CASSETTE_Seek_Ctx(CASi, pos)
#define CASSETTE_GetByte()               CASSETTE_GetByte_Ctx(CASi)
#define CASSETTE_IOLineStatus()          CASSETTE_IOLineStatus_Ctx(CASi)
#define CASSETTE_PutByte(byte)           CASSETTE_PutByte_Ctx(CASi, byte)
#define CASSETTE_TapeMotor(onoff)        CASSETTE_TapeMotor_Ctx(CASi, onoff)
#define CASSETTE_ToggleWriteProtect()    CASSETTE_ToggleWriteProtect_Ctx(CASi)
#define CASSETTE_ToggleRecord()          CASSETTE_ToggleRecord_Ctx(CASi)
#define CASSETTE_AddScanLine()           CASSETTE_AddScanLine_Ctx(CASi)
#define CASSETTE_ResetPOKEY()            CASSETTE_ResetPOKEY_Ctx(CASi)
#define CASSETTE_AddGap(gaptime)         CASSETTE_AddGap_Ctx(CASi, gaptime)
#define CASSETTE_LeaderLoad()            CASSETTE_LeaderLoad_Ctx(CASi)
#define CASSETTE_LeaderSave()            CASSETTE_LeaderSave_Ctx(CASi)
#define CASSETTE_ReadToMemory(dest, len) CASSETTE_ReadToMemory_Ctx(CASi, dest, len)
#define CASSETTE_WriteFromMemory(src, len) CASSETTE_WriteFromMemory_Ctx(CASi, src, len)

/* Time till the end of the current tape event (byte or gap), in CPU ticks. */

/* Indicates that there is a SERIN transmission in progress and when it ends,
   the current byte should be copied to POKEY_SERIN. This can be reset by
   rewinding/removing the tape or by resetting POKEY.
   Note that this variable has any meaning when PASSING_GAP is FALSE,
   so it doesn't have to be reset during PASSING_IRG. */

/* Indicates that an Inter-Record-Gap is currently being passed. It's set to TRUE
   at the beginning of each block. */

/* if penting_serin == TRUE, this holds the byte that is currently loaded from
   tape. It might be later copied to serin_byte. */

/* Indicates whether the tape has ended. During saving the value is always 0;
   during loading it is equal to (CASSETTE_GetPosition() >= CASSETTE_GetSize()). */

/* Call this function after each change of
   cassette_motor, CASSETTE_status or eof_of_tape. */
static void UpdateFlags(void)
{
	CASSETTE_readable = cassette_motor &&
	                    (CASSETTE_status == CASSETTE_STATUS_READ_WRITE ||
	                     CASSETTE_status == CASSETTE_STATUS_READ_ONLY) &&
	                     !eof_of_tape;
	CASSETTE_writable = cassette_motor &&
	                    CASSETTE_status == CASSETTE_STATUS_READ_WRITE &&
	                    !CASSETTE_write_protect;
}

int CASSETTE_ReadConfig_Ctx(Atari800_Instance *inst, char *string, char *ptr)
{
	CASSETTE_PIN_CTX(inst);
	if (strcmp(string, "CASSETTE_FILENAME") == 0)
		Util_strlcpy(CASSETTE_filename, ptr, sizeof(CASSETTE_filename));
	else if (strcmp(string, "CASSETTE_LOADED") == 0) {
		int value = Util_sscanbool(ptr);
		if (value == -1)
			return FALSE;
		CASSETTE_status = (value ? CASSETTE_STATUS_READ_WRITE : CASSETTE_STATUS_NONE);
	}
	else if (strcmp(string, "CASSETTE_WRITE_PROTECT") == 0) {
		int value = Util_sscanbool(ptr);
		if (value == -1)
			return FALSE;
		CASSETTE_write_protect = value;
	}
	else return FALSE;
	return TRUE;
}

void CASSETTE_WriteConfig_Ctx(Atari800_Instance *inst, FILE *fp)
{
	CASSETTE_PIN_CTX(inst);
	fprintf(fp, "CASSETTE_FILENAME=%s\n", CASSETTE_filename);
	fprintf(fp, "CASSETTE_LOADED=%d\n", CASSETTE_status != CASSETTE_STATUS_NONE);
	fprintf(fp, "CASSETTE_WRITE_PROTECT=%d\n", CASSETTE_write_protect);
}

int CASSETTE_Initialise_Ctx(Atari800_Instance *inst, int *argc, char *argv[])
{
	CASSETTE_PIN_CTX(inst);
	int i;
	int j;
	int protect = FALSE; /* Is write-protect requested in command line? */

	for (i = j = 1; i < *argc; i++) {
		int i_a = (i + 1 < *argc);		/* is argument available? */
		int a_m = FALSE;			/* error, argument missing! */

		if (strcmp(argv[i], "-tape") == 0) {
			if (i_a) {
				Util_strlcpy(CASSETTE_filename, argv[++i], sizeof(CASSETTE_filename));
				CASSETTE_status = CASSETTE_STATUS_READ_WRITE;
				/* Reset any write-protection read from config file. */
				CASSETTE_write_protect = FALSE;
			}
			else a_m = TRUE;
		}
		else if (strcmp(argv[i], "-boottape") == 0) {
			if (i_a) {
				Util_strlcpy(CASSETTE_filename, argv[++i], sizeof(CASSETTE_filename));
				CASSETTE_status = CASSETTE_STATUS_READ_WRITE;
				/* Reset any write-protection read from config file. */
				CASSETTE_write_protect = FALSE;
				CASSETTE_hold_start = 1;
			}
			else a_m = TRUE;
		}
		else if (strcmp(argv[i], "-tape-readonly") == 0)
			protect = TRUE;
		else {
			if (strcmp(argv[i], "-help") == 0) {
				Log_print("\t-tape <file>      Insert cassette image");
				Log_print("\t-boottape <file>  Insert cassette image and boot it");
				Log_print("\t-tape-readonly    Mark the attached cassette image as read-only");
			}
			argv[j++] = argv[i];
		}

		if (a_m) {
			Log_print("Missing argument for '%s'", argv[i]);
			return FALSE;
		}
	}

	*argc = j;

	/* If CASSETTE_status was set in this function or in CASSETTE_ReadConfig(),
	   then tape is to be mounted. */
	if (CASSETTE_status != CASSETTE_STATUS_NONE && CASSETTE_filename[0] != '\0') {
		/* Tape is mounted unprotected by default - overrun it if needed. */
		protect = protect || CASSETTE_write_protect;
		if (!CASSETTE_Insert(CASSETTE_filename)) {
			CASSETTE_status = CASSETTE_STATUS_NONE;
			Log_print("Cannot open cassette image %s", CASSETTE_filename);
		}
		else if (protect)
			CASSETTE_ToggleWriteProtect();
	}

	return TRUE;
}

void CASSETTE_Exit_Ctx(Atari800_Instance *inst)
{
	CASSETTE_PIN_CTX(inst);
	CASSETTE_Remove();
}

int CASSETTE_Insert_Ctx(Atari800_Instance *inst, const char *filename)
{
	CASSETTE_PIN_CTX(inst);
	int writable;
	char const *description;

	IMG_TAPE_t *file = IMG_TAPE_Open(filename, &writable, &description);
	if (file == NULL)
		return FALSE;

	CASSETTE_Remove();
	cassette_file = file;
	/* Guard against providing CASSETTE_filename as parameter. */
	if (CASSETTE_filename != filename)
		strcpy(CASSETTE_filename, filename);
	eof_of_tape = 0;

	CASSETTE_status = (writable ? CASSETTE_STATUS_READ_WRITE : CASSETTE_STATUS_READ_ONLY);
	event_time_left = 0;
	pending_serin = FALSE;
	passing_gap = FALSE;

	if (description != NULL)
		Util_strlcpy(CASSETTE_description, description, sizeof(CASSETTE_description));
	CASSETTE_write_protect = FALSE;
	CASSETTE_record = FALSE;
	UpdateFlags();
	cassette_gapdelay = 0;

	return TRUE;
}

void CASSETTE_Remove_Ctx(Atari800_Instance *inst)
{
	CASSETTE_PIN_CTX(inst);
	if (cassette_file != NULL) {
		IMG_TAPE_Close(cassette_file);
		cassette_file = NULL;
	}
	CASSETTE_status = CASSETTE_STATUS_NONE;
	CASSETTE_description[0] = '\0';
	UpdateFlags();
}

int CASSETTE_CreateCAS_Ctx(Atari800_Instance *inst, const char *filename, const char *description) {
	CASSETTE_PIN_CTX(inst);
	IMG_TAPE_t *file = IMG_TAPE_Create(filename, description);
	if (file == NULL)
		return FALSE;

	CASSETTE_Remove(); /* Unmount any previous tape image. */
	cassette_file = file;
	Util_strlcpy(CASSETTE_filename, filename, sizeof(CASSETTE_filename));
	if (description != NULL)
		Util_strlcpy(CASSETTE_description, description, sizeof(CASSETTE_description));
	CASSETTE_status = CASSETTE_STATUS_READ_WRITE;
	event_time_left = 0;
	pending_serin = FALSE;
	passing_gap = FALSE;
	cassette_gapdelay = 0;
	eof_of_tape = 0;
	CASSETTE_record = TRUE;
	CASSETTE_write_protect = FALSE;
	UpdateFlags();

	return TRUE;
}

unsigned int CASSETTE_GetPosition_Ctx(Atari800_Instance *inst)
{
	CASSETTE_PIN_CTX(inst);
	if (cassette_file == NULL)
		return 0;
	return IMG_TAPE_GetPosition(cassette_file) + 1;
}

unsigned int CASSETTE_GetSize_Ctx(Atari800_Instance *inst)
{
	CASSETTE_PIN_CTX(inst);
	if (cassette_file == NULL)
		return 0;
	return IMG_TAPE_GetSize(cassette_file);
}

void CASSETTE_Seek_Ctx(Atari800_Instance *inst, unsigned int position)
{
	CASSETTE_PIN_CTX(inst);
	if (cassette_file != NULL) {
		if (position > 0)
			position --;
		IMG_TAPE_Seek(cassette_file, position);

		event_time_left = 0;
		pending_serin = FALSE;
		passing_gap = FALSE;
		eof_of_tape = 0;
		CASSETTE_record = FALSE;
		UpdateFlags();
	}
}

int CASSETTE_GetByte_Ctx(Atari800_Instance *inst)
{
	CASSETTE_PIN_CTX(inst);
	return serin_byte;
}

int CASSETTE_IOLineStatus_Ctx(Atari800_Instance *inst)
{
	CASSETTE_PIN_CTX(inst);
	/* if motor off and EOF return always 1 (equivalent the mark tone) */
	if (!CASSETTE_readable || CASSETTE_record) {
		return 1;
	}

	return IMG_TAPE_SerinStatus(cassette_file, event_time_left);
}

void CASSETTE_PutByte_Ctx(Atari800_Instance *inst, int byte)
{
	CASSETTE_PIN_CTX(inst);
	if (!ESC_enable_sio_patch && CASSETTE_writable && CASSETTE_record)
		IMG_TAPE_WriteByte(cassette_file, byte, POKEY_AUDF[POKEY_CHAN3] + POKEY_AUDF[POKEY_CHAN4]*0x100);
}

void CASSETTE_TapeMotor_Ctx(Atari800_Instance *inst, int onoff)
{
	CASSETTE_PIN_CTX(inst);
	if (cassette_motor != onoff) {
		if (CASSETTE_record && CASSETTE_writable)
			/* Recording disabled, flush the tape */
			IMG_TAPE_Flush(cassette_file);
		cassette_motor = onoff;
		UpdateFlags();
	}
}

int CASSETTE_ToggleWriteProtect_Ctx(Atari800_Instance *inst)
{
	CASSETTE_PIN_CTX(inst);
	if (CASSETTE_status != CASSETTE_STATUS_READ_WRITE)
		return FALSE;
	CASSETTE_write_protect = !CASSETTE_write_protect;
	UpdateFlags();
	return TRUE;
}

int CASSETTE_ToggleRecord_Ctx(Atari800_Instance *inst)
{
	CASSETTE_PIN_CTX(inst);
	if (CASSETTE_status == CASSETTE_STATUS_NONE)
		return FALSE;
	CASSETTE_record = !CASSETTE_record;
	if (CASSETTE_record)
		eof_of_tape = FALSE;
	else if (CASSETTE_writable)
		/* Recording disabled, flush the tape */
		IMG_TAPE_Flush(cassette_file);
	event_time_left = 0;
	pending_serin = FALSE;
	passing_gap = FALSE;
	UpdateFlags();
	/* Return FALSE to indicate that recording will not work. */
	return !CASSETTE_record || (CASSETTE_status == CASSETTE_STATUS_READ_WRITE && !CASSETTE_write_protect);
}

static void CassetteWrite(int num_ticks)
{
	if (CASSETTE_writable)
		IMG_TAPE_WriteAdvance(cassette_file, num_ticks);
}

/* Sets the stamp of next SERIN IRQ event and loads new record if necessary.
   Returns TRUE if a new byte was loaded and POKEY_SERIN should be updated.
   The function assumes that current_block <= max_block. */
static int CassetteRead(int num_ticks)
{
	if (CASSETTE_readable) {
		int loaded = FALSE; /* Function's return value */
		event_time_left -= num_ticks;
		while (event_time_left < 0) {
			unsigned int length;
			if (!passing_gap && pending_serin) {
				serin_byte = pending_serin_byte;
				/* A byte is loaded, return TRUE so it gets stored in POKEY_SERIN. */
				loaded = TRUE;
			}

			/* If POKEY is in reset state, no serial I/O occurs. */
			pending_serin = (POKEY_SKCTL & 0x03) != 0;

			if (!IMG_TAPE_Read(cassette_file, &length, &passing_gap, &pending_serin_byte)) {
				eof_of_tape = 1;
				UpdateFlags();
				return loaded;
			}

			event_time_left += length;
		}
		return loaded;
	}
	return FALSE;
}

int CASSETTE_AddScanLine_Ctx(Atari800_Instance *inst)
{
	CASSETTE_PIN_CTX(inst);
	/* increment elapsed cassette time */
	if (CASSETTE_record) {
		CassetteWrite(114);
		return FALSE;
	} else
		return CassetteRead(114);
}

void CASSETTE_ResetPOKEY_Ctx(Atari800_Instance *inst)
{
	CASSETTE_PIN_CTX(inst);
	/* Resetting POKEY stops any serial transmission. */
	pending_serin = FALSE;
	pending_serin_byte = 0xff;
}

/* --- Functions for loading/saving with SIO patch --- */

int CASSETTE_AddGap_Ctx(Atari800_Instance *inst, int gaptime)
{
	CASSETTE_PIN_CTX(inst);
	cassette_gapdelay += gaptime;
	if (cassette_gapdelay < 0)
		cassette_gapdelay = 0;
	return cassette_gapdelay;
}

/* Indicates that a loading leader is expected by the OS */
void CASSETTE_LeaderLoad_Ctx(Atari800_Instance *inst)
{
	CASSETTE_PIN_CTX(inst);
	if (CASSETTE_record)
		CASSETTE_ToggleRecord();
	CASSETTE_TapeMotor(TRUE);
	cassette_gapdelay = 9600;
}

/* indicates that a save leader is written by the OS */
void CASSETTE_LeaderSave_Ctx(Atari800_Instance *inst)
{
	CASSETTE_PIN_CTX(inst);
	if (!CASSETTE_record)
	CASSETTE_ToggleRecord();
	CASSETTE_TapeMotor(TRUE);
	cassette_gapdelay = 19200;
}

int CASSETTE_ReadToMemory_Ctx(Atari800_Instance *inst, UWORD dest_addr, int length)
{
	CASSETTE_PIN_CTX(inst);
	CASSETTE_TapeMotor(1);
	if (!CASSETTE_readable)
		return 0;

	/* Convert wait_time to ms ( wait_time * 1000 / 1789790 ) and subtract. */
	cassette_gapdelay -= event_time_left / 1789; /* better accuracy not needed */
	if (!IMG_TAPE_SkipToData(cassette_file, cassette_gapdelay)) {
		/* Ignore the eventual error, assume it is the end of file */
		cassette_gapdelay = 0;
		eof_of_tape = 1;
		UpdateFlags();
		return 0;
	}
	cassette_gapdelay = 0;

	/* Load bytes */
	switch (IMG_TAPE_ReadToMemory(cassette_file, dest_addr, length)) {
	case TRUE:
		return TRUE;
	case -1: /* Read error/EOF */
		eof_of_tape = 1;
		UpdateFlags();
		/* FALLTHROUGH */
	default: /* case FALSE */
		return FALSE;
	}
}

int CASSETTE_WriteFromMemory_Ctx(Atari800_Instance *inst, UWORD src_addr, int length)
{
	CASSETTE_PIN_CTX(inst);
	int result;
	CASSETTE_TapeMotor(1);
	if (!CASSETTE_writable)
		return 0;

	result = IMG_TAPE_WriteFromMemory(cassette_file, src_addr, length, cassette_gapdelay);
	cassette_gapdelay = 0;
	return result;
}


/*
vim:ts=4:sw=4:
*/
