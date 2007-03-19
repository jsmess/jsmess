/***************************************************************************

	vidhrdw/mac.c
	
	Macintosh video hardware

	Emulates the video hardware for compact Macintosh series (original
	Macintosh (128k, 512k, 512ke), Macintosh Plus, Macintosh SE, Macintosh
	Classic)

***************************************************************************/

#include "driver.h"
#include "includes/mac.h"

static int screen_buffer;

PALETTE_INIT( mac )
{
	palette_set_color(machine, 0, 0xff, 0xff, 0xff);
	palette_set_color(machine, 1, 0x00, 0x00, 0x00);
}



void mac_set_screen_buffer(int buffer)
{
	screen_buffer = buffer;
}



VIDEO_START( mac )
{
	screen_buffer = 0;
	return 0;
}



#define MAC_MAIN_SCREEN_BUF_OFFSET	0x5900
#define MAC_ALT_SCREEN_BUF_OFFSET	0xD900

VIDEO_UPDATE( mac )
{
	UINT32 video_base;
	const UINT16 *video_ram;
	UINT16 word;
	UINT16 *line;
	int y, x, b;

	video_base = mess_ram_size - (screen_buffer ? MAC_MAIN_SCREEN_BUF_OFFSET : MAC_ALT_SCREEN_BUF_OFFSET);
	video_ram = (const UINT16 *) (mess_ram + video_base);

	for (y = 0; y < Machine->screen[0].height; y++)
	{
		line = BITMAP_ADDR16(bitmap, y, 0);

		for (x = 0; x < Machine->screen[0].width; x += 16)
		{
			word = *(video_ram++);
			for (b = 0; b < 16; b++)
			{
				line[x + b] = (word >> (15 - b)) & 0x0001;
			}
		}
	}
	return 0;
}
