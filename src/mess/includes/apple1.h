#include "devices/snapquik.h"

/*----------- defined in machine/apple1.c -----------*/

DRIVER_INIT( apple1 );
MACHINE_RESET( apple1 );
SNAPSHOT_LOAD( apple1 );

READ8_HANDLER( apple1_cassette_r );
WRITE8_HANDLER( apple1_cassette_w );

/*----------- defined in video/apple1.c -----------*/

VIDEO_START( apple1 );
VIDEO_UPDATE( apple1 );

void apple1_vh_dsp_w (int data);
void apple1_vh_dsp_clr (void);
attotime apple1_vh_dsp_time_to_ready (void);

extern int apple1_vh_clrscrn_pressed;

/*----------- defined in drivers/apple1.c -----------*/

extern const gfx_layout apple1_charlayout;
