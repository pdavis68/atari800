#ifndef PAL_BLENDING_H_
#define PAL_BLENDING_H_

#include "atari.h"

/* Forward declaration for the context-aware API (defined in instance.h). */
struct Atari800_Instance;

/* Updates blitter lookup tables according to the current display pixel
   format. Call after changing host video mode or after adjusting colours. */
void PAL_BLENDING_UpdateLookup_Ctx(struct Atari800_Instance *inst);

/* Blit without scaling to a 16-BPP screen. */
void PAL_BLENDING_Blit16_Ctx(struct Atari800_Instance *inst, ULONG *dest, UBYTE *src, int pitch, int width, int height, int start_odd);
/* Blit without scaling to a 32-BPP screen. */
void PAL_BLENDING_Blit32_Ctx(struct Atari800_Instance *inst, ULONG *dest, UBYTE *src, int pitch, int width, int height, int start_odd);

/* Blit with scaling to a 16-BPP screen. */
void PAL_BLENDING_BlitScaled16_Ctx(struct Atari800_Instance *inst, ULONG *dest, UBYTE *src, int pitch, int width, int height, int dest_width, int dest_height, int start_odd);
/* Blit with scaling to a 32-BPP screen. */
void PAL_BLENDING_BlitScaled32_Ctx(struct Atari800_Instance *inst, ULONG *dest, UBYTE *src, int pitch, int width, int height, int dest_width, int dest_height, int start_odd);

#include "instance.h"

#define PAL_BLENDING_UpdateLookup() \
	PAL_BLENDING_UpdateLookup_Ctx(Atari800_default)
#define PAL_BLENDING_Blit16(dest, src, pitch, width, height, start_odd) \
	PAL_BLENDING_Blit16_Ctx(Atari800_default, (dest), (src), (pitch), (width), (height), (start_odd))
#define PAL_BLENDING_Blit32(dest, src, pitch, width, height, start_odd) \
	PAL_BLENDING_Blit32_Ctx(Atari800_default, (dest), (src), (pitch), (width), (height), (start_odd))
#define PAL_BLENDING_BlitScaled16(dest, src, pitch, width, height, dest_width, dest_height, start_odd) \
	PAL_BLENDING_BlitScaled16_Ctx(Atari800_default, (dest), (src), (pitch), (width), (height), (dest_width), (dest_height), (start_odd))
#define PAL_BLENDING_BlitScaled32(dest, src, pitch, width, height, dest_width, dest_height, start_odd) \
	PAL_BLENDING_BlitScaled32_Ctx(Atari800_default, (dest), (src), (pitch), (width), (height), (dest_width), (dest_height), (start_odd))

#endif /* PAL_BLENDING_H_ */
