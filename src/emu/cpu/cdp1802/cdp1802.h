/*****************************************************************************
 *
 *   cdp1802.h
 *   portable cosmac cdp1802 emulator interface
 *
 *   Copyright (c) 2000 Peter Trauner, all rights reserved.
 *
 *   - This source code is released as freeware for non-commercial purposes.
 *   - You are free to use and redistribute this code in modified or
 *     unmodified form, provided you list me in the credits.
 *   - If you modify this source code, you must add a notice to each modified
 *     source file that it has been changed.  If you're a nice person, you
 *     will clearly mark each change too.  :)
 *   - If you wish to use this for commercial purposes, please contact me at
 *     peter.trauner@jk.uni-linz.ac.at
 *   - The author of this copywritten work reserves the right to change the
 *     terms of its usage and license at any time, including retroactively
 *   - This entire notice must remain in the source code.
 *
 *****************************************************************************/

#ifndef _CDP1802_H
#define _CDP1802_H

#include "cpuintrf.h"

#define CDP1802_INT_NONE	0
#define CDP1802_IRQ			1

// CDP1802 I/O Flags

enum {
	EF1 = 1,
	EF2	= 2,
	EF3	= 4,
	EF4 = 8
};

// CDP1802 Registers

enum {
	CDP1802_PC = 1,
	CDP1802_P,		// Designates which register is Program Counter
	CDP1802_X,		// Designates which register is Data Pointer
	CDP1802_D,		// Data Register (Accumulator)
	CDP1802_B,		// Auxiliary Holding Register
	CDP1802_T,		// Holds old X, P after Interrupt (X is high nibble)

	CDP1802_R0,		// 1 of 16 Scratchpad Registers
	CDP1802_R1,
	CDP1802_R2,
	CDP1802_R3,
	CDP1802_R4,
	CDP1802_R5,
	CDP1802_R6,
	CDP1802_R7,
	CDP1802_R8,
	CDP1802_R9,
	CDP1802_Ra,
	CDP1802_Rb,
	CDP1802_Rc,
	CDP1802_Rd,
	CDP1802_Re,
	CDP1802_Rf,

	CDP1802_DF,		// Data Flag (ALU Carry)
	CDP1802_IE,		// Interrupt Enable
	CDP1802_Q,		// Output Flip-Flop
	CDP1802_N,		// Holds Low-Order Instruction Digit
	CDP1802_I,		// Holds High-Order Instruction Digit
	CDP1802_IRQ_STATE,
};

// CDP1802 Configuration

typedef struct {
	/* called after execution of an instruction with cycles,
       return cycles taken by dma hardware */
	void (*dma)(int cycles);
	void (*out_q)(int level);
	int (*in_ef)(void);
} CDP1802_CONFIG;


void cdp1802_dma_write(UINT8 data);
int cdp1802_dma_read(void);

extern int cdp1802_icount;				// cycle count

void cdp1802_get_info(UINT32 state, cpuinfo *info);

#ifdef MAME_DEBUG
offs_t cdp1802_dasm(char *buffer, offs_t pc, const UINT8 *oprom, const UINT8 *opram);
#endif /* MAME_DEBUG */

#endif
