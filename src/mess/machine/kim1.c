/******************************************************************************
	KIM-1

	machine Driver

	Juergen Buchmueller, Jan 2000

******************************************************************************/

#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "video/generic.h"
#include "includes/kim1.h"
#include "sound/dac.h"

typedef struct
{
	UINT8 dria; 	/* Data register A input */
	UINT8 droa; 	/* Data register A output */
	UINT8 ddra; 	/* Data direction register A; 1 bits = output */
	UINT8 drib; 	/* Data register B input */
	UINT8 drob; 	/* Data register B output */
	UINT8 ddrb; 	/* Data direction register B; 1 bits = output */
	UINT8 irqen;	/* IRQ enabled ? */
	UINT8 state;	/* current timer state (bit 7) */
	double clock;	/* 100000/1(,8,64,1024) */
	mame_timer *timer;	/* timer callback */
}
M6530;

static M6530 m6530[2];

static void m6530_timer_cb(int chip);

DRIVER_INIT( kim1 )
{
	UINT8 *dst;
	int x, y, i;

	static const char *seg7 =
	"....aaaaaaaaaaaaa." \
	"...f.aaaaaaaaaaa.b" \
	"...ff.aaaaaaaaa.bb" \
	"...fff.........bbb" \
	"...fff.........bbb" \
	"...fff.........bbb" \
	"..fff.........bbb." \
	"..fff.........bbb." \
	"..fff.........bbb." \
	"..ff...........bb." \
	"..f.ggggggggggg.b." \
	"..gggggggggggggg.." \
	".e.ggggggggggg.c.." \
	".ee...........cc.." \
	".eee.........ccc.." \
	".eee.........ccc.." \
	".eee.........ccc.." \
	"eee.........ccc..." \
	"eee.........ccc..." \
	"eee.........ccc..." \
	"ee.ddddddddd.cc..." \
	"e.ddddddddddd.c..." \
	".ddddddddddddd...." \
	"..................";


	static const char *keys[24] =
	{
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaccccaaaaaaaaaa" \
		"aaaaaaaaccccccccaaaaaaaa" \
		"aaaaaaaaccaaaaccaaaaaaaa" \
		"aaaaaaaccaaaaaaccaaaaaaa" \
		"aaaaaaaccaaaaacccaaaaaaa" \
		"aaaaaaaccaaaaccccaaaaaaa" \
		"aaaaaaaccaaaccaccaaaaaaa" \
		"aaaaaaaccaaccaaccaaaaaaa" \
		"aaaaaaaccaccaaaccaaaaaaa" \
		"aaaaaaaccccaaaaccaaaaaaa" \
		"aaaaaaacccaaaaaccaaaaaaa" \
		"aaaaaaaaccaaaaccaaaaaaaa" \
		"aaaaaaaaccccccccaaaaaaaa" \
		"aaaaaaaaaaccccaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa",

		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaccaaaaaaaaaaa" \
		"aaaaaaaaaacccaaaaaaaaaaa" \
		"aaaaaaaaaccccaaaaaaaaaaa" \
		"aaaaaaaaccaccaaaaaaaaaaa" \
		"aaaaaaaaaaaccaaaaaaaaaaa" \
		"aaaaaaaaaaaccaaaaaaaaaaa" \
		"aaaaaaaaaaaccaaaaaaaaaaa" \
		"aaaaaaaaaaaccaaaaaaaaaaa" \
		"aaaaaaaaaaaccaaaaaaaaaaa" \
		"aaaaaaaaaaaccaaaaaaaaaaa" \
		"aaaaaaaaaaaccaaaaaaaaaaa" \
		"aaaaaaaaaaaccaaaaaaaaaaa" \
		"aaaaaaaaaaaccaaaaaaaaaaa" \
		"aaaaaaaaaaaccaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa",

		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaccccccccaaaaaaaa" \
		"aaaaaaaccccccccccaaaaaaa" \
		"aaaaaaaccaaaaaaccaaaaaaa" \
		"aaaaaaaaaaaaaaaccaaaaaaa" \
		"aaaaaaaaaaaaaacccaaaaaaa" \
		"aaaaaaaaaaaaccccaaaaaaaa" \
		"aaaaaaaaaccccccaaaaaaaaa" \
		"aaaaaaaaccccaaaaaaaaaaaa" \
		"aaaaaaacccaaaaaaaaaaaaaa" \
		"aaaaaaaccaaaaaaaaaaaaaaa" \
		"aaaaaaaccaaaaaaaaaaaaaaa" \
		"aaaaaaaccaaaaaaaaaaaaaaa" \
		"aaaaaaaccccccccccaaaaaaa" \
		"aaaaaaaccccccccccaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa",

		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaccccccaaaaaaaaa" \
		"aaaaaaaaccccccccaaaaaaaa" \
		"aaaaaaacccaaaacccaaaaaaa" \
		"aaaaaaaccaaaaaaccaaaaaaa" \
		"aaaaaaaaaaaaaaaccaaaaaaa" \
		"aaaaaaaaaaaaaacccaaaaaaa" \
		"aaaaaaaaaaccccccaaaaaaaa" \
		"aaaaaaaaaaccccccaaaaaaaa" \
		"aaaaaaaaaaaaaacccaaaaaaa" \
		"aaaaaaaaaaaaaaaccaaaaaaa" \
		"aaaaaaaccaaaaaaccaaaaaaa" \
		"aaaaaaacccaaaacccaaaaaaa" \
		"aaaaaaaaccccccccaaaaaaaa" \
		"aaaaaaaaaccccccaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa",

		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaacaaaaaaaaa" \
		"aaaaaaaaaaaaaccaaaaaaaaa" \
		"aaaaaaaaaaaacccaaaaaaaaa" \
		"aaaaaaaaaaaccccaaaaaaaaa" \
		"aaaaaaaaaaccaccaaaaaaaaa" \
		"aaaaaaaaaccaaccaaaaaaaaa" \
		"aaaaaaaaccaaaccaaaaaaaaa" \
		"aaaaaaaccaaaaccaaaaaaaaa" \
		"aaaaaacccccccccccaaaaaaa" \
		"aaaaaacccccccccccaaaaaaa" \
		"aaaaaaaaaaaaaccaaaaaaaaa" \
		"aaaaaaaaaaaaaccaaaaaaaaa" \
		"aaaaaaaaaaaaaccaaaaaaaaa" \
		"aaaaaaaaaaaaaccaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa",

		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaccccccccccaaaaaaa" \
		"aaaaaaaccccccccccaaaaaaa" \
		"aaaaaaaccaaaaaaaaaaaaaaa" \
		"aaaaaaaccaaaaaaaaaaaaaaa" \
		"aaaaaaaccccccccaaaaaaaaa" \
		"aaaaaaacccccccccaaaaaaaa" \
		"aaaaaaaaaaaaaacccaaaaaaa" \
		"aaaaaaaaaaaaaaaccaaaaaaa" \
		"aaaaaaaaaaaaaaaccaaaaaaa" \
		"aaaaaaaaaaaaaaaccaaaaaaa" \
		"aaaaaaaccaaaaaaccaaaaaaa" \
		"aaaaaaacccaaaacccaaaaaaa" \
		"aaaaaaaaccccccccaaaaaaaa" \
		"aaaaaaaaaccccccaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa",

		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaccccccaaaaaaaaa" \
		"aaaaaaaaccccccccaaaaaaaa" \
		"aaaaaaacccaaaacccaaaaaaa" \
		"aaaaaaaccaaaaaaaaaaaaaaa" \
		"aaaaaaaccaaaaaaaaaaaaaaa" \
		"aaaaaaaccaccccaaaaaaaaaa" \
		"aaaaaaacccccccccaaaaaaaa" \
		"aaaaaaaccccaaaccaaaaaaaa" \
		"aaaaaaaccaaaaaaccaaaaaaa" \
		"aaaaaaaccaaaaaaccaaaaaaa" \
		"aaaaaaaccaaaaaaccaaaaaaa" \
		"aaaaaaacccaaaacccaaaaaaa" \
		"aaaaaaaaccccccccaaaaaaaa" \
		"aaaaaaaaaccccccaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa",

		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaccccccccccaaaaaaa" \
		"aaaaaaaccccccccccaaaaaaa" \
		"aaaaaaaaaaaaaaaccaaaaaaa" \
		"aaaaaaaaaaaaaacccaaaaaaa" \
		"aaaaaaaaaaaaacccaaaaaaaa" \
		"aaaaaaaaaaaacccaaaaaaaaa" \
		"aaaaaaaaaaacccaaaaaaaaaa" \
		"aaaaaaaaaacccaaaaaaaaaaa" \
		"aaaaaaaaacccaaaaaaaaaaaa" \
		"aaaaaaaacccaaaaaaaaaaaaa" \
		"aaaaaaacccaaaaaaaaaaaaaa" \
		"aaaaaacccaaaaaaaaaaaaaaa" \
		"aaaaaacccaaaaaaaaaaaaaaa" \
		"aaaaaaccaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa",

		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaccccccaaaaaaaaa" \
		"aaaaaaaaccccccccaaaaaaaa" \
		"aaaaaaacccaaaacccaaaaaaa" \
		"aaaaaaaccaaaaaaccaaaaaaa" \
		"aaaaaaaccaaaaaaccaaaaaaa" \
		"aaaaaaaaccaaaaccaaaaaaaa" \
		"aaaaaaaaaccccccaaaaaaaaa" \
		"aaaaaaaaccccccccaaaaaaaa" \
		"aaaaaaaccaaaaaaccaaaaaaa" \
		"aaaaaaaccaaaaaaccaaaaaaa" \
		"aaaaaaaccaaaaaaccaaaaaaa" \
		"aaaaaaacccaaaacccaaaaaaa" \
		"aaaaaaaaccccccccaaaaaaaa" \
		"aaaaaaaaaccccccaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa",

		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaccccccaaaaaaaaa" \
		"aaaaaaaaccccccccaaaaaaaa" \
		"aaaaaaacccaaaacccaaaaaaa" \
		"aaaaaaaccaaaaaaccaaaaaaa" \
		"aaaaaaaccaaaaaaccaaaaaaa" \
		"aaaaaaaccaaaaaaccaaaaaaa" \
		"aaaaaaaacccaaacccaaaaaaa" \
		"aaaaaaaacccccccccaaaaaaa" \
		"aaaaaaaaaaccccaccaaaaaaa" \
		"aaaaaaaaaaaaaaaccaaaaaaa" \
		"aaaaaaaaaaaaaaaccaaaaaaa" \
		"aaaaaaacccaaaacccaaaaaaa" \
		"aaaaaaaaccccccccaaaaaaaa" \
		"aaaaaaaaaccccccaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa",

		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaccaaaaaaaaaaa" \
		"aaaaaaaaaaccccaaaaaaaaaa" \
		"aaaaaaaaaaccccaaaaaaaaaa" \
		"aaaaaaaaaccccccaaaaaaaaa" \
		"aaaaaaaaaccaaccaaaaaaaaa" \
		"aaaaaaaaaccaaccaaaaaaaaa" \
		"aaaaaaaaccccccccaaaaaaaa" \
		"aaaaaaaaccccccccaaaaaaaa" \
		"aaaaaaaaccaaaaccaaaaaaaa" \
		"aaaaaaacccaaaacccaaaaaaa" \
		"aaaaaaaccaaaaaaccaaaaaaa" \
		"aaaaaaaccaaaaaaccaaaaaaa" \
		"aaaaaacccaaaaaacccaaaaaa" \
		"aaaaaaccaaaaaaaaccaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa",

		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaccccccccaaaaaaaaa" \
		"aaaaaaacccccccccaaaaaaaa" \
		"aaaaaaaccaaaaacccaaaaaaa" \
		"aaaaaaaccaaaaaaccaaaaaaa" \
		"aaaaaaaccaaaaaaccaaaaaaa" \
		"aaaaaaaccaaaaaccaaaaaaaa" \
		"aaaaaaaccccccccaaaaaaaaa" \
		"aaaaaaacccccccccaaaaaaaa" \
		"aaaaaaaccaaaaacccaaaaaaa" \
		"aaaaaaaccaaaaaaccaaaaaaa" \
		"aaaaaaaccaaaaaaccaaaaaaa" \
		"aaaaaaaccaaaaacccaaaaaaa" \
		"aaaaaaacccccccccaaaaaaaa" \
		"aaaaaaaccccccccaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa",

		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaccccccccaaaaaaaa" \
		"aaaaaaaccccccccccaaaaaaa" \
		"aaaaaaaccaaaaaaccaaaaaaa" \
		"aaaaaaaccaaaaaaaaaaaaaaa" \
		"aaaaaaaccaaaaaaaaaaaaaaa" \
		"aaaaaaaccaaaaaaaaaaaaaaa" \
		"aaaaaaaccaaaaaaaaaaaaaaa" \
		"aaaaaaaccaaaaaaaaaaaaaaa" \
		"aaaaaaaccaaaaaaaaaaaaaaa" \
		"aaaaaaaccaaaaaaaaaaaaaaa" \
		"aaaaaaaccaaaaaaaaaaaaaaa" \
		"aaaaaaaccaaaaaaccaaaaaaa" \
		"aaaaaaaccccccccccaaaaaaa" \
		"aaaaaaaaccccccccaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa",

		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaacccccccaaaaaaaaaa" \
		"aaaaaaaccccccccaaaaaaaaa" \
		"aaaaaaaccaaaacccaaaaaaaa" \
		"aaaaaaaccaaaaaccaaaaaaaa" \
		"aaaaaaaccaaaaacccaaaaaaa" \
		"aaaaaaaccaaaaaaccaaaaaaa" \
		"aaaaaaaccaaaaaaccaaaaaaa" \
		"aaaaaaaccaaaaaaccaaaaaaa" \
		"aaaaaaaccaaaaaaccaaaaaaa" \
		"aaaaaaaccaaaaacccaaaaaaa" \
		"aaaaaaaccaaaaaccaaaaaaaa" \
		"aaaaaaaccaaaacccaaaaaaaa" \
		"aaaaaaaccccccccaaaaaaaaa" \
		"aaaaaaacccccccaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa",

		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaccccccccccaaaaaaa" \
		"aaaaaaaccccccccccaaaaaaa" \
		"aaaaaaaccaaaaaaaaaaaaaaa" \
		"aaaaaaaccaaaaaaaaaaaaaaa" \
		"aaaaaaaccaaaaaaaaaaaaaaa" \
		"aaaaaaaccaaaaaaaaaaaaaaa" \
		"aaaaaaaccccccaaaaaaaaaaa" \
		"aaaaaaaccccccaaaaaaaaaaa" \
		"aaaaaaaccaaaaaaaaaaaaaaa" \
		"aaaaaaaccaaaaaaaaaaaaaaa" \
		"aaaaaaaccaaaaaaaaaaaaaaa" \
		"aaaaaaaccaaaaaaaaaaaaaaa" \
		"aaaaaaaccccccccccaaaaaaa" \
		"aaaaaaaccccccccccaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa",

		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaccccccccccaaaaaaa" \
		"aaaaaaaccccccccccaaaaaaa" \
		"aaaaaaaccaaaaaaaaaaaaaaa" \
		"aaaaaaaccaaaaaaaaaaaaaaa" \
		"aaaaaaaccaaaaaaaaaaaaaaa" \
		"aaaaaaaccaaaaaaaaaaaaaaa" \
		"aaaaaaaccccccaaaaaaaaaaa" \
		"aaaaaaaccccccaaaaaaaaaaa" \
		"aaaaaaaccaaaaaaaaaaaaaaa" \
		"aaaaaaaccaaaaaaaaaaaaaaa" \
		"aaaaaaaccaaaaaaaaaaaaaaa" \
		"aaaaaaaccaaaaaaaaaaaaaaa" \
		"aaaaaaaccaaaaaaaaaaaaaaa" \
		"aaaaaaaccaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa",

		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaccaaaaacccccaaaaaa" \
		"aaaaaaccaaaaaccccccaaaaa" \
		"aaaaaccccaaaaccaacccaaaa" \
		"aaaaaccccaaaaccaaaccaaaa" \
		"aaaaccccccaaaccaaaaccaaa" \
		"aaaaccaaccaaaccaaaaccaaa" \
		"aaaaccccccaaaccaaaaccaaa" \
		"aaaccccccccaaccaaaaccaaa" \
		"aaacccaacccaaccaaaaccaaa" \
		"aaaccaaaaccaaccaaaaccaaa" \
		"aaaccaaaaccaaccaaaccaaaa" \
		"aacccaaaacccaccaacccaaaa" \
		"aaccaaaaaaccaccccccaaaaa" \
		"aaccaaaaaaccacccccaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa",

		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaacccccaaaaaaaccaaaaaaa" \
		"aaaccccccaaaaaaccaaaaaaa" \
		"aaaccaacccaaaaccccaaaaaa" \
		"aaaccaaaccaaaaccccaaaaaa" \
		"aaaccaaaaccaaccccccaaaaa" \
		"aaaccaaaaccaaccaaccaaaaa" \
		"aaaccaaaaccaaccccccaaaaa" \
		"aaaccaaaaccaccccccccaaaa" \
		"aaaccaaaaccacccaacccaaaa" \
		"aaaccaaaaccaccaaaaccaaaa" \
		"aaaccaaaccaaccaaaaccaaaa" \
		"aaaccaacccacccaaaacccaaa" \
		"aaaccccccaaccaaaaaaccaaa" \
		"aaacccccaaaccaaaaaaccaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa",

		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaccaaaaaaaaaaa" \
		"aaaaaaaaaaaccaaaaaaaaaaa" \
		"aaaaaaaaaaaccaaaaaaaaaaa" \
		"aaaaaaaaaaaccaaaaaaaaaaa" \
		"aaaaaaaaaaaccaaaaaaaaaaa" \
		"aaaaaaaaaaaccaaaaaaaaaaa" \
		"aaaaaccccccccccccccaaaaa" \
		"aaaaaccccccccccccccaaaaa" \
		"aaaaaaaaaaaccaaaaaaaaaaa" \
		"aaaaaaaaaaaccaaaaaaaaaaa" \
		"aaaaaaaaaaaccaaaaaaaaaaa" \
		"aaaaaaaaaaaccaaaaaaaaaaa" \
		"aaaaaaaaaaaccaaaaaaaaaaa" \
		"aaaaaaaaaaaccaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa",

		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaccccaaaaaaccccaaaaa" \
		"aaaaccccccaaaaccccccaaaa" \
		"aaacccaacccaacccaacccaaa" \
		"aaaccaaaaccaaccaaaaccaaa" \
		"aaaccaaaaaaacccaaaacccaa" \
		"aaaccaaaaaaaccaaaaaaccaa" \
		"aaaccacccccaccaaaaaaccaa" \
		"aaaccacccccaccaaaaaaccaa" \
		"aaaccaaaaccaccaaaaaaccaa" \
		"aaaccaaaaccacccaaaacccaa" \
		"aaaccaaaaccaaccaaaaccaaa" \
		"aaacccaacccaacccaacccaaa" \
		"aaaacccccccaaaccccccaaaa" \
		"aaaaacccaccaaaaccccaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa",

		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaccccccaaaaaaccccaaaaa" \
		"aaacccccccaaaaccccccaaaa" \
		"aaaccaaacccaacccaacccaaa" \
		"aaaccaaaaccaaccaaaaccaaa" \
		"aaaccaaaaccacccaaaaaaaaa" \
		"aaaccaaacccaccaaaaaaaaaa" \
		"aaacccccccaaccaaaaaaaaaa" \
		"aaaccccccaaaccaaaaaaaaaa" \
		"aaaccaaaaaaaccaaaaaaaaaa" \
		"aaaccaaaaaaacccaaaaaaaaa" \
		"aaaccaaaaaaaaccaaaaccaaa" \
		"aaaccaaaaaaaacccaacccaaa" \
		"aaaccaaaaaaaaaccccccaaaa" \
		"aaaccaaaaaaaaaaccccaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa",

		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaccccaaaaccccccccaaa" \
		"aaaaccccccaaaccccccccaaa" \
		"aaacccaacccaaaaaccaaaaaa" \
		"aaaccaaaaccaaaaaccaaaaaa" \
		"aaaccaaaaaaaaaaaccaaaaaa" \
		"aaacccaaaaaaaaaaccaaaaaa" \
		"aaaacccccaaaaaaaccaaaaaa" \
		"aaaaacccccaaaaaaccaaaaaa" \
		"aaaaaaaacccaaaaaccaaaaaa" \
		"aaaaaaaaaccaaaaaccaaaaaa" \
		"aaaccaaaaccaaaaaccaaaaaa" \
		"aaacccaacccaaaaaccaaaaaa" \
		"aaaaccccccaaaaaaccaaaaaa" \
		"aaaaaccccaaaaaaaccaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa",

		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaccccccaaaaaaccccaaaaa" \
		"aaacccccccaaaaccccccaaaa" \
		"aaaccaaacccaacccaacccaaa" \
		"aaaccaaaaccaaccaaaaccaaa" \
		"aaaccaaaaccaaccaaaaaaaaa" \
		"aaaccaaacccaacccaaaaaaaa" \
		"aaacccccccaaaacccccaaaaa" \
		"aaaccccccaaaaaacccccaaaa" \
		"aaaccaacccaaaaaaaacccaaa" \
		"aaaccaaacccaaaaaaaaccaaa" \
		"aaaccaaaaccaaccaaaaccaaa" \
		"aaaccaaaacccacccaacccaaa" \
		"aaaccaaaaaccaaccccccaaaa" \
		"aaaccaaaaaccaaaccccaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa" \
		"aaaaaaaaaaaaaaaaaaaaaaaa",

		"........................" \
		"........................" \
		".bbbbbbbbbbbbbbbbbbbbbb." \
		".baaaaaaaaaaaaaaaaaaaab." \
		".baccccccccaaaaaaaaaaab." \
		".baccccccccaaaaaaaaaaab." \
		".baccccccccaaaaaaaaaaab." \
		".baccccccccaaaaaaaaaaab." \
		".baccccccccaaaaaaaaaaab." \
		".baccccccccaaaaaaaaaaab." \
		".baccccccccaaaaaaaaaaab." \
		".baccccccccaaaaaaaaaaab." \
		".baccccccccaaaaaaaaaaab." \
		".baccccccccaaaaaaaaaaab." \
		".baccccccccaaaaaaaaaaab." \
		".baccccccccaaaaaaaaaaab." \
		".bbbbbbbbbbbbbbbbbbbbbb." \
		"........................"};

	dst = memory_region(REGION_GFX1);
	memset(dst, 0, 128 * 24 * 24 / 8);
	for (i = 0; i < 128; i++)
	{
		for (y = 0; y < 24; y++)
		{
			for (x = 0; x < 18; x++)
			{
				switch (seg7[y * 18 + x])
				{
				case 'a':
					if (i & 1)
						*dst |= 0x80 >> (x & 7);
					break;
				case 'b':
					if (i & 2)
						*dst |= 0x80 >> (x & 7);
					break;
				case 'c':
					if (i & 4)
						*dst |= 0x80 >> (x & 7);
					break;
				case 'd':
					if (i & 8)
						*dst |= 0x80 >> (x & 7);
					break;
				case 'e':
					if (i & 16)
						*dst |= 0x80 >> (x & 7);
					break;
				case 'f':
					if (i & 32)
						*dst |= 0x80 >> (x & 7);
					break;
				case 'g':
					if (i & 64)
						*dst |= 0x80 >> (x & 7);
					break;
				}
				if ((x & 7) == 7)
					dst++;
			}
			dst++;
		}
	}

	dst = memory_region(2);
	memset(dst, 0, 24 * 18 * 24 / 8);
	for (i = 0; i < 24; i++)
	{
		for (y = 0; y < 18; y++)
		{
			for (x = 0; x < 24; x++)
			{
				switch (keys[i][y * 24 + x])
				{
				case 'a':
					*dst |= 0x80 >> ((x & 3) * 2);
					break;
				case 'b':
					*dst |= 0x40 >> ((x & 3) * 2);
					break;
				case 'c':
					*dst |= 0xc0 >> ((x & 3) * 2);
					break;
				}
				if ((x & 3) == 3)
					dst++;
			}
		}
	}
//	artwork_set_overlay(kim1_overlay);
}

