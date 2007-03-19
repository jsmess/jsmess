/* ASG 971222 -- rewrote this interface */
#ifndef __I286INTR_H_
#define __I286INTR_H_

#include "i86intf.h"

enum {
		I286_PC=0,
        I286_IP, I286_AX, I286_CX, I286_DX, I286_BX, I286_SP, I286_BP, I286_SI, I286_DI,
        I286_FLAGS,
		I286_ES, I286_CS, I286_SS, I286_DS,
		I286_ES_2, I286_CS_2, I286_SS_2, I286_DS_2,
		I286_MSW,
		I286_GDTR, I286_IDTR, I286_LDTR, I286_TR,
		I286_GDTR_2, I286_IDTR_2, I286_LDTR_2, I286_TR_2,
        I286_VECTOR, I286_PENDING, I286_NMI_STATE, I286_IRQ_STATE, I286_EMPTY
};

/* Public functions */
extern void i286_set_irq_line(int irqline, int state);
extern void i286_set_irq_callback(int (*callback)(int irqline));
extern const char *i286_info(void *context, int regnum);
void i286_get_info(UINT32 state, cpuinfo *info);

#ifdef MAME_DEBUG
unsigned DasmI286(char* buffer, unsigned pc);
#endif

#endif
