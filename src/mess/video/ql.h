#include "driver.h"
#include "video/generic.h"

#define ZX8301_X1 15000000.0

PALETTE_INIT( ql );
WRITE8_HANDLER( ql_videoram_w );
WRITE8_HANDLER( ql_video_ctrl_w );
