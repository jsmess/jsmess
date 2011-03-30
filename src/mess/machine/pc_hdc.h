#ifndef PC_HDC_H
#define PC_HDC_H

#include "emu.h"

int pc_hdc_setup(running_machine &machine, void (*hdc_set_irq_func)(running_machine &,int,int));

READ8_HANDLER  ( pc_HDC1_r );
WRITE8_HANDLER ( pc_HDC1_w );
READ8_HANDLER  ( pc_HDC2_r );
WRITE8_HANDLER ( pc_HDC2_w );

READ16_HANDLER  ( pc16le_HDC1_r );
WRITE16_HANDLER ( pc16le_HDC1_w );
READ16_HANDLER  ( pc16le_HDC2_r );
WRITE16_HANDLER ( pc16le_HDC2_w );

READ32_HANDLER  ( pc32le_HDC1_r );
WRITE32_HANDLER ( pc32le_HDC1_w );
READ32_HANDLER  ( pc32le_HDC2_r );
WRITE32_HANDLER ( pc32le_HDC2_w );

int pc_hdc_dack_r(running_machine &machine);
void pc_hdc_dack_w(running_machine &machine,int data);

void pc_hdc_set_dma8237_device( device_t *dma8237 );

MACHINE_CONFIG_EXTERN( pc_hdc );

#endif /* PC_HDC_H */
