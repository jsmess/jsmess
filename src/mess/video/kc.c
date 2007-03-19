/***************************************************************************

  kc.c

  Functions to emulate the video hardware of the kc85/4,kc85/3

***************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "includes/kc.h"
#include "eventlst.h"

/* KC85/4 and KC85/3 common graphics hardware */

static unsigned short kc85_colour_table[KC85_PALETTE_SIZE] =
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
        16, 17, 18, 19, 20, 21, 22, 23
};

/*
	foreground:
 
		"full" of each component

		black, 
		blue, 
		red, 
		magenta, 
		green, 
		cyan
		yellow
		white

		"full of each component + half of another component"
		black
		violet 
		red/purple
		pastel green
		sky blue
		yellow/green
		white

	background:
		"half" of each component
		black
		dark blue
		dark red
		dark magenta
		dark green
		dark cyan
		dark yellow
		dark white (grey)

 */


static unsigned char kc85_palette[KC85_PALETTE_SIZE * 3] =
{
		/* 3 bit colour value. bit 2->green, bit 1->red, bit 0->blue */
		 
		/* foreground colours */
		0x00, 0x00, 0x00,
		0x00, 0x00, 0xd0,
		0xd0, 0x00, 0x00,
		0xd0, 0x00, 0xd0,
		0x00, 0xd0, 0x00,
		0x00, 0xd0, 0xd0,
		0xd0, 0xd0, 0x00,
		0xd0, 0xd0, 0xd0,

		0x00, 0x00, 0x00,
		0x60, 0x00, 0xa0,
		0xa0, 0x60, 0x00,
		0xa0, 0x00, 0x60,
		0x00, 0xa0, 0x60,
		0x00, 0x60, 0xa0,
		0xa0, 0xa0, 0x60,
		0xd0, 0xd0, 0xd0,

		/* background colours are slightly darker than foreground colours */
		0x00, 0x00, 0x00,
		0x00, 0x00, 0xa0,
		0xa0, 0x00, 0x00,
		0xa0, 0x00, 0xa0,
		0x00, 0xa0, 0x00,
		0x00, 0xa0, 0xa0,
		0xa0, 0xa0, 0x00,
		0xa0, 0xa0, 0xa0

};


/* Initialise the palette */
PALETTE_INIT( kc85 )
{
	palette_set_colors(machine, 0, kc85_palette, sizeof(kc85_palette) / 3);
	memcpy(colortable, kc85_colour_table, sizeof (kc85_colour_table));
}

static int kc85_blink_state;

enum
{
	KC85_VIDEO_EVENT_SET_BLINK_STATE
};

/* set new blink state - record blink state in event list */
void	kc85_video_set_blink_state(int data)
{
	EventList_AddItemOffset(KC85_VIDEO_EVENT_SET_BLINK_STATE, ((data & 0x01)<<7), TIME_TO_CYCLES(0,cpu_getscanline()*cpu_getscanlineperiod()));
}


/* draw 8 pixels */
static void kc85_draw_8_pixels(mame_bitmap *bitmap,int x,int y, unsigned char colour_byte, unsigned char gfx_byte)
{
	int a;
	int background_pen;
	int foreground_pen;
	int pens[2];
	int px;

	/* 16 foreground colours, 8 background colours */

	/* bit 7 = 1: flash between foreground and background colour 0: no flash */
	/* bit 6: adjusts foreground colours by adding half of another component */
	/* bit 5,4,3 = foreground colour */
		/* bit 5: background colour -> Green */
		/* bit 4: background colour -> Red */
		/* bit 3: background colour -> Blue */	
	/* bit 2,1,0 = background colour */
		/* bit 2: background colour -> Green */
		/* bit 1: background colour -> Red */
		/* bit 0: background colour -> Blue */
    background_pen = (colour_byte&7) + 16;	
    foreground_pen = ((colour_byte>>3) & 0x0f);	

	if (colour_byte & kc85_blink_state)
	{
		foreground_pen = background_pen;
	}

    pens[0] = Machine->pens[background_pen];
	pens[1] = Machine->pens[foreground_pen];

	px = x;

    for (a=0; a<8; a++)
    {
        int pen;
		
		pen = pens[(gfx_byte>>7) & 0x01];

        plot_pixel(bitmap, px, y,pen);
		px++;
	    gfx_byte = gfx_byte<<1;
	}
}


