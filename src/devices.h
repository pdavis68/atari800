#ifndef DEVICES_H_
#define DEVICES_H_

#include <stdio.h> /* FILENAME_MAX */
#include "atari.h" /* UWORD */
#include "instance.h" /* Devices_state_t (transitional default-instance aliases) */

int Devices_Initialise(int *argc, char *argv[]);
void Devices_Exit(void);
int Devices_PatchOS(void);
void Devices_Frame(void);
void Devices_UpdatePatches(void);

UWORD Devices_SkipDeviceName(void);

/* Transitional Option C bridge: the per-instance Devices state lives in
   Devices_state_t (instance.h). Until all callers pass an instance
   explicitly, the legacy global names are aliased to the default instance. */
#define Devices_enable_h_patch (Atari800_default->devices.enable_h_patch)
#define Devices_enable_p_patch (Atari800_default->devices.enable_p_patch)
#define Devices_enable_r_patch (Atari800_default->devices.enable_r_patch)
#define Devices_enable_b_patch (Atari800_default->devices.enable_b_patch)

#define Devices_atari_h_dir    (Atari800_default->devices.atari_h_dir)
#define Devices_h_read_only    (Atari800_default->devices.h_read_only)

#define Devices_h_exe_path     (Atari800_default->devices.h_exe_path)

#define Devices_h_device_name  (Atari800_default->devices.h_device_name)

#define Devices_h_current_dir  (Atari800_default->devices.h_current_dir)

int Devices_H_CountOpen(void);
void Devices_H_CloseAll(void);

#define Devices_print_command  (Atari800_default->devices.print_command)

int Devices_SetPrintCommand(const char *command);

#define dev_b_status (Atari800_default->devices.dev_b_status)


#define	Devices_ICHIDZ	0x0020
#define	Devices_ICDNOZ	0x0021
#define	Devices_ICCOMZ	0x0022
#define	Devices_ICSTAZ	0x0023
#define	Devices_ICBALZ	0x0024
#define	Devices_ICBAHZ	0x0025
#define	Devices_ICPTLZ	0x0026
#define	Devices_ICPTHZ	0x0027
#define	Devices_ICBLLZ	0x0028
#define	Devices_ICBLHZ	0x0029
#define	Devices_ICAX1Z	0x002a
#define	Devices_ICAX2Z	0x002b

#define Devices_IOCB0   0x0340
#define	Devices_ICHID	0x0000
#define	Devices_ICDNO	0x0001
#define	Devices_ICCOM	0x0002
#define	Devices_ICSTA	0x0003
#define	Devices_ICBAL	0x0004
#define	Devices_ICBAH	0x0005
#define	Devices_ICPTL	0x0006
#define	Devices_ICPTH	0x0007
#define	Devices_ICBLL	0x0008
#define	Devices_ICBLH	0x0009
#define	Devices_ICAX1	0x000a
#define	Devices_ICAX2	0x000b
#define	Devices_ICAX3	0x000c
#define	Devices_ICAX4	0x000d
#define	Devices_ICAX5	0x000e
#define	Devices_ICAX6	0x000f

#define Devices_TABLE_OPEN	0
#define Devices_TABLE_CLOS	2
#define Devices_TABLE_READ	4
#define Devices_TABLE_WRIT	6
#define Devices_TABLE_STAT	8
#define Devices_TABLE_SPEC	10
#define Devices_TABLE_INIT	12

UWORD Devices_UpdateHATABSEntry(char device, UWORD entry_address, UWORD table_address);
void Devices_RemoveHATABSEntry(char device, UWORD entry_address, UWORD table_address);

#endif /* DEVICES_H_ */
