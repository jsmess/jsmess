/***************************************************************************

	vidhrdw/oric.c
	
	All graphic effects are supported including mid-line changes.
	There may be some small bugs.

	TODO:
	- speed up this code a bit?
	
***************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "includes/oric.h"

static void oric_vh_update_flash(void);
static void oric_vh_update_attribute(int c);
static void oric_refresh_charset(void);

/* current state of the display */
/* some attributes persist until they are turned off.
This structure holds this persistant information */
struct oric_vh_state
{
	/* foreground and background colour used for rendering */
	/* if flash attribute is set, these two will both be equal
	to background colour */
	int active_foreground_colour;
	int active_background_colour;
	/* current foreground and background colour */
	int foreground_colour;
	int background_colour;
	int mode;
	/* text attributes */
	int text_attributes;

	unsigned long read_addr;

	/* current addr to fetch data */
	unsigned char *char_data;
	/* base of char data */
	unsigned char *char_base;
	
	/* if (1<<3), display graphics, if 0, hide graphics */
	int flash_state;
	/* current count */
	int flash_count;
};


static struct oric_vh_state vh_state;

static void oric_vh_timer_callback(int reg)
{
	/* update flash count */
	vh_state.flash_count++;
	if (vh_state.flash_count == 30)
	{	
		vh_state.flash_count = 0;
		vh_state.flash_state ^=(1<<3);
		oric_vh_update_flash();
	}
}

VIDEO_START( oric )
{
	/* initialise flash timer */
	vh_state.flash_count = 0;
	vh_state.flash_state = 0;
	timer_pulse(TIME_IN_HZ(50), 0, oric_vh_timer_callback);
	/* mode */
	oric_vh_update_attribute((1<<3)|(1<<4));
    return 0;
}


static void oric_vh_update_flash(void)
{
	/* flash active? */
	if (vh_state.text_attributes & (1<<2))
	{
		/* yes */

		/* show or hide text? */
		if (vh_state.flash_state)
		{
			/* hide */
			/* set foreground and background to be the same */
			vh_state.active_foreground_colour = vh_state.background_colour;
			vh_state.active_background_colour = vh_state.background_colour;
			return;
		}
	}
	

	/* show */
	vh_state.active_foreground_colour = vh_state.foreground_colour;
	vh_state.active_background_colour = vh_state.background_colour;
}

/* the alternate charset follows from the standard charset.
Each charset holds 128 chars with 8 bytes for each char.

The start address for the standard charset is dependant on the video mode */
static void oric_refresh_charset(void)
{
	/* alternate char set? */
	if ((vh_state.text_attributes & (1<<0))==0)
	{
		/* no */
		vh_state.char_data = vh_state.char_base;
	}
	else
	{
		/* yes */
		vh_state.char_data = vh_state.char_base + (128*8);
	}
}

/* update video hardware state depending on the new attribute */
static void oric_vh_update_attribute(int c)
{
	/* attribute */
	int attribute = c & 0x03f;

	switch ((attribute>>3) & 0x03)
	{
		case 0:
		{
			/* set foreground colour */
			vh_state.foreground_colour = attribute & 0x07;
			oric_vh_update_flash();
		}
		break;

		case 1:
		{
			vh_state.text_attributes = attribute & 0x07;

			oric_refresh_charset();

			/* text attributes */
			oric_vh_update_flash();
		}
		break;

		case 2:
		{
			/* set background colour */
			vh_state.background_colour = attribute & 0x07;
			oric_vh_update_flash();
		}
		break;

		case 3:
		{
			/* set video mode */
			vh_state.mode = attribute & 0x07;

			/* a different charset base is used depending on the video mode */
			/* hires takes all the data from 0x0a000 through to about 0x0bf68,
			so the charset is moved to 0x09800 */
			/* text mode starts at 0x0bb80 and so the charset is in a different location */
			if (vh_state.mode & (1<<2))
			{
				/* set screen memory base and standard charset location for this mode */
				vh_state.read_addr = 0x0a000;
				if (oric_ram)
					vh_state.char_base = oric_ram + (unsigned long)0x09800; 
				else
					vh_state.char_base = memory_get_read_ptr(0, ADDRESS_SPACE_PROGRAM, 0x09800);
				
				/* changing the mode also changes the position of the standard charset
				and alternative charset */
				oric_refresh_charset();
			}
			else
			{
				/* set screen memory base and standard charset location for this mode */
				vh_state.read_addr = 0x0bb80;
				if (oric_ram)
					vh_state.char_base = oric_ram + (unsigned long)0x0b400;
				else
					vh_state.char_base = memory_get_read_ptr(0, ADDRESS_SPACE_PROGRAM, 0x0b400);
			
				/* changing the mode also changes the position of the standard charset
				and alternative charset */
				oric_refresh_charset();
			}
		}
		break;

		default:
			break;
	}
}


