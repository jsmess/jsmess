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

UINT32 *mac_se30_vram;
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
	video_ram = (const UINT16 *) &mac_se30_vram[video_base/4];

	for (y = 0; y < MAC_V_VIS; y++)
	{
		line = BITMAP_ADDR16(bitmap, y, 0);

		for (x = 0; x < MAC_H_VIS; x += 16)
		{
//          word = *(video_ram++);
			word = video_ram[((y * MAC_H_VIS)/16) + ((x/16)^1)];
			for (b = 0; b < 16; b++)
			{
				line[x + b] = (word >> (15 - b)) & 0x0001;
			}
		}
	}
	return 0;
}

/*
    RasterOps ColorBoard 264: fixed resolution 640x480 NuBus video card, 1/4/8/16/24 bit color
    1.5? MB of VRAM (tests up to 0x1fffff), Bt473 RAMDAC, and two custom gate arrays.

        - 0xfsff6004 is color depth: 0 for 1bpp, 1 for 2bpp, 2 for 4bpp, 3 for 8bpp, 4 for 24bpp (resolution is fixed 640x480)
        - 0xfsff6014 is VBL ack: write 1 to ack
    - 0xfsff603c is VBL disable: write 1 to disable, 0 to enable
*/

UINT32 *mac_cb264_vram;
static UINT32 cb264_mode, cb264_vbl_disable, cb264_toggle;
static UINT32 cb264_palette[256], cb264_colors[3], cb264_count, cb264_clutoffs;

VIDEO_START( mac_cb264 )
{
	cb264_toggle = 0;
	cb264_count = 0;
	cb264_clutoffs = 0;
	cb264_vbl_disable = 1;
	cb264_mode = 0;
}

// we do this here because VIDEO_UPDATE is called constantly when stepping in the debugger,
// which makes it hard to get accurate timings (or escape the VIA2 IRQ handler)
INTERRUPT_GEN( mac_cb264_vbl )
{
	if (!cb264_vbl_disable)
	{
		mac_nubus_slot_interrupt(device->machine, 0xe, 1);
	}
}

VIDEO_UPDATE( mac_cb264 )
{
	UINT32 *scanline, *base;
	int x, y;

	switch (cb264_mode)
	{
		case 0: // 1 bpp
			{
				UINT8 *vram8 = (UINT8 *)mac_cb264_vram;
				UINT8 pixels;

				for (y = 0; y < 480; y++)
				{
					scanline = BITMAP_ADDR32(bitmap, y, 0);
					for (x = 0; x < 640/8; x++)
					{
						pixels = vram8[(y * 1024) + (x^3)];

						*scanline++ = cb264_palette[pixels&0x80];
						*scanline++ = cb264_palette[(pixels<<1)&0x80];
						*scanline++ = cb264_palette[(pixels<<2)&0x80];
						*scanline++ = cb264_palette[(pixels<<3)&0x80];
						*scanline++ = cb264_palette[(pixels<<4)&0x80];
						*scanline++ = cb264_palette[(pixels<<5)&0x80];
						*scanline++ = cb264_palette[(pixels<<6)&0x80];
						*scanline++ = cb264_palette[(pixels<<7)&0x80];
					}
				}
			}
			break;

		case 1: // 2 bpp (3f/7f/bf/ff)
			{
				UINT8 *vram8 = (UINT8 *)mac_cb264_vram;
				UINT8 pixels;

				for (y = 0; y < 480; y++)
				{
					scanline = BITMAP_ADDR32(bitmap, y, 0);
					for (x = 0; x < 640/4; x++)
					{
						pixels = vram8[(y * 1024) + (x^3)];

						*scanline++ = cb264_palette[pixels&0xc0];
						*scanline++ = cb264_palette[(pixels<<2)&0xc0];
						*scanline++ = cb264_palette[(pixels<<4)&0xc0];
						*scanline++ = cb264_palette[(pixels<<6)&0xc0];
					}
				}
			}
			break;

		case 2: // 4 bpp
			{
				UINT8 *vram8 = (UINT8 *)mac_cb264_vram;
				UINT8 pixels;

				for (y = 0; y < 480; y++)
				{
					scanline = BITMAP_ADDR32(bitmap, y, 0);

					for (x = 0; x < 640/2; x++)
					{
						pixels = vram8[(y * 1024) + (x^3)];

						*scanline++ = cb264_palette[pixels&0xf0];
						*scanline++ = cb264_palette[(pixels<<4)&0xf0];
					}
				}
			}
			break;

		case 3: // 8 bpp
			{
				UINT8 *vram8 = (UINT8 *)mac_cb264_vram;
				UINT8 pixels;

				for (y = 0; y < 480; y++)
				{
					scanline = BITMAP_ADDR32(bitmap, y, 0);

					for (x = 0; x < 640; x++)
					{
						pixels = vram8[(y * 1024) + (x^3)];
						*scanline++ = cb264_palette[pixels];
					}
				}
			}
			break;

		case 4:	// 24 bpp
		case 7: // ???
			for (y = 0; y < 480; y++)
			{
				scanline = BITMAP_ADDR32(bitmap, y, 0);
				base = &mac_cb264_vram[y * 1024];
				for (x = 0; x < 640; x++)
				{
					*scanline++ = *base++;
				}
			}
			break;

		default:
			fatalerror("cb264: unknown video mode %d\n", cb264_mode);
			break;

	}

	return 0;
}

READ32_HANDLER( mac_cb264_r )
{
	switch (offset)
	{
		case 0x0c/4:
		case 0x28/4:
			break;

		case 0x34/4:
			cb264_toggle ^= 1;
			return cb264_toggle;	// bit 0 is vblank?

		default:
			logerror("cb264_r: reg %x (mask %x PC %x)\n", offset*4, mem_mask, cpu_get_pc(space->cpu));
			break;
	}

	return 0;
}

WRITE32_HANDLER( mac_cb264_w )
{
	switch (offset)
	{
		case 0x4/4:	// 0 = 1 bpp, 1 = 2bpp, 2 = 4bpp, 3 = 8bpp, 4 = 24bpp
			cb264_mode = data;
			break;

		case 0x14/4:	// VBL ack
			mac_nubus_slot_interrupt(space->machine, 0xe, 0);
			break;

		case 0x3c/4:	// VBL disable
			cb264_vbl_disable = data;
			break;

		default:
//          printf("cb264_w: %x to reg %x (mask %x PC %x)\n", data, offset*4, mem_mask, cpu_get_pc(space->cpu));
			break;
	}
}

WRITE32_HANDLER( mac_cb264_ramdac_w )
{
	switch (offset)
	{
		case 0:
			cb264_clutoffs = data>>24;
			cb264_count = 0;
			break;

		case 1:
			cb264_colors[cb264_count++] = data>>24;

			if (cb264_count == 3)
			{
				palette_set_color(space->machine, cb264_clutoffs, MAKE_RGB(cb264_colors[0], cb264_colors[1], cb264_colors[2]));
				cb264_palette[cb264_clutoffs] = MAKE_RGB(cb264_colors[0], cb264_colors[1], cb264_colors[2]);
				cb264_clutoffs++;
				cb264_count = 0;
			}
			break;

		default:
//          printf("%x to unknown RAMDAC register @ %x\n", data, offset);
			break;
	}
}

