/*
 * colours.c - Atari colour palette adjustment - functions common for NTSC and
 *             PAL palettes
 *
 * Copyright (C) 1995-1998 David Firth
 * Copyright (C) 1998-2010 Atari800 development team (see DOC/CREDITS)
 *
 * This file is part of the Atari800 emulator project which emulates
 * the Atari 400, 800, 800XL, 130XE, and 5200 8-bit computers.
 *
 * Atari800 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Atari800 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Atari800; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include <stdio.h>
#include <string.h>	/* for strcmp() */
#include <math.h>
#include <stdlib.h>
#include "atari.h"
#include "cfg.h"
#include "colours.h"
#include "colours_external.h"
#include "colours_ntsc.h"
#include "colours_pal.h"
#include "log.h"
#include "util.h"
#include "platform.h"

#ifndef M_PI
#define M_PI		3.14159265358979323846
#endif

/* Per-instance context (Option C transitional pattern): the _Ctx entry
   points pin the file-scope context below; the legacy state names are
   re-pointed to the pinned instance's Colours_state_t (including
   Colours_table, which aliases CO->table inside this file).
   references it from a static initialiser. */
static Atari800_Instance *COI;
static Colours_state_t *CO;

#undef Colours_setup
#undef Colours_external
#define Colours_setup    (CO->setup)
#define Colours_external (CO->external)

#define COLOURS_PIN_CTX(inst) \
	do { COI = (inst); CO = &COI->colours; } while (0)

/* The NTSC and PAL TV systems maintain that the gamma factor of CRT TV
   receivers should be 2.5 and 2.8, respectively. However, typical CRT TVs
   (both NTSC and PAL) would have the gamma factor somewhere in the range
   between 2.35 and 2.55. Therefore we choose 2.35 as the default gamma for
   both TV systems. Reference: sections 10, 11 and 19 of
   http://www.poynton.com/notes/colour_and_gamma/GammaFAQ.html */
static Colours_setup_t const presets[] = {
	/* Hue, Saturation, Contrast, Brightness, Gamma adjustment, GTIA delay, Black level, White level */
	{ 0.0, 0.0, 0.0, 0.0, 2.35, 0.0, 16, 235 }, /* Standard preset */
	{ 0.0, 0.0, 0.08, -0.08, 2.35, 0.0, 16, 235 }, /* Deep blacks preset */
	{ 0.0, 0.26, 0.72, -0.16, 2.00, 0.0, 16, 235 } /* Vibrant colours & levels preset */
};
static char const * const preset_cfg_strings[COLOURS_PRESET_SIZE] = {
	"STANDARD",
	"DEEP-BLACK",
	"VIBRANT"
};

/* The computed palette now lives in Colours_state_t.table (instance.h);
   inside this file it is re-pointed to the pinned context. */
#undef Colours_table
#define Colours_table (CO->table)

void Colours_SetRGB_Ctx(Atari800_Instance *inst, int i, int r, int g, int b, int *colortable_ptr)
{
	(void) inst; /* stateless */
	if (r < 0)
		r = 0;
	else if (r > 255)
		r = 255;
	if (g < 0)
		g = 0;
	else if (g > 255)
		g = 255;
	if (b < 0)
		b = 0;
	else if (b > 255)
		b = 255;
	colortable_ptr[i] = (r << 16) + (g << 8) + b;
}

/* 3x3 matrix for conversion from RGB to YUV colourspace. */
static double const RGB2YUV_matrix[3][3] = {
	{ 0.299, 0.587, 0.114 },
	{ -0.14713, -0.28886, 0.436 },
	{ 0.615, -0.51499, -0.10001 }
};

/* 3x3 matrix for conversion from YUV to RGB colourspace. */
static double const YUV2RGB_matrix[3][3] = {
	{ 1.0, 0.0, 1.13983 },
	{ 1.0, -0.39465, -0.58060 },
	{ 1.0, 2.03211, 0.0 }
};

/* Multiply 3x3 matrix MATRIX by vector IN[1..3] and return result in OUT[1..3]. */
static void MultiplyMatrix(double in1, double in2, double in3, double *out1, double *out2, double *out3, double const matrix[3][3])
{
	*out1 = matrix[0][0] * in1 + matrix[0][1] * in2 + matrix[0][2] * in3;
	*out2 = matrix[1][0] * in1 + matrix[1][1] * in2 + matrix[1][2] * in3;
	*out3 = matrix[2][0] * in1 + matrix[2][1] * in2 + matrix[2][2] * in3;
}

void Colours_RGB2YUV(double r, double g, double b, double *y, double *u, double *v)
{
	MultiplyMatrix(r, g, b, y, u, v, RGB2YUV_matrix);
}

void Colours_YUV2RGB(double y, double u, double v, double *r, double *g, double *b)
{
	MultiplyMatrix(y, u, v, r, g, b, YUV2RGB_matrix);
}

