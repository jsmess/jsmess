/**************************************************************************
Change Lanes
(C) Taito 1983

Jarek Burczynski
Phil Stroffolino
Tomasz Slanina

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "sound/ay8910.h"

static unsigned char portA_in,portA_out,ddrA;
static unsigned char portB_out,ddrB;
static unsigned char portC_in,portC_out,ddrC;

static unsigned char mcu_out;
static unsigned char mcu_in;
static unsigned char mcu_PC1;
static unsigned char mcu_PC0;

static UINT32 slopeROM_bank;
static UINT32 address;

static UINT8 * memory_devices;
static UINT32  mem_dev_selected; /* an offset within memory_devices area */

MACHINE_RESET (changela)
{
	mcu_PC1=0;
	mcu_PC0=0;
}


VIDEO_START( changela )
{
	memory_devices = auto_malloc(4 * 0x800); /* 0 - not connected, 1,2,3 - RAMs*/
	return 0;
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
	const UINT8 * obj0rom;
	const UINT8 *pSource;
	const UINT8 *ROM1;
	const UINT8 *ROM111;
	UINT8 *RAM1;
	UINT8 *RAM2;
	UINT8 tile,attr;

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
	return 0;
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
				plot_pixel(bitmap, sx, sy, Machine->pens[data] );
			}
		}

		if (sy&1)
			ROM1+=8;	/* every second line */
	}


	/* OBJ 0 - sprites */
	obj0rom = memory_region(REGION_USER1);

	pSource = spriteram;
	for (i = 0; i < 0x400; i+=4)
	{
		int vr,hr,hs;
		UINT32 syy,hsize;
		UINT8  roz;
		UINT32 vsiz;
		UINT32 ypos;
		UINT32 xpos;

		roz = pSource[i+0];			/* 0x80->VR, 0x40->HR, 0x20->HS */
		vsiz= roz & 0x1f;			/* vertical size-1 (0 = one line) */
		ypos= 0x100-pSource[i+1];
		xpos= pSource[i+3];
		tile= pSource[i+2];
		vr = (roz&0x80)>>7;
		hr = (roz&0x40)>>6;
		hs = (roz&0x20)>>5;	/* horizontal size in 8 bytes units (2 pixels per byte) */

		for (syy = 0; syy <= vsiz; syy++)
		{
			unsigned int ls00 = ( (vr^((syy>>4)&1)) & ((vsiz>>4)&1) )^1;
			unsigned int A7 = (ls00 & (tile&1)) ^1;
			unsigned int A8 = ((tile&2)>>1) ^ (hr&hs) ^ hs /*^hs = ^/Qls109*/ ;
			UINT32 rom_addr = ((tile>>2)<<9) | (A8<<8) | (A7<<7) | ((syy&0x0f)<<3);

			if (vr)	/* vertical rotate */
				rom_addr ^= (0x0f<<3);

			for (hsize = 0; hsize <= hs; hsize++)
			{
				if (hsize==1)
					rom_addr ^= 0x100;

				if ( ((syy+ypos)>=cliprect->min_y) && ((syy+ypos)<=cliprect->max_y) )
				{

					for (sx = 0; sx < 8; sx++)	/* 8 steps == 16 pixels */
					{
						UINT32 xp,yp,pixdata;
						UINT8 data;

						if (!hr)
							data = obj0rom[rom_addr+sx];
						else
						{
							data = obj0rom[rom_addr+(sx^7)];
							data = ((data&0x0f)<<4) | ((data&0xf0)>>4); /* horizontal rotate */
						}

						xp = 2*sx + xpos + hsize*16;
						yp = syy + ypos - vsiz;

						pixdata = (data>>4) & 0x0f;
						if ((xp<256) && (yp<256))
						{
							if ( (pixdata!=0x0f) && (pixdata!=0) )
								plot_pixel(bitmap, xp, yp, Machine->pens[16+pixdata] );
						}

						pixdata = data & 0x0f;
						xp++;
						if ((xp<256) && (yp<256))
						{
							if ( (pixdata!=0x0f) && (pixdata!=0) )
								plot_pixel(bitmap, xp, yp, Machine->pens[16+pixdata] );
						}
					}
				}
			}
		}
	}

	/* OBJ 1 - text */
	pSource = videoram;
	attr = 0;
	for (sy = 0; sy < 256; sy+=8)
	{
		for (sx = 0; sx < 256; sx+=8)
		{
			tile = pSource[0];
			if (!pSource[1]&0x10) /* D4=0 enables latch at U32 */
				attr = pSource[1];
			pSource += 2;
#if 1
			drawgfx(bitmap, Machine->gfx[1],
				tile,
				8 + ((attr>>6)&0x03), /* color; OBJ1s use palette entries 0x20-0x2f (color code=8) */
				0,0, /* flip */
				sx+(attr&0x0f),sy,
				cliprect,
				TRANSPARENCY_PEN,3 ); /* fix it: should be 7 */
#endif
		}
	}
	return 0;
}