#ifndef MIN
#define MIN(a,b) ((a<b) ? a : b)
#endif /* MIN */

/* height of screen in lines */
#define KC85_SCREEN_HEIGHT 256
/* width of display in pixels */
#define KC85_SCREEN_WIDTH 320
/* height of top border in lines */
#define KC85_TOP_BORDER_SIZE 8
/* height of bottom border in lines */
#define KC85_BOTTOM_BORDER_SIZE 8
/* total number of lines in whole frame */
#define KC85_FRAME_NUM_LINES 312
/* width of whole line in pixels */
#define KC85_LINE_SIZE 512
/* width of left border in pixels */
#define KC85_LEFT_BORDER_SIZE 8
/* width of right border in pixels */
#define KC85_RIGHT_BORDER_SIZE 8
/* width of horizontal retrace in pixels */
#define KC85_RETRACE_SIZE (KC85_LINE_SIZE - KC85_LEFT_BORDER_SIZE - KC85_RIGHT_BORDER_SIZE - KC85_SCREEN_WIDTH)
/* number of lines in vertical retrace */
#define KC85_NUM_RETRACE_LINES (KC85_FRAME_NUM_LINES - KC85_TOP_BORDER_SIZE - KC85_BOTTOM_BORDER_SIZE - KC85_SCREEN_HEIGHT)


#define KC85_CYCLES_PER_FRAME 1750000.0f/50.0f
#define KC85_CYCLES_PER_LINE (KC85_CYCLES_PER_FRAME/KC85_FRAME_NUM_LINES)
#define KC85_CYCLES_PER_PIXEL (KC85_CYCLES_PER_LINE/KC85_LINE_SIZE)

/* 

	- 1750000 cpu cycles per second
	- 35000 cpu cycles per frame
	- 35000/312 = 112.179 cycles per line
	- 112.179/(512/8) = 1.752 cycles per 8-pixels
*/

/* Operation:

	- Vertical and horizontal dimensions are split into states. 
	- Horizontal states take a multiple of horizontal cycles, and vertical states take a multiple of lines.
	- The horizontal states comprise left border, main display, right border and horizontal retrace timing.
	- The vertical states comprise top border, main display, bottom border and vertical retrace timing.


	- if frame rate is 50hz, and there are 312 lines, then the line rate is 15600hz.
	- Each frame takes 0.02 seconds, and each line takes 0.0000064 seconds - approx 64microseconds per line 

	- the event list is based on cpu time
		
*/

static int horizontal_next_state_table[]=
{
	1,2,3,0
};

/* number of cycles for each state */
static int horizontal_graphics_state_cycles[]=
{
	/* left border */
	(KC85_LEFT_BORDER_SIZE*KC85_CYCLES_PER_PIXEL),
	/* main display */
	(KC85_SCREEN_WIDTH*KC85_CYCLES_PER_PIXEL),
	/* right border */
	(KC85_RIGHT_BORDER_SIZE*KC85_CYCLES_PER_PIXEL),
	/* horizontal retrace */
	(KC85_RETRACE_SIZE*KC85_CYCLES_PER_PIXEL)
};

static int vertical_next_state_table[]=
{
	1,2,3,0
};

/* number of lines for each state */
static int vertical_graphics_state_lines[]=
{
	/* top border */
	KC85_TOP_BORDER_SIZE*KC85_CYCLES_PER_LINE,
	/* main display */
	KC85_SCREEN_HEIGHT*KC85_CYCLES_PER_LINE,
	/* bottom display */
	KC85_BOTTOM_BORDER_SIZE*KC85_CYCLES_PER_LINE,
	/* vertical retrace */
	KC85_NUM_RETRACE_LINES*KC85_CYCLES_PER_LINE
};

