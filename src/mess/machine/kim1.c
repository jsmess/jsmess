/******************************************************************************
	KIM-1

	machine Driver

	Juergen Buchmueller, Jan 2000

******************************************************************************/

#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "includes/kim1.h"
#include "sound/dac.h"

DRIVER_INIT( kim1 )
{
	UINT8 *dst;
	int x, y, i;

	static const char seg7[] =
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


	static const char *const keys[24] =
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

	dst = memory_region(machine, "gfx1");
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

	dst = memory_region(machine, "gfx2");
	memset(dst, 0, memory_region_length(machine, "gfx2"));
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


MACHINE_RESET( kim1 )
{
	UINT8 *RAM = memory_region(machine, "main");

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
}

static DEVICE_IMAGE_LOAD( kim1_cassette )
{
	const char magic[] = "KIM1";
	char buff[4];
	UINT16 addr, size;
	UINT8 ident, *RAM = memory_region(image->machine, "main");

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


void kim1_cassette_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_TYPE:							info->i = IO_CASSETTE; break;
		case MESS_DEVINFO_INT_READABLE:						info->i = 1; break;
		case MESS_DEVINFO_INT_WRITEABLE:						info->i = 0; break;
		case MESS_DEVINFO_INT_CREATABLE:						info->i = 0; break;
		case MESS_DEVINFO_INT_COUNT:							info->i = 1; break;
		case MESS_DEVINFO_INT_RESET_ON_LOAD:					info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:							info->load = DEVICE_IMAGE_LOAD_NAME(kim1_cassette); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "kim1"); break;
	}
}


