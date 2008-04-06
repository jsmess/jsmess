/*****************************************************************************
 *
 * includes/pc.h
 *
 ****************************************************************************/

#ifndef PC_H_
#define PC_H_

/*----------- defined in machine/pc.c -----------*/

extern const struct dma8237_interface pc_dma8237_config;

void mess_init_pc_common(UINT32 flags);

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

INTERRUPT_GEN( pc_cga_frame_interrupt );
INTERRUPT_GEN( pc_pc1512_frame_interrupt );
INTERRUPT_GEN( pc_mda_frame_interrupt );
INTERRUPT_GEN( tandy1000_frame_interrupt );
INTERRUPT_GEN( pc_aga_frame_interrupt );
INTERRUPT_GEN( pc_vga_frame_interrupt );


#endif /* PC_H_ */
