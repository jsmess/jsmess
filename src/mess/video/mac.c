/***************************************************************************

    video/mac.c

    Macintosh video hardware

    Emulates the video hardware for compact Macintosh series (original
    Macintosh (128k, 512k, 512ke), Macintosh Plus, Macintosh SE, Macintosh
    Classic)

***************************************************************************/


#include "driver.h"
#include "includes/mac.h"
#include "devices/messram.h"

static int screen_buffer;

PALETTE_INIT( mac )
{
	palette_set_color_rgb(machine, 0, 0xff, 0xff, 0xff);
	palette_set_color_rgb(machine, 1, 0x00, 0x00, 0x00);
}



void mac_set_screen_buffer(int buffer)
{
	screen_buffer = buffer;
}



VIDEO_START( mac )
{
	screen_buffer = 0;
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

	video_base = messram_get_size(devtag_get_device(screen->machine, "messram")) - (screen_buffer ? MAC_MAIN_SCREEN_BUF_OFFSET : MAC_ALT_SCREEN_BUF_OFFSET);
	video_ram = (const UINT16 *) (messram_get_ptr(devtag_get_device(screen->machine, "messram")) + video_base);

	for (y = 0; y < MAC_V_VIS; y++)
	{
		line = BITMAP_ADDR16(bitmap, y, 0);

		for (x = 0; x < MAC_H_VIS; x += 16)
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

VIDEO_UPDATE( macse30 )
{
	UINT32 video_base;
	const UINT16 *video_ram;
	UINT16 word;
	UINT16 *line;
	int y, x, b;

	video_base = screen_buffer ? 0x8000 : 0;
	video_ram = (const UINT16 *) &se30_vram[video_base/4];

	for (y = 0; y < MAC_V_VIS; y++)
	{
		line = BITMAP_ADDR16(bitmap, y, 0);

		for (x = 0; x < MAC_H_VIS; x += 16)
		{
//			word = *(video_ram++);
			word = video_ram[((y * MAC_H_VIS)/16) + ((x/16)^1)];
			for (b = 0; b < 16; b++)
			{
				line[x + b] = (word >> (15 - b)) & 0x0001;
			}
		}
	}
	return 0;
}