static WRITE8_HANDLER (colors_w)
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

	palette_set_color(Machine,color_index,r,g,b);
}

static READ8_HANDLER( mcu_r )
{
	//mame_printf_debug("Z80 MCU  R = %x\n",mcu_out);
	return mcu_out;
}

/* latch LS374 at U39 */
static WRITE8_HANDLER( mcu_w )
{
	mcu_in = data;
}



/*********************************
        MCU
*********************************/



READ8_HANDLER( changela_68705_portA_r )
{
	return (portA_out & ddrA) | (portA_in & ~ddrA);
}

WRITE8_HANDLER( changela_68705_portA_w )
{
	portA_out = data;
}

WRITE8_HANDLER( changela_68705_ddrA_w )
{
	ddrA = data;
}


READ8_HANDLER( changela_68705_portB_r )
{
	return (portB_out & ddrB) | (readinputport(4) & ~ddrB);
}

WRITE8_HANDLER( changela_68705_portB_w )
{
	portB_out = data;
}

WRITE8_HANDLER( changela_68705_ddrB_w )
{
	ddrB = data;
}


READ8_HANDLER( changela_68705_portC_r )
{
	return (portC_out & ddrC) | (portC_in & ~ddrC);
}

WRITE8_HANDLER( changela_68705_portC_w )
{
	/* PC3 is connected to the CLOCK input of the LS374,
        so we latch the data on positive going edge of the clock */

/* this is strange because if we do this corectly - it just doesn't work */
	if( (data&8) /*& (!(portC_out&8))*/ )
		mcu_out  = portA_out;


	/* PC2 is connected to the /OE input of the LS374 */

	if(!(data&4))
		portA_in = mcu_in;

	portC_out = data;
}

WRITE8_HANDLER( changela_68705_ddrC_w )
{
	ddrC = data;
}


static ADDRESS_MAP_START( mcu_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(11) )
	AM_RANGE(0x0000, 0x0000) AM_READ(changela_68705_portA_r)
	AM_RANGE(0x0001, 0x0001) AM_READ(changela_68705_portB_r)
	AM_RANGE(0x0002, 0x0002) AM_READ(changela_68705_portC_r)

	AM_RANGE(0x0000, 0x007f) AM_READ(MRA8_RAM)
	AM_RANGE(0x0080, 0x07ff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mcu_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(11) )
	AM_RANGE(0x0000, 0x0000) AM_WRITE(changela_68705_portA_w)
	AM_RANGE(0x0001, 0x0001) AM_WRITE(changela_68705_portB_w)
	AM_RANGE(0x0002, 0x0002) AM_WRITE(changela_68705_portC_w)
	AM_RANGE(0x0004, 0x0004) AM_WRITE(changela_68705_ddrA_w)
	AM_RANGE(0x0005, 0x0005) AM_WRITE(changela_68705_ddrB_w)
	AM_RANGE(0x0006, 0x0006) AM_WRITE(changela_68705_ddrC_w)

	AM_RANGE(0x0000, 0x007f) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x0080, 0x07ff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END



