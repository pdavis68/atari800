/*
 * pokeyrec.c - record pokey registers to a file
 *
 * Copyright (C) 2015 Ivo van Poorten
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
*/

#include "config.h"
#include "pokeyrec.h"
#include "pokey.h"
#include "log.h"
#include "util.h"
#include <string.h>
#include <stdio.h>

/* Transitional Option C bridge: the per-instance Pokeyrec state lives in
   Pokeyrec_state_t (instance.h). The *_Ctx entry points pin the file-scope
   context (RECC = &inst->pokeyrec); inside this file the legacy state names
   route through RECC, so the _Ctx bodies operate on their own instance. */
static Pokeyrec_state_t *RECC;

#define POKEYREC_PIN_CTX(inst) do { \
	RECC = &(inst)->pokeyrec; \
} while (0)

/* Route the legacy state names through the pinned context.
   Non-zero defaults match the old static initialisers; they are set in
   POKEYREC_Initialise_Ctx (the instance state itself is zero-initialised). */
#define enabled  (RECC->enabled)
#define counter  (RECC->counter)
#define interval (RECC->interval)
#define filename (RECC->filename)
#define fmt      (RECC->fmt)
#define fp       (RECC->fp)
#ifdef STEREO_SOUND
#define stereo   (RECC->stereo)
#endif

static void output_pokey_values(int pokeynr) {
    int i;
    for (i=0; i<4; i++) {
        fprintf(fp, fmt, POKEY_AUDF[(pokeynr*4)+i]);
        fprintf(fp, fmt, POKEY_AUDC[(pokeynr*4)+i]);
    }
    fprintf(fp, fmt, POKEY_AUDCTL[pokeynr]);
}

void POKEYREC_Recorder_Ctx(Atari800_Instance *inst) {
    POKEYREC_PIN_CTX(inst);
    if (!enabled) return;

    if (++counter == interval) {
        counter = 0;
        output_pokey_values(0);
#ifdef STEREO_SOUND
        if (stereo) output_pokey_values(1);
#endif
        if (fmt[1] != 'c')
            fputc('\n', fp);
    }
}

int POKEYREC_Initialise_Ctx(Atari800_Instance *inst, int *argc, char *argv[]) {
    int i, j;

    POKEYREC_PIN_CTX(inst);
    if (!filename) filename = "pokeyrec.dat";
    if (!fmt) fmt = "%c";
    interval = Atari800_tv_mode;

    for (i = j = 1; i < *argc; i++) {
        int available = i + 1 < *argc;

        if (!strcmp(argv[i], "-pokeyrec")) {
            enabled = 1;
        } else if (!strcmp(argv[i], "-pokeyrec-interval")) {
            if (!available) goto missing_argument;
            interval = Util_sscandec(argv[++i]);
            if (interval < 0) {
                Log_print("Negative interval not allowed");
                return FALSE;
            }
        } else if (!strcmp(argv[i], "-pokeyrec-ascii")) {
            fmt = "%02x";
        } else if (!strcmp(argv[i], "-pokeyrec-file")) {
            if (!available) goto missing_argument;
            filename = Util_strdup(argv[++i]);
#ifdef STEREO_SOUND
        } else if (!strcmp(argv[i], "-pokeyrec-stereo")) {
            stereo = 1;
#endif
        } else {
            if (!strcmp(argv[i], "-help")) {
                Log_print("\t-pokeyrec                  "
                                "Enable Pokey registers recording");
                Log_print("\t-pokeyrec-interval <n>     "
                                "Sampling interval in scanlines (default: %d)",
                                                                    interval);
                Log_print("\t-pokeyrec-ascii            "
                                "Store ascii values (default: raw)");
                Log_print("\t-pokeyrec-file <filename>  "
                                "Specify output filename "
                                                    "(default: pokeyrec.dat)");
#ifdef STEREO_SOUND
                Log_print("\t-pokeyrec-stereo           "
                                "Record second Pokey, too "
                                                    "(default: mono)");
#endif
            }
            argv[j++] = argv[i];
        }
    }

    *argc = j;

    if (enabled) {
        if (!(fp = fopen(filename, "wb"))) {
            Log_print("Unable to open '%s' for writing", filename);
            return FALSE;
        }
    }

    return TRUE;

missing_argument:
    Log_print("Missing argument for '%s'", argv[i]);
    return FALSE;
}

void POKEYREC_Exit_Ctx(Atari800_Instance *inst) {
    POKEYREC_PIN_CTX(inst);
    if (fp) fclose(fp);
}
