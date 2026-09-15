/*
 * cpu.h - 6502 CPU emulation
 *
 * Copyright (C) 1995-1998 David Firth
 * Copyright (C) 1998-2025 Atari800 development team (see DOC/CREDITS)
 *
 * This file is part of the Atari800 emulator project which emulates
 * the Atari 400, 800, 800XL, 130XE, and 5200 8-bit computers.
 *
 * Atari800 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Atari800 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Atari800; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef CPU_H_
#define CPU_H_

#include "config.h"
#ifdef ASAP /* external project, see http://asap.sf.net */
#include "asap_internal.h"
#else
#include "atari.h"
#include "instance.h"
#endif

#define CPU_N_FLAG 0x80
#define CPU_V_FLAG 0x40
#define CPU_B_FLAG 0x10
#define CPU_D_FLAG 0x08
#define CPU_I_FLAG 0x04
#define CPU_Z_FLAG 0x02
#define CPU_C_FLAG 0x01

void CPU_GetStatus(Atari800_Instance *inst);
void CPU_PutStatus(Atari800_Instance *inst);
void CPU_Reset(Atari800_Instance *inst);
void CPU_StateSave(Atari800_Instance *inst, UBYTE SaveVerbose);
void CPU_StateRead(Atari800_Instance *inst, UBYTE SaveVerbose, UBYTE StateVersion);
void CPU_NMI(Atari800_Instance *inst);
void CPU_GO(Atari800_Instance *inst, int limit);
#define CPU_GenerateIRQ() (CPU_IRQ = 1)

/* Transitional bridge (Option C refactor): the CPU register globals are aliased
   to the default instance's CPU state so the tree stays buildable during the
   incremental migration. Modules that have been migrated use inst->cpu directly;
   CPU_GO()/CPU_NMI()/CPU_Reset() redefine these names to the instance they
   operate on (see cpu.c). */
#define CPU_regPC (Atari800_default->cpu.regPC)
#define CPU_regA  (Atari800_default->cpu.regA)
#define CPU_regP  (Atari800_default->cpu.regP)
#define CPU_regS  (Atari800_default->cpu.regS)
#define CPU_regY  (Atari800_default->cpu.regY)
#define CPU_regX  (Atari800_default->cpu.regX)

#define CPU_SetN CPU_regP |= CPU_N_FLAG
#define CPU_ClrN CPU_regP &= (~CPU_N_FLAG)
#define CPU_SetV CPU_regP |= CPU_V_FLAG
#define CPU_ClrV CPU_regP &= (~CPU_V_FLAG)
#define CPU_SetB CPU_regP |= CPU_B_FLAG
#define CPU_ClrB CPU_regP &= (~CPU_B_FLAG)
#define CPU_SetD CPU_regP |= CPU_D_FLAG
#define CPU_ClrD CPU_regP &= (~CPU_D_FLAG)
#define CPU_SetI CPU_regP |= CPU_I_FLAG
#define CPU_ClrI CPU_regP &= (~CPU_I_FLAG)
#define CPU_SetZ CPU_regP |= CPU_Z_FLAG
#define CPU_ClrZ CPU_regP &= (~CPU_Z_FLAG)
#define CPU_SetC CPU_regP |= CPU_C_FLAG
#define CPU_ClrC CPU_regP &= (~CPU_C_FLAG)

#define CPU_IRQ (Atari800_default->cpu.IRQ)

#define CPU_rts_handler (Atari800_default->cpu.rts_handler)

#define CPU_cim_encountered (Atari800_default->cpu.cim_encountered)

#define CPU_REMEMBER_PC_STEPS 64
#ifdef MONITOR_BREAK
#define CPU_remember_PC (Atari800_default->cpu.remember_PC)
#define CPU_remember_op (Atari800_default->cpu.remember_op)
#define CPU_remember_PC_curpos (Atari800_default->cpu.remember_PC_curpos)
#define CPU_remember_xpos (Atari800_default->cpu.remember_xpos)
#endif

#define CPU_REMEMBER_JMP_STEPS 16
#ifdef MONITOR_BREAK
#define CPU_remember_JMP (Atari800_default->cpu.remember_JMP)
#define CPU_remember_jmp_curpos (Atari800_default->cpu.remember_jmp_curpos)
#endif

#ifdef MONITOR_PROFILE
#define CPU_instruction_count (Atari800_default->cpu.instruction_count)
#endif

#endif /* CPU_H_ */
