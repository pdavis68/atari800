/*
 * sdl/grid.c - SDL-layer support for the multi-instance grid frontend
 *
 * Key bindings (Shift+F1/F2/F3, Shift+Ctrl+arrows by default), window
 * growth on instance add, and grid rendering via the SDL2 renderer.
 * See aidocs/atari-grid-design.md and src/grid.h.
 */

#include "config.h"

#include <SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "atari.h"
#include "../grid.h"
#include "instance.h"
#include "log.h"
#include "screen.h"
#include "util.h"
#include "videomode.h"

#include "grid.h"
#include "palette.h"
#include "video.h"

#ifndef KEY_SDL
#define KEY_SDL "SDL_"
#endif

/* Default grid key bindings: Shift+F1 = new instance, Shift+F2 = delete
   selected, Shift+F3 = toggle grid/selected view. */
static int GRID_KEY_NEW = SDLK_F1;
static int GRID_KEY_DELETE = SDLK_F2;
static int GRID_KEY_TOGGLE = SDLK_F3;

/* Cell inset (border between instances), in pixels. */
#define GRID_CELL_INSET 2
/* Selection outline thickness, in pixels. */
#define GRID_OUTLINE_THICKNESS 5

static void GridLayoutChanged(int cols, int rows);

void SDL_GRID_Initialise(void)
{
	GRID_SetLayoutChangedCallback(GridLayoutChanged);
}

/* Parse a decimal SDL keysym (same format as the other SDL_*_KEY
   options; see SDLKeyBind in sdl/input.c). */
static int ParseKey(int *retval, char *sdlKeySymIntStr)
{
	int ksym = Util_sscandec(sdlKeySymIntStr);
	if (ksym <= SDLK_UNKNOWN)
		return FALSE;
	*retval = ksym;
	return TRUE;
}

int SDL_GRID_ReadConfig(char *option, char *parameters)
{
	if (strcmp(option, KEY_SDL "GRID_NEW_KEY") == 0)
		return ParseKey(&GRID_KEY_NEW, parameters);
	else if (strcmp(option, KEY_SDL "GRID_DELETE_KEY") == 0)
		return ParseKey(&GRID_KEY_DELETE, parameters);
	else if (strcmp(option, KEY_SDL "GRID_TOGGLE_KEY") == 0)
		return ParseKey(&GRID_KEY_TOGGLE, parameters);
	return FALSE;
}

void SDL_GRID_WriteConfig(FILE *fp)
{
	fprintf(fp, KEY_SDL "GRID_NEW_KEY=%d\n", GRID_KEY_NEW);
	fprintf(fp, KEY_SDL "GRID_DELETE_KEY=%d\n", GRID_KEY_DELETE);
	fprintf(fp, KEY_SDL "GRID_TOGGLE_KEY=%d\n", GRID_KEY_TOGGLE);
}

int SDL_GRID_KeyboardHook(int lastkey, int shift, int ctrl, int *key_pressed)
{
	int dir;

	if (!GRID_Enabled() || !*key_pressed)
		return FALSE;
	if (shift && !ctrl) {
		if (lastkey == GRID_KEY_NEW) {
			*key_pressed = 0;
			GRID_AddInstance();
			return TRUE;
		}
		if (lastkey == GRID_KEY_DELETE) {
			*key_pressed = 0;
			GRID_RemoveInstance();
			return TRUE;
		}
		if (lastkey == GRID_KEY_TOGGLE) {
			*key_pressed = 0;
			GRID_ToggleMode();
			return TRUE;
		}
	}
	if (shift && ctrl) {
		dir = -1;
		switch (lastkey) {
		case SDLK_UP:    dir = GRID_DIR_UP; break;
		case SDLK_DOWN:  dir = GRID_DIR_DOWN; break;
		case SDLK_LEFT:  dir = GRID_DIR_LEFT; break;
		case SDLK_RIGHT: dir = GRID_DIR_RIGHT; break;
		}
		if (dir >= 0) {
			*key_pressed = 0;
			GRID_MoveSelection(dir);
			return TRUE;
		}
	}
	return FALSE;
}