/* render 6-pixels using foreground and background colours specified */
/* used in hires and text mode */
static void oric_vh_render_6pixels(mame_bitmap *bitmap,int x,int y, int fg, int bg,int data, int invert_flag)
{
	int i;
	int pens[2];
	int px;
	
	/* invert? */
	if (invert_flag)
	{
		fg ^=0x07;
		bg ^=0x07;
	}

	pens[1] = Machine->pens[fg];
	pens[0] = Machine->pens[bg];
	
	px = x;
	for (i=0; i<6; i++)
	{
		int col;

		col = pens[(data>>5) & 0x01];
		plot_pixel(bitmap,px, y, col);
		px++;
		data = data<<1;
	}
}

			



/***************************************************************************
  oric_vh_screenrefresh
***************************************************************************/
VIDEO_UPDATE( oric )
{
	unsigned char *RAM;
	int byte_offset;
	int y;
	unsigned long read_addr_base;
	int hires_active;

	RAM = oric_ram;

	/* set initial base */
	read_addr_base = vh_state.read_addr;

	/* is hires active? */
	if (vh_state.mode & (1<<2))
	{
		hires_active = 1;
	}
	else
	{
		hires_active = 0;
	}			


	for (y = 0; y < 224; y++)
	{
		int x = 0;

		/* foreground colour white */
		oric_vh_update_attribute(7);
		/* background colour black */
		oric_vh_update_attribute((1<<3));

		oric_vh_update_attribute((1<<4));
		
		for (byte_offset=0; byte_offset<40; byte_offset++)
		{
			int c;
			unsigned long read_addr;

			/* after line 200 all rendering is done in text mode */
			if (y<200)
			{
				/* calculate fetch address based on current line and
				current mode */
				if (hires_active)
				{
					read_addr = (unsigned long)read_addr_base + (unsigned long)byte_offset + (unsigned long)(y*40);
				}
				else
				{
					int char_line;

					char_line = (y>>3);
					read_addr = (unsigned long)read_addr_base + (unsigned long)byte_offset + (unsigned long)(char_line*40);		
				}
			}
			else
			{
				int char_line;

				char_line = ((y-200)>>3);
				read_addr = (unsigned long)read_addr_base + (unsigned long)byte_offset + (unsigned long)(char_line*40);		
			}

			/* fetch data */
			c = RAM ? RAM[read_addr] : program_read_byte_8(read_addr);

			/* if bits 6 and 5 are zero, the byte contains a serial attribute */
			if ((c & ((1<<6) | (1<<5)))==0)
			{
				oric_vh_update_attribute(c);

				/* display background colour when attribute has been found */
				oric_vh_render_6pixels(bitmap,x,y,vh_state.active_foreground_colour, vh_state.active_background_colour, 0,(c & 0x080));

				if (y<200)
				{				
					/* is hires active? */
					if (vh_state.mode & (1<<2))
					{
						hires_active = 1;
					}
					else
					{
						hires_active = 0;
					}
					
					read_addr_base = vh_state.read_addr;
				}
			}
			else
			{
				/* hires? */
				if (hires_active)
				{
					int pixel_data = c & 0x03f;
					/* plot hires pixels */
					oric_vh_render_6pixels(bitmap,x,y,vh_state.active_foreground_colour, vh_state.active_background_colour, pixel_data,(c & 0x080));
				}
				else
				{
					int char_index;
					int char_data;
					int ch_line;

					char_index = (c & 0x07f);

					ch_line = y & 7;

					/* is double height set? */
					if (vh_state.text_attributes & (1<<1))
					{ 
						/* if char line is even, top half of character is displayed,
						if char line is odd, bottom half of character is displayed */
						int double_height_flag = ((y>>3) & 0x01);

						/* calculate line to fetch */
						ch_line = (ch_line>>1) + (double_height_flag<<2);
					}
					
					/* fetch pixel data for this char line */
					char_data = vh_state.char_data[(char_index<<3) | ch_line] & 0x03f;

					/* draw! */
					oric_vh_render_6pixels(bitmap,x,y,
						vh_state.active_foreground_colour, 
						vh_state.active_background_colour, char_data, (c & 0x080));
				}

			}
			
			x=x+6;	
		}
		
		/* after 200 lines have been drawn, force a change of the read address */
		/* there are 200 lines of hires/text mode, then 24 lines of text mode */
		/* the mode can't be changed in the last 24 lines. */
		if (y==199)
		{
			/* mode */
			read_addr_base = (unsigned long)0x0bf68;
			hires_active = 0;
		}
	}
	return 0;
}