/* U30 */
static READ8_HANDLER( changela_24_r )
{
	return ((portC_out&2)<<2) | 7;	/* bits 2,1,0-N/C inputs */
}

static READ8_HANDLER( changela_25_r )
{
	return 0x03;//collisions on bits 3,2, bits 1,0-N/C inputs
}

static READ8_HANDLER( changela_30_r )
{
	return readinputport(7);	//wheel control (clocked input) signal on bits 3,2,1,0
}

static READ8_HANDLER( changela_31_r )
{
	return 0x00;	//wheel UP/DOWN control signal on bit 3, collisions on bits:2,1,0
}

static READ8_HANDLER( changela_2c_r )
{
	int val = readinputport(5);

    val = (val&0x30) | ((val&1)<<7) | (((val&1)^1)<<6);

	return val;
}

static READ8_HANDLER( changela_2d_r )
{
	/* the schems are unreadable - i'm not sure it is V8 (page 74, SOUND I/O BOARD SCHEMATIC 1 OF 2, FIGURE 24 - in the middle on the right side) */
	int v8 = 0;

	if ((cpu_getscanline() & 0xf8)==0xf8)
		v8 = 1;

	return (readinputport(6) & 0xe0) | (v8<<4);
}

static WRITE8_HANDLER( mcu_PC0_w )
{
	portC_in = (portC_in&0xfe) | (data&1);
	//mame_printf_debug("PC0 W = %x\n",data&1);
}

static WRITE8_HANDLER( changela_collision_reset_0 )
{

}

static WRITE8_HANDLER( changela_collision_reset_1 )
{

}

static WRITE8_HANDLER( changela_coin_counter_w )
{
	coin_counter_w(offset,data);
}


static int dev_sel;


static WRITE8_HANDLER( mem_device_select_w )
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

static WRITE8_HANDLER( mem_device_w )
{
	memory_devices[mem_dev_selected + offset] = data;

    if((mem_dev_selected==0x800) & (offset>0x3f))
		popmessage("RAM1 offset=%4x",offset);

}
static READ8_HANDLER( mem_device_r )
{
	return memory_devices[mem_dev_selected + offset];
}




static WRITE8_HANDLER( slope_rom_addr_hi_w )
{
	slopeROM_bank = (data&3)<<9;
//  popmessage("bank = %04x", slopeROM_bank);
}

static WRITE8_HANDLER( slope_rom_addr_lo_w )
{
	address = data;
//    popmessage("address = %04x", address);
}

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0x83ff) AM_READ(MRA8_RAM)				/* OBJ0 RAM */
	AM_RANGE(0x9000, 0x97ff) AM_READ(MRA8_RAM)				/* OBJ1 RAM */
	AM_RANGE(0xb000, 0xbfff) AM_READ(MRA8_ROM)

	AM_RANGE(0xc000, 0xc7ff) AM_READ(mem_device_r)			/* RAM4 (River Bed RAM); RAM5 (Tree RAM) */

	AM_RANGE(0xd000, 0xd000) AM_READ(AY8910_read_port_0_r)
	AM_RANGE(0xd010, 0xd010) AM_READ(AY8910_read_port_1_r)

	/* LS139 - U24 */
	AM_RANGE(0xd024, 0xd024) AM_READ(changela_24_r)
	AM_RANGE(0xd025, 0xd025) AM_READ(changela_25_r)
	AM_RANGE(0xd028, 0xd028) AM_READ(mcu_r)
	AM_RANGE(0xd02c, 0xd02c) AM_READ(changela_2c_r)
	AM_RANGE(0xd02d, 0xd02d) AM_READ(changela_2d_r)

	AM_RANGE(0xd030, 0xd030) AM_READ(changela_30_r)
	AM_RANGE(0xd031, 0xd031) AM_READ(changela_31_r)

	AM_RANGE(0xf000, 0xf7ff) AM_READ(MRA8_RAM)	/* RAM2 (Processor RAM) */
