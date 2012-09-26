#include "emu.h"
#include "includes/channelf.h"

static const rgb_t channelf_palette[] =
{
	MAKE_RGB(0x10, 0x10, 0x10),	/* black */
	MAKE_RGB(0xfd, 0xfd, 0xfd),	/* white */
	MAKE_RGB(0xff, 0x31, 0x53),	/* red   */
	MAKE_RGB(0x02, 0xcc, 0x5d),	/* green */
	MAKE_RGB(0x4b, 0x3f, 0xf3),	/* blue  */
	MAKE_RGB(0xe0, 0xe0, 0xe0),	/* ltgray  */
	MAKE_RGB(0x91, 0xff, 0xa6),	/* ltgreen */
	MAKE_RGB(0xce, 0xd0, 0xff)	/* ltblue  */
};

#define BLACK	0
#define WHITE   1
#define RED     2
#define GREEN   3
#define BLUE    4
#define LTGRAY  5
#define LTGREEN 6
#define LTBLUE	7

static const UINT16 colormap[] = {
	BLACK,   WHITE, WHITE, WHITE,
	LTBLUE,  BLUE,  RED,   GREEN,
	LTGRAY,  BLUE,  RED,   GREEN,
	LTGREEN, BLUE,  RED,   GREEN,
};

/* Initialise the palette */
PALETTE_INIT( channelf )
{
	palette_set_colors(machine, 0, channelf_palette, ARRAY_LENGTH(channelf_palette));
}

VIDEO_START( channelf )
{
	channelf_state *state = machine.driver_data<channelf_state>();
	state->m_videoram = auto_alloc_array(machine, UINT8, 0x2000);
}

static int recalc_palette_offset(int reg1, int reg2)
{
	/* Note: This is based on the decoding they used to   */
	/*       determine which palette this line is using   */

	return ((reg2&0x2)|(reg1>>1)) << 2;
}

SCREEN_UPDATE( channelf )
{
	channelf_state *state = screen->machine().driver_data<channelf_state>();
	UINT8 *videoram = state->m_videoram;
	int x,y,offset, palette_offset;
	int color;
	UINT16 pen;

	for(y=0;y<64;y++)
	{
		palette_offset = recalc_palette_offset(videoram[y*128+125]&3,videoram[y*128+126]&3);
		for (x=0;x<128;x++)
		{
			offset = y*128+x;
			color = palette_offset+(videoram[offset]&3);
			pen = colormap[color];
			*BITMAP_ADDR16(bitmap, y, x) = pen;
		}
	}
	return 0;
}
