/*
 * grid.c - multi-instance grid frontend state (core, SDL-independent)
 *
 * See aidocs/atari-grid-design.md and src/grid.h.
 *
 * Input/audio/UI routing works by re-pointing the transitional
 * Atari800_default bridge pointer at the selected instance: all legacy
 * module macros (INPUT_key_code, Screen_atari, POKEY_*, ...) dereference
 * Atari800_default, so the SDL port's input, sound, UI and monitor paths
 * automatically operate on the selected instance. The emulation core
 * itself is instance-explicit (Atari800_FrameInstance etc.).
 */

#include "config.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "atari.h"
#include "grid.h"
#include "instance.h"
#include "log.h"
#include "screen.h"
#include "util.h"

#ifdef SOUND
#include "pokeysnd.h"
#include "sound.h"
#endif

int GRID_enabled = FALSE;
int GRID_max_instances = 16;
int GRID_background_refresh = 1;
unsigned int GRID_outline_colour = 0x40c040;

static Atari800_Instance *instances[GRID_MAX_INSTANCES_LIMIT];
static int count = 0;
static int selected = 0;
static int mode = GRID_MODE;
static int cols = 1;
static int rows = 1;
static unsigned long frame_tick = 0;

static void (*layout_changed_cb)(int cols, int rows) = NULL;

int GRID_Enabled(void)
{
	return GRID_enabled;
}

int GRID_Count(void)
{
	return count;
}

int GRID_Mode(void)
{
	return mode;
}

void GRID_GetDims(int *out_cols, int *out_rows)
{
	*out_cols = cols;
	*out_rows = rows;
}

Atari800_Instance *GRID_Selected(void)
{
	if (count == 0)
		return Atari800_default;
	return instances[selected];
}

int GRID_SelectedIndex(void)
{
	return selected;
}

Atari800_Instance *GRID_Instance(int i)
{
	if (i < 0 || i >= count)
		return NULL;
	return instances[i];
}

int GRID_Index(Atari800_Instance *inst)
{
	int i;
	for (i = 0; i < count; i++)
		if (instances[i] == inst)
			return i;
	return -1;
}

void GRID_SetLayoutChangedCallback(void (*cb)(int cols, int rows))
{
	layout_changed_cb = cb;
}

void GRID_LayoutFor(int n, int *out_cols, int *out_rows)
{
	int c;
	if (n < 1)
		n = 1;
	c = (int) ceil(sqrt((double) n));
	if (c < 1)
		c = 1;
	*out_cols = c;
	*out_rows = (n + c - 1) / c;
}

/* Re-point the transitional default-instance bridge at the selected
   instance so all legacy macros (input, sound, UI, monitor, screen)
   operate on it. Also re-init the (still global) sound pipeline from the
   new instance's POKEY registers to avoid stale audio state. */
static void SelectInstance(int i)
{
	if (i < 0 || i >= count)
		return;
	selected = i;
	Atari800_default = instances[i];
#ifdef SOUND
	if (Sound_enabled) {
		POKEYSND_Init(POKEYSND_FREQ_17_EXACT, Sound_out.freq, Sound_out.channels,
		              Sound_out.sample_size == 2 ? POKEYSND_BIT16 : 0);
	}
#endif
}

static void RecomputeLayout(void)
{
	GRID_LayoutFor(count, &cols, &rows);
	if (layout_changed_cb != NULL)
		layout_changed_cb(cols, rows);
}

void GRID_Initialise(void)
{
	instances[0] = Atari800_default;
	count = 1;
	selected = 0;
	mode = GRID_MODE;
	GRID_LayoutFor(count, &cols, &rows);
	if (GRID_enabled)
		SelectInstance(0);
}

int GRID_AddInstance(void)
{
	Atari800_Instance *inst;

	if (!GRID_enabled) {
		Screen_SetStatusText_Ctx(Atari800_default, "Grid is disabled (GRID_ENABLED=0)", 2);
		return FALSE;
	}
	if (count >= GRID_max_instances || count >= GRID_MAX_INSTANCES_LIMIT) {
		Screen_SetStatusText_Ctx(Atari800_default, "Instance limit reached", 2);
		return FALSE;
	}
	inst = Atari800_NewInstance();
	if (inst == NULL) {
		Screen_SetStatusText_Ctx(Atari800_default, "Out of memory for new instance", 2);
		return FALSE;
	}
	if (!Atari800_InitInstanceFromTemplate(inst)) {
		Atari800_FreeInstance(inst);
		Screen_SetStatusText_Ctx(Atari800_default, "Failed to initialise new instance", 2);
		return FALSE;
	}
	instances[count++] = inst;
	RecomputeLayout();
	SelectInstance(count - 1);
	Log_print("Grid: created instance %d (grid %dx%d)", count - 1, cols, rows);
	return TRUE;
}