/*static int kc85_blink_state;*/

struct video_state
{
	/* current state */
	int state;
	/* number of cycles remaining in this state */
	int cycles_remaining_in_state;
	/* number of cycles remaining (starts at total of all states) */
	int cycles_remaining;
};

struct grab_info
{
	unsigned char *pixel_ram;
	unsigned char *colour_ram;
};

struct video_update_state
{
	/* bitmap to render to */
	mame_bitmap *bitmap;
	/* grab colour and pixel information for 8-pixels referenced by x,y coordinate */
	void (*pixel_grab_callback)(struct grab_info *,int x,int y, unsigned char *colour_ptr, unsigned char *pixel_ptr);
	/* current coords */
	int x,y;
	/* render coordinates */
	int render_x,render_y;

	struct video_state horizontal;
	struct video_state vertical;
	struct grab_info grab_data;
};


/* process visible cycles within a line */
/* the cycles will never span over the end of a line */
static void kc85_common_process_cycles(struct video_update_state *video_update, int cycles)
{
	while (cycles!=0)
	{
		int cycles_to_do;

		/* do as many cycles up to end of current state */
		cycles_to_do = MIN(video_update->horizontal.cycles_remaining_in_state, cycles);

		//logerror("process cycles: cycles_to_do %d\n",cycles_to_do);

		/* update screen based on current state */
		switch (video_update->horizontal.state)
		{
			/* border */
			case 0:
			{
			//	video_update->render_x+=(cycles_to_do)*8;				
			}
			break;

			/* pixels */
			case 1:
			{				
				int i;
	
				for (i=0; i<cycles_to_do; i++)
				{
					unsigned char colour_byte;
					unsigned char gfx_byte;

					/* grab colour and pixel information */
					video_update->pixel_grab_callback(&video_update->grab_data,video_update->x,video_update->y,&colour_byte, &gfx_byte);
					/* draw to screen */
					kc85_draw_8_pixels(video_update->bitmap, video_update->render_x, video_update->render_y,colour_byte, gfx_byte);
					/* update render coordinate */
					video_update->render_x+=8;
					video_update->x++;

					if (video_update->x>=(320/8))
					{
						video_update->x = (320/8)-1;
					}

					if (video_update->render_x>=320)
					{
						video_update->render_x = 319;
					}


				}
			}
			break;

			/* border */
			case 2:
			{
			//	video_update->render_x+=(cycles_to_do)*8;
			}
			break;

			/* invisible/retrace */
			case 3:
			{


			}
			break;
		}
		
		video_update->horizontal.cycles_remaining_in_state -=cycles_to_do;

		/* is state over? */
		if (video_update->horizontal.cycles_remaining_in_state<=0)
		{
			/* update state */
			video_update->horizontal.state = horizontal_next_state_table[video_update->horizontal.state];
			video_update->horizontal.cycles_remaining_in_state+=horizontal_graphics_state_cycles[video_update->horizontal.state];
		}

		cycles-=cycles_to_do;
	}
}

/* process a whole visible line */
static int kc85_common_vh_process_line(struct video_update_state *video_update, int cycles)
{
	int cycles_to_do;

	/* do cycles in line */
	while ((video_update->horizontal.cycles_remaining!=0) && (cycles!=0))
	{
		/* do as many cycles as will fit onto the line */
		cycles_to_do = MIN(cycles,video_update->horizontal.cycles_remaining);
	
		//logerror("process line: cycles_to_do: %d\n",cycles_to_do);
		
		/* do the cycles - draw them */
		kc85_common_process_cycles(video_update, cycles_to_do);

		video_update->horizontal.cycles_remaining -= cycles_to_do;
		cycles -=cycles_to_do;
	}

	/* line over */

	if (video_update->horizontal.cycles_remaining==0)
	{
		/* reset */
		video_update->horizontal.cycles_remaining+=KC85_CYCLES_PER_LINE;
		/* update x,y fetch pos */
		video_update->x = 0;
		video_update->y++;
	
		/* update x,y render pos */
		video_update->render_x = 0;
		video_update->render_y++;

		if (video_update->y>=256)
		{
			video_update->y = 255;
		}

		if (video_update->render_y>=256)
		{
			video_update->render_y = 255;
		}


	}

	/* cycles remaining */
	return cycles;
}

