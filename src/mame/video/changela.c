/**************************************************************************
Change Lanes - Video Hardware
(C) Taito 1983

Jarek Burczynski
Phil Stroffolino
Tomasz Slanina
Adam Bousley

***************************************************************************/

#include "driver.h"


static UINT32 slopeROM_bank;
static UINT32 address;

static UINT8 * memory_devices;
static UINT32  mem_dev_selected; /* an offset within memory_devices area */


VIDEO_START( changela )
{
	memory_devices = auto_malloc(4 * 0x800); /* 0 - not connected, 1,2,3 - RAMs*/
}

/**************************************************************************

    Obj 0 - Sprite Layer

***************************************************************************/
static void draw_obj0(running_machine *machine, mame_bitmap *bitmap)
{
	int sx, sy, i;

	UINT8* ROM = memory_region(REGION_USER1);
	UINT8* RAM = spriteram;

	for(sy = 0; sy < 256; sy++)
	{
		for(sx = 0; sx < 256; sx++)
		{
			int vr = (RAM[sx*4 + 0] & 0x80) >> 7;
			int hr = (RAM[sx*4 + 0] & 0x40) >> 6;
			int hs = (RAM[sx*4 + 0] & 0x20) >> 5;
			UINT32 vsize = RAM[sx*4 + 0] & 0x1f;
			UINT8 ypos = ~RAM[sx*4 + 1];
			UINT8 tile = RAM[sx*4 + 2];
			UINT8 xpos = RAM[sx*4 + 3];

			if(sy - ypos <= vsize)
			{
				for(i = 0; i < 16; i++)
				{
					UINT32 A7, A8, rom_addr;
					UINT8 counter, data;
					UINT8 sum = sy - ypos;

					counter = i;
					if(hr) counter ^= 0x0f;

					A8 = ((tile & 0x02) >> 1) ^ ((hr & hs) ^ hs);
					A7 = ( (((vr ^ ((sum & 0x10) >> 4)) & ((vsize & 0x10) >> 4)) ^ 0x01) & (tile & 0x01) ) ^ 0x01;
					rom_addr = (counter >> 1) | ((sum & 0x0f) << 3) | (A7 << 7) | (A8 << 8) | ((tile >> 2) << 9);
					if(vr) rom_addr ^= (0x0f << 3);

					if(counter & 1)
						data = ROM[rom_addr] & 0x0f;
					else
						data = (ROM[rom_addr] & 0xf0) >> 4;

					if((data != 0x0f) && (data != 0))
						*BITMAP_ADDR16(bitmap, sy, xpos+i) = machine->pens[16+data];

					if(hs)
					{
						if(counter & 1)
							data = ROM[rom_addr ^ 0x100] & 0x0f;
						else
							data = (ROM[rom_addr ^ 0x100] & 0xf0) >> 4;

						if((data != 0x0f) && (data != 0))
							*BITMAP_ADDR16(bitmap, sy, xpos+i+16) = machine->pens[16+data];
					}
				}
			}
		}
	}
}


