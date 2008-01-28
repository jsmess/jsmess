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

int nascom1_read_cassette (void);
int nascom1_init_cartridge (int id, mame_file *fp);
 READ8_HANDLER( nascom1_port_00_r);
 READ8_HANDLER( nascom1_port_01_r);
 READ8_HANDLER( nascom1_port_02_r);
WRITE8_HANDLER( nascom1_port_00_w);
WRITE8_HANDLER( nascom1_port_01_w);


/*----------- defined in video/nascom1.c -----------*/

VIDEO_UPDATE( nascom1 );
VIDEO_UPDATE( nascom2 );


#endif /* NASCOM1_H_ */