static void kc85_common_vh_process_lines(struct video_update_state *video_update, int cycles)
{
	while (cycles!=0)
	{
		int cycles_to_do;
		int cycles_done;
		
		cycles_to_do = MIN(cycles, KC85_CYCLES_PER_LINE);
		cycles_done = cycles_to_do;

		//logerror("process lines: cycles_to_do: %d\n",cycles_to_do);


		switch (video_update->vertical.state)
		{
			/* border */
			case 0:
			{
	//			cycles_done = cycles_to_do;
			}
			break;

			/* graphics */
			case 1:
			{
				int cycles_remaining;

				/* update cycles with number of cycles not processed */
				cycles_remaining = kc85_common_vh_process_line(video_update, cycles_to_do);

				cycles_done = cycles_to_do - cycles_remaining;

			}
			break;

			/* border */
			case 2:
			{
	//			cycles_done = cycles_to_do;
			}
			break;

			/* not visible */
			case 3:
			{
	//			cycles_done = cycles_to_do;
			}
			break;
		}

			
		//logerror("cycles done: %d\n",cycles_done);
		video_update->vertical.cycles_remaining_in_state -=cycles_done;

		/* is state over? */
		if (video_update->vertical.cycles_remaining_in_state<=0)
		{
			/* update state */
			video_update->vertical.state = vertical_next_state_table[video_update->vertical.state];
			video_update->vertical.cycles_remaining_in_state+=vertical_graphics_state_lines[video_update->vertical.state];
		}
		cycles-=cycles_done;
	}
}



/* the kc85 screen is 320 pixels wide and 256 pixels tall */
/* if we assume a 50hz display, there are 312 lines for the complete frame, leaving 56 lines not visible */
static void kc85_common_process_frame(mame_bitmap *bitmap, void (*pixel_grab_callback)(struct grab_info *,int x,int y,unsigned char *, unsigned char *),struct grab_info *grab_data)
{
	int cycles_remaining_in_frame = KC85_CYCLES_PER_FRAME;

	EVENT_LIST_ITEM *pItem;
    int NumItems;

	struct video_update_state video_update;
	int cycles_offset = 0;

	video_update.render_x = 0;
	video_update.render_y = 0;
	video_update.x = 0;
	video_update.y = 0;
	memcpy(&video_update.grab_data, grab_data, sizeof(struct grab_info));
	video_update.bitmap = bitmap;
	video_update.pixel_grab_callback = pixel_grab_callback;
	video_update.horizontal.state = 0;
	video_update.horizontal.cycles_remaining_in_state = horizontal_graphics_state_cycles[video_update.horizontal.state];
	video_update.horizontal.cycles_remaining = KC85_CYCLES_PER_LINE;
	video_update.vertical.state = 0;
	video_update.vertical.cycles_remaining_in_state = vertical_graphics_state_lines[video_update.vertical.state];
	video_update.vertical.cycles_remaining = KC85_CYCLES_PER_FRAME;

	/* first item in list */
	pItem = EventList_GetFirstItem();
	/* number of items remaining */
	NumItems = EventList_NumEvents();

	while (NumItems)
	{
		int delta_cycles;
		
		/* number of cycles until event will trigger */
		delta_cycles = pItem->Event_Time - cycles_offset;

		//logerror("cycles between this event and next: %d\n",delta_cycles);
		kc85_common_vh_process_lines(&video_update, delta_cycles);

		/* update number of cycles remaining in frame */
		cycles_remaining_in_frame -= delta_cycles;
		/* set new blink state */
		kc85_blink_state = pItem->Event_Data;

		/* set new cycles into frame */
		cycles_offset = pItem->Event_Time;
		/* next event */
		pItem++;
		/* update number of events remaining */
		NumItems--;
	}


	/* process remainder */
	kc85_common_vh_process_lines(&video_update, cycles_remaining_in_frame);
	EventList_Reset();
	EventList_SetOffsetStartTime ( TIME_TO_CYCLES(0,cpu_getscanline()*cpu_getscanlineperiod()) );
}



