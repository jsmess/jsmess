/*****************************************************************************
 *
 * includes/abc80x.h
 *
 ****************************************************************************/

#ifndef __ABC80X__
#define __ABC80X__

#define ABC800_X01	XTAL_12MHz
#define ABC806_X02	XTAL_32_768kHz

/*----------- defined in video/abc80x.c -----------*/

MACHINE_DRIVER_EXTERN(abc800m_video);
MACHINE_DRIVER_EXTERN(abc800c_video);
MACHINE_DRIVER_EXTERN(abc802_video);
MACHINE_DRIVER_EXTERN(abc806_video);

void abc802_mux80_40_w(int level);

READ8_HANDLER( abc806_videoram_r );
WRITE8_HANDLER( abc806_videoram_w );
READ8_HANDLER( abc806_colorram_r );
WRITE8_HANDLER( abc806_colorram_w );
READ8_HANDLER( abc806_fgctlprom_r );
WRITE8_HANDLER( abc806_sync_w );

#endif /* __ABC80X__ */
