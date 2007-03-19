/***************************************************************************

  z88.c

  Functions to emulate the video hardware of the Acorn Z88
  
***************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "includes/z88.h"

static int frame_number = 0;
static int flash_invert = 0;

/***************************************************************************
  Start the video hardware emulation.
***************************************************************************/

/* two colours */
static unsigned short z88_colour_table[Z88_NUM_COLOURS] =
{
	0, 1, 2
};

/* black/white */
static unsigned char z88_palette[Z88_NUM_COLOURS * 3] =
{
	0x000, 0x000, 0x000,
	0x0ff, 0x0ff, 0x0ff,
	0x080, 0x080, 0x080
};


/* Initialise the palette */
PALETTE_INIT( z88 )
{
	palette_set_colors(machine, 0, z88_palette, sizeof(z88_palette) / 3);
	memcpy(colortable, z88_colour_table, sizeof (z88_colour_table));
}

extern struct blink_hw blink;

/* temp - change to gfxelement structure */

static void z88_vh_render_8x8(mame_bitmap *bitmap, int x, int y, int pen0, int pen1, unsigned char *pData)
{
        int h,b;

        for (h=0; h<8; h++)
        {
            UINT8 data;

            data = pData[h];
            for (b=0; b<8; b++)
            {
                int pen;

                if (data & 0x080)
                {
                  pen = pen1;
                }
                else
                {
                  pen = pen0;
                }

                plot_pixel(bitmap, x+b, y+h, pen);

                data = data<<1;
            }
        }
}

static void z88_vh_render_6x8(mame_bitmap *bitmap, int x, int y, int pen0, int pen1, unsigned char *pData)
{
	int h,b;

	for (h=0; h<8; h++)
	{
		UINT8 data;

		data = pData[h];
		data = data<<2;

		for (b=0; b<6; b++)
		{
			int pen;
			if (data & 0x080)
			{
			  pen = pen1;
			}
			else
			{
			  pen = pen0;
			}

			plot_pixel(bitmap, x+1+b, y+h, pen);
			data = data<<1;
		}

		plot_pixel(bitmap,x,y+h, pen0);
		plot_pixel(bitmap,x+7,y+h, pen0);
	}
}

static void z88_vh_render_line(mame_bitmap *bitmap, int x, int y,int pen)
{
	plot_pixel(bitmap, x, y+7, pen);
	plot_pixel(bitmap, x+1, y+7, pen);
	plot_pixel(bitmap, x+2, y+7, pen);
	plot_pixel(bitmap, x+3, y+7, pen);
	plot_pixel(bitmap, x+4, y+7, pen);
	plot_pixel(bitmap, x+5, y+7, pen);
	plot_pixel(bitmap, x+6, y+7, pen);
	plot_pixel(bitmap, x+7, y+7, pen);
}

/* convert absolute offset into correct address to get data from */
static unsigned  char *z88_convert_address(unsigned long offset)
{
//        return mess_ram;
	if (offset>(32*16384))
	{
		unsigned long get_offset;
		get_offset = offset - (32*16384);
		get_offset = get_offset & 0x01fffff;
		return mess_ram + get_offset;
	}
	else
	{
		offset = offset & 0x01FFFF;
		return memory_region(REGION_CPU1) + 0x010000 + offset;
	}
}


VIDEO_EOF( z88 )
{
	frame_number++;
	if (frame_number >= 50)
	{
		frame_number = 0;
		flash_invert = !flash_invert;
	}
}



/***************************************************************************
  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this fuz88tion,
  it will be called by the main emulation engine.
***************************************************************************/
VIDEO_UPDATE( z88 )
{
    int x,y;
    unsigned char *ptr = z88_convert_address(blink.sbf);
	unsigned char *stored_ptr = ptr;
    int pen0, pen1;

	for (y=0; y<(Z88_SCREEN_HEIGHT>>3); y++)
	{
		ptr = stored_ptr;

		for (x=0; x<(Z88_SCREEN_WIDTH>>3); x++)
		{
			int ch;

			UINT8 byte0, byte1;

			byte0 = ptr[0];
			byte1 = ptr[1];
			ptr+=2;

			/* inverted graphics? */
				if (byte1 & Z88_SCR_HW_REV)
				{
				pen1 = Machine->pens[0];
				
				if (byte1 & Z88_SCR_HW_GRY)
				{
					pen0 = Machine->pens[2];
				}
				else
				{
					pen0 = Machine->pens[1];
				}
				}
				else
                {       
				pen0 = Machine->pens[0];
				if (byte1 & Z88_SCR_HW_GRY)
				{
					pen1 = Machine->pens[2];
				}
				else
				{
					pen1 = Machine->pens[1];
				}
			}

//			if (byte1 & Z88_SCR_HW_FLS)
//			{
//				if (flash_invert)
//				{
//					pen1 = pen0;
//				}
//			}


			/* hi-res? */
			if (byte1 & Z88_SCR_HW_HRS)
			{
					int ch_index;
					unsigned char *pCharGfx;

					ch = (byte0 & 0x0ff) | ((byte1 & 0x01)<<8);

					if (ch & 0x0100)
					{
						ch_index =ch & 0x0ff;	//(~0x0100);
						pCharGfx = z88_convert_address(blink.hires1);
					}
					else
					{
						ch_index = ch & 0x0ff;
						pCharGfx = z88_convert_address(blink.hires0);
					}

					pCharGfx += (ch_index<<3);

				z88_vh_render_8x8(bitmap, (x<<3),(y<<3), pen0, pen1, pCharGfx);
			}
			else
			{
				unsigned char *pCharGfx;
				int ch_index;

				ch = (byte0 & 0x0ff) | ((byte1 & 0x01)<<8);

				if ((ch & 0x01c0)==0x01c0)
				{
				   ch_index = ch & (~0x01c0);

				   pCharGfx = z88_convert_address(blink.lores0);
				}
				else
				{
				   ch_index = ch;

				   pCharGfx = z88_convert_address(blink.lores1);
				}

				pCharGfx += (ch_index<<3);

				z88_vh_render_6x8(bitmap, (x<<3),(y<<3), pen0,pen1,pCharGfx);

				/* underline? */
				if (byte1 & Z88_SCR_HW_UND)
				{
					z88_vh_render_line(bitmap, (x<<3), (y<<3), pen1);
				}
			}



		}

		stored_ptr+=256;
	}		
	return 0;
}
