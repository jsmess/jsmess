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
	device_t *m_speaker;
	isa16_device *m_isabus;

	int m_poll_delay;
	UINT8 m_at_spkrdata;
	UINT8 m_at_speaker_input;

	int m_dma_channel;
	UINT8 m_dma_offset[2][4];
	UINT8 m_at_pages[0x10];
	UINT16 m_dma_high_byte;
	UINT8 m_at_speaker;
	UINT8 m_at_offset1;
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
