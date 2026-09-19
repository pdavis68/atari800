#ifndef COLOURS_H_
#define COLOURS_H_

#include "colours_external.h"

typedef enum {
	COLOURS_PRESET_STANDARD,
	COLOURS_PRESET_DEEPBLACK,
	COLOURS_PRESET_VIBRANT,
	COLOURS_PRESET_CUSTOM,
	/* Number of "normal" (not including CUSTOM) values in enumerator */
	COLOURS_PRESET_SIZE = COLOURS_PRESET_CUSTOM
} Colours_preset_t;

/* Contains controls for palette adjustment. These controls are available for
   NTSC and PAL palettes. */
typedef struct Colours_setup_t {
	double hue; /* TV tint control */
	double saturation;
	double contrast;
	double brightness;
	double gamma;
	/* Delay between phases of two consecutive chromas, in degrees.
	   Corresponds to the color adjustment potentiometer on the bottom of
	   Atari computers. */
	double color_delay;
	int black_level; /* 0..255. ITU-R Recommendation BT.601 advises it to be 16. */
	int white_level; /* 0..255. ITU-R Recommendation BT.601 advises it to be 235. */
} Colours_setup_t;

/* Limits for the adjustable values. */
#define COLOURS_HUE_MIN -1.0
#define COLOURS_HUE_MAX 1.0
#define COLOURS_SATURATION_MIN -1.0
#define COLOURS_SATURATION_MAX 1.0
#define COLOURS_CONTRAST_MIN -2.0
#define COLOURS_CONTRAST_MAX 2.0
#define COLOURS_BRIGHTNESS_MIN -2.0
#define COLOURS_BRIGHTNESS_MAX 2.0
#define COLOURS_GAMMA_MIN 1.0
#define COLOURS_GAMMA_MAX 3.5
#define COLOURS_DELAY_MIN 10
#define COLOURS_DELAY_MAX 50

/* NOTE: instance.h includes this header (for the Colours_setup_t and
   COLOURS_EXTERNAL_t types used by Colours_state_t) before defining
   Atari800_Instance, so the prototypes below use a forward declaration of
   the struct tag to break the include cycle. */
struct Atari800_Instance;

/* Pointer to the current palette setup. Depending on the current TV system,
   it points to the NTSC setup, or the PAL setup. (See COLOURS_NTSC_setup and
   COLOURS_PAL_setup.) */
void Colours_SetRGB_Ctx(struct Atari800_Instance *inst, int i, int r, int g, int b, int *colortable_ptr);

/* Called when the TV system changes, it updates the current palette
   accordingly. */
void Colours_SetVideoSystem_Ctx(struct Atari800_Instance *inst, int mode);

/* Updates the current palette - should be called after changing palette setup
   or loading/unloading an external palette. */
void Colours_Update_Ctx(struct Atari800_Instance *inst);
/* Restores default setup for the current palette (NTSC or PAL one).
   Colours_Update should be called afterwards to apply changes. */
void Colours_RestoreDefaults_Ctx(struct Atari800_Instance *inst);
/* Save the current colours, including adjustments, to a palette file.
   Returns TRUE on success or FALSE on error. */
int Colours_Save_Ctx(struct Atari800_Instance *inst, const char *filename);

/* Initialise variables before loading from config file. */
void Colours_PreInitialise_Ctx(struct Atari800_Instance *inst);

/* Read/write to configuration file. */
int Colours_ReadConfig_Ctx(struct Atari800_Instance *inst, char *option, char *ptr);
void Colours_WriteConfig_Ctx(struct Atari800_Instance *inst, FILE *fp);

/* Colours initialisation and processing of command-line arguments. */
int Colours_Initialise_Ctx(struct Atari800_Instance *inst, int *argc, char *argv[]);

/* Functions for setting and getting the color preset. PRESET cannot equal
   COLOURS_PRESET_CUSTOM. */
void Colours_SetPreset_Ctx(struct Atari800_Instance *inst, Colours_preset_t preset);
Colours_preset_t Colours_GetPreset_Ctx(struct Atari800_Instance *inst);

/* Convert given R, G and B values to corresponding Y, U, V values (all in the
   0.0 - 1.0 range). */
void Colours_RGB2YUV(double r, double g, double b, double *y, double *u, double *v);
/* Convert given Y, U and V values to corresponding R, G, B values (all in the
   0.0 - 1.0 range). */
void Colours_YUV2RGB(double y, double u, double v, double *r, double *g, double *b);

/* Converts a gamma-adjusted color value c (0 <= c <= 1) into linear value. */
double Colours_Gamma2Linear(double c, double gamma_adj);
/* Converts a linear color value c (0 <= c <= 1) into sRGB gamma-corrected
   value. */
double Colours_Linear2sRGB(double c);

#include "instance.h" /* Atari800_Instance, Atari800_default (transitional) */

/* Transitional Option C bridge: the per-instance Colours state lives in
   Colours_state_t (instance.h). Until all callers pass an instance
   explicitly, the legacy global names are aliased to the default instance.
   Colours_table lives in Colours_state_t.table; sdl/palette.c resolves the
   pointer at runtime (SDL_PALETTE_Initialise). */

#define Colours_setup    (Atari800_default->colours.setup)
#define Colours_external (Atari800_default->colours.external)
#define Colours_table    (Atari800_default->colours.table)

/* The legacy names below are forwarding macros that pass the default
   instance, so not-yet-migrated callers are unchanged. */
#define Colours_SetRGB(i, r, g, b, colortable_ptr) \
	Colours_SetRGB_Ctx(Atari800_default, (i), (r), (g), (b), (colortable_ptr))
#define Colours_SetVideoSystem(mode) \
	Colours_SetVideoSystem_Ctx(Atari800_default, (mode))
#define Colours_Update() \
	Colours_Update_Ctx(Atari800_default)
#define Colours_RestoreDefaults() \
	Colours_RestoreDefaults_Ctx(Atari800_default)
#define Colours_Save(filename) \
	Colours_Save_Ctx(Atari800_default, (filename))
#define Colours_PreInitialise() \
	Colours_PreInitialise_Ctx(Atari800_default)
#define Colours_ReadConfig(option, ptr) \
	Colours_ReadConfig_Ctx(Atari800_default, (option), (ptr))
#define Colours_WriteConfig(fp) \
	Colours_WriteConfig_Ctx(Atari800_default, (fp))
#define Colours_Initialise(argc, argv) \
	Colours_Initialise_Ctx(Atari800_default, (argc), (argv))
#define Colours_SetPreset(preset) \
	Colours_SetPreset_Ctx(Atari800_default, (preset))
#define Colours_GetPreset() \
	Colours_GetPreset_Ctx(Atari800_default)

#define Colours_GetR(x) ((UBYTE) (Colours_table[x] >> 16))
#define Colours_GetG(x) ((UBYTE) (Colours_table[x] >> 8))
#define Colours_GetB(x) ((UBYTE) Colours_table[x])

#endif /* COLOURS_H_ */
