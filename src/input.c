/*
 * input.c - keyboard, joysticks and mouse emulation
 *
 * Copyright (C) 2001-2002 Piotr Fusik
 * Copyright (C) 2001-2006 Atari800 development team (see DOC/CREDITS)
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
#include <string.h>
#include <stdlib.h> /* for exit() */
#include "antic.h"
#include "atari.h"
#include "cassette.h"
#include "cpu.h"
#include "gtia.h"
#include "input.h"
#include "akey.h"
#include "log.h"
#include "memory.h"
#include "pia.h"
#include "platform.h"
#include "pokey.h"
#include "util.h"
#ifndef CURSES_BASIC
#include "screen.h" /* for Screen_atari */
#endif
#ifdef __PLUS
#include "input_win.h"
#endif
#ifdef EVENT_RECORDING
#include <zlib.h>
#endif

#ifndef MOUSE_SHIFT
#define MOUSE_SHIFT 4
#endif

/* Transitional Option C bridge: the per-instance input state lives in
   Input_state_t (instance.h). The *_Ctx entry points pin the file-scope
   context (IN = &inst->input, INI = inst); inside this file the legacy
   state names route through IN, so the _Ctx bodies operate on their own
   instance. The chip/peripheral register names written by INPUT_Frame
   (POKEY_*, GTIA_*, PIA_*, ANTIC_*, CASSETTE_press_space) are re-pointed
   to the pinned instance as well. */
static Atari800_Instance *INI;
static Input_state_t *IN;

#define INPUT_PIN_CTX(inst) do { \
	INI = (inst); \
	IN = &INI->input; \
} while (0)

/* Route the legacy state names through the pinned context. */
#undef INPUT_key_code
#undef INPUT_key_shift
#undef INPUT_key_consol
#undef INPUT_joy_autofire
#undef INPUT_joy_block_opposite_directions
#undef INPUT_joy_multijoy
#undef INPUT_joy_5200_min
#undef INPUT_joy_5200_center
#undef INPUT_joy_5200_max
#undef INPUT_cx85
#undef INPUT_mouse_mode
#undef INPUT_mouse_port
#undef INPUT_mouse_delta_x
#undef INPUT_mouse_delta_y
#undef INPUT_mouse_buttons
#undef INPUT_mouse_speed
#undef INPUT_mouse_pot_min
#undef INPUT_mouse_pot_max
#undef INPUT_mouse_pen_ofs_h
#undef INPUT_mouse_pen_ofs_v
#undef INPUT_mouse_joy_inertia
#undef INPUT_direct_mouse
#define INPUT_key_code   (IN->key_code)
#define INPUT_key_shift  (IN->key_shift)
#define INPUT_key_consol (IN->key_consol)
#define INPUT_joy_autofire (IN->joy_autofire)
#define INPUT_joy_block_opposite_directions (IN->joy_block_opposite_directions)
#define INPUT_joy_multijoy (IN->joy_multijoy)
#define INPUT_joy_5200_min    (IN->joy_5200_min)
#define INPUT_joy_5200_center (IN->joy_5200_center)
#define INPUT_joy_5200_max    (IN->joy_5200_max)
#define INPUT_cx85 (IN->cx85)
#define INPUT_mouse_mode        (IN->mouse_mode)
#define INPUT_mouse_port        (IN->mouse_port)
#define INPUT_mouse_delta_x     (IN->mouse_delta_x)
#define INPUT_mouse_delta_y     (IN->mouse_delta_y)
#define INPUT_mouse_buttons     (IN->mouse_buttons)
#define INPUT_mouse_speed       (IN->mouse_speed)
#define INPUT_mouse_pot_min     (IN->mouse_pot_min)
#define INPUT_mouse_pot_max     (IN->mouse_pot_max)
#define INPUT_mouse_pen_ofs_h   (IN->mouse_pen_ofs_h)
#define INPUT_mouse_pen_ofs_v   (IN->mouse_pen_ofs_v)
#define INPUT_mouse_joy_inertia (IN->mouse_joy_inertia)
#define INPUT_direct_mouse      (IN->direct_mouse)

/* Previously file-scope/function-local statics, now per-instance. */
#define mouse_x              (IN->mouse_x)
#define mouse_y              (IN->mouse_y)
#define mouse_move_x         (IN->mouse_move_x)
#define mouse_move_y         (IN->mouse_move_y)
#define mouse_step_e         (IN->mouse_step_e)
#define mouse_pen_show_pointer (IN->mouse_pen_show_pointer)
#define mouse_last_right     (IN->mouse_last_right)
#define mouse_last_down      (IN->mouse_last_down)
#define STICK                (IN->STICK)
#define TRIG_input           (IN->TRIG_input)
#define joy_multijoy_no      (IN->joy_multijoy_no)
#define cx85_port            (IN->cx85_port)
#define max_scanline_counter (IN->max_scanline_counter)
#define scanline_counter     (IN->scanline_counter)
#define last_key_code        (IN->last_key_code)
#define last_key_break       (IN->last_key_break)
#define last_stick           (IN->last_stick)
#define last_mouse_buttons   (IN->last_mouse_buttons)
#define bit5_5200            (IN->bit5_5200)

/* Re-point the registers written/read by INPUT_Frame to the pinned
   instance (the headers alias them to Atari800_default). */
