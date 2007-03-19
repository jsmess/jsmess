/***************************************************************************

	vidhrdw/apple3.c

	Apple ///

***************************************************************************/

#include "includes/apple3.h"

#define	BLACK	0
#define DKRED	1
#define	DKBLUE	2
#define PURPLE	3
#define DKGREEN	4
#define DKGRAY	5
#define	BLUE	6
#define LTBLUE	7
#define BROWN	8
#define ORANGE	9
#define	GRAY	10
#define PINK	11
#define GREEN	12
#define YELLOW	13
#define AQUA	14
#define	WHITE	15


static const UINT32 text_map[] =
{
	0x400, 0x480, 0x500, 0x580, 0x600, 0x680, 0x700, 0x780,
	0x428, 0x4a8, 0x528, 0x5a8, 0x628, 0x6a8, 0x728, 0x7a8,
	0x450, 0x4d0, 0x550, 0x5d0, 0x650, 0x6d0, 0x750, 0x7d0
};
static UINT8 *char_mem;
static UINT32 *hgr_map;


void apple3_write_charmem(void)
{
	static const UINT32 screen_hole_map[] =
	{
		0x478, 0x4f8, 0x578, 0x5f8, 0x678, 0x6f8, 0x778, 0x7f8
	};
	int i, j, addr;
	UINT8 val;

	for (i = 0; i < 8; i++)
	{
		for (j = 0; j < 4; j++)
		{
			addr = 0x7f & program_read_byte(screen_hole_map[i] + 0x400 + j + 0);
			val = program_read_byte(screen_hole_map[i] + j + 0);
			char_mem[((addr * 8) + ((i & 3) * 2) + 0) & 0x3ff] = val;

			addr = 0x7f & program_read_byte(screen_hole_map[i] + 0x400 + j + 4);
			val = program_read_byte(screen_hole_map[i] + j + 4);
			char_mem[((addr * 8) + ((i & 3) * 2) + 1) & 0x3ff] = val;
		}
	}
}



VIDEO_START( apple3 )
{
	int i, j;
	UINT32 v;

	char_mem = auto_malloc(0x800);
	memset(char_mem, 0, 0x800);

	hgr_map = auto_malloc(192 * sizeof(*hgr_map));
	for (i = 0; i < 24; i++)
	{
		v = text_map[i] - 0x0400;
		for (j = 0; j < 8; j++)
		{
			hgr_map[(i * 8) + j] = 0x2000 + v + (j * 0x400);
		}
	}
	return 0;
}



static void apple3_video_text40(mame_bitmap *bitmap)
{
	int x, y, col, row;
	offs_t offset;
	UINT8 ch;
	const UINT8 *char_data;
	pen_t fg, bg, temp;
	UINT16 *dest;

	for (y = 0; y < 24; y++)
	{
		for (x = 0; x < 40; x++)
		{
			offset = mess_ram_size - 0x8000 + text_map[y] + x + (a3 & VAR_VM2 ? 0x0400 : 0x0000);
			ch = mess_ram[offset];

			if (a3 & VAR_VM0)
			{
				/* color text */
				offset = mess_ram_size - 0x8000 + text_map[y] + x + (a3 & VAR_VM2 ? 0x0000 : 0x0400);
				bg = (mess_ram[offset] >> 0) & 0x0F;
				fg = (mess_ram[offset] >> 4) & 0x0F;
			}
			else
			{
				/* monochrome */
				bg = BLACK;
				fg = GREEN;
			}

			/* inverse? */
			if (!(ch & 0x80))
			{
				temp = fg;
				fg = bg;
				bg = temp;
			}

			char_data = &char_mem[(ch & 0x7F) * 8];

			for (row = 0; row < 8; row++)
			{
				for (col = 0; col < 7; col++)
				{
					dest = BITMAP_ADDR16(bitmap, y * 8 + row, x * 14 + col * 2);
					dest[0] = (char_data[row] & (1 << col)) ? fg : bg;
					dest[1] = (char_data[row] & (1 << col)) ? fg : bg;
				}
			}
		}
	}
}