/***************************************************************************
 KC85/4 video hardware
***************************************************************************/

static void kc85_common_vh_eof_callback(void)
{
		EVENT_LIST_ITEM *pItem;
		int NumItems;

		/* Empty event buffer */
		NumItems = EventList_NumEvents();

		/* to do: the entries in this buffer are ignored. OK? */
		if (NumItems)
		{
			pItem = EventList_GetFirstItem();
			EventList_Reset();
			EventList_SetOffsetStartTime ( TIME_TO_CYCLES(0,cpu_getscanline()*cpu_getscanlineperiod()) );
			logerror ("Event log reset in callback fn.\n");
		}
}


static void kc85_common_vh_start(void)
{
	kc85_blink_state = 0;
	EventList_Initialise(30000);
}

static unsigned char *kc85_4_display_video_ram;

static unsigned char *kc85_4_video_ram;

VIDEO_START( kc85_4 )
{
	kc85_common_vh_start();

    kc85_4_video_ram = auto_malloc(
        (KC85_4_SCREEN_COLOUR_RAM_SIZE*2) +
        (KC85_4_SCREEN_PIXEL_RAM_SIZE*2));

	kc85_4_display_video_ram = kc85_4_video_ram;

	return 0;
}

void kc85_4_video_ram_select_bank(int bank)
{
    /* calculate address of video ram to display */
    unsigned char *video_ram;

    video_ram = kc85_4_video_ram;

    if (bank!=0)
    {
		video_ram +=
				   (KC85_4_SCREEN_PIXEL_RAM_SIZE +
				   KC85_4_SCREEN_COLOUR_RAM_SIZE);
	}

    kc85_4_display_video_ram = video_ram;
}

unsigned char *kc85_4_get_video_ram_base(int bank, int colour)
{
    /* base address: screen 0 pixel data */
	unsigned char *addr = kc85_4_video_ram;

	if (bank!=0)
	{
		/* access screen 1 */
		addr += KC85_4_SCREEN_PIXEL_RAM_SIZE +
				KC85_4_SCREEN_COLOUR_RAM_SIZE;
	}

	if (colour!=0)
	{
		/* access colour information of selected screen */
		addr += KC85_4_SCREEN_PIXEL_RAM_SIZE;
	}

	return addr;
}

static void kc85_4_pixel_grab_callback(struct grab_info *grab_data,int x,int y, unsigned char *colour, unsigned char *gfx)
{
	int offset;

	offset = (y & 0xff) | ((x & 0x0ff)<<8);

	*colour = grab_data->colour_ram[offset];
	*gfx = grab_data->pixel_ram[offset];

}


/***************************************************************************
  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function,
  it will be called by the main emulation engine.
***************************************************************************/
VIDEO_UPDATE( kc85_4 )
{
#if 0
    unsigned char *pixel_ram = kc85_4_display_video_ram;
    unsigned char *colour_ram = pixel_ram + 0x04000;

    int x,y;

	for (y=0; y<KC85_SCREEN_HEIGHT; y++)
	{
		for (x=0; x<(KC85_SCREEN_WIDTH>>3); x++)
		{
			unsigned char colour_byte, gfx_byte;
			int offset;
			
			offset = y | (x<<8);

			colour_byte = colour_ram[offset];
		    gfx_byte = pixel_ram[offset];

			kc85_draw_8_pixels(bitmap,(x<<3),y, colour_byte, gfx_byte);
		
		}
	}
#endif
	struct grab_info grab_data;

	grab_data.pixel_ram = kc85_4_display_video_ram;
	grab_data.colour_ram = kc85_4_display_video_ram + 0x04000;

	kc85_common_process_frame(bitmap, kc85_4_pixel_grab_callback,&grab_data);

	return 0;
}

