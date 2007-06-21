#ifndef __ABC80X_VIDEO__
#define __ABC80X_VIDEO__

#include "driver.h"
#include "video/generic.h"
#include "video/crtc6845.h"

#define ABC800_X01 12000000.0

WRITE8_HANDLER( abc800_videoram_w );
PALETTE_INIT( abc800m );
PALETTE_INIT( abc800c );
VIDEO_START( abc800m );
VIDEO_START( abc800c );
VIDEO_UPDATE( abc800 );
VIDEO_START( abc802 );
VIDEO_UPDATE( abc802 );

void abc802_set_columns(int columns);

#endif
