/***************************************************************************

  spectrum.c

  Functions to emulate the video hardware of the ZX Spectrum.

  Changes:

  DJR 08/02/00 - Added support for FLASH 1.
  DJR 16/05/00 - Support for TS2068/TC2048 hires and 64 column modes.
  DJR 19/05/00 - Speeded up Spectrum 128 screen refresh.
  DJR 23/05/00 - Preliminary support for border colour emulation.

***************************************************************************/

#include "driver.h"
#include "includes/spectrum.h"
#include "eventlst.h"
#include "video/border.h"


UINT8 *spectrum_characterram;
UINT8 *spectrum_colorram;

int frame_number;    /* Used for handling FLASH 1 */
int flash_invert;

/***************************************************************************
  Start the video hardware emulation.
***************************************************************************/
VIDEO_START( spectrum )
{
	frame_number = 0;
	flash_invert = 0;
	EventList_Initialise(30000);
}


/* return the color to be used inverting FLASHing colors if necessary */
INLINE unsigned char get_display_color (unsigned char color, int invert)
{
        if (invert && (color & 0x80))
                return (color & 0xc0) + ((color & 0x38) >> 3) + ((color & 0x07) << 3);
        else
                return color;
}

/* Code to change the FLASH status every 25 frames. Note this must be
   independent of frame skip etc. */
VIDEO_EOF( spectrum )
{
        EVENT_LIST_ITEM *pItem;
        int NumItems;

        frame_number++;
        if (frame_number >= 25)
        {
                frame_number = 0;
                flash_invert = !flash_invert;
        }

        /* Empty event buffer for undisplayed frames noting the last border
           colour (in case colours are not changed in the next frame). */
        NumItems = EventList_NumEvents();
        if (NumItems)
        {
                pItem = EventList_GetFirstItem();
                set_last_border_color ( pItem[NumItems-1].Event_Data );
                EventList_Reset();
				EventList_SetOffsetStartTime ( ATTOTIME_TO_CYCLES(0, attotime_mul(video_screen_get_scan_period(machine->primary_screen), video_screen_get_vpos(machine->primary_screen))) );
                logerror ("Event log reset in callback fn.\n");
        }
}



/***************************************************************************
  Update the spectrum screen display.

  The screen consists of 312 scanlines as follows:
  64  border lines (the last 48 are actual border lines; the others may be
                    border lines or vertical retrace)
  192 screen lines
  56  border lines

  Each screen line has 48 left border pixels, 256 screen pixels and 48 right
  border pixels.

  Each scanline takes 224 T-states divided as follows:
  128 Screen (reads a screen and ATTR byte [8 pixels] every 4 T states)
  24  Right border
  48  Horizontal retrace
  24  Left border

  The 128K Spectrums have only 63 scanlines before the TV picture (311 total)
  and take 228 T-states per scanline.

***************************************************************************/

VIDEO_UPDATE( spectrum )
{
	int count;
	int full_refresh = 1;
    static int last_invert = 0;

	if (full_refresh)
	{
		last_invert = flash_invert;
	}
	else
	{
		/* Update all flashing characters when necessary */
		if (last_invert != flash_invert)
		{
			last_invert = flash_invert;
		}
	}

    for (count=0;count<32*8;count++)
    {
			decodechar( screen->machine->gfx[0],count,spectrum_characterram);
			decodechar( screen->machine->gfx[1],count,&spectrum_characterram[0x800]);
			decodechar( screen->machine->gfx[2],count,&spectrum_characterram[0x1000]);
	}

    for (count=0;count<32*8;count++)
    {
		int sx=count%32;
		int sy=count/32;
		unsigned char color;

        color=get_display_color(spectrum_colorram[count],flash_invert);
		drawgfx(bitmap,screen->machine->gfx[0],
			count,
			color+8, // use 2nd part of palette
			0,0,
        	(sx*8)+SPEC_LEFT_BORDER,(sy*8)+SPEC_TOP_BORDER,
			0,TRANSPARENCY_NONE,0);

        color=get_display_color(spectrum_colorram[count+0x100],flash_invert);
		drawgfx(bitmap,screen->machine->gfx[1],
			count,
			color+8, // use 2nd part of palette
			0,0,
			(sx*8)+SPEC_LEFT_BORDER,((sy+8)*8)+SPEC_TOP_BORDER,
			0,TRANSPARENCY_NONE,0);
            
        color=get_display_color(spectrum_colorram[count+0x200],flash_invert);
		drawgfx(bitmap,screen->machine->gfx[2],
			count,
			color+8, // use 2nd part of palette
			0,0,
			(sx*8)+SPEC_LEFT_BORDER,((sy+16)*8)+SPEC_TOP_BORDER,
			0,TRANSPARENCY_NONE,0);
	}

    /* When screen refresh is called there is only one blank line
        (synchronised with start of screen data) before the border lines.
        There should be 16 blank lines after an interrupt is called.
    */
    draw_border(screen->machine, bitmap, full_refresh,
            SPEC_TOP_BORDER, SPEC_DISPLAY_YSIZE, SPEC_BOTTOM_BORDER,
            SPEC_LEFT_BORDER, SPEC_DISPLAY_XSIZE, SPEC_RIGHT_BORDER,
            SPEC_LEFT_BORDER_CYCLES, SPEC_DISPLAY_XSIZE_CYCLES,
            SPEC_RIGHT_BORDER_CYCLES, SPEC_RETRACE_CYCLES, 200, 0xfe);
	return 0;
}

