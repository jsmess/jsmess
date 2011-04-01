/******************************************************************************
 *  Sharp MZ700
 *
 *  variables and function prototypes
 *
 *  Juergen Buchmueller <pullmoll@t-online.de>, Jul 2000
 *
 *  Reference: http://sharpmz.computingmuseum.com
 *
 ******************************************************************************/

#ifndef MZ700_H_
#define MZ700_H_

#include "machine/i8255a.h"
#include "machine/pit8253.h"
#include "machine/z80pio.h"


class mz_state : public driver_device
{
public:
	mz_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	int m_mz700;				/* 1 if running on an mz700 */

	device_t *m_pit;
	device_t *m_ppi;

	int m_cursor_timer;
	int m_other_timer;

	int m_intmsk;	/* PPI8255 pin PC2 */

	int m_mz700_ram_lock;		/* 1 if ram lock is active */
	int m_mz700_ram_vram;		/* 1 if vram is banked in */

	/* mz800 specific */
	UINT8 *m_cgram;

	int m_mz700_mode;			/* 1 if in mz700 mode */
	int m_mz800_ram_lock;		/* 1 if lock is active */
	int m_mz800_ram_monitor;	/* 1 if monitor rom banked in */

	int m_hires_mode;			/* 1 if in 640x200 mode */
	int m_screen; 			/* screen designation */
	UINT8 *m_colorram;
	UINT8 *m_videoram;
	UINT8 m_speaker_level;
	UINT8 m_prev_state;
	UINT16 m_mz800_ramaddr;
	UINT8 m_mz800_palette[4];
	UINT8 m_mz800_palette_bank;
};


/*----------- defined in machine/mz700.c -----------*/

extern const struct pit8253_config mz700_pit8253_config;
extern const struct pit8253_config mz800_pit8253_config;
extern const i8255a_interface mz700_ppi8255_interface;
extern const z80pio_interface mz800_z80pio_config;

DRIVER_INIT( mz700 );
DRIVER_INIT( mz800 );
MACHINE_START( mz700 );

/* bank switching */
WRITE8_HANDLER( mz700_bank_0_w );
WRITE8_HANDLER( mz700_bank_1_w );
WRITE8_HANDLER( mz700_bank_2_w );
WRITE8_HANDLER( mz700_bank_3_w );
WRITE8_HANDLER( mz700_bank_4_w );
WRITE8_HANDLER( mz700_bank_5_w );
WRITE8_HANDLER( mz700_bank_6_w );

/* bankswitching, mz800 only */
READ8_HANDLER( mz800_bank_0_r );
READ8_HANDLER( mz800_bank_1_r );
WRITE8_HANDLER( mz800_bank_0_w );


READ8_HANDLER( mz800_crtc_r );
READ8_HANDLER( mz800_ramdisk_r );

WRITE8_HANDLER( mz800_write_format_w );
WRITE8_HANDLER( mz800_read_format_w );
WRITE8_HANDLER( mz800_display_mode_w );
WRITE8_HANDLER( mz800_scroll_border_w );
WRITE8_HANDLER( mz800_ramdisk_w );
WRITE8_HANDLER( mz800_ramaddr_w );
WRITE8_HANDLER( mz800_palette_w );


/*----------- defined in video/mz700.c -----------*/

PALETTE_INIT( mz700 );
SCREEN_UPDATE( mz700 );
VIDEO_START( mz800 );
SCREEN_UPDATE( mz800 );
WRITE8_HANDLER( mz800_cgram_w );


#endif /* MZ700_H_ */
