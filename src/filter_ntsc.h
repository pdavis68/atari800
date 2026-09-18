#ifndef FILTER_NTSC_H_
#define FILTER_NTSC_H_

#include <stdio.h>
#include "atari_ntsc/atari_ntsc.h"

/* Limits for the adjustable values. */
#define FILTER_NTSC_SHARPNESS_MIN -1.0
#define FILTER_NTSC_SHARPNESS_MAX 1.0
#define FILTER_NTSC_RESOLUTION_MIN -1.0
#define FILTER_NTSC_RESOLUTION_MAX 1.0
#define FILTER_NTSC_ARTIFACTS_MIN -1.0
#define FILTER_NTSC_ARTIFACTS_MAX 1.0
#define FILTER_NTSC_FRINGING_MIN -1.0
#define FILTER_NTSC_FRINGING_MAX 1.0
#define FILTER_NTSC_BLEED_MIN -1.0
#define FILTER_NTSC_BLEED_MAX 1.0
#define FILTER_NTSC_BURST_PHASE_MIN -1.0
#define FILTER_NTSC_BURST_PHASE_MAX 1.0

/* Forward declaration for the context-aware API (defined in instance.h). */
struct Atari800_Instance;

/* Allocates memory for a new NTSC filter. Stateless (no instance state). */
atari_ntsc_t *FILTER_NTSC_New(void);
/* Frees memory used by an NTSC filter, FILTER. Stateless (no instance state). */
void FILTER_NTSC_Delete(atari_ntsc_t *filter);
/* Reinitialises an NTSC filter, FILTER. Should be called after changing
   palette setup or loading/unloading an external palette. */
void FILTER_NTSC_Update_Ctx(struct Atari800_Instance *inst, atari_ntsc_t *filter);
/* Restores default values for NTSC-filter-specific colour controls.
   FILTER_NTSC_Update should be called afterwards to apply changes. */
void FILTER_NTSC_RestoreDefaults_Ctx(struct Atari800_Instance *inst);

/* Set/get one of the available preset adjustments: Composite, S-Video, RGB,
   Monochrome. */
enum {
	FILTER_NTSC_PRESET_COMPOSITE,
	FILTER_NTSC_PRESET_SVIDEO,
	FILTER_NTSC_PRESET_RGB,
	FILTER_NTSC_PRESET_MONOCHROME,
	FILTER_NTSC_PRESET_CUSTOM,
	/* Number of "normal" (not including CUSTOM) values in enumerator */
	FILTER_NTSC_PRESET_SIZE = FILTER_NTSC_PRESET_CUSTOM
};
/* FILTER_NTSC_Update should be called afterwards these functions to apply changes. */
void FILTER_NTSC_SetPreset_Ctx(struct Atari800_Instance *inst, int preset);
int FILTER_NTSC_GetPreset_Ctx(struct Atari800_Instance *inst);
void FILTER_NTSC_NextPreset_Ctx(struct Atari800_Instance *inst);

/* Initialise variables before loading from config file. */
void FILTER_NTSC_PreInitialise_Ctx(struct Atari800_Instance *inst);

/* Read/write to configuration file. */
int FILTER_NTSC_ReadConfig_Ctx(struct Atari800_Instance *inst, char *option, char *ptr);
void FILTER_NTSC_WriteConfig_Ctx(struct Atari800_Instance *inst, FILE *fp);

/* NTSC filter initialisation and processing of command-line arguments. */
int FILTER_NTSC_Initialise_Ctx(struct Atari800_Instance *inst, int *argc, char *argv[]);

#include "instance.h"

/* Contains controls used to adjust the palette in the NTSC filter. */
#define FILTER_NTSC_setup (Atari800_default->filter_ntsc.setup)
/* Pointer to the NTSC filter structure. Initialise it by setting it to value
   returned by FILTER_NTSC_New(). */
#define FILTER_NTSC_emu   (Atari800_default->filter_ntsc.emu)

#define FILTER_NTSC_Update(filter)          FILTER_NTSC_Update_Ctx(Atari800_default, (filter))
#define FILTER_NTSC_RestoreDefaults()       FILTER_NTSC_RestoreDefaults_Ctx(Atari800_default)
#define FILTER_NTSC_SetPreset(preset)       FILTER_NTSC_SetPreset_Ctx(Atari800_default, (preset))
#define FILTER_NTSC_GetPreset()             FILTER_NTSC_GetPreset_Ctx(Atari800_default)
#define FILTER_NTSC_NextPreset()            FILTER_NTSC_NextPreset_Ctx(Atari800_default)
#define FILTER_NTSC_PreInitialise()         FILTER_NTSC_PreInitialise_Ctx(Atari800_default)
#define FILTER_NTSC_ReadConfig(option, ptr) FILTER_NTSC_ReadConfig_Ctx(Atari800_default, (option), (ptr))
#define FILTER_NTSC_WriteConfig(fp)         FILTER_NTSC_WriteConfig_Ctx(Atari800_default, (fp))
#define FILTER_NTSC_Initialise(argc, argv)  FILTER_NTSC_Initialise_Ctx(Atari800_default, (argc), (argv))

#endif /* FILTER_NTSC_H_ */
