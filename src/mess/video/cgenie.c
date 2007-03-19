/***************************************************************************

  cgenie.c

  Functions to emulate the video controller 6845.

***************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "includes/cgenie.h"

int cgenie_font_offset[4] = {0, 0, 0, 0};

static CRTC6845 crt;
static int graphics = 0;
static mame_bitmap *dlybitmap = NULL;
static UINT8 *cleanbuffer = NULL;
static UINT8 *colorbuffer = NULL;
static int update_all = 0;
static int off_x = 0;
static int off_y = 0;


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
VIDEO_START( cgenie )
{
	videoram_size = 0x4000;

	if( video_start_generic(machine) != 0 )
        return 1;

    dlybitmap = auto_bitmap_alloc(Machine->screen[0].width, Machine->screen[0].height, BITMAP_FORMAT_INDEXED16);

    cleanbuffer = (UINT8*)auto_malloc(64 * 32 * 8);
	memset(cleanbuffer, 0, 64 * 32 * 8);

	colorbuffer = (UINT8*)auto_malloc(64 * 32 * 8);
	memset(colorbuffer, 0, 64 * 32 * 8);

	return 0;
}

/***************************************************************************

  Calculate the horizontal and vertical offset for the
  current register settings of the 6845 CRTC

***************************************************************************/
static void cgenie_offset_xy(void)
{
	if( crt.horizontal_sync_pos )
		off_x = crt.horizontal_total - crt.horizontal_sync_pos - 14;
	else
		off_x = -15;

	off_y = (crt.vertical_total - crt.vertical_sync_pos) *
		(crt.scan_lines + 1) + crt.vertical_adjust
		- 32;

	if( off_y < 0 )
		off_y = 0;

	if( off_y > 128 )
		off_y = 128;

// logerror("cgenie offset x:%d  y:%d\n", off_x, off_y);
}


/***************************************************************************
  Write to an indexed register of the 6845 CRTC
***************************************************************************/
WRITE8_HANDLER ( cgenie_register_w )
{
	int addr;

	switch (crt.idx)
	{
		case 0:
			if( crt.horizontal_total == data )
				break;
			crt.horizontal_total = data;
			cgenie_offset_xy();
			break;
		case 1:
			if( crt.horizontal_displayed == data )
				break;
			crt.horizontal_displayed = data;
			break;
		case 2:
			if( crt.horizontal_sync_pos == data )
				break;
			crt.horizontal_sync_pos = data;
			cgenie_offset_xy();
			break;
		case 3:
			crt.horizontal_length = data;
			break;
		case 4:
			if( crt.vertical_total == data )
				break;
			crt.vertical_total = data;
			cgenie_offset_xy();
			break;
		case 5:
			if( crt.vertical_adjust == data )
				break;
			crt.vertical_adjust = data;
			cgenie_offset_xy();
			break;
		case 6:
			if( crt.vertical_displayed == data )
				break;
			crt.vertical_displayed = data;
			break;
		case 7:
			if( crt.vertical_sync_pos == data )
				break;
			crt.vertical_sync_pos = data;
			cgenie_offset_xy();
			break;
		case 8:
			crt.crt_mode = data;
			break;
		case 9:
			data &= 15;
			if( crt.scan_lines == data )
				break;
			crt.scan_lines = data;
			cgenie_offset_xy();
			break;
		case 10:
			if( crt.cursor_top == data )
				break;
			crt.cursor_top = data;
			addr = 256 * crt.cursor_address_hi + crt.cursor_address_lo;
			dirtybuffer[addr] = 1;
            break;
		case 11:
			if( crt.cursor_bottom == data )
				break;
			crt.cursor_bottom = data;
			addr = 256 * crt.cursor_address_hi + crt.cursor_address_lo;
			dirtybuffer[addr] = 1;
            break;
		case 12:
			data &= 63;
			if( crt.screen_address_hi == data )
				break;
			update_all = 1;
			crt.screen_address_hi = data;
			break;
		case 13:
			if( crt.screen_address_lo == data )
				break;
			update_all = 1;
			crt.screen_address_lo = data;
			break;
		case 14:
			data &= 63;
			if( crt.cursor_address_hi == data )
				break;
			crt.cursor_address_hi = data;
			addr = 256 * crt.cursor_address_hi + crt.cursor_address_lo;
			dirtybuffer[addr] = 1;
            break;
		case 15:
			if( crt.cursor_address_lo == data )
				break;
			crt.cursor_address_lo = data;
			addr = 256 * crt.cursor_address_hi + crt.cursor_address_lo;
			dirtybuffer[addr] = 1;
            break;
	}
}

/***************************************************************************
  Write to the index register of the 6845 CRTC
***************************************************************************/
WRITE8_HANDLER ( cgenie_index_w )
{
	crt.idx = data & 15;
}

/***************************************************************************
  Read from an indexed register of the 6845 CRTC
***************************************************************************/
 READ8_HANDLER ( cgenie_register_r )
{
	return cgenie_get_register(crt.idx);
}

