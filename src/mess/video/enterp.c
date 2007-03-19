/***************************************************************************

  Functions to emulate the video hardware of the Enterprise.

***************************************************************************/

#include "driver.h"
#include "video/nick.h"
#include "video/epnick.h"
#include "includes/enterp.h"

/***************************************************************************
  Start the video hardware emulation.
***************************************************************************/
VIDEO_START( enterprise )
{
	Nick_vh_start();
	return video_start_generic_bitmapped(machine);
}

/***************************************************************************
  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function,
  it will be called by the main emulation engine.
***************************************************************************/
VIDEO_UPDATE( enterprise )
{
	Nick_DoScreen(tmpbitmap);
	return video_update_generic_bitmapped(machine, screen, bitmap, cliprect);
}

