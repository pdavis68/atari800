#ifndef INPUT_H_
#define INPUT_H_

#include "instance.h" /* Atari800_Instance (transitional default-instance aliases) */

/* Keyboard AKEY_* are in akey.h */

/* INPUT_key_consol masks */
/* Note: INPUT_key_consol should be INPUT_CONSOL_NONE if no consol key is pressed.
   When a consol key is pressed, corresponding bit should be cleared.
 */
#define INPUT_CONSOL_NONE		0x07
#define INPUT_CONSOL_START	0x01
#define INPUT_CONSOL_SELECT	0x02
#define INPUT_CONSOL_OPTION	0x04

/* Transitional Option C bridge: the per-instance input state lives in
   Input_state_t (instance.h). Until all callers pass an instance
   explicitly, the legacy global names are aliased to the default instance. */
#define INPUT_key_code   (Atari800_default->input.key_code)   /* regular Atari key code */
#define INPUT_key_shift  (Atari800_default->input.key_shift)  /* Shift key pressed */
#define INPUT_key_consol (Atari800_default->input.key_consol) /* Start, Select and Option keys */

/* Joysticks ----------------------------------------------------------- */

/* joystick position */
#define	INPUT_STICK_LL		0x09
#define	INPUT_STICK_BACK		0x0d
#define	INPUT_STICK_LR		0x05
#define	INPUT_STICK_LEFT		0x0b
#define	INPUT_STICK_CENTRE	0x0f
#define	INPUT_STICK_RIGHT		0x07
#define	INPUT_STICK_UL		0x0a
#define	INPUT_STICK_FORWARD	0x0e
#define	INPUT_STICK_UR		0x06

/* joy_autofire values */
#define INPUT_AUTOFIRE_OFF	0
#define INPUT_AUTOFIRE_FIRE	1	/* Fire dependent */
#define INPUT_AUTOFIRE_CONT	2	/* Continuous */

#define INPUT_joy_autofire (Atari800_default->input.joy_autofire) /* autofire mode for each Atari port */

#define INPUT_joy_block_opposite_directions (Atari800_default->input.joy_block_opposite_directions) /* can't move joystick left
																		   and right simultaneously */

#define INPUT_joy_multijoy (Atari800_default->input.joy_multijoy) /* emulate MultiJoy4 interface */

/* 5200 joysticks values */
#define INPUT_joy_5200_min    (Atari800_default->input.joy_5200_min)
#define INPUT_joy_5200_center (Atari800_default->input.joy_5200_center)
#define INPUT_joy_5200_max    (Atari800_default->input.joy_5200_max)

/* Mouse --------------------------------------------------------------- */

/* INPUT_mouse_mode values */
#define INPUT_MOUSE_OFF		0
#define INPUT_MOUSE_PAD		1	/* Paddles */
#define INPUT_MOUSE_TOUCH	2	/* Atari touch tablet */
#define INPUT_MOUSE_KOALA	3	/* Koala pad */
#define INPUT_MOUSE_PEN		4	/* Light pen */
#define INPUT_MOUSE_GUN		5	/* Light gun */
#define INPUT_MOUSE_AMIGA	6	/* Amiga mouse */
#define INPUT_MOUSE_ST		7	/* Atari ST mouse */
#define INPUT_MOUSE_TRAK	8	/* Atari CX22 Trak-Ball */
#define INPUT_MOUSE_JOY		9	/* Joystick */

#define INPUT_mouse_mode        (Atari800_default->input.mouse_mode)      /* device emulated with mouse */
#define INPUT_mouse_port        (Atari800_default->input.mouse_port)      /* Atari port, to which the emulated device is attached */
#define INPUT_mouse_delta_x     (Atari800_default->input.mouse_delta_x)   /* x motion since last frame */
#define INPUT_mouse_delta_y     (Atari800_default->input.mouse_delta_y)   /* y motion since last frame */
#define INPUT_mouse_buttons     (Atari800_default->input.mouse_buttons)   /* buttons pressed (b0: left, b1: right, b2: middle */
#define INPUT_mouse_speed       (Atari800_default->input.mouse_speed)     /* how fast the mouse pointer moves */
#define INPUT_mouse_pot_min     (Atari800_default->input.mouse_pot_min)   /* min. value of POKEY's POT register */
#define INPUT_mouse_pot_max     (Atari800_default->input.mouse_pot_max)   /* max. value of POKEY's POT register */
#define INPUT_mouse_pen_ofs_h   (Atari800_default->input.mouse_pen_ofs_h) /* light pen/gun horizontal offset (for calibration) */
#define INPUT_mouse_pen_ofs_v   (Atari800_default->input.mouse_pen_ofs_v) /* light pen/gun vertical offset (for calibration) */
#define INPUT_mouse_joy_inertia (Atari800_default->input.mouse_joy_inertia) /* how long the mouse pointer can move (time in Atari frames)
																		   after a fast motion of mouse */
#define INPUT_direct_mouse      (Atari800_default->input.direct_mouse)    /* When true, convert the mouse pointer
																			position directly into POKEY POT values */

#define INPUT_cx85 (Atari800_default->input.cx85) /* emulate CX85 numeric keypad */

/* Functions ----------------------------------------------------------- */

/* Context-aware entry points (Option C). The legacy names below are
   forwarding macros that pass the default instance, so not-yet-migrated
   callers are unchanged. */
int INPUT_Initialise_Ctx(Atari800_Instance *inst, int *argc, char *argv[]);
void INPUT_Exit_Ctx(Atari800_Instance *inst);
void INPUT_Frame_Ctx(Atari800_Instance *inst);
void INPUT_Scanline_Ctx(Atari800_Instance *inst);
void INPUT_SelectMultiJoy_Ctx(Atari800_Instance *inst, int no);
void INPUT_CenterMousePointer_Ctx(Atari800_Instance *inst);
void INPUT_DrawMousePointer_Ctx(Atari800_Instance *inst);
int INPUT_Recording_Ctx(Atari800_Instance *inst);
int INPUT_Playingback_Ctx(Atari800_Instance *inst);
void INPUT_RecordInt_Ctx(Atari800_Instance *inst, int i);
int INPUT_PlaybackInt_Ctx(Atari800_Instance *inst);

#define INPUT_Initialise(argc, argv)       INPUT_Initialise_Ctx(Atari800_default, argc, argv)
#define INPUT_Exit()                       INPUT_Exit_Ctx(Atari800_default)
#define INPUT_Frame()                      INPUT_Frame_Ctx(Atari800_default)
#define INPUT_Scanline()                   INPUT_Scanline_Ctx(Atari800_default)
#define INPUT_SelectMultiJoy(no)           INPUT_SelectMultiJoy_Ctx(Atari800_default, no)
#define INPUT_CenterMousePointer()         INPUT_CenterMousePointer_Ctx(Atari800_default)
#define INPUT_DrawMousePointer()           INPUT_DrawMousePointer_Ctx(Atari800_default)
#define INPUT_Recording()                  INPUT_Recording_Ctx(Atari800_default)
#define INPUT_Playingback()                INPUT_Playingback_Ctx(Atari800_default)
#define INPUT_RecordInt(i)                 INPUT_RecordInt_Ctx(Atari800_default, i)
#define INPUT_PlaybackInt()                INPUT_PlaybackInt_Ctx(Atari800_default)

#ifdef DREAMCAST
extern int Atari_POT(int);
#elif SDL2
extern int Atari_POT(int);
#else
#define Atari_POT(x) 228
#endif

#endif /* INPUT_H_ */
