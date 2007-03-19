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
#include "video/generic.h"
#include "includes/spectrum.h"
#include "eventlst.h"
#include "video/border.h"

unsigned char *spectrum_characterram;
unsigned char *spectrum_colorram;
unsigned char *charsdirty;
static int frame_number;    /* Used for handling FLASH 1 */
static int flash_invert;

/***************************************************************************
  Start the video hardware emulation.
***************************************************************************/
VIDEO_START( spectrum )
{
	frame_number = 0;
	flash_invert = 0;
	spectrum_characterram = auto_malloc(0x1800);

	spectrum_colorram = auto_malloc(0x300);

	charsdirty = auto_malloc(0x300);

	memset(charsdirty,1,0x300);
	EventList_Initialise(30000);
	return 0;
}

/* screen is stored as:
32 chars wide. first 0x100 bytes are top scan of lines 0 to 7 */

WRITE8_HANDLER (spectrum_characterram_w)
{
	spectrum_characterram[offset] = data;

	charsdirty[((offset & 0x0f800)>>3) + (offset & 0x0ff)] = 1;
}

 READ8_HANDLER (spectrum_characterram_r)
{
	return(spectrum_characterram[offset]);
}


WRITE8_HANDLER (spectrum_colorram_w)
{
        /* Will eventually be used to emulate hi-res colour effects. No point
           doing it now as contented memory is not emulated so timings will
           be way off. (eg Zynaps taking 212 cycles not 224 per scanline)
        */
/*        EventList_AddItemOffset(offset+0x5800, data, cpu_getcurrentcycles()); */

	spectrum_colorram[offset] = data;
	charsdirty[offset] = 1;
}

 READ8_HANDLER (spectrum_colorram_r)
{
	return(spectrum_colorram[offset]);
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
                EventList_SetOffsetStartTime ( TIME_TO_CYCLES(0,cpu_getscanline()*cpu_getscanlineperiod()) );
                logerror ("Event log reset in callback fn.\n");
        }
}


/* Update FLASH status for ts2068. Assumes flash update every 1/2s. */
VIDEO_EOF( ts2068 )
{
        EVENT_LIST_ITEM *pItem;
        int NumItems;

        frame_number++;
        if (frame_number >= 30)
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
                EventList_SetOffsetStartTime ( TIME_TO_CYCLES(0,cpu_getscanline()*cpu_getscanlineperiod()) );
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
		memset(charsdirty,1,0x300);
		last_invert = flash_invert;
	}
	else
	{
		/* Update all flashing characters when necessary */
		if (last_invert != flash_invert)
		{
			for (count=0;count<0x300;count++)
				if (spectrum_colorram[count] & 0x80)
					charsdirty[count] = 1;
			last_invert = flash_invert;
		}
	}

    for (count=0;count<32*8;count++)
    {
		if (charsdirty[count]) {
			decodechar( Machine->gfx[0],count,spectrum_characterram,
					Machine->drv->gfxdecodeinfo[0].gfxlayout );
		}

		if (charsdirty[count+256]) {
			decodechar( Machine->gfx[1],count,&spectrum_characterram[0x800],
					Machine->drv->gfxdecodeinfo[0].gfxlayout );
		}

		if (charsdirty[count+512]) {
			decodechar( Machine->gfx[2],count,&spectrum_characterram[0x1000],
					Machine->drv->gfxdecodeinfo[0].gfxlayout );
		}
	}

    for (count=0;count<32*8;count++)
    {
	int sx=count%32;
	int sy=count/32;
	unsigned char color;

            if (charsdirty[count]) {
                    color=get_display_color(spectrum_colorram[count],
                                            flash_invert);
		drawgfx(bitmap,Machine->gfx[0],
			count,
			color,
			0,0,
                            (sx*8)+SPEC_LEFT_BORDER,(sy*8)+SPEC_TOP_BORDER,
			0,TRANSPARENCY_NONE,0);
		charsdirty[count] = 0;
	}

	if (charsdirty[count+256]) {
                    color=get_display_color(spectrum_colorram[count+0x100],
                                            flash_invert);
		drawgfx(bitmap,Machine->gfx[1],
			count,
			color,
			0,0,
                            (sx*8)+SPEC_LEFT_BORDER,((sy+8)*8)+SPEC_TOP_BORDER,
			0,TRANSPARENCY_NONE,0);
		charsdirty[count+256] = 0;
	}

	if (charsdirty[count+512]) {
                    color=get_display_color(spectrum_colorram[count+0x200],
                                            flash_invert);
		drawgfx(bitmap,Machine->gfx[2],
			count,
			color,
			0,0,
                            (sx*8)+SPEC_LEFT_BORDER,((sy+16)*8)+SPEC_TOP_BORDER,
			0,TRANSPARENCY_NONE,0);
		charsdirty[count+512] = 0;
	}
}

    /* When screen refresh is called there is only one blank line
        (synchronised with start of screen data) before the border lines.
        There should be 16 blank lines after an interrupt is called.
    */
    draw_border(bitmap, full_refresh,
            SPEC_TOP_BORDER, SPEC_DISPLAY_YSIZE, SPEC_BOTTOM_BORDER,
            SPEC_LEFT_BORDER, SPEC_DISPLAY_XSIZE, SPEC_RIGHT_BORDER,
            SPEC_LEFT_BORDER_CYCLES, SPEC_DISPLAY_XSIZE_CYCLES,
            SPEC_RIGHT_BORDER_CYCLES, SPEC_RETRACE_CYCLES, 200, 0xfe);
	return 0;
}

