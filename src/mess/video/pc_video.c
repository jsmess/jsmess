/*********************************************************************

	pc_video.c

	PC Video code

*********************************************************************/

#include "video/pc_video.h"
#include "video/generic.h"
#include "state.h"



/***************************************************************************

	Local variables

***************************************************************************/

static pc_video_update_proc (*pc_choosevideomode)(int *width, int *height, struct crtc6845 *crtc);
static struct crtc6845 *pc_crtc;
static int pc_anythingdirty;
static int pc_current_height;
static int pc_current_width;
static const UINT16 dummy_palette[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };



/**************************************************************************/

static void pc_video_postload(void)
{
	pc_anythingdirty = 1;
	pc_current_height = -1;
	pc_current_width = -1;
}



struct crtc6845 *pc_video_start(const struct crtc6845_config *config,
	pc_video_update_proc (*choosevideomode)(int *width, int *height, struct crtc6845 *crtc),
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
		pc_crtc = crtc6845_init(config);
		if (!pc_crtc)
			return NULL;
	}

	if (videoram_size)
	{
		if (video_start_generic(Machine))
			return NULL;
	}

	state_save_register_func_postload(pc_video_postload);
	return pc_crtc;
}



VIDEO_UPDATE( pc_video )
{
	UINT32 rc = 0;
	int w = 0, h = 0;
	pc_video_update_proc video_update;

	if (pc_crtc)
	{
		w = crtc6845_get_char_columns(pc_crtc);
		h = crtc6845_get_char_height(pc_crtc) * crtc6845_get_char_lines(pc_crtc);
	}

	video_update = pc_choosevideomode(&w, &h, pc_crtc);

	if (video_update)
	{
		if ((pc_current_width != w) || (pc_current_height != h)) 
		{
			pc_current_width = w;
			pc_current_height = h;
			pc_anythingdirty = 1;

			if (pc_current_width > Machine->screen[0].width)
				pc_current_width = Machine->screen[0].width;
			if (pc_current_height > Machine->screen[0].height)
				pc_current_height = Machine->screen[0].height;

			if ((pc_current_width > 100) && (pc_current_height > 100))
				video_screen_set_visarea(0, 0, pc_current_width-1, 0, pc_current_height-1);

			fillbitmap(bitmap, 0, cliprect);
		}

		video_update(tmpbitmap ? tmpbitmap : bitmap, pc_crtc);

		if (tmpbitmap)
		{
			copybitmap(bitmap, tmpbitmap, 0, 0, 0, 0, cliprect, TRANSPARENCY_NONE, 0);
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
		if (dirtybuffer)
			dirtybuffer[offset] = 1;
		pc_anythingdirty = 1;
	}
}


WRITE32_HANDLER( pc_video_videoram32_w )
{
	COMBINE_DATA(((UINT32 *) videoram) + offset);
	pc_anythingdirty = 1;
	if (dirtybuffer)
	{
		if ((mem_mask & 0x000000FF) == 0)
			dirtybuffer[offset * 4 + 0] = 1;
		if ((mem_mask & 0x0000FF00) == 0)
			dirtybuffer[offset * 4 + 1] = 1;
		if ((mem_mask & 0x00FF0000) == 0)
			dirtybuffer[offset * 4 + 2] = 1;
		if ((mem_mask & 0xFF000000) == 0)
			dirtybuffer[offset * 4 + 3] = 1;
	}
}



/*************************************
 *
 *	Graphics renderers
 *
 *************************************/

void pc_render_gfx_1bpp(mame_bitmap *bitmap, struct crtc6845 *crtc,
	const UINT8 *vram, const UINT16 *palette, int interlace)
{
	int sx, sy, sh;
	int	offs = crtc6845_get_start(crtc)*2;
	int lines = crtc6845_get_char_lines(crtc);
	int height = crtc6845_get_char_height(crtc);
	int columns = crtc6845_get_char_columns(crtc)*2;

	if (!vram)
		vram = videoram;
	if (!palette)
		palette = dummy_palette;

	for (sy = 0; sy < lines; sy++)
	{
		for (sh = 0; sh < height; sh++)
		{
			UINT16 *dest = BITMAP_ADDR16(bitmap, sy * height + sh, 0);
			const UINT8 *src = &vram[offs | ((sh % interlace) << 13)];

			for (sx = 0; sx < columns; sx++)
			{
				UINT8 b = *(src++);
				*(dest++) = palette[(b >> 7) & 0x01];
				*(dest++) = palette[(b >> 6) & 0x01];
				*(dest++) = palette[(b >> 5) & 0x01];
				*(dest++) = palette[(b >> 4) & 0x01];
				*(dest++) = palette[(b >> 3) & 0x01];
				*(dest++) = palette[(b >> 2) & 0x01];
				*(dest++) = palette[(b >> 1) & 0x01];
				*(dest++) = palette[(b >> 0) & 0x01];
			}
		}
		offs = (offs + columns) & 0x1fff;
	}
}



void pc_render_gfx_2bpp(mame_bitmap *bitmap, struct crtc6845 *crtc,
	const UINT8 *vram, const UINT16 *palette, int interlace)
{
	int sx, sy, sh;
	int	offs = crtc6845_get_start(crtc)*2;
	int lines = crtc6845_get_char_lines(crtc);
	int height = crtc6845_get_char_height(crtc);
	int columns = crtc6845_get_char_columns(crtc)*2;
	UINT16 *dest;
	const UINT8 *src;

	if (!vram)
		vram = videoram;
	if (!palette)
		palette = dummy_palette;

	for (sy = 0; sy < lines; sy++)
	{
		for (sh = 0; sh < height; sh++)
		{
			if (sy*height+sh >= bitmap->height)
				return;

			dest = BITMAP_ADDR16(bitmap, sy * height + sh, 0);
			src = &vram[offs | ((sh % interlace) << 13)];

			for (sx = 0; sx < columns; sx++)
			{
				UINT8 b = *(src++);
				*(dest++) = palette[(b >> 6) & 0x03];
				*(dest++) = palette[(b >> 4) & 0x03];
				*(dest++) = palette[(b >> 2) & 0x03];
				*(dest++) = palette[(b >> 0) & 0x03];
			}
		}
		offs = (offs + columns) & 0x1fff;
	}
}



void pc_render_gfx_4bpp(mame_bitmap *bitmap, struct crtc6845 *crtc,
	const UINT8 *vram, const UINT16 *palette, int interlace)
{
	int sx, sy, sh;
	int	offs = crtc6845_get_start(crtc)*2;
	int lines = crtc6845_get_char_lines(crtc);
	int height = crtc6845_get_char_height(crtc);
	int columns = crtc6845_get_char_columns(crtc)*2;

	if (!vram)
		vram = videoram;
	if (!palette)
		palette = dummy_palette;

	for (sy = 0; sy < lines; sy++)
	{
		for (sh = 0; sh < height; sh++)
		{
			UINT16 *dest = BITMAP_ADDR16(bitmap, sy * height + sh, 0);
			const UINT8 *src = &vram[offs | ((sh % interlace) << 13)];

			for (sx = 0; sx < columns; sx++)
			{
				UINT8 b = *(src++);
				*(dest++) = palette[(b >> 4) & 0x0F];
				*(dest++) = palette[(b >> 0) & 0x0F];
			}
		}
		offs = (offs + columns) & 0x1fff;
	}
}
