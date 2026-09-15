#ifndef MEMORY_H_
#define MEMORY_H_

#include "config.h"
#include <string.h>	/* memcpy, memset */

#include "atari.h"
#include "instance.h" /* MEMORY_state_t */

#define MEMORY_dGetByte(x)				(MEMORY_mem[x])
#define MEMORY_dPutByte(x, y)			(MEMORY_mem[x] = y)

#ifndef WORDS_BIGENDIAN
#ifdef WORDS_UNALIGNED_OK
#define MEMORY_dGetWord(x)				UNALIGNED_GET_WORD(MEMORY_mem+(x), memory_read_word_stat)
#define MEMORY_dPutWord(x, y)			UNALIGNED_PUT_WORD(MEMORY_mem+(x), (y), memory_write_word_stat)
#define MEMORY_dGetWordAligned(x)		UNALIGNED_GET_WORD(MEMORY_mem+(x), memory_read_aligned_word_stat)
#define MEMORY_dPutWordAligned(x, y)	UNALIGNED_PUT_WORD(MEMORY_mem+(x), (y), memory_write_aligned_word_stat)
#else	/* WORDS_UNALIGNED_OK */
#define MEMORY_dGetWord(x)				(MEMORY_mem[x] + (MEMORY_mem[(x) + 1] << 8))
#define MEMORY_dPutWord(x, y)			(MEMORY_mem[x] = (UBYTE) (y), MEMORY_mem[(x) + 1] = (UBYTE) ((y) >> 8))
/* faster versions of MEMORY_jdGetWord and MEMORY_dPutWord for even addresses */
/* TODO: guarantee that memory is UWORD-aligned and use UWORD access */
#define MEMORY_dGetWordAligned(x)		MEMORY_dGetWord(x)
#define MEMORY_dPutWordAligned(x, y)	MEMORY_dPutWord(x, y)
#endif	/* WORDS_UNALIGNED_OK */
#else	/* WORDS_BIGENDIAN */
/* can't do any word optimizations for big endian machines */
#define MEMORY_dGetWord(x)				(MEMORY_mem[x] + (MEMORY_mem[(x) + 1] << 8))
#define MEMORY_dPutWord(x, y)			(MEMORY_mem[x] = (UBYTE) (y), MEMORY_mem[(x) + 1] = (UBYTE) ((y) >> 8))
#define MEMORY_dGetWordAligned(x)		MEMORY_dGetWord(x)
#define MEMORY_dPutWordAligned(x, y)	MEMORY_dPutWord(x, y)
#endif	/* WORDS_BIGENDIAN */

#define MEMORY_dCopyFromMem(from, to, size)	memcpy(to, MEMORY_mem + (from), size)
#define MEMORY_dCopyToMem(from, to, size)		memcpy(MEMORY_mem + (to), from, size)
#define MEMORY_dFillMem(addr1, value, length)	memset(MEMORY_mem + (addr1), value, length)

/* The context-aware memory accessors (MEMORY_*Ctx) are defined at the end of
   this header, after the MEMORY_HwGetByte/MEMORY_HwPutByte declarations. */

/* The legacy memory globals are aliased to the default instance's memory so
   the tree stays buildable during the incremental Option C migration. */
#define MEMORY_mem (Atari800_default->memory.mem)

/* RAM size in kilobytes.
   Valid values for Atari800_MACHINE_800 are: 16, 48, 52.
   Valid values for Atari800_MACHINE_XLXE are: 16, 64, 128, 192, RAM_320_RAMBO,
   RAM_320_COMPY_SHOP, 576, 1088.
   The only valid value for Atari800_MACHINE_5200 is 16. */
#define MEMORY_RAM_320_RAMBO       320
#define MEMORY_RAM_320_COMPY_SHOP  321
/* Per-instance memory state is aliased to the default instance during the
   incremental Option C migration (see docs/multi-instance-refactor.md). */