/***************************************************************************
  Read from a register of the 6845 CRTC
***************************************************************************/
int cgenie_get_register(int indx)
{
	switch (indx)
	{
		case 0:
			return crt.horizontal_total;
		case 1:
			return crt.horizontal_displayed;
		case 2:
			return crt.horizontal_sync_pos;
		case 3:
			return crt.horizontal_length;
		case 4:
			return crt.vertical_total;
		case 5:
			return crt.vertical_adjust;
		case 6:
			return crt.vertical_displayed;
		case 7:
			return crt.vertical_sync_pos;
		case 8:
			return crt.crt_mode;
		case 9:
			return crt.scan_lines;
		case 10:
			return crt.cursor_top;
		case 11:
			return crt.cursor_bottom;
		case 12:
			return crt.screen_address_hi;
		case 13:
			return crt.screen_address_lo;
		case 14:
			return crt.cursor_address_hi;
		case 15:
			return crt.cursor_address_lo;
	}
	return 0;
}

/***************************************************************************
  Read the index register of the 6845 CRTC
***************************************************************************/
 READ8_HANDLER ( cgenie_index_r )
{
	return crt.idx;
}

/***************************************************************************
  Switch mode between character generator and graphics
***************************************************************************/
void cgenie_mode_select(int mode)
{
	graphics = (mode) ? 1 : 0;
}

/***************************************************************************
  Invalidate a range of characters with codes from l to h
***************************************************************************/
void cgenie_invalidate_range(int l, int h)
{
	int base = 256 * crt.screen_address_hi + crt.screen_address_lo;
	int size = crt.horizontal_displayed * crt.vertical_displayed;
	int addr;
	int i;

	for (addr = 0; addr < size; addr++)
	{
		i = (base + addr) & 0x3fff;
		if( videoram[i] >= l && videoram[i] <= h )
			dirtybuffer[i] = 1;
	}
}


static void cgenie_refresh_monitor(mame_bitmap * bitmap, int full_refresh)
{
	int i, address, offset, cursor, size, code, x, y;
    rectangle r;

	if( crt.vertical_displayed == 0 || crt.horizontal_displayed == 0 )
	{
		fillbitmap(bitmap, Machine->remapped_colortable[0], &Machine->screen[0].visarea);
	}
	else
	{
		offset = 256 * crt.screen_address_hi + crt.screen_address_lo;
		size = crt.horizontal_displayed * crt.vertical_displayed;
		cursor = 256 * crt.cursor_address_hi + crt.cursor_address_lo;

		if( full_refresh )
		{
			full_refresh = 0;
			fillbitmap(bitmap, Machine->remapped_colortable[0], &Machine->screen[0].visarea);
			for( i = offset; i < offset + size; i++ )
				dirtybuffer[i] = 1;
		}

		/*
		 * for every character in the Video RAM, check if it has been modified since
		 * last time and update it accordingly.
		 */
		for( address = 0; address < size; address++ )
		{
			i = (offset + address) & 0x3fff;
			x = address % crt.horizontal_displayed + off_x;
			y = address / crt.horizontal_displayed;
			if( dirtybuffer[i] || (update_all &&
				(cleanbuffer[y * 64 + x] != videoram[i] ||
				  colorbuffer[y * 64 + x] != colorram[i & 0x3ff])) )
			{
				r.min_x = x * 8;
				r.max_x = r.min_x + 7;
				r.min_y = y * (crt.scan_lines + 1) + off_y;
				r.max_y = r.min_y + crt.scan_lines;

				colorbuffer[y * 64 + x] = colorram[i & 0x3ff];
				cleanbuffer[y * 64 + x] = videoram[i];
				dirtybuffer[i] = 0;

				if( graphics )
				{
					/* get graphics code */
					code = videoram[i];
					drawgfx(bitmap, Machine->gfx[1], code, 0,
						0, 0, r.min_x, r.min_y, &r, TRANSPARENCY_NONE, 0);
				}
				else
				{
					/* get character code */
					code = videoram[i];
					/* translate defined character sets */
					code += cgenie_font_offset[(code >> 6) & 3];
					drawgfx(bitmap, Machine->gfx[0], code, colorram[i&0x3ff],
						0, 0, r.min_x, r.min_y, &r, TRANSPARENCY_NONE, 0);
				}

				if( i == cursor )
				{
				rectangle rc;

				/* check if cursor turned off */
					if( (crt.cursor_top & 0x60) == 0x20 )
						continue;
					dirtybuffer[i] = 1;
					if( (crt.cursor_top & 0x60) == 0x60 )
					{
						crt.cursor_visible = 1;
					}
					else
					{
						crt.cursor_phase++;
						crt.cursor_visible = (crt.cursor_phase >> 3) & 1;
					}
					if( !crt.cursor_visible )
						continue;
					rc.min_x = r.min_x;
					rc.max_x = r.max_x;
					rc.min_y = r.min_y + (crt.cursor_top & 15);
					rc.max_y = r.min_y + (crt.cursor_bottom & 15);
					drawgfx(bitmap, Machine->gfx[0], 0x7f, colorram[i&0x3ff],
						0, 0, rc.min_x, rc.min_y, &rc, TRANSPARENCY_NONE, 0);
				}
			}
		}
	}
	update_all = 0;
}

