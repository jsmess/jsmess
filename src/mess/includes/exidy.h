#include "driver.h"
#include "osdepend.h"
#define EXIDY_NUM_COLOURS 2

/* 64 chars wide, 30 chars tall */
#define EXIDY_SCREEN_WIDTH        (64*8)
#define EXIDY_SCREEN_HEIGHT       (30*8)

VIDEO_START( exidy );
VIDEO_UPDATE( exidy );
PALETTE_INIT( exidy );

DEVICE_LOAD( exidy_cassette );
