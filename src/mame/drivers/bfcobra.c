/*
    Bell-Fruit Cobra Hardware

    Working:
    * A Question of Sport

    Other games:
    * Brain Box
    * Every Second Counts
    * Inquizitor
    * Quizvaders
    * Treble Top

    TODO:
    * Correctly implement blitter.
    * DSP. A Question of Sport doesn't appear to touch it (later PCB revisions don't have it?)
    * Unknown Slipstream I/O accesses.
    * Hook up inputs, EM meters, lamps etc
    * Floppy disk controller.
    * Remove instances of PULSE_LINE, dodgy bank selection and deprecated MAME functions.
*/

#include "driver.h"
#include "debugger.h"
#include "machine/6850acia.h"
#include "cpu/z80/z80.h"
#include "cpu/m6809/m6809.h"
#include "sound/upd7759.h"
#include "sound/ay8910.h"

/* TODO */
#define Z80_XTAL	4000000
#define M6809_XTAL	1000000

#ifdef MAME_DEBUG
int DEBUG_BLITTER = 0;
#endif

/* Typedefs */
typedef union
{
#ifdef LSB_FIRST
	struct { UINT8 addr0, addr1, addr2; };
#else
	struct { UINT8 addr2, addr1, addr0; };
#endif
	UINT32 addr;
} ADDR_REG;

/* Blitter register flag bits */
#define	CMD_RUN		0x01
#define CMD_COLST	0x02
#define CMD_PARRD	0x04		/* Never used? */
#define CMD_SRCUP	0x08
#define CMD_DSTUP	0x10
#define CMD_SRCEN	0x20
#define CMD_DSTEN	0x40
#define CMD_LINEDRAW	0x80

/* All unconfirmed */
//#define SRCDST_CMP    0x10
//#define SRCDST_WRAP   0x20
//#define SRCDST_SIGN   0x40
#define SRCDST_A_1	0x80		/* This might be correct for line drawing? */

/* These appear to be correct */
#define MODE_SSIGN	0x80		// TODO: Implement these outside line mode.
#define MODE_DSIGN	0x40
#define MODE_YFRAC	0x20
#define MODE_BITTOBYTE	0x04
#define MODE_UNKNOWN	0x10		// TODO: Used to adjust photo palette?

/* All unconfirmed */
//#define CMPFUNC_EQ    0x01
//#define CMPFUNC_NE    0x02
//#define CMPFUNC_GT    0x04
//#define CMPFUNC_PLANE 0x08

#define CMPFUNC_EQ	0x08
#define CMPFUNC_LOG0	0x10
#define CMPFUNC_LOG1	0x20
#define CMPFUNC_LOG2	0x40
#define CMPFUNC_LOG3	0x80


/*
    State for the blitter unit
*/
static struct
{
	ADDR_REG	program;

	UINT8		control;
	UINT8		status;

	UINT8		command;
	ADDR_REG	source;
	ADDR_REG	dest;
	UINT8		modectl;
	UINT8		compfunc;
	UINT8		outercnt;

	UINT8		innercnt;
	UINT8		step;
	UINT8		pattern;
} blitter;

/*
    MUSIC Semiconductor TR9C1710 RAMDAC or equivalent
*/
static struct
{
	UINT8	addr_w;
	UINT8	addr_r;
	UINT8	mask;

	/* 18-bit colors */
	UINT8	color_r[3];
	UINT8	color_w[3];
	UINT32	table[256];

	/* Access counts */
	UINT8	count_r;
	UINT8	count_w;
} ramdac;

/*
    Globals
*/

static UINT8 bank[4];
static UINT8 *work_ram;
static UINT8 *video_ram;
static UINT8 h_scroll;
static UINT8 flip_8;
static UINT8 flip_22;

/* UART source/sinks */
static UINT8 z80_m6809_line;
static UINT8 m6809_z80_line;
static UINT8 data_r;
static UINT8 data_t;

static UINT8 z80_int;
static UINT8 z80_inten;

/* EM and lamps stuff */
static UINT32 meter_latch;
static UINT32 mux_input;
static UINT32 mux_outputlatch;

INLINE void z80_bank(int num, int data);

/* TODO */
static MACHINE_RESET( bfcobra )
{
	bank[0] = 1;
	memset(&ramdac, 0, sizeof(ramdac));
}

/* TODO: Put in video/bfcobra.c */
static VIDEO_UPDATE( bfcobra )
{
	int x, y;
	UINT8 *src;
	UINT32 *dest;
	UINT32 offset;

	if (flip_8 == 0x44 && flip_22 == 0x41)
	{
		offset = 0x10000;
	}
	else
	{
		offset = 0;
	}

	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		src = &video_ram[offset + y * 256];
		dest = BITMAP_ADDR32(bitmap,y,0);

		for (x = cliprect->min_x; x <= cliprect->max_x; x++)
		{
			*dest++ = machine->pens[*src++];
		}
	}

	return 0;
}

