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


unsigned char *spectrum_video_ram;
static UINT8 retrace_cycles;
int spectrum_frame_number;    /* Used for handling FLASH 1 */
int spectrum_flash_invert;

/***************************************************************************
  Start the video hardware emulation.
***************************************************************************/
VIDEO_START( spectrum )
{
	spectrum_frame_number = 0;
	spectrum_flash_invert = 0;

	EventList_Initialise(machine, 30000);

	retrace_cycles = SPEC_RETRACE_CYCLES;

	spectrum_screen_location = spectrum_video_ram;
}

VIDEO_START( spectrum_128 )
{
	spectrum_frame_number = 0;
	spectrum_flash_invert = 0;

	EventList_Initialise(machine, 30000);

	retrace_cycles = SPEC128_RETRACE_CYCLES;
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

        spectrum_frame_number++;
        if (spectrum_frame_number >= 25)
        {
                spectrum_frame_number = 0;
                spectrum_flash_invert = !spectrum_flash_invert;
        }

        /* Empty event buffer for undisplayed frames noting the last border
           colour (in case colours are not changed in the next frame). */
        NumItems = EventList_NumEvents();
        if (NumItems)
        {
                pItem = EventList_GetFirstItem();
                border_set_last_color ( pItem[NumItems-1].Event_Data );
                EventList_Reset();
				EventList_SetOffsetStartTime ( cpu_attotime_to_clocks(machine->firstcpu, attotime_mul(video_screen_get_scan_period(machine->primary_screen), video_screen_get_vpos(machine->primary_screen))) );
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

INLINE void spectrum_plot_pixel(bitmap_t *bitmap, int x, int y, UINT32 color)
{
	*BITMAP_ADDR16(bitmap, y, x) = (UINT16)color;
}

VIDEO_UPDATE( spectrum )
{
        /* for now do a full-refresh */
        int x, y, b, scrx, scry;
        unsigned short ink, pap;
        unsigned char *attr, *scr;
		int full_refresh = 1;

        scr=spectrum_screen_location;

        for (y=0; y<192; y++)
        {
                scrx=SPEC_LEFT_BORDER;
                scry=((y&7) * 8) + ((y&0x38)>>3) + (y&0xC0);
                attr=spectrum_screen_location + ((scry>>3)*32) + 0x1800;

                for (x=0;x<32;x++)
                {
                        /* Get ink and paper colour with bright */
                        if (spectrum_flash_invert && (*attr & 0x80))
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
                                        spectrum_plot_pixel(bitmap,scrx++,SPEC_TOP_BORDER+scry,ink);
                                else
                                        spectrum_plot_pixel(bitmap,scrx++,SPEC_TOP_BORDER+scry,pap);
			}
                scr++;
                attr++;
                }
	}

	border_draw(screen->machine, bitmap, full_refresh,
		SPEC_TOP_BORDER, SPEC_DISPLAY_YSIZE, SPEC_BOTTOM_BORDER,
		SPEC_LEFT_BORDER, SPEC_DISPLAY_XSIZE, SPEC_RIGHT_BORDER,
		SPEC_LEFT_BORDER_CYCLES, SPEC_DISPLAY_XSIZE_CYCLES,
		SPEC_RIGHT_BORDER_CYCLES, retrace_cycles, 200, 0xfe);
	return 0;
}


static const rgb_t spectrum_palette[16] = {
	MAKE_RGB(0x00, 0x00, 0x00),
	MAKE_RGB(0x00, 0x00, 0xbf),
	MAKE_RGB(0xbf, 0x00, 0x00),
	MAKE_RGB(0xbf, 0x00, 0xbf),
	MAKE_RGB(0x00, 0xbf, 0x00),
	MAKE_RGB(0x00, 0xbf, 0xbf),
	MAKE_RGB(0xbf, 0xbf, 0x00),
	MAKE_RGB(0xbf, 0xbf, 0xbf),
	MAKE_RGB(0x00, 0x00, 0x00),
	MAKE_RGB(0x00, 0x00, 0xff),
	MAKE_RGB(0xff, 0x00, 0x00),
	MAKE_RGB(0xff, 0x00, 0xff),
	MAKE_RGB(0x00, 0xff, 0x00),
	MAKE_RGB(0x00, 0xff, 0xff),
	MAKE_RGB(0xff, 0xff, 0x00),
	MAKE_RGB(0xff, 0xff, 0xff)
};
/* Initialise the palette */
PALETTE_INIT( spectrum )
{
	palette_set_colors(machine, 0, spectrum_palette, ARRAY_LENGTH(spectrum_palette));
}