#undef POKEY_KBCODE
#undef POKEY_IRQST
#undef POKEY_IRQEN
#undef POKEY_SKSTAT
#undef POKEY_POT_input
#undef GTIA_TRIG
#undef PIA_PORT_input
#undef ANTIC_PENH_input
#undef ANTIC_PENV_input
#undef CASSETTE_press_space
#define POKEY_KBCODE       (INI->pokey.KBCODE)
#define POKEY_IRQST        (INI->pokey.IRQST)
#define POKEY_IRQEN        (INI->pokey.IRQEN)
#define POKEY_SKSTAT       (INI->pokey.SKSTAT)
#define POKEY_POT_input    (INI->pokey.POT_input)
#define GTIA_TRIG          (INI->gtia.TRIG)
#define PIA_PORT_input     (INI->pia.PORT_input)
#define ANTIC_PENH_input   (INI->antic.PENH_input)
#define ANTIC_PENV_input   (INI->antic.PENV_input)
#define CASSETTE_press_space (INI->cassette.press_space)

/* Route the legacy function names through the pinned context. */
#undef INPUT_Initialise
#undef INPUT_Exit
#undef INPUT_Frame
#undef INPUT_Scanline
#undef INPUT_SelectMultiJoy
#undef INPUT_CenterMousePointer
#undef INPUT_DrawMousePointer
#undef INPUT_Recording
#undef INPUT_Playingback
#undef INPUT_RecordInt
#undef INPUT_PlaybackInt

static const UBYTE mouse_amiga_codes[16] = {
	0x00, 0x02, 0x0a, 0x08,
	0x01, 0x03, 0x0b, 0x09,
	0x05, 0x07, 0x0f, 0x0d,
	0x04, 0x06, 0x0e, 0x0c
};

static const UBYTE mouse_st_codes[16] = {
	0x00, 0x02, 0x03, 0x01,
	0x08, 0x0a, 0x0b, 0x09,
	0x0c, 0x0e, 0x0f, 0x0d,
	0x04, 0x06, 0x07, 0x05
};

#ifdef EVENT_RECORDING
static gzFile recordfp = NULL; /*output file for input recording*/
static gzFile playbackfp = NULL; /*input file for playback*/
static int recording = FALSE;
static int playingback = FALSE;
static int playingback_exit_after = TRUE;
static void update_adler32_of_screen(void);
static unsigned int compute_adler32_of_screen(void);
static int recording_version;
#define GZBUFSIZE 256
static char gzbuf[GZBUFSIZE+1];
#define EVENT_RECORDING_VERSION 1
#endif

int INPUT_Initialise_Ctx(Atari800_Instance *inst, int *argc, char *argv[])
{
	INPUT_PIN_CTX(inst);
	int i;
	int j;

	for (i = j = 1; i < *argc; i++) {
		int i_a = (i + 1 < *argc);		/* is argument available? */
		int a_m = FALSE;			/* error, argument missing! */

		if (strcmp(argv[i], "-mouse") == 0) {
			if (i_a) {
				char *mode = argv[++i];
				if (strcmp(mode, "off") == 0)
					INPUT_mouse_mode = INPUT_MOUSE_OFF;
				else if (strcmp(mode, "pad") == 0)
					INPUT_mouse_mode = INPUT_MOUSE_PAD;
				else if (strcmp(mode, "touch") == 0)
					INPUT_mouse_mode = INPUT_MOUSE_TOUCH;
				else if (strcmp(mode, "koala") == 0)
					INPUT_mouse_mode = INPUT_MOUSE_KOALA;
				else if (strcmp(mode, "pen") == 0)
					INPUT_mouse_mode = INPUT_MOUSE_PEN;
				else if (strcmp(mode, "gun") == 0)
					INPUT_mouse_mode = INPUT_MOUSE_GUN;
				else if (strcmp(mode, "amiga") == 0)
					INPUT_mouse_mode = INPUT_MOUSE_AMIGA;
				else if (strcmp(mode, "st") == 0)
					INPUT_mouse_mode = INPUT_MOUSE_ST;
				else if (strcmp(mode, "trak") == 0)
					INPUT_mouse_mode = INPUT_MOUSE_TRAK;
				else if (strcmp(mode, "joy") == 0)
					INPUT_mouse_mode = INPUT_MOUSE_JOY;
			}
			else a_m = TRUE;
		}
		else if (strcmp(argv[i], "-mouseport") == 0) {
			if (i_a) {
				INPUT_mouse_port = Util_sscandec(argv[++i]) - 1;
				if (INPUT_mouse_port < 0 || INPUT_mouse_port > 3) {
					Log_print("Invalid mouse port number - should be between 0 and 3");
					return FALSE;
				}
			}
			else a_m = TRUE;
		}
		else if (strcmp(argv[i], "-mousespeed") == 0) {
			if (i_a) {
				INPUT_mouse_speed = Util_sscandec(argv[++i]);
				if (INPUT_mouse_speed < 1 || INPUT_mouse_speed > 9) {
					Log_print("Invalid mouse speed - should be between 1 and 9");
					return FALSE;
				}
			}
			else a_m = TRUE;
		}
		else if (strcmp(argv[i], "-multijoy") == 0) {
			INPUT_joy_multijoy = 1;
		}
#ifdef EVENT_RECORDING
		else if (strcmp(argv[i], "-record") == 0) {
			if (i_a) {
				char *recfilename = argv[++i];
				if ((recordfp = gzopen(recfilename, "wb")) == NULL) {
					Log_print("Cannot open record file");
					return FALSE;
				}
				else {
					recording = TRUE;
					gzprintf(recordfp, "Atari800 event recording, version: %d\n", EVENT_RECORDING_VERSION);
				}
			}
			else a_m = TRUE;
		}
		else if (strcmp(argv[i], "-playback") == 0) {
			if (i_a) {
				char *pbfilename = argv[++i];
				if ((playbackfp = gzopen(pbfilename, "rb")) == NULL) {
					Log_print("Cannot open playback file");
					return FALSE;
				}
				else {
					playingback = TRUE;
					gzgets(playbackfp, gzbuf, GZBUFSIZE);
					if (sscanf(gzbuf, "Atari800 event recording, version: %d\n", &recording_version) != 1) {
						Log_print("Invalid playback file");
						playingback = FALSE;
						gzclose(playbackfp);
						return FALSE;
					}
					else if (recording_version > EVENT_RECORDING_VERSION) {
						Log_print("Newer version of playback file than this version of Atari800 can handle");
						playingback = FALSE;
						gzclose(playbackfp);
						return FALSE;
					}
				}
			}
			else a_m = TRUE;
		} else if (strcmp(argv[i], "-playbacknoexit") == 0) {
			playingback_exit_after = FALSE;
		}
#endif /* EVENT_RECORDING */
 		else if (strcmp(argv[i], "-directmouse") == 0) {
			INPUT_direct_mouse = 1;
		}
		else if (strcmp(argv[i], "-cx85") == 0) {
			if (i_a) {
				INPUT_cx85 = 1;
				cx85_port = Util_sscandec(argv[++i]) - 1;
				if (cx85_port < 0 || cx85_port > 3) {
					Log_print("Invalid cx85 port - should be between 0 and 3");
					return FALSE;
				}
			}
			else a_m = TRUE;
		}
		else {
			if (strcmp(argv[i], "-help") == 0) {
				Log_print("\t-mouse off       Do not use mouse");
				Log_print("\t-mouse pad       Emulate paddles");
				Log_print("\t-mouse touch     Emulate Atari Touch Tablet");
				Log_print("\t-mouse koala     Emulate Koala Pad");
				Log_print("\t-mouse pen       Emulate Light Pen");
				Log_print("\t-mouse gun       Emulate Light Gun");
				Log_print("\t-mouse amiga     Emulate Amiga mouse");
				Log_print("\t-mouse st        Emulate Atari ST mouse");
				Log_print("\t-mouse trak      Emulate Atari Trak-Ball");
				Log_print("\t-mouse joy       Emulate joystick using mouse");
				Log_print("\t-mouseport <n>   Set mouse port 1-4 (default 1)");
				Log_print("\t-mousespeed <n>  Set mouse speed 1-9 (default 3)");
				Log_print("\t-directmouse     Use absolute X/Y mouse coords");
				Log_print("\t-cx85 <n>        Emulate CX85 numeric keypad on port <n>");
				Log_print("\t-multijoy        Emulate MultiJoy4 interface");
				#ifdef EVENT_RECORDING
					Log_print("\t-record <file>   Record input to <file>");
					Log_print("\t-playback <file> Playback input from <file>");
					Log_print("\t-playbacknoexit  Don't exit the emulator after playback finishes");
				#endif /* EVENT_RECORDING */
				
			}
			argv[j++] = argv[i];
		}

		/* this is the end of the additional argument check */
		if (a_m) {
			Log_print("Missing argument for '%s'", argv[i]);
			return FALSE;
		}
	}

	if(INPUT_direct_mouse && !(
				INPUT_mouse_mode == INPUT_MOUSE_PAD ||
				INPUT_mouse_mode == INPUT_MOUSE_TOUCH ||
				INPUT_mouse_mode == INPUT_MOUSE_KOALA)) {
		Log_print("-directmouse only valid with -mouse pad|touch|koala");
		return FALSE;
	}

	INPUT_CenterMousePointer_Ctx(INI);
	*argc = j;

	return TRUE;
}

