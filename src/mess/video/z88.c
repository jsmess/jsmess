/***************************************************************************

  z88.c

  Functions to emulate the video hardware of the Acorn Z88

***************************************************************************/

#include "emu.h"
#include "machine/ram.h"
#include "includes/z88.h"


INLINE void z88_plot_pixel(bitmap_t *bitmap, int x, int y, UINT32 color)
{
	*BITMAP_ADDR16(bitmap, y, x) = (UINT16)color;
}

/***************************************************************************
  Start the video hardware emulation.
***************************************************************************/

/* two colours */
static const unsigned short z88_colour_table[Z88_NUM_COLOURS] =
{
	0, 1, 2
};

/* black/white */
static const rgb_t z88_palette[Z88_NUM_COLOURS] =
{
	MAKE_RGB(0x000, 0x000, 0x000),
	MAKE_RGB(0x0ff, 0x0ff, 0x0ff),
	MAKE_RGB(0x080, 0x080, 0x080)
};


/* Initialise the palette */
PALETTE_INIT( z88 )
{
	palette_set_colors(machine, 0, z88_palette, ARRAY_LENGTH(z88_palette));
}

/* temp - change to gfxelement structure */

static void z88_vh_render_8x8(bitmap_t *bitmap, int x, int y, int pen0, int pen1, unsigned char *pData)
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

                z88_plot_pixel(bitmap, x+b, y+h, pen);

                data = data<<1;
            }
        }
}

static void z88_vh_render_6x8(bitmap_t *bitmap, int x, int y, int pen0, int pen1, unsigned char *pData)
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

			z88_plot_pixel(bitmap, x+1+b, y+h, pen);
			data = data<<1;
		}

		z88_plot_pixel(bitmap,x,y+h, pen0);
		z88_plot_pixel(bitmap,x+7,y+h, pen0);
	}
}

static void z88_vh_render_line(bitmap_t *bitmap, int x, int y,int pen)
{
	z88_plot_pixel(bitmap, x, y+7, pen);
	z88_plot_pixel(bitmap, x+1, y+7, pen);
	z88_plot_pixel(bitmap, x+2, y+7, pen);
	z88_plot_pixel(bitmap, x+3, y+7, pen);
	z88_plot_pixel(bitmap, x+4, y+7, pen);
	z88_plot_pixel(bitmap, x+5, y+7, pen);
	z88_plot_pixel(bitmap, x+6, y+7, pen);
	z88_plot_pixel(bitmap, x+7, y+7, pen);
}

/* convert absolute offset into correct address to get data from */
static unsigned char *z88_convert_address(running_machine &machine, unsigned long offset)
{
//        return ram_get_ptr(machine.device(RAM_TAG));
	if (offset>(32*16384))
	{
		unsigned long get_offset;
		get_offset = offset - (32*16384);
		get_offset = get_offset & 0x01fffff;
		return ram_get_ptr(machine.device(RAM_TAG)) + get_offset;
	}
	else
	{
		offset = offset & 0x01FFFF;
		return machine.region("maincpu")->base() + 0x010000 + offset;
	}
}


SCREEN_EOF( z88 )
{
	z88_state *state = machine.driver_data<z88_state>();
	state->m_frame_number++;
	if (state->m_frame_number >= 50)
	{
		state->m_frame_number = 0;
		state->m_flash_invert = !state->m_flash_invert;
	}
}



/***************************************************************************
  Draw the game screen in the given bitmap_t.
  Do NOT call osd_update_display() from this fuz88tion,
  it will be called by the main emulation engine.
***************************************************************************/
SCREEN_UPDATE( z88 )
{
	z88_state *state = screen->machine().driver_data<z88_state>();
	int x,y;
	unsigned char *ptr = z88_convert_address(screen->machine(), state->m_blink.sbf);
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
				pen1 = 0;

				if (byte1 & Z88_SCR_HW_GRY)
				{
					pen0 = 2;
				}
				else
				{
					pen0 = 1;
				}
			}
			else
			{
				pen0 = 0;
				if (byte1 & Z88_SCR_HW_GRY)
				{
					pen1 = 2;
				}
				else
				{
					pen1 = 1;
				}
			}

//          if (byte1 & Z88_SCR_HW_FLS)
//          {
//              if (state->m_flash_invert)
//              {
//                  pen1 = pen0;
//              }
//          }


			/* hi-res? */
			if (byte1 & Z88_SCR_HW_HRS)
			{
					int ch_index;
					unsigned char *pCharGfx;

					ch = (byte0 & 0x0ff) | ((byte1 & 0x01)<<8);

					if (ch & 0x0100)
					{
						ch_index =ch & 0x0ff;	//(~0x0100);
						pCharGfx = z88_convert_address(screen->machine(), state->m_blink.hires1);
					}
					else
					{
						ch_index = ch & 0x0ff;
						pCharGfx = z88_convert_address(screen->machine(), state->m_blink.hires0);
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

				   pCharGfx = z88_convert_address(screen->machine(), state->m_blink.lores0);
				}
				else
				{
				   ch_index = ch;

				   pCharGfx = z88_convert_address(screen->machine(), state->m_blink.lores1);
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