VIDEO_START( spectrum_128 )
{
	frame_number = 0;
	flash_invert = 0;

	EventList_Initialise(30000);

	return 0;
}

/* Refresh the spectrum 128 screen (code modified from COUPE.C) */
VIDEO_UPDATE( spectrum_128 )
{
        /* for now do a full-refresh */
        int x, y, b, scrx, scry;
        unsigned short ink, pap;
        unsigned char *attr, *scr;
		int full_refresh = 1;

        scr=spectrum_128_screen_location;

        for (y=0; y<192; y++)
        {
                scrx=SPEC_LEFT_BORDER;
                scry=((y&7) * 8) + ((y&0x38)>>3) + (y&0xC0);
                attr=spectrum_128_screen_location + ((scry>>3)*32) + 0x1800;

                for (x=0;x<32;x++)
                {
                        /* Get ink and paper colour with bright */
                        if (flash_invert && (*attr & 0x80))
                        {
                                ink=((*attr)>>3) & 0x0f;
                                pap=((*attr) & 0x07) + (((*attr)>>3) & 0x08);
                        }
                        else
                        {
                                ink=((*attr) & 0x07) + (((*attr)>>3) & 0x08);
                                pap=((*attr)>>3) & 0x0f;
                        }

                        for (b=0x80;b!=0;b>>=1)
                        {
                                if (*scr&b)
                                        plot_pixel(bitmap,scrx++,SPEC_TOP_BORDER+scry,Machine->pens[ink]);
                                else
                                        plot_pixel(bitmap,scrx++,SPEC_TOP_BORDER+scry,Machine->pens[pap]);
			}
                scr++;
                attr++;
                }
	}

	draw_border(bitmap, full_refresh,
		SPEC_TOP_BORDER, SPEC_DISPLAY_YSIZE, SPEC_BOTTOM_BORDER,
		SPEC_LEFT_BORDER, SPEC_DISPLAY_XSIZE, SPEC_RIGHT_BORDER,
		SPEC_LEFT_BORDER_CYCLES, SPEC_DISPLAY_XSIZE_CYCLES,
		SPEC_RIGHT_BORDER_CYCLES, SPEC128_RETRACE_CYCLES, 200, 0xfe);
	return 0;
}

/*******************************************************************
 *
 *      Update the TS2068 display.
 *
 *      Port ff is used to set the display mode.
 *
 *      bits 2..0  Video Mode Select
 *      000 = Primary DFILE active   (at 0x4000-0x5aff)
 *      001 = Secondary DFILE active (at 0x6000-0x7aff)
 *      010 = Extended Colour Mode   (chars at 0x4000-0x57ff, colors 0x6000-0x7aff)
 *      110 = 64 column mode         (columns 0,2,4,...62 from DFILE 1
 *                                    columns 1,3,5,...63 from DFILE 2)
 *      other = unpredictable results
 *
 *      bits 5..3  64 column mode ink/paper selection (attribute value in brackets)
 *      000 = Black/White   (56)        100 = Green/Magenta (28)
 *      001 = Blue/Yellow   (49)        101 = Cyan/Red      (21)
 *      010 = Red/Cyan      (42)        110 = Yellow/Blue   (14)
 *      011 = Magenta/Green (35)        111 = White/Black   (7)
 *
 *******************************************************************/