ADDRESS_MAP_END


static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM) 				/* Processor ROM */
	AM_RANGE(0x8000, 0x83ff) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram) 	/* OBJ0 RAM (A10 = GND )*/
	AM_RANGE(0x9000, 0x97ff) AM_WRITE(MWA8_RAM) AM_BASE(&videoram)		/* OBJ1 RAM */
	AM_RANGE(0xa000, 0xa07f) AM_WRITE(colors_w) AM_BASE(&colorram)		/* Color 93419 RAM 64x9(nine!!!) bits A0-used as the 8-th bit data input (d0-d7->normal, a0->d8) */
	AM_RANGE(0xb000, 0xbfff) AM_WRITE(MWA8_ROM)				/* Processor ROM */

	AM_RANGE(0xc000, 0xc7ff) AM_WRITE(mem_device_w)			/* River-Tree RAMs, slope ROM, tree ROM */

	/* LS138 - U16 */
	AM_RANGE(0xc800, 0xc800) AM_WRITE(MWA8_NOP)				/* not connected */
	AM_RANGE(0xc900, 0xc900) AM_WRITE(mem_device_select_w)	/* selects the memory device to be accessible at 0xc000-0xc7ff */
	AM_RANGE(0xca00, 0xca00) AM_WRITE(slope_rom_addr_hi_w)
	AM_RANGE(0xcb00, 0xcb00) AM_WRITE(slope_rom_addr_lo_w)

	AM_RANGE(0xd000, 0xd000) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0xd001, 0xd001) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0xd010, 0xd010) AM_WRITE(AY8910_control_port_1_w)
	AM_RANGE(0xd011, 0xd011) AM_WRITE(AY8910_write_port_1_w)

	/* LS259 - U44 */
	AM_RANGE(0xd020, 0xd020) AM_WRITE(changela_collision_reset_0)
	AM_RANGE(0xd021, 0xd022) AM_WRITE(changela_coin_counter_w)
//AM_RANGE(0xd023, 0xd023) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xd024, 0xd024) AM_WRITE(mcu_PC0_w)
	AM_RANGE(0xd025, 0xd025) AM_WRITE(changela_collision_reset_1)
	AM_RANGE(0xd026, 0xd026) AM_WRITE(MWA8_NOP)

	AM_RANGE(0xd030, 0xd030) AM_WRITE(mcu_w)
	AM_RANGE(0xe000, 0xe000) AM_WRITE(watchdog_reset_w)		/* Watchdog */
	AM_RANGE(0xf000, 0xf7ff) AM_WRITE(MWA8_RAM)				/* Processor RAM */
ADDRESS_MAP_END


