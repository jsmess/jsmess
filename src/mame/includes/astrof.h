#include "sound/samples.h"

/*----------- defined in video/astrof.c -----------*/

extern unsigned char *astrof_color;
extern unsigned char *tomahawk_protection;

PALETTE_INIT( astrof );
VIDEO_START( astrof );
VIDEO_UPDATE( astrof );
WRITE8_HANDLER( astrof_videoram_w );
WRITE8_HANDLER( tomahawk_videoram_w );
WRITE8_HANDLER( astrof_video_control1_w );
WRITE8_HANDLER( astrof_video_control2_w );
WRITE8_HANDLER( tomahawk_video_control2_w );
READ8_HANDLER( tomahawk_protection_r );


/*----------- defined in audio/astrof.c -----------*/

WRITE8_HANDLER( astrof_sample1_w );
WRITE8_HANDLER( astrof_sample2_w );

extern struct Samplesinterface astrof_samples_interface;
extern struct Samplesinterface tomahawk_samples_interface;
