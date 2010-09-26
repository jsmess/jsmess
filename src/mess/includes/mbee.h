/*****************************************************************************
 *
 * includes/mbee.h
 *
 ****************************************************************************/

#ifndef MBEE_H_
#define MBEE_H_

#include "machine/wd17xx.h"
#include "devices/snapquik.h"
#include "devices/z80bin.h"
#include "machine/z80pio.h"
#include "devices/cassette.h"
#include "machine/ctronics.h"
#include "sound/speaker.h"


class mbee_state : public driver_device
{
public:
	mbee_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 dummy;
};


/*----------- defined in drivers/mbee.c -----------*/

extern size_t mbee_size;

/*----------- defined in machine/mbee.c -----------*/

extern const wd17xx_interface mbee_wd17xx_interface;
extern const z80pio_interface mbee_z80pio_intf;

MACHINE_RESET( mbee );

READ8_HANDLER ( mbee_fdc_status_r );
WRITE8_HANDLER ( mbee_fdc_motor_w );
INTERRUPT_GEN( mbee_interrupt );
Z80BIN_EXECUTE( mbee );
QUICKLOAD_LOAD( mbee );


/*----------- defined in video/mbee.c -----------*/

extern UINT8 *mbee_pcgram;

READ8_HANDLER ( m6545_status_r );
WRITE8_HANDLER ( m6545_index_w );
READ8_HANDLER ( m6545_data_r );
WRITE8_HANDLER ( m6545_data_w );
READ8_HANDLER( mbee_low_r );
READ8_HANDLER( mbee_high_r );
READ8_HANDLER( mbeeic_high_r );
WRITE8_HANDLER( mbeeic_high_w );
WRITE8_HANDLER( mbee_low_w );
WRITE8_HANDLER( mbee_high_w );
READ8_HANDLER( mbeeic_08_r );
WRITE8_HANDLER( mbeeic_08_w );
READ8_HANDLER( mbee_0b_r );
WRITE8_HANDLER( mbee_0b_w );
READ8_HANDLER( mbeeic_0a_r );
WRITE8_HANDLER( mbeeic_0a_w );
READ8_HANDLER ( mbee_color_bank_r );
WRITE8_HANDLER ( mbee_color_bank_w );
READ8_HANDLER ( mbee_video_bank_r );
WRITE8_HANDLER ( mbee_video_bank_w );
READ8_HANDLER ( mbee_netrom_bank_r );
READ8_HANDLER ( mbee_bank_netrom_r );
READ8_HANDLER ( mbee_pcg_color_latch_r );
WRITE8_HANDLER ( mbee_pcg_color_latch_w );
WRITE8_HANDLER ( mbee_0a_w );
WRITE8_HANDLER ( mbee_1c_w );

WRITE8_HANDLER ( mbee_videoram_w );
WRITE8_HANDLER ( mbee_pcg_color_w );
WRITE8_HANDLER ( mbee_pcg_w );

VIDEO_START( mbee );
VIDEO_UPDATE( mbee );

VIDEO_START( mbeeic );
VIDEO_UPDATE( mbeeic );
PALETTE_INIT( mbeeic );

#endif /* MBEE_H_ */
