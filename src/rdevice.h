/*
 * rdevice.h - Atari850 emulation header file
 *
 * Copyright (c) ???? Tom Hunt, Chris Martin
 * Copyright (c) 2003,2008 Atari800 development team (see DOC/CREDITS)
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

#ifndef RDEVICE_H_
#define RDEVICE_H_

#include "instance.h" /* Atari800_Instance, Atari800_default */

/* Context-aware entry points. */
extern void RDevice_OPEN_Ctx(Atari800_Instance *inst);
extern void RDevice_CLOS_Ctx(Atari800_Instance *inst);
extern void RDevice_READ_Ctx(Atari800_Instance *inst);
extern void RDevice_WRIT_Ctx(Atari800_Instance *inst);
extern void RDevice_STAT_Ctx(Atari800_Instance *inst);
extern void RDevice_SPEC_Ctx(Atari800_Instance *inst);
extern void RDevice_INIT_Ctx(Atari800_Instance *inst);

extern void RDevice_Exit_Ctx(Atari800_Instance *inst);

/* The RDevice_OPEN..INIT functions are registered as context-free escape
   handlers (ESC_AddEscRts in devices.c); they remain real functions that
   pin the default instance and forward to the _Ctx versions. */
extern void RDevice_OPEN(void);
extern void RDevice_CLOS(void);
extern void RDevice_READ(void);
extern void RDevice_WRIT(void);
extern void RDevice_STAT(void);
extern void RDevice_SPEC(void);
extern void RDevice_INIT(void);

extern void RDevice_Exit(void);

/* State aliases (transitional: default instance). */
#define RDevice_serial_enabled (Atari800_default->rdevice.serial_enabled)
#define RDevice_serial_device  (Atari800_default->rdevice.serial_device)

#endif /* RDEVICE_H_ */
