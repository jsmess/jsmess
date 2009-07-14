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


/*----------- defined in machine/dai.c -----------*/

extern const struct pit8253_config dai_pit8253_intf;
extern const i8255a_interface dai_ppi82555_intf;
extern const tms5501_interface dai_tms5501_interface;

MACHINE_START( dai );
MACHINE_RESET( dai );
READ8_HANDLER( dai_io_discrete_devices_r );
WRITE8_HANDLER( dai_io_discrete_devices_w );
WRITE8_HANDLER( dai_stack_interrupt_circuit_w );
READ8_HANDLER( amd9511_r );
WRITE8_HANDLER( amd9511_w );
extern UINT8 dai_noise_volume;
extern UINT8 dai_osc_volume[3];


/*----------- defined in video/dai.c -----------*/

extern const unsigned char dai_palette[16*3];

VIDEO_START( dai );
VIDEO_UPDATE( dai );
PALETTE_INIT( dai );


/*----------- defined in audio/dai.c -----------*/

#define SOUND_DAI	DEVICE_GET_INFO_NAME( dai_sound )

DEVICE_GET_INFO( dai_sound );
void dai_set_input(running_machine *machine, int index, int state);


#endif /* DAI_H_ */
