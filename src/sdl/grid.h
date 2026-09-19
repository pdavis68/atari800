/*
 * sdl/grid.h - SDL-layer support for the multi-instance grid frontend
 *
 * Key bindings, window management and grid rendering. The grid state
 * itself lives in the core (src/grid.c).
 */

#ifndef SDL_GRID_H_
#define SDL_GRID_H_

#include <stdio.h>

/* Config file integration for the SDL_GRID_* key bindings.
   Called from SDL_INPUT_ReadConfig / SDL_INPUT_WriteConfig. */
int SDL_GRID_ReadConfig(char *option, char *parameters);
void SDL_GRID_WriteConfig(FILE *fp);

/* Register the grid key bindings and the window-growth callback.
   Call once during SDL input init. */
void SDL_GRID_Initialise(void);

/* Keyboard hook: consumes the grid control keys (Shift+F1/F2/F3 and
   Shift+Ctrl+arrows by default) before the Atari key-translation path.
   LASTKEY is the pending SDL keysym, SHIFT/CTRL the modifier states,
   KEY_PRESSED a pointer to the "key pressed" flag (cleared when the key
   is consumed). Returns TRUE if the key was consumed. */
int SDL_GRID_KeyboardHook(int lastkey, int shift, int ctrl, int *key_pressed);

/* Grid display: compose all live instances into their cells (SDL2
   renderer) and present. Call instead of the normal display path while
   the grid is in GRID_MODE. */
void SDL_GRID_DisplayScreen(void);

#endif /* SDL_GRID_H_ */
