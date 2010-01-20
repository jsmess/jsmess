/***************************************************************************

        Vector06c driver by Miodrag Milanovic

        10/07/2008 Preliminary driver.

****************************************************************************/


#include "emu.h"
#include "includes/vector06.h"
#include "devices/messram.h"

VIDEO_START( vector06 )
{
}

VIDEO_UPDATE( vector06 )
{
 	UINT8 code1,code2,code3,code4;
 	UINT8 col;
	int y, x, b,draw_y;

	int width = (vector06_video_mode==0x00) ? 256 : 512;
	rectangle screen_area = {0,width+64-1,0,256+64-1};
	// fill border color
	bitmap_fill(bitmap, &screen_area, vector06_color_index);

	// draw image
	for (x = 0; x < 32; x++)
	{
		for (y = 0; y < 256; y++)
		{
			// port A form 8255 also used as scroll
			draw_y = ((255-y-vector06_keyboard_mask) & 0xff) +32;
			code1 = messram_get_ptr(devtag_get_device(screen->machine, "messram"))[0x8000 + x*256 + y];
			code2 = messram_get_ptr(devtag_get_device(screen->machine, "messram"))[0xa000 + x*256 + y];
			code3 = messram_get_ptr(devtag_get_device(screen->machine, "messram"))[0xc000 + x*256 + y];
			code4 = messram_get_ptr(devtag_get_device(screen->machine, "messram"))[0xe000 + x*256 + y];
			for (b = 0; b < 8; b++)
			{
				col = ((code1 >> b) & 0x01) * 8 + ((code2 >> b) & 0x01) * 4 + ((code3 >> b) & 0x01)* 2+ ((code4 >> b) & 0x01);
				if (vector06_video_mode==0x00) {
					*BITMAP_ADDR16(bitmap, draw_y, x*8+(7-b)+32) =  col;
				} else {
					*BITMAP_ADDR16(bitmap, draw_y, x*16+(7-b)*2+1+32) =  ((code2 >> b) & 0x01) * 2;
					*BITMAP_ADDR16(bitmap, draw_y, x*16+(7-b)*2+32)   =  ((code3 >> b) & 0x01) * 2;
				}
			}
		}
	}
	return 0;
}

PALETTE_INIT( vector06 )
{
	int i;
	for(i=0;i<16;i++) {
		palette_set_color( machine, i, MAKE_RGB(0,0,0) );
	}
}
