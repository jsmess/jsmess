#include "sound/custom.h"
#include "streams.h"

/* machine/odyssey2.c */
extern int odyssey2_framestart;
extern int odyssey2_videobank;

DRIVER_INIT( odyssey2 );
MACHINE_RESET( odyssey2 );


/* vidhrdw/odyssey2.c */
extern int odyssey2_vh_hpos;

extern UINT8 odyssey2_colors[];

VIDEO_START( odyssey2 );
VIDEO_UPDATE( odyssey2 );
PALETTE_INIT( odyssey2 );
void odyssey2_vh_write(int data);
void odyssey2_vh_update(int data);
READ8_HANDLER ( odyssey2_t1_r );
INTERRUPT_GEN( odyssey2_line );
READ8_HANDLER ( odyssey2_video_r );
WRITE8_HANDLER ( odyssey2_video_w );

/* sndhrdw/odyssey2.c */
extern sound_stream *odyssey2_sh_channel;
extern struct CustomSound_interface odyssey2_sound_interface;
void *odyssey2_sh_start(int clock, const struct CustomSound_interface *config);
void odyssey2_sh_update( void *param,stream_sample_t **inputs, stream_sample_t **_buffer,int length );

/* i/o ports */
READ8_HANDLER ( odyssey2_bus_r );
WRITE8_HANDLER ( odyssey2_bus_w );

READ8_HANDLER( odyssey2_getp1 );
WRITE8_HANDLER ( odyssey2_putp1 );

READ8_HANDLER( odyssey2_getp2 );
WRITE8_HANDLER ( odyssey2_putp2 );

READ8_HANDLER( odyssey2_getbus );
WRITE8_HANDLER ( odyssey2_putbus );
