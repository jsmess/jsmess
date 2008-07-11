/*****************************************************************************
 *
 * includes/astrocde.h
 *
 * IBM AT Compatibles
 *
 ****************************************************************************/

#ifndef AT_H_
#define AT_H_


/*----------- defined in machine/at.c -----------*/

extern const struct pic8259_interface at_pic8259_master_config;
extern const struct pic8259_interface at_pic8259_slave_config;
extern const struct pit8253_config at_pit8254_config;
extern const struct dma8237_interface at_dma8237_1_config;
extern const struct dma8237_interface at_dma8237_2_config;
extern const ins8250_interface ibm5170_com_interface[4];


READ8_HANDLER( at_page8_r );
WRITE8_HANDLER( at_page8_w );

MACHINE_DRIVER_EXTERN( at_kbdc8042 );
MACHINE_DRIVER_EXTERN( keytronic_keyboard );

READ8_HANDLER(at_kbdc8042_r);
WRITE8_HANDLER(at_kbdc8042_w);

DRIVER_INIT( atcga );
DRIVER_INIT( atega );
DRIVER_INIT( at386 );
DRIVER_INIT( at586 );

DRIVER_INIT( at_vga );
DRIVER_INIT( ps2m30286 );

MACHINE_START( at );
MACHINE_RESET( at );

#endif /* AT_H_ */
