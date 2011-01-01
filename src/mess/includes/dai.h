/*****************************************************************************
 *
 * includes/dai.h
 *
 ****************************************************************************/

#ifndef DAI_H_
#define DAI_H_

#include "machine/i8255a.h"
#include "machine/tms5501.h"

#define DAI_DEBUG	1


class dai_state : public driver_device
{
public:
	dai_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 paddle_select;
	UINT8 paddle_enable;
	UINT8 cassette_motor[2];
	device_t *sound;
	device_t *tms5501;
	UINT8 keyboard_scan_mask;
	unsigned short _4_colours_palette[4];
};


/*----------- defined in machine/dai.c -----------*/

extern const struct pit8253_config dai_pit8253_intf;
extern const i8255a_interface dai_ppi82555_intf;
extern const tms5501_interface dai_tms5501_interface;

MACHINE_START( dai );
MACHINE_RESET( dai );
READ8_HANDLER( dai_io_discrete_devices_r );
WRITE8_HANDLER( dai_io_discrete_devices_w );
WRITE8_HANDLER( dai_stack_interrupt_circuit_w );
READ8_HANDLER( dai_amd9511_r );
WRITE8_HANDLER( dai_amd9511_w );


/*----------- defined in video/dai.c -----------*/

extern const unsigned char dai_palette[16*3];

VIDEO_START( dai );
VIDEO_UPDATE( dai );
PALETTE_INIT( dai );


/*----------- defined in audio/dai.c -----------*/

DECLARE_LEGACY_SOUND_DEVICE(DAI, dai_sound);

void dai_set_input(device_t *device, int index, int state);
void dai_set_volume(device_t *device, int offset, UINT8 data);


#endif /* DAI_H_ */
