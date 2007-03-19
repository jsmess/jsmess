/* ASG 971222 -- rewrote this interface */
#ifndef __I186INTR_H_
#define __I186INTR_H_

#include "i86intf.h"

/* Public functions */
void i186_get_info(UINT32 state, cpuinfo *info);

#ifdef MAME_DEBUG
extern unsigned DasmI186(char* buffer, unsigned pc);
#endif

#endif
