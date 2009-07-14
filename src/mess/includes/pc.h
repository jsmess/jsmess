/*****************************************************************************
 *
 * includes/pc.h
 *
 ****************************************************************************/

#ifndef PC_H_
#define PC_H_

#include "machine/ins8250.h"
#include "machine/i8255a.h"

/*----------- defined in machine/pc.c -----------*/

extern const struct dma8237_interface ibm5150_dma8237_config;
extern const struct pit8253_config ibm5150_pit8253_config;
extern const struct pit8253_config pcjr_pit8253_config;
extern const struct pic8259_interface ibm5150_pic8259_master_config;
extern const struct pic8259_interface ibm5150_pic8259_slave_config;
extern const struct pic8259_interface pcjr_pic8259_master_config;
extern const ins8250_interface ibm5150_com_interface[4];
extern const i8255a_interface ibm5150_ppi8255_interface;
extern const i8255a_interface ibm5160_ppi8255_interface;
extern const i8255a_interface pc_ppi8255_interface;
extern const i8255a_interface pcjr_ppi8255_interface;

UINT8 pc_speaker_get_spk(void);
void pc_speaker_set_spkrdata(running_machine *machine, UINT8 data);
void pc_speaker_set_input(running_machine *machine, UINT8 data);

void mess_init_pc_common( running_machine *machine, UINT32 flags, void (*set_keyb_int_func)(running_machine *, int), void (*set_hdc_int_func)(int,int));

WRITE8_HANDLER( pc_nmi_enable_w );
READ8_HANDLER( pcjr_nmi_enable_r );

READ8_HANDLER( pc_page_r );
WRITE8_HANDLER( pc_page_w );

DRIVER_INIT( ibm5150 );
DRIVER_INIT( pccga );
DRIVER_INIT( pcmda );
DRIVER_INIT( europc );
DRIVER_INIT( bondwell );
DRIVER_INIT( pc200 );
DRIVER_INIT( pc1512 );
DRIVER_INIT( pc1640 );
DRIVER_INIT( pc_vga );
DRIVER_INIT( t1000hx );
DRIVER_INIT( pcjr );

MACHINE_START( pc );
MACHINE_RESET( pc );
MACHINE_RESET( pcjr );

DEVICE_IMAGE_LOAD( pcjr_cartridge );

INTERRUPT_GEN( pc_frame_interrupt );
INTERRUPT_GEN( pc_vga_frame_interrupt );
INTERRUPT_GEN( pcjr_frame_interrupt );

#endif /* PC_H_ */