INPUT_PORTS_START( changela )
	PORT_START /* 0 */ /* DSWA */
	PORT_DIPNAME( 0x07, 0x06, "Steering Wheel Ratio" )
	PORT_DIPSETTING(    0x06, "Recommended" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Loop on Memory Failures" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Control Type" )
	PORT_DIPSETTING(    0x00, DEF_STR( Joystick ) )
	PORT_DIPSETTING(    0x20, "Steering Wheel" )
	PORT_DIPNAME( 0x40, 0x40, "Diagnostic" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Number of Players" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x80, "2" )

	PORT_START /* 1 */ /* DSWB */
	PORT_DIPNAME( 0x01, 0x00, "Max Bonus Fuels" )
	PORT_DIPSETTING(    0x02, "1" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x03, "99" )
	PORT_DIPNAME( 0x0c, 0x00, "Game Difficulty" )
	PORT_DIPSETTING(    0xc0, DEF_STR( Very_Easy) )
	PORT_DIPSETTING(    0x80, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x30, 0x00, "Traffic Difficulty" )
	PORT_DIPSETTING(    0x30, DEF_STR( Very_Easy) )
	PORT_DIPSETTING(    0x20, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x40, 0x00, "Land Collisions Enabled" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Car Collisions Enabled" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START /* 2 */ /* DSWC - coinage (see manual) */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START /* 3 */ /* DSWD - bonus */
	PORT_DIPNAME( 0x01, 0x01, "Right Coin" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Left Coin" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x1c, 0x00, "Credits for Bonus" )
	PORT_DIPSETTING(    0x1c, "0" )
	PORT_DIPSETTING(    0x14, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "6" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPUNUSED( 0x20, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0xc0, 0xc0, "King of the World" )
	PORT_DIPSETTING(    0x00, "No Name" )
	PORT_DIPSETTING(    0x80, "Short Name" )
	PORT_DIPSETTING(    0xc0, "Long Name" )

	PORT_START /* port 4 */ /* MCU */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR(Free_Play) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN3 )	/* NC */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START /* 5 */ /* 0xDx2C */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("Gear Shift") PORT_CODE(KEYCODE_SPACE) PORT_TOGGLE /* Gear shift */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* FWD - negated bit 7 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* REV - gear position */

	PORT_START /* 6 */ /* 0xDx2D */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) //gas1
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 ) //gas2

	PORT_START /* 7 */ /* 0xDx30 DRIVING_WHEEL */
	PORT_BIT( 0x0f, 0x00, IPT_PADDLE ) PORT_MINMAX(0,0x0f) PORT_SENSITIVITY(50) PORT_KEYDELTA(1)
//PORT_BIT(  mask,default,type ) PORT_MINMAX(min,max) PORT_SENSITIVITY(sensitivity) PORT_KEYDELTA(delta)


INPUT_PORTS_END


static const gfx_layout tile_layout =
{
	8,16,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{	0*32,1*32,2*32,3*32,4*32,5*32,6*32,7*32,
		8*32,9*32,10*32,11*32,12*32,13*32,14*32,15*32 },
	16*32
};

static const gfx_layout obj1_layout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 0,4 },
	{ 0,1,2,3, 8,9,10,11 },
	{ 0*16,1*16,2*16,3*16,4*16,5*16,6*16,7*16 },
	8*8*2
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &tile_layout, 0, 4 },
	{ REGION_GFX2, 0x0000, &obj1_layout, 0, 16 },
	{ -1 }
};

static struct AY8910interface ay8910_interface_1 = {
	input_port_0_r,
	input_port_2_r
};

static struct AY8910interface ay8910_interface_2 = {
	input_port_1_r,
	input_port_3_r
};


INTERRUPT_GEN( chl_interrupt )
{
	int vector = cpu_getvblank() ? 0xdf : 0xcf; /* 4 irqs per frame: 3 times 0xcf, 1 time 0xdf */

//    video_screen_update_partial(0, cpu_getscanline());

	cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, vector);

	/* it seems the V8 == Vblank and it is connected to the INT on the 68705 */
	//so we should cause an INT on the cpu 1 here, as well.
	//but only once per frame !
	if (vector == 0xdf) /* only on vblank */
		cpunum_set_input_line(1, 0, PULSE_LINE );

}

