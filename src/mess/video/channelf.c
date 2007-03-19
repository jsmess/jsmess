#include "driver.h"
#include "video/generic.h"
#include "includes/channelf.h"

UINT8 channelf_val_reg = 0;
UINT8 channelf_row_reg = 0;
UINT8 channelf_col_reg = 0;

static UINT8 channelf_palette[] = {
	0x00, 0x00, 0x00,	/* black */
	0xff, 0xff, 0xff,	/* white */
	0xff, 0x00, 0x00,	/* red	 */
	0x00, 0xff, 0x00,	/* green */
	0x00, 0x00, 0xff,	/* blue  */
	0xbf, 0xbf, 0xbf,	/* ltgray  */
	0xbf, 0xff, 0xbf,	/* ltgreen */
	0xbf, 0xbf, 0xff	/* ltblue  */
};

#define BLACK	0
#define WHITE   1
#define RED     2
#define GREEN   3
#define BLUE    4
#define LTGRAY  5
#define LTGREEN 6
#define LTBLUE	7

static UINT16 colormap[] = {
	BLACK,   WHITE, WHITE, WHITE,
	LTBLUE,  BLUE,  RED,   GREEN,
	LTGRAY,  BLUE,  RED,   GREEN,
	LTGREEN, BLUE,  RED,   GREEN,
};

/* Initialise the palette */
PALETTE_INIT( channelf )
{
	palette_set_colors(machine, 0, channelf_palette, sizeof(channelf_palette) / 3);
}

VIDEO_START( channelf )
{
	videoram_size = 0x2000;
	videoram = auto_malloc(videoram_size);
	return video_start_generic(machine);
}

static int recalc_palette_offset(int reg1, int reg2)
{
	/* Note: This is based on the decoding they used to   */
	/*       determine which palette this line is using   */

	return ((reg2&0x2)|(reg1>>1)) << 2;
}

VIDEO_UPDATE( channelf )
{
	int x,y,offset, palette_offset;
	int pen, color;

	for(y=0;y<64;y++)
	{
		palette_offset = recalc_palette_offset(videoram[y*128+125]&3,videoram[y*128+126]&3);
		for (x=0;x<128;x++)
		{
			offset = y*128+x;
			color = palette_offset+(videoram[offset]&3);
			pen = Machine->pens[colormap[color]];
			plot_pixel(bitmap, x, y, pen);
		}
	}
	return 0;
}


