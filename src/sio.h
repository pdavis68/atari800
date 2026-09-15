#ifndef SIO_H_
#define SIO_H_

#include "config.h"

#include <stdio.h> /* FILENAME_MAX */

#include "atari.h"
#include "instance.h" /* SIO_state_t, SIO_MAX_DRIVES, SIO_UnitStatus */

/* Transitional Option C bridge: the per-instance SIO state lives in
   SIO_state_t (instance.h). Until all callers pass an instance explicitly,
   the legacy global names are aliased to the default instance. */
#define SIO_status      (Atari800_default->sio.status)
#define SIO_drive_status (Atari800_default->sio.drive_status)
#define SIO_filename    (Atari800_default->sio.filename)

#define SIO_LAST_READ 0
#define SIO_LAST_WRITE 1
#define SIO_last_op      (Atari800_default->sio.last_op)
#define SIO_last_op_time (Atari800_default->sio.last_op_time)
#define SIO_last_drive   (Atari800_default->sio.last_drive) /* 1 .. 8 */
#define SIO_last_sector  (Atari800_default->sio.last_sector)

int SIO_Mount(int diskno, const char *filename, int b_open_readonly);
void SIO_Dismount(int diskno);
void SIO_DisableDrive(int diskno);
int SIO_RotateDisks(void);
void SIO_Handler(void);

UBYTE SIO_ChkSum(const UBYTE *buffer, int length);
void SIO_SwitchCommandFrame(int onoff);
#ifdef NETSIO
void NetSIO_PutByte(int byte);
#endif /* NETSIO */
void SIO_PutByte(int byte);
int SIO_GetByte(void);
int SIO_Initialise(int *argc, char *argv[]);
void SIO_Exit(void);

/* Some defines about the serial I/O timing. Currently fixed! */
#define SIO_XMTDONE_INTERVAL  15
#define SIO_SERIN_INTERVAL     8
#define SIO_SEROUT_INTERVAL    8
#define SIO_ACK_INTERVAL      36

/* These functions are also used by the 1450XLD Parallel disk device */
#define SIO_format_sectorcount (Atari800_default->sio.format_sectorcount)
#define SIO_format_sectorsize  (Atari800_default->sio.format_sectorsize)
int SIO_ReadStatusBlock(int unit, UBYTE *buffer);
int SIO_FormatDisk(int unit, UBYTE *buffer, int sectsize, int sectcount);
void SIO_SizeOfSector(UBYTE unit, int sector, int *sz, ULONG *ofs);
int SIO_ReadSector(int unit, int sector, UBYTE *buffer);
int SIO_DriveStatus(int unit, UBYTE *buffer);
int SIO_WriteStatusBlock(int unit, const UBYTE *buffer);
int SIO_WriteSector(int unit, int sector, const UBYTE *buffer);
void SIO_StateSave(void);
void SIO_StateRead(void);

#endif	/* SIO_H_ */