int GRID_RemoveInstance(void)
{
	int i;

	if (!GRID_enabled)
		return FALSE;
	if (count <= 1) {
		Screen_SetStatusText_Ctx(Atari800_default, "Cannot delete the last instance", 2);
		return FALSE;
	}
	Atari800_FreeInstance(instances[selected]);
	/* Compact the array. */
	for (i = selected; i < count - 1; i++)
		instances[i] = instances[i + 1];
	instances[count - 1] = NULL;
	count--;
	/* Fix up selection: same index if it still exists, else the previous
	   one; if we deleted the last, select the last remaining. */
	if (selected >= count)
		selected = count - 1;
	RecomputeLayout();
	SelectInstance(selected);
	Log_print("Grid: deleted instance (now %d, grid %dx%d)", count, cols, rows);
	return TRUE;
}

void GRID_ToggleMode(void)
{
	if (!GRID_enabled)
		return;
	mode = (mode == GRID_MODE) ? SELECTED_MODE : GRID_MODE;
}

void GRID_MoveSelection(int direction)
{
	int col, row, newcol, newrow;

	if (!GRID_enabled || count <= 1)
		return;
	col = selected % cols;
	row = selected / cols;
	newcol = col;
	newrow = row;
	switch (direction) {
	case GRID_DIR_LEFT:
		newcol = col > 0 ? col - 1 : cols - 1;
		break;
	case GRID_DIR_RIGHT:
		newcol = col < cols - 1 ? col + 1 : 0;
		break;
	case GRID_DIR_UP:
		newrow = row > 0 ? row - 1 : rows - 1;
		break;
	case GRID_DIR_DOWN:
		newrow = row < rows - 1 ? row + 1 : 0;
		break;
	default:
		return;
	}
	/* Wrap within the grid bounds; if the destination cell is empty
	   (beyond count), clamp to the nearest occupied cell in that row
	   (for horizontal moves) or column (for vertical moves). */
	if (newcol != col) {
		/* Horizontal move: clamp the column to the occupied range of the
		   (possibly wrapped) row. */
		int row_start = newrow * cols;
		int row_end = row_start + cols; /* exclusive */
		if (row_end > count)
			row_end = count;
		if (newcol >= row_end - row_start)
			newcol = row_end - row_start - 1;
		if (newcol < 0)
			newcol = 0;
	}
	if (newrow != row) {
		/* Vertical move: keep the column; if that column has no instance
		   in the destination row, move to the last occupied cell of that
		   row (nearest occupied in row-major order). */
		int idx = newrow * cols + newcol;
		if (idx >= count) {
			idx = count - 1;
			if (idx / cols != newrow)
				idx = newrow * cols; /* empty row: shouldn't happen */
		}
		newcol = idx % cols;
		newrow = idx / cols;
	}
	SelectInstance(newrow * cols + newcol);
}

void GRID_FrameAll(void)
{
	int i;
	unsigned long phase;

	if (!GRID_enabled) {
		Atari800_FrameInstance(Atari800_default);
		return;
	}
	phase = frame_tick % (GRID_background_refresh < 1 ? 1 : GRID_background_refresh);
	for (i = 0; i < count; i++) {
		if (i == selected || phase == 0)
			Atari800_FrameInstance(instances[i]);
	}
	frame_tick++;
}

int GRID_ReadConfig(char *option, char *parameters)
{
	if (strcmp(option, "GRID_ENABLED") == 0)
		return (GRID_enabled = Util_sscanbool(parameters)) != -1;
	else if (strcmp(option, "GRID_MAX_INSTANCES") == 0) {
		int v = Util_sscandec(parameters);
		if (v < 1 || v > GRID_MAX_INSTANCES_LIMIT)
			return FALSE;
		GRID_max_instances = v;
		return TRUE;
	}
	else if (strcmp(option, "GRID_BACKGROUND_REFRESH") == 0) {
		int v = Util_sscandec(parameters);
		if (v < 1)
			return FALSE;
		GRID_background_refresh = v;
		return TRUE;
	}
	else if (strcmp(option, "GRID_OUTLINE_COLOUR") == 0) {
		unsigned int v = (unsigned int) Util_sscanhex(parameters);
		GRID_outline_colour = v & 0xffffff;
		return TRUE;
	}
	return FALSE;
}

void GRID_WriteConfig(FILE *fp)
{
	fprintf(fp, "GRID_ENABLED=%d\n", GRID_enabled);
	fprintf(fp, "GRID_MAX_INSTANCES=%d\n", GRID_max_instances);
	fprintf(fp, "GRID_BACKGROUND_REFRESH=%d\n", GRID_background_refresh);
	fprintf(fp, "GRID_OUTLINE_COLOUR=%06X\n", GRID_outline_colour);
}
