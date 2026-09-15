#ifndef PBI_SCSI_H_
#define PBI_SCSI_H_

#include "atari.h"
#include <stdio.h>

#include "instance.h" /* Atari800_Instance (transitional default-instance aliases) */

/* Transitional Option C bridge: the per-instance SCSI state lives in
   SCSI_state_t (instance.h). Until all callers pass an instance
   explicitly, the legacy global names are aliased to the default
   instance. */
#define PBI_SCSI_CD   (Atari800_default->scsi.CD)
#define PBI_SCSI_MSG  (Atari800_default->scsi.MSG)
#define PBI_SCSI_IO   (Atari800_default->scsi.IO)
#define PBI_SCSI_BSY  (Atari800_default->scsi.BSY)
#define PBI_SCSI_REQ  (Atari800_default->scsi.REQ)
#define PBI_SCSI_ACK  (Atari800_default->scsi.ACK)
#define PBI_SCSI_SEL  (Atari800_default->scsi.SEL)
#define PBI_SCSI_disk (Atari800_default->scsi.disk)

/* Context-aware entry points (Option C). The legacy names below are
   forwarding macros that pass the default instance, so not-yet-migrated
   callers (pbi_bb.c, pbi_mio.c) are unchanged; they will be re-routed
   per-instance as those modules become instance-aware. */
void PBI_SCSI_PutByte_Ctx(Atari800_Instance *inst, UBYTE byte);
UBYTE PBI_SCSI_GetByte_Ctx(Atari800_Instance *inst);
void PBI_SCSI_PutSEL_Ctx(Atari800_Instance *inst, int newsel);
void PBI_SCSI_PutACK_Ctx(Atari800_Instance *inst, int newack);

#define PBI_SCSI_PutByte(byte)  PBI_SCSI_PutByte_Ctx(Atari800_default, byte)
#define PBI_SCSI_GetByte()      PBI_SCSI_GetByte_Ctx(Atari800_default)
#define PBI_SCSI_PutSEL(sel)    PBI_SCSI_PutSEL_Ctx(Atari800_default, sel)
#define PBI_SCSI_PutACK(ack)    PBI_SCSI_PutACK_Ctx(Atari800_default, ack)

#endif /* PBI_MIO_H_ */
