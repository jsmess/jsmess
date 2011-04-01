/*************************************************************************

    Memotech MTX 500, MTX 512 and RS 128

*************************************************************************/

#ifndef __MTX__
#define __MTX__

#include "imagedev/snapquik.h"

#define Z80_TAG			"z80"
#define Z80CTC_TAG		"z80ctc"
#define Z80DART_TAG		"z80dart"
#define FD1793_TAG		"fd1793" // SDX
#define FD1791_TAG		"fd1791" // FDX
#define SN76489A_TAG	"sn76489a"
#define MC6845_TAG		"mc6845"
#define SCREEN_TAG		"screen"
#define CASSETTE_TAG	"cassette"
#define CENTRONICS_TAG	"centronics"

class mtx_state : public driver_device
{
public:
	mtx_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* keyboard state */
	UINT8 m_key_sense;

	/* video state */
	UINT8 *m_video_ram;
	UINT8 *m_attr_ram;

	/* sound state */
	UINT8 m_sound_latch;

	/* devices */
	device_t *m_z80ctc;
	device_t *m_z80dart;
	device_t *m_cassette;
	device_t *m_centronics;

	/* timers */
	device_t *m_cassette_timer;
};

/*----------- defined in machine/mtx.c -----------*/

MACHINE_START( mtx512 );
MACHINE_RESET( mtx512 );
INTERRUPT_GEN( mtx_interrupt );
SNAPSHOT_LOAD( mtx );

WRITE8_HANDLER( mtx_bankswitch_w );

/* Sound */
READ8_DEVICE_HANDLER( mtx_sound_strobe_r );
WRITE8_HANDLER( mtx_sound_latch_w );

/* Cassette */
WRITE8_DEVICE_HANDLER( mtx_cst_w );

/* Printer */
READ8_DEVICE_HANDLER( mtx_strobe_r );
READ8_DEVICE_HANDLER( mtx_prt_r );

/* Keyboard */
WRITE8_HANDLER( mtx_sense_w );
READ8_HANDLER( mtx_key_lo_r );
READ8_HANDLER( mtx_key_hi_r );

/* HRX */
WRITE8_HANDLER( hrx_address_w );
READ8_HANDLER( hrx_data_r );
WRITE8_HANDLER( hrx_data_w );
READ8_HANDLER( hrx_attr_r );
WRITE8_HANDLER( hrx_attr_w );

#endif /* __MTX_H__ */
