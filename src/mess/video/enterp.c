/***************************************************************************

  Functions to emulate the video hardware of the Enterprise.

***************************************************************************/

#include "driver.h"
#include "video/epnick.h"
#include "includes/enterp.h"

/***************************************************************************
  Start the video hardware emulation.
***************************************************************************/
VIDEO_START( enterprise )
{
	Nick_vh_start();
	VIDEO_START_CALL(generic_bitmapped);
}

/***************************************************************************
  Draw the game screen in the given bitmap_t.
  Do NOT call osd_update_display() from this function,
  it will be called by the main emulation engine.
***************************************************************************/
VIDEO_UPDATE( enterprise )
{
	Nick_DoScreen(machine, tmpbitmap);
	return VIDEO_UPDATE_CALL(generic_bitmapped);
}