static void set_chip_clock(int chip, int data)
{
	timer_adjust(m6530[chip].timer, 0, chip, TIME_IN_HZ((data + 1) * m6530[chip].clock / 256 / 256));
}

MACHINE_RESET( kim1 )
{
	UINT8 *RAM = memory_region(REGION_CPU1);

	/* setup RAM IRQ vector */
	if (RAM[0x17fa] == 0x00 && RAM[0x17fb] == 0x00)
	{
		RAM[0x17fa] = 0x00;
		RAM[0x17fb] = 0x1c;
	}
	/* setup RAM NMI vector */
	if (RAM[0x17fe] == 0x00 && RAM[0x17ff] == 0x00)
	{
		RAM[0x17fe] = 0x00;
		RAM[0x17ff] = 0x1c;
	}

	/* reset the 6530 */
	memset(&m6530, 0, sizeof (m6530));

	m6530[0].dria = 0xff;
	m6530[0].drib = 0xff;
	m6530[0].clock = (double) 1000000 / 1;
	m6530[0].timer = timer_alloc(m6530_timer_cb);
	set_chip_clock(0, 255);

	m6530[1].dria = 0xff;
	m6530[1].drib = 0xff;
	m6530[1].clock = (double) 1000000 / 1;
	m6530[1].timer = timer_alloc(m6530_timer_cb);
	set_chip_clock(1, 255);
}