/* TODO */
INLINE UINT8* blitter_get_addr(UINT32 addr)
{
	if (addr < 0x10000)
	{
		return (UINT8*)(memory_region(REGION_USER1) + addr);	// Is this region fixed?

	}
	else if(addr < 0x20000)
	{
		addr &= 0xffff;
		addr += (bank[0] & 1) ? 0x10000 : 0;

		return (UINT8*)(memory_region(REGION_USER1) + addr + ((bank[0] >> 1) * 0x20000));
	}
	else if (addr >= 0x20000 && addr < 0x40000)
	{
		return (UINT8*)&video_ram[addr - 0x20000];
	}
	else
	{
		return (UINT8*)&work_ram[addr - 0x40000];
	}
}


/*
    TODO: Needs re-writing.
*/
static void RunBlit(void)
{
#define BLITPRG_READ(x)		blitter.x = *(blitter_get_addr(blitter.program.addr++))

	do
	{
		UINT8 srcdata = 0;
		UINT8 dstdata = 0;

		/* Read the blitter command */
		BLITPRG_READ(source.addr0);
		BLITPRG_READ(source.addr1);
		BLITPRG_READ(source.addr2);
		BLITPRG_READ(dest.addr0);
		BLITPRG_READ(dest.addr1);
		BLITPRG_READ(dest.addr2);
		BLITPRG_READ(modectl);
		BLITPRG_READ(compfunc);
		BLITPRG_READ(outercnt);
		BLITPRG_READ(innercnt);
		BLITPRG_READ(step);
		BLITPRG_READ(pattern);

	#ifdef MAME_DEBUG
		if (DEBUG_BLITTER)
		{
			// Check
			printf("\nBlitter: Running command from 0x%.5x\n\n",blitter.program.addr - 12);

			printf("Command Reg         %.2x",	blitter.command);
			printf("		%s %s %s %s %s %s %s\n",
				blitter.command & CMD_RUN ? "RUN" : "     ",
				blitter.command & CMD_COLST ? "COLST" : "     ",
				blitter.command & CMD_PARRD ? "PARRD" : "     ",
				blitter.command & CMD_SRCUP ? "SRCUP" : "     ",
				blitter.command & CMD_DSTUP ? "DSTUP" : "     ",
				blitter.command & CMD_SRCEN ? "SRCEN" : "     ",
				blitter.command & CMD_DSTEN ? "DSTEN" : "     ");

			printf("Src Address Byte 0  %.2x\n",	blitter.source.addr0);
			printf("Src Address Byte 1  %.2x\n",	blitter.source.addr1);
			printf("Src Control         %.2x\n",	blitter.source.addr2);
			printf("  Src Address       %.5x\n",	blitter.source.addr & 0xfffff);
			printf("Dest Address Byte 0 %.2x\n",	blitter.dest.addr0);
			printf("Dest Address Byte 1 %.2x\n",	blitter.dest.addr1);
			printf("Dest Control        %.2x\n",	blitter.dest.addr2);
			printf("  Dst. Address      %.5x\n",	blitter.dest.addr & 0xfffff);
			printf("Mode Control        %.2x",	blitter.modectl);
			printf("		%s\n", blitter.modectl & MODE_BITTOBYTE ? "BIT_TO_BYTE" : "");

			printf("Comp. and LFU       %.2x\n",	blitter.compfunc);
			printf("Outer Loop Count    %.2x (%d)\n", blitter.outercnt, blitter.outercnt);
			printf("Inner Loop Count    %.2x (%d)\n", blitter.innercnt, blitter.innercnt);
			printf("Step Value          %.2x\n",	blitter.step);
			printf("Pattern Byte        %.2x\n",	blitter.pattern);
		}
	#endif

	/* TODO */
	if (blitter.dest.addr == 0)
	{
		return;
	}

		/* Begin outer loop */
		do
		{
			UINT8 innercnt = blitter.innercnt;

			/*
                Line Drawing Mode

                x1 - x0
                y1 - y0

                d1 = bigger
                d2 = smaller

                * SRC0 = d1/2
                * SRC1 = d1
                * Inner Count = d1
                * DST0 = X0
                * DST1 = Y0
                * Source control = LSB of d1
                * Step reg = d2
            */
			if (blitter.command & CMD_LINEDRAW)
			{
				// TODO
			}
			else do
			{
				//UINT32    srcaddr = 0;
				UINT8	inhibit = 0;

				// TODO: Set this correctly.
				UINT8	result = blitter.pattern;

				/* Source value is read only once in character mode */
				if (blitter.modectl & MODE_BITTOBYTE && innercnt == blitter.innercnt)
				{
					if (blitter.command & CMD_SRCEN)
					{
						srcdata = *(blitter_get_addr(blitter.source.addr & 0xfffff));
					}
				}

				/* Enable source address read and increment? */
				if (!(blitter.modectl & MODE_BITTOBYTE))
				{
					if (blitter.command & CMD_SRCEN)
					{
						srcdata = *(blitter_get_addr(blitter.source.addr & 0xfffff));
						blitter.source.addr = (blitter.source.addr + 1) & 0xfffff;
						result = srcdata;
					}
				}

				/* Read destination pixel? */
				if (blitter.command & CMD_DSTEN)
				{
					dstdata = *blitter_get_addr(blitter.dest.addr & 0xfffff);
				}

				/* Inhibit depending on the bit selected by the inner count */

				// Switch on comparator type?
				if (blitter.modectl & MODE_BITTOBYTE)
				{
					inhibit = !(srcdata & (1 << (8 - innercnt)));

					if (innercnt == 1)
					{
						blitter.source.addr = (blitter.source.addr + 1) & 0xfffff;
					}
				}

				if (blitter.compfunc & 0x08)
				{
					if (srcdata == blitter.pattern)
					{
						inhibit = 1;

						/* TODO: Resume from inhibit */
						if (blitter.command & CMD_COLST)
						{
							return;
						}
					}
				}
#if 0
				else
				{
					UINT32 LOG0, LOG1, LOG2, LOG3;

					LOG0 = (blitter.compfunc & CMPFUNC_LOG0) ? ~srcdata & ~dstdata : 0;
					LOG1 = (blitter.compfunc & CMPFUNC_LOG1) ? ~srcdata & dstdata : 0;
					LOG2 = (blitter.compfunc & CMPFUNC_LOG2) ? srcdata & ~dstdata : 0;
					LOG3 = (blitter.compfunc & CMPFUNC_LOG3) ? srcdata & dstdata : 0;

					result = LOG0 | LOG1 | LOG2 | LOG3;
					inhibit = 0;
				}
#endif

				/* Write the data if not inhibited */
				if (!inhibit)
				{
					/* TODO: This is definitely not right */
					if (blitter.modectl & 0x10)
					{
						*blitter_get_addr(blitter.dest.addr) += 0x80;
					}
					else
					{
						*blitter_get_addr(blitter.dest.addr) = result;
					}
				}

				/* Update destination address */
				blitter.dest.addr = (blitter.dest.addr + 1) & 0xfffff;

			}while (--innercnt);

			if (!--blitter.outercnt)
			{
				break;
			}
			else
			{
				if (blitter.command & CMD_DSTUP)
				{
					blitter.dest.addr = (blitter.dest.addr + blitter.step) & 0xfffff;
				}

				if (blitter.command & CMD_SRCUP)
				{
					blitter.source.addr = (blitter.source.addr + blitter.step) & 0xfffff;
				}

				if (blitter.command & CMD_PARRD)
				{
					BLITPRG_READ(innercnt);
					BLITPRG_READ(step);
					BLITPRG_READ(pattern);
				}
			}

		}while (1);

		/* Read next command header */
		BLITPRG_READ(command);

	}while (blitter.command  & CMD_RUN);
}

