#ifndef CASSETTE_H_
#define CASSETTE_H_

#include <stdio.h>		/* for FILE and FILENAME_MAX */

#include "atari.h"		/* for UBYTE */
#include "instance.h" /* Cassette_state_t (transitional default-instance aliases) */

#define CASSETTE_DESCRIPTION_MAX 256

/* Transitional Option C bridge: the per-instance cassette state lives in
   Cassette_state_t (instance.h). Until all callers pass an instance
   explicitly, the legacy global names are aliased to the default instance. */
#define CASSETTE_filename    (Atari800_default->cassette.filename)
#define CASSETTE_description (Atari800_default->cassette.description)
#define CASSETTE_status      (Atari800_default->cassette.status)

/* Context-aware entry points (Option C). The legacy names below are
   forwarding macros that pass the default instance, so not-yet-migrated
   callers are unchanged. */
/* Used in Atari800_Initialise during emulator initialisation */
int CASSETTE_Initialise_Ctx(Atari800_Instance *inst, int *argc, char *argv[]);
void CASSETTE_Exit_Ctx(Atari800_Instance *inst);
/* Config file read/write */
int CASSETTE_ReadConfig_Ctx(Atari800_Instance *inst, char *string, char *ptr);
void CASSETTE_WriteConfig_Ctx(Atari800_Instance *inst, FILE *fp);

#define CASSETTE_Initialise(argc, argv) CASSETTE_Initialise_Ctx(Atari800_default, argc, argv)
#define CASSETTE_Exit()                 CASSETTE_Exit_Ctx(Atari800_default)
#define CASSETTE_ReadConfig(str, ptr)   CASSETTE_ReadConfig_Ctx(Atari800_default, str, ptr)
#define CASSETTE_WriteConfig(fp)        CASSETTE_WriteConfig_Ctx(Atari800_default, fp)

/* Attaches a tape image. Also resets CASSETTE_write_protect to FALSE.
   Returns TRUE on success, FALSE otherwise. */
int CASSETTE_Insert_Ctx(Atari800_Instance *inst, const char *filename);
void CASSETTE_Remove_Ctx(Atari800_Instance *inst);
/* Creates a new file in CAS format. DESCRIPTION can be NULL.
   Returns TRUE on success, FALSE otherwise. */
int CASSETTE_CreateCAS_Ctx(Atari800_Instance *inst, char const *filename, char const *description);

#define CASSETTE_Insert(filename)       CASSETTE_Insert_Ctx(Atari800_default, filename)
#define CASSETTE_Remove()               CASSETTE_Remove_Ctx(Atari800_default)
#define CASSETTE_CreateCAS(fn, desc)    CASSETTE_CreateCAS_Ctx(Atari800_default, fn, desc)

#define CASSETTE_hold_start           (Atari800_default->cassette.hold_start)
#define CASSETTE_hold_start_on_reboot (Atari800_default->cassette.hold_start_on_reboot) /* preserve hold_start after reboot */
#define CASSETTE_press_space          (Atari800_default->cassette.press_space)

/* Is cassette file write-protected? Don't change directly, use CASSETTE_ToggleWriteProtect(). */
#define CASSETTE_write_protect        (Atari800_default->cassette.write_protect)
/* Switches RO/RW. Fails with FALSE if the tape cannot be switched to RW. */
int CASSETTE_ToggleWriteProtect_Ctx(Atari800_Instance *inst);
#define CASSETTE_ToggleWriteProtect()   CASSETTE_ToggleWriteProtect_Ctx(Atari800_default)

 /* Is cassette record button pressed? Don't change directly, use CASSETTE_ToggleRecord(). */
#define CASSETTE_record               (Atari800_default->cassette.record)
/* If tape is mounted, switches recording on/off (otherwise return FALSE).
   Recording operations would fail if the tape is read-only. In such
   situation, when switching recording on the function returns FALSE. */
int CASSETTE_ToggleRecord_Ctx(Atari800_Instance *inst);

void CASSETTE_Seek_Ctx(Atari800_Instance *inst, unsigned int position);
/* Returns status of the DATA IN line. */
int CASSETTE_IOLineStatus_Ctx(Atari800_Instance *inst);
/* Get the byte which was recently loaded from tape. */
int CASSETTE_GetByte_Ctx(Atari800_Instance *inst);
/* Put a byte into the cas file.
   The block is being written at first putbyte of the subsequent block */