/* For event recording */
void INPUT_Exit_Ctx(Atari800_Instance *inst) {
	INPUT_PIN_CTX(inst);
#ifdef EVENT_RECORDING
	if (recording) {
		gzclose(recordfp);
		recording = FALSE;
	}
	if (playingback) {
		gzclose(playbackfp);
		playingback = FALSE;
	}
#endif
}

/* mouse_step is used in Amiga, ST, trak-ball and joystick modes.
   It moves mouse_x and mouse_y in the direction given by
   mouse_move_x and mouse_move_y.
   Bresenham's algorithm is used:
   if (abs(deltaX) >= abs(deltaY)) {
     if (deltaX == 0)
       return;
     MoveHorizontally();
     e -= abs(deltaY);
     if (e < 0) {
       e += abs(deltaX);
       MoveVertically();
     }
   }
   else {
     MoveVertically();
     e -= abs(deltaX);
     if (e < 0) {
       e += abs(deltaY);
       MoveHorizontally();
     }
   }
   Returned is stick code for the movement direction.
*/
static UBYTE mouse_step(void)
{
	UBYTE r = INPUT_STICK_CENTRE;
	int dx = mouse_move_x >= 0 ? mouse_move_x : -mouse_move_x;
	int dy = mouse_move_y >= 0 ? mouse_move_y : -mouse_move_y;
	if (dx >= dy) {
		if (mouse_move_x == 0)
			return r;
		if (mouse_move_x < 0) {
			r &= INPUT_STICK_LEFT;
			mouse_last_right = 0;
			mouse_x--;
			mouse_move_x += 1 << MOUSE_SHIFT;
			if (mouse_move_x > 0)
				mouse_move_x = 0;
		}
		else {
			r &= INPUT_STICK_RIGHT;
			mouse_last_right = 1;
			mouse_x++;
			mouse_move_x -= 1 << MOUSE_SHIFT;
			if (mouse_move_x < 0)
				mouse_move_x = 0;
		}
		mouse_step_e -= dy;
		if (mouse_step_e < 0) {
			mouse_step_e += dx;
			if (mouse_move_y < 0) {
				r &= INPUT_STICK_FORWARD;
				mouse_last_down = 0;
				mouse_y--;
				mouse_move_y += 1 << MOUSE_SHIFT;
				if (mouse_move_y > 0)
					mouse_move_y = 0;
			}
			else {
				r &= INPUT_STICK_BACK;
				mouse_last_down = 1;
				mouse_y++;
				mouse_move_y -= 1 << MOUSE_SHIFT;
				if (mouse_move_y < 0)
					mouse_move_y = 0;
			}
		}
	}
	else {
		if (mouse_move_y < 0) {
			r &= INPUT_STICK_FORWARD;
			mouse_last_down = 0;
			mouse_y--;
			mouse_move_y += 1 << MOUSE_SHIFT;
			if (mouse_move_y > 0)
				mouse_move_y = 0;
		}
		else {
			r &= INPUT_STICK_BACK;
			mouse_last_down = 1;
			mouse_y++;
			mouse_move_y -= 1 << MOUSE_SHIFT;
			if (mouse_move_y < 0)
				mouse_move_y = 0;
		}
		mouse_step_e -= dx;
		if (mouse_step_e < 0) {
			mouse_step_e += dy;
			if (mouse_move_x < 0) {
				r &= INPUT_STICK_LEFT;
				mouse_last_right = 0;
				mouse_x--;
				mouse_move_x += 1 << MOUSE_SHIFT;
				if (mouse_move_x > 0)
					mouse_move_x = 0;
			}
			else {
				r &= INPUT_STICK_RIGHT;
				mouse_last_right = 1;
				mouse_x++;
				mouse_move_x -= 1 << MOUSE_SHIFT;
				if (mouse_move_x < 0)
					mouse_move_x = 0;
			}
		}
	}
	return r;
}

