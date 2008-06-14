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

INLINE void spectrum_plot_pixel(bitmap_t *bitmap, int x, int y, UINT32 color)
{
	*BITMAP_ADDR16(bitmap, y, x) = (UINT16)color;
}


VIDEO_START( spectrum_128 )
{
	frame_number = 0;
	flash_invert = 0;

	EventList_Initialise(30000);
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
                                        spectrum_plot_pixel(bitmap,scrx++,SPEC_TOP_BORDER+scry,ink);
                                else
                                        spectrum_plot_pixel(bitmap,scrx++,SPEC_TOP_BORDER+scry,pap);
			}
                scr++;
                attr++;
                }
	}

	draw_border(screen->machine, bitmap, full_refresh,
		SPEC_TOP_BORDER, SPEC_DISPLAY_YSIZE, SPEC_BOTTOM_BORDER,
		SPEC_LEFT_BORDER, SPEC_DISPLAY_XSIZE, SPEC_RIGHT_BORDER,
		SPEC_LEFT_BORDER_CYCLES, SPEC_DISPLAY_XSIZE_CYCLES,
		SPEC_RIGHT_BORDER_CYCLES, SPEC128_RETRACE_CYCLES, 200, 0xfe);
	return 0;
}



static const rgb_t spectrum_128_palette[16] = {
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

PALETTE_INIT( spectrum_128 )
{	
	palette_set_colors(machine, 0, spectrum_128_palette, ARRAY_LENGTH(spectrum_128_palette));
}

