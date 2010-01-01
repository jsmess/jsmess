/***************************************************************************

  avigo.c

  Functions to emulate the video hardware of the TI Avigo 100 PDA

***************************************************************************/

#include "driver.h"
#include "includes/avigo.h"

/***************************************************************************
  Start the video hardware emulation.
***************************************************************************/

/* mem size = 0x017c0 */

static UINT8 *avigo_video_memory;

/* current column to read/write */
static UINT8 avigo_screen_column = 0;

#define AVIGO_VIDEO_DEBUG 0
#define LOG(x) do { if (AVIGO_VIDEO_DEBUG) logerror x; } while (0)

static int stylus_x;
static int stylus_y;

static const gfx_layout pointerlayout =
{
	8, 8,
	1,
	2,
	{0, 64},
	{0, 1, 2, 3, 4, 5, 6, 7},
	{0 * 8, 1 * 8, 2 * 8, 3 * 8, 4 * 8, 5 * 8, 6 * 8, 7 * 8},
	8 * 8
};

static const UINT8 pointermask[] =
{
	0x00, 0x70, 0x60, 0x50, 0x08, 0x04, 0x00, 0x00,		/* blackmask */
	0xf0, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00	/* whitemask */
};

static gfx_element *stylus_pointer;

void	avigo_vh_set_stylus_marker_position(int x,int y)
{
	stylus_x = x;
	stylus_y = y;
}

READ8_HANDLER(avigo_vid_memory_r)
{
	if (!offset)
		return avigo_screen_column;

	if ((offset<0x0100) || (offset>=0x01f0))
	{
		LOG(("vid mem read: %04x\n", offset));
		return 0;
	}

	/* 0x0100-0x01f0 contains data for selected column */
	return avigo_video_memory[avigo_screen_column + ((offset&0xff)*(AVIGO_SCREEN_WIDTH>>3))];
}

WRITE8_HANDLER(avigo_vid_memory_w)
{
	if (!offset)
	{
		/* select column to read/write */
		avigo_screen_column = data;

		LOG(("vid mem column write: %02x\n",data));

		if (data>=(AVIGO_SCREEN_WIDTH>>3))
		{
			LOG(("error: vid mem column write: %02x\n",data));
		}
		return;
	}

        if ((offset<0x0100) || (offset>=0x01f0))
        {
		LOG(("vid mem write: %04x %02x\n", offset, data));
		return;
        }


	/* 0x0100-0x01f0 contains data for selected column */
	avigo_video_memory[avigo_screen_column + ((offset&0xff)*(AVIGO_SCREEN_WIDTH>>3))] = data;
}

VIDEO_START( avigo )
{
	/* current selected column to read/write */
	avigo_screen_column = 0;

	/* allocate video memory */
	avigo_video_memory = auto_alloc_array(machine, UINT8, ((AVIGO_SCREEN_WIDTH>>3)*AVIGO_SCREEN_HEIGHT+1));
	machine->gfx[0] = stylus_pointer = gfx_element_alloc(machine, &pointerlayout, pointermask, machine->config->total_colors / 16, 0);

	stylus_pointer->total_colors = 3;
}

/* Initialise the palette */
PALETTE_INIT( avigo )
{
	palette_set_color(machine,0,MAKE_RGB(0xff,0xff,0xff)); /* white  */
	palette_set_color(machine,1,MAKE_RGB(0x00,0x00,0x00)); /* black  */
}

//static unsigned int avigo_ad_x;
//static unsigned int avigo_ad_y;

/***************************************************************************
  Draw the game screen in the given bitmap_t.
  Do NOT call osd_update_display() from this function,
  it will be called by the main emulation engine.
***************************************************************************/
VIDEO_UPDATE( avigo )
{
	int y;
	int b;
	int x;
	rectangle r;

	/* draw avigo display */
	for (y=0; y<AVIGO_SCREEN_HEIGHT; y++)
	{
		int by;
		unsigned char *line_ptr = avigo_video_memory +  (y*(AVIGO_SCREEN_WIDTH>>3));

		x = 0;
		for (by=((AVIGO_SCREEN_WIDTH>>3)-1); by>=0; by--)
		{
			int px;
			unsigned char byte;

			byte = line_ptr[0];

			px = x;
			for (b=7; b>=0; b--)
			{
				*BITMAP_ADDR16(bitmap, y, px) = ((byte>>7) & 0x01);
				px++;
				byte = byte<<1;
			}

			x = px;
			line_ptr = line_ptr+1;
		}
	}

	r.min_x = 0;
	r.max_x = AVIGO_SCREEN_WIDTH;
	r.min_y = 0;
	r.max_y = AVIGO_SCREEN_HEIGHT;

	/* draw stylus marker */
	drawgfx_transpen (bitmap, &r, stylus_pointer, 0, 0, 0, 0, stylus_x, stylus_y, 0);
#if 0
	{

	char	avigo_text[256];
	sprintf(avigo_text,"X: %03x Y: %03x",avigo_ad_x, avigo_ad_y);
	// FIXME
	//ui_draw_text(avigo_text, 0, 200);
	}
	{
	int xb,yb,zb,ab,bb;
	char	avigo_text[256];
	xb =cpunum_read_byte(0,0x0c1cf) & 0x0ff;
	yb = cpunum_read_byte(0,0x0c1d0) & 0x0ff;
	zb =cpunum_read_byte(0,0x0c1d1) & 0x0ff;
	ab = cpunum_read_byte(0,0x0c1d2) & 0x0ff;
	bb =cpunum_read_byte(0,0x0c1d3) & 0x0ff;

	sprintf(avigo_text,"Xb: %02x Yb: %02x zb: %02x ab:%02x bb:%02x",xb, yb,zb,ab,bb);
	// FIXME
	//ui_draw_text(avigo_text, 0, 216+16);
	}
#endif
	return 0;
}