void INPUT_Frame_Ctx(Atari800_Instance *inst)
{
	INPUT_PIN_CTX(inst);
	int i;

	scanline_counter = 10000;	/* do nothing in INPUT_Scanline() */

	/* handle keyboard */

	if (Atari800_keyboard_detached) {
		/* Disable keyboard if it's not connedted. */
		INPUT_key_code = AKEY_NONE;
		INPUT_key_shift = 0;
	}

	/* In Atari 5200 joystick there's a second fire button, which acts
	   like the Shift key in 800/XL/XE (bit 3 in SKSTAT) and generates IRQ
	   like the Break key (bit 7 in IRQST and IRQEN).
	   Note that in 5200 the joystick position and first fire button are
	   separate for each port, but the keypad and 2nd button are common.
	   That is, if you press a key in the emulator, it's like you really pressed
	   it in all the controllers simultaneously. Normally the port to read
	   keypad & 2nd button is selected with the CONSOL register in GTIA
	   (this is simply not emulated).
	   INPUT_key_code is used for keypad keys and INPUT_key_shift is used for 2nd button.
	*/
#ifdef EVENT_RECORDING
	if (playingback) {
		gzgets(playbackfp, gzbuf, GZBUFSIZE);
		sscanf(gzbuf, "%d %d %d ", &INPUT_key_code, &INPUT_key_shift, &INPUT_key_consol);
	}
	if (recording) {
		gzprintf(recordfp, "%d %d %d ", INPUT_key_code, INPUT_key_shift, INPUT_key_consol);
	}
#endif
	i = Atari800_machine_type == Atari800_MACHINE_5200 ? INPUT_key_shift : (INPUT_key_code == AKEY_BREAK);
	if (i && !last_key_break) {
		if (POKEY_IRQEN & 0x80) {
			POKEY_IRQST &= ~0x80;
			CPU_GenerateIRQ();
		}
	}
	last_key_break = i;

	POKEY_SKSTAT |= 0xc;
	if (INPUT_key_shift)
		POKEY_SKSTAT &= ~8;

	if (INPUT_key_code < 0) {
		if (CASSETTE_press_space) {
			INPUT_key_code = AKEY_SPACE;
			CASSETTE_press_space = 0;
		}
		else {
			last_key_code = AKEY_NONE;
		}
	}
	/* Following keys cannot be read with both shift and control pressed:
	   J K L ; + * Z X C V B F1 F2 F3 F4 HELP	 */
	/* which are 0x00-0x07 and 0x10-0x17 */
	/* This is caused by the keyboard itself, these keys generate 'ghost keys'
	 * when pressed with shift and control */
	if (Atari800_machine_type != Atari800_MACHINE_5200 && (INPUT_key_code&~0x17) == AKEY_SHFTCTRL) {
		INPUT_key_code = AKEY_NONE;
	}
	if (INPUT_key_code >= 0) {
		/* The 5200 has only 4 of the 6 keyboard scan lines connected */
		/* Pressing one 5200 key is like pressing 4 Atari 800 keys. */
		/* The LSB (bit 0) and bit 5 are the two missing lines. */
		/* When debounce is enabled, multiple keys pressed generate
		 * no results. */
		/* When debounce is disabled, multiple keys pressed generate
		 * results only when in numerical sequence. */
		/* Thus the LSB being one of the missing lines is important
		 * because that causes events to be generated. */
		/* Two events are generated every 64 scan lines
		 * but this code only does one every frame. */
		/* Bit 5 is different for each keypress because it is one
		 * of the missing lines. */
		if (Atari800_machine_type == Atari800_MACHINE_5200) {
			if (bit5_5200) {
				INPUT_key_code &= ~0x20;
			}
			bit5_5200 = !bit5_5200;
			/* 5200 2nd fire button generates CTRL as well */
			if (INPUT_key_shift) {
				INPUT_key_code |= AKEY_SHFTCTRL;
			}
		}
		POKEY_SKSTAT &= ~4;
		if ((INPUT_key_code ^ last_key_code) & ~AKEY_SHFTCTRL) {
		/* ignore if only shift or control has changed its state */
			last_key_code = INPUT_key_code;
			POKEY_KBCODE = (UBYTE) INPUT_key_code;
			if (POKEY_IRQEN & 0x40) {
				if (POKEY_IRQST & 0x40) {
					POKEY_IRQST &= ~0x40;
					CPU_GenerateIRQ();
				}
				else {
					/* keyboard over-run */
					POKEY_SKSTAT &= ~0x40;
					/* assert(CPU_IRQ != 0); */
				}
			}
		}
	}

	/* handle joysticks */
#ifdef EVENT_RECORDING
	if (playingback) {
		gzgets(playbackfp, gzbuf, GZBUFSIZE);
		sscanf(gzbuf,"%d ",&i);
	} else {
#endif
		i = PLATFORM_PORT(0);
#ifdef EVENT_RECORDING
	}
	if (recording) {
		gzprintf(recordfp,"%d ",i);
	}
#endif

	STICK[0] = i & 0x0f;
	STICK[1] = (i >> 4) & 0x0f;
#ifdef EVENT_RECORDING
	if (playingback) {
		gzgets(playbackfp, gzbuf, GZBUFSIZE);
		sscanf(gzbuf,"%d ",&i);
	} else {
#endif
		i = PLATFORM_PORT(1);
#ifdef EVENT_RECORDING
	}
	if (recording) {
		gzprintf(recordfp,"%d ",i);
	}
#endif
	STICK[2] = i & 0x0f;
	STICK[3] = (i >> 4) & 0x0f;

	for (i = 0; i < 4; i++) {
		if (INPUT_joy_block_opposite_directions) {
			if ((STICK[i] & 0x0c) == 0) {	/* right and left simultaneously */
				if (last_stick[i] & 0x04)	/* if wasn't left before, move left */
					STICK[i] |= 0x08;
				else						/* else move right */
					STICK[i] |= 0x04;
			}
			else {
				last_stick[i] &= 0x03;
				last_stick[i] |= STICK[i] & 0x0c;
			}
			if ((STICK[i] & 0x03) == 0) {	/* up and down simultaneously */
				if (last_stick[i] & 0x01)	/* if wasn't up before, move up */
					STICK[i] |= 0x02;
				else						/* else move down */
					STICK[i] |= 0x01;
			}
			else {
				last_stick[i] &= 0x0c;
				last_stick[i] |= STICK[i] & 0x03;
			}
		}
		else
			last_stick[i] = STICK[i];
		/* Joystick Triggers */
#ifdef EVENT_RECORDING
		if(playingback){
			int trigtemp;
			gzgets(playbackfp, gzbuf, GZBUFSIZE);
			sscanf(gzbuf,"%d ",&trigtemp);
			TRIG_input[i] = trigtemp;

		} else {
#endif
			TRIG_input[i] = PLATFORM_TRIG(i);
#ifdef EVENT_RECORDING
		}
		if(recording){
			gzprintf(recordfp,"%d ",TRIG_input[i]);
		}
#endif
		if ((INPUT_joy_autofire[i] == INPUT_AUTOFIRE_FIRE && !TRIG_input[i]) || (INPUT_joy_autofire[i] == INPUT_AUTOFIRE_CONT))
			TRIG_input[i] = (Atari800_nframes & 2) ? 1 : 0;
	}
#ifdef EVENT_RECORDING
	if(recording){
		gzprintf(recordfp,"\n");
	}
#endif

	/* handle analog joysticks in Atari 5200 */
	if (Atari800_machine_type != Atari800_MACHINE_5200) {
		if(!INPUT_direct_mouse) {
			for (i = 0; i < 4; i++)
				POKEY_POT_input[i] = Atari_POT(i);
			if (Atari800_machine_type != Atari800_MACHINE_XLXE) {
				for (i = 4; i < 8; ++i)
					POKEY_POT_input[i] = Atari_POT(i);
			}
		}
	}
	else {
		for (i = 0; i < 4; i++) {
#ifdef DREAMCAST
			/* first get analog js data */
			POKEY_POT_input[2 * i] = Atari_POT(2 * i);         /* x */
			POKEY_POT_input[2 * i + 1] = Atari_POT(2 * i + 1); /* y */
			if (POKEY_POT_input[2 * i] != INPUT_joy_5200_center
			 || POKEY_POT_input[2 * i + 1] != INPUT_joy_5200_center)
				continue;
			/* if analog js is unused, alternatively try keypad */
#endif
			if ((STICK[i] & (INPUT_STICK_CENTRE ^ INPUT_STICK_LEFT)) == 0)
				POKEY_POT_input[2 * i] = INPUT_joy_5200_min;
			else if ((STICK[i] & (INPUT_STICK_CENTRE ^ INPUT_STICK_RIGHT)) == 0)
				POKEY_POT_input[2 * i] = INPUT_joy_5200_max;
			else
				POKEY_POT_input[2 * i] = INPUT_joy_5200_center;
			if ((STICK[i] & (INPUT_STICK_CENTRE ^ INPUT_STICK_FORWARD)) == 0)
				POKEY_POT_input[2 * i + 1] = INPUT_joy_5200_min;
			else if ((STICK[i] & (INPUT_STICK_CENTRE ^ INPUT_STICK_BACK)) == 0)
				POKEY_POT_input[2 * i + 1] = INPUT_joy_5200_max;
			else
				POKEY_POT_input[2 * i + 1] = INPUT_joy_5200_center;
		}
	}

	/* handle mouse */
#ifdef __PLUS
	if (g_Input.ulState & IS_CAPTURE_MOUSE)
#endif
	switch (INPUT_mouse_mode) {
	case INPUT_MOUSE_PAD:
	case INPUT_MOUSE_TOUCH:
	case INPUT_MOUSE_KOALA:
		if(!INPUT_direct_mouse) {
			if (INPUT_mouse_mode != INPUT_MOUSE_PAD || Atari800_machine_type == Atari800_MACHINE_5200)
				mouse_x += INPUT_mouse_delta_x * INPUT_mouse_speed;
			else
				mouse_x -= INPUT_mouse_delta_x * INPUT_mouse_speed;
			if (mouse_x < INPUT_mouse_pot_min << MOUSE_SHIFT)
				mouse_x = INPUT_mouse_pot_min << MOUSE_SHIFT;
			else if (mouse_x > INPUT_mouse_pot_max << MOUSE_SHIFT)
				mouse_x = INPUT_mouse_pot_max << MOUSE_SHIFT;
			if (INPUT_mouse_mode == INPUT_MOUSE_KOALA || Atari800_machine_type == Atari800_MACHINE_5200)
				mouse_y += INPUT_mouse_delta_y * INPUT_mouse_speed;
			else
				mouse_y -= INPUT_mouse_delta_y * INPUT_mouse_speed;
			if (mouse_y < INPUT_mouse_pot_min << MOUSE_SHIFT)
				mouse_y = INPUT_mouse_pot_min << MOUSE_SHIFT;
			else if (mouse_y > INPUT_mouse_pot_max << MOUSE_SHIFT)
				mouse_y = INPUT_mouse_pot_max << MOUSE_SHIFT;
			POKEY_POT_input[INPUT_mouse_port << 1] = mouse_x >> MOUSE_SHIFT;
			POKEY_POT_input[(INPUT_mouse_port << 1) + 1] = mouse_y >> MOUSE_SHIFT;
		}
		if (Atari800_machine_type == Atari800_MACHINE_5200) {
			if (INPUT_mouse_buttons & 1)
				TRIG_input[INPUT_mouse_port] = 0;
			if (INPUT_mouse_buttons & 2)
				POKEY_SKSTAT &= ~8;
		}
		else
			STICK[INPUT_mouse_port] &= ~(INPUT_mouse_buttons << 2);
		break;
	case INPUT_MOUSE_PEN:
	case INPUT_MOUSE_GUN:
		mouse_x += INPUT_mouse_delta_x * INPUT_mouse_speed;
		if (mouse_x < 0)
			mouse_x = 0;
		else if (mouse_x > 167 << MOUSE_SHIFT)
			mouse_x = 167 << MOUSE_SHIFT;
		mouse_y += INPUT_mouse_delta_y * INPUT_mouse_speed;
		if (mouse_y < 0)
			mouse_y = 0;
		else if (mouse_y > 119 << MOUSE_SHIFT)
			mouse_y = 119 << MOUSE_SHIFT;
		ANTIC_PENH_input = 44 + INPUT_mouse_pen_ofs_h + (mouse_x >> MOUSE_SHIFT);
		ANTIC_PENV_input = 4 + INPUT_mouse_pen_ofs_v + (mouse_y >> MOUSE_SHIFT);
		if ((INPUT_mouse_buttons & 1) == (INPUT_mouse_mode == INPUT_MOUSE_PEN))
			STICK[INPUT_mouse_port] &= ~1;
		if ((INPUT_mouse_buttons & 2) && !(last_mouse_buttons & 2))
			mouse_pen_show_pointer = !mouse_pen_show_pointer;
		break;
	case INPUT_MOUSE_AMIGA:
	case INPUT_MOUSE_ST:
	case INPUT_MOUSE_TRAK:
		mouse_move_x += (INPUT_mouse_delta_x * INPUT_mouse_speed) >> 1;
		mouse_move_y += (INPUT_mouse_delta_y * INPUT_mouse_speed) >> 1;

		/* i = max(abs(mouse_move_x), abs(mouse_move_y)); */
		i = mouse_move_x >= 0 ? mouse_move_x : -mouse_move_x;
		if (mouse_move_y > i)
			i = mouse_move_y;
		else if (-mouse_move_y > i)
			i = -mouse_move_y;

		{
			if (i > 0) {
				i += (1 << MOUSE_SHIFT) - 1;
				i >>= MOUSE_SHIFT;
				if (i > 50)
					max_scanline_counter = scanline_counter = 5;
				else
					max_scanline_counter = scanline_counter = Atari800_tv_mode / i;
				mouse_step();
			}
			if (INPUT_mouse_mode == INPUT_MOUSE_TRAK) {
				/* bit 3 toggles - vertical movement, bit 2 = 0 - up */
				/* bit 1 toggles - horizontal movement, bit 0 = 0 - left */
				STICK[INPUT_mouse_port] = ((mouse_y & 1) << 3) | (mouse_last_down << 2)
									| ((mouse_x & 1) << 1) | mouse_last_right;
			}
			else {
				STICK[INPUT_mouse_port] = (INPUT_mouse_mode == INPUT_MOUSE_AMIGA ? mouse_amiga_codes : mouse_st_codes)
									[(mouse_y & 3) * 4 + (mouse_x & 3)];
			}
		}

		if (INPUT_mouse_buttons & 1)
			TRIG_input[INPUT_mouse_port] = 0;
		if (INPUT_mouse_buttons & 2)
			POKEY_POT_input[INPUT_mouse_port << 1] = 1;
		if (INPUT_mouse_buttons & 4)
			POKEY_POT_input[(INPUT_mouse_port << 1) + 1] = 1;
		break;
	case INPUT_MOUSE_JOY:
		if (Atari800_machine_type == Atari800_MACHINE_5200) {
			/* 2 * INPUT_mouse_speed is ok for Super Breakout, for Ballblazer you need more */
			int val = INPUT_joy_5200_center + ((INPUT_mouse_delta_x * 2 * INPUT_mouse_speed) >> MOUSE_SHIFT);
			if (val < INPUT_joy_5200_min)
				val = INPUT_joy_5200_min;
			else if (val > INPUT_joy_5200_max)
				val = INPUT_joy_5200_max;
			POKEY_POT_input[INPUT_mouse_port << 1] = val;
			val = INPUT_joy_5200_center + ((INPUT_mouse_delta_y * 2 * INPUT_mouse_speed) >> MOUSE_SHIFT);
			if (val < INPUT_joy_5200_min)
				val = INPUT_joy_5200_min;
			else if (val > INPUT_joy_5200_max)
				val = INPUT_joy_5200_max;
			POKEY_POT_input[(INPUT_mouse_port << 1) + 1] = val;
			if (INPUT_mouse_buttons & 2)
				POKEY_SKSTAT &= ~8;
		}
		else {
			mouse_move_x += INPUT_mouse_delta_x * INPUT_mouse_speed;
			mouse_move_y += INPUT_mouse_delta_y * INPUT_mouse_speed;
			if (mouse_move_x < -INPUT_mouse_joy_inertia << MOUSE_SHIFT ||
				mouse_move_x > INPUT_mouse_joy_inertia << MOUSE_SHIFT ||
				mouse_move_y < -INPUT_mouse_joy_inertia << MOUSE_SHIFT ||
				mouse_move_y > INPUT_mouse_joy_inertia << MOUSE_SHIFT) {
				mouse_move_x >>= 1;
				mouse_move_y >>= 1;
			}
			STICK[INPUT_mouse_port] &= mouse_step();
		}
		if (INPUT_mouse_buttons & 1)
			TRIG_input[INPUT_mouse_port] = 0;
		break;
	default:
		break;
	}
	last_mouse_buttons = INPUT_mouse_buttons;

	/* CX85 numeric keypad */
	if (INPUT_key_code <= AKEY_CX85_1 && INPUT_key_code >= AKEY_CX85_YES) {
		int val = 0;
		switch (INPUT_key_code) {
			case AKEY_CX85_1:
				val = 0x19;
				break;
			case AKEY_CX85_2:
				val = 0x1a;
				break;
			case AKEY_CX85_3:
				val = 0x1b;
				break;
			case AKEY_CX85_4:
				val = 0x11;
				break;
			case AKEY_CX85_5:
				val = 0x12;
				break;
			case AKEY_CX85_6:
				val = 0x13;
				break;
			case AKEY_CX85_7:
				val = 0x15;
				break;
			case AKEY_CX85_8:
				val = 0x16;
				break;
			case AKEY_CX85_9:
				val = 0x17;
				break;
			case AKEY_CX85_0:
				val = 0x1c;
				break;
			case AKEY_CX85_PERIOD:
				val = 0x1d;
				break;
			case AKEY_CX85_MINUS:
				val = 0x1f;
				break;
			case AKEY_CX85_PLUS_ENTER:
				val = 0x1e;
				break;
			case AKEY_CX85_ESCAPE:
				val = 0x0c;
				break;
			case AKEY_CX85_NO:
				val = 0x14;
				break;
			case AKEY_CX85_DELETE:
				val = 0x10;
				break;
			case AKEY_CX85_YES:
				val = 0x18;
				break;
		}
		if (val > 0) {
			STICK[cx85_port] = (val&0x0f);
			POKEY_POT_input[cx85_port*2+1] = ((val&0x10) ? 0 : 228);
			TRIG_input[cx85_port] = 0;
		}
	}

	if (INPUT_joy_multijoy && Atari800_machine_type != Atari800_MACHINE_5200) {
		PIA_PORT_input[0] = 0xf0 | STICK[joy_multijoy_no];
		PIA_PORT_input[1] = 0xff;
		GTIA_TRIG[0] = TRIG_input[joy_multijoy_no];
		GTIA_TRIG[1] = 1;
	}
	else {
		GTIA_TRIG[0] = TRIG_input[0];
		GTIA_TRIG[1] = TRIG_input[1];
		PIA_PORT_input[0] = (STICK[1] << 4) | STICK[0];
		PIA_PORT_input[1] = (STICK[3] << 4) | STICK[2];
	}
	if (Atari800_machine_type != Atari800_MACHINE_XLXE) {
		GTIA_TRIG[2] = TRIG_input[2];
		GTIA_TRIG[3] = TRIG_input[3];
	}

#ifdef EVENT_RECORDING
	update_adler32_of_screen();
#endif
}