/**************************************************************************

    Obj 1 - Text Layer

***************************************************************************/
static void draw_obj1(running_machine *machine, mame_bitmap *bitmap)
{
	int sx, sy;

	UINT8* ROM = memory_region(REGION_GFX2);
	UINT8* RAM = videoram;

	UINT8 reg[4] = { 0 }; /* 4x4-bit registers (U58, U59) */

	UINT8 tile;
	UINT8 attrib = 0;

	for(sy = 0; sy < 256; sy++)
	{
		for(sx = 0; sx < 256; sx++)
		{
			int c0, c1, col, sum;

			/* 11 Bits: H1, H3, H4, H5, H6, H7, V3, V4, V5, V6, V7 */
			int ram_addr = ((sx & 0xf8) >> 2) | ((sy & 0xf8) << 3);
			int tile_addr = RAM[ram_addr];

			if(!(RAM[ram_addr+1] & 0x10) && (sx & 0x04)) /* D4=0 enables latch at U32 */
				attrib = RAM[ram_addr+1];

			tile = ROM[(tile_addr << 4) | ((sx & 0x04) >> 2) | ((sy & 0x07) << 1)];
			reg[(sx & 0x0c) >> 2] = tile;
			sum = (sx & 0x0f) + (attrib & 0x0f); /* 4-bit adder (U45) */

			/* Multiplexors (U57) */
			if((sum & 0x03) == 0)
			{
				c0 = (reg[(sum & 0x0c) >> 2] & 0x08) >> 3;
				c1 = (reg[(sum & 0x0c) >> 2] & 0x80) >> 7;
			}
			else if((sum & 0x03) == 1)
			{
				c0 = (reg[(sum & 0x0c) >> 2] & 0x04) >> 2;
				c1 = (reg[(sum & 0x0c) >> 2] & 0x40) >> 6;
			}
			else if((sum & 0x03) == 2)
			{
				c0 = (reg[(sum & 0x0c) >> 2] & 0x02) >> 1;
				c1 = (reg[(sum & 0x0c) >> 2] & 0x20) >> 5;
			}
			else
			{
				c0 = (reg[(sum & 0x0c) >> 2] & 0x01) >> 0;
				c1 = (reg[(sum & 0x0c) >> 2] & 0x10) >> 4;
			}

			col = c0 | (c1 << 1) | ((attrib & 0xc0) >> 4);
			if((col & 0x07) != 0x07)
				*BITMAP_ADDR16(bitmap, sy, sx) = machine->pens[col | 0x20];
		}
	}
}



/*
--+-------------------+-----------------------------------------------------+-----------------------------------------------------------------
St|  PROM contents:   |                Main signals:                        |                      DESCRIPTION
at+-------------------+-----------------------------------------------------+-----------------------------------------------------------------
e:|7 6 5 4 3 2 1 0 Hex|/RAMw /RAMr /ROM  /AdderOutput  AdderInput TrainInputs|
  |                   |           enable GateU61Enable Enable     Enable    |
--+-------------------+-----------------------------------------------------+-----------------------------------------------------------------
00|0 0 0 0 1 1 0 1 0d | 1     1      0      1            0          1       |                                       (noop ROM 00-lsb to adder)
01|0 0 0 0 1 1 1 1 0f | 0     1      0      1            1          0       |   ROM 00-lsb to train, and to RAM 00
02|0 1 0 0 1 1 0 1 4d | 1     0      1      1            0          1       |                                       (noop RAM 00 to adder)
03|0 0 1 0 1 1 1 1 2f | 0     1      0      1            1          0       |   ROM 00-msb to train, and to RAM 01
04|1 1 0 0 1 1 0 1 cd | 1     0      1      1            0          1       |                                       (noop RAM 00 to adder)
05|0 0 0 0 1 1 1 1 0f | 0     1      0      1            1          0       |   ROM 01-lsb to train, and to RAM 02
06|0 1 0 0 1 1 0 1 4d | 1     0      1      1            0          1       |                                       (noop RAM 02 to adder)
07|0 0 1 0 1 1 1 1 2f | 0     1      0      1            1          0       |   ROM 01-msb to train, and to RAM 03
08|1 1 0 0 0 1 0 1 c5 | 1     0      1      1            0          1       |   CLR carry
09|0 0 0 0 1 1 0 1 0d | 1     1      0      1            0          1       |   ROM 02-lsb to adder
0a|0 1 1 0 1 1 0 1 6d | 1     0      1      1            0          1       |   RAM 05 to adder
0b|1 1 1 0 1 1 1 0 ee | 0     1      1      0            1          0       |   Adder to train, and to RAM 05, CLOCK carry
0c|0 0 0 0 1 1 0 1 0d | 1     1      0      1            0          1       |   ROM 02-msb to adder
0d|0 1 1 0 1 1 0 1 6d | 1     0      1      1            0          1       |   RAM 07 to adder
0e|1 1 1 0 1 1 1 0 ee | 0     1      1      0            1          0       |   Adder to train, and to RAM 07, CLOCK carry
0f|0 0 0 0 1 1 0 1 0d | 1     1      0      1            0          1       |   ROM 03-lsb to adder
10|0 1 1 0 1 1 0 1 6d | 1     0      1      1            0          1       |   RAM 09 to adder
11|1 1 1 0 1 1 1 0 ee | 0     1      1      0            1          0       |   Adder to train, and to RAM 09, CLOCK carry
12|1 0 0 0 1 1 0 1 8d | 1     1      0      1            0          1       |                                       (noop ROM 03-msb to adder)
13|0 1 0 0 1 1 0 1 4d | 1     0      1      1            0          1       |                                       (noop RAM 0c to adder)
14|0 0 0 0 0 0 0 1 01 | 1     1      0      1            0          1       |   ROM 04-lsb to adder, CLR carry
15|0 1 1 0 1 0 0 1 69 | 1     0      1      1            0          1       |   RAM 0d to adder
16|1 1 1 0 1 0 1 0 ea | 0     1      1      0            1          0       |   Adder to train and to RAM 0d, CLOCK carry
17|0 0 0 0 1 0 0 1 09 | 1     1      0      1            0          1       |   ROM 04-msb to adder
18|0 1 1 0 1 0 0 1 69 | 1     0      1      1            0          1       |   RAM 0f to adder
19|1 1 1 0 1 0 1 0 ea | 0     1      1      0            1          0       |   Adder to train and to RAM 0f, CLOCK carry
1a|0 0 0 1 1 0 0 1 19 | 1     1      0      1            0          1       |   ROM 05-lsb to adder, /LD HOSC
1b|0 1 1 0 1 0 0 1 69 | 1     0      1      1            0          1       |   RAM 11 to adder
1c|1 1 1 0 1 0 1 0 ea | 0     1      1      0            1          0       |   Adder to train and to RAM 11, CLOCK carry
1d|0 0 0 0 1 0 0 1 09 | 1     1      0      1            0          1       |   ROM 05-msb to adder
1e|0 1 1 0 1 0 0 1 69 | 1     0      1      1            0          1       |   RAM 13 to adder
1f|1 1 1 0 1 0 1 0 ea | 0     1      1      0            1          0       |   Adder to train and to RAM 13, CLOCK carry
                        *   =========================   ====================
                        *   only one of these signals   these signals select
                        *   can be active at a time     the output for the result
                        *    ------- SOURCE --------     ----- TARGET -----
                        *
                 ******************
                 result needs to be
                 written back to RAM
*/