#define MEMORY_ram_size        (Atari800_default->memory.ram_size)
#define MEMORY_xe_bank         (Atari800_default->memory.xe_bank)
#define MEMORY_selftest_enabled (Atari800_default->memory.selftest_enabled)
#define MEMORY_have_basic      (Atari800_default->memory.have_basic)
#define MEMORY_cartA0BF_enabled (Atari800_default->memory.cartA0BF_enabled)
#define MEMORY_mosaic_num_banks (Atari800_default->memory.mosaic_num_banks)
#define MEMORY_axlon_0f_mirror (Atari800_default->memory.axlon_0f_mirror)
#define MEMORY_axlon_num_banks (Atari800_default->memory.axlon_num_banks)
#define MEMORY_enable_mapram   (Atari800_default->memory.enable_mapram)
#define MEMORY_os              (Atari800_default->memory.os)
#define MEMORY_basic           (Atari800_default->memory.basic)
#define MEMORY_xegame          (Atari800_default->memory.xegame)

#define MEMORY_RAM       0
#define MEMORY_ROM       1
#define MEMORY_HARDWARE  2

#ifndef PAGED_ATTRIB

#define MEMORY_attrib (Atari800_default->memory.attrib)
/* Reads a byte from ADDR. Can potentially have side effects, when reading
   from hardware area. */
#define MEMORY_GetByte(addr)		(MEMORY_attrib[addr] == MEMORY_HARDWARE ? MEMORY_HwGetByte(addr, FALSE) : MEMORY_mem[addr])
/* Reads a byte from ADDR, but without any side effects. */
#define MEMORY_SafeGetByte(addr)		(MEMORY_attrib[addr] == MEMORY_HARDWARE ? MEMORY_HwGetByte(addr, TRUE) : MEMORY_mem[addr])
#define MEMORY_PutByte(addr, byte)	 do { if (MEMORY_attrib[addr] == MEMORY_RAM) MEMORY_mem[addr] = byte; else if (MEMORY_attrib[addr] == MEMORY_HARDWARE) MEMORY_HwPutByte(addr, byte); } while (0)
#define MEMORY_SetRAM(addr1, addr2) memset(MEMORY_attrib + (addr1), MEMORY_RAM, (addr2) - (addr1) + 1)
#define MEMORY_SetROM(addr1, addr2) memset(MEMORY_attrib + (addr1), MEMORY_ROM, (addr2) - (addr1) + 1)
#define MEMORY_SetHARDWARE(addr1, addr2) memset(MEMORY_attrib + (addr1), MEMORY_HARDWARE, (addr2) - (addr1) + 1)

#else /* PAGED_ATTRIB */

typedef UBYTE (*MEMORY_rdfunc)(UWORD addr, int no_side_effects);
typedef void (*MEMORY_wrfunc)(UWORD addr, UBYTE value);
#define MEMORY_readmap (Atari800_default->memory.readmap)
#define MEMORY_safe_readmap (Atari800_default->memory.safe_readmap)
#define MEMORY_writemap (Atari800_default->memory.writemap)
void MEMORY_ROM_PutByte(UWORD addr, UBYTE byte);
/* Reads a byte from ADDR. Can potentially have side effects, when reading
   from hardware area. */
#define MEMORY_GetByte(addr)		(MEMORY_readmap[(addr) >> 8] ? (*MEMORY_readmap[(addr) >> 8])(addr, FALSE) : MEMORY_mem[addr])
/* Reads a byte from ADDR, but without any side effects. */
#define MEMORY_SafeGetByte(addr)		(MEMORY_readmap[(addr) >> 8] ? (*MEMORY_readmap[(addr) >> 8])(addr, TRUE) : MEMORY_mem[addr])
#define MEMORY_PutByte(addr,byte)	(MEMORY_writemap[(addr) >> 8] ? ((*MEMORY_writemap[(addr) >> 8])(addr, byte), 0) : (MEMORY_mem[addr] = byte))
#define MEMORY_SetRAM(addr1, addr2) do { \
		int i; \
		for (i = (addr1) >> 8; i <= (addr2) >> 8; i++) { \
			MEMORY_readmap[i] = NULL; \
			MEMORY_writemap[i] = NULL; \
		} \
	} while (0)