static READ8_HANDLER( ramdac_r )
{
	UINT8 val = 0xff;

	switch (offset & 3)
	{
		case 1:
		{
			UINT8 *count = &ramdac.count_r;

			if (*count == 0)
			{
				rgb_t color;
				color = palette_get_color(Machine, ramdac.addr_r);

				ramdac.color_r[0] = RGB_RED(color);
				ramdac.color_r[1] = RGB_GREEN(color);
				ramdac.color_r[2] = RGB_BLUE(color);
			}

			val = ramdac.color_r[*count];

			/* 8bpp -> 6bpp */
			val = ((val & 0xc0) >> 2) | ((val >>2) & 0xf);

			if (++*count == 3)
			{
				*count = 0;
				/* Address register auto-increments */
				ramdac.addr_r++;
			}
			break;
		}
		default:
		{
			printf("Odd DAC access\n");
			DEBUGGER_BREAK;
			break;
		}
	}

	return val;
}

static WRITE8_HANDLER( ramdac_w )
{
	switch (offset & 3)
	{
		case 0:
		{
			ramdac.addr_w = data;
			break;
		}
		case 1:
		{
			data &= 0x3f;
			ramdac.color_w[ramdac.count_w] = pal6bit(data);
			if (++ramdac.count_w == 3)
			{
				palette_set_color_rgb(Machine, ramdac.addr_w, ramdac.color_w[0], ramdac.color_w[1], ramdac.color_w[2]);
				ramdac.count_w = 0;
				ramdac.addr_w++;
			}
			break;
		}
		case 2:
		{
			ramdac.mask = data;
			if (data != 0xff)
			{
				printf("Unimplemented: RAMDAC Mask %x\n",data);
				DEBUGGER_BREAK;
			}
			break;
		}
		case 3:
		{
			ramdac.addr_r = data;
			break;
		}
	}
}

/*
    WD37C656C-PL (or equivalent) Floppy Disk Controller
*/
static WRITE8_HANDLER( fdctrl_w )
{

}

static READ8_HANDLER( fdctrl_r )
{
	return 0x80;
}


