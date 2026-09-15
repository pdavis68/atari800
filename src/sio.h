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

/* Context-aware entry points (Option C). The legacy names below are
   forwarding macros that pass the default instance, so not-yet-migrated
   callers are unchanged. SIO_ChkSum is stateless and keeps its signature. */
int SIO_Mount_Ctx(Atari800_Instance *inst, int diskno, const char *filename, int b_open_readonly);
void SIO_Dismount_Ctx(Atari800_Instance *inst, int diskno);
void SIO_DisableDrive_Ctx(Atari800_Instance *inst, int diskno);
int SIO_RotateDisks_Ctx(Atari800_Instance *inst);
void SIO_Handler_Ctx(Atari800_Instance *inst);

UBYTE SIO_ChkSum(const UBYTE *buffer, int length);
void SIO_SwitchCommandFrame_Ctx(Atari800_Instance *inst, int onoff);
#ifdef NETSIO
void NetSIO_PutByte(int byte);
#endif /* NETSIO */
void SIO_PutByte_Ctx(Atari800_Instance *inst, int byte);
int SIO_GetByte_Ctx(Atari800_Instance *inst);
int SIO_Initialise_Ctx(Atari800_Instance *inst, int *argc, char *argv[]);
void SIO_Exit_Ctx(Atari800_Instance *inst);

#define SIO_Mount(diskno, filename, ro) SIO_Mount_Ctx(Atari800_default, diskno, filename, ro)
#define SIO_Dismount(diskno)            SIO_Dismount_Ctx(Atari800_default, diskno)
#define SIO_DisableDrive(diskno)        SIO_DisableDrive_Ctx(Atari800_default, diskno)
#define SIO_RotateDisks()               SIO_RotateDisks_Ctx(Atari800_default)
/* SIO_Handler is registered as a function pointer (esc.c), so it stays a
   real function pinning the default instance. */
void SIO_Handler(void);
#define SIO_SwitchCommandFrame(onoff)   SIO_SwitchCommandFrame_Ctx(Atari800_default, onoff)
#define SIO_PutByte(byte)               SIO_PutByte_Ctx(Atari800_default, byte)
#define SIO_GetByte()                   SIO_GetByte_Ctx(Atari800_default)
#define SIO_Initialise(argc, argv)      SIO_Initialise_Ctx(Atari800_default, argc, argv)
#define SIO_Exit()                      SIO_Exit_Ctx(Atari800_default)

/* Some defines about the serial I/O timing. Currently fixed! */
#define SIO_XMTDONE_INTERVAL  15
#define SIO_SERIN_INTERVAL     8
#define SIO_SEROUT_INTERVAL    8
#define SIO_ACK_INTERVAL      36

/* These functions are also used by the 1450XLD Parallel disk device */
#define SIO_format_sectorcount (Atari800_default->sio.format_sectorcount)
#define SIO_format_sectorsize  (Atari800_default->sio.format_sectorsize)
int SIO_ReadStatusBlock_Ctx(Atari800_Instance *inst, int unit, UBYTE *buffer);
int SIO_FormatDisk_Ctx(Atari800_Instance *inst, int unit, UBYTE *buffer, int sectsize, int sectcount);
void SIO_SizeOfSector_Ctx(Atari800_Instance *inst, UBYTE unit, int sector, int *sz, ULONG *ofs);
int SIO_ReadSector_Ctx(Atari800_Instance *inst, int unit, int sector, UBYTE *buffer);
int SIO_DriveStatus_Ctx(Atari800_Instance *inst, int unit, UBYTE *buffer);
int SIO_WriteStatusBlock_Ctx(Atari800_Instance *inst, int unit, const UBYTE *buffer);
int SIO_WriteSector_Ctx(Atari800_Instance *inst, int unit, int sector, const UBYTE *buffer);
void SIO_StateSave_Ctx(Atari800_Instance *inst);
void SIO_StateRead_Ctx(Atari800_Instance *inst);

#define SIO_ReadStatusBlock(unit, buf)  SIO_ReadStatusBlock_Ctx(Atari800_default, unit, buf)
#define SIO_FormatDisk(unit, buf, ss, sc) SIO_FormatDisk_Ctx(Atari800_default, unit, buf, ss, sc)
#define SIO_SizeOfSector(unit, sec, sz, ofs) SIO_SizeOfSector_Ctx(Atari800_default, unit, sec, sz, ofs)
#define SIO_ReadSector(unit, sec, buf)  SIO_ReadSector_Ctx(Atari800_default, unit, sec, buf)
#define SIO_DriveStatus(unit, buf)      SIO_DriveStatus_Ctx(Atari800_default, unit, buf)
#define SIO_WriteStatusBlock(unit, buf) SIO_WriteStatusBlock_Ctx(Atari800_default, unit, buf)
#define SIO_WriteSector(unit, sec, buf) SIO_WriteSector_Ctx(Atari800_default, unit, sec, buf)
#define SIO_StateSave()                 SIO_StateSave_Ctx(Atari800_default)
#define SIO_StateRead()                 SIO_StateRead_Ctx(Atari800_default)

#endif	/* SIO_H_ */