/* Draw a scanline in TS2068/TC2048 hires mode (code modified from COUPE.C) */
static void ts2068_hires_scanline(mame_bitmap *bitmap, int y, int borderlines)
{
	int x,b,scrx,scry;
	unsigned short ink,pap;
        unsigned char *attr, *scr;

        scrx=TS2068_LEFT_BORDER;
	scry=((y&7) * 8) + ((y&0x38)>>3) + (y&0xC0);

        scr=mess_ram + y*32;
        attr=scr + 0x2000;

        for (x=0;x<32;x++)
	{
                /* Get ink and paper colour with bright */
                if (flash_invert && (*attr & 0x80))
                {
                        ink=((*attr)>>3) & 0x0f;
                        pap=((*attr) & 0x07) + (((*attr)>>3) & 0x08);
                }
                else
                {
                        ink=((*attr) & 0x07) + (((*attr)>>3) & 0x08);
                        pap=((*attr)>>3) & 0x0f;
                }

		for (b=0x80;b!=0;b>>=1)
		{
                        if (*scr&b)
			{
                                plot_pixel(bitmap,scrx++,scry+borderlines,Machine->pens[ink]);
                                plot_pixel(bitmap,scrx++,scry+borderlines,Machine->pens[ink]);
			}
			else
			{
                                plot_pixel(bitmap,scrx++,scry+borderlines,Machine->pens[pap]);
                                plot_pixel(bitmap,scrx++,scry+borderlines,Machine->pens[pap]);
			}
		}
                scr++;
                attr++;
	}
}

/* Draw a scanline in TS2068/TC2048 64-column mode */
static void ts2068_64col_scanline(mame_bitmap *bitmap, int y, int borderlines, unsigned short inkcolor)
{
	int x,b,scrx,scry;
        unsigned char *scr1, *scr2;

        scrx=TS2068_LEFT_BORDER;
	scry=((y&7) * 8) + ((y&0x38)>>3) + (y&0xC0);

        scr1=mess_ram + y*32;
        scr2=scr1 + 0x2000;

        for (x=0;x<32;x++)
	{
		for (b=0x80;b!=0;b>>=1)
		{
                        if (*scr1&b)
                                plot_pixel(bitmap,scrx++,scry+borderlines,Machine->pens[inkcolor]);
			else
                                plot_pixel(bitmap,scrx++,scry+borderlines,Machine->pens[7-inkcolor]);
		}
                scr1++;

		for (b=0x80;b!=0;b>>=1)
		{
                        if (*scr2&b)
                                plot_pixel(bitmap,scrx++,scry+borderlines,Machine->pens[inkcolor]);
			else
                                plot_pixel(bitmap,scrx++,scry+borderlines,Machine->pens[7-inkcolor]);
		}
                scr2++;
	}
}

/* Draw a scanline in TS2068/TC2048 lores (normal Spectrum) mode */
static void ts2068_lores_scanline(mame_bitmap *bitmap, int y, int borderlines, int screen)
{
	int x,b,scrx,scry;
	unsigned short ink,pap;
	unsigned char *attr, *scr;

	scrx=TS2068_LEFT_BORDER;
	scry=((y&7) * 8) + ((y&0x38)>>3) + (y&0xC0);

	scr = mess_ram + y*32 + screen*0x2000;
	attr = mess_ram + ((scry>>3)*32) + screen*0x2000 + 0x1800;

	for (x=0;x<32;x++)
	{
		/* Get ink and paper colour with bright */
		if (flash_invert && (*attr & 0x80))
		{
			ink=((*attr)>>3) & 0x0f;
			pap=((*attr) & 0x07) + (((*attr)>>3) & 0x08);
		}
		else
		{
			ink=((*attr) & 0x07) + (((*attr)>>3) & 0x08);
			pap=((*attr)>>3) & 0x0f;
		}

		for (b=0x80;b!=0;b>>=1)
		{
			if (*scr&b)
			{
				plot_pixel(bitmap,scrx++,scry+borderlines,Machine->pens[ink]);
				plot_pixel(bitmap,scrx++,scry+borderlines,Machine->pens[ink]);
			}
			else
			{
				plot_pixel(bitmap,scrx++,scry+borderlines,Machine->pens[pap]);
				plot_pixel(bitmap,scrx++,scry+borderlines,Machine->pens[pap]);
			}
		}
		scr++;
		attr++;
	}
}