static UINT8 train[16];
static UINT8 ls195_latch;

VIDEO_UPDATE( changela )
{
	int i;
	int sx,sy;
	const UINT8 *ROM1;
	const UINT8 *ROM111;
	UINT8 *RAM1;
	UINT8 *RAM2;

	UINT8 ls377_u110 = 0;
	int carry, add1, add2;


	fillbitmap( bitmap, 0x00, cliprect );


	RAM1 = memory_devices + 0x0800;
	RAM2 = memory_devices + 0x1000;
	ROM1 = memory_region(REGION_USER2) + slopeROM_bank + ((address&0x7e)<<2);
	ROM111 = memory_region(REGION_GFX1) + 0;

	ls195_latch = 0;

logerror("bank=%4x addr=%4x\n", slopeROM_bank, address);

    /* /PRELOAD */
	/* address = 0xff */

		/* step 1: ROM 00-lsb to train, and to RAM 00 */
		i =  ROM1[0] & 0x0f;
		train[0] = RAM1[0x20] = i;
		/* step 3: ROM 00-msb to train, and to RAM 01 */
		i = (ROM1[0]>>4) & 0x0f;
		train[1] = RAM1[0x21] = i;
		/* step 5: ROM 01-lsb to train, and to RAM 02 */
		i =  ROM1[1] & 0x0f;
		train[2] = RAM1[0x22] = i;
		/* step 7: ROM 01-msb to train, and to RAM 03 */
		i = (ROM1[1]>>4) & 0x0f;
		train[3] = RAM1[0x23] = i;

		/* step 8: CLEAR CARRY */
		carry = 0;

		/* step 9: ROM 02-lsb to adder */
		add1 = ROM1[2] & 0x0f;
		/* step a: RAM 05 to adder */
		add2 = RAM1[0x5] & 0x0f;
		/* step b: Adder to train, and to RAM 05, CLOCK carry */
		train[4] = RAM1[0x25] = ((add2) & 0x0f);
		carry = ((add1+add2+carry) & 0x10) >> 4;

		/* step c: ROM 02-msb to adder */
		add1 = (ROM1[2]>>4) & 0x0f;
		/* step d: RAM 07 to adder */
		add2 = RAM1[0x7] & 0x0f;
		/* step e: Adder to train, and to RAM 07, CLOCK carry */
		train[5] = RAM1[0x27] = ((add2) & 0x0f);
		carry = ((add1+add2+carry) & 0x10) >> 4;

		/* step f: ROM 03-lsb to adder */
		add1 = ROM1[3] & 0x0f;
		/* step10: RAM 09 to adder */
		add2 = RAM1[0x9] & 0x0f;
		/* step11: Adder to train, and to RAM 09, CLOCK carry */
		train[6] = RAM1[0x29] = ((add2) & 0x0f);
		carry = ((add1+add2+carry) & 0x10) >> 4;

		/* step14: ROM 04-lsb to adder, CLR carry */
		carry = 0;
		add1 = ROM1[4] & 0x0f;
		/* step15: RAM 0d to adder */
		add2 = RAM1[0xd] & 0x0f;
		/* step16: Adder to train and to RAM 0d, CLOCK carry */
		train[7] = RAM1[0x2d] = ((add2) & 0x0f);
		carry = ((add1+add2+carry) & 0x10) >> 4;

		/* step17: ROM 04-msb to adder */
		add1 = (ROM1[4]>>4) & 0x0f;
		/* step18: RAM 0f to adder */
		add2 = RAM1[0xf] & 0x0f;
		/* step19: Adder to train and to RAM 0f, CLOCK carry */
		train[8] = RAM1[0x2f] = ((add2) & 0x0f);
		carry = ((add1+add2+carry) & 0x10) >> 4;

		/* step1a: ROM 05-lsb to adder, /LD HOSC */
/* HOSC */

		add1 = ROM1[5] & 0x0f;
		/* step1b: RAM 11 to adder */
		add2 = RAM1[0x11] & 0x0f;
		/* step1c: Adder to train and to RAM 11, CLOCK carry */
		train[9] = RAM1[0x31] = ((add2) & 0x0f);
		carry = ((add1+add2+carry) & 0x10) >> 4;

		/* step1d: ROM 05-msb to adder */
		add1 = (ROM1[5]>>4) & 0x0f;
		/* step1e: RAM 13 to adder */
		add2 = RAM1[0x13] & 0x0f;
		/* step1f: Adder to train and to RAM 13, CLOCK carry */
		train[10] = RAM1[0x33] = ((add2) & 0x0f);
		carry = ((add1+add2+carry) & 0x10) >> 4;


    /* /PRELOAD */
	/* address = 0x00 */

		/* step 1: ROM 00-lsb to train, and to RAM 00 */
		i =  ROM1[0] & 0x0f;
		train[0] = RAM1[0x20] = i;
		/* step 3: ROM 00-msb to train, and to RAM 01 */
		i = (ROM1[0]>>4) & 0x0f;
		train[1] = RAM1[0x21] = i;
		/* step 5: ROM 01-lsb to train, and to RAM 02 */
		i =  ROM1[1] & 0x0f;
		train[2] = RAM1[0x22] = i;
		/* step 7: ROM 01-msb to train, and to RAM 03 */
		i = (ROM1[1]>>4) & 0x0f;
		train[3] = RAM1[0x23] = i;

		/* step 8: CLEAR CARRY */
		carry = 0;

		/* step 9: ROM 02-lsb to adder */
		add1 = ROM1[2] & 0x0f;
		/* step a: RAM 05 to adder */
		add2 = RAM1[0x5] & 0x0f;
		/* step b: Adder to train, and to RAM 05, CLOCK carry */
		train[4] = RAM1[0x25] = ((add1+add2+carry) & 0x0f);
		carry = ((add1+add2+carry) & 0x10) >> 4;

		/* step c: ROM 02-msb to adder */
		add1 = (ROM1[2]>>4) & 0x0f;
		/* step d: RAM 07 to adder */
		add2 = RAM1[0x7] & 0x0f;
		/* step e: Adder to train, and to RAM 07, CLOCK carry */
		train[5] = RAM1[0x27] = ((add1+add2+carry) & 0x0f);
		carry = ((add1+add2+carry) & 0x10) >> 4;

		/* step f: ROM 03-lsb to adder */
		add1 = ROM1[3] & 0x0f;
		/* step10: RAM 09 to adder */
		add2 = RAM1[0x9] & 0x0f;
		/* step11: Adder to train, and to RAM 09, CLOCK carry */
		train[6] = RAM1[0x29] = ((add1+add2+carry) & 0x0f);
		carry = ((add1+add2+carry) & 0x10) >> 4;

		/* step14: ROM 04-lsb to adder, CLR carry */
		carry = 0;
		add1 = ROM1[4] & 0x0f;
		/* step15: RAM 0d to adder */
		add2 = RAM1[0xd] & 0x0f;
		/* step16: Adder to train and to RAM 0d, CLOCK carry */
		train[7] = RAM1[0x2d] = ((add1+add2+carry) & 0x0f);
		carry = ((add1+add2+carry) & 0x10) >> 4;

		/* step17: ROM 04-msb to adder */
		add1 = (ROM1[4]>>4) & 0x0f;
		/* step18: RAM 0f to adder */
		add2 = RAM1[0xf] & 0x0f;
		/* step19: Adder to train and to RAM 0f, CLOCK carry */
		train[8] = RAM1[0x2f] = ((add1+add2+carry) & 0x0f);
		carry = ((add1+add2+carry) & 0x10) >> 4;

		/* step1a: ROM 05-lsb to adder, /LD HOSC */
/* HOSC */

		add1 = ROM1[5] & 0x0f;
		/* step1b: RAM 11 to adder */
		add2 = RAM1[0x11] & 0x0f;
		/* step1c: Adder to train and to RAM 11, CLOCK carry */
		train[9] = RAM1[0x31] = ((add1+add2+carry) & 0x0f);
		carry = ((add1+add2+carry) & 0x10) >> 4;

		/* step1d: ROM 05-msb to adder */
		add1 = (ROM1[5]>>4) & 0x0f;
		/* step1e: RAM 13 to adder */
		add2 = RAM1[0x13] & 0x0f;
		/* step1f: Adder to train and to RAM 13, CLOCK carry */
		train[10] = RAM1[0x33] = ((add1+add2+carry) & 0x0f);
		carry = ((add1+add2+carry) & 0x10) >> 4;


	ROM1 = memory_region(REGION_USER2) + slopeROM_bank + (((address+1)&0x7e)<<2);


	logerror("addrlow=%4x (addr=%4x)\n", (((address+1)&0x7e)<<2), address );

	for (sy = 0; sy < 128; sy++)
	{
		int ls163_u107;

	/* compute the math train */

		/* step 1: ROM 00-lsb to train, and to RAM 00 */
		i =  ROM1[0] & 0x0f;
		train[0] = RAM1[0x20] = i;
		/* step 3: ROM 00-msb to train, and to RAM 01 */
		i = (ROM1[0]>>4) & 0x0f;
		train[1] = RAM1[0x21] = i;
		/* step 5: ROM 01-lsb to train, and to RAM 02 */
		i =  ROM1[1] & 0x0f;
		train[2] = RAM1[0x22] = i;
		/* step 7: ROM 01-msb to train, and to RAM 03 */
		i = (ROM1[1]>>4) & 0x0f;
		train[3] = RAM1[0x23] = i;


	if (sy&1)
	{
		/* step 8: CLEAR CARRY */
		carry = 0;

		/* step 9: ROM 02-lsb to adder */
		add1 = ROM1[2] & 0x0f;
		/* step a: RAM 05 to adder */
		add2 = RAM1[0x25] & 0x0f;
		/* step b: add2 to train, and to RAM 05, CLOCK carry */
		train[4] = RAM1[0x25] = ((add2) & 0x0f);
		carry = ((add1+add2+carry) & 0x10) >> 4;

		/* step c: ROM 02-msb to adder */
		add1 = (ROM1[2]>>4) & 0x0f;
		/* step d: RAM 07 to adder */
		add2 = RAM1[0x27] & 0x0f;
		/* step e: add2 to train, and to RAM 07, CLOCK carry */
		train[5] = RAM1[0x27] = ((add2) & 0x0f);
		carry = ((add1+add2+carry) & 0x10) >> 4;

		/* step f: ROM 03-lsb to adder */
		add1 = ROM1[3] & 0x0f;
		/* step10: RAM 09 to adder */
		add2 = RAM1[0x29] & 0x0f;
		/* step11: Adder to train, and to RAM 09, CLOCK carry */
		train[6] = RAM1[0x29] = ((add2) & 0x0f);
		carry = ((add1+add2+carry) & 0x10) >> 4;

		/* step14: ROM 04-lsb to adder, CLR carry */
		carry = 0;
		add1 = ROM1[4] & 0x0f;
		/* step15: RAM 0d to adder */
		add2 = RAM1[0x2d] & 0x0f;
		/* step16: Adder to train and to RAM 0d, CLOCK carry */
		train[7] = RAM1[0x2d] = ((add2) & 0x0f);
		carry = ((add1+add2+carry) & 0x10) >> 4;

		/* step17: ROM 04-msb to adder */
		add1 = (ROM1[4]>>4) & 0x0f;
		/* step18: RAM 0f to adder */
		add2 = RAM1[0x2f] & 0x0f;
		/* step19: Adder to train and to RAM 0f, CLOCK carry */
		train[8] = RAM1[0x2f] = ((add2) & 0x0f);
		carry = ((add1+add2+carry) & 0x10) >> 4;

		/* step1a: ROM 05-lsb to adder, /LD HOSC */
		/* HOSC */

		add1 = ROM1[5] & 0x0f;
		/* step1b: RAM 11 to adder */
		add2 = RAM1[0x31] & 0x0f;
		/* step1c: Adder to train and to RAM 11, CLOCK carry */
		train[9] = RAM1[0x31] = ((add2) & 0x0f);
		carry = ((add1+add2+carry) & 0x10) >> 4;

		/* step1d: ROM 05-msb to adder */
		add1 = (ROM1[5]>>4) & 0x0f;
		/* step1e: RAM 13 to adder */
		add2 = RAM1[0x33] & 0x0f;
		/* step1f: Adder to train and to RAM 13, CLOCK carry */
		train[10] = RAM1[0x33] = ((add2) & 0x0f);
		carry = ((add1+add2+carry) & 0x10) >> 4;
	}
	else
	{
		/* step 8: CLEAR CARRY */
		carry = 0;

		/* step 9: ROM 02-lsb to adder */
		add1 = ROM1[2] & 0x0f;
		/* step a: RAM 05 to adder */
		add2 = RAM1[0x25] & 0x0f;
		/* step b: Adder to train, and to RAM 05, CLOCK carry */
		train[4] = RAM1[0x25] = ((add1+add2+carry) & 0x0f);
		carry = ((add1+add2+carry) & 0x10) >> 4;

		/* step c: ROM 02-msb to adder */
		add1 = (ROM1[2]>>4) & 0x0f;
		/* step d: RAM 07 to adder */
		add2 = RAM1[0x27] & 0x0f;
		/* step e: Adder to train, and to RAM 07, CLOCK carry */
		train[5] = RAM1[0x27] = ((add1+add2+carry) & 0x0f);
		carry = ((add1+add2+carry) & 0x10) >> 4;

		/* step f: ROM 03-lsb to adder */
		add1 = ROM1[3] & 0x0f;
		/* step10: RAM 09 to adder */
		add2 = RAM1[0x29] & 0x0f;
		/* step11: Adder to train, and to RAM 09, CLOCK carry */
		train[6] = RAM1[0x29] = ((add1+add2+carry) & 0x0f);
		carry = ((add1+add2+carry) & 0x10) >> 4;

		/* step14: ROM 04-lsb to adder, CLR carry */
		carry = 0;
		add1 = ROM1[4] & 0x0f;
		/* step15: RAM 0d to adder */
		add2 = RAM1[0x2d] & 0x0f;
		/* step16: Adder to train and to RAM 0d, CLOCK carry */
		train[7] = RAM1[0x2d] = ((add1+add2+carry) & 0x0f);
		carry = ((add1+add2+carry) & 0x10) >> 4;

		/* step17: ROM 04-msb to adder */
		add1 = (ROM1[4]>>4) & 0x0f;
		/* step18: RAM 0f to adder */
		add2 = RAM1[0x2f] & 0x0f;
		/* step19: Adder to train and to RAM 0f, CLOCK carry */
		train[8] = RAM1[0x2f] = ((add1+add2+carry) & 0x0f);
		carry = ((add1+add2+carry) & 0x10) >> 4;

		/* step1a: ROM 05-lsb to adder, /LD HOSC */
		/* HOSC */

		add1 = ROM1[5] & 0x0f;
		/* step1b: RAM 11 to adder */
		add2 = RAM1[0x31] & 0x0f;
		/* step1c: Adder to train and to RAM 11, CLOCK carry */
		train[9] = RAM1[0x31] = ((add1+add2+carry) & 0x0f);
		carry = ((add1+add2+carry) & 0x10) >> 4;

		/* step1d: ROM 05-msb to adder */
		add1 = (ROM1[5]>>4) & 0x0f;
		/* step1e: RAM 13 to adder */
		add2 = RAM1[0x33] & 0x0f;
		/* step1f: Adder to train and to RAM 13, CLOCK carry */
		train[10] = RAM1[0x33] = ((add1+add2+carry) & 0x0f);
		carry = ((add1+add2+carry) & 0x10) >> 4;
	}



		/* "burst of 16 10 MHz clocks" */
		for (sx = 0; sx < 16; sx++)
		{
			int tmph,tmpv;

			tmpv = (train[8]<<0) | (train[9]<<4) | (train[10]<<8);
			tmpv>>=2;

			tmph = (train[4]<<0) | (train[5]<<4) | (train[6]<<8);
					/* RWCLK */
					train[4]++;
					if (train[4] > 15)
					{
						train[4] &= 15;
						train[5]++;
						if (train[5] > 15)
						{
							train[5] &= 15;
							train[6]++;
							train[6] &= 15;
						}
					}
					tmph = (train[4]<<0) | (train[5]<<4) | (train[6]<<8);

					if ((tmph&7)==7)
						ls377_u110 = RAM2[((tmph&0x1f8)>>3) | ((tmpv&0x1f0)<<2)];

					if ((tmph&1)==0)
						ls195_latch = ROM111[((tmph>>1)&3) | ((tmpv&15)<<2) | ((ls377_u110&0x7f)<<6) ];
					else
						ls195_latch>>=4;
		}



		/* draw the scanline */

		ls163_u107 = 0x80 | ((train[0] | (train[1]<<4)) >> 1);

		for (sx = 0; sx < 256; sx++)
		{
			int tmph,tmpv;

			tmpv = (train[8]<<0) | (train[9]<<4) | (train[10]<<8);
			tmpv>>=2;
			tmph = (train[4]<<0) | (train[5]<<4) | (train[6]<<8);

			ls163_u107 += 4; /* ls163s clocked at 20MHz = 4 clocks per pixel */

			if (ls163_u107 >= 256)
			{
				while (ls163_u107 >= 256)
				{
					ls163_u107-=256;
					/* RWCLK */
					train[4]++;
					if (train[4] > 15)
					{
						train[4] &= 15;
						train[5]++;
						if (train[5] > 15)
						{
							train[5] &= 15;
							train[6]++;
							train[6] &= 15;
						}
					}
					ls163_u107+= (0x80 | ((train[2] | (train[3]<<4)) >> 1));

					tmph = (train[4]<<0) | (train[5]<<4) | (train[6]<<8);

					if ((tmph&7)==7)
						ls377_u110 = RAM2[((tmph&0x1f8)>>3) | ((tmpv&0x1f0)<<2)];

					if ((tmph&1)==0)
						ls195_latch = ROM111[((tmph>>1)&3) | ((tmpv&15)<<2) | ((ls377_u110&0x7f)<<6) ];
					else
						ls195_latch>>=4;
				}
				if (ls163_u107 >= 256)
					popmessage("ERROR!!!!");
			}

			if ( (sy>=cliprect->min_y) && (sy<=cliprect->max_y) )
			{
				int data  = (ls195_latch>>0) & 0x0f;
				*BITMAP_ADDR16(bitmap, sy, sx) = machine->pens[data];
			}
		}

		if (sy&1)
			ROM1+=8;	/* every second line */
	}


	/* OBJ 0 - sprites */
	draw_obj0(machine, bitmap);

	/* OBJ 1 - text */
	draw_obj1(machine, bitmap);

	return 0;
}