/*
    01  Bank control for Z80 region 0x4000-0x7fff (16kB)    RW
    02  Bank control for Z80 region 0x8000-0xbfff (16kB)    RW
    03  Bank control for Z80 region 0xc000-0xffff (16kB)    RW

    06 Interrupt status?        WR
    07 Interrupt ack?           WR
       ???? Written with 0x21
    08
    09 ???? Linked with c001        W
    0A ???? Written with 0 and 1            W

    0B H scroll ??              W
    0C V scroll ??              W

    0D  ???? Written with $FF       W
    0E  ???? Written with $A2 (162)     W
    0F  ???? Written with $B4 (180)     W

    14                  W

    18  Blitter program low byte        WR
    19  Blitter program middle byte     W
    1A  Blitter program high byte       W
    1B  Blitter command?            W
    1C  Blitter status?         R
    20  Blitter control register        W

    22  ???? Linked with C002       WR
        Mask of 20
    40: ROM bank select         W
*/

/*
    Z80 IO addressing handled by Slipstream
*/
static READ8_HANDLER( slipstream_r )
{
	UINT8 val = 0xff;

	switch(offset)
	{
		case 1:
		case 2:
		case 3:
		{
			val = bank[offset];
			break;
		}
		case 6:
		{
			val = z80_int << 4;
			break;
		}
		case 7:
		{
			z80_int = 0;
			val = 0x1;
			break;
		}
		case 0x1C:
		{
			/* Blitter status ? */
			val = 0;
			break;
		}
		case 0x20:
		{
			/* TODO: Seems correct */
			val = blitter.dest.addr0;
			break;
		}
		default:
		{
		#ifdef MAME_DEBUG
//          printf("SS IO R: %x (%x)\n",offset);
		#endif
		}
	}

	return val;
}

static WRITE8_HANDLER( slipstream_w )
{
	switch(offset)
	{
		case 0x01:
		case 0x02:
		case 0x03:
		{
			data &= 0x3f;
			bank[offset] = data;
			z80_bank(offset, data);
			break;
		}
		case 0x07:
		{
			z80_inten = data;
			break;
		}
		case 0x08:
		{
			flip_8 = data;
			break;
		}
		case 0x22:
		{
			flip_22 = data;
			break;
		}
		case 0x0B:
		{
			h_scroll = data;
			break;
		}
		case 0x18:
		{
			blitter.program.addr0 = data;
			break;
		}
		case 0x19:
		{
			blitter.program.addr1 = data;
			break;
		}
		case 0x1A:
		{
			blitter.program.addr2 = data;
			break;
		}
		case 0x1B:
		{
			/* TODO: Blitter resume. */
			break;
		}
		case 0x20:
		{
			blitter.command = data;
			if (data & CMD_RUN)
			{
				RunBlit();
			}
			else
			{
			#ifdef MAME_DEBUG
				printf ("Blitter stopped by IO.\n");
			#endif
			}
			break;
		}
		default:
		{
		#ifdef MAME_DEBUG
			printf("SS IO W: %x (%x)\n",offset, data);
		#endif
		}
	}
}

INLINE void z80_bank(int num, int data)
{
	if (data < 8)
	{
		/* TODO: Don't need this table! */
		static UINT32 offs_table[2][8] =
		{ { 0x10000, 0x14000, 0x18000, 0x1c000, 0x00000, 0x04000, 0x08000, 0x0c000 },
		{ 0x00000, 0x04000, 0x08000, 0x0c000, 0x10000, 0x14000, 0x18000, 0x1c000 } };

		UINT32 offset = ((bank[0] >> 1) * 0x20000) + offs_table[bank[0] & 0x1][data];

		memory_set_bankptr(num, memory_region(REGION_USER1) + offset);
	}
	else if (data < 0x10)
	{
		memory_set_bankptr(num, &video_ram[(data * 0x4000) - 0x40000]);
	}
	else
	{
		memory_set_bankptr(num, &work_ram[(data * 0x4000) - 0x40000]);
	}
}

static WRITE8_HANDLER( rombank_w )
{
	bank[0] = data;
	z80_bank(1, bank[1]);
	z80_bank(2, bank[2]);
	z80_bank(3, bank[3]);
}


static ADDRESS_MAP_START( z80_prog_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_BANK4)
	AM_RANGE(0x4000, 0x7fff) AM_READWRITE(MRA8_BANK1, MWA8_BANK1)
	AM_RANGE(0x8000, 0xbfff) AM_READWRITE(MRA8_BANK2, MWA8_BANK2)
	AM_RANGE(0xc000, 0xffff) AM_READWRITE(MRA8_BANK3, MWA8_BANK3)
ADDRESS_MAP_END

