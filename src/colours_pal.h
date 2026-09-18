#ifndef COLOURS_PAL_H_
#define COLOURS_PAL_H_

#include "colours.h"
#include "colours_external.h"

/* NOTE: colours.h includes instance.h, so Atari800_Instance is available
   here. The per-instance PAL palette state lives in Colours_state_t
   (instance.h); the legacy global names below are aliased to the default
   instance (transitional). */
#define COLOURS_PAL_setup    (Atari800_default->colours.pal_setup)
#define COLOURS_PAL_external (Atari800_default->colours.pal_external)

/* Updates the PAL palette - should be called after changing palette setup
   or loading/unloading an external palette. */
void COLOURS_PAL_Update_Ctx(Atari800_Instance *inst, int colourtable[256]);

/* Restores default values for PAL-specific colour controls.
   Colours_PAL_Update should be called afterwards to apply changes. */
void COLOURS_PAL_RestoreDefaults_Ctx(Atari800_Instance *inst);

/* Read/write to configuration file. */
int COLOURS_PAL_ReadConfig_Ctx(Atari800_Instance *inst, char *option, char *ptr);
void COLOURS_PAL_WriteConfig_Ctx(Atari800_Instance *inst, FILE *fp);

/* PAL Colours initialisation and processing of command-line arguments. */
int COLOURS_PAL_Initialise_Ctx(Atari800_Instance *inst, int *argc, char *argv[]);

/* Function for getting the PAL-specific color preset. */
Colours_preset_t COLOURS_PAL_GetPreset_Ctx(Atari800_Instance *inst);

/* Writes the PAL palette (generated or external) as {Y, even U, odd U,
   even V, odd V} quintuples in YUV_TABLE. */
void COLOURS_PAL_GetYUV_Ctx(Atari800_Instance *inst, double yuv_table[256*5]);

/* The legacy names below are forwarding macros that pass the default
   instance, so not-yet-migrated callers are unchanged. */
#define COLOURS_PAL_Update(colourtable) \
	COLOURS_PAL_Update_Ctx(Atari800_default, (colourtable))
#define COLOURS_PAL_RestoreDefaults() \
	COLOURS_PAL_RestoreDefaults_Ctx(Atari800_default)
#define COLOURS_PAL_GetYUV(yuv_table) \
	COLOURS_PAL_GetYUV_Ctx(Atari800_default, (yuv_table))
#define COLOURS_PAL_ReadConfig(option, ptr) \
	COLOURS_PAL_ReadConfig_Ctx(Atari800_default, (option), (ptr))
#define COLOURS_PAL_WriteConfig(fp) \
	COLOURS_PAL_WriteConfig_Ctx(Atari800_default, (fp))
#define COLOURS_PAL_Initialise(argc, argv) \
	COLOURS_PAL_Initialise_Ctx(Atari800_default, (argc), (argv))
#define COLOURS_PAL_GetPreset() \
	COLOURS_PAL_GetPreset_Ctx(Atari800_default)

#endif /* COLOURS_PAL_H_ */
