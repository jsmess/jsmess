#include "driver.h"
#include "video/generic.h"

WRITE8_HANDLER( zx8301_control_w );

PALETTE_INIT( zx8301 );
VIDEO_START( zx8301 );
VIDEO_UPDATE( zx8301 );
