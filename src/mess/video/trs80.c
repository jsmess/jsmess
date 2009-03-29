/***************************************************************************

  trs80.c

  Functions to emulate the video hardware of the TRS80.

***************************************************************************/
#include "driver.h"
#include "includes/trs80.h"

#define FW  TRS80_FONT_W
#define FH  TRS80_FONT_H

/* Bit assignment for "trs80_mode"
	d7 Page select
	d3 Invert characters with bit 7 set (1=invert)
	d2 80/40 or 64/32 characters per line (1=80)
	d0 80/64 or 40/32 characters per line (1=32) */

static UINT16 start_address=0;
static UINT8 crtc_reg;

WRITE8_HANDLER( trs80m4_88_w )
{
/* This is for the programming of the CRTC registers.
	However this CRTC is mask-programmed, and only the
	start address register can be used. The cursor and
	light-pen facilities are ignored. The character clock
	is changed depending on the screen size chosen.
	It is assumed that character size of 6x12 is used in
	all screen sizes, however decapping is needed to be
	certain. Therefore it is easier to use normal
	coding rather than the mc6845 device. */

	if (!offset) crtc_reg = data & 0x1f;

	if (offset) switch (crtc_reg)
	{
		case 12:
			start_address = (start_address & 0x00ff) | (data << 8);
			break;
		case 13:
			start_address = (start_address & 0xff00) | data;
	}
}


/* 7 or 8-bit video, 32/64 characters per line = trs80, trs80l2, sys80 */
VIDEO_UPDATE( trs80 )
{
	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,ma=0,x;
	UINT8 *FNT = memory_region(screen->machine, "gfx1");
	UINT8 seven_bit = screen->machine->gamedrv->flags >> 24;
	static UINT8 size_store=0;
	static UINT8 cols=64,skip=1;
	UINT8 lores1,lores2;

	if ((trs80_mode & 0x7f) != size_store)
	{
		size_store = trs80_mode & 0x7f;
		cols = (trs80_mode & 1) ? 32 : 64;
		skip = (trs80_mode & 1) ? 2 : 1;
		video_screen_set_visarea(screen, 0, cols*FW-1, 0, 16*FH-1);
	}

	for (y = 0; y < 16; y++)
	{
		for (ra = 0; ra < FH; ra++)
		{
			UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);

			for (x = ma; x < ma + 64; x+=skip)
			{
				chr = videoram[x];

				if (chr & 0x80)
				{
					lores1 = 1<<((ra & 0x0c)>>1);
					lores2 = lores1<<1; 
					/* Display one line of a lores character (6 pixels) */
					*p = ( chr & lores1 ) ? 1 : 0; p++;
					*p = ( chr & lores1 ) ? 1 : 0; p++;
					*p = ( chr & lores1 ) ? 1 : 0; p++;
					*p = ( chr & lores2 ) ? 1 : 0; p++;
					*p = ( chr & lores2 ) ? 1 : 0; p++;
					*p = ( chr & lores2 ) ? 1 : 0; p++;
				}
				else
				{
					if ((seven_bit) && (chr < 32)) chr+=64;

					/* get pattern of pixels for that character scanline */
					if (ra < 8)
						gfx = FNT[(chr<<3) | ra ];
					else
						gfx = 0;

					/* Display a scanline of a character (6 pixels) */
					*p = ( gfx & 0x20 ) ? 1 : 0; p++;
					*p = ( gfx & 0x10 ) ? 1 : 0; p++;
					*p = ( gfx & 0x08 ) ? 1 : 0; p++;
					*p = ( gfx & 0x04 ) ? 1 : 0; p++;
					*p = ( gfx & 0x02 ) ? 1 : 0; p++;
					*p = ( gfx & 0x01 ) ? 1 : 0; p++;
				}
			}
		}
		ma+=64;
	}
	return 0;
}


/* 7 or 8-bit video, 64/32 characters per line = ht1080z, ht1080z2, ht108064 */		
VIDEO_UPDATE( ht1080z )
{
	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,ma=0,x;
	UINT8 *FNT = memory_region(screen->machine, "gfx1");
	UINT8 seven_bit = screen->machine->gamedrv->flags >> 24;
	static UINT8 size_store=0;
	static UINT8 cols=64,skip=1;
	UINT8 lores1,lores2;

	if ((trs80_mode & 0x7f) != size_store)
	{
		size_store = trs80_mode & 0x7f;
		cols = (trs80_mode & 1) ? 32 : 64;
		skip = (trs80_mode & 1) ? 2 : 1;
		video_screen_set_visarea(screen, 0, cols*FW-1, 0, 16*FH-1);
	}

	for (y = 0; y < 16; y++)
	{
		for (ra = 0; ra < FH; ra++)
		{
			UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);

			for (x = ma; x < ma + 64; x+=skip)
			{
				chr = videoram[x];

				if (chr & 0x80)
				{
					lores1 = 1<<((ra & 0x0c)>>1);
					lores2 = lores1<<1; 
					/* Display one line of a lores character (6 pixels) */
					*p = ( chr & lores1 ) ? 1 : 0; p++;
					*p = ( chr & lores1 ) ? 1 : 0; p++;
					*p = ( chr & lores1 ) ? 1 : 0; p++;
					*p = ( chr & lores2 ) ? 1 : 0; p++;
					*p = ( chr & lores2 ) ? 1 : 0; p++;
					*p = ( chr & lores2 ) ? 1 : 0; p++;
				}
				else
				{
					if ((seven_bit) && (chr < 32)) chr+=64;

					/* get pattern of pixels for that character scanline */
					gfx = FNT[(chr<<4) | ra ];

					/* Display a scanline of a character (6 pixels) */
					*p = ( gfx & 0x80 ) ? 1 : 0; p++;
					*p = ( gfx & 0x40 ) ? 1 : 0; p++;
					*p = ( gfx & 0x20 ) ? 1 : 0; p++;
					*p = ( gfx & 0x10 ) ? 1 : 0; p++;
					*p = ( gfx & 0x08 ) ? 1 : 0; p++;
					*p = ( gfx & 0x04 ) ? 1 : 0; p++;
				}
			}
		}
		ma+=64;
	}
	return 0;
}

