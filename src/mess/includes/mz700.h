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


/*----------- defined in machine/mz700.c -----------*/

DRIVER_INIT( mz700 );
MACHINE_RESET( mz700 );

READ8_HANDLER( mz700_mmio_r );
WRITE8_HANDLER( mz700_mmio_w );
WRITE8_HANDLER( mz700_bank_w );

READ8_HANDLER( mz800_crtc_r );
READ8_HANDLER( mz800_mmio_r );
READ8_HANDLER( mz800_bank_r );
READ8_HANDLER( mz800_ramdisk_r );

WRITE8_HANDLER( mz800_write_format_w );
WRITE8_HANDLER( mz800_read_format_w );
WRITE8_HANDLER( mz800_display_mode_w );
WRITE8_HANDLER( mz800_scroll_border_w );
WRITE8_HANDLER( mz800_mmio_w );
WRITE8_HANDLER( mz800_bank_w );
WRITE8_HANDLER( mz800_ramdisk_w );
WRITE8_HANDLER( mz800_ramaddr_w );
WRITE8_HANDLER( mz800_palette_w );

WRITE8_HANDLER( pcgram_w );

DRIVER_INIT( mz800 );


/*----------- defined in video/mz700.c -----------*/

PALETTE_INIT( mz700 );
VIDEO_UPDATE( mz700 );


#endif /* MZ700_H_ */
