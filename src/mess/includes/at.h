/*****************************************************************************
 *
 * includes/astrocde.h
 *
 * IBM AT Compatibles
 *
 ****************************************************************************/

#ifndef AT_H_
#define AT_H_

#include "machine/8237dma.h"

typedef struct _at_state at_state;
struct _at_state
{
	const device_config *maincpu;
	const device_config	*pic8259_master;
	const device_config	*pic8259_slave;
	const device_config	*dma8237_1;
	const device_config	*dma8237_2;
	const device_config	*pit8254;
};


/*----------- defined in machine/at.c -----------*/

extern const struct pic8259_interface at_pic8259_master_config;
extern const struct pic8259_interface at_pic8259_slave_config;
extern const struct pit8253_config at_pit8254_config;
extern const i8237_interface at_dma8237_1_config;
extern const i8237_interface at_dma8237_2_config;
extern const ins8250_interface ibm5170_com_interface[4];


READ8_HANDLER( at_page8_r );
WRITE8_HANDLER( at_page8_w );

MACHINE_DRIVER_EXTERN( at_kbdc8042 );

READ8_HANDLER(at_kbdc8042_r);
WRITE8_HANDLER(at_kbdc8042_w);
WRITE8_HANDLER( at_kbdc8042_set_clock_signal );
WRITE8_HANDLER( at_kbdc8042_set_data_signal );

DRIVER_INIT( atcga );
DRIVER_INIT( atega );
DRIVER_INIT( at386 );

DRIVER_INIT( at_vga );
DRIVER_INIT( ps2m30286 );

MACHINE_START( at );
MACHINE_RESET( at );
MACHINE_START( at586 );
MACHINE_RESET( at586 );

#endif /* AT_H_ */