/* ------------------------------------------------------------------ */
/* Window management                                                   */
/* ------------------------------------------------------------------ */

static int prev_cols = -1;
static int prev_rows = -1;

/* Called by the core whenever the grid layout changes. Grows the window
   so each cell keeps its current size (clamped to the desktop); never
   shrinks (design 3.3). */
static void GridLayoutChanged(int cols, int rows)
{
#if SDL2
	int cell_w, cell_h, new_w, new_h;
	SDL_Rect bounds;
	int max_w, max_h;

	if (SDL_VIDEO_wnd != NULL && prev_cols >= 1 && prev_rows >= 1) {
		cell_w = SDL_VIDEO_width / prev_cols;
		cell_h = SDL_VIDEO_height / prev_rows;
		if (cell_w < 1) cell_w = 1;
		if (cell_h < 1) cell_h = 1;
		new_w = cell_w * cols;
		new_h = cell_h * rows;
		/* Clamp to the desktop bounds. */
		max_w = 0;
		max_h = 0;
		if (SDL_GetDisplayBounds(SDL_GetWindowDisplayIndex(SDL_VIDEO_wnd), &bounds) == 0) {
			max_w = bounds.w;
			max_h = bounds.h;
		}
		if (max_w > 0 && new_w > max_w) new_w = max_w;
		if (max_h > 0 && new_h > max_h) new_h = max_h;
		if (new_w > SDL_VIDEO_width || new_h > SDL_VIDEO_height)
			SDL_SetWindowSize(SDL_VIDEO_wnd, new_w, new_h);
	}
#endif
	prev_cols = cols;
	prev_rows = rows;
}

/* ------------------------------------------------------------------ */
/* Rendering (SDL2 renderer)                                           */
/* ------------------------------------------------------------------ */

#if SDL2

/* Per-instance cached conversion texture + pixel buffer. */
typedef struct {
	SDL_Texture *texture;
	Uint32 *pixels;
	int width, height;
} grid_tex_t;

static grid_tex_t grid_tex[GRID_MAX_INSTANCES_LIMIT];
static int grid_tex_count = 0; /* number of initialised entries */

static void FreeGridTextures(void)
{
	int i;
	for (i = 0; i < grid_tex_count; i++) {
		if (grid_tex[i].texture != NULL)
			SDL_DestroyTexture(grid_tex[i].texture);
		free(grid_tex[i].pixels);
		grid_tex[i].texture = NULL;
		grid_tex[i].pixels = NULL;
		grid_tex[i].width = grid_tex[i].height = 0;
	}
	grid_tex_count = 0;
}

/* Convert an instance's visible framebuffer area into an ARGB8888
   streaming texture. Returns the texture or NULL. */
static SDL_Texture *InstanceTexture(int idx, Atari800_Instance *inst, int *w, int *h)
{
	grid_tex_t *t = &grid_tex[idx];
	int x, y;
	int vw, vh;
	UBYTE *src;
	Uint32 *dst;
	const int *palette = inst->colours.table;

	vw = inst->screen.visible_x2 - inst->screen.visible_x1;
	vh = inst->screen.visible_y2 - inst->screen.visible_y1;
	if (vw < 1 || vh < 1)
		return NULL;

	if (t->texture == NULL || t->width != vw || t->height != vh) {
		if (t->texture != NULL)
			SDL_DestroyTexture(t->texture);
		free(t->pixels);
		t->texture = SDL_CreateTexture(SDL_VIDEO_renderer,
			SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
			vw, vh);
		if (t->texture == NULL) {
			t->pixels = NULL;
			t->width = t->height = 0;
			return NULL;
		}
		t->pixels = (Uint32 *) malloc((size_t) vw * vh * sizeof(Uint32));
		t->width = vw;
		t->height = vh;
	}

	src = (UBYTE *) inst->screen.atari
	      + inst->screen.visible_y1 * Screen_WIDTH
	      + inst->screen.visible_x1;
	dst = t->pixels;
	for (y = 0; y < vh; y++) {
		for (x = 0; x < vw; x++)
			dst[x] = 0xff000000u | (Uint32) palette[src[x]];
		src += Screen_WIDTH;
		dst += vw;
	}
	SDL_UpdateTexture(t->texture, NULL, t->pixels, vw * sizeof(Uint32));
	*w = vw;
	*h = vh;
	return t->texture;
}

