#ifndef SCREEN_H_
#define SCREEN_H_

#include <stdio.h>

#include "atari.h"  /* UBYTE */

/* Dimensions of Screen_atari.
   Screen_atari is Screen_WIDTH * Screen_HEIGHT bytes.
   Each byte is an Atari color code - use Colours_Get[RGB] functions
   to get actual RGB codes.
   You should never display anything outside the middle 336 columns.
   NOTE: these must be defined before including instance.h, because
   instance.h includes this header (for the ANTIC pm_scanline buffer
   size) while the aliases below need Atari800_Instance from it. */
#define Screen_WIDTH  384
#define Screen_HEIGHT 240

/* NOTE: instance.h includes this header (for the ANTIC pm_scanline buffer
   size) before defining Atari800_Instance, so the prototypes below use a
   forward declaration of the struct tag to break the include cycle. */
struct Atari800_Instance;

int Screen_Initialise_Ctx(struct Atari800_Instance *inst, int *argc, char *argv[]);
int Screen_ReadConfig_Ctx(struct Atari800_Instance *inst, char *string, char *ptr);
void Screen_WriteConfig_Ctx(struct Atari800_Instance *inst, FILE *fp);
void Screen_DrawAtariSpeed_Ctx(struct Atari800_Instance *inst, double cur_time);
void Screen_DrawDiskLED_Ctx(struct Atari800_Instance *inst);
void Screen_Draw1200LED_Ctx(struct Atari800_Instance *inst);
void Screen_DrawMultimediaStats_Ctx(struct Atari800_Instance *inst);
int Screen_SaveScreenshot_Ctx(struct Atari800_Instance *inst, const char *filename, int interlaced);
void Screen_SaveNextScreenshot_Ctx(struct Atari800_Instance *inst, int interlaced);
void Screen_EntireDirty_Ctx(struct Atari800_Instance *inst);
void Screen_SetStatusText_Ctx(struct Atari800_Instance *inst, const char *text, int duration);
void Screen_DrawStatusText_Ctx(struct Atari800_Instance *inst);

#include "instance.h" /* Atari800_Instance, Atari800_default (transitional) */

/* Transitional Option C bridge: the per-instance Screen state lives in
   Screen_state_t (instance.h). Until all callers pass an instance explicitly,
   the legacy global names are aliased to the default instance. */

#ifdef DIRTYRECT
/* Dirty-rectangle tracking buffer (Screen_WIDTH * Screen_HEIGHT / 8 bytes). */
#define Screen_dirty (Atari800_default->screen.dirty)
#endif /* DIRTYRECT */

#define Screen_atari (Atari800_default->screen.atari)

#ifdef BITPL_SCR
#define Screen_atari_b (Atari800_default->screen.atari_b)
#define Screen_atari1  (Atari800_default->screen.atari1)
#define Screen_atari2  (Atari800_default->screen.atari2)
#endif

/* The area that can been seen is Screen_visible_x1 <= x < Screen_visible_x2,
   Screen_visible_y1 <= y < Screen_visible_y2.
   Full Atari screen is 336x240. Screen_WIDTH is 384 only because
   the code in antic.c sometimes draws more than 336 bytes in a line.
   Currently Screen_visible variables are used only to place
   disk led and snailmeter in the corners of the screen.
*/
#define Screen_visible_x1 (Atari800_default->screen.visible_x1)
#define Screen_visible_y1 (Atari800_default->screen.visible_y1)
#define Screen_visible_x2 (Atari800_default->screen.visible_x2)
#define Screen_visible_y2 (Atari800_default->screen.visible_y2)

#define Screen_show_atari_speed      (Atari800_default->screen.show_atari_speed)
#define Screen_show_disk_led         (Atari800_default->screen.show_disk_led)
#define Screen_show_sector_counter   (Atari800_default->screen.show_sector_counter)
#define Screen_show_1200_leds        (Atari800_default->screen.show_1200_leds)
#define Screen_show_multimedia_stats (Atari800_default->screen.show_multimedia_stats)

/* The legacy names below are forwarding macros that pass the default
   instance, so not-yet-migrated callers are unchanged. */
#define Screen_Initialise(argc, argv) \
	Screen_Initialise_Ctx(Atari800_default, (argc), (argv))
#define Screen_ReadConfig(string, ptr) \
	Screen_ReadConfig_Ctx(Atari800_default, (string), (ptr))
#define Screen_WriteConfig(fp) \
	Screen_WriteConfig_Ctx(Atari800_default, (fp))
#define Screen_DrawAtariSpeed(cur_time) \
	Screen_DrawAtariSpeed_Ctx(Atari800_default, (cur_time))
#define Screen_DrawDiskLED() \
	Screen_DrawDiskLED_Ctx(Atari800_default)
#define Screen_Draw1200LED() \
	Screen_Draw1200LED_Ctx(Atari800_default)
#define Screen_DrawMultimediaStats() \
	Screen_DrawMultimediaStats_Ctx(Atari800_default)
#define Screen_SaveScreenshot(filename, interlaced) \
	Screen_SaveScreenshot_Ctx(Atari800_default, (filename), (interlaced))
#define Screen_SaveNextScreenshot(interlaced) \
	Screen_SaveNextScreenshot_Ctx(Atari800_default, (interlaced))
#define Screen_EntireDirty() \
	Screen_EntireDirty_Ctx(Atari800_default)
#define Screen_SetStatusText(text, duration) \
	Screen_SetStatusText_Ctx(Atari800_default, (text), (duration))
#define Screen_DrawStatusText() \
	Screen_DrawStatusText_Ctx(Atari800_default)

#endif /* SCREEN_H_ */