/*
    All addressing is handled by Slipstream ASICs
*/
static ADDRESS_MAP_START( z80_io_map, ADDRESS_SPACE_IO, 8 )
ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x23) AM_READWRITE(slipstream_r, slipstream_w)
	AM_RANGE(0x24, 0x24) AM_WRITE(acia6850_0_ctrl_w)
	AM_RANGE(0x25, 0x25) AM_WRITE(acia6850_0_data_w)
	AM_RANGE(0x26, 0x26) AM_READ(acia6850_0_stat_r)
	AM_RANGE(0x27, 0x27) AM_READ(acia6850_0_data_r)
	AM_RANGE(0x40, 0x40) AM_WRITE(rombank_w)
	AM_RANGE(0x50, 0x53) AM_READWRITE(ramdac_r, ramdac_w)
	AM_RANGE(0x30, 0x33) AM_READWRITE(fdctrl_r, fdctrl_w)
ADDRESS_MAP_END

/*
    MC68B09P: Processes sound and I/O

    EF68B50P            - For communication with Z80
    EF68B50P            - For data retrieval
    AY38912A
    UPD7759C
    BFM1090 (CF30056)   - 'TEXAS' I/O chip, lamps and misc
    BFM1095 (CF30057)   - 'TEXAS' I/O chip, handles switches
    BFM1071             - 6809 Address decoding and bus control

    /IRQ provided by EF68850P at IC38
    /FIRQ connected to meter current sensing circuitry
    TODO: Watchdog timer provided by NE556. Calculate correct timeout period.
*/

/* TODO */
static READ8_HANDLER( int_latch_r )
{
	return 2 | 1;
}

/* TODO */
static READ8_HANDLER( meter_r )
{
	return meter_latch;
}

/* TODO */
static WRITE8_HANDLER( meter_w )
{
	//int changed = meter_latch ^ data;

	meter_latch = data;

	/*
        When a meter is triggered, the current drawn is sensed. If a meter
        is connected, the /FIRQ line will be pulsed.
    */
	if ( data )
	{
		cpunum_set_input_line(1, M6809_FIRQ_LINE, PULSE_LINE );
	}
}

/* TODO */
static READ8_HANDLER( latch_r )
{
	return mux_input;
}

static WRITE8_HANDLER( latch_w )
{
	/* TODO: This is borrowed from Scorpion 1 */
	switch(offset)
	{
		case 0:
		{
			int changed = mux_outputlatch ^ data;

			mux_outputlatch = data;

			// Clock has changed
			if (changed & 0x08)
			{
				int input_strobe = data & 0x7;
					// Clock is low
				if (!(data & 0x08))
				{
					mux_input = readinputport(input_strobe);
				}
			}
			break;
		}
		case 1:
		{
//          strobe_data_l = data;
			break;
		}
		case 2:
		{
//          strobe_data_h = data;
			break;
		}
	}
}

static READ8_HANDLER( upd7759_r )
{
	return upd7759_busy_r(0);
}

static WRITE8_HANDLER( upd7759_w )
{
	upd7759_start_w(0, data & 0x40);
	upd7759_reset_w(0, data & 0x80);
	upd7759_port_w(0, data & 0x3f);
}

static ADDRESS_MAP_START( m6809_prog_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_RAM	AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x2000, 0x2000) AM_RAM		// W 'B', 6F
	AM_RANGE(0x2200, 0x2200) AM_RAM		// W 'F'
	AM_RANGE(0x2600, 0x2600) AM_READWRITE(meter_r, meter_w)
	AM_RANGE(0x2800, 0x2800) AM_RAM		// W
	AM_RANGE(0x2A00, 0x2A02) AM_READWRITE(latch_r, latch_w)
	AM_RANGE(0x2E00, 0x2E00) AM_READ(int_latch_r)
	AM_RANGE(0x3001, 0x3001) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x3201, 0x3201) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x3404, 0x3404) AM_READWRITE(acia6850_1_stat_r, acia6850_1_ctrl_w)
	AM_RANGE(0x3405, 0x3405) AM_READWRITE(acia6850_1_data_r, acia6850_1_data_w)
	AM_RANGE(0x3406, 0x3406) AM_READWRITE(acia6850_2_stat_r, acia6850_2_ctrl_w)
	AM_RANGE(0x3407, 0x3407) AM_READWRITE(acia6850_2_data_r, acia6850_2_data_w)
	AM_RANGE(0x3801, 0x3801) AM_READWRITE(upd7759_r, upd7759_w)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_ROM)
	AM_RANGE(0xf000, 0xf000) AM_WRITE(MWA8_NOP)	/* Watchdog */
ADDRESS_MAP_END