#ifdef EVENT_RECORDING
static void update_adler32_of_screen(void)
{
	unsigned int adler32val = 0;
	static unsigned int adler32_errors = 0;
	static int first = TRUE;
	if (first) { /* don't calculate the first frame */
		first = FALSE;
		adler32val = 0;
	}
	else if (recording || playingback){
		adler32val = compute_adler32_of_screen();
	}

	if (recording) {
		gzprintf(recordfp, "%08X \n", adler32val);
	}
	if (playingback) {
		unsigned int pb_adler32val;
		gzgets(playbackfp, gzbuf, GZBUFSIZE);
		sscanf(gzbuf, "%08X ", &pb_adler32val);
		if (pb_adler32val != adler32val){
			Log_print("adler32 does not match");
			adler32_errors++;
		}
		
	}
	if (playingback && gzeof(playbackfp)) {
		playingback = FALSE;
		gzclose(playbackfp);
		if (playingback_exit_after) { /* exit emulation when not set otherwise */
			Atari800_ErrExit();
			exit(adler32_errors > 0 ? 1 : 0); /* return code indicates errors*/
		}
	}
}
/* Compute the adler32 value of the visible screen */
/* Note that the visible portion is 24..360 on the horizontal and */
/* 0..Screen_HEIGHT on the vertical */
static unsigned int compute_adler32_of_screen(void)
{
	int y;
	unsigned int adler = adler32(0L,Z_NULL,0);
	for (y = 0; y < Screen_HEIGHT; y++) {
		adler = adler32(adler, (unsigned char*)Screen_atari + 24 + Screen_WIDTH*y, 360 - 24);
	}
	return adler;
}
#endif /* EVENT_RECORDING */