/* Fit a w x h source into a cw x ch cell, preserving aspect ratio,
   centred. */
static void FitRect(int w, int h, int cw, int ch, SDL_Rect *r)
{
	double scale;
	if (w < 1 || h < 1 || cw < 1 || ch < 1) {
		r->w = r->h = 0;
		return;
	}
	scale = (double) cw / w;
	if ((double) ch / h < scale)
		scale = (double) ch / h;
	r->w = (int) (w * scale);
	r->h = (int) (h * scale);
	if (r->w < 1) r->w = 1;
	if (r->h < 1) r->h = 1;
	r->x = (cw - r->w) / 2;
	r->y = (ch - r->h) / 2;
}

static void DrawOutline(const SDL_Rect *cell)
{
	SDL_SetRenderDrawColor(SDL_VIDEO_renderer,
		(GRID_outline_colour >> 16) & 0xff,
		(GRID_outline_colour >> 8) & 0xff,
		GRID_outline_colour & 0xff,
		0xff);
	/* Four filled rects form a ~5 px outline around the cell. */
	{
		SDL_Rect top =    { cell->x, cell->y, cell->w, GRID_OUTLINE_THICKNESS };
		SDL_Rect bottom = { cell->x, cell->y + cell->h - GRID_OUTLINE_THICKNESS, cell->w, GRID_OUTLINE_THICKNESS };
		SDL_Rect left =   { cell->x, cell->y, GRID_OUTLINE_THICKNESS, cell->h };
		SDL_Rect right =  { cell->x + cell->w - GRID_OUTLINE_THICKNESS, cell->y, GRID_OUTLINE_THICKNESS, cell->h };
		SDL_RenderFillRect(SDL_VIDEO_renderer, &top);
		SDL_RenderFillRect(SDL_VIDEO_renderer, &bottom);
		SDL_RenderFillRect(SDL_VIDEO_renderer, &left);
		SDL_RenderFillRect(SDL_VIDEO_renderer, &right);
	}
}

void SDL_GRID_DisplayScreen(void)
{
	int n, cols, rows, cw, ch, i;

	if (SDL_VIDEO_renderer == NULL)
		return;

	if (grid_tex_count != GRID_Count()) {
		/* Instances were added or removed: rebuild the texture cache. */
		FreeGridTextures();
		grid_tex_count = GRID_Count();
	}

	SDL_SetRenderDrawColor(SDL_VIDEO_renderer, 0, 0, 0, 0xff);
	SDL_RenderClear(SDL_VIDEO_renderer);

	GRID_GetDims(&cols, &rows);
	cw = SDL_VIDEO_width / cols;
	ch = SDL_VIDEO_height / rows;
	n = GRID_Count();

	for (i = 0; i < n; i++) {
		Atari800_Instance *inst = GRID_Instance(i);
		SDL_Texture *tex;
		int w, h;
		SDL_Rect cell, dst;
		if (inst == NULL || inst->screen.atari == NULL)
			continue;
		tex = InstanceTexture(i, inst, &w, &h);
		if (tex == NULL)
			continue;
		cell.x = (i % cols) * cw;
		cell.y = (i / cols) * ch;
		cell.w = cw;
		cell.h = ch;
		FitRect(w, h, cw - 2 * GRID_CELL_INSET, ch - 2 * GRID_CELL_INSET, &dst);
		dst.x += cell.x + GRID_CELL_INSET;
		dst.y += cell.y + GRID_CELL_INSET;
		SDL_RenderCopy(SDL_VIDEO_renderer, tex, NULL, &dst);
		if (i == GRID_SelectedIndex())
			DrawOutline(&cell);
	}

	SDL_RenderPresent(SDL_VIDEO_renderer);
}

#else /* !SDL2 */

/* SDL1 does not have the renderer API; the grid falls back to showing
   only the selected instance via the normal display path (documented
   deviation from the design). */
void SDL_GRID_DisplayScreen(void)
{
}

#endif /* SDL2 */
