/***************************************************************************

    Gottlieb Exterminator hardware

***************************************************************************/

#include "driver.h"
#include "cpu/tms34010/tms34010.h"
#include "exterm.h"


UINT16 *exterm_master_videoram, *exterm_slave_videoram;



/*************************************
 *
 *  Palette setup
 *
 *************************************/

PALETTE_INIT( exterm )
{
	int i;

	/* initialize 555 RGB lookup */
	for (i = 0; i < 32768; i++)
		palette_set_color(machine, i+4096, pal5bit(i >> 10), pal5bit(i >> 5), pal5bit(i >> 0));
}



/*************************************
 *
 *  Master shift register
 *
 *************************************/

void exterm_to_shiftreg_master(unsigned int address, UINT16 *shiftreg)
{
	memcpy(shiftreg, &exterm_master_videoram[TOWORD(address)], 256 * sizeof(UINT16));
}


void exterm_from_shiftreg_master(unsigned int address, UINT16 *shiftreg)
{
	memcpy(&exterm_master_videoram[TOWORD(address)], shiftreg, 256 * sizeof(UINT16));
}


void exterm_to_shiftreg_slave(unsigned int address, UINT16 *shiftreg)
{
	memcpy(shiftreg, &exterm_slave_videoram[TOWORD(address)], 256 * 2 * sizeof(UINT8));
}


void exterm_from_shiftreg_slave(unsigned int address, UINT16 *shiftreg)
{
	memcpy(&exterm_slave_videoram[TOWORD(address)], shiftreg, 256 * 2 * sizeof(UINT8));
}



/*************************************
 *
 *  Main video refresh
 *
 *************************************/

VIDEO_UPDATE( exterm )
{
	int x, y;

	/* if the display is blanked, fill with black */
	if (tms34010_io_display_blanked(0))
	{
		fillbitmap(bitmap, get_black_pen(machine), cliprect);
		return 0;
	}

	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT16 *bgsrc = &exterm_master_videoram[256 * (y - Machine->screen[0].visarea.min_y)];
		UINT16 *dest = BITMAP_ADDR16(bitmap, y, 0);

		/* on the top/bottom of the screen, it's all background */
		if ((y - Machine->screen[0].visarea.min_y) < 40 || (y - Machine->screen[0].visarea.min_y) > 238)
			for (x = 0; x < 256; x++)
			{
				UINT16 bgdata = *bgsrc++;
				dest[x] = (bgdata & 0x8000) ? (bgdata & 0xfff) : (bgdata + 0x1000);
			}

		/* elsewhere, we have to blend foreground and background */
		else
		{
			UINT16 *fgsrc = (tms34010_get_DPYSTRT(1) & 0x800) ? &exterm_slave_videoram[(y - Machine->screen[0].visarea.min_y) * 128] : &exterm_slave_videoram[(256 + y - Machine->screen[0].visarea.min_y) * 128];

			for (x = 0; x < 256; x += 2)
			{
				UINT16 fgdata = *fgsrc++;
				UINT16 bgdata;

				if (fgdata & 0x00ff)
					dest[x] = fgdata & 0x00ff;
				else
				{
					bgdata = bgsrc[0];
					dest[x] = (bgdata & 0x8000) ? (bgdata & 0xfff) : (bgdata + 0x1000);
				}

				if (fgdata & 0xff00)
					dest[x+1] = fgdata >> 8;
				else
				{
					bgdata = bgsrc[1];
					dest[x+1] = (bgdata & 0x8000) ? (bgdata & 0xfff) : (bgdata + 0x1000);
				}

				bgsrc += 2;
			}
		}
	}
	return 0;
}