int INPUT_Recording_Ctx(Atari800_Instance *inst)
{
	INPUT_PIN_CTX(inst);
#ifdef EVENT_RECORDING
	return recording;
#else
	return 0;
#endif
}

int INPUT_Playingback_Ctx(Atari800_Instance *inst)
{
	INPUT_PIN_CTX(inst);
#ifdef EVENT_RECORDING
	return playingback;
#else
	return 0;
#endif
}

void INPUT_RecordInt_Ctx(Atari800_Instance *inst, int i)
{
	INPUT_PIN_CTX(inst);
#ifdef EVENT_RECORDING
	if (recording) gzprintf(recordfp, "%d\n", i);
#endif
}

int INPUT_PlaybackInt_Ctx(Atari800_Instance *inst)
{
	INPUT_PIN_CTX(inst);
	int i = 0;
#ifdef EVENT_RECORDING
	if (playingback) {
		gzgets(playbackfp, gzbuf, GZBUFSIZE);
		sscanf(gzbuf, "%d", &i);
	}
#endif
	return i;
}

void INPUT_Scanline_Ctx(Atari800_Instance *inst)
{
	INPUT_PIN_CTX(inst);
	if (--scanline_counter == 0) {
		mouse_step();
		if (INPUT_mouse_mode == INPUT_MOUSE_TRAK) {
			/* bit 3 toggles - vertical movement, bit 2 = 0 - up */
			/* bit 1 toggles - horizontal movement, bit 0 = 0 - left */
			STICK[INPUT_mouse_port] = ((mouse_y & 1) << 3) | (mouse_last_down << 2)
								| ((mouse_x & 1) << 1) | mouse_last_right;
		}
		else {
			STICK[INPUT_mouse_port] = (INPUT_mouse_mode == INPUT_MOUSE_AMIGA ? mouse_amiga_codes : mouse_st_codes)
								[(mouse_y & 3) * 4 + (mouse_x & 3)];
		}
		PIA_PORT_input[0] = (STICK[1] << 4) | STICK[0];
		PIA_PORT_input[1] = (STICK[3] << 4) | STICK[2];
		scanline_counter = max_scanline_counter;
	}
}

