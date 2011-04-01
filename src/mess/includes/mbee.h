/*****************************************************************************
 *
 * includes/mbee.h
 *
 ****************************************************************************/

#ifndef MBEE_H_
#define MBEE_H_

#include "machine/wd17xx.h"
#include "imagedev/snapquik.h"
#include "machine/z80pio.h"
#include "imagedev/cassette.h"
#include "machine/ctronics.h"
#include "machine/mc146818.h"
#include "video/mc6845.h"
#include "sound/speaker.h"


class mbee_state : public driver_device
{
public:
	mbee_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 m_dummy;
	size_t m_size;
	UINT8 m_clock_pulse;
	UINT8 m_mbee256_key_available;
	UINT8 m_fdc_intrq;
	UINT8 m_fdc_drq;
	UINT8 m_0a;
	device_t *m_fdc;
	mc146818_device *m_rtc;
	device_t *m_z80pio;
	device_t *m_speaker;
	device_t *m_cassette;
	device_t *m_printer;
	UINT8 m_mbee256_was_pressed[15];
	UINT8 m_mbee256_q[20];
	UINT8 m_mbee256_q_pos;
	UINT8 m_framecnt;
	UINT8 *m_gfxram;
	UINT8 *m_colorram;
	UINT8 *m_videoram;
	UINT8 *m_attribram;
	UINT8 m_08;
	UINT8 m_0b;
	UINT8 m_1c;
	UINT8 m_is_premium;
	UINT8 m_sy6545_cursor[16];
	UINT8 m_sy6545_status;
	device_t *m_mc6845;
	UINT8 m_speed;
	UINT8 m_flash;
	UINT16 m_cursor;
	UINT8 m_sy6545_reg[32];
	UINT8 m_sy6545_ind;
};


/*----------- defined in machine/mbee.c -----------*/

extern const wd17xx_interface mbee_wd17xx_interface;
extern const z80pio_interface mbee_z80pio_intf;

DRIVER_INIT( mbee );
DRIVER_INIT( mbeeic );
DRIVER_INIT( mbeepc );
DRIVER_INIT( mbeepc85 );
DRIVER_INIT( mbeeppc );
DRIVER_INIT( mbee56 );
DRIVER_INIT( mbee64 );
DRIVER_INIT( mbee128 );
DRIVER_INIT( mbee256 );
DRIVER_INIT( mbeett );
MACHINE_RESET( mbee );
MACHINE_RESET( mbee56 );
MACHINE_RESET( mbee64 );
MACHINE_RESET( mbee128 );
MACHINE_RESET( mbee256 );
MACHINE_RESET( mbeett );
WRITE8_HANDLER( mbee_04_w );
WRITE8_HANDLER( mbee_06_w );
READ8_HANDLER( mbee_07_r );
READ8_HANDLER( mbeeic_0a_r );
WRITE8_HANDLER( mbeeic_0a_w );
READ8_HANDLER( mbeepc_telcom_low_r );
READ8_HANDLER( mbeepc_telcom_high_r );
READ8_HANDLER( mbee256_speed_low_r );
READ8_HANDLER( mbee256_speed_high_r );
READ8_HANDLER( mbee256_18_r );
WRITE8_HANDLER( mbee64_50_w );
WRITE8_HANDLER( mbee128_50_w );
WRITE8_HANDLER( mbee256_50_w );

READ8_HANDLER ( mbee_fdc_status_r );
WRITE8_HANDLER ( mbee_fdc_motor_w );
INTERRUPT_GEN( mbee_interrupt );
Z80BIN_EXECUTE( mbee );
QUICKLOAD_LOAD( mbee );


/*----------- defined in video/mbee.c -----------*/

READ8_HANDLER( m6545_status_r );
WRITE8_HANDLER( m6545_index_w );
READ8_HANDLER( m6545_data_r );
WRITE8_HANDLER( m6545_data_w );
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
READ8_HANDLER( mbeeppc_1c_r );
WRITE8_HANDLER( mbeeppc_1c_w );
WRITE8_HANDLER( mbee256_1c_w );
READ8_HANDLER( mbeeppc_low_r );
READ8_HANDLER( mbeeppc_high_r );
WRITE8_HANDLER( mbeeppc_high_w );
WRITE8_HANDLER( mbeeppc_low_w );
MC6845_UPDATE_ROW( mbee_update_row );
MC6845_UPDATE_ROW( mbeeic_update_row );
MC6845_UPDATE_ROW( mbeeppc_update_row );
MC6845_ON_UPDATE_ADDR_CHANGED( mbee_update_addr );
MC6845_ON_UPDATE_ADDR_CHANGED( mbee256_update_addr );

VIDEO_START( mbee );
SCREEN_UPDATE( mbee );
VIDEO_START( mbeeic );
SCREEN_UPDATE( mbeeic );
VIDEO_START( mbeeppc );
SCREEN_UPDATE( mbeeppc );
PALETTE_INIT( mbeeic );
PALETTE_INIT( mbeepc85b );
PALETTE_INIT( mbeeppc );

#endif /* MBEE_H_ */
