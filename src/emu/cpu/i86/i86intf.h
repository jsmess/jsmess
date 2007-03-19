/* ASG 971222 -- rewrote this interface */
#ifndef __I86INTRF_H_
#define __I86INTRF_H_

#include "cpuintrf.h"

enum {
	I86_PC=0,
	I86_IP, I86_AX, I86_CX, I86_DX, I86_BX, I86_SP, I86_BP, I86_SI, I86_DI,
	I86_FLAGS, I86_ES, I86_CS, I86_SS, I86_DS,
	I86_VECTOR
};

/* Public functions */
void i86_get_info(UINT32 state, cpuinfo *info);

#ifdef MAME_DEBUG
extern unsigned DasmI86(char* buffer, unsigned pc);
#endif

#endif
