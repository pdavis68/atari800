#ifndef MONITOR_H_
#define MONITOR_H_

#include "config.h"
#include <stdio.h>

#include "atari.h"
#include "instance.h" /* Atari800_Instance, Monitor_state_t (transitional aliases) */

/* Option C refactor (docs/refactor-checklist.md §4.6): the monitor's
   break/breakpoint/coverage state lives in Monitor_state_t, embedded in
   Atari800_Instance. Each public function takes the instance it attaches
   to; the legacy un-suffixed names below are forwarding macros that route
   to Atari800_default, so not-yet-migrated callers keep working unchanged. */

int MONITOR_Run_Ctx(Atari800_Instance *inst);
#define MONITOR_Run() MONITOR_Run_Ctx(Atari800_default)

#ifdef MONITOR_HINTS
void MONITOR_PreloadLabelFile(char *filename);
#endif

#ifdef MONITOR_TRACE
/* Host-side trace output file (process-global, not per-instance). */
extern FILE *MONITOR_trace_file;
#endif

#ifdef MONITOR_BREAK
void MONITOR_BBRK_on_Ctx(Atari800_Instance *inst);
void MONITOR_BPC_Ctx(Atari800_Instance *inst, char *arg);
#define MONITOR_BBRK_on() MONITOR_BBRK_on_Ctx(Atari800_default)
#define MONITOR_BPC(arg)  MONITOR_BPC_Ctx(Atari800_default, (arg))

#define MONITOR_break_addr  (Atari800_default->monitor.break_addr)
#define MONITOR_break_step  (Atari800_default->monitor.break_step)
#define MONITOR_break_ret   (Atari800_default->monitor.break_ret)
#define MONITOR_break_brk   (Atari800_default->monitor.break_brk)
#define MONITOR_ret_nesting (Atari800_default->monitor.ret_nesting)
#endif

extern const UBYTE MONITOR_optype6502[256]; /* shared, read-only */

void MONITOR_Exit_Ctx(Atari800_Instance *inst);
#define MONITOR_Exit() MONITOR_Exit_Ctx(Atari800_default)

void MONITOR_ShowState_Ctx(Atari800_Instance *inst, FILE *fp, UWORD pc, UBYTE a, UBYTE x, UBYTE y, UBYTE s,
                char n, char v, char z, char c);
#define MONITOR_ShowState(fp, pc, a, x, y, s, n, v, z, c) \
	MONITOR_ShowState_Ctx(Atari800_default, (fp), (pc), (a), (x), (y), (s), (n), (v), (z), (c))

#ifdef MONITOR_BREAKPOINTS

/* Breakpoint conditions */

#define MONITOR_BREAKPOINT_OR          1
#define MONITOR_BREAKPOINT_FLAG_CLEAR  2
#define MONITOR_BREAKPOINT_FLAG_SET    3

/* these three may be ORed together and must be ORed with MONITOR_BREAKPOINT_PC .. MONITOR_BREAKPOINT_WRITE */
#define MONITOR_BREAKPOINT_LESS        1
#define MONITOR_BREAKPOINT_EQUAL       2
#define MONITOR_BREAKPOINT_GREATER     4

#define MONITOR_BREAKPOINT_PC          8
#define MONITOR_BREAKPOINT_A           16
#define MONITOR_BREAKPOINT_X           32
#define MONITOR_BREAKPOINT_Y           40
#define MONITOR_BREAKPOINT_S           48
#define MONITOR_BREAKPOINT_READ        64
#define MONITOR_BREAKPOINT_WRITE       128
#define MONITOR_BREAKPOINT_MEMORY      256
#define MONITOR_BREAKPOINT_ACCESS      (MONITOR_BREAKPOINT_READ | MONITOR_BREAKPOINT_WRITE)

/* MONITOR_breakpoint_cond and MONITOR_BREAKPOINT_TABLE_MAX are defined in
   instance.h (Monitor_state_t embeds the table by value). */
#define MONITOR_breakpoint_table        (Atari800_default->monitor.breakpoint_table)
#define MONITOR_breakpoint_table_size   (Atari800_default->monitor.breakpoint_table_size)
#define MONITOR_breakpoints_enabled     (Atari800_default->monitor.breakpoints_enabled)

#endif /* MONITOR_BREAKPOINTS */

#ifdef MONITOR_PROFILE

#define MONITOR_coverage        (Atari800_default->monitor.coverage)
#define MONITOR_coverage_insns  (Atari800_default->monitor.coverage_insns)
#define MONITOR_coverage_cycles (Atari800_default->monitor.coverage_cycles)

#endif /* MONITOR_PROFILE */

#endif /* MONITOR_H_ */
