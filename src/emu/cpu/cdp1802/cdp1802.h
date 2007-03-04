#ifndef _CDP1802_H
#define _CDP1802_H

#include "cpuintrf.h"

#define CDP1802_CYCLES_RESET 		1
#define CDP1802_CYCLES_INIT			9
#define CDP1802_CYCLES_FETCH		8
#define CDP1802_CYCLES_EXECUTE		8
#define CDP1802_CYCLES_DMA			1
#define CDP1802_CYCLES_INTERRUPT	1

enum {
	EF1 = 0x01,
	EF2 = 0x02,
	EF3 = 0x04,
	EF4 = 0x08
};

enum {
	CDP1802_STATE_0_FETCH,
	CDP1802_STATE_1_RESET,
	CDP1802_STATE_1_INIT,
	CDP1802_STATE_1_EXECUTE,
	CDP1802_STATE_2_DMA_IN,
	CDP1802_STATE_2_DMA_OUT,
	CDP1802_STATE_3_INT
};

enum {
	CDP1802_MODE_LOAD,
	CDP1802_MODE_RESET,
	CDP1802_MODE_PAUSE,
	CDP1802_MODE_RUN
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
};

void cdp1802_get_info(UINT32 state, cpuinfo *info);

#ifdef MAME_DEBUG
offs_t cdp1802_dasm(char *buffer, offs_t pc, const UINT8 *oprom, const UINT8 *opram);
#endif

typedef struct {
	/* called after execution of an instruction with cycles,
       return cycles taken by dma hardware */
	void (*dma)(int cycles);
	void (*out_q)(int level);
	int (*in_ef)(void);
} CDP1802_CONFIG;

void cdp1802_dma_write(UINT8 data);
int cdp1802_dma_read(void);

void cdp1802_set_mode(int mode);

#endif