#define MEMORY_SetROM(addr1, addr2) do { \
		int i; \
		for (i = (addr1) >> 8; i <= (addr2) >> 8; i++) { \
			MEMORY_readmap[i] = NULL; \
			MEMORY_writemap[i] = MEMORY_ROM_PutByte; \
		} \
	} while (0)

#endif /* PAGED_ATTRIB */

/* Verifies if SIZE is a correct value for RAM size. (stateless, no context) */
int MEMORY_SizeValid(int size);

/* Context-aware memory subsystem functions (Option C refactor). Each takes
   the instance whose MEMORY_state_t it operates on. The legacy un-suffixed
   names below are forwarding macros that route to Atari800_default, so
   not-yet-migrated callers keep working unchanged. */

void MEMORY_InitialiseMachineCtx(Atari800_Instance *inst);
void MEMORY_StateSaveCtx(Atari800_Instance *inst, UBYTE SaveVerbose);
void MEMORY_StateReadCtx(Atari800_Instance *inst, UBYTE SaveVerbose, UBYTE StateVersion);
void MEMORY_CopyFromMemCtx(Atari800_Instance *inst, UWORD from, UBYTE *to, int size);
void MEMORY_CopyToMemCtx(Atari800_Instance *inst, const UBYTE *from, UWORD to, int size);
void MEMORY_HandlePORTBCtx(Atari800_Instance *inst, UBYTE byte, UBYTE oldval);
void MEMORY_Cart809fDisableCtx(Atari800_Instance *inst);
void MEMORY_Cart809fEnableCtx(Atari800_Instance *inst);
void MEMORY_CartA0bfDisableCtx(Atari800_Instance *inst);
void MEMORY_CartA0bfEnableCtx(Atari800_Instance *inst);
void MEMORY_GetCharsetCtx(Atari800_Instance *inst, UBYTE *cs);

#define MEMORY_InitialiseMachine()          MEMORY_InitialiseMachineCtx(Atari800_default)
#define MEMORY_StateSave(SaveVerbose)       MEMORY_StateSaveCtx(Atari800_default, (SaveVerbose))
#define MEMORY_StateRead(SaveVerbose, StateVersion) \
                                            MEMORY_StateReadCtx(Atari800_default, (SaveVerbose), (StateVersion))
#define MEMORY_CopyFromMem(from, to, size)  MEMORY_CopyFromMemCtx(Atari800_default, (from), (to), (size))
#define MEMORY_CopyToMem(from, to, size)    MEMORY_CopyToMemCtx(Atari800_default, (from), (to), (size))
#define MEMORY_HandlePORTB(byte, oldval)    MEMORY_HandlePORTBCtx(Atari800_default, (byte), (oldval))
#define MEMORY_Cart809fDisable()            MEMORY_Cart809fDisableCtx(Atari800_default)
#define MEMORY_Cart809fEnable()             MEMORY_Cart809fEnableCtx(Atari800_default)
#define MEMORY_CartA0bfDisable()            MEMORY_CartA0bfDisableCtx(Atari800_default)
#define MEMORY_CartA0bfEnable()             MEMORY_CartA0bfEnableCtx(Atari800_default)
#define MEMORY_GetCharset(cs)               MEMORY_GetCharsetCtx(Atari800_default, (cs))

#define MEMORY_CopyFromCart(addr1, addr2, src) memcpy(MEMORY_mem + (addr1), src, (addr2) - (addr1) + 1)
#define MEMORY_CopyToCart(addr1, addr2, dst) memcpy(dst, MEMORY_mem + (addr1), (addr2) - (addr1) + 1)

#ifndef PAGED_MEM
/* Reads a byte from the specified special address (not RAM or ROM). */
UBYTE MEMORY_HwGetByte(UWORD addr, int safe);