static void cgenie_refresh_tv_set(mame_bitmap * bitmap, int full_refresh)
{
	int i, address, offset, cursor, size, code, x, y;
    rectangle r;

    if( crt.vertical_displayed == 0 || crt.horizontal_displayed == 0 )
	{
		fillbitmap(tmpbitmap, Machine->remapped_colortable[0], &Machine->screen[0].visarea);
		fillbitmap(dlybitmap, Machine->remapped_colortable[0], &Machine->screen[0].visarea);
	}
	else
	{
		offset = 256 * crt.screen_address_hi + crt.screen_address_lo;
		size = crt.horizontal_displayed * crt.vertical_displayed;
		cursor = 256 * crt.cursor_address_hi + crt.cursor_address_lo;

		if( full_refresh )
		{
			full_refresh = 0;
			fillbitmap(tmpbitmap, Machine->remapped_colortable[0], &Machine->screen[0].visarea);
			fillbitmap(dlybitmap, Machine->remapped_colortable[0], &Machine->screen[0].visarea);
			for (i = offset; i < offset + size; i++)
				dirtybuffer[i] = 1;
		}

		/*
		 * for every character in the Video RAM, check if it has been modified since
		 * last time and update it accordingly.
		 */
		for( address = 0; address < size; address++ )
		{
			i = (offset + address) & 0x3fff;
			x = address % crt.horizontal_displayed + off_x;
			y = address / crt.horizontal_displayed;
			if( dirtybuffer[i] || (update_all &&
				 (cleanbuffer[y * 64 + x] != videoram[i] ||
				  colorbuffer[y * 64 + x] != colorram[i & 0x3ff])) )
			{
				r.min_x = x * 8;
				r.max_x = r.min_x + 7;
				r.min_y = y * (crt.scan_lines + 1) + off_y;
				r.max_y = r.min_y + crt.scan_lines;

				colorbuffer[y * 64 + x] = colorram[i & 0x3ff];
				cleanbuffer[y * 64 + x] = videoram[i];
				dirtybuffer[i] = 0;

				if( graphics )
				{
					/* get graphics code */
					code = videoram[i];
					drawgfx(tmpbitmap, Machine->gfx[1], code, 1,
						0, 0, r.min_x, r.min_y, &r, TRANSPARENCY_NONE, 0);
					drawgfx(dlybitmap, Machine->gfx[1], code, 2,
						0, 0, r.min_x, r.min_y, &r, TRANSPARENCY_NONE, 0);
				}
				else
				{
					/* get character code */
					code = videoram[i];
					/* translate defined character sets */
					code += cgenie_font_offset[(code >> 6) & 3];
					drawgfx(tmpbitmap, Machine->gfx[0], code, colorram[i&0x3ff] + 16,
						0, 0, r.min_x, r.min_y, &r, TRANSPARENCY_NONE, 0);
					drawgfx(dlybitmap, Machine->gfx[0], code, colorram[i&0x3ff] + 32,
						0, 0, r.min_x, r.min_y, &r, TRANSPARENCY_NONE, 0);
				}

				if( i == cursor )
				{
					rectangle rc;

					/* check if cursor turned off */
					if( (crt.cursor_top & 0x60) == 0x20 )
						continue;
					dirtybuffer[i] = 1;
					if( (crt.cursor_top & 0x60) == 0x60 )
					{
						crt.cursor_visible = 1;
					}
					else
					{
						crt.cursor_phase++;
						crt.cursor_visible = (crt.cursor_phase >> 3) & 1;
					}
					if( !crt.cursor_visible )
						continue;
					rc.min_x = r.min_x;
					rc.max_x = r.max_x;
					rc.min_y = r.min_y + (crt.cursor_top & 15);
					rc.max_y = r.min_y + (crt.cursor_bottom & 15);
					drawgfx(tmpbitmap, Machine->gfx[0], 0x7f, colorram[i&0x3ff] + 16,
						0, 0, rc.min_x, rc.min_y, &rc, TRANSPARENCY_NONE, 0);
					drawgfx(dlybitmap, Machine->gfx[0], 0x7f, colorram[i&0x3ff] + 32,
						0, 0, rc.min_x, rc.min_y, &rc, TRANSPARENCY_NONE, 0);
				}
			}
		}
	}
	update_all = 0;
	copybitmap(bitmap, tmpbitmap, 0, 0, 0, 0,
		&Machine->screen[0].visarea, TRANSPARENCY_NONE, 0);
	copybitmap(bitmap, dlybitmap, 0, 0, 1, 0,
		&Machine->screen[0].visarea, TRANSPARENCY_COLOR, 0);
}

/***************************************************************************
  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function,
  it will be called by the main emulation engine.
***************************************************************************/
VIDEO_UPDATE( cgenie )
{
	int full_refresh = 1;
    if( cgenie_tv_mode )
		cgenie_refresh_tv_set(bitmap,full_refresh);
	else
		cgenie_refresh_monitor(bitmap,full_refresh);
	return 0;
}
