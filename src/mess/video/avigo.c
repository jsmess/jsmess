/***************************************************************************

  avigo.c

  Functions to emulate the video hardware of the TI Avigo 100 PDA

***************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "includes/avigo.h"

/***************************************************************************
  Start the video hardware emulation.
***************************************************************************/

/* backdrop */
struct artwork_info *avigo_backdrop;

/* mem size = 0x017c0 */

static unsigned char *avigo_video_memory;

/* current column to read/write */
static UINT8 avigo_screen_column = 0;

//#define AVIGO_VIDEO_DEBUG

static int stylus_x;
static int stylus_y;


/* colour table filled in from avigo colour table*/
static UINT32 stylus_color_table[3] = {0,0,0};

static gfx_layout pointerlayout =
{
	8, 8,
	1,
	2,
	{0, 64},
	{0, 1, 2, 3, 4, 5, 6, 7},
	{0 * 8, 1 * 8, 2 * 8, 3 * 8, 4 * 8, 5 * 8, 6 * 8, 7 * 8},
	8 * 8
};

static UINT8 pointermask[] =
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
        unsigned char *ptr;

        if (offset==0)
        {
           return avigo_screen_column;
        }

        if ((offset<0x0100) || (offset>=0x01f0))
        {
#ifdef AVIGO_VIDEO_DEBUG
			logerror("vid mem read: %04x\n", offset);
#endif
			return 0;

		}

		/* 0x0100-0x01f0 contains data for selected column */
		ptr = avigo_video_memory + avigo_screen_column + ((offset-0x0100)*(AVIGO_SCREEN_WIDTH>>3));

		return ptr[0];
}

WRITE8_HANDLER(avigo_vid_memory_w)
{
		if (offset==0)
		{
			/* select column to read/write */
			avigo_screen_column = data;
#ifdef AVIGO_VIDEO_DEBUG
			logerror("vid mem column write: %02x\n",data);
#endif
			if (data>=(AVIGO_SCREEN_WIDTH>>3))
			{
#ifdef AVIGO_VIDEO_DEBUG
				logerror("error: vid mem column write: %02x\n",data);
#endif
			}
			return;
        }

        if ((offset<0x0100) || (offset>=0x01f0))
        {
#ifdef AVIGO_VIDEO_DEBUG
			logerror("vid mem write: %04x %02x\n", offset, data);
#endif
			return;
        }

        if ((offset>=0x0100) && (offset<=0x01f0))
        {
			unsigned char *ptr;

			/* 0x0100-0x01f0 contains data for selected column */
			ptr = avigo_video_memory + avigo_screen_column + ((offset-0x0100)*(AVIGO_SCREEN_WIDTH>>3));

			ptr[0] = data;

			return;
        }
}

VIDEO_START( avigo )
{
    /* current selected column to read/write */
    avigo_screen_column = 0;

    /* allocate video memory */
    avigo_video_memory = auto_malloc(((AVIGO_SCREEN_WIDTH>>3)*AVIGO_SCREEN_HEIGHT));
/*	
	backdrop_refresh(avigo_backdrop);
*/
	Machine->gfx[0] = stylus_pointer = allocgfx(&pointerlayout);
	decodegfx(stylus_pointer, pointermask, 0, 1);
	stylus_pointer->colortable = stylus_color_table;
	stylus_pointer->total_colors = 3;
	stylus_color_table[1] = Machine->pens[0];
	stylus_color_table[2] = Machine->pens[1]; 
	return 0;
}

/* two colours */
static unsigned short avigo_colour_table[AVIGO_NUM_COLOURS] =
{
	0, 1
};

/* black/white */
static unsigned char avigo_palette[AVIGO_NUM_COLOURS * 3] =
{
    0x0ff, 0x0ff, 0x0ff,
    0x000, 0x000, 0x000
};


/* Initialise the palette */
PALETTE_INIT( avigo )
{
/*	char *backdrop_name;
    int used = 2; */
	palette_set_colors(machine, 0, avigo_palette, sizeof(avigo_palette) / 3);
    memcpy(colortable, avigo_colour_table, sizeof (avigo_colour_table));

	/* load backdrop */
#if 0
	backdrop_name = malloc(strlen(Machine->gamedrv->name)+4+1);

    if (backdrop_name!=NULL)
	{
		strcpy(backdrop_name, Machine->gamedrv->name);
		strcat(backdrop_name, ".png");

		logerror("%s\n",backdrop_name);

        artwork_load(&avigo_backdrop, backdrop_name, used,Machine->drv->total_colors-used);

		if (avigo_backdrop)
		{
#ifdef AVIGO_VIDEO_DEBUG
			logerror("backdrop %s successfully loaded\n", backdrop_name);
#endif
            memcpy (&sys_palette[used * 3], avigo_backdrop->orig_palette, 
                    avigo_backdrop->num_pens_used * 3 * sizeof (unsigned char));
		}
		else
		{
			logerror("no backdrop loaded\n");
		}

        free(backdrop_name);
		backdrop_name = NULL;
	}
#endif
}
unsigned int avigo_ad_x;
unsigned int avigo_ad_y;



/***************************************************************************
  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function,
  it will be called by the main emulation engine.
***************************************************************************/
VIDEO_UPDATE( avigo )
{
    int y;
    int b;
    int x;
    int pens[2];
	rectangle r;

	/* draw avigo display */
    pens[0] = Machine->pens[0];
    pens[1] = Machine->pens[1];

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
				plot_pixel(bitmap, px, y, pens[(byte>>7) & 0x01]);
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
	drawgfx (bitmap, stylus_pointer, 0, 0, 0, 0,
			 stylus_x, stylus_y, &r, TRANSPARENCY_PEN, 0);

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
	return 0;
}