static void apple3_video_text80(mame_bitmap *bitmap)
{
	int x, y, col, row;
	offs_t offset;
	UINT8 ch;
	const UINT8 *char_data;
	pen_t fg, bg;
	UINT16 *dest;

	for (y = 0; y < 24; y++)
	{
		for (x = 0; x < 40; x++)
		{
			offset = mess_ram_size - 0x8000 + text_map[y] + x;

			/* first character */
			ch = mess_ram[offset + 0x0000];
			char_data = &char_mem[(ch & 0x7F) * 8];
			fg = (ch & 0x80) ? GREEN : BLACK;
			bg = (ch & 0x80) ? BLACK : GREEN;

			for (row = 0; row < 8; row++)
			{
				for (col = 0; col < 7; col++)
				{
					dest = BITMAP_ADDR16(bitmap, y * 8 + row, x * 14 + col + 0);
					dest[0] = (char_data[row] & (1 << col)) ? fg : bg;
					dest[1] = (char_data[row] & (1 << col)) ? fg : bg;
				}
			}

			/* second character */
			ch = mess_ram[offset + 0x0400];
			char_data = &char_mem[(ch & 0x7F) * 8];
			fg = (ch & 0x80) ? GREEN : BLACK;
			bg = (ch & 0x80) ? BLACK : GREEN;

			for (row = 0; row < 8; row++)
			{
				for (col = 0; col < 7; col++)
				{
					dest = BITMAP_ADDR16(bitmap, y * 8 + row, x * 14 + col + 7);
					dest[0] = (char_data[row] & (1 << col)) ? fg : bg;
					dest[1] = (char_data[row] & (1 << col)) ? fg : bg;
				}
			}
		}
	}
}



static void apple3_video_graphics_hgr(mame_bitmap *bitmap)
{
	/* hi-res mode: 280x192x2 */
	int y, i, x;
	const UINT8 *pix_info;
	UINT16 *ptr;
	UINT8 b;

	for (y = 0; y < 192; y++)
	{
		if (a3 & VAR_VM2)
			pix_info = &mess_ram[hgr_map[y]];
		else
			pix_info = &mess_ram[hgr_map[y] - 0x2000];
		ptr = BITMAP_ADDR16(bitmap, y, 0);

		for (i = 0; i < 40; i++)
		{
			b = *(pix_info++);

			for (x = 0; x < 7; x++)
			{
				ptr[0] = ptr[1] = (b & 0x01) ? WHITE : BLACK;
				ptr += 2;
				b >>= 1;
			}
		}
	}
}



static UINT8 swap_bits(UINT8 b)
{
	return (b & 0x08 ? 0x01 : 0x00)
		|  (b & 0x04 ? 0x02 : 0x00)
		|  (b & 0x02 ? 0x04 : 0x00)
		|  (b & 0x01 ? 0x08 : 0x00);
}



static void apple3_video_graphics_chgr(mame_bitmap *bitmap)
{
	/* color hi-res mode: 280x192x16 */
	int y, i, x;
	const UINT8 *pix_info;
	const UINT8 *col_info;
	UINT16 *ptr;
	UINT8 b;
	UINT16 fgcolor, bgcolor;

	for (y = 0; y < 192; y++)
	{
		if (a3 & VAR_VM2)
		{
			pix_info = &mess_ram[hgr_map[y]];
			col_info = &mess_ram[hgr_map[y] - 0x2000];
		}
		else
		{
			pix_info = &mess_ram[hgr_map[y] - 0x2000];
			col_info = &mess_ram[hgr_map[y]];
		}
		ptr = BITMAP_ADDR16(bitmap, y, 0);

		for (i = 0; i < 40; i++)
		{
			bgcolor = swap_bits((*col_info >> 0) & 0x0F);
			fgcolor = swap_bits((*col_info >> 4) & 0x0F);

			b = *pix_info;

			for (x = 0; x < 7; x++)
			{
				ptr[0] = ptr[1] = (b & 1) ? fgcolor : bgcolor;
				ptr += 2;
				b >>= 1;
			}
			pix_info++;
			col_info++;
		}
	}
}



