/*****************************************************************************
 *
 * includes/ti85.h
 *
 ****************************************************************************/

#ifndef TI85_H_
#define TI85_H_

#include "devices/snapquik.h"


/*----------- defined in machine/ti85.c -----------*/

extern UINT8 ti85_LCD_memory_base;
extern UINT8 ti85_LCD_contrast;
extern UINT8 ti85_LCD_status;
extern UINT8 ti85_timer_interrupt_mask;
extern UINT8 ti82_video_buffer[0x300];

MACHINE_START( ti81 );
MACHINE_START( ti82 );
MACHINE_START( ti83 );
MACHINE_START( ti85 );
MACHINE_START( ti86 );

NVRAM_HANDLER( ti86 );

SNAPSHOT_LOAD( ti8x );

DEVICE_START( ti85_serial );
DEVICE_IMAGE_LOAD( ti85_serial );
DEVICE_IMAGE_UNLOAD( ti85_serial );

WRITE8_HANDLER( ti81_port_0007_w);
 READ8_HANDLER( ti85_port_0000_r);
 READ8_HANDLER( ti85_port_0001_r);
 READ8_HANDLER( ti85_port_0002_r);
 READ8_HANDLER( ti85_port_0003_r);
 READ8_HANDLER( ti85_port_0004_r);
 READ8_HANDLER( ti85_port_0005_r);
 READ8_HANDLER( ti85_port_0006_r);
 READ8_HANDLER( ti85_port_0007_r);
 READ8_HANDLER( ti86_port_0005_r);
 READ8_HANDLER( ti86_port_0006_r);
 READ8_HANDLER( ti82_port_0002_r);
 READ8_HANDLER( ti82_port_0010_r);
 READ8_HANDLER( ti82_port_0011_r);
 READ8_HANDLER( ti83_port_0000_r);
 READ8_HANDLER( ti83_port_0002_r);
 READ8_HANDLER( ti83_port_0003_r);
WRITE8_HANDLER( ti85_port_0000_w);
WRITE8_HANDLER( ti85_port_0001_w);
WRITE8_HANDLER( ti85_port_0002_w);
WRITE8_HANDLER( ti85_port_0003_w);
WRITE8_HANDLER( ti85_port_0004_w);
WRITE8_HANDLER( ti85_port_0005_w);
WRITE8_HANDLER( ti85_port_0006_w);
WRITE8_HANDLER( ti85_port_0007_w);
WRITE8_HANDLER( ti86_port_0005_w);
WRITE8_HANDLER( ti86_port_0006_w);
WRITE8_HANDLER( ti82_port_0002_w);
WRITE8_HANDLER( ti82_port_0010_w);
WRITE8_HANDLER( ti82_port_0011_w);
WRITE8_HANDLER( ti83_port_0000_w);
WRITE8_HANDLER( ti83_port_0002_w);
WRITE8_HANDLER( ti83_port_0003_w);

/*----------- defined in video/ti85.c -----------*/

VIDEO_START( ti85 );
VIDEO_UPDATE( ti85 );
VIDEO_UPDATE( ti82 );
PALETTE_INIT( ti85 );


#endif /* TI85_H_ */