static MACHINE_DRIVER_START( changela )

	MDRV_CPU_ADD(Z80,5000000)
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(chl_interrupt,4)

	MDRV_CPU_ADD(M68705,2000000)
	MDRV_CPU_PROGRAM_MAP(mcu_readmem,mcu_writemem)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(changela)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x40)

	MDRV_VIDEO_START(changela)
	MDRV_VIDEO_UPDATE(changela)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 1250000)
	MDRV_SOUND_CONFIG(ay8910_interface_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_ADD(AY8910, 1250000)
	MDRV_SOUND_CONFIG(ay8910_interface_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END


ROM_START( changela )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* Z80 code */
	ROM_LOAD( "cl25a",	0x0000, 0x2000, CRC(38530a60) SHA1(0b0ef1abe11c5271fcd1671322b77165217553c3) )
	ROM_LOAD( "cl24a",	0x2000, 0x2000, CRC(2fcf4a82) SHA1(c33355e2d4d3fab32c8d713a680ec0fceedab341) )
	ROM_LOAD( "cl23",	0x4000, 0x2000, CRC(08385891) SHA1(d8d66664ec25db067d5a4a6c35ec0ac65b9e0c6a) )
	ROM_LOAD( "cl22",	0x6000, 0x2000, CRC(796e0abd) SHA1(64dd9fc1f9bc44519a253ef0c02e181dd13904bf) )
	ROM_LOAD( "cl27",	0xb000, 0x1000, CRC(3668afb8) SHA1(bcfb788baf806edcb129ea9f9dcb1d4260684773) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 68705U3 */
	ROM_LOAD( "cl38a",	0x0000, 0x800, CRC(b70156ce) SHA1(c5eab8bbd65c4f587426298da4e22f991ce01dde) )

	ROM_REGION( 0x4000, REGION_GFX1, 0 )	/* tile data */
	ROM_LOAD( "cl111",	0x0000, 0x2000, CRC(41c0149d) SHA1(3ea53a3821b044b3d0451fec1b4ee2c28da393ca) )
	ROM_LOAD( "cl113",	0x2000, 0x2000, CRC(ddf99926) SHA1(e816b88302c5639c7284f4845d450f232d63a10c) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )	/* obj 1 data */
	ROM_LOAD( "cl46",	0x0000, 0x1000, CRC(9c0a7d28) SHA1(fac9180ea0d9aeea56a84b35cc0958f0dd86a801) )

	ROM_REGION( 0x8000, REGION_USER1, 0 )	/* obj 0 data */
	ROM_LOAD( "cl100",	0x0000, 0x2000, CRC(3fa9e4fa) SHA1(9abd7df5fcf143a0c476bd8c8753c5ea294b9f74) )
	ROM_LOAD( "cl99",	0x2000, 0x2000, CRC(67b27b9e) SHA1(7df0f93851959359218c8d2272e30d242a77039d) )
	ROM_LOAD( "cl98",	0x4000, 0x2000, CRC(bffe4149) SHA1(5cf0b98f9d342bd06d575c565ea01bbd79f5e04b) )
	ROM_LOAD( "cl97",	0x6000, 0x2000, CRC(5abab8f9) SHA1(f5156855bbcdf0740fd44520386318ee53ebbf9a) )

	ROM_REGION( 0x1000, REGION_USER2, 0 )	/* math tables: SLOPE ROM (river-tree schematic page 1/3) */
	ROM_LOAD( "cl44",	0x0000, 0x1000, CRC(160d2bc7) SHA1(2609208c2bd4618ea340923ee01af69278980c36) ) /* first and 2nd half identical */

	ROM_REGION( 0x3000, REGION_USER3, 0 )	/* math tables: TREE ROM (river-tree schematic page 3/3)*/
	ROM_LOAD( "cl7",	0x0000, 0x0800, CRC(01e3efca) SHA1(b26203787f105ba32773e37035c39253050f9c82) ) /* fixed bits: 0xxxxxxx */
	ROM_LOAD( "cl9",	0x1000, 0x2000, CRC(4e53cdd0) SHA1(6255411cfdccbe2c581c83f9127d582623453c3a) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "cl88",	0x0000, 0x0020, CRC(da4d6625) SHA1(2d9a268973518252eb36f479ab650af8c16c885c) ) /* math train state machine */
ROM_END

static DRIVER_INIT(changela)
{
}

GAME( 1983, changela, 0, changela, changela, changela, ROT180, "Taito Corporation", "Change Lanes", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS )
