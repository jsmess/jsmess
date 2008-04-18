/***************************************************************************

        BK video driver by Miodrag Milanovic

        10/03/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"

UINT16 *bk0010_video_ram;
extern UINT16 bk_scrool;

VIDEO_START( bk0010 )
{
}

VIDEO_UPDATE( bk0010 )
{
  	UINT16 code;
	int y, x, b;
	int nOfs;

	nOfs = (bk_scrool - 728) % 256;

	for (y = 0; y < 256; y++)
	{
		for (x = 0; x < 32; x++)
		{
			code = bk0010_video_ram[((y+nOfs) %256)*32 + x];
			for (b = 0; b < 16; b++)
			{
				*BITMAP_ADDR16(bitmap, y, x*16 + b) =  (code >> b) & 0x01;
			}
		}
	}
	return 0;
}