VIDEO_UPDATE( lnw80 )
{
	const UINT16 rows[] = { 0, 0x200, 0x100, 0x300, 1, 0x201, 0x101, 0x301 };
	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,ma=0,x;
	UINT8 *FNT = memory_region(screen->machine, "gfx1");
	static UINT8 size_store=0;
	static UINT8 cols=64,skip=1;
	UINT8 lores1,lores2;

	if ((trs80_mode & 0x7f) != size_store)
	{
		size_store = trs80_mode & 0x7f;
		cols = (trs80_mode & 1) ? 32 : 64;
		skip = (trs80_mode & 1) ? 2 : 1;
		video_screen_set_visarea(screen, 0, cols*FW-1, 0, 16*FH-1);
	}

	for (y = 0; y < 16; y++)
	{
		for (ra = 0; ra < FH; ra++)
		{
			UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);

			for (x = ma; x < ma + 64; x+=skip)
			{
				chr = videoram[x];

				if (chr & 0x80)
				{
					lores1 = 1<<((ra & 0x0c)>>1);
					lores2 = lores1<<1; 
					/* Display one line of a lores character (6 pixels) */
					*p = ( chr & lores1 ) ? 1 : 0; p++;
					*p = ( chr & lores1 ) ? 1 : 0; p++;
					*p = ( chr & lores1 ) ? 1 : 0; p++;
					*p = ( chr & lores2 ) ? 1 : 0; p++;
					*p = ( chr & lores2 ) ? 1 : 0; p++;
					*p = ( chr & lores2 ) ? 1 : 0; p++;
				}
				else
				{
					/* get pattern of pixels for that character scanline */
					if (ra < 8)
						gfx = FNT[(chr<<1) | rows[ra] ];
					else
						gfx = 0;

					/* Display a scanline of a character (6 pixels) */
					*p = ( gfx & 0x04 ) ? 1 : 0; p++;
					*p = ( gfx & 0x02 ) ? 1 : 0; p++;
					*p = ( gfx & 0x40 ) ? 1 : 0; p++;
					*p = ( gfx & 0x80 ) ? 1 : 0; p++;
					*p = ( gfx & 0x20 ) ? 1 : 0; p++;
					*p = ( gfx & 0x08 ) ? 1 : 0; p++;
				}
			}
		}
		ma+=64;
	}
	return 0;
}


VIDEO_UPDATE( radionic )
{
	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,ma=0,x;
	UINT8 *FNT = memory_region(screen->machine, "gfx1");
	static UINT8 size_store=0;
	static UINT8 cols=64,skip=1;
	UINT8 lores1,lores2;

	if ((trs80_mode & 0x7f) != size_store)
	{
		size_store = trs80_mode & 0x7f;
		cols = (trs80_mode & 1) ? 32 : 64;
		skip = (trs80_mode & 1) ? 2 : 1;
		video_screen_set_visarea(screen, 0, cols*FW-1, 0, 16*FH-1);
	}

	for (y = 0; y < 16; y++)
	{
		for (ra = 0; ra < FH; ra++)
		{
			UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);

			for (x = ma; x < ma + 64; x+=skip)
			{
				chr = videoram[x];

				if ((chr & 0xc0) == 0x80)
				{
					lores1 = 1<<((ra & 0x0c)>>1);
					lores2 = lores1<<1; 
					/* Display one line of a lores character (6 pixels) */
					*p = ( chr & lores1 ) ? 1 : 0; p++;
					*p = ( chr & lores1 ) ? 1 : 0; p++;
					*p = ( chr & lores1 ) ? 1 : 0; p++;
					*p = ( chr & lores2 ) ? 1 : 0; p++;
					*p = ( chr & lores2 ) ? 1 : 0; p++;
					*p = ( chr & lores2 ) ? 1 : 0; p++;
				}
				else
				{
					/* get pattern of pixels for that character scanline */
					if (ra < 8)
						gfx = FNT[(chr<<3) | ra ];
					else
						gfx = FNT[(chr<<3) | (ra & 7) | 0x800];

					/* Display a scanline of a character (6 pixels) */
					*p = ( gfx & 0x01 ) ? 1 : 0; p++;
					*p = ( gfx & 0x02 ) ? 1 : 0; p++;
					*p = ( gfx & 0x04 ) ? 1 : 0; p++;
					*p = ( gfx & 0x08 ) ? 1 : 0; p++;
					*p = ( gfx & 0x10 ) ? 1 : 0; p++;
					*p = ( gfx & 0x20 ) ? 1 : 0; p++;
				}
			}
		}
		ma+=64;
	}
	return 0;
}


/***************************************************************************
  Write to video ram
***************************************************************************/

READ8_HANDLER( trs80_videoram_r )
{
	if (trs80_mode & 0x80) offset |= 0x400;
	return videoram[offset];
}

WRITE8_HANDLER( trs80_videoram_w )
{
	if (trs80_mode & 0x80) offset |= 0x400;
	videoram[offset] = data;
}

