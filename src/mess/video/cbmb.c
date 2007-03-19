/***************************************************************************

  PeT mess@utanet.at

***************************************************************************/

#include "driver.h"

#include "video/generic.h"
#include "includes/crtc6845.h"
#include "mscommon.h"
#include "includes/cbmb.h"

static int cbmb_font=0;

void cbm600_vh_init(void)
{
	UINT8 *gfx = memory_region(REGION_GFX1);
	int i;

	/* inversion logic on board */
	for (i=0; i<0x800; i++) {
		gfx[0x1000+i]=gfx[0x800+i];
		gfx[0x1800+i]=gfx[0x1000+i]^0xff;
		gfx[0x800+i]=gfx[i]^0xff;
	}
}

void cbm700_vh_init(void)
{
	UINT8 *gfx = memory_region(REGION_GFX1);
	int i;
	for (i=0; i<0x800; i++) {
		gfx[0x1000+i]=gfx[0x800+i];
		gfx[0x1800+i]=gfx[0x1000+i]^0xff;
		gfx[0x800+i]=gfx[i]^0xff;
	}
}

VIDEO_START( cbm700 )
{
	int i;

    /* remove pixel column 9 for character codes 0 - 175 and 224 - 255 */
	for( i = 0; i < 256; i++)
	{
//		if( i < 176 || i > 223 )
		{
			int y;
			for( y = 0; y < Machine->gfx[0]->height; y++ ) {
				Machine->gfx[0]->gfxdata[(i * Machine->gfx[0]->height + y) * Machine->gfx[0]->width + 8] = 0;
				Machine->gfx[1]->gfxdata[(i * Machine->gfx[1]->height + y) * Machine->gfx[1]->width + 8] = 0;
			}
		}
	}

    return video_start_generic(machine);
}

void cbmb_vh_cursor(struct crtc6845_cursor *cursor)
{
	dirtybuffer[cursor->pos]=1;
}

void cbmb_vh_set_font(int font)
{
	cbmb_font=font;
}

VIDEO_UPDATE( cbmb )
{
	int x, y, i;
	rectangle rect, rect2;
	int w=crtc6845_get_char_columns(crtc6845);
	int h=crtc6845_get_char_lines(crtc6845);
	int height=crtc6845_get_char_height(crtc6845);
	int start=crtc6845_get_start(crtc6845)&0x7ff;
	struct crtc6845_cursor cursor;
	int full_refresh = 1;

	rect.min_x=Machine->screen[0].visarea.min_x;
	rect.max_x=Machine->screen[0].visarea.max_x;
	if (full_refresh) {
		memset(dirtybuffer, 1, videoram_size);
	}

	crtc6845_time(crtc6845);
	crtc6845_get_cursor(crtc6845, &cursor);

	for (y=0, rect.min_y=0, rect.max_y=height-1, i=start; y<h;
		 y++, rect.min_y+=height, rect.max_y+=height) {
		for (x=0; x<w; x++, i=(i+1)&0x7ff) {
			if (dirtybuffer[i]) {
				drawgfx(bitmap,Machine->gfx[cbmb_font],
						videoram[i], 0, 0, 0, Machine->gfx[cbmb_font]->width*x,height*y,
						&rect,TRANSPARENCY_NONE,0);
				if ((cursor.on)&&(i==cursor.pos)) {
					int k=height-cursor.top;
					rect2=rect;
					rect2.min_y+=cursor.top; 
					if (cursor.bottom<height) k=cursor.bottom-cursor.top+1;

					if (k>0)
						plot_box(bitmap, Machine->gfx[cbmb_font]->width*x, 
								 height*y+cursor.top, 
								 Machine->gfx[cbmb_font]->width, k, Machine->pens[1]);
				}

				dirtybuffer[i]=0;
			}
		}
	}
	return 0;
}