VIDEO_UPDATE( ts2068 )
{
	/* for now TS2068 will do a full-refresh */
	int count;
	int full_refresh = 1;

        if ((ts2068_port_ff_data & 7) == 6)
        {
                /* 64 Column mode */
                unsigned short inkcolor = (ts2068_port_ff_data & 0x38) >> 3;
                for (count = 0; count < 192; count++)
                        ts2068_64col_scanline(bitmap, count, TS2068_TOP_BORDER, inkcolor);
	}
        else if ((ts2068_port_ff_data & 7) == 2)
        {
                /* Extended Color mode */
                for (count = 0; count < 192; count++)
                        ts2068_hires_scanline(bitmap, count, TS2068_TOP_BORDER);
        }
        else if ((ts2068_port_ff_data & 7) == 1)
        {
                /* Screen 6000-7aff */
                for (count = 0; count < 192; count++)
                        ts2068_lores_scanline(bitmap, count, TS2068_TOP_BORDER, 1);
        }
        else
        {
                /* Screen 4000-5aff */
                for (count = 0; count < 192; count++)
                        ts2068_lores_scanline(bitmap, count, TS2068_TOP_BORDER, 0);
        }

        draw_border(bitmap, full_refresh,
                TS2068_TOP_BORDER, SPEC_DISPLAY_YSIZE, TS2068_BOTTOM_BORDER,
                TS2068_LEFT_BORDER, TS2068_DISPLAY_XSIZE, TS2068_RIGHT_BORDER,
                SPEC_LEFT_BORDER_CYCLES, SPEC_DISPLAY_XSIZE_CYCLES,
                SPEC_RIGHT_BORDER_CYCLES, SPEC_RETRACE_CYCLES, 200, 0xfe);
	return 0;
}

VIDEO_UPDATE( tc2048 )
{
	/* for now TS2068 will do a full-refresh */
	int count;
	int full_refresh = 1;

	if ((ts2068_port_ff_data & 7) == 6)
	{
		/* 64 Column mode */
		unsigned short inkcolor = (ts2068_port_ff_data & 0x38) >> 3;
		for (count = 0; count < 192; count++)
			ts2068_64col_scanline(bitmap, count, SPEC_TOP_BORDER, inkcolor);
	}
	else if ((ts2068_port_ff_data & 7) == 2)
	{
		/* Extended Color mode */
		for (count = 0; count < 192; count++)
			ts2068_hires_scanline(bitmap, count, SPEC_TOP_BORDER);
	}
	else if ((ts2068_port_ff_data & 7) == 1)
	{
		/* Screen 6000-7aff */
		for (count = 0; count < 192; count++)
			ts2068_lores_scanline(bitmap, count, SPEC_TOP_BORDER, 1);
	}
	else
	{
		/* Screen 4000-5aff */
		for (count = 0; count < 192; count++)
			ts2068_lores_scanline(bitmap, count, SPEC_TOP_BORDER, 0);
	}

	draw_border(bitmap, full_refresh,
		SPEC_TOP_BORDER, SPEC_DISPLAY_YSIZE, SPEC_BOTTOM_BORDER,
		TS2068_LEFT_BORDER, TS2068_DISPLAY_XSIZE, TS2068_RIGHT_BORDER,
		SPEC_LEFT_BORDER_CYCLES, SPEC_DISPLAY_XSIZE_CYCLES,
		SPEC_RIGHT_BORDER_CYCLES, SPEC_RETRACE_CYCLES, 200, 0xfe);
	return 0;
}
