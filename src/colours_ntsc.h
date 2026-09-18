#ifndef COLOURS_NTSC_H_
#define COLOURS_NTSC_H_

#include "config.h"
#include "colours.h"
#include "colours_external.h"

#ifndef M_PI
# define M_PI 3.141592653589793
#endif

/* NOTE: colours.h includes instance.h, so Atari800_Instance is available
   here. The per-instance NTSC palette state lives in Colours_state_t
   (instance.h); the legacy global names below are aliased to the default
   instance (transitional). */
#define COLOURS_NTSC_setup    (Atari800_default->colours.ntsc_setup)
#define COLOURS_NTSC_external (Atari800_default->colours.ntsc_external)

/* Updates the NTSC palette - should be called after changing palette setup
   or loading/unloading an external palette. */
void COLOURS_NTSC_Update_Ctx(Atari800_Instance *inst, int colourtable[256]);
/* Restores default values for NTSC-specific colour controls.
   Colours_NTSC_Update should be called afterwards to apply changes. */
void COLOURS_NTSC_RestoreDefaults_Ctx(Atari800_Instance *inst);

/* Writes the NTSC palette (internal or external) as YIQ triplets in
   YIQ_TABLE. START_ANGLE defines the phase shift of chroma #1. This function
   is to be used exclusively by the NTSC Filter module. */
void COLOURS_NTSC_GetYIQ_Ctx(Atari800_Instance *inst, double yiq_table[768], const double start_angle);

/* Read/write to configuration file. */
int COLOURS_NTSC_ReadConfig_Ctx(Atari800_Instance *inst, char *option, char *ptr);
void COLOURS_NTSC_WriteConfig_Ctx(Atari800_Instance *inst, FILE *fp);

/* NTSC Colours initialisation and processing of command-line arguments. */
int COLOURS_NTSC_Initialise_Ctx(Atari800_Instance *inst, int *argc, char *argv[]);

/* Function for getting the NTSC-specific color preset. */
Colours_preset_t COLOURS_NTSC_GetPreset_Ctx(Atari800_Instance *inst);

/* The legacy names below are forwarding macros that pass the default
   instance, so not-yet-migrated callers are unchanged. */
#define COLOURS_NTSC_Update(colourtable) \
	COLOURS_NTSC_Update_Ctx(Atari800_default, (colourtable))
#define COLOURS_NTSC_RestoreDefaults() \
	COLOURS_NTSC_RestoreDefaults_Ctx(Atari800_default)
#define COLOURS_NTSC_GetYIQ(yiq_table, start_angle) \
	COLOURS_NTSC_GetYIQ_Ctx(Atari800_default, (yiq_table), (start_angle))
#define COLOURS_NTSC_ReadConfig(option, ptr) \
	COLOURS_NTSC_ReadConfig_Ctx(Atari800_default, (option), (ptr))
#define COLOURS_NTSC_WriteConfig(fp) \
	COLOURS_NTSC_WriteConfig_Ctx(Atari800_default, (fp))
#define COLOURS_NTSC_Initialise(argc, argv) \
	COLOURS_NTSC_Initialise_Ctx(Atari800_default, (argc), (argv))
#define COLOURS_NTSC_GetPreset() \
	COLOURS_NTSC_GetPreset_Ctx(Atari800_default)

#endif /* COLOURS_NTSC_H_ */