static DEVICE_LOAD( kim1_cassette )
{
	const char magic[] = "KIM1";
	char buff[4];
	UINT16 addr, size;
	UINT8 ident, *RAM = memory_region(REGION_CPU1);

	image_fread(image, buff, sizeof (buff));
	if (memcmp(buff, magic, sizeof (buff)))
	{
		logerror("kim1_rom_load: magic '%s' not found\n", magic);
		return INIT_FAIL;
	}
	image_fread(image, &addr, 2);
	addr = LITTLE_ENDIANIZE_INT16(addr);
	image_fread(image, &size, 2);
	size = LITTLE_ENDIANIZE_INT16(size);
	image_fread(image, &ident, 1);
	logerror("kim1_rom_load: $%04X $%04X $%02X\n", addr, size, ident);
	while (size-- > 0)
		image_fread(image, &RAM[addr++], 1);
	return INIT_PASS;
}

static void m6530_timer_cb(int chip)
{
	logerror("m6530(%d) timer expired\n", chip);
	m6530[chip].state |= 0x80;
	if (m6530[chip].irqen)			   /* with IRQ? */
		cpunum_set_input_line(0, 0, HOLD_LINE);
}

INTERRUPT_GEN( kim1_interrupt )
{
	int i;

	/* decrease the brightness of the six 7segment LEDs */
	for (i = 0; i < 6; i++)
	{
		if (videoram[i * 2 + 1] > 0)
			videoram[i * 2 + 1] -= 1;
	}
}

