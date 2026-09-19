/*
 * grid.h - multi-instance grid frontend state (core, SDL-independent)
 *
 * Manages a grid of Atari800 instances in one process. One instance is
 * selected at any time; it receives input and is the only source of audio.
 * See aidocs/atari-grid-design.md.
 *
 * The design keeps single-instance behaviour as the default (GRID_ENABLED=0)
 * so the feature is fully opt-in.
 */

#ifndef GRID_H_
#define GRID_H_

#include <stdio.h>

#include "atari.h" /* TRUE/FALSE */
#include "instance.h" /* Atari800_Instance */

/* Hard cap on the number of instances (configurable up to this). */
#define GRID_MAX_INSTANCES_LIMIT 64

/* View modes. */
enum {
	GRID_MODE,      /* all instances in a grid, selected one outlined */
	SELECTED_MODE   /* only the selected instance, full window */
};

/* Selection movement directions. */
enum {
	GRID_DIR_UP,
	GRID_DIR_DOWN,
	GRID_DIR_LEFT,
	GRID_DIR_RIGHT
};

/* Configuration (read from the config file, see GRID_ReadConfig). */
extern int GRID_enabled;            /* GRID_ENABLED (default 0) */
extern int GRID_max_instances;      /* GRID_MAX_INSTANCES (default 16) */
extern int GRID_background_refresh; /* GRID_BACKGROUND_REFRESH (default 1) */
extern unsigned int GRID_outline_colour; /* GRID_OUTLINE_COLOUR, 0x40C040 */

/* Initialise the grid: one instance (the default instance) selected,
   1x1 grid, GRID_MODE. Call after Atari800_Initialise(). */
void GRID_Initialise(void);

/* TRUE if the grid feature is enabled (GRID_ENABLED != 0). */
int GRID_Enabled(void);

/* Number of live instances. */
int GRID_Count(void);

/* Current view mode (GRID_MODE / SELECTED_MODE). */
int GRID_Mode(void);

/* Current grid dimensions. */
void GRID_GetDims(int *cols, int *rows);

/* The selected instance. */
Atari800_Instance *GRID_Selected(void);

/* Index of the selected instance in the grid. */
int GRID_SelectedIndex(void);

/* Instance at grid index i, or NULL. */
Atari800_Instance *GRID_Instance(int i);

/* Grid index of an instance pointer, or -1. */
int GRID_Index(Atari800_Instance *inst);

/* Compute grid dimensions for a given instance count:
   cols = ceil(sqrt(count)) (>= rows), rows = ceil(count / cols). */
void GRID_LayoutFor(int count, int *cols, int *rows);

/* Create a new instance (initialised from the config template), append it
   to the grid, recompute the layout and select it. Returns FALSE (with a
   status message) when the instance cap is reached. */
int GRID_AddInstance(void);

/* Delete the selected instance. Refuses (with a status message) when it is
   the only one. Frees the instance, compacts the array, recomputes the
   layout and fixes up the selection. */
int GRID_RemoveInstance(void);

/* Toggle GRID_MODE <-> SELECTED_MODE. */
void GRID_ToggleMode(void);

/* Move the selection in the grid layout (wrap/clamp per design 5.3). */
void GRID_MoveSelection(int direction);

/* Advance every live instance one frame. Non-selected instances are run
   every GRID_BACKGROUND_REFRESH-th frame (1 = full rate). */
void GRID_FrameAll(void);

/* Register a callback invoked whenever the grid layout changes (instance
   added/removed). The SDL layer uses it to grow the window. */
void GRID_SetLayoutChangedCallback(void (*cb)(int cols, int rows));

/* Config file integration (GRID_* options). */
int GRID_ReadConfig(char *option, char *parameters);
void GRID_WriteConfig(FILE *fp);

#endif /* GRID_H_ */
