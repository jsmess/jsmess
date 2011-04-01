/*****************************************************************************
 *
 * includes/trs80.h
 *
 ****************************************************************************/

#ifndef TRS80_H_
#define TRS80_H_

#include "imagedev/snapquik.h"
#include "machine/wd17xx.h"

#define FW 6
#define FH 12


class trs80_state : public driver_device
{
public:
	trs80_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *m_videoram;
	UINT8 m_model4;
	UINT8 *m_gfxram;
	UINT8 m_mode;
	UINT8 m_irq;
	UINT8 m_mask;
	UINT8 m_nmi_mask;
	UINT8 m_port_ec;
	UINT8 m_tape_unit;
	UINT8 m_reg_load;
	UINT8 m_nmi_data;
#ifdef USE_TRACK
	UINT8 m_track[4];
#endif
	UINT8 m_head;
#ifdef USE_SECTOR
	UINT8 m_sector[4];
#endif
	UINT8 m_cassette_data;
	emu_timer *m_cassette_data_timer;
	device_t *m_printer;
	device_t *m_ay31015;
	device_t *m_cass;
	device_t *m_speaker;
	device_t *m_fdc;
	double m_old_cassette_val;
	UINT16 m_start_address;
	UINT8 m_crtc_reg;
	UINT8 m_size_store;
};


/*----------- defined in machine/trs80.c -----------*/

extern const wd17xx_interface trs80_wd17xx_interface;

MACHINE_START( trs80 );
MACHINE_RESET( trs80 );
MACHINE_RESET( trs80m4 );
MACHINE_RESET( lnw80 );

WRITE8_HANDLER ( trs80_ff_w );
WRITE8_HANDLER ( lnw80_fe_w );
WRITE8_HANDLER ( sys80_fe_w );
WRITE8_HANDLER ( sys80_f8_w );
WRITE8_HANDLER ( trs80m4_ff_w );
WRITE8_HANDLER ( trs80m4_f4_w );
WRITE8_HANDLER ( trs80m4_ec_w );
WRITE8_HANDLER ( trs80m4_eb_w );
WRITE8_HANDLER ( trs80m4_ea_w );
WRITE8_HANDLER ( trs80m4_e9_w );
WRITE8_HANDLER ( trs80m4_e8_w );
WRITE8_HANDLER ( trs80m4_e4_w );
WRITE8_HANDLER ( trs80m4_e0_w );
WRITE8_HANDLER ( trs80m4p_9c_w );
WRITE8_HANDLER ( trs80m4_90_w );
WRITE8_HANDLER ( trs80m4_84_w );
READ8_HANDLER ( lnw80_fe_r );
READ8_HANDLER ( trs80_ff_r );
READ8_HANDLER ( sys80_f9_r );
READ8_HANDLER ( trs80m4_ff_r );
READ8_HANDLER ( trs80m4_ec_r );
READ8_HANDLER ( trs80m4_eb_r );
READ8_HANDLER ( trs80m4_ea_r );
READ8_HANDLER ( trs80m4_e8_r );
READ8_HANDLER ( trs80m4_e4_r );
READ8_HANDLER ( trs80m4_e0_r );

INTERRUPT_GEN( trs80_rtc_interrupt );
INTERRUPT_GEN( trs80_fdc_interrupt );

READ8_HANDLER( trs80_irq_status_r );

READ8_HANDLER( trs80_printer_r );
WRITE8_HANDLER( trs80_printer_w );

WRITE8_HANDLER( trs80_cassunit_w );
WRITE8_HANDLER( trs80_motor_w );

READ8_HANDLER( trs80_keyboard_r );
READ8_DEVICE_HANDLER (trs80_wd179x_r);


/*----------- defined in video/trs80.c -----------*/

VIDEO_START( trs80 );
SCREEN_UPDATE( trs80 );
SCREEN_UPDATE( ht1080z );
SCREEN_UPDATE( lnw80 );
SCREEN_UPDATE( radionic );
SCREEN_UPDATE( trs80m4 );

WRITE8_HANDLER ( trs80m4_88_w );

READ8_HANDLER( trs80_videoram_r );
WRITE8_HANDLER( trs80_videoram_w );
READ8_HANDLER( trs80_gfxram_r );
WRITE8_HANDLER( trs80_gfxram_w );

PALETTE_INIT( lnw80 );


#endif	/* TRS80_H_ */