INLINE int m6530_r(int chip, int offset)
{
	int data = 0xff;

	switch (offset)
	{
	case 0x00:
	case 0x08:						   /* Data register A */
		if (chip == 1)
		{
			int which = ((m6530[1].drob & m6530[1].ddrb) >> 1) & 0x0f;

			switch (which)
			{
			case 0:				   /* key row 1 */
				m6530[1].dria = input_port_0_r(0);
				logerror("read keybd(%d): %c%c%c%c%c%c%c\n",
					 which,
					 (m6530[1].dria & 0x40) ? '.' : '0',
					 (m6530[1].dria & 0x20) ? '.' : '1',
					 (m6530[1].dria & 0x10) ? '.' : '2',
					 (m6530[1].dria & 0x08) ? '.' : '3',
					 (m6530[1].dria & 0x04) ? '.' : '4',
					 (m6530[1].dria & 0x02) ? '.' : '5',
					 (m6530[1].dria & 0x01) ? '.' : '6');
				break;
			case 1:				   /* key row 2 */
				m6530[1].dria = input_port_1_r(0);
				logerror("read keybd(%d): %c%c%c%c%c%c%c\n",
					 which,
					 (m6530[1].dria & 0x40) ? '.' : '7',
					 (m6530[1].dria & 0x20) ? '.' : '8',
					 (m6530[1].dria & 0x10) ? '.' : '9',
					 (m6530[1].dria & 0x08) ? '.' : 'A',
					 (m6530[1].dria & 0x04) ? '.' : 'B',
					 (m6530[1].dria & 0x02) ? '.' : 'C',
					 (m6530[1].dria & 0x01) ? '.' : 'D');
				break;
			case 2:				   /* key row 3 */
				m6530[1].dria = input_port_2_r(0);
				logerror("read keybd(%d): %c%c%c%c%c%c%c\n",
					 which,
					 (m6530[1].dria & 0x40) ? '.' : 'E',
					 (m6530[1].dria & 0x20) ? '.' : 'F',
					 (m6530[1].dria & 0x10) ? '.' : 'a',
					 (m6530[1].dria & 0x08) ? '.' : 'd',
					 (m6530[1].dria & 0x04) ? '.' : '+',
					 (m6530[1].dria & 0x02) ? '.' : 'g',
					 (m6530[1].dria & 0x01) ? '.' : 'p');
				break;
			case 3:				   /* WR4?? */
				m6530[1].dria = 0xff;
				break;
			default:
				m6530[1].dria = 0xff;
				logerror("read DRA(%d) $ff\n", which);
			}
		}
		data = (m6530[chip].dria & ~m6530[chip].ddra) | (m6530[chip].droa & m6530[chip].ddra);
		logerror("m6530(%d) DRA   read : $%02x\n", chip, data);
		break;
	case 0x01:
	case 0x09:						   /* Data direction register A */
		data = m6530[chip].ddra;
		logerror("m6530(%d) DDRA  read : $%02x\n", chip, data);
		break;
	case 0x02:
	case 0x0a:						   /* Data register B */
		data = (m6530[chip].drib & ~m6530[chip].ddrb) | (m6530[chip].drob & m6530[chip].ddrb);
		logerror("m6530(%d) DRB   read : $%02x\n", chip, data);
		break;
	case 0x03:
	case 0x0b:						   /* Data direction register B */
		data = m6530[chip].ddrb;
		logerror("m6530(%d) DDRB  read : $%02x\n", chip, data);
		break;
	case 0x04:
	case 0x0c:						   /* Timer count read (not supported?) */
		data = (int) (256 * timer_timeleft(m6530[chip].timer) / TIME_IN_HZ(m6530[chip].clock));
		m6530[chip].irqen = (offset & 8) ? 1 : 0;
		logerror("m6530(%d) TIMR  read : $%02x%s\n", chip, data, (offset & 8) ? " (IRQ)" : "");
		break;
	case 0x05:
	case 0x0d:						   /* Timer count read (not supported?) */
		data = (int) (256 * timer_timeleft(m6530[chip].timer) / TIME_IN_HZ(m6530[chip].clock));
		m6530[chip].irqen = (offset & 8) ? 1 : 0;
		logerror("m6530(%d) TIMR  read : $%02x%s\n", chip, data, (offset & 8) ? " (IRQ)" : "");
		break;
	case 0x06:
	case 0x0e:						   /* Timer count read */
		data = (int) (256 * timer_timeleft(m6530[chip].timer) / TIME_IN_HZ(m6530[chip].clock));
		m6530[chip].irqen = (offset & 8) ? 1 : 0;
		logerror("m6530(%d) TIMR  read : $%02x%s\n", chip, data, (offset & 8) ? " (IRQ)" : "");
		break;
	case 0x07:
	case 0x0f:						   /* Timer status read */
		data = m6530[chip].state;
		m6530[chip].state &= ~0x80;
		m6530[chip].irqen = (offset & 8) ? 1 : 0;
		logerror("m6530(%d) STAT  read : $%02x%s\n", chip, data, (offset & 8) ? " (IRQ)" : "");
		break;
	}
	return data;
}

 READ8_HANDLER ( m6530_003_r )
{
	return m6530_r(0, offset);
}
 READ8_HANDLER ( m6530_002_r )
{
	return m6530_r(1, offset);
}