void CASSETTE_PutByte_Ctx(Atari800_Instance *inst, int byte);
/* Set motor status: 1 - on, 0 - off */
void CASSETTE_TapeMotor_Ctx(Atari800_Instance *inst, int onoff);
/* Advance the tape by a scanline. Return TRUE if a new byte has been loaded
   and POKEY_SERIN must be updated. */
int CASSETTE_AddScanLine_Ctx(Atari800_Instance *inst);
/* Reset cassette serial transmission; call when resseting POKEY by SKCTL. */
void CASSETTE_ResetPOKEY_Ctx(Atari800_Instance *inst);

#define CASSETTE_ToggleRecord()         CASSETTE_ToggleRecord_Ctx(Atari800_default)
#define CASSETTE_Seek(position)         CASSETTE_Seek_Ctx(Atari800_default, position)
#define CASSETTE_IOLineStatus()         CASSETTE_IOLineStatus_Ctx(Atari800_default)
#define CASSETTE_GetByte()              CASSETTE_GetByte_Ctx(Atari800_default)
#define CASSETTE_PutByte(byte)          CASSETTE_PutByte_Ctx(Atari800_default, byte)
#define CASSETTE_TapeMotor(onoff)       CASSETTE_TapeMotor_Ctx(Atari800_default, onoff)
#define CASSETTE_AddScanLine()          CASSETTE_AddScanLine_Ctx(Atari800_default)
#define CASSETTE_ResetPOKEY()           CASSETTE_ResetPOKEY_Ctx(Atari800_default)

/* Return size in blocks of the currently-mounted tape file. */
unsigned int CASSETTE_GetSize_Ctx(Atari800_Instance *inst);
/* Return current position (block number) of the mounted tape (counted from 1). */
unsigned int CASSETTE_GetPosition_Ctx(Atari800_Instance *inst);
#define CASSETTE_GetSize()              CASSETTE_GetSize_Ctx(Atari800_default)
#define CASSETTE_GetPosition()          CASSETTE_GetPosition_Ctx(Atari800_default)

/* --- Functions used by patched SIO --- */
/* -- SIO_Handler() -- */
int CASSETTE_AddGap_Ctx(Atari800_Instance *inst, int gaptime);
#define CASSETTE_AddGap(gaptime)        CASSETTE_AddGap_Ctx(Atari800_default, gaptime)
/* Reads a record from tape and copies its contents (max. LENGTH bytes,
   excluding the trailing checksum) to memory starting at address DEST_ADDR.
   Returns FALSE if number of bytes in record doesn't equal LENGTH, or
   checksum is incorrect, or there was a read error/end of file; otherwise
   returns TRUE. */
int CASSETTE_ReadToMemory_Ctx(Atari800_Instance *inst, UWORD dest_addr, int length);
#define CASSETTE_ReadToMemory(dest, len) CASSETTE_ReadToMemory_Ctx(Atari800_default, dest, len)
/* Reads LENGTH bytes from memory starting at SRC_ADDR and writes them as
   a record (with added checksum) to tape. Returns FALSE if there was a write
   error, TRUE otherwise. */
int CASSETTE_WriteFromMemory_Ctx(Atari800_Instance *inst, UWORD src_addr, int length);
/* -- Other -- */
void CASSETTE_LeaderLoad_Ctx(Atari800_Instance *inst);
void CASSETTE_LeaderSave_Ctx(Atari800_Instance *inst);
#define CASSETTE_WriteFromMemory(src, len) CASSETTE_WriteFromMemory_Ctx(Atari800_default, src, len)
#define CASSETTE_LeaderLoad()           CASSETTE_LeaderLoad_Ctx(Atari800_default)
#define CASSETTE_LeaderSave()           CASSETTE_LeaderSave_Ctx(Atari800_default)

/* Indicates whether the tape can be read from, ie. it's mounted and not on its
   end. */
#define CASSETTE_readable             (Atari800_default->cassette.readable)
/* Indicates whether the tape can be written to, ie. it's mounted and not
   read-only. */
#define CASSETTE_writable             (Atari800_default->cassette.writable)

#endif /* CASSETTE_H_ */