const gfx_layout spectrum_charlayout =
{
	8,8,
	256,
	1,						/* 1 bits per pixel */

	{ 0 },					/* no bitplanes; 1 bit per pixel */

	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0, 8*256, 16*256, 24*256, 32*256, 40*256, 48*256, 56*256 },

	8				/* every char takes 1 consecutive byte */
};

#define ZX_COL_0	MAKE_RGB(0x00, 0x00, 0x00)
#define ZX_COL_1	MAKE_RGB(0x00, 0x00, 0xbf)
#define ZX_COL_2	MAKE_RGB(0xbf, 0x00, 0x00) 
#define ZX_COL_3	MAKE_RGB(0xbf, 0x00, 0xbf)
#define ZX_COL_4	MAKE_RGB(0x00, 0xbf, 0x00) 
#define ZX_COL_5	MAKE_RGB(0x00, 0xbf, 0xbf)
#define ZX_COL_6	MAKE_RGB(0xbf, 0xbf, 0x00) 
#define ZX_COL_7	MAKE_RGB(0xbf, 0xbf, 0xbf)
#define ZX_COL_8	MAKE_RGB(0x00, 0x00, 0x00)
#define ZX_COL_9	MAKE_RGB(0x00, 0x00, 0xff)
#define ZX_COL_A	MAKE_RGB(0xff, 0x00, 0x00)
#define ZX_COL_B	MAKE_RGB(0xff, 0x00, 0xff)
#define ZX_COL_C	MAKE_RGB(0x00, 0xff, 0x00)
#define ZX_COL_D	MAKE_RGB(0x00, 0xff, 0xff)
#define ZX_COL_E	MAKE_RGB(0xff, 0xff, 0x00)
#define ZX_COL_F	MAKE_RGB(0xff, 0xff, 0xff)

