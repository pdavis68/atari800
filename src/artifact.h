#ifndef ARTIFACT_H_
#define ARTIFACT_H_

#include <stdio.h>

#include "config.h"

typedef enum ARTIFACT_t {
	ARTIFACT_NONE,       /* Artifacting disabled */
	ARTIFACT_NTSC_OLD,   /* Original NTSC artifacting */
	ARTIFACT_NTSC_NEW,   /* New NTSC artifacting */
#if NTSC_FILTER
	ARTIFACT_NTSC_FULL,  /* NTSC filter */
#endif /* NTSC_FILTER */
#ifndef NO_SIMPLE_PAL_BLENDING
	ARTIFACT_PAL_SIMPLE, /* ANTIC-level simple PAL blending */
#endif /* NO_SIMPLE_PAL_BLENDING */
#ifdef PAL_BLENDING
	ARTIFACT_PAL_BLEND,  /* Accurate PAL blending */
#endif /* PAL_BLENDING */
	ARTIFACT_SIZE
} ARTIFACT_t;

/* Forward declaration for the context-aware API (defined in instance.h). */
struct Atari800_Instance;

/* Set artifacting mode for the current TV system. */
void ARTIFACT_Set_Ctx(struct Atari800_Instance *inst, ARTIFACT_t mode);

/* Call after updating Atari800_tv_mode to update the artifacting mode accordingly. */
void ARTIFACT_SetTVMode_Ctx(struct Atari800_Instance *inst, int tv_mode);

/* Read/write to configuration file. */
void ARTIFACT_WriteConfig_Ctx(struct Atari800_Instance *inst, FILE *fp);
int ARTIFACT_ReadConfig_Ctx(struct Atari800_Instance *inst, char *option, char *ptr);

/* Module initialisation and processing of command-line arguments. */
int ARTIFACT_Initialise_Ctx(struct Atari800_Instance *inst, int *argc, char *argv[]);

#include "instance.h"

/* The currently used artifact emulation mode. Use ARTIFACT_Set to change this value. */
#define ARTIFACT_mode (Atari800_default->artifact.mode)

#define ARTIFACT_Set(mode)              ARTIFACT_Set_Ctx(Atari800_default, (mode))
#define ARTIFACT_SetTVMode(tv_mode)     ARTIFACT_SetTVMode_Ctx(Atari800_default, (tv_mode))
#define ARTIFACT_WriteConfig(fp)        ARTIFACT_WriteConfig_Ctx(Atari800_default, (fp))
#define ARTIFACT_ReadConfig(option, ptr) ARTIFACT_ReadConfig_Ctx(Atari800_default, (option), (ptr))
#define ARTIFACT_Initialise(argc, argv) ARTIFACT_Initialise_Ctx(Atari800_default, (argc), (argv))

#endif /* ARTIFACT_H_ */