static void apple3_video_graphics_shgr(mame_bitmap *bitmap)
{
	/* super hi-res mode: 560x192x2 */
	int y, i, x;
	const UINT8 *pix_info1;
	const UINT8 *pix_info2;
	UINT16 *ptr;
	UINT8 b1, b2;

	for (y = 0; y < 192; y++)
	{
		if (a3 & VAR_VM2)
		{
			pix_info1 = &mess_ram[hgr_map[y]];
			pix_info2 = &mess_ram[hgr_map[y] + 0x2000];
		}
		else
		{
			pix_info1 = &mess_ram[hgr_map[y] - 0x2000];
			pix_info2 = &mess_ram[hgr_map[y]];
		}
		ptr = BITMAP_ADDR16(bitmap, y, 0);

		for (i = 0; i < 40; i++)
		{
			b1 = *(pix_info1++);
			b2 = *(pix_info2++);

			for (x = 0; x < 7; x++)
			{
				*(ptr++) = (b1 & 0x01) ? WHITE : BLACK;
				*(ptr++) = (b2 & 0x01) ? WHITE : BLACK;
				b1 >>= 1;
				b2 >>= 1;
			}
		}
	}
}



static void apple3_video_graphics_chires(mame_bitmap *bitmap)
{
	UINT16 *pen;
	PAIR pix;
	int y, i;

	for (y = 0; y < 192; y++)
	{
		pen = BITMAP_ADDR16(bitmap, y, 0);
		for (i = 0; i < 20; i++)
		{
			pix.b.l  = mess_ram[hgr_map[y] - 0x2000 + (i * 2) + (a3 & VAR_VM2 ? 1 : 0) + 0];
			pix.b.h  = mess_ram[hgr_map[y] - 0x0000 + (i * 2) + (a3 & VAR_VM2 ? 1 : 0) + 0];
			pix.b.h2 = mess_ram[hgr_map[y] - 0x2000 + (i * 2) + (a3 & VAR_VM2 ? 1 : 0) + 1];
			pix.b.h3 = mess_ram[hgr_map[y] - 0x0000 + (i * 2) + (a3 & VAR_VM2 ? 1 : 0) + 1];

			pen[ 0] = pen[ 1] = pen[ 2] = pen[ 3] = ((pix.d >>  0) & 0x0F);
			pen[ 4] = pen[ 5] = pen[ 6] = pen[ 7] = ((pix.d >>  4) & 0x07) | ((pix.d >>  1) & 0x08);
			pen[ 8] = pen[ 9] = pen[10] = pen[11] = ((pix.d >>  9) & 0x0F);
			pen[12] = pen[13] = pen[14] = pen[15] = ((pix.d >> 13) & 0x03) | ((pix.d >> 14) & 0x0C);
			pen[16] = pen[17] = pen[18] = pen[19] = ((pix.d >> 18) & 0x0F);
			pen[20] = pen[21] = pen[22] = pen[23] = ((pix.d >> 22) & 0x01) | ((pix.d >> 23) & 0x0E);
			pen[24] = pen[25] = pen[26] = pen[27] = ((pix.d >> 27) & 0x0F);
			pen += 28;
		}
	}
}



VIDEO_UPDATE( apple3 )
{
	switch(a3 & (VAR_VM3|VAR_VM1|VAR_VM0))
	{
		case 0:
		case VAR_VM0:
			apple3_video_text40(bitmap);
			break;

		case VAR_VM1:
		case VAR_VM1|VAR_VM0:
			apple3_video_text80(bitmap);
			break;

		case VAR_VM3:
			apple3_video_graphics_hgr(bitmap);	/* hgr mode */
			break;

		case VAR_VM3|VAR_VM0:
			apple3_video_graphics_chgr(bitmap);
			break;

		case VAR_VM3|VAR_VM1:
			apple3_video_graphics_shgr(bitmap);
			break;

		case VAR_VM3|VAR_VM1|VAR_VM0:
			apple3_video_graphics_chires(bitmap);
			break;
	}
	return 0;
}