const rgb_t spectrum_palette[256 + 16] = {
	ZX_COL_0,ZX_COL_1,ZX_COL_2,ZX_COL_3,ZX_COL_4,ZX_COL_5,ZX_COL_6,ZX_COL_7,ZX_COL_8,ZX_COL_9,ZX_COL_A,ZX_COL_B,ZX_COL_C,ZX_COL_D,ZX_COL_E,ZX_COL_F,
	
	ZX_COL_0,ZX_COL_0,ZX_COL_0,ZX_COL_1,ZX_COL_0,ZX_COL_2,ZX_COL_0,ZX_COL_3,ZX_COL_0,ZX_COL_4,ZX_COL_0,ZX_COL_5,ZX_COL_0,ZX_COL_6,ZX_COL_0,ZX_COL_7,
	ZX_COL_1,ZX_COL_0,ZX_COL_1,ZX_COL_1,ZX_COL_1,ZX_COL_2,ZX_COL_1,ZX_COL_3,ZX_COL_1,ZX_COL_4,ZX_COL_1,ZX_COL_5,ZX_COL_1,ZX_COL_6,ZX_COL_1,ZX_COL_7,
	ZX_COL_2,ZX_COL_0,ZX_COL_2,ZX_COL_1,ZX_COL_2,ZX_COL_2,ZX_COL_2,ZX_COL_3,ZX_COL_2,ZX_COL_4,ZX_COL_2,ZX_COL_5,ZX_COL_2,ZX_COL_6,ZX_COL_2,ZX_COL_7,	
	ZX_COL_3,ZX_COL_0,ZX_COL_3,ZX_COL_1,ZX_COL_3,ZX_COL_2,ZX_COL_3,ZX_COL_3,ZX_COL_3,ZX_COL_4,ZX_COL_3,ZX_COL_5,ZX_COL_3,ZX_COL_6,ZX_COL_3,ZX_COL_7,	
	ZX_COL_4,ZX_COL_0,ZX_COL_4,ZX_COL_1,ZX_COL_4,ZX_COL_2,ZX_COL_4,ZX_COL_3,ZX_COL_4,ZX_COL_4,ZX_COL_4,ZX_COL_5,ZX_COL_4,ZX_COL_6,ZX_COL_4,ZX_COL_7,	
	ZX_COL_5,ZX_COL_0,ZX_COL_5,ZX_COL_1,ZX_COL_5,ZX_COL_2,ZX_COL_5,ZX_COL_3,ZX_COL_5,ZX_COL_4,ZX_COL_5,ZX_COL_5,ZX_COL_5,ZX_COL_6,ZX_COL_5,ZX_COL_7,
	ZX_COL_6,ZX_COL_0,ZX_COL_6,ZX_COL_1,ZX_COL_6,ZX_COL_2,ZX_COL_6,ZX_COL_3,ZX_COL_6,ZX_COL_4,ZX_COL_6,ZX_COL_5,ZX_COL_6,ZX_COL_6,ZX_COL_6,ZX_COL_7,
	ZX_COL_7,ZX_COL_0,ZX_COL_7,ZX_COL_1,ZX_COL_7,ZX_COL_2,ZX_COL_7,ZX_COL_3,ZX_COL_7,ZX_COL_4,ZX_COL_7,ZX_COL_5,ZX_COL_7,ZX_COL_6,ZX_COL_7,ZX_COL_7,	
	ZX_COL_8,ZX_COL_8,ZX_COL_8,ZX_COL_9,ZX_COL_8,ZX_COL_A,ZX_COL_8,ZX_COL_B,ZX_COL_8,ZX_COL_C,ZX_COL_8,ZX_COL_D,ZX_COL_8,ZX_COL_E,ZX_COL_8,ZX_COL_F,
	ZX_COL_9,ZX_COL_8,ZX_COL_9,ZX_COL_9,ZX_COL_9,ZX_COL_A,ZX_COL_9,ZX_COL_B,ZX_COL_9,ZX_COL_C,ZX_COL_9,ZX_COL_D,ZX_COL_9,ZX_COL_E,ZX_COL_9,ZX_COL_F,
	ZX_COL_A,ZX_COL_8,ZX_COL_A,ZX_COL_9,ZX_COL_A,ZX_COL_A,ZX_COL_A,ZX_COL_B,ZX_COL_A,ZX_COL_C,ZX_COL_A,ZX_COL_D,ZX_COL_A,ZX_COL_E,ZX_COL_A,ZX_COL_F,	
	ZX_COL_B,ZX_COL_8,ZX_COL_B,ZX_COL_9,ZX_COL_B,ZX_COL_A,ZX_COL_B,ZX_COL_B,ZX_COL_B,ZX_COL_C,ZX_COL_B,ZX_COL_D,ZX_COL_B,ZX_COL_E,ZX_COL_B,ZX_COL_F,	
	ZX_COL_C,ZX_COL_8,ZX_COL_C,ZX_COL_9,ZX_COL_C,ZX_COL_A,ZX_COL_C,ZX_COL_B,ZX_COL_C,ZX_COL_C,ZX_COL_C,ZX_COL_D,ZX_COL_C,ZX_COL_E,ZX_COL_C,ZX_COL_F,	
	ZX_COL_D,ZX_COL_8,ZX_COL_D,ZX_COL_9,ZX_COL_D,ZX_COL_A,ZX_COL_D,ZX_COL_B,ZX_COL_D,ZX_COL_C,ZX_COL_D,ZX_COL_D,ZX_COL_D,ZX_COL_E,ZX_COL_D,ZX_COL_F,
	ZX_COL_E,ZX_COL_8,ZX_COL_E,ZX_COL_9,ZX_COL_E,ZX_COL_A,ZX_COL_E,ZX_COL_B,ZX_COL_E,ZX_COL_C,ZX_COL_E,ZX_COL_D,ZX_COL_E,ZX_COL_E,ZX_COL_E,ZX_COL_F,
	ZX_COL_F,ZX_COL_8,ZX_COL_F,ZX_COL_9,ZX_COL_F,ZX_COL_A,ZX_COL_F,ZX_COL_B,ZX_COL_F,ZX_COL_C,ZX_COL_F,ZX_COL_D,ZX_COL_F,ZX_COL_E,ZX_COL_F,ZX_COL_F
	};


/* Initialise the palette */
PALETTE_INIT( spectrum )
{
	palette_set_colors(machine, 0, spectrum_palette, ARRAY_LENGTH(spectrum_palette));
}