static void m6530_w(int chip, int offset, int data)
{
	switch (offset)
	{
	case 0x00:
	case 0x08:						   /* Data register A */
		logerror("m6530(%d) DRA  write: $%02x\n", chip, data);
		m6530[chip].droa = data;
		if (chip == 1)
		{
			int which = (m6530[1].drob & m6530[1].ddrb) >> 1;

			switch (which)
			{
			case 0:				   /* key row 1 */
				break;
			case 1:				   /* key row 2 */
				break;
			case 2:				   /* key row 3 */
				break;
			case 3:				   /* WR4?? */
				break;
				/* write LED # 1-6 */
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
				if (data & 0x80)
				{
					logerror("write 7seg(%d): %c%c%c%c%c%c%c\n",
						 which + 1 - 4,
						 (data & 0x01) ? 'a' : '.',
						 (data & 0x02) ? 'b' : '.',
						 (data & 0x04) ? 'c' : '.',
						 (data & 0x08) ? 'd' : '.',
						 (data & 0x10) ? 'e' : '.',
						 (data & 0x20) ? 'f' : '.',
						 (data & 0x40) ? 'g' : '.');
					videoram[(which - 4) * 2 + 0] = data & 0x7f;
					videoram[(which - 4) * 2 + 1] = 15;
				}
			}
		}
		break;
	case 0x01:
	case 0x09:						   /* Data direction register A */
		logerror("m6530(%d) DDRA  write: $%02x\n", chip, data);
		m6530[chip].ddra = data;
		break;
	case 0x02:
	case 0x0a:						   /* Data register B */
		logerror("m6530(%d) DRB   write: $%02x\n", chip, data);
		m6530[chip].drob = data;
		if (chip == 1)
		{
			int which = m6530[1].ddrb & m6530[1].drob;

			if ((which & 0x3f) == 0x27)
			{
				/* This is the cassette output port */
				logerror("write cassette port: %d\n", (which & 0x80) ? 1 : 0);
				DAC_signed_data_w(0, (which & 0x80) ? 255 : 0);
			}
		}
		break;
	case 0x03:
	case 0x0b:						   /* Data direction register B */
		logerror("m6530(%d) DDRB  write: $%02x\n", chip, data);
		m6530[chip].ddrb = data;
		break;
	case 0x04:
	case 0x0c:						   /* Timer 1 start */
		logerror("m6530(%d) TMR1  write: $%02x%s\n", chip, data, (offset & 8) ? " (IRQ)" : "");
		m6530[chip].state &= ~0x80;
		m6530[chip].irqen = (offset & 8) ? 1 : 0;
		m6530[chip].clock = (double) 1000000 / 1;
		set_chip_clock(chip, data);
		break;
	case 0x05:
	case 0x0d:						   /* Timer 8 start */
		logerror("m6530(%d) TMR8  write: $%02x%s\n", chip, data, (offset & 8) ? " (IRQ)" : "");
		m6530[chip].state &= ~0x80;
		m6530[chip].irqen = (offset & 8) ? 1 : 0;
		m6530[chip].clock = (double) 1000000 / 8;
		set_chip_clock(chip, data);
		break;
	case 0x06:
	case 0x0e:						   /* Timer 64 start */
		logerror("m6530(%d) TMR64 write: $%02x%s\n", chip, data, (offset & 8) ? " (IRQ)" : "");
		m6530[chip].state &= ~0x80;
		m6530[chip].irqen = (offset & 8) ? 1 : 0;
		m6530[chip].clock = (double) 1000000 / 64;
		set_chip_clock(chip, data);
		break;
	case 0x07:
	case 0x0f:						   /* Timer 1024 start */
		logerror("m6530(%d) TMR1K write: $%02x%s\n", chip, data, (offset & 8) ? " (IRQ)" : "");
		m6530[chip].state &= ~0x80;
		m6530[chip].irqen = (offset & 8) ? 1 : 0;
		m6530[chip].clock = (double) 1000000 / 1024;
		set_chip_clock(chip, data);
		break;
	}
}

WRITE8_HANDLER ( m6530_003_w )
{
	m6530_w(0, offset, data);
}
WRITE8_HANDLER ( m6530_002_w )
{
	m6530_w(1, offset, data);
}

void kim1_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:							info->i = IO_CASSETTE; break;
		case DEVINFO_INT_READABLE:						info->i = 1; break;
		case DEVINFO_INT_WRITEABLE:						info->i = 0; break;
		case DEVINFO_INT_CREATABLE:						info->i = 0; break;
		case DEVINFO_INT_COUNT:							info->i = 1; break;
		case DEVINFO_INT_RESET_ON_LOAD:					info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_kim1_cassette; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "kim1"); break;
	}
}