static INPUT_PORTS_START( qos )
PORT_START_TAG("STROBE0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_NAME("Coin: 10p")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_NAME("Coin: 20p")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 ) PORT_NAME("Coin: 50p")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN4 ) PORT_NAME("Coin: 1 pound") // ?1 is invalid!  src/mame/drivers/bfcobra.c: qos input 'Coin: ?1' has invalid characters
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_NAME("Green Test?") PORT_CODE(KEYCODE_F1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("STROBE1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("PASS") PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("CONTINUE") PORT_CODE(KEYCODE_S)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("COLLECT") PORT_CODE(KEYCODE_D)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 ) PORT_NAME("START")
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("BONUS") PORT_CODE(KEYCODE_F)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON3 )

	PORT_START_TAG("STROBE2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_NAME("<A")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_NAME("<B")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON7 ) PORT_NAME("<C")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON8 ) PORT_NAME("A>")
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON9 ) PORT_NAME("B>")
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON10 ) PORT_NAME("C>")
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Cash box door") PORT_CODE(KEYCODE_Y) PORT_TOGGLE
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_NAME("Unknown (resets)") PORT_CODE(KEYCODE_T) PORT_TOGGLE
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_NAME("Refill Key") PORT_CODE(KEYCODE_R) PORT_TOGGLE
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE4")
	PORT_BIT( 0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE5")
	PORT_BIT( 0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE6")
	PORT_DIPNAME( 0x01, 0x00, "DIL09" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL10" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "DIL11" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "DIL12" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "DIL13" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )
	PORT_DIPNAME( 0x20, 0x00, "DIL14" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On  ) )
	PORT_DIPNAME( 0x40, 0x00, "DIL15" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On  ) )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE7")
	PORT_DIPNAME( 0x01, 0x00, "DIL02" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL03" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "DIL04" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "DIL05" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "DIL06" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )
	PORT_DIPNAME( 0x20, 0x00, "DIL07" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On  ) )
	PORT_DIPNAME( 0x40, 0x00, "DIL08" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On  ) )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

#if 0
INPUT_PORTS_START( brainbox )
INPUT_PORTS_END

INPUT_PORTS_START( trebltop )
INPUT_PORTS_END

INPUT_PORTS_START( inquiztr )
INPUT_PORTS_END

INPUT_PORTS_START( everysec )
INPUT_PORTS_END
#endif

static void z80_acia_irq(int state)
{
	cpunum_set_input_line(0, 0, state ? HOLD_LINE : CLEAR_LINE);
}

static void m6809_data_irq(int state)
{
	cpunum_set_input_line(1, M6809_IRQ_LINE, state ? HOLD_LINE : CLEAR_LINE);
}

static struct acia6850_interface z80_acia_if =
{
	500000,
	500000,
	&m6809_z80_line,
	&z80_m6809_line,
	z80_acia_irq
};

static struct acia6850_interface m6809_acia_if =
{
	500000,
	500000,
	&z80_m6809_line,
	&m6809_z80_line,
	NULL
};

static struct acia6850_interface data_acia_if =
{
	500000,
	500000,
	&data_r,
	&data_t,
	m6809_data_irq
};


DRIVER_INIT( bfcobra )
{
	/*
        6809 ROM address and data lines are scrambled.
        This is the same scrambling as Scorpion 2.
    */
	static UINT8 datalookup[] = { 1, 3, 5, 6, 4, 2, 0, 7 };
	static UINT8 addrlookup[] = { 11, 12, 0, 2, 3, 5, 7, 9, 8, 6, 1, 4, 10, 13, 14 };

	UINT32 i;
	UINT8 *rom;
	UINT8 *tmp;

	tmp = malloc_or_die(0x8000);
	rom = memory_region(REGION_CPU2) + 0x8000;
	memcpy(tmp, rom, 0x8000);

	for (i = 0; i < 0x8000; i++)
	{
		UINT16 addr = 0;
		UINT8 x;
		UINT8 data = 0;
		UINT8 val = tmp[i];

		for (x = 0; x < 8; x ++)
		{
			data |= ((val >> x) & 1) << datalookup[x];
		}

		for (x = 0; x < 15; x ++)
		{
			addr |= ((i >> x) & 1)  << addrlookup[x];
		}

		rom[addr] = data;
	}

	free(tmp);

	/* 768kB work RAM */
	work_ram = auto_malloc(0xC0000);
	memset(work_ram, 0, 0xc0000);

	/* 128kB video RAM */
	video_ram = auto_malloc(0x20000);
	memset(video_ram, 0, 0x20000);

	bank[0] = 1;
	bank[1] = 0;
	bank[2] = 0;
	bank[3] = 0;

	/* Fixed 16kB ROM region */
	memory_set_bankptr(4, memory_region(REGION_USER1));

	/* Configure the ACIAs */
	acia6850_config(0, &z80_acia_if);
	acia6850_config(1, &m6809_acia_if);
	acia6850_config(2, &data_acia_if);

	/* TODO: Properly sort out the data ACIA */
	data_r = 1;
}

static struct upd7759_interface upd7759_interface =
{
	REGION_SOUND1
};

/* TODO: Verify */
static INTERRUPT_GEN( timer_irq )
{
	cpunum_set_input_line(1, M6809_IRQ_LINE, PULSE_LINE);
}

/* TODO */
static INTERRUPT_GEN( vblank_irq )
{
	if (z80_inten & 0x1)
	{
		cpunum_set_input_line(0, 0, HOLD_LINE);
		z80_int = 1;
	}
}

static MACHINE_DRIVER_START( bfcobra )
	MDRV_CPU_ADD(Z80, Z80_XTAL)
	MDRV_CPU_PROGRAM_MAP(z80_prog_map, 0)
	MDRV_CPU_IO_MAP(z80_io_map, 0)
	MDRV_CPU_VBLANK_INT(vblank_irq, 1)

	MDRV_CPU_ADD(M6809, M6809_XTAL)
	MDRV_CPU_PROGRAM_MAP(m6809_prog_map, 0)
	MDRV_CPU_PERIODIC_INT(timer_irq, 1000)

	MDRV_NVRAM_HANDLER(generic_0fill)
	MDRV_MACHINE_RESET(bfcobra)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_SCREEN_VISIBLE_AREA(0, 256-1, 0, 256-1)
	MDRV_PALETTE_LENGTH(256)

	MDRV_SPEAKER_STANDARD_MONO("mono")

        /* TODO: Verify */
	MDRV_SOUND_ADD(AY8910, 1500000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.01)

	MDRV_SOUND_ADD(UPD7759, UPD7759_STANDARD_CLOCK)
	MDRV_SOUND_CONFIG(upd7759_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.30)

	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(bfcobra)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/
/* TODO: Check region sizes, correct CRC/SHA1 entries */
ROM_START( qos )
	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "qos_nondata_68f4.bin", 0x08000, 0x8000, CRC(5f40005a) SHA1(180017acf6b432bc135d1090099fdf99f1e3583a) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )
	ROM_LOAD( "snd1_218.ic7", 0x00000, 0x10000, CRC(061f496d) SHA1(653d16454d909c034191813b37d14010da7258c6) )
	ROM_LOAD( "snd2_219.ic8", 0x10000, 0x10000, CRC(d7874a47) SHA1(5bbd4040c7c0299e8cc135e6c6cd05370b260e9b) )

	ROM_REGION( 0x200000, REGION_USER1, 0 )
	ROM_LOAD( "95000338.rm0", 0x000000, 0x80000, CRC(96918aae) SHA1(849ce7b8eccc89c45aacc840a73935f95788a141) )
	ROM_LOAD( "95000339.rm1", 0x080000, 0x80000, CRC(b4c6dcc0) SHA1(56d8761766dfbd5b0e71f8c3ca575e88f1bc9929) )
	ROM_LOAD( "95000340.rm2", 0x100000, 0x80000, CRC(66d121fd) SHA1(ac65cc0ac6b0a41e78a3159c21ee44f765bdb5c8) )
	ROM_LOAD( "95000341.rm3", 0x180000, 0x80000, CRC(ef13658d) SHA1(240bc589900214eac79c91a531f254a9ac2f4ef6) )
