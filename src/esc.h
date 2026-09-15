#ifndef ESC_H_
#define ESC_H_

#include "atari.h"
#include "instance.h" /* Atari800_Instance, ESC_FunctionType (transitional default-instance aliases) */

/* Transitional Option C bridge: the per-instance ESC state lives in
   ESC_state_t (instance.h). Until all callers pass an instance explicitly,
   the legacy global names are aliased to the default instance. */
#define ESC_enable_sio_patch (Atari800_default->esc.enable_sio_patch)

/* Escape codes used to mark places in 6502 code that must
   be handled specially by the emulator. An escape sequence
   is an illegal 6502 opcode 0xF2 or 0xD2 followed
   by one of these escape codes: */
enum ESC_t {

	/* SIO patch. */
	ESC_SIOV,

	/* stdio-based handlers for the BASIC version
	   and handlers for Atari Basic loader. */
	ESC_EHOPEN,
	ESC_EHCLOS,
	ESC_EHREAD,
	ESC_EHWRIT,
	ESC_EHSTAT,
	ESC_EHSPEC,

	ESC_KHOPEN,
	ESC_KHCLOS,
	ESC_KHREAD,
	ESC_KHWRIT,
	ESC_KHSTAT,
	ESC_KHSPEC,

	/* Atari executable loader. */
	ESC_BINLOADER_CONT,

	/* Cassette emulation. */
	ESC_COPENLOAD = 0xa8,
	ESC_COPENSAVE = 0xa9,

	/* Printer. */
	ESC_PHOPEN = 0xb0,
	ESC_PHCLOS = 0xb1,
	ESC_PHREAD = 0xb2,
	ESC_PHWRIT = 0xb3,
	ESC_PHSTAT = 0xb4,
	ESC_PHSPEC = 0xb5,
	ESC_PHINIT = 0xb6,

#ifdef R_IO_DEVICE
	/* R: device. */
	ESC_ROPEN = 0xd0,
	ESC_RCLOS = 0xd1,
	ESC_RREAD = 0xd2,
	ESC_RWRIT = 0xd3,
	ESC_RSTAT = 0xd4,
	ESC_RSPEC = 0xd5,
	ESC_RINIT = 0xd6,
#endif

	/* H: device. */
	ESC_HHOPEN = 0xc0,
	ESC_HHCLOS = 0xc1,
	ESC_HHREAD = 0xc2,
	ESC_HHWRIT = 0xc3,
	ESC_HHSTAT = 0xc4,
	ESC_HHSPEC = 0xc5,
	ESC_HHINIT = 0xc6,

	/* B: device. */
	ESC_BOPEN = 0xe0,
	ESC_BCLOS = 0xe1,
	ESC_BREAD = 0xe2,
	ESC_BWRIT = 0xe3,
	ESC_BSTAT = 0xe4,
	ESC_BSPEC = 0xe5,
	ESC_BINIT = 0xe6
};

/* A function called to handle an escape sequence is typedef'd as
   ESC_FunctionType in instance.h (it is context-free for now; registered
   handlers pin the default instance until every handler module is
   converted). */

/* Context-aware entry points (Option C). The legacy names below are
   forwarding macros that pass the default instance, so not-yet-migrated
   callers are unchanged. cpu.c passes its own instance to ESC_Run_Ctx
   because the escape machinery runs inside the CPU decoder's context. */
void ESC_Add_Ctx(Atari800_Instance *inst, UWORD address, UBYTE esc_code, ESC_FunctionType function);
void ESC_AddEscRts_Ctx(Atari800_Instance *inst, UWORD address, UBYTE esc_code, ESC_FunctionType function);
void ESC_AddEscRts2_Ctx(Atari800_Instance *inst, UWORD address, UBYTE esc_code, ESC_FunctionType function);
void ESC_Remove_Ctx(Atari800_Instance *inst, UBYTE esc_code);
void ESC_Run_Ctx(Atari800_Instance *inst, UBYTE esc_code);
void ESC_PatchOS_Ctx(Atari800_Instance *inst);
void ESC_ClearAll_Ctx(Atari800_Instance *inst);
void ESC_UpdatePatches_Ctx(Atari800_Instance *inst);

#define ESC_Add(addr, code, fn)        ESC_Add_Ctx(Atari800_default, addr, code, fn)
#define ESC_AddEscRts(addr, code, fn)  ESC_AddEscRts_Ctx(Atari800_default, addr, code, fn)
#define ESC_AddEscRts2(addr, code, fn) ESC_AddEscRts2_Ctx(Atari800_default, addr, code, fn)
#define ESC_Remove(code)               ESC_Remove_Ctx(Atari800_default, code)
#define ESC_Run(code)                  ESC_Run_Ctx(Atari800_default, code)
#define ESC_PatchOS()                  ESC_PatchOS_Ctx(Atari800_default)
#define ESC_ClearAll()                 ESC_ClearAll_Ctx(Atari800_default)
#define ESC_UpdatePatches()            ESC_UpdatePatches_Ctx(Atari800_default)

#endif /* ESC_H_ */
