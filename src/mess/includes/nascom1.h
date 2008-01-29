/*****************************************************************************
 *
 * includes/nascom1.h
 *
 ****************************************************************************/

#ifndef NASCOM1_H_
#define NASCOM1_H_

#include "devices/snapquik.h"


/*----------- defined in machine/nascom1.c -----------*/

DRIVER_INIT( nascom1 );
DEVICE_LOAD( nascom1_cassette );
DEVICE_UNLOAD( nascom1_cassette );
SNAPSHOT_LOAD( nascom1 );

READ8_HANDLER( nascom2_fdc_select_r );
WRITE8_HANDLER( nascom2_fdc_select_w );
READ8_HANDLER( nascom2_fdc_status_r );
DEVICE_LOAD( nascom2_floppy );

READ8_HANDLER( nascom1_port_00_r);
READ8_HANDLER( nascom1_port_01_r);
READ8_HANDLER( nascom1_port_02_r);
WRITE8_HANDLER( nascom1_port_00_w);
WRITE8_HANDLER( nascom1_port_01_w);


/*----------- defined in video/nascom1.c -----------*/

VIDEO_UPDATE( nascom1 );
VIDEO_UPDATE( nascom2 );


#endif /* NASCOM1_H_ */