double Colours_Gamma2Linear(double c, double gamma_adj)
{
	if (c >= 0.0)
		return pow(c, gamma_adj);
	else
		/* Can't do pow() on a negative c. Let's divide c by the same value as
		   in Colours_Linear2sRGB, so that a later conversion to sRGB will
		   result in the value of c being unchanged. */
		return c / 12.92;
}

double Colours_Linear2sRGB(double c)
{
	/* Source: https://en.wikipedia.org/wiki/SRGB */
	if (c <= 0.0031308)
		return c * 12.92;
	else
		return 1.055 * pow(c, 1.0/2.4) - 0.055;
}

static void UpdateModeDependentPointers_Ctx(Atari800_Instance *inst, int tv_mode)
{
	/* Set pointers to the current setup and external palette. */
	if (tv_mode == Atari800_TV_NTSC) {
		Colours_setup = &CO->ntsc_setup;
		Colours_external = &CO->ntsc_external;
	}
       	else if (tv_mode == Atari800_TV_PAL) {
		Colours_setup = &CO->pal_setup;
		Colours_external = &CO->pal_external;
	}
	else {
		Atari800_ErrExit();
		Log_print("Interal error: Invalid Atari800_tv_mode\n");
		exit(1);
	}
}

void Colours_SetVideoSystem_Ctx(Atari800_Instance *inst, int mode)
{
	COLOURS_PIN_CTX(inst);
	UpdateModeDependentPointers_Ctx(inst, mode);
	/* Apply changes */
	Colours_Update_Ctx(inst);
}

/* Copies the loaded external palette into current palette - without applying
   adjustments. */
static void CopyExternalWithoutAdjustments_Ctx(Atari800_Instance *inst)
{
	int i;
	unsigned char *ext_ptr;
	for (i = 0, ext_ptr = Colours_external->palette; i < 256; i ++, ext_ptr += 3)
		Colours_SetRGB_Ctx(inst, i, *ext_ptr, *(ext_ptr + 1), *(ext_ptr + 2), Colours_table);
}

/* Updates contents of Colours_table. */
static void UpdatePalette_Ctx(Atari800_Instance *inst)
{
	if (Colours_external->loaded && !Colours_external->adjust)
		CopyExternalWithoutAdjustments_Ctx(inst);
	else if (Atari800_tv_mode == Atari800_TV_NTSC)
		COLOURS_NTSC_Update_Ctx(inst, Colours_table);
	else /* PAL */
		COLOURS_PAL_Update_Ctx(inst, Colours_table);
}

void Colours_Update_Ctx(Atari800_Instance *inst)
{
	COLOURS_PIN_CTX(inst);
	UpdatePalette_Ctx(inst);
#if SUPPORTS_PLATFORM_PALETTEUPDATE
	PLATFORM_PaletteUpdate();
#endif
}

void Colours_RestoreDefaults_Ctx(Atari800_Instance *inst)
{
	Colours_SetPreset_Ctx(inst, COLOURS_PRESET_STANDARD);
}

/* Sets the video calibration profile to the user preference */
void Colours_SetPreset_Ctx(Atari800_Instance *inst, Colours_preset_t preset)
{
	COLOURS_PIN_CTX(inst);
	if (preset < COLOURS_PRESET_CUSTOM) {
		*Colours_setup = presets[preset];
		if (Atari800_tv_mode == Atari800_TV_NTSC)
			COLOURS_NTSC_RestoreDefaults_Ctx(inst);
		else
			COLOURS_PAL_RestoreDefaults_Ctx(inst);
	}
}

/* Compares the current settings to the available calibration profiles
   and returns the matching profile -- or CUSTOM if no match is found */
Colours_preset_t Colours_GetPreset_Ctx(Atari800_Instance *inst)
{
	int i;

	COLOURS_PIN_CTX(inst);
	if ((Atari800_tv_mode == Atari800_TV_NTSC &&
	     COLOURS_NTSC_GetPreset_Ctx(inst) != COLOURS_PRESET_STANDARD) ||
	    (Atari800_tv_mode == Atari800_TV_PAL &&
	     COLOURS_PAL_GetPreset_Ctx(inst) != COLOURS_PRESET_STANDARD))
		return COLOURS_PRESET_CUSTOM;

	for (i = 0; i < COLOURS_PRESET_SIZE; i ++) {
		if (Util_almostequal(Colours_setup->hue, presets[i].hue, 0.001) &&
		    Util_almostequal(Colours_setup->saturation, presets[i].saturation, 0.001) &&
		    Util_almostequal(Colours_setup->contrast, presets[i].contrast, 0.001) &&
		    Util_almostequal(Colours_setup->brightness, presets[i].brightness, 0.001) &&
		    Util_almostequal(Colours_setup->gamma, presets[i].gamma, 0.001) &&
		    Colours_setup->black_level == presets[i].black_level &&
		    Colours_setup->white_level == presets[i].white_level)
			return (Colours_preset_t)i; 
	}
	return COLOURS_PRESET_CUSTOM;
}

