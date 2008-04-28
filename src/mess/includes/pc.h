/*****************************************************************************
 *
 * includes/pc.h
 *
 ****************************************************************************/

#ifndef PC_H_
#define PC_H_

#include "machine/ins8250.h"

/*----------- defined in machine/pc.c -----------*/

extern const struct dma8237_interface pc_dma8237_config;
extern const struct pit8253_config pc_pit8253_config;
extern const struct pit8253_config pcjr_pit8253_config;
extern const struct pic8259_interface pc_pic8259_master_config;
extern const struct pic8259_interface pc_pic8259_slave_config;
extern const ins8250_interface ibmpc_com_interface[4];

void mess_init_pc_common(UINT32 flags, void (*set_keyb_int_func)(int), void (*set_hdc_int_func)(int,int));

WRITE8_HANDLER( pc_nmi_enable_w );

READ8_HANDLER( pc_page_r );
WRITE8_HANDLER( pc_page_w );

DRIVER_INIT( pccga );
DRIVER_INIT( pcmda );
DRIVER_INIT( europc );
DRIVER_INIT( bondwell );
DRIVER_INIT( pc200 );
DRIVER_INIT( pc1512 );
DRIVER_INIT( pc1640 );
DRIVER_INIT( pc_vga );
DRIVER_INIT( t1000hx );

MACHINE_START( pc );
MACHINE_RESET( pc );

INTERRUPT_GEN( pc_frame_interrupt );
INTERRUPT_GEN( pc_vga_frame_interrupt );
INTERRUPT_GEN( pcjr_frame_interrupt );

#endif /* PC_H_ */
