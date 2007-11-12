#include "driver.h"
#include "video/generic.h"

PALETTE_INIT( ql );
WRITE8_HANDLER( ql_videoram_w );
WRITE8_HANDLER( ql_video_ctrl_w );
