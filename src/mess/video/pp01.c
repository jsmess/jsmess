/***************************************************************************

        PP-01 driver by Miodrag Milanovic

        08/09/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "includes/pp01.h"

VIDEO_START( pp01 )
{
}

VIDEO_UPDATE( pp01 )
{
/*  UINT8 code1;
    UINT8 code2;
    UINT8 col;
    int y, x, b;

    for (x = 0; x < 48; x++)
    {
        for (y = 0; y < 256; y++)
        {
            if (b2m_video_page==0) {
                code1 = mess_ram[0x11000 + x*256 + ((y + b2m_video_scroll) & 0xff)];
                code2 = mess_ram[0x15000 + x*256 + ((y + b2m_video_scroll) & 0xff)];
            } else {
                code1 = mess_ram[0x19000 + x*256 + ((y + b2m_video_scroll) & 0xff)];
                code2 = mess_ram[0x1d000 + x*256 + ((y + b2m_video_scroll) & 0xff)];
            }
            for (b = 7; b >= 0; b--)
            {
                col = (((code2 >> b) & 0x01)<<1) + ((code1 >> b) & 0x01);
                *BITMAP_ADDR16(bitmap, y, x*8+b) =  col;
            }
        }
    }
    */
	return 0;
}

static const rgb_t pp01_palette[8] = {
	MAKE_RGB(0x00, 0x00, 0x00), // 0
	MAKE_RGB(0x00, 0x00, 0x80), // 1
	MAKE_RGB(0x00, 0x80, 0x00), // 2
	MAKE_RGB(0x00, 0x80, 0x80), // 3
	MAKE_RGB(0x80, 0x00, 0x00), // 4
	MAKE_RGB(0x80, 0x00, 0x80), // 5
	MAKE_RGB(0x80, 0x80, 0x00), // 6
	MAKE_RGB(0x80, 0x80, 0x80), // 7
};

PALETTE_INIT( pp01 )
{
	palette_set_colors(machine, 0, pp01_palette, ARRAY_LENGTH(pp01_palette));
}
