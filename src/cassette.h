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

/* Used in Atari800_Initialise during emulator initialisation */
int CASSETTE_Initialise(int *argc, char *argv[]);
void CASSETTE_Exit(void);
/* Config file read/write */
int CASSETTE_ReadConfig(char *string, char *ptr);
void CASSETTE_WriteConfig(FILE *fp);

/* Attaches a tape image. Also resets CASSETTE_write_protect to FALSE.
   Returns TRUE on success, FALSE otherwise. */
int CASSETTE_Insert(const char *filename);
void CASSETTE_Remove(void);
/* Creates a new file in CAS format. DESCRIPTION can be NULL.
   Returns TRUE on success, FALSE otherwise. */
int CASSETTE_CreateCAS(char const *filename, char const *description);

#define CASSETTE_hold_start           (Atari800_default->cassette.hold_start)
#define CASSETTE_hold_start_on_reboot (Atari800_default->cassette.hold_start_on_reboot) /* preserve hold_start after reboot */
#define CASSETTE_press_space          (Atari800_default->cassette.press_space)

/* Is cassette file write-protected? Don't change directly, use CASSETTE_ToggleWriteProtect(). */
#define CASSETTE_write_protect        (Atari800_default->cassette.write_protect)
/* Switches RO/RW. Fails with FALSE if the tape cannot be switched to RW. */
int CASSETTE_ToggleWriteProtect(void);

 /* Is cassette record button pressed? Don't change directly, use CASSETTE_ToggleRecord(). */
#define CASSETTE_record               (Atari800_default->cassette.record)
/* If tape is mounted, switches recording on/off (otherwise return FALSE).
   Recording operations would fail if the tape is read-only. In such
   situation, when switching recording on the function returns FALSE. */
int CASSETTE_ToggleRecord(void);

void CASSETTE_Seek(unsigned int position);
/* Returns status of the DATA IN line. */
int CASSETTE_IOLineStatus(void);
/* Get the byte which was recently loaded from tape. */
int CASSETTE_GetByte(void);
/* Put a byte into the cas file.
   The block is being written at first putbyte of the subsequent block */
void CASSETTE_PutByte(int byte);
/* Set motor status: 1 - on, 0 - off */
void CASSETTE_TapeMotor(int onoff);
/* Advance the tape by a scanline. Return TRUE if a new byte has been loaded
   and POKEY_SERIN must be updated. */
int CASSETTE_AddScanLine(void);
/* Reset cassette serial transmission; call when resseting POKEY by SKCTL. */
void CASSETTE_ResetPOKEY(void);

/* Return size in blocks of the currently-mounted tape file. */
unsigned int CASSETTE_GetSize(void);
/* Return current position (block number) of the mounted tape (counted from 1). */
unsigned int CASSETTE_GetPosition(void);

/* --- Functions used by patched SIO --- */
/* -- SIO_Handler() -- */
int CASSETTE_AddGap(int gaptime);
/* Reads a record from tape and copies its contents (max. LENGTH bytes,
   excluding the trailing checksum) to memory starting at address DEST_ADDR.
   Returns FALSE if number of bytes in record doesn't equal LENGTH, or
   checksum is incorrect, or there was a read error/end of file; otherwise
   returns TRUE. */
int CASSETTE_ReadToMemory(UWORD dest_addr, int length);
/* Reads LENGTH bytes from memory starting at SRC_ADDR and writes them as
   a record (with added checksum) to tape. Returns FALSE if there was a write
   error, TRUE otherwise. */
int CASSETTE_WriteFromMemory(UWORD src_addr, int length);
/* -- Other -- */
void CASSETTE_LeaderLoad(void);
void CASSETTE_LeaderSave(void);

/* Indicates whether the tape can be read from, ie. it's mounted and not on its
   end. */
#define CASSETTE_readable             (Atari800_default->cassette.readable)
/* Indicates whether the tape can be written to, ie. it's mounted and not
   read-only. */
#define CASSETTE_writable             (Atari800_default->cassette.writable)

#endif /* CASSETTE_H_ */