ROM_END

ROM_START( qosa )
	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "95740599.bin", 0x08000, 0x8000, CRC(bf1e321f) SHA1(51f18620f22ba2a1b110954284ddf00614d51a0e) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )
	ROM_LOAD( "snd1_218.ic7", 0x00000, 0x10000, CRC(061f496d) SHA1(653d16454d909c034191813b37d14010da7258c6) )
	ROM_LOAD( "snd2_219.ic8", 0x10000, 0x10000, CRC(d7874a47) SHA1(5bbd4040c7c0299e8cc135e6c6cd05370b260e9b) )

	ROM_REGION( 0x200000, REGION_USER1, 0 )
	ROM_LOAD( "0306.bin", 0x000000, 0x80000, CRC(c26c8f83) SHA1(6949027e1fe241cbb2e1cbbce18e47bcb0d84550) )
	ROM_LOAD( "1307.bin", 0x080000, 0x80000, CRC(94611c03) SHA1(81f545ff96ff3d44285315400da94d870c89f896) )
	ROM_LOAD( "2308.bin", 0x100000, 0x80000, CRC(f5572726) SHA1(e109265c5571d21213a6f405a13459e7bc6699bc) )
	ROM_LOAD( "3309.bin", 0x180000, 0x80000, CRC(1b5edfa8) SHA1(348488debd4aa52f064e351ed0c082274da1db2b) )
ROM_END

#if 0
/* This dump is crap */
ROM_START( qosb )
	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "qospa", 0x08000, 0x8000, CRC(5f40005a) SHA1(180017acf6b432bc135d1090099fdf99f1e3583a) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )
	ROM_LOAD( "qosrom0",	0x00000, 0x10000, BAD_DUMP CRC(061f496d) SHA1(653d16454d909c034191813b37d14010da7258c6) )
	ROM_LOAD( "qosrom1",	0x10000, 0x10000, CRC(d7874a47) SHA1(5bbd4040c7c0299e8cc135e6c6cd05370b260e9b) )

	ROM_REGION( 0x200000, REGION_USER1, 0 )
	ROM_LOAD( "qosrom0", 0x000000, 0x80000, BAD_DUMP CRC(96918aae) SHA1(849ce7b8eccc89c45aacc840a73935f95788a141) )
	ROM_LOAD( "qosrom1", 0x080000, 0x80000, CRC(b4c6dcc0) SHA1(56d8761766dfbd5b0e71f8c3ca575e88f1bc9929) )
	ROM_LOAD( "qosrom2", 0x100000, 0x80000, BAD_DUMP CRC(66d121fd) SHA1(ac65cc0ac6b0a41e78a3159c21ee44f765bdb5c8) )
	ROM_LOAD( "qosrom3", 0x180000, 0x80000, BAD_DUMP CRC(ef13658d) SHA1(240bc589900214eac79c91a531f254a9ac2f4ef6) )
