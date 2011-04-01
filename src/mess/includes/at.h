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

class at_state : public driver_device
{
public:
	at_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	device_t *m_maincpu;
	device_t *m_pic8259_master;
	device_t *m_pic8259_slave;
	device_t *m_dma8237_1;
	device_t *m_dma8237_2;
	device_t *m_pit8254;
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

READ8_HANDLER( at_portb_r );
WRITE8_HANDLER( at_portb_w );

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
