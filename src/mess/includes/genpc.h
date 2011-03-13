/*****************************************************************************
 *
 * includes/pc.h
 *
 ****************************************************************************/

#ifndef GENPC_H_
#define GENPC_H_

#include "machine/ins8250.h"
#include "machine/i8255a.h"
#include "machine/8237dma.h"
#include "machine/isa.h"

class genpc_state : public driver_device
{
public:
	genpc_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	device_t *maincpu;
	device_t *pic8259;
	device_t *dma8237;
	device_t *pit8253;
	isa8_device *isabus;
	/* U73 is an LS74 - dual flip flop */
	/* Q2 is set by OUT1 from the 8253 and goes to DRQ1 on the 8237 */
	UINT8	u73_q2;
	UINT8	out1;
	int dma_channel;
	UINT8 dma_offset[4];
	UINT8 pc_spkrdata;
	UINT8 pc_input;

	int						ppi_portc_switch_high;
	int						ppi_speaker;
	int						ppi_keyboard_clear;
	UINT8					ppi_keyb_clock;
	UINT8					ppi_portb;
	UINT8					ppi_clock_signal;
	UINT8					ppi_data_signal;
	UINT8					ppi_shift_register;
	UINT8					ppi_shift_enable;
};

/*----------- defined in machine/genpc.c -----------*/

extern const i8237_interface genpc_dma8237_config;
extern const struct pit8253_config genpc_pit8253_config;
extern const struct pic8259_interface genpc_pic8259_config;
extern const i8255a_interface genpc_ppi8255_interface;

READ8_HANDLER( genpc_page_r );
WRITE8_HANDLER( genpc_page_w );

WRITE8_HANDLER( genpc_kb_set_clock_signal );
WRITE8_HANDLER( genpc_kb_set_data_signal );

DRIVER_INIT( genpc );
DRIVER_INIT( genpcvga );

MACHINE_START( genpc );
MACHINE_RESET( genpc );

#endif /* GENPC_H_ */