/* Stores a byte at the specified special address (not RAM or ROM). */
void MEMORY_HwPutByte(UWORD addr, UBYTE byte);
#endif /* PAGED_MEM */

/* ------------------------------------------------------------------ */
/* Context-aware memory accessors (Option C refactor)                 */
/*                                                                    */
/* These take a MEMORY_state_t * as the first parameter and are used  */
/* by modules that have been migrated to the instance context. The    */
/* legacy macros above remain for modules not yet migrated.           */
/* ------------------------------------------------------------------ */

static inline UBYTE MEMORY_dGetByteCtx(MEMORY_state_t *m, UWORD x)
{
	return m->mem[x];
}

static inline void MEMORY_dPutByteCtx(MEMORY_state_t *m, UWORD x, UBYTE y)
{
	m->mem[x] = y;
}

static inline UWORD MEMORY_dGetWordCtx(MEMORY_state_t *m, UWORD x)
{
	return m->mem[x] + (m->mem[(x) + 1] << 8);
}

static inline void MEMORY_dPutWordCtx(MEMORY_state_t *m, UWORD x, UWORD y)
{
	m->mem[x] = (UBYTE) y;
	m->mem[(x) + 1] = (UBYTE) (y >> 8);
}

static inline UWORD MEMORY_dGetWordAlignedCtx(MEMORY_state_t *m, UWORD x)
{
	return MEMORY_dGetWordCtx(m, x);
}

static inline void MEMORY_dPutWordAlignedCtx(MEMORY_state_t *m, UWORD x, UWORD y)
{
	MEMORY_dPutWordCtx(m, x, y);
}

#ifndef PAGED_ATTRIB
static inline UBYTE MEMORY_GetByteCtx(MEMORY_state_t *m, UWORD addr, int safe)
{
	return m->attrib[addr] == MEMORY_HARDWARE ? MEMORY_HwGetByte(addr, safe) : m->mem[addr];
}

static inline UBYTE MEMORY_SafeGetByteCtx(MEMORY_state_t *m, UWORD addr)
{
	return m->attrib[addr] == MEMORY_HARDWARE ? MEMORY_HwGetByte(addr, TRUE) : m->mem[addr];
}

static inline void MEMORY_PutByteCtx(MEMORY_state_t *m, UWORD addr, UBYTE byte)
{
	if (m->attrib[addr] == MEMORY_RAM)
		m->mem[addr] = byte;
	else if (m->attrib[addr] == MEMORY_HARDWARE)
		MEMORY_HwPutByte(addr, byte);
}
#else /* PAGED_ATTRIB */
static inline UBYTE MEMORY_GetByteCtx(MEMORY_state_t *m, UWORD addr, int safe)
{
	return m->readmap[(addr) >> 8] ? (*m->readmap[(addr) >> 8])(addr, safe) : m->mem[addr];
}

static inline UBYTE MEMORY_SafeGetByteCtx(MEMORY_state_t *m, UWORD addr)
{
	return m->safe_readmap[(addr) >> 8] ? (*m->safe_readmap[(addr) >> 8])(addr, TRUE) : m->mem[addr];
}

static inline void MEMORY_PutByteCtx(MEMORY_state_t *m, UWORD addr, UBYTE byte)
{
	if (m->writemap[(addr) >> 8])
		(*m->writemap[(addr) >> 8])(addr, byte);
	else
		m->mem[addr] = byte;
}
#endif /* PAGED_ATTRIB */

static inline void MEMORY_dCopyFromMemCtx(MEMORY_state_t *m, UWORD from, UBYTE *to, int size)
{
	memcpy(to, m->mem + from, size);
}

static inline void MEMORY_dCopyToMemCtx(MEMORY_state_t *m, const UBYTE *from, UWORD to, int size)
{
	memcpy(m->mem + to, from, size);
}

static inline void MEMORY_dFillMemCtx(MEMORY_state_t *m, UWORD addr1, UBYTE value, int length)
{
	memset(m->mem + addr1, value, length);
}

#endif /* MEMORY_H_ */
