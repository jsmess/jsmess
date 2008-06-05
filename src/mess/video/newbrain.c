#include "driver.h"
#include "includes/newbrain.h"

static VIDEO_START( newbrain )
{
//	newbrain_state *state = machine->driver_data;

	/* register for state saving */

//	state_save_register_global(state->);
}

static VIDEO_UPDATE( newbrain )
{
	return 0;
}

/* Machine Drivers */

MACHINE_DRIVER_START( newbrain_video )
	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_SIZE(640, 400)
	MDRV_SCREEN_VISIBLE_AREA(0, 639, 0, 399)

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(newbrain)
	MDRV_VIDEO_UPDATE(newbrain)
MACHINE_DRIVER_END