/***************************************************************************
 KC85/3 video
***************************************************************************/

VIDEO_START( kc85_3 )
{
	kc85_common_vh_start();
	return 0;
}


static void kc85_3_pixel_grab_callback(struct grab_info *grab_data,int x,int y, unsigned char *colour, unsigned char *gfx)
{
	int pixel_offset,colour_offset;

	/* this has got to be speeded up!!! */
	if ((x & 0x020)==0)
	{
		pixel_offset = (x & 0x01f) | (((y>>2) & 0x03)<<5) |
		((y & 0x03)<<7) | (((y>>4) & 0x0f)<<9);

		colour_offset = (x & 0x01f) | (((y>>2) & 0x03f)<<5);
	}
	else
	{
		/* 1  0  1  0  0  V7 V6 V1  V0 V3 V2 V5 V4 H2 H1 H0 */
		/* 1  0  1  1  0  0  0  V7  V6 V3 V2 V5 V4 H2 H1 H0 */

		pixel_offset = 0x02000+((x & 0x07) | (((y>>4) & 0x03)<<3) |
			(((y>>2) & 0x03)<<5) | ((y & 0x03)<<7) | ((y>>6) & 0x03)<<9);

		colour_offset = 0x0800+((x & 0x07) | (((y>>4) & 0x03)<<3) |
			(((y>>2) & 0x03)<<5) | ((y>>6) & 0x03)<<7);
	}

	*colour = grab_data->colour_ram[colour_offset];
	*gfx = grab_data->pixel_ram[pixel_offset];

}

/***************************************************************************
  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function,
  it will be called by the main emulation engine.
***************************************************************************/
VIDEO_UPDATE( kc85_3 )
{
#if 0
	/* colour ram takes up 0x02800 bytes */
	   unsigned char *pixel_ram = mess_ram+0x08000;	
    unsigned char *colour_ram = pixel_ram + 0x02800;

    int x,y;

	for (y=0; y<KC85_SCREEN_HEIGHT; y++)
	{
		for (x=0; x<(KC85_SCREEN_WIDTH>>3); x++)
		{
			unsigned char colour_byte, gfx_byte;
			int pixel_offset,colour_offset;
			
			if ((x & 0x020)==0)
			{
				pixel_offset = (x & 0x01f) | (((y>>2) & 0x03)<<5) |
				((y & 0x03)<<7) | (((y>>4) & 0x0f)<<9);
	
				colour_offset = (x & 0x01f) | (((y>>2) & 0x03f)<<5);
			}
			else
			{
				/* 1  0  1  0  0  V7 V6 V1  V0 V3 V2 V5 V4 H2 H1 H0 */
				/* 1  0  1  1  0  0  0  V7  V6 V3 V2 V5 V4 H2 H1 H0 */

				pixel_offset = 0x02000+((x & 0x07) | (((y>>4) & 0x03)<<3) |
					(((y>>2) & 0x03)<<5) | ((y & 0x03)<<7) | ((y>>6) & 0x03)<<9);
	
				colour_offset = 0x0800+((x & 0x07) | (((y>>4) & 0x03)<<3) |
					(((y>>2) & 0x03)<<5) | ((y>>6) & 0x03)<<7);
			}
		
            colour_byte = colour_ram[colour_offset];
            gfx_byte = pixel_ram[pixel_offset];

			kc85_draw_8_pixels(bitmap,(x<<3),y, colour_byte, gfx_byte);
		}
	}
#endif

	struct grab_info grab_data;

	grab_data.pixel_ram = mess_ram+0x08000;
	grab_data.colour_ram = mess_ram+0x08000 + 0x02800;

	kc85_common_process_frame(bitmap, kc85_3_pixel_grab_callback,&grab_data);

	return 0;
}
