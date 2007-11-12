#include "video/ql.h"

static int mode4_colors[] = { 0, 2, 4, 7 };

PALETTE_INIT( ql )
{
	palette_set_color_rgb(machine,  0x00, 0x00, 0x00, 0x00 );
	palette_set_color_rgb(machine,  0x01, 0x00, 0x00, 0xff );
	palette_set_color_rgb(machine,  0x02, 0x00, 0xff, 0x00 );
	palette_set_color_rgb(machine,  0x03, 0x00, 0xff, 0xff );
	palette_set_color_rgb(machine,  0x04, 0xff, 0x00, 0x00 );
	palette_set_color_rgb(machine,  0x05, 0xff, 0x00, 0xff );
	palette_set_color_rgb(machine,  0x06, 0xff, 0xff, 0x00 );
	palette_set_color_rgb(machine,  0x07, 0xff, 0xff, 0xff );
}

WRITE8_HANDLER( ql_videoram_w )
{
	int i, x, y, r, g, color, byte0, byte1;
	int offs;

	videoram[offset] = data;

	offs = offset / 2;

	byte0 = videoram[offs];
	byte1 = videoram[offs + 1];

	x = (offs % 64) << 3;
	y = offs / 64;

//	logerror("ofs %u data %u x %u y %u\n", offset, data, x, y);

		/*

		# Note: QL video is encoded as 2-byte chunks
		# msb->lsb (Green Red)
		# mode 4: GGGGGGGG RRRRRRRR
		# R+G=White

		*/

		for (i = 0; i < 8; i++)
		{
			r = (byte1 & 0x80) >> 6;
			g = (byte0 & 0x80) >> 7;

			color = r | g;
			
//			logerror("x %u y %u color %u\n", x, y, color);

			*BITMAP_ADDR16(tmpbitmap, y, x++) = Machine->pens[mode4_colors[color]];

			byte0 <<= 1;
			byte1 <<= 1;

		}

		/*

		# Note: QL video is encoded as 2-byte chunks
		# msb->lsb (Green Flash Red Blue)
		# mode 8: GFGFGFGF RBRBRBRB

		*/

/*
		for (i = 0; i < 8; i++)
		{
			r = (data & 0x0080) >> 5;
			g = (data & 0x8000) >> 14;
			b = (data & 0x0040) >> 6;
			f = (data & 0x4000) >> 14;

			color = r | g | b;

			plot_pixel(tmpbitmap, x++, y, Machine->pens[color]);
			plot_pixel(tmpbitmap, x++, y, Machine->pens[color]);

			data <<= 2;
		}
*/
}

WRITE8_HANDLER( ql_video_ctrl_w )
{
	/*
	18063 is write only (Quasar p.618)
	bit 1: 0: screen on, 1: screen off
	bit 3: 0: mode 512, 1: mode 256
	bit 7: 0: base=0x20000, 1: base=0x28000
	*/
}