void INPUT_SelectMultiJoy_Ctx(Atari800_Instance *inst, int no)
{
	INPUT_PIN_CTX(inst);
	no &= 3;
	joy_multijoy_no = no;
	if (INPUT_joy_multijoy && Atari800_machine_type != Atari800_MACHINE_5200) {
		PIA_PORT_input[0] = 0xf0 | STICK[no];
		GTIA_TRIG[0] = TRIG_input[no];
	}
}

void INPUT_CenterMousePointer_Ctx(Atari800_Instance *inst)
{
	INPUT_PIN_CTX(inst);
	switch (INPUT_mouse_mode) {
	case INPUT_MOUSE_PAD:
	case INPUT_MOUSE_TOUCH:
	case INPUT_MOUSE_KOALA:
		mouse_x = 114 << MOUSE_SHIFT;
		mouse_y = 114 << MOUSE_SHIFT;
		break;
	case INPUT_MOUSE_PEN:
	case INPUT_MOUSE_GUN:
		mouse_x = 84 << MOUSE_SHIFT;
		mouse_y = 60 << MOUSE_SHIFT;
		break;
	case INPUT_MOUSE_AMIGA:
	case INPUT_MOUSE_ST:
	case INPUT_MOUSE_TRAK:
	case INPUT_MOUSE_JOY:
		mouse_move_x = 0;
		mouse_move_y = 0;
		break;
	}
}

