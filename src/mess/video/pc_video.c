/*********************************************************************

    pc_video.c

    PC Video code

*********************************************************************/

#include "driver.h"
#include "memconv.h"
#include "includes/crtc6845.h"
#include "video/pc_video.h"



/***************************************************************************

    Local variables

***************************************************************************/

static pc_video_update_proc (*pc_choosevideomode)(running_machine *machine, int *width, int *height, struct mscrtc6845 *crtc);
static struct mscrtc6845 *pc_crtc;
static int pc_anythingdirty;
static int pc_current_height;
static int pc_current_width;
static const UINT16 dummy_palette[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };



/**************************************************************************/

static STATE_POSTLOAD( pc_video_postload )
{
	pc_anythingdirty = 1;
	pc_current_height = -1;
	pc_current_width = -1;
}



struct mscrtc6845 *pc_video_start(running_machine *machine, const struct mscrtc6845_config *config,
	pc_video_update_proc (*choosevideomode)(running_machine *machine, int *width, int *height, struct mscrtc6845 *crtc),
	size_t vramsize)
{
	pc_choosevideomode = choosevideomode;
	pc_crtc = NULL;
	pc_anythingdirty = 1;
	pc_current_height = -1;
	pc_current_width = -1;
	tmpbitmap = NULL;

	videoram_size = vramsize;

	if (config)
	{
		pc_crtc = mscrtc6845_init(machine, config);
		if (!pc_crtc)
			return NULL;
	}

	if (videoram_size)
	{
		video_start_generic_bitmapped(machine);
	}

	state_save_register_postload(machine, pc_video_postload, NULL);
	return pc_crtc;
}



VIDEO_UPDATE( pc_video )
{
	UINT32 rc = 0;
	int w = 0, h = 0;
	pc_video_update_proc video_update;

	if (pc_crtc)
	{
		w = mscrtc6845_get_char_columns(pc_crtc);
		h = mscrtc6845_get_char_height(pc_crtc) * mscrtc6845_get_char_lines(pc_crtc);
	}

	video_update = pc_choosevideomode(screen->machine, &w, &h, pc_crtc);

	if (video_update)
	{
		if ((pc_current_width != w) || (pc_current_height != h))
		{
			int width = video_screen_get_width(screen);
			int height = video_screen_get_height(screen);

			pc_current_width = w;
			pc_current_height = h;
			pc_anythingdirty = 1;

			if (pc_current_width > width)
				pc_current_width = width;
			if (pc_current_height > height)
				pc_current_height = height;

			if ((pc_current_width > 100) && (pc_current_height > 100))
				video_screen_set_visarea(screen, 0, pc_current_width-1, 0, pc_current_height-1);

			bitmap_fill(bitmap, cliprect, 0);
		}

		video_update(tmpbitmap ? tmpbitmap : bitmap, pc_crtc);

		if (tmpbitmap)
		{
			copybitmap(bitmap, tmpbitmap, 0, 0, 0, 0, cliprect);
			if (!pc_anythingdirty)
				rc = UPDATE_HAS_NOT_CHANGED;
			pc_anythingdirty = 0;
		}
	}
	return rc;
}



WRITE8_HANDLER ( pc_video_videoram_w )
{
	if (videoram && videoram[offset] != data)
	{
		videoram[offset] = data;
		pc_anythingdirty = 1;
	}
}


WRITE16_HANDLER( pc_video_videoram16le_w ) { write16le_with_write8_handler(pc_video_videoram_w, space, offset, data, mem_mask); }

WRITE32_HANDLER( pc_video_videoram32_w )
{
	COMBINE_DATA(((UINT32 *) videoram) + offset);
	pc_anythingdirty = 1;
}
