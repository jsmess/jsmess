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
#include "machine/isa.h"

class at_state : public driver_device
{
public:
	at_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

	device_t *m_maincpu;
	device_t *m_pic8259_master;
	device_t *m_pic8259_slave;
	device_t *m_dma8237_1;
	device_t *m_dma8237_2;
	device_t *m_pit8254;
	isa16_device *m_isabus;
};


/*----------- defined in machine/at.c -----------*/

extern const struct pic8259_interface at_pic8259_master_config;
extern const struct pic8259_interface at_pic8259_slave_config;
extern const struct pit8253_config at_pit8254_config;
extern const i8237_interface at_dma8237_1_config;
extern const i8237_interface at_dma8237_2_config;


READ8_HANDLER( at_page8_r );
WRITE8_HANDLER( at_page8_w );

READ8_HANDLER( at_portb_r );
WRITE8_HANDLER( at_portb_w );

DRIVER_INIT( atcga );
DRIVER_INIT( atega );
DRIVER_INIT( atvga );

MACHINE_START( at );
MACHINE_RESET( at );

#endif /* AT_H_ */