ROM_END

ROM_START( inquiztr )
	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "inq6809", 0x00000, 0x08000, NO_DUMP )

	ROM_REGION( 0x20000, REGION_USER1, 0 )
	ROM_LOAD( "inqvypp1", 0x00000, 0x10000, CRC(96918aae) SHA1(849ce7b8eccc89c45aacc840a73935f95788a141) )
	ROM_LOAD( "inqvypp2", 0x10000, 0x10000, CRC(96918aae) SHA1(849ce7b8eccc89c45aacc840a73935f95788a141) )

	ROM_REGION( 0x1c2000, REGION_USER2, 0 )
	ROM_LOAD( "inqdisk.img", 0x000000, 0x1c2000, NO_DUMP )
ROM_END

/* TODO: No Z80 dump; is this actually Cobra-based? */
ROM_START( brainbox )
	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "39360122.p1", 0x08000, 0x08000, CRC(96918aae) SHA1(849ce7b8eccc89c45aacc840a73935f95788a141) )

	ROM_REGION( 0x10000, REGION_USER1, 0 )
	ROM_LOAD( "bboxz80.bin", 0x00000, 0x10000, NO_DUMP )

	ROM_REGION( 0x1c2000, REGION_USER2, 0 )
	ROM_LOAD( "bboxdisk.img", 0x000000, 0x1c2000, NO_DUMP )
ROM_END

ROM_START( everysec )
	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "escint1b", 0x08000, 0x08000, CRC(96918aae) SHA1(849ce7b8eccc89c45aacc840a73935f95788a141) )
//  ROM_LOAD( "esc12int", 0x08000, 0x08000, CRC(96918aae) SHA1(849ce7b8eccc89c45aacc840a73935f95788a141) )

	ROM_REGION( 0x10000, REGION_USER1, 0 )
	ROM_LOAD( "esccobpa", 0x00000, 0x10000, CRC(96918aae) SHA1(849ce7b8eccc89c45aacc840a73935f95788a141) )

	ROM_REGION( 0x1c2000, REGION_USER2, 0 )
	ROM_LOAD( "escdisk.img", 0x000000, 0x1c2000, NO_DUMP )
ROM_END

ROM_START( trebltop )
	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "ttpint20",	0x08000, 0x08000, CRC(96918aae) SHA1(849ce7b8eccc89c45aacc840a73935f95788a141) )

	ROM_REGION( 0x20000, REGION_USER1, 0 )
	ROM_LOAD( "ttpcob.pa",	0x00000, 0x10000, CRC(96918aae) SHA1(849ce7b8eccc89c45aacc840a73935f95788a141) )
	ROM_LOAD( "ttpcob.pb",	0x10000, 0x10000, CRC(96918aae) SHA1(849ce7b8eccc89c45aacc840a73935f95788a141) )

	ROM_REGION( 0x1c2000, REGION_USER2, 0 )
	ROM_LOAD( "ttpdisk.img", 0x000000, 0x1c2000, NO_DUMP )
ROM_END
#endif

GAME( 1992, qos,  0,   bfcobra, qos, bfcobra, ROT0, "BFM", "A Question of Sport (39-960-099)", GAME_IMPERFECT_GRAPHICS )
GAME( 1992, qosa, qos, bfcobra, qos, bfcobra, ROT0, "BFM", "A Question of Sport (39-960-089)", GAME_IMPERFECT_GRAPHICS )

//GAME( 1992, qosb, qos, bfcobra, qos, bfcobra, ROT0, "BFM", "A Question of Sport Set 2", GAME_NOT_WORKING )
//GAME( 1989, inquiztr, 0, bfcobra, inquiztr, bfcobra, ROT0, "BFM", "Inquizitor", GAME_NOT_WORKING )
//GAME( 1989, brainbox, 0, bfcobra, brainbox, bfcobra, ROT0, "BFM", "Brain Box", GAME_NOT_WORKING )
//GAME( 1990, everysec, 0, bfcobra, qos, bfcobra, ROT0, "BFM", "Every Second Counts (39-360-053)", GAME_NOT_WORKING )
//GAME( 1991, trebltop, 0, bfcobra, qos, bfcobra, ROT0, "BFM", "Treble Top (39-360-070)", GAME_NOT_WORKING )