#ifndef CURSES_BASIC

#define PLOT(dx, dy)	do {\
							ptr[(dx) + Screen_WIDTH * (dy)] ^= 0x0f0f;\
							ptr[(dx) + Screen_WIDTH * (dy) + Screen_WIDTH / 2] ^= 0x0f0f;\
						} while (0)

/* draw light pen cursor */
void INPUT_DrawMousePointer_Ctx(Atari800_Instance *inst)
{
	INPUT_PIN_CTX(inst);
	if ((INPUT_mouse_mode == INPUT_MOUSE_PEN || INPUT_mouse_mode == INPUT_MOUSE_GUN) && mouse_pen_show_pointer) {
		int x = mouse_x >> MOUSE_SHIFT;
		int y = mouse_y >> MOUSE_SHIFT;
		if (x >= 0 && x <= 167 && y >= 0 && y <= 119) {
			UWORD *ptr = & ((UWORD *) Screen_atari)[12 + x + Screen_WIDTH * y];
			PLOT(-2, 0);
			PLOT(-1, 0);
			PLOT(1, 0);
			PLOT(2, 0);
			if (y >= 1) {
				PLOT(0, -1);
				if (y >= 2)
					PLOT(0, -2);
			}
			if (y <= 118) {
				PLOT(0, 1);
				if (y <= 117)
					PLOT(0, 2);
			}
		}
	}
}

#endif /* CURSES_BASIC */
