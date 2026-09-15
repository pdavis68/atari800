#ifndef BINLOAD_H_
#define BINLOAD_H_

#include <stdio.h> /* FILE */
#include "atari.h" /* UBYTE */
#include "instance.h" /* Atari800_Instance (transitional default-instance aliases) */

/* Transitional Option C bridge: the per-instance Binload state lives in
   Binload_state_t (instance.h). Until all callers pass an instance
   explicitly, the legacy global names are aliased to the default instance. */
#define BINLOAD_bin_file          (Atari800_default->binload.bin_file)
#define BINLOAD_start_binloading  (Atari800_default->binload.start_binloading)
#define BINLOAD_loading_basic     (Atari800_default->binload.loading_basic)
#define BINLOAD_slow_xex_loading  (Atari800_default->binload.slow_xex_loading)
#define BINLOAD_wait_active       (Atari800_default->binload.wait_active)
#define BINLOAD_pause_loading     (Atari800_default->binload.pause_loading)

/* Context-aware entry points (Option C). The legacy names below are
   forwarding macros that pass the default instance, so not-yet-migrated
   callers are unchanged. BINLOAD_LoaderStart is also invoked as an SIO
   boot-sector callback from sio.c's _Ctx bodies, which pass their own
   instance. The internal loader_cont handler remains a context-free
   ESC_Add callback (like the other escape handlers) that pins the default
   instance until the ESC handler machinery carries a context. */
int BINLOAD_Loader_Ctx(Atari800_Instance *inst, const char *filename);
int BINLOAD_LoaderStart_Ctx(Atari800_Instance *inst, UBYTE *buffer);

#define BINLOAD_Loader(filename)       BINLOAD_Loader_Ctx(Atari800_default, filename)
#define BINLOAD_LoaderStart(buffer)    BINLOAD_LoaderStart_Ctx(Atari800_default, buffer)

#define BINLOAD_LOADING_BASIC_SAVED              1
#define BINLOAD_LOADING_BASIC_LISTED             2
#define BINLOAD_LOADING_BASIC_LISTED_ATARI       3
#define BINLOAD_LOADING_BASIC_LISTED_LF          4
#define BINLOAD_LOADING_BASIC_LISTED_CR          5
#define BINLOAD_LOADING_BASIC_LISTED_CRLF        6
#define BINLOAD_LOADING_BASIC_RUN                7

#endif /* BINLOAD_H_ */