WRITE8_HANDLER( changela_colors_w )
{
	int bit0, bit1, bit2, r, g, b;
	UINT32 c, color_index;

	c = (data) | ((offset&1)<<8); /* a0 used as D8 bit input */

	c ^= 0x1ff; /* active low */

	color_index = offset/2;
	color_index ^=0x30;	/* A4 and A5 lines are negated */

	/* red component */
	bit0 = (c >> 0) & 0x01;
	bit1 = (c >> 1) & 0x01;
	bit2 = (c >> 2) & 0x01;
	r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

	/* green component */
	bit0 = (c >> 3) & 0x01;
	bit1 = (c >> 4) & 0x01;
	bit2 = (c >> 5) & 0x01;
	g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

	/* blue component */
	bit0 = (c >> 6) & 0x01;
	bit1 = (c >> 7) & 0x01;
	bit2 = (c >> 8) & 0x01;
	b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

	palette_set_color_rgb(Machine,color_index,r,g,b);
}


static int dev_sel;


WRITE8_HANDLER( changela_mem_device_select_w )
{
	dev_sel = data & 7;

	mem_dev_selected = 0;

	if(dev_sel > 3)
	{
		popmessage("memory device selected = %i", dev_sel);
	}

	mem_dev_selected = dev_sel * 0x800;

	/*
    dev_sel possible settings:
    0 - not connected (no device)
    1 - ADR1 is 2114 RAM at U59 (state machine) (accessible range: 0x0000-0x003f)
    2 - ADR2 is 2128 RAM at U109 (River RAM)    (accessible range: 0x0000-0x07ff)
    3 - ADR3 is 2128 RAM at U114? (Tree RAM)    (accessible range: 0x0000-0x07ff)
    4 - ADR4 is 2732 ROM at U7    (Tree ROM)    (accessible range: 0x0000-0x07ff)
    5 - SLOPE is ROM at U44 (state machine)     (accessible range: 0x0000-0x07ff)
    */
}

WRITE8_HANDLER( changela_mem_device_w )
{
	memory_devices[mem_dev_selected + offset] = data;

    if((mem_dev_selected==0x800) & (offset>0x3f))
		popmessage("RAM1 offset=%4x",offset);

}


READ8_HANDLER( changela_mem_device_r )
{
	return memory_devices[mem_dev_selected + offset];
}


WRITE8_HANDLER( changela_slope_rom_addr_hi_w )
{
	slopeROM_bank = (data&3)<<9;
//  popmessage("bank = %04x", slopeROM_bank);
}

WRITE8_HANDLER( changela_slope_rom_addr_lo_w )
{
	address = data;
//    popmessage("address = %04x", address);
}