int Colours_Save_Ctx(Atari800_Instance *inst, const char *filename)
{
	COLOURS_PIN_CTX(inst);
	FILE *fp;
	int i;

	fp = fopen(filename, "wb");
	if (fp == NULL) {
		return FALSE;
	}

	/* Create a raw 768-byte file with RGB values. */
	for (i = 0; i < 256; i ++) {
		char rgb[3];
		rgb[0] = Colours_GetR(i);
		rgb[1] = Colours_GetG(i);
		rgb[2] = Colours_GetB(i);
		if (fwrite(rgb, sizeof(rgb), 1, fp) != 1) {
			fclose(fp);
			return FALSE;
		}
	}

	fclose(fp);
	return TRUE;
}

void Colours_PreInitialise_Ctx(Atari800_Instance *inst)
{
	COLOURS_PIN_CTX(inst);
	/* Copy the default setup for both NTSC and PAL. */
	CO->ntsc_setup = CO->pal_setup = presets[COLOURS_PRESET_STANDARD];
	COLOURS_NTSC_RestoreDefaults_Ctx(inst);
	COLOURS_PAL_RestoreDefaults_Ctx(inst);
}

int Colours_ReadConfig_Ctx(Atari800_Instance *inst, char *option, char *ptr)
{
	COLOURS_PIN_CTX(inst);
	if (COLOURS_NTSC_ReadConfig_Ctx(inst, option, ptr)) {
	}
	else if (COLOURS_PAL_ReadConfig_Ctx(inst, option, ptr)) {
	}
	else return FALSE; /* no match */
	return TRUE; /* matched something */
}

void Colours_WriteConfig_Ctx(Atari800_Instance *inst, FILE *fp)
{
	COLOURS_PIN_CTX(inst);
	COLOURS_NTSC_WriteConfig_Ctx(inst, fp);
	COLOURS_PAL_WriteConfig_Ctx(inst, fp);
}

int Colours_Initialise_Ctx(Atari800_Instance *inst, int *argc, char *argv[])
{
	int i;
	int j;

	COLOURS_PIN_CTX(inst);

	for (i = j = 1; i < *argc; i++) {
		int i_a = (i + 1 < *argc);		/* is argument available? */
		int a_m = FALSE;			/* error, argument missing! */
		
		if (strcmp(argv[i], "-saturation") == 0) {
			if (i_a)
				CO->ntsc_setup.saturation = CO->pal_setup.saturation = atof(argv[++i]);
			else a_m = TRUE;
		}
		else if (strcmp(argv[i], "-contrast") == 0) {
			if (i_a)
				CO->ntsc_setup.contrast = CO->pal_setup.contrast = atof(argv[++i]);
			else a_m = TRUE;
		}
		else if (strcmp(argv[i], "-brightness") == 0) {
			if (i_a)
				CO->ntsc_setup.brightness = CO->pal_setup.brightness = atof(argv[++i]);
			else a_m = TRUE;
		}
		else if (strcmp(argv[i], "-gamma") == 0) {
			if (i_a)
				CO->ntsc_setup.gamma = CO->pal_setup.gamma = atof(argv[++i]);
			else a_m = TRUE;
		}
		else if (strcmp(argv[i], "-tint") == 0) {
			if (i_a)
				CO->ntsc_setup.hue = CO->pal_setup.hue = atof(argv[++i]);
			else a_m = TRUE;
		}
		else if (strcmp(argv[i], "-colors-preset") == 0) {
			if (i_a) {
				int idx = CFG_MatchTextParameter(argv[++i], preset_cfg_strings, COLOURS_PRESET_SIZE);
				if (idx < 0) {
					Log_print("Invalid value for -colors-preset");
					return FALSE;
				}
				CO->ntsc_setup = CO->pal_setup = presets[idx];
				COLOURS_NTSC_RestoreDefaults_Ctx(inst);
				COLOURS_PAL_RestoreDefaults_Ctx(inst);
			} else a_m = TRUE;
		}

		else {
			if (strcmp(argv[i], "-help") == 0) {
				Log_print("\t-colors-preset standard|deep-black|vibrant");
				Log_print("\t                       Use one of predefined color adjustments");
				Log_print("\t-saturation <num>      Set color saturation");
				Log_print("\t-contrast <num>        Set contrast");
				Log_print("\t-brightness <num>      Set brightness");
				Log_print("\t-gamma <num>           Set color gamma factor");
				Log_print("\t-tint <num>            Set tint");
			}
			argv[j++] = argv[i];
		}

		if (a_m) {
			Log_print("Missing argument for '%s'", argv[i]);
			return FALSE;
		}
	}
	*argc = j;

	if (!COLOURS_NTSC_Initialise_Ctx(inst, argc, argv) ||
	    !COLOURS_PAL_Initialise_Ctx(inst, argc, argv))
		return FALSE;

	/* Assume that Atari800_tv_mode has been already initialised. */
	UpdateModeDependentPointers_Ctx(inst, Atari800_tv_mode);
	UpdatePalette_Ctx(inst);
	return TRUE;
}
