/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

Amstrad hardware consists of:

- General Instruments AY-3-8912 (audio and keyboard scanning)
- Intel 8255PPI (keyboard, access to AY-3-8912, cassette etc)
- Z80A CPU
- 765 FDC (disc drive interface)
- "Gate Array" (custom chip by Amstrad controlling colour, mode,
rom/ram selection


On the Amstrad, any part of the 64k memory can be access by the video
hardware (GA and CRTC - the CRTC specifies the memory address to access,
and the GA fetches 2 bytes of data for each 1us cycle.

The Z80 must also access the same ram.

To maintain the screen display, the Z80 is halted on each memory access.

The result is that timing for opcodes, appears to fall into a nice pattern,
where the time for each opcode can be measured in NOP cycles. NOP cycles is
the name I give to the time taken for one NOP command to execute.

This happens to be 1us.

From measurement, there are 64 NOPs per line, with 312 lines per screen.
This gives a total of 19968 NOPs per frame.

***************************************************************************/


#include "driver.h"
#include "cpu/z80/z80.h"
#include "machine/i8255a.h"
#include "machine/mc146818.h"
#include "machine/nec765.h"
#include "machine/ctronics.h"
#include "devices/dsk.h"
#include "devices/cassette.h"
#include "devices/snapquik.h"
#include "includes/amstrad.h"
#include "sound/ay8910.h"


#define MANUFACTURER_NAME 0x07
#define TV_REFRESH_RATE 0x10

enum {
	SYSTEM_CPC,
	SYSTEM_ALESTE,
	SYSTEM_PLUS,
	SYSTEM_GX4000
};


/* originally from drivers/amstrad.c */
// &ff,&77,&b3,&51,&a8,&d4,&62,&39,&9c,&46,&2b,&15,&8a,&cd,&ee
// This is the sequence for unlocking the ASIC in the CPC+/GX4000
// These are outed to port &bc00, after syncing the lock by outing a non-zero value then a zero to &bc00
// To lock the ASIC again, repeat the sequence without the last &ee
static const UINT8 asic_unlock_seq[15] =
{
	0xff, 0x77, 0xb3, 0x51, 0xa8, 0xd4, 0x62, 0x39, 0x9c, 0x46, 0x2b, 0x15, 0x8a, 0xcd, 0xee
};

static int amstrad_system_type;
static UINT8 aleste_mode;
static int amstrad_plus_irq_cause;  // part of the interrupt vector for IM 2.  6 = raster IRQ, 4 = DMA channel 2, 2 = DMA channel 1, 0 = DMA channel 0


/****************************
 * Gate Array data (CPC) -
 ****************************/
static struct {
	UINT8	pen_selected;		/* Pen selection */
	UINT8	mrer;				/* Mode and ROM Enable Register */
	UINT8	upper_bank;
} gate_array;


/****************************
 * ASIC data (CPC plus)
 ****************************/
static struct {
	UINT8	enabled;			/* Are CPC plus features enabled/unlocked */
	UINT8	pri;				/* Programmable raster interrupt */
	UINT8	seqptr;				/* Current position in the ASIC unlocking sequence */
	UINT8	rmr2;				/* ROM mapping register 2 */
	UINT16	split_ma_base;		/* Used to handle split screen support */
	UINT16	split_ma_started;	/* Used to handle split screen support */

	/* DMA */
	UINT8	dma_status;
	UINT8	dma_clear;			/* Set if DMA interrupts are to be cleared automatically */
	UINT8	dma_prescaler[3];	/* DMA channel prescaler */
	UINT16	dma_repeat[3];		/* Location of the DMA channel's last repeat */
	UINT16	dma_addr[3];		/* DMA channel address */
	UINT16	dma_loopcount[3];	/* Count loops taken on this channel */
	UINT16	dma_pause[3];		/* DMA pause count */
} asic;


/* Ram configuration */
static int amstrad_GateArray_RamConfiguration = 0;

/* pointers to current ram configuration selected for banks */
static unsigned char *AmstradCPC_RamBanks[4];
static unsigned char *Aleste_RamBanks[4];
static int aleste_active_page[4];  // page mapped to CPU RAM (is readable?)

/* pointer to RAM used for the CPC+ ASIC memory-mapped registers */
static UINT8 *amstrad_plus_asic_ram;

/* the hardware allows selection of 256 ROMs. Rom 0 is usually BASIC and Rom 7 is AMSDOS */
/* With the CPC hardware, if a expansion ROM is not connected, BASIC rom will be selected instead */
static unsigned char *Amstrad_ROM_Table[256];
/* data present on input of ppi, and data written to ppi output */
#define amstrad_ppi_PortA 0
#define amstrad_ppi_PortB 1
#define amstrad_ppi_PortC 2

static UINT8 ppi_port_inputs[3];
static UINT8 ppi_port_outputs[3];

static int aleste_rtc_function;
static int aleste_fdc_int;

/*------------------
  - Ram Management -
  ------------------*/
/* There are 8 different ram configurations which work on the currently selected 64k logical block.
   The following tables show the possible ram configurations :*/
static const int RamConfigurations[8 * 4] =
{
	0, 1, 2, 3, 					   /* config 0 */
	0, 1, 2, 7, 					   /* config 1 */
	4, 5, 6, 7, 					   /* config 2 */
	0, 3, 2, 7, 					   /* config 3 */
	0, 4, 2, 3, 					   /* config 4 */
	0, 5, 2, 3, 					   /* config 5 */
	0, 6, 2, 3, 					   /* config 6 */
	0, 7, 2, 3					     /* config 7 */
};


static int prev_reg;

static int amstrad_CRTC_HS = 0; /* HS = Horizontal Sync */
static int amstrad_CRTC_VS = 0;        /* VS = Vertical Sync */
//static int amstrad_CRTC_CR = 0;        /* CR = Cursor Enabled */

/* this contains the colours in machine->pens form.*/
/* this is updated from the eventlist and reflects the current state
of the render colours - these may be different to the current colour palette values */
/* colours can be changed at any time and will take effect immediatly */
static UINT16 amstrad_GateArray_render_colours[17];
/* The gate array counts CRTC HSYNC pulses. (It has a internal 6-bit counter). */
static int amstrad_CRTC_HS_Counter;
/* 2 HSYNCS after the VSYNC Counter */
static int amstrad_CRTC_HS_After_VS_Counter;

static mc6845_update_row_func draw_function;

static UINT8 mode0_lookup[256][2];
static UINT8 mode1_lookup[256][4];


/* The Amstrad CPC has a fixed palette of 27 colours generated from 3 levels of Red, Green and Blue.
The hardware allows selection of 32 colours, but these extra colours are copies of existing colours.*/

static const rgb_t amstrad_palette[32] =
{
	MAKE_RGB(0x060, 0x060, 0x060),			   /* white */
	MAKE_RGB(0x060, 0x060, 0x060),			   /* white */
	MAKE_RGB(0x000, 0x0ff, 0x060),			   /* sea green */
	MAKE_RGB(0x0ff, 0x0ff, 0x060),			   /* pastel yellow */
	MAKE_RGB(0x000, 0x000, 0x060),			   /* blue */
	MAKE_RGB(0x0ff, 0x000, 0x060),			   /* purple */
	MAKE_RGB(0x000, 0x060, 0x060),			   /* cyan */
	MAKE_RGB(0x0ff, 0x060, 0x060),			   /* pink */
	MAKE_RGB(0x0ff, 0x000, 0x060),			   /* purple */
	MAKE_RGB(0x0ff, 0x0ff, 0x060),			   /* pastel yellow */
	MAKE_RGB(0x0ff, 0x0ff, 0x000),			   /* bright yellow */
	MAKE_RGB(0x0ff, 0x0ff, 0x0ff),			   /* bright white */
	MAKE_RGB(0x0ff, 0x000, 0x000),			   /* bright red */
	MAKE_RGB(0x0ff, 0x000, 0x0ff),			   /* bright magenta */
	MAKE_RGB(0x0ff, 0x060, 0x000),			   /* orange */
	MAKE_RGB(0x0ff, 0x060, 0x0ff),			   /* pastel magenta */
	MAKE_RGB(0x000, 0x000, 0x060),			   /* blue */
	MAKE_RGB(0x000, 0x0ff, 0x060),			   /* sea green */
	MAKE_RGB(0x000, 0x0ff, 0x000),			   /* bright green */
	MAKE_RGB(0x000, 0x0ff, 0x0ff),			   /* bright cyan */
	MAKE_RGB(0x000, 0x000, 0x000),			   /* black */
	MAKE_RGB(0x000, 0x000, 0x0ff),			   /* bright blue */
	MAKE_RGB(0x000, 0x060, 0x000),			   /* green */
	MAKE_RGB(0x000, 0x060, 0x0ff),			   /* sky blue */
	MAKE_RGB(0x060, 0x000, 0x060),			   /* magenta */
	MAKE_RGB(0x060, 0x0ff, 0x060),			   /* pastel green */
	MAKE_RGB(0x060, 0x0ff, 0x060),			   /* lime */
	MAKE_RGB(0x060, 0x0ff, 0x0ff),			   /* pastel cyan */
	MAKE_RGB(0x060, 0x000, 0x000),			   /* Red */
	MAKE_RGB(0x060, 0x000, 0x0ff),			   /* mauve */
	MAKE_RGB(0x060, 0x060, 0x000),			   /* yellow */
	MAKE_RGB(0x060, 0x060, 0x0ff)	  		   /* pastel blue */
};


/* the green brightness is equal to the firmware colour index */
static const rgb_t amstrad_green_palette[32] =
{
	MAKE_RGB(0x000, 0x07F, 0x000),        /*13*/
	MAKE_RGB(0x000, 0x07F, 0x000),        /*13*/
	MAKE_RGB(0x000, 0x0BA, 0x000),        /*19*/
	MAKE_RGB(0x000, 0x0F5, 0x000),        /*25*/
	MAKE_RGB(0x000, 0x009, 0x000),        /*1*/
	MAKE_RGB(0x000, 0x044, 0x000),        /*7*/
	MAKE_RGB(0x000, 0x062, 0x000),        /*10*/
	MAKE_RGB(0x000, 0x09C, 0x000),        /*16*/
	MAKE_RGB(0x000, 0x044, 0x000),        /*7*/
	MAKE_RGB(0x000, 0x0F5, 0x000),        /*25*/
	MAKE_RGB(0x000, 0x0EB, 0x000),        /*24*/
	MAKE_RGB(0x000, 0x0FF, 0x000),        /*26*/
	MAKE_RGB(0x000, 0x03A, 0x000),        /*6*/
	MAKE_RGB(0x000, 0x04E, 0x000),        /*8*/
	MAKE_RGB(0x000, 0x093, 0x000),        /*15*/
	MAKE_RGB(0x000, 0x0A6, 0x000),        /*17*/
	MAKE_RGB(0x000, 0x009, 0x000),        /*1*/
	MAKE_RGB(0x000, 0x0BA, 0x000),        /*19*/
	MAKE_RGB(0x000, 0x0B0, 0x000),        /*18*/
	MAKE_RGB(0x000, 0x0C4, 0x000),        /*20*/
	MAKE_RGB(0x000, 0x000, 0x000),        /*0*/
	MAKE_RGB(0x000, 0x013, 0x000),        /*2*/
	MAKE_RGB(0x000, 0x058, 0x000),        /*9*/
	MAKE_RGB(0x000, 0x06B, 0x000),        /*11*/
	MAKE_RGB(0x000, 0x027, 0x000),        /*4*/
	MAKE_RGB(0x000, 0x0D7, 0x000),        /*22*/
	MAKE_RGB(0x000, 0x0CD, 0x000),        /*21*/
	MAKE_RGB(0x000, 0x0E1, 0x000),        /*23*/
	MAKE_RGB(0x000, 0x01D, 0x000),        /*3*/
	MAKE_RGB(0x000, 0x031, 0x000),        /*5*/
	MAKE_RGB(0x000, 0x075, 0x000),        /*12*/
	MAKE_RGB(0x000, 0x089, 0x000)         /*14*/
};


/*******************************************************************
  Prototypes
*******************************************************************/

static MC6845_UPDATE_ROW( amstrad_update_row_mode0 );
static MC6845_UPDATE_ROW( amstrad_update_row_mode1 );
static MC6845_UPDATE_ROW( amstrad_update_row_mode2 );
static MC6845_UPDATE_ROW( amstrad_update_row_mode3 );
static MC6845_UPDATE_ROW( aleste_update_row_mode2 );
static MC6845_UPDATE_ROW( aleste_update_row_mode3 );
static MC6845_UPDATE_ROW( amstrad_plus_update_row_mode0 );
static MC6845_UPDATE_ROW( amstrad_plus_update_row_mode1 );
static MC6845_UPDATE_ROW( amstrad_plus_update_row_mode2 );
static MC6845_UPDATE_ROW( amstrad_plus_update_row_mode3 );
static MC6845_UPDATE_ROW( amstrad_plus_update_row_sprites );
static void amstrad_rethinkMemory(running_machine *machine);


/* Initialise the palette */
PALETTE_INIT( amstrad_cpc )
{
	palette_set_colors(machine, 0, amstrad_palette, ARRAY_LENGTH(amstrad_palette));
}


PALETTE_INIT( amstrad_cpc_green )
{
	palette_set_colors(machine, 0, amstrad_green_palette, ARRAY_LENGTH(amstrad_green_palette));
}


WRITE_LINE_DEVICE_HANDLER( aleste_interrupt )
{
	if(state == CLEAR_LINE)
		aleste_fdc_int = 0;
	else
		aleste_fdc_int = 1;
}


/*************************************************************************/
/* KC Compact

The palette is defined by a colour rom. The only rom dump that exists (from the KC-Club webpage)
is 2K, which seems correct. In this rom the same 32 bytes of data is repeated throughout the rom.

When a I/O write is made to "Gate Array" to select the colour, Bit 7 and 6 are used by the
"Gate Array" to define the function, bit 7 = 0, bit 6 = 1. In the  Amstrad CPC, bits 4..0
define the hardware colour number, but in the KC Compact, it seems bits 5..0
define the hardware colour number allowing 64 colours to be chosen.

It is possible therefore that the colour rom could be reprogrammed, so that other colour
selections could be chosen allowing 64 different colours to be used. But this has not been tested
and co

colour rom byte:

Bit Function
7 not used
6 not used
5,4 Green value
3,2 Red value
1,0 Blue value

Green value, Red value, Blue value: 0 = 0%, 01/10 = 50%, 11 = 100%.
The 01 case is not used, it is unknown if this produces a different amount of colour.
*/

static unsigned char kccomp_get_colour_element(int colour_value)
{
	switch (colour_value)
	{
		case 0:
			return 0x00;
		case 1:
			return 0x60;
		case 2:
			return 0x60;
		case 3:
			return 0xff;
	}

	return 0xff;
}


/* the colour rom has the same 32 bytes repeated, but it might be possible to put a new rom in
with different data and be able to select the other entries - not tested on a real kc compact yet
and not supported by this driver */
PALETTE_INIT( kccomp )
{
	int i;

	color_prom = color_prom+0x018000;

	for (i=0; i<32; i++)
	{
		palette_set_color_rgb(machine, i,
			kccomp_get_colour_element((color_prom[i]>>2) & 0x03),
			kccomp_get_colour_element((color_prom[i]>>4) & 0x03),
			kccomp_get_colour_element((color_prom[i]>>0) & 0x03));
	}
}


/********************************************
Amstrad Plus

The Amstrad Plus has a 4096 colour palette
*********************************************/

PALETTE_INIT( amstrad_plus )
{
	int i;

	palette_set_colors(machine, 0, amstrad_palette, sizeof(amstrad_palette) / 3);
	for ( i = 0; i < 0x1000; i++ )
	{
		int r, g, b;

		g = ( i >> 8 ) & 0x0f;
		r = ( i >> 4 ) & 0x0f;
		b = i & 0x0f;

		r = ( r << 4 ) | ( r );
		g = ( g << 4 ) | ( g );
		b = ( b << 4 ) | ( b );

		palette_set_color_rgb(machine, i, r, g, b);
	}
}


PALETTE_INIT( aleste )
{
	int i;

	/* CPC Colour data is stored in the colour ROM (RFCOLDAT.BIN) at 0x140-0x17f */
	unsigned char* pal = memory_region(machine,"user4");

	for(i=0; i<32; i++)
	{
		int r,g,b;

		b = (pal[0x140+i] >> 4) & 0x03;
		g = (pal[0x140+i] >> 2) & 0x03;
		r = pal[0x140+i] & 0x03;

		r = (r << 6);
		g = (g << 6);
		b = (b << 6);

		palette_set_color_rgb(machine, i, r, g, b);
	}

	/* MSX colour palette is 6-bit RGB */
	for(i=0; i<64; i++)
	{
		int r,g,b;

		r = (i >> 4) & 0x03;
		g = (i >> 2) & 0x03;
		b = i & 0x03;

		r = (r << 6);
		g = (g << 6);
		b = (b << 6);

		palette_set_color_rgb(machine, i+32, r, g, b);
	}
}


static void amstrad_init_lookups(void)
{
	int i;

	for ( i = 0; i < 256; i++ )
	{
		mode0_lookup[i][0] = ( ( i & 0x80 ) >> 7 ) | ( ( i & 0x20 ) >> 3 ) | ( ( i & 0x08 ) >> 2 ) | ( ( i & 0x02 ) << 2 );
		mode0_lookup[i][1] = ( ( i & 0x40 ) >> 6 ) | ( ( i & 0x10 ) >> 2 ) | ( ( i & 0x04 ) >> 1 ) | ( ( i & 0x01 ) << 3 );

		mode1_lookup[i][0] = ( ( i & 0x80 ) >> 7 ) | ( ( i & 0x08 ) >> 2 );
		mode1_lookup[i][1] = ( ( i & 0x40 ) >> 6 ) | ( ( i & 0x04 ) >> 1 );
		mode1_lookup[i][2] = ( ( i & 0x20 ) >> 5 ) | ( ( i & 0x02 )      );
		mode1_lookup[i][3] = ( ( i & 0x10 ) >> 4 ) | ( ( i & 0x01 ) << 1 );
	}
}


/* Set the new screen mode (0,1,2,4) from the GateArray */
static void amstrad_vh_update_mode( void )
{
	if ( amstrad_system_type == SYSTEM_PLUS || amstrad_system_type == SYSTEM_GX4000 )
	{
		/* select a cpc plus mode */
		switch ( gate_array.mrer & 0x03 )
		{
		case 0:		/* Mode 0: 160x200, 16 colours */
			draw_function = amstrad_plus_update_row_mode0;
			break;

		case 1:		/* Mode 1: 320x200, 4 colous */
			draw_function = amstrad_plus_update_row_mode1;
			break;

		case 2:		/* Mode 2: 640x200, 2 colours */
			draw_function = amstrad_plus_update_row_mode2;
			break;

		case 3:		/* Mode 3: 160x200, 4 colours */
			draw_function = amstrad_plus_update_row_mode3;
			break;
		}
	}
	else
	{
		if ( aleste_mode & 0x02 )
		{
			/* select an aleste mode */
			switch ( gate_array.mrer & 0x03 )
			{
			case 0:		/* Aleste Mode 0 (= Amstrad CPC mode 2): 640x200, 2 colours */
				draw_function = amstrad_update_row_mode2;
				break;

			case 1:		/* Aleste mode 1: 320x200, 4 colours */
				draw_function = amstrad_update_row_mode1;
				break;

			case 2:		/* Aleste mode 2: 4 colours */
				draw_function = aleste_update_row_mode2;
				break;

			case 3:		/* Aleste mode 3: 16 colours */
				draw_function = aleste_update_row_mode3;
				break;
			}
		}
		else
		{
			/* select an original cpc mode */
			switch ( gate_array.mrer & 0x03 )
			{
			case 0:		/* Mode 0: 160x200, 16 colours */
				draw_function = amstrad_update_row_mode0;
				break;

			case 1:		/* Mode 1: 320x200, 4 colous */
				draw_function = amstrad_update_row_mode1;
				break;

			case 2:		/* Mode 2: 640x200, 2 colours */
				draw_function = amstrad_update_row_mode2;
				break;

			case 3:		/* Mode 3: 160x200, 4 colours */
				draw_function = amstrad_update_row_mode3;
				break;
			}
		}
	}
}


/*
DMA commands

0RDDh 	LOAD R,D 	Load 8 bit data D to PSG register R (0<=R<=15)
1NNNh 	PAUSE N 	Pause for N prescaled ticks (0<N<=4095)
2NNNh 	REPEAT N 	Set loop counter to N for this stream (0<N<=4095), and mark next instruction as loop start.
3xxxh 	(reserved) 	Do not use
4000h 	NOP 	No operation (64us idle)
4001h 	LOOP 	If loop counter non zero, loop back to the first instruction after REPEAT instruction and decrement loop counter.
4010h 	INT 	Interrupt the CPU
4020h 	STOP 	Stop processing the sound list.
*/

static void amstrad_plus_dma_parse(running_machine *machine, int channel)
{
	unsigned short command;

	if( asic.dma_addr[channel] & 0x01)
		asic.dma_addr[channel]++;  // align to even address

	if ( asic.dma_pause[channel] != 0 )
	{  // do nothing, this channel is paused
		asic.dma_prescaler[channel]--;
		if ( asic.dma_prescaler[channel] == 0 )
		{
			asic.dma_pause[channel]--;
			asic.dma_prescaler[channel] = amstrad_plus_asic_ram[0x2c02 + (4*channel)] + 1;
		}
		return;
	}
	command = (mess_ram[asic.dma_addr[channel]+1] << 8) + mess_ram[asic.dma_addr[channel]];
//	logerror("DMA #%i: address %04x: command %04x\n",channel,asic.dma_addr[channel],command);
	switch (command & 0xf000)
	{
	case 0x0000:  // Load PSG register
		{
			const device_config *ay8910 = devtag_get_device(machine, "ay");
			ay8910_address_w(ay8910, 0, (command & 0x0f00) >> 8);
			ay8910_data_w(ay8910, 0, command & 0x00ff);
			ay8910_address_w(ay8910, 0, prev_reg);
		}
		logerror("DMA %i: LOAD %i, %i\n",channel,(command & 0x0f00) >> 8, command & 0x00ff);
		break;
	case 0x1000:  // Pause for n HSYNCs (0 - 4095)
		asic.dma_pause[channel] = (command & 0x0fff) - 1;
		logerror("DMA %i: PAUSE %i\n",channel,command & 0x0fff);
		break;
	case 0x2000:  // Beginning of repeat loop
		asic.dma_repeat[channel] = asic.dma_addr[channel];
		asic.dma_loopcount[channel] = (command & 0x0fff);
		logerror("DMA %i: REPEAT %i\n",channel,command & 0x0fff);
		break;
	case 0x4000:  // Control functions
		if (command & 0x01) // Loop back to last Repeat instruction
		{
			if (asic.dma_loopcount[channel] > 0)
			{
				asic.dma_addr[channel] = asic.dma_repeat[channel];
				logerror("DMA %i: LOOP (%i left)\n",channel,asic.dma_loopcount[channel]);
				asic.dma_loopcount[channel]--;
			}
			else
				logerror("DMA %i: LOOP (end)\n",channel);
		}
		if (command & 0x10) // Cause interrupt
		{
			amstrad_plus_irq_cause = channel * 2;
			amstrad_plus_asic_ram[0x2c0f] |= (0x40 >> channel);
			cputag_set_input_line(machine, "maincpu", 0, ASSERT_LINE);
			logerror("DMA %i: INT\n",channel);
		}
		if (command & 0x20)  // Stop processing on this channel
		{
			asic.dma_status &= ~(0x01 << channel);
			logerror("DMA %i: STOP\n",channel);
		}
		break;
	default:
		logerror("DMA: Unknown DMA command - %04x - at address &%04x\n",command,asic.dma_addr[channel]);
	}
	asic.dma_addr[channel] += 2;  // point to next DMA instruction
}


static void amstrad_plus_handle_dma(running_machine *machine)
{
	if ( asic.dma_status & 0x01 )  // DMA channel 0
		amstrad_plus_dma_parse( machine, 0 );

	if ( asic.dma_status & 0x02 )  // DMA channel 1
		amstrad_plus_dma_parse( machine, 1 );

	if ( asic.dma_status & 0x04 )  // DMA channel 2
		amstrad_plus_dma_parse( machine, 2 );
}


/* Set the new colour from the GateArray */
static void amstrad_vh_update_colour(int PenIndex, UINT16 hw_colour_index)
{
	int val;

	amstrad_GateArray_render_colours[PenIndex] = hw_colour_index;

	if ( amstrad_system_type == SYSTEM_PLUS || amstrad_system_type == SYSTEM_GX4000 )
	{
		/* CPC+/GX4000 - normal palette changes through the Gate Array also makes the corresponding change in the ASIC palette */
		val = (amstrad_palette[hw_colour_index] & 0xf00000) >> 16; /* red */
		val |= (amstrad_palette[hw_colour_index] & 0x0000f0) >> 4; /* blue */
		amstrad_plus_asic_ram[0x2400+PenIndex*2] = val;
		val = (amstrad_palette[hw_colour_index] & 0x00f000) >> 12; /* green */
		amstrad_plus_asic_ram[0x2401+PenIndex*2] = val;
	}
}


static void aleste_vh_update_colour(int PenIndex, UINT16 hw_colour_index)
{
	amstrad_GateArray_render_colours[PenIndex] = hw_colour_index+32;
}


/* Mode 0: 160x200, 16 colours */
static MC6845_UPDATE_ROW( amstrad_update_row_mode0 )
{
	UINT16	*p = BITMAP_ADDR16( bitmap, y, 0 );
	int i;

	for ( i = 0; i < x_count; i++ )
	{
		UINT16	address = ( ( ( ma + i ) & 0x3000 ) << 2 ) | ( ( ra & 0x07 ) << 11 ) | ( ( ( ma + i ) & 0x3ff ) << 1 );
		UINT8	data0 = mess_ram[ address ];
		UINT8	data1 = mess_ram[ address + 1 ];
		UINT16	c;

		c = amstrad_GateArray_render_colours[ mode0_lookup[data0][0] ];
		*p = c; p++; *p = c; p++; *p = c; p++; *p = c; p++;
		c = amstrad_GateArray_render_colours[ mode0_lookup[data0][1] ];
		*p = c; p++; *p = c; p++; *p = c; p++; *p = c; p++;
		c = amstrad_GateArray_render_colours[ mode0_lookup[data1][0] ];
		*p = c; p++; *p = c; p++; *p = c; p++; *p = c; p++;
		c = amstrad_GateArray_render_colours[ mode0_lookup[data1][1] ];
		*p = c; p++; *p = c; p++; *p = c; p++; *p = c; p++;
	}
}


/* Mode 1: 320x200, 4 colous */
static MC6845_UPDATE_ROW( amstrad_update_row_mode1 )
{
	UINT16	*p = BITMAP_ADDR16( bitmap, y, 0 );
	int i;

	for ( i = 0; i < x_count; i++ )
	{
		UINT16	address = ( ( ( ma + i ) & 0x3000 ) << 2 ) | ( ( ra & 0x07 ) << 11 ) | ( ( ( ma + i ) & 0x3ff ) << 1 );
		UINT8	data0 = mess_ram[ address ];
		UINT8	data1 = mess_ram[ address + 1 ];
		UINT16	c;

		c = amstrad_GateArray_render_colours[ mode1_lookup[data0][0] ];
		*p = c; p++; *p = c; p++;
		c = amstrad_GateArray_render_colours[ mode1_lookup[data0][1] ];
		*p = c; p++; *p = c; p++;
		c = amstrad_GateArray_render_colours[ mode1_lookup[data0][2] ];
		*p = c; p++; *p = c; p++;
		c = amstrad_GateArray_render_colours[ mode1_lookup[data0][3] ];
		*p = c; p++; *p = c; p++;
		c = amstrad_GateArray_render_colours[ mode1_lookup[data1][0] ];
		*p = c; p++; *p = c; p++;
		c = amstrad_GateArray_render_colours[ mode1_lookup[data1][1] ];
		*p = c; p++; *p = c; p++;
		c = amstrad_GateArray_render_colours[ mode1_lookup[data1][2] ];
		*p = c; p++; *p = c; p++;
		c = amstrad_GateArray_render_colours[ mode1_lookup[data1][3] ];
		*p = c; p++; *p = c; p++;
	}
}


/* Mode 2: 640x200, 2 colours */
static MC6845_UPDATE_ROW( amstrad_update_row_mode2 )
{
	UINT16	*p = BITMAP_ADDR16( bitmap, y, 0 );
	int i;

	for ( i = 0; i < x_count; i++ )
	{
		UINT16	address = ( ( ( ma + i ) & 0x3000 ) << 2 ) | ( ( ra & 0x07 ) << 11 ) | ( ( ( ma + i ) & 0x3ff ) << 1 );
		UINT8	data0 = mess_ram[ address ];
		UINT8	data1 = mess_ram[ address + 1 ];

		*p = amstrad_GateArray_render_colours[ ( data0 & 0x80 ) >> 7 ]; p++;
		*p = amstrad_GateArray_render_colours[ ( data0 & 0x40 ) >> 6 ]; p++;
		*p = amstrad_GateArray_render_colours[ ( data0 & 0x20 ) >> 5 ]; p++;
		*p = amstrad_GateArray_render_colours[ ( data0 & 0x10 ) >> 4 ]; p++;
		*p = amstrad_GateArray_render_colours[ ( data0 & 0x08 ) >> 3 ]; p++;
		*p = amstrad_GateArray_render_colours[ ( data0 & 0x04 ) >> 2 ]; p++;
		*p = amstrad_GateArray_render_colours[ ( data0 & 0x02 ) >> 1 ]; p++;
		*p = amstrad_GateArray_render_colours[ ( data0 & 0x01 ) ]; p++;
		*p = amstrad_GateArray_render_colours[ ( data1 & 0x80 ) >> 7 ]; p++;
		*p = amstrad_GateArray_render_colours[ ( data1 & 0x40 ) >> 6 ]; p++;
		*p = amstrad_GateArray_render_colours[ ( data1 & 0x20 ) >> 5 ]; p++;
		*p = amstrad_GateArray_render_colours[ ( data1 & 0x10 ) >> 4 ]; p++;
		*p = amstrad_GateArray_render_colours[ ( data1 & 0x08 ) >> 3 ]; p++;
		*p = amstrad_GateArray_render_colours[ ( data1 & 0x04 ) >> 2 ]; p++;
		*p = amstrad_GateArray_render_colours[ ( data1 & 0x02 ) >> 1 ]; p++;
		*p = amstrad_GateArray_render_colours[ ( data1 & 0x01 ) ]; p++;
	}
}


/* Mode 3: 160x200, 4 colours */
static MC6845_UPDATE_ROW( amstrad_update_row_mode3 )
{
	UINT16	*p = BITMAP_ADDR16( bitmap, y, 0 );
	int i;

	for ( i = 0; i < x_count; i++ )
	{
		UINT16	address = ( ( ( ma + i ) & 0x3000 ) << 2 ) | ( ( ra & 0x07 ) << 11 ) | ( ( ( ma + i ) & 0x3ff ) << 1 );
		UINT8	data0 = mess_ram[ address ];
		UINT8	data1 = mess_ram[ address + 1 ];
		UINT16	c;

		c = amstrad_GateArray_render_colours[ mode1_lookup[data0][0] ];
		*p = c; p++; *p = c; p++; *p = c; p++; *p = c; p++;
		c = amstrad_GateArray_render_colours[ mode1_lookup[data0][1] ];
		*p = c; p++; *p = c; p++; *p = c; p++; *p = c; p++;
		c = amstrad_GateArray_render_colours[ mode1_lookup[data1][0] ];
		*p = c; p++; *p = c; p++; *p = c; p++; *p = c; p++;
		c = amstrad_GateArray_render_colours[ mode1_lookup[data1][1] ];
		*p = c; p++; *p = c; p++; *p = c; p++; *p = c; p++;

	}
}


/* Plus Mode 0: 160x200, 16 colours */
static MC6845_UPDATE_ROW( amstrad_plus_update_row_mode0 )
{
	UINT16	*p = BITMAP_ADDR16( bitmap, y, 0 );
	UINT16	c;
	int i;

	logerror( "amstrad_plus_update_row_mode0: ma = 0x%04x, ra = %d, y = %d, x_count = %d, cursor_x = %d\n", ma, ra, y, x_count, cursor_x );

	/* Apply horizontal background scrolling */
	for ( i = amstrad_plus_asic_ram[0x2804] & 0x0f; i > 0; i-- )
	{
		if ( amstrad_plus_asic_ram[0x2804] & 0x80 )
		{
			/* Border colour */
			c = amstrad_plus_asic_ram[0x2420] + ( amstrad_plus_asic_ram[0x2421] << 8 );
		}
		else
		{
			/* Random(?) colour data */
			c = amstrad_plus_asic_ram[0x2420] + ( amstrad_plus_asic_ram[0x2421] << 8 );
		}
		*p = c; p++;
	}

	/* Apply vertical background scrolling */
	ra += ( ( amstrad_plus_asic_ram[0x2804] >> 4 ) & 0x07 );
	if ( ra > 7 )
	{
		ma += x_count;
	}

	for ( i = 0; i < x_count; i++ )
	{
		UINT16	address = ( ( ( ma + i ) & 0x3000 ) << 2 ) | ( ( ra & 0x07 ) << 11 ) | ( ( ( ma + i ) & 0x3ff ) << 1 );
		UINT16	caddr;
		UINT8	data0 = mess_ram[ address ];
		UINT8	data1 = mess_ram[ address + 1 ];

		caddr = 0x2400 + mode0_lookup[data0][0] * 2;
		c = amstrad_plus_asic_ram[caddr] + ( amstrad_plus_asic_ram[caddr+1] << 8 );
		*p = c; p++; *p = c; p++; *p = c; p++; *p = c; p++;
		caddr = 0x2400 + mode0_lookup[data0][1] * 2;
		c = amstrad_plus_asic_ram[caddr] + ( amstrad_plus_asic_ram[caddr+1] << 8 );
		*p = c; p++; *p = c; p++; *p = c; p++; *p = c; p++;
		caddr = 0x2400 + mode0_lookup[data1][0] * 2;
		c = amstrad_plus_asic_ram[caddr] + ( amstrad_plus_asic_ram[caddr+1] << 8 );
		*p = c; p++; *p = c; p++; *p = c; p++; *p = c; p++;
		caddr = 0x2400 + mode0_lookup[data1][1] * 2;
		c = amstrad_plus_asic_ram[caddr] + ( amstrad_plus_asic_ram[caddr+1] << 8 );
		*p = c; p++; *p = c; p++; *p = c; p++; *p = c; p++;
	}
}


/* Plus Mode 1: 320x200, 4 colours */
static MC6845_UPDATE_ROW( amstrad_plus_update_row_mode1 )
{
	UINT16	*p = BITMAP_ADDR16( bitmap, y, 0 );
	UINT16	c;
	int i;

	logerror( "amstrad_plus_update_row_mode1: ma = 0x%04x, ra = %d, y = %d, x_count = %d, cursor_x = %d\n", ma, ra, y, x_count, cursor_x );

	/* Apply horizontal background scrolling */
	for ( i = amstrad_plus_asic_ram[0x2804] & 0x0f; i > 0; i-- )
	{
		if ( amstrad_plus_asic_ram[0x2804] & 0x80 )
		{
			/* Border colour */
			c = amstrad_plus_asic_ram[0x2420] + ( amstrad_plus_asic_ram[0x2421] << 8 );
		}
		else
		{
			/* Random(?) colour data */
			c = amstrad_plus_asic_ram[0x2420] + ( amstrad_plus_asic_ram[0x2421] << 8 );
		}
		*p = c; p++;
	}

	/* Apply vertical background scrolling */
	ra += ( ( amstrad_plus_asic_ram[0x2804] >> 4 ) & 0x07 );
	if ( ra > 7 )
	{
		ma += x_count;
	}

	for ( i = 0; i < x_count; i++ )
	{
		UINT16	address = ( ( ( ma + i ) & 0x3000 ) << 2 ) | ( ( ra & 0x07 ) << 11 ) | ( ( ( ma + i ) & 0x3ff ) << 1 );
		UINT16	caddr;
		UINT8	data0 = mess_ram[ address ];
		UINT8	data1 = mess_ram[ address + 1 ];

		caddr = 0x2400 + mode1_lookup[data0][0] * 2;
		c = amstrad_plus_asic_ram[caddr] + ( amstrad_plus_asic_ram[caddr+1] << 8 );
		*p = c; p++; *p = c; p++;
		caddr = 0x2400 + mode1_lookup[data0][1] * 2;
		c = amstrad_plus_asic_ram[caddr] + ( amstrad_plus_asic_ram[caddr+1] << 8 );
		*p = c; p++; *p = c; p++;
		caddr = 0x2400 + mode1_lookup[data0][2] * 2;
		c = amstrad_plus_asic_ram[caddr] + ( amstrad_plus_asic_ram[caddr+1] << 8 );
		*p = c; p++; *p = c; p++;
		caddr = 0x2400 + mode1_lookup[data0][3] * 2;
		c = amstrad_plus_asic_ram[caddr] + ( amstrad_plus_asic_ram[caddr+1] << 8 );
		*p = c; p++; *p = c; p++;
		caddr = 0x2400 + mode1_lookup[data1][0] * 2;
		c = amstrad_plus_asic_ram[caddr] + ( amstrad_plus_asic_ram[caddr+1] << 8 );
		*p = c; p++; *p = c; p++;
		caddr = 0x2400 + mode1_lookup[data1][1] * 2;
		c = amstrad_plus_asic_ram[caddr] + ( amstrad_plus_asic_ram[caddr+1] << 8 );
		*p = c; p++; *p = c; p++;
		caddr = 0x2400 + mode1_lookup[data1][2] * 2;
		c = amstrad_plus_asic_ram[caddr] + ( amstrad_plus_asic_ram[caddr+1] << 8 );
		*p = c; p++; *p = c; p++;
		caddr = 0x2400 + mode1_lookup[data1][3] * 2;
		c = amstrad_plus_asic_ram[caddr] + ( amstrad_plus_asic_ram[caddr+1] << 8 );
		*p = c; p++; *p = c; p++;
	}
}


/* Plus Mode 2: 640x200, 2 colours */
static MC6845_UPDATE_ROW( amstrad_plus_update_row_mode2 )
{
	UINT16	*p = BITMAP_ADDR16( bitmap, y, 0 );
	UINT16	c;
	int i;

	logerror( "amstrad_plus_update_row_mode2: ma = 0x%04x, ra = %d, y = %d, x_count = %d, cursor_x = %d\n", ma, ra, y, x_count, cursor_x );

	/* Apply horizontal background scrolling */
	for ( i = amstrad_plus_asic_ram[0x2804] & 0x0f; i > 0; i-- )
	{
		if ( amstrad_plus_asic_ram[0x2804] & 0x80 )
		{
			/* Border colour */
			c = amstrad_plus_asic_ram[0x2420] + ( amstrad_plus_asic_ram[0x2421] << 8 );
		}
		else
		{
			/* Random(?) colour data */
			c = amstrad_plus_asic_ram[0x2420] + ( amstrad_plus_asic_ram[0x2421] << 8 );
		}
		*p = c; p++;
	}

	/* Apply vertical background scrolling */
	ra += ( ( amstrad_plus_asic_ram[0x2804] >> 4 ) & 0x07 );
	if ( ra > 7 )
	{
		ma += x_count;
	}

	for ( i = 0; i < x_count; i++ )
	{
		UINT16	address = ( ( ( ma + i ) & 0x3000 ) << 2 ) | ( ( ra & 0x07 ) << 11 ) | ( ( ( ma + i ) & 0x3ff ) << 1 );
		UINT16	caddr;
		UINT8	data0 = mess_ram[ address ];
		UINT8	data1 = mess_ram[ address + 1 ];

		caddr = 0x2400 + ( ( data0 & 0x80 ) ? 2 : 0 );
		c = amstrad_plus_asic_ram[caddr] + ( amstrad_plus_asic_ram[caddr+1] << 8 );
		*p = c; p++;
		caddr = 0x2400 + ( ( data0 & 0x40 ) ? 2 : 0 );
		c = amstrad_plus_asic_ram[caddr] + ( amstrad_plus_asic_ram[caddr+1] << 8 );
		*p = c; p++;
		caddr = 0x2400 + ( ( data0 & 0x20 ) ? 2 : 0 );
		c = amstrad_plus_asic_ram[caddr] + ( amstrad_plus_asic_ram[caddr+1] << 8 );
		*p = c; p++;
		caddr = 0x2400 + ( ( data0 & 0x10 ) ? 2 : 0 );
		c = amstrad_plus_asic_ram[caddr] + ( amstrad_plus_asic_ram[caddr+1] << 8 );
		*p = c; p++;
		caddr = 0x2400 + ( ( data0 & 0x08 ) ? 2 : 0 );
		c = amstrad_plus_asic_ram[caddr] + ( amstrad_plus_asic_ram[caddr+1] << 8 );
		*p = c; p++;
		caddr = 0x2400 + ( ( data0 & 0x04 ) ? 2 : 0 );
		c = amstrad_plus_asic_ram[caddr] + ( amstrad_plus_asic_ram[caddr+1] << 8 );
		*p = c; p++;
		caddr = 0x2400 + ( ( data0 & 0x02 ) ? 2 : 0 );
		c = amstrad_plus_asic_ram[caddr] + ( amstrad_plus_asic_ram[caddr+1] << 8 );
		*p = c; p++;
		caddr = 0x2400 + ( ( data0 & 0x01 ) ? 2 : 0 );
		c = amstrad_plus_asic_ram[caddr] + ( amstrad_plus_asic_ram[caddr+1] << 8 );
		*p = c; p++;
		caddr = 0x2400 + ( ( data1 & 0x80 ) ? 2 : 0 );
		c = amstrad_plus_asic_ram[caddr] + ( amstrad_plus_asic_ram[caddr+1] << 8 );
		*p = c; p++;
		caddr = 0x2400 + ( ( data1 & 0x40 ) ? 2 : 0 );
		c = amstrad_plus_asic_ram[caddr] + ( amstrad_plus_asic_ram[caddr+1] << 8 );
		*p = c; p++;
		caddr = 0x2400 + ( ( data1 & 0x20 ) ? 2 : 0 );
		c = amstrad_plus_asic_ram[caddr] + ( amstrad_plus_asic_ram[caddr+1] << 8 );
		*p = c; p++;
		caddr = 0x2400 + ( ( data1 & 0x10 ) ? 2 : 0 );
		c = amstrad_plus_asic_ram[caddr] + ( amstrad_plus_asic_ram[caddr+1] << 8 );
		*p = c; p++;
		caddr = 0x2400 + ( ( data1 & 0x08 ) ? 2 : 0 );
		c = amstrad_plus_asic_ram[caddr] + ( amstrad_plus_asic_ram[caddr+1] << 8 );
		*p = c; p++;
		caddr = 0x2400 + ( ( data1 & 0x04 ) ? 2 : 0 );
		c = amstrad_plus_asic_ram[caddr] + ( amstrad_plus_asic_ram[caddr+1] << 8 );
		*p = c; p++;
		caddr = 0x2400 + ( ( data1 & 0x02 ) ? 2 : 0 );
		c = amstrad_plus_asic_ram[caddr] + ( amstrad_plus_asic_ram[caddr+1] << 8 );
		*p = c; p++;
		caddr = 0x2400 + ( ( data1 & 0x01 ) ? 2 : 0 );
		c = amstrad_plus_asic_ram[caddr] + ( amstrad_plus_asic_ram[caddr+1] << 8 );
		*p = c; p++;
	}
}


/* Plus Mode 3: 160x200, 4 colours */
static MC6845_UPDATE_ROW( amstrad_plus_update_row_mode3 )
{
	UINT16	*p = BITMAP_ADDR16( bitmap, y, 0 );
	UINT16	c;
	int i;

	logerror( "amstrad_plus_update_row_mode3: ma = 0x%04x, ra = %d, y = %d, x_count = %d, cursor_x = %d\n", ma, ra, y, x_count, cursor_x );

	/* Apply horizontal background scrolling */
	for ( i = amstrad_plus_asic_ram[0x2804] & 0x0f; i > 0; i-- )
	{
		if ( amstrad_plus_asic_ram[0x2804] & 0x80 )
		{
			/* Border colour */
			c = amstrad_plus_asic_ram[0x2420] + ( amstrad_plus_asic_ram[0x2421] << 8 );
		}
		else
		{
			/* Random(?) colour data */
			c = amstrad_plus_asic_ram[0x2420] + ( amstrad_plus_asic_ram[0x2421] << 8 );
		}
		*p = c; p++;
	}

	/* Apply vertical background scrolling */
	ra += ( ( amstrad_plus_asic_ram[0x2804] >> 4 ) & 0x07 );
	if ( ra > 7 )
	{
		ma += x_count;
	}

	for ( i = 0; i < x_count; i++ )
	{
		UINT16	address = ( ( ( ma + i ) & 0x3000 ) << 2 ) | ( ( ra & 0x07 ) << 11 ) | ( ( ( ma + i ) & 0x3ff ) << 1 );
		UINT16	caddr;
		UINT8	data0 = mess_ram[ address ];
		UINT8	data1 = mess_ram[ address + 1 ];

		caddr = 0x2400 + mode1_lookup[data0][0] * 2;
		c = amstrad_plus_asic_ram[caddr] + ( amstrad_plus_asic_ram[caddr+1] << 8 );
		*p = c; p++; *p = c; p++; *p = c; p++; *p = c; p++;
		caddr = 0x2400 + mode1_lookup[data0][1] * 2;
		c = amstrad_plus_asic_ram[caddr] + ( amstrad_plus_asic_ram[caddr+1] << 8 );
		*p = c; p++; *p = c; p++; *p = c; p++; *p = c; p++;
		caddr = 0x2400 + mode1_lookup[data1][0] * 2;
		c = amstrad_plus_asic_ram[caddr] + ( amstrad_plus_asic_ram[caddr+1] << 8 );
		*p = c; p++; *p = c; p++; *p = c; p++; *p = c; p++;
		caddr = 0x2400 + mode1_lookup[data1][1] * 2;
		c = amstrad_plus_asic_ram[caddr] + ( amstrad_plus_asic_ram[caddr+1] << 8 );
		*p = c; p++; *p = c; p++; *p = c; p++; *p = c; p++;
	}
}


/* Plus sprites: 16 16x16 sprites, 16 colours */
static MC6845_UPDATE_ROW( amstrad_plus_update_row_sprites )
{
	UINT16	*p = BITMAP_ADDR16( bitmap, y, 0 );
	int		i;

//	logerror( "amstrad_plus_update_row_sprites: ma = 0x%04x, ra = %d, y = %d, x_count = %d, cursor_x = %d\n", ma, ra, y, x_count, cursor_x );

	for ( i = 15 * 8; i >= 0; i -= 8 )
	{
		UINT8	xmag = ( amstrad_plus_asic_ram[ 0x2000 + i + 4 ] >> 2 ) & 0x03;
		UINT8	ymag = amstrad_plus_asic_ram[ 0x2000 + i + 4 ] & 0x03;

		/* Check if sprite is enabled */
		if ( xmag && ymag )
		{
			INT16	spr_x = amstrad_plus_asic_ram[ 0x2000 + i ] + ( amstrad_plus_asic_ram[ 0x2001 + i ] << 8 );
			INT16	spr_y = amstrad_plus_asic_ram[ 0x2002 + i ] + ( amstrad_plus_asic_ram[ 0x2003 + i ] << 8 );

			xmag -= 1;
			ymag -= 1;

			/* Check if sprite would appear on this scanline */
			if ( spr_y <= y && y < spr_y + ( 16 << ymag ) && spr_x < 16 * x_count && spr_x + ( 16 << xmag ) > 0 )
			{
				UINT16	spr_addr = i * 32 + ( ( ( y - spr_y ) >> ymag ) * 16 );
				int		j, k;

				for ( j = 0; j < 16; j++ )
				{
					for ( k = 0; k < ( 1 << xmag ); k++ )
					{
						INT16 x = spr_x + ( j << xmag ) + k;

						if ( x >= 0 && x < 16 * x_count )
						{
							UINT8	spr_col = ( amstrad_plus_asic_ram[ spr_addr + j ] & 0x0f ) * 2;

							if ( spr_col )
								p[x] = amstrad_plus_asic_ram[ 0x2420 + spr_col ] + ( amstrad_plus_asic_ram[ 0x2421 + spr_col ] << 8 );
						}
					}
				}
			}
		}
	}
}


/* Aleste Mode 2: 640x200(?), 4 colours */
static MC6845_UPDATE_ROW( aleste_update_row_mode2 )
{
	UINT16	*p = BITMAP_ADDR16( bitmap, y, 0 );
	int i;

//	logerror( "aleste_update_row_mode2: ma = 0x%04x, ra = %d, y = %d, x_count = %d, cursor_x = %d\n", ma, ra, y, x_count, cursor_x );

	for ( i = 0; i < x_count; i++ )
	{
		UINT16	address = ( ( ( ma + i ) & 0x2000 ) << 2 ) | ( ( ra & 0x06 ) << 11 ) | ( ( ra & 0x01 ) << 14 ) | ( ( ( ma + i ) & 0x7ff ) << 1 );
		UINT8	data0 = mess_ram[ address ];
		UINT8	data1 = mess_ram[ address + 1 ];
		UINT16	c;

		if ( ~ aleste_mode & 0x08 )
		{
			c = amstrad_GateArray_render_colours[ 16 ];
			*p = c; p++; *p = c; p++; *p = c; p++; *p = c; p++;
			*p = c; p++; *p = c; p++; *p = c; p++; *p = c; p++;
			*p = c; p++; *p = c; p++; *p = c; p++; *p = c; p++;
			*p = c; p++; *p = c; p++; *p = c; p++; *p = c; p++;
		}
		else
		{
			c = amstrad_GateArray_render_colours[ mode1_lookup[data0][0] ];
			*p = c; p++; *p = c; p++;
			c = amstrad_GateArray_render_colours[ mode1_lookup[data0][1] ];
			*p = c; p++; *p = c; p++;
			c = amstrad_GateArray_render_colours[ mode1_lookup[data0][2] ];
			*p = c; p++; *p = c; p++;
			c = amstrad_GateArray_render_colours[ mode1_lookup[data0][3] ];
			*p = c; p++; *p = c; p++;
			c = amstrad_GateArray_render_colours[ mode1_lookup[data1][0] ];
			*p = c; p++; *p = c; p++;
			c = amstrad_GateArray_render_colours[ mode1_lookup[data1][1] ];
			*p = c; p++; *p = c; p++;
			c = amstrad_GateArray_render_colours[ mode1_lookup[data1][2] ];
			*p = c; p++; *p = c; p++;
			c = amstrad_GateArray_render_colours[ mode1_lookup[data1][3] ];
			*p = c; p++; *p = c; p++;
		}
	}
}


/* Aleste Mode 3: 320x200, 16 colours */
static MC6845_UPDATE_ROW( aleste_update_row_mode3 )
{
	UINT16	*p = BITMAP_ADDR16( bitmap, y, 0 );
	int i;

//	logerror( "aleste_update_row_mode3: ma = 0x%04x, ra = %d, y = %d, x_count = %d, cursor_x = %d\n", ma, ra, y, x_count, cursor_x );

	for ( i = 0; i < x_count; i++ )
	{
		UINT16	address = ( ( ( ma + i ) & 0x2000 ) << 2 ) | ( ( ra & 0x06 ) << 11 ) | ( ( ra & 0x01 ) << 14 ) | ( ( ( ma + i ) & 0x7ff ) << 1 );
		UINT8	data0 = mess_ram[ address ];
		UINT8	data1 = mess_ram[ address + 1 ];
		UINT16	c;

		if ( ~ aleste_mode & 0x08 )
		{
			c = amstrad_GateArray_render_colours[ 16 ];
			*p = c; p++; *p = c; p++; *p = c; p++; *p = c; p++;
			*p = c; p++; *p = c; p++; *p = c; p++; *p = c; p++;
			*p = c; p++; *p = c; p++; *p = c; p++; *p = c; p++;
			*p = c; p++; *p = c; p++; *p = c; p++; *p = c; p++;
		}
		else
		{
			c = amstrad_GateArray_render_colours[ mode0_lookup[data0][0] ];
			*p = c; p++; *p = c; p++; *p = c; p++; *p = c; p++;
			c = amstrad_GateArray_render_colours[ mode0_lookup[data0][1] ];
			*p = c; p++; *p = c; p++; *p = c; p++; *p = c; p++;
			c = amstrad_GateArray_render_colours[ mode0_lookup[data1][0] ];
			*p = c; p++; *p = c; p++; *p = c; p++; *p = c; p++;
			c = amstrad_GateArray_render_colours[ mode0_lookup[data1][1] ];
			*p = c; p++; *p = c; p++; *p = c; p++; *p = c; p++;
		}
	}
}


static MC6845_UPDATE_ROW( amstrad_update_row )
{
//	logerror( "amstrad_update_row: ma = 0x%04x, ra = %d, y = %d, x_count = %d, cursor_x = %d\n", ma, ra, y, x_count, cursor_x );

	if ( draw_function )
		draw_function( device, bitmap, cliprect, ma, ra, y, x_count, cursor_x, param );
}


INLINE UINT16 amstrad_plus_translate_split( UINT16 ma, int y )
{
	if ( y == 0 )
	{
		asic.split_ma_base = 0x0000;
		asic.split_ma_started = 0x0000;
	}
	else if ( asic.enabled && amstrad_plus_asic_ram[0x2801] == y )
	{
		asic.split_ma_started = ma;
		asic.split_ma_base = ( amstrad_plus_asic_ram[0x2802] << 8 ) | amstrad_plus_asic_ram[0x2803];
	}

	return ( ma - asic.split_ma_started ) + asic.split_ma_base;
}


static MC6845_UPDATE_ROW( amstrad_plus_update_row )
{
//  logerror( "amstrad_update_row: ma = 0x%04x, ra = %d, y = %d, x_count = %d, cursor_x = %d\n", ma, ra, y, x_count, cursor_x );

	ma = amstrad_plus_translate_split( ma, y );

	if ( draw_function )
		draw_function( device, bitmap, cliprect, ma, ra, y, x_count, cursor_x, param );

	amstrad_plus_update_row_sprites( device, bitmap, cliprect, ma, ra, y, x_count, cursor_x, param );
}


static WRITE_LINE_DEVICE_HANDLER( amstrad_hsync_changed )
{
	if ( state )
	{
		amstrad_CRTC_HS_Counter++;

		if ( amstrad_CRTC_HS_After_VS_Counter != 0 )  // counters still operate regardless of PRI state
		{
			amstrad_CRTC_HS_After_VS_Counter--;

			if (amstrad_CRTC_HS_After_VS_Counter == 0)
			{
				if (amstrad_CRTC_HS_Counter >= 32)
				{
					cputag_set_input_line(device->machine, "maincpu", 0, ASSERT_LINE);
				}
				amstrad_CRTC_HS_Counter = 0;
			}
		}

		if ( amstrad_CRTC_HS_Counter >= 52 )
		{
			amstrad_CRTC_HS_Counter = 0;
			cputag_set_input_line(device->machine, "maincpu", 0, ASSERT_LINE);
		}
	}
	amstrad_CRTC_HS = state ? 1 : 0;
}


static WRITE_LINE_DEVICE_HANDLER( amstrad_plus_hsync_changed )
{
	if ( state )
	{
		amstrad_CRTC_HS_Counter++;

		if ( amstrad_CRTC_HS_After_VS_Counter != 0 )  // counters still operate regardless of PRI state
		{
			amstrad_CRTC_HS_After_VS_Counter--;

			if (amstrad_CRTC_HS_After_VS_Counter == 0)
			{
				if (amstrad_CRTC_HS_Counter >= 32)
				{
					if( asic.pri == 0 || asic.enabled == 0)
					{
						cputag_set_input_line(device->machine, "maincpu", 0, ASSERT_LINE);
					}
				}
				amstrad_CRTC_HS_Counter = 0;
			}
		}

		if ( amstrad_CRTC_HS_Counter >= 52 )
		{
			amstrad_CRTC_HS_Counter = 0;
			if ( asic.pri == 0 || asic.enabled == 0 )
			{
				cputag_set_input_line(device->machine, "maincpu", 0, ASSERT_LINE);
			}
		}

		if ( asic.enabled )
		{
			// CPC+/GX4000 Programmable Raster Interrupt (disabled if &6800 in ASIC RAM is 0)
			if ( asic.pri != 0 )
			{
				if ( asic.pri == video_screen_get_vpos( device->machine->primary_screen ) )
				{
					logerror("PRI: triggered, scanline %d\n",asic.pri);
					cputag_set_input_line(device->machine, "maincpu", 0, ASSERT_LINE);
					amstrad_plus_irq_cause = 0x06;  // raster interrupt vector
					amstrad_CRTC_HS_Counter &= ~0x20;  // ASIC PRI resets the MSB of the raster counter
				}
			}
			// CPC+/GX4000 Split screen registers  (disabled if &6801 in ASIC RAM is 0)
			if(amstrad_plus_asic_ram[0x2801] != 0)
			{
				if ( amstrad_plus_asic_ram[0x2801] == video_screen_get_vpos( device->machine->primary_screen ) )	// split occurs here (hopefully)
				{
					video_screen_update_partial( device->machine->primary_screen, video_screen_get_vpos( device->machine->primary_screen ) );
					logerror("SSCR: Split screen occured at scanline %d\n",amstrad_plus_asic_ram[0x2801]);
				}
			}
			// CPC+/GX4000 DMA channels
			amstrad_plus_handle_dma(device->machine);  // a DMA command is handled at the leading edge of HSYNC (every 64us)
		}
	}
	amstrad_CRTC_HS = state ? 1 : 0;
}


static WRITE_LINE_DEVICE_HANDLER( amstrad_vsync_changed )
{
	if ( ! amstrad_CRTC_VS && state )
	{
		/* Reset the amstrad_CRTC_HS_After_VS_Counter */
		amstrad_CRTC_HS_After_VS_Counter = 2;
	}

	amstrad_CRTC_VS = state ? 1 : 0;
}


VIDEO_START( amstrad )
{
	amstrad_init_lookups();

	draw_function = NULL;
	amstrad_CRTC_HS_After_VS_Counter = 2;
}


VIDEO_UPDATE( amstrad )
{
	const device_config *mc6845 = devtag_get_device(screen->machine, "mc6845" );
	mc6845_update( mc6845, bitmap, cliprect );
	return 0;
}


const mc6845_interface mc6845_amstrad_intf =
{
	"screen",					/* screen name */
	16,							/* number of pixels per video memory address */
	NULL,						/* begin_update */
	amstrad_update_row,			/* update_row */
	NULL,						/* end_update */
	DEVCB_NULL,					/* on_de_changed */
	DEVCB_NULL,					/* on_cur_changed */
	DEVCB_LINE(amstrad_hsync_changed),		/* on_hsync_changed */
	DEVCB_LINE(amstrad_vsync_changed),		/* on_vsync_changed */
	NULL
};


const mc6845_interface mc6845_amstrad_plus_intf =
{
	"screen",					/* screen name */
	16,							/* number of pixels per video memory address */
	NULL,						/* begin_update */
	amstrad_plus_update_row,	/* update_row */
	NULL,						/* end_update */
	DEVCB_NULL,					/* on_de_changed */
	DEVCB_NULL,					/* on_cur_changed */
	DEVCB_LINE(amstrad_plus_hsync_changed),	/* on_hsync_changed */
	DEVCB_LINE(amstrad_vsync_changed),		/* on_vsync_changed */
	NULL
};


/******************************************************************************************
    Multiface emulation
  ****************************************************************************************/

static unsigned char *multiface_ram;
static unsigned long multiface_flags;

/* stop button has been pressed */
#define MULTIFACE_STOP_BUTTON_PRESSED	0x0001
/* ram/rom is paged into memory space */
#define MULTIFACE_RAM_ROM_ENABLED		0x0002
/* when visible OUT commands are performed! */
#define MULTIFACE_VISIBLE				0x0004

/* multiface traps calls to 0x0065 when it is active.
This address has a RET and so executes no code.

It is believed that it is used to make multiface invisible to programs */

/*#define MULTIFACE_0065_TOGGLE                   0x0008*/


/* used to setup computer if a snapshot was specified */
static DIRECT_UPDATE_HANDLER( amstrad_multiface_directoverride )
{
		int pc;

		pc = cpu_get_pc(cputag_get_cpu(space->machine, "maincpu"));

		/* there are two places where CALL &0065 can be found
        in the multiface rom. At this address there is a RET.

        To disable the multiface from being detected, the multiface
        stop button must be pressed, then the program that was stopped
        must be returned to. When this is done, the multiface cannot
        be detected and the out operations to page the multiface
        ram/rom into the address space will not work! */

		/* I assume that the hardware in the multiface detects
        the PC set to 0x065 and uses this to enable/disable the multiface
        */

		/* I also use this to allow the stop button to be pressed again */
		if (pc==0x0164)
		{
			/* first call? */
			multiface_flags |= MULTIFACE_VISIBLE;
		}
		else if (pc==0x0c98)
		{
		  /* second call */

		  /* no longer visible */
		  multiface_flags &= ~(MULTIFACE_VISIBLE|MULTIFACE_STOP_BUTTON_PRESSED);

		 /* clear op base override */
				memory_set_direct_update_handler(space,0);
		}

		return pc;
}

static void multiface_init(running_machine *machine)
{
	/* after a reset the multiface is visible */
	multiface_flags = MULTIFACE_VISIBLE;

	/* allocate ram */
	multiface_ram = auto_alloc_array(machine, UINT8, 8192);
}

/* call when a system reset is done */
static void multiface_reset(void)
{
		/* stop button not pressed and ram/rom disabled */
		multiface_flags &= ~(MULTIFACE_STOP_BUTTON_PRESSED |
						MULTIFACE_RAM_ROM_ENABLED);
		/* as on the real hardware the multiface is visible after a reset! */
		multiface_flags |= MULTIFACE_VISIBLE;
}

static int multiface_hardware_enabled(running_machine *machine)
{
		if (multiface_ram!=NULL)
		{
				if ((input_port_read(machine, "multiface") & 0x01)!=0)
				{
						return 1;
				}
		}

		return 0;
}


static void multiface_rethink_memory(running_machine *machine)
{
	unsigned char *multiface_rom;

	/* multiface hardware enabled? */
	if (!multiface_hardware_enabled(machine))
		return;

	multiface_rom = &memory_region(machine, "maincpu")[0x01C000];

	if (
		((multiface_flags & MULTIFACE_RAM_ROM_ENABLED)!=0) &&
		((gate_array.mrer & 0x04) == 0)
		)
	{

		/* set bank addressess */
		memory_set_bankptr(machine,1, multiface_rom);
		memory_set_bankptr(machine,2, multiface_ram);
		memory_set_bankptr(machine,9, multiface_rom);
		memory_set_bankptr(machine,10, multiface_ram);
	}
}


/* simulate the stop button has been pressed */
static void multiface_stop(running_machine *machine)
{
	/* multiface hardware enabled? */
	if (!multiface_hardware_enabled(machine))
		return;

	/* if stop button not already pressed, do press action */
	/* pressing stop button while multiface is running has no effect */
	if ((multiface_flags & MULTIFACE_STOP_BUTTON_PRESSED)==0)
	{
		/* initialise 0065 toggle */
				/*multiface_flags &= ~MULTIFACE_0065_TOGGLE;*/

		multiface_flags |= MULTIFACE_RAM_ROM_ENABLED;

		/* stop button has been pressed, furthur pressess will not issue a NMI */
		multiface_flags |= MULTIFACE_STOP_BUTTON_PRESSED;

		gate_array.mrer &=~0x04;

		/* page rom into memory */
		multiface_rethink_memory(machine);

		/* pulse the nmi line */
		cputag_set_input_line(machine, "maincpu", INPUT_LINE_NMI, PULSE_LINE);

		/* initialise 0065 override to monitor calls to 0065 */
		memory_set_direct_update_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), amstrad_multiface_directoverride);
	}

}


/* any io writes are passed through here */
static WRITE8_HANDLER(multiface_io_write)
{
	/* multiface hardware enabled? */
		if (!multiface_hardware_enabled(space->machine))
		return;

		/* visible? */
	if (multiface_flags & MULTIFACE_VISIBLE)
	{
		if (offset==0x0fee8)
		{
			multiface_flags |= MULTIFACE_RAM_ROM_ENABLED;
			amstrad_rethinkMemory(space->machine);
		}

		if (offset==0x0feea)
		{
			multiface_flags &= ~MULTIFACE_RAM_ROM_ENABLED;
			amstrad_rethinkMemory(space->machine);
		}
	}

	/* update multiface ram with data */
		/* these are decoded fully! */
		switch ((offset>>8) & 0x0ff)
		{
				/* gate array */
				case 0x07f:
				{
						switch (data & 0x0c0)
						{
								/* pen index */
								case 0x00:
								{
									multiface_ram[0x01fcf] = data;

								}
								break;

								/* pen colour */
								case 0x040:
								{
									int pen_index;

									pen_index = multiface_ram[0x01fcf] & 0x0f;

									if (multiface_ram[0x01fcf] & 0x010)
									{

										multiface_ram[0x01fdf + pen_index] = data;
									}
									else
									{
										multiface_ram[0x01f90 + pen_index] = data & 0x01f;
									}

								}
								break;

								/* rom/mode selection */
								case 0x080:
								{

									multiface_ram[0x01fef] = data;

								}
								break;

								/* ram configuration */
								case 0x0c0:
								{

									multiface_ram[0x01fff] = data;

								}
								break;

								default:
								  break;

						}

				}
				break;


				/* crtc register index */
				case 0x0bc:
				{
						multiface_ram[0x01cff] = data;
				}
				break;

				/* crtc register write */
				case 0x0bd:
				{
						int reg_index;

						reg_index = multiface_ram[0x01cff] & 0x0f;

						multiface_ram[0x01db0 + reg_index] = data;
				}
				break;


				/* 8255 ppi control */
				case 0x0f7:
				{
				  multiface_ram[0x017ff] = data;

				}
				break;

				/* rom select */
				case 0x0df:
				{
				   multiface_ram[0x01aac] = data;
				}
				break;

				default:
				   break;

		 }

}


/*--------------------------
  - Ram and Rom management -
  --------------------------*/
/*-----------------
  - Set Lower Rom -
  -----------------*/
static void amstrad_setLowerRom(running_machine *machine)
{
	UINT8 *bank_base;

	/* b2 : "1" Lower rom area disable or "0" Lower rom area enable */
	if ( amstrad_system_type == SYSTEM_CPC || amstrad_system_type == SYSTEM_ALESTE )
	{
		if ((gate_array.mrer & (1<<2)) == 0)
		{
			bank_base = &memory_region(machine, "maincpu")[0x010000];
		}
		else
		{
			if(aleste_mode & 0x04)
				bank_base = Aleste_RamBanks[0];
			else
				bank_base = AmstradCPC_RamBanks[0];
		}
		memory_set_bankptr(machine,1, bank_base);
		memory_set_bankptr(machine,2, bank_base+0x02000);
	}
	else  // CPC+/GX4000
	{
		const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

		if ( asic.enabled && ( asic.rmr2 & 0x18 ) == 0x18 )
		{
			memory_install_read8_handler(space, 0x4000, 0x5fff, 0, 0, amstrad_plus_asic_4000_r);
			memory_install_read8_handler(space, 0x6000, 0x7fff, 0, 0, amstrad_plus_asic_6000_r);
			memory_install_write8_handler(space, 0x4000, 0x5fff, 0, 0, amstrad_plus_asic_4000_w);
			memory_install_write8_handler(space, 0x6000, 0x7fff, 0, 0, amstrad_plus_asic_6000_w);
		}
		else
		{
			memory_install_read8_handler(space, 0x4000, 0x5fff, 0, 0, SMH_BANK(3));
			memory_install_read8_handler(space, 0x6000, 0x7fff, 0, 0, SMH_BANK(4));
			memory_install_write8_handler(space, 0x4000, 0x5fff, 0, 0, SMH_BANK(11));
			memory_install_write8_handler(space, 0x6000, 0x7fff, 0, 0, SMH_BANK(12));
		}

		if(AmstradCPC_RamBanks[0] != NULL)
		{
			memory_set_bankptr(machine,1, AmstradCPC_RamBanks[0]);
			memory_set_bankptr(machine,2, AmstradCPC_RamBanks[0]+0x2000);
			memory_set_bankptr(machine,3, AmstradCPC_RamBanks[1]);
			memory_set_bankptr(machine,4, AmstradCPC_RamBanks[1]+0x2000);
			memory_set_bankptr(machine,5, AmstradCPC_RamBanks[2]);
			memory_set_bankptr(machine,6, AmstradCPC_RamBanks[2]+0x2000);
		}

		if ( (gate_array.mrer & (1<<2)) == 0)
		{  // ASIC secondary lower ROM selection (bit 5: 1 = enabled)
			if ( asic.enabled )
			{
//				logerror("L-ROM: Lower ROM enabled, cart bank %i\n", asic.rmr2 & 0x07 );
				bank_base = &memory_region(machine, "maincpu")[0x4000 * ( asic.rmr2 & 0x07 )];
				switch( asic.rmr2 & 0x18 )
				{
				case 0x00:
//					logerror("L-ROM: located at &0000\n");
					memory_set_bankptr(machine,1, bank_base);
					memory_set_bankptr(machine,2, bank_base+0x02000);
					break;
				case 0x08:
//					logerror("L-ROM: located at &4000\n");
					memory_set_bankptr(machine,3, bank_base);
					memory_set_bankptr(machine,4, bank_base+0x02000);
					break;
				case 0x10:
//					logerror("L-ROM: located at &8000\n");
					memory_set_bankptr(machine,5, bank_base);
					memory_set_bankptr(machine,6, bank_base+0x02000);
					break;
				case 0x18:
//					logerror("L-ROM: located at &0000, ASIC registers enabled\n");
					memory_set_bankptr(machine,1, bank_base);
					memory_set_bankptr(machine,2, bank_base+0x02000);
					break;
				}
			}
			else
			{
				memory_set_bankptr( machine, 1, memory_region( machine,"maincpu" ) );
				memory_set_bankptr( machine, 2, memory_region( machine,"maincpu" ) + 0x2000 );
			}
		}
	}
}


/*-----------------
  - Set Upper Rom -
  -----------------*/
static void amstrad_setUpperRom(running_machine *machine)
{
	UINT8 *bank_base = NULL;

	/* b3 : "1" Upper rom area disable or "0" Upper rom area enable */
	if ( ! ( gate_array.mrer & 0x08 ) )
	{
		bank_base = Amstrad_ROM_Table[ gate_array.upper_bank ];
	}
	else
	{
		if(aleste_mode & 0x04)
			bank_base = Aleste_RamBanks[3];
		else
			bank_base = AmstradCPC_RamBanks[3];
	}

	if (bank_base)
	{
		memory_set_bankptr(machine,7, bank_base);
		memory_set_bankptr(machine,8, bank_base+0x2000);
	}
}


/*--------------------------
  - Ram and Rom management -
  --------------------------*/

/* simplified ram configuration - e.g. only correct for 128k machines

RAM Expansion Bits
                             7 6 5 4  3  2  1  0
CPC6128                      1 1 - -  -  s2 s1 s0
Dk'tronics 256K Silicon Disk 1 1 1 b1 b0 b2 -  -

"-" - this bit is ignored. The value of this bit is not important.
"0" - this bit must be set to "0"
"1" - this bit must be set to "1"
"b0,b1,b2" - this bit is used to define the logical 64k block that the ram configuration uses
"s0,s1,s2" - this bit is used to define the ram configuration

The CPC6128 has a 64k ram expansion built-in, giving 128K of RAM in this system.
In the CPC464,CPC664 and KC compact if a ram expansion is not present, then writing to this port has no effect and the ram will be in the same arrangement as if configuration 0 had been selected.
*/
static void AmstradCPC_GA_SetRamConfiguration(running_machine *machine)
{
	int ConfigurationIndex = amstrad_GateArray_RamConfiguration & 0x07;
	int BankIndex,i;
	unsigned char *BankAddr;

/* if b5 = 0 */
	if(((amstrad_GateArray_RamConfiguration) & (1<<5)) == 0)
	{
	    for (i=0;i<4;i++)
		{
			BankIndex = RamConfigurations[(ConfigurationIndex << 2) + i];
			BankAddr = mess_ram + (BankIndex << 14);
			Aleste_RamBanks[i] = BankAddr;
			AmstradCPC_RamBanks[i] = BankAddr;
		}
	} 
	else 
	{/* Need to add the ram expansion configuration here ! */
	}
	amstrad_rethinkMemory(machine);
}


/* ASIC RAM */
/*
    ASIC RAM Layout.  Always is mapped to &4000-&7fff

    Hardware sprites: 16 sprites, 16x16, basic zooming, 15 colour (12-bit palette)
    Pixel data is 0 - 15 for sprite pen number, low 4 bits
    &4000 - &400f Pixel data for first line of first sprite
    &4010 - &401f Pixel data for second line of first sprite
      ...     ...
    &40f0 - &40ff Pixel data for last (16th) line of first sprite
      ...     ...
    &4100 - &41ff Pixel data for second sprite
    &4200 - &42ff Third sprite
      ...     ...
    &4f00 - &4fff Pixel data for last (16th) sprite

    &6000 - &607f Sprite properties (8 bytes each)
                  +0 Sprite X position LSB
                  +1 Sprite X position MSB
                  +2 Sprite Y position LSB (scanline)
                  +3 Sprite Y position MSB
                  +4 Sprite zoom - bits 3,2 X magnification, bits 1,0 Y magnification
                      Magnification: 00 = not displayed, 01 = x1, 10 = x2, 11 = x4

    Palette: LSB first, presumably GGGGBBBBxxxxRRRR
    &6400 - &641f Pen palette (12bpp, 2 bytes each, 16 total)
    &6420 - &6421 Border palette  (12bpp, 2 bytes)
    &6422 - &643f Hardware sprite palette (12bpp, 2 bytes each, 15 total)

    Programmable Raster Interrupt:
    &6800  Scanline for IRQ to be triggered after
           If 0, raster interrupts and DMA interrupts occur
           Otherwise, the PRI interrupt is triggered only

    Hardware split screen:
    &6801  Scanline for split to occur at
    &6802  LSB of screen address for split (like reg 12 of the 6845)
    &6803  MSB of the above (like reg 13 of the 6845)

    Soft Scroll Control Register:
    &6804  bits 3-0 - horizontal delay in mode 2 pixels (shifts display to the right)
           bits 6-4 - added to the 3 LSBs for the scanline address (shifts display up)
           bit 7    - extends the border by two bytes (16 mode 2 pixels), masking the bad data from the horizontal scroll

    Analogue paddle ports:
    &6808 - &680f  Analogue input, read-only, 6 bit

    PSG DMA channels:
    &6c00  DMA channel 0 address LSB
    &6c01  DMA channel 0 address MSB
    &6c02  DMA channel 0 prescaler
    &6c03  unused
    &6c04-7  DMA channel 1
    &6c08-b  DMA channel 2
    &6c0f  Control and Status register
             bit 7 - raster interrupt
             bit 6 - DMA channel 0 interrupt
             bit 5 - DMA channel 1 interrupt
             bit 4 - DMA channel 2 interrupt
             bit 3 - unused (write 0)
             bit 2 - DMA channel 2 enable
             bit 1 - DMA channel 1 enable
             bit 0 - DMA channel 0 enable

 */

WRITE8_HANDLER( amstrad_plus_asic_4000_w )
{
	video_screen_update_partial( space->machine->primary_screen, video_screen_get_vpos( space->machine->primary_screen ) );
//	logerror("ASIC: Write to register at &%04x\n",offset+0x4000);
	amstrad_plus_asic_ram[offset] = data & 0x0f;
}


WRITE8_HANDLER( amstrad_plus_asic_6000_w )
{
	video_screen_update_partial( space->machine->primary_screen, video_screen_get_vpos( space->machine->primary_screen ) );
	amstrad_plus_asic_ram[offset+0x2000] = data;
	if(offset >= 0x0400 && offset < 0x440 && ( offset & 0x01 ) ) // ASIC palette
	{
		amstrad_plus_asic_ram[ offset + 0x2000 ] = data & 0x0f;
	}
	if(offset == 0x0800)  // Programmable raster interrupt
	{
//		logerror("ASIC: Wrote %02x to PRI\n",data);
		asic.pri = data;
	}
	if(offset >= 0x0801 && offset <= 0x0803)  // Split screen registers
	{
		logerror("ASIC: Split screen at line %i, address &%04x\n",amstrad_plus_asic_ram[0x2801],amstrad_plus_asic_ram[0x2803] + (amstrad_plus_asic_ram[0x2802] << 8));
	}
	if(offset == 0x0804)  // Soft scroll register
	{
	}
	if(offset == 0x0805)  // Interrupt vector register (for IM 2, used by Pang)
	{
		// high 5 bits go to interrupt vector
		int vector;

		if ( asic.enabled )
		{
			vector = (data & 0xf8) + (amstrad_plus_irq_cause);
			cpu_set_input_line_vector(cputag_get_cpu(space->machine, "maincpu"), 0, vector);
			logerror("ASIC: IM 2 vector write %02x, data = &%02x\n",vector,data);
		}
		asic.dma_clear = data & 0x01;
	}
	// DMA channels
	switch(offset)
	{
	case 0x0c00:
	case 0x0c01:
		asic.dma_addr[0] = (amstrad_plus_asic_ram[0x2c01] << 8) + amstrad_plus_asic_ram[0x2c00];
		asic.dma_status &= ~0x01;
		logerror("ASIC: DMA 0 address set to &%04x\n",asic.dma_addr[0]);
		break;
	case 0x0c04:
	case 0x0c05:
		asic.dma_addr[1] = (amstrad_plus_asic_ram[0x2c05] << 8) + amstrad_plus_asic_ram[0x2c04];
		asic.dma_status &= ~0x02;
		logerror("ASIC: DMA 1 address set to &%04x\n",asic.dma_addr[1]);
		break;
	case 0x0c08:
	case 0x0c09:
		asic.dma_addr[2] = (amstrad_plus_asic_ram[0x2c09] << 8) + amstrad_plus_asic_ram[0x2c08];
		asic.dma_status &= ~0x04;
		logerror("ASIC: DMA 2 address set to &%04x\n",asic.dma_addr[2]);
		break;
	case 0x0c02:
		asic.dma_prescaler[0] = data + 1;
		logerror("ASIC: DMA 0 pause prescaler set to %i\n",data);
		break;
	case 0x0c06:
		asic.dma_prescaler[1] = data + 1;
		logerror("ASIC: DMA 1 pause prescaler set to %i\n",data);
		break;
	case 0x0c0a:
		asic.dma_prescaler[2] = data + 1;
		logerror("ASIC: DMA 2 pause prescaler set to %i\n",data);
		break;
	case 0x0c0f:
		asic.dma_status = data;
		logerror("ASIC: DMA status write - %02x\n",data);
		if(data & 0x40)
		{
			logerror("ASIC: DMA 0 IRQ acknowledge\n");
			cputag_set_input_line(space->machine, "maincpu", 0, CLEAR_LINE);
			amstrad_plus_irq_cause = 0x06;
			amstrad_plus_asic_ram[0x2c0f] &= ~0x40;
		}
		if(data & 0x20)
		{
			logerror("ASIC: DMA 1 IRQ acknowledge\n");
			cputag_set_input_line(space->machine, "maincpu", 0, CLEAR_LINE);
			amstrad_plus_irq_cause = 0x06;
			amstrad_plus_asic_ram[0x2c0f] &= ~0x20;
		}
		if(data & 0x10)
		{
			logerror("ASIC: DMA 2 IRQ acknowledge\n");
			cputag_set_input_line(space->machine, "maincpu", 0, CLEAR_LINE);
			amstrad_plus_irq_cause = 0x06;
			amstrad_plus_asic_ram[0x2c0f] &= ~0x10;
		}
		break;
	}
}


READ8_HANDLER( amstrad_plus_asic_4000_r )
{
//  logerror("RAM: read from &%04x\n",offset+0x4000);
	return amstrad_plus_asic_ram[offset];
}


READ8_HANDLER( amstrad_plus_asic_6000_r )
{
//  logerror("RAM: read from &%04x\n",offset+0x6000);
	// Analogue ports
	if(offset == 0x0808)
	{
		return (input_port_read(space->machine, "analog1") & 0x3f);
	}
	if(offset == 0x0809)
	{
		return (input_port_read(space->machine, "analog2") & 0x3f);
	}
	if(offset == 0x080a)
	{
		return (input_port_read(space->machine, "analog3") & 0x3f);
	}
	if(offset == 0x080b)
	{
		return (input_port_read(space->machine, "analog4") & 0x3f);
	}
	if(offset == 0x080c || offset == 0x080e)
	{
		return 0x3f;
	}
	if(offset == 0x080d || offset == 0x080f)
	{
		return 0x00;
	}
/*      if(offset == 0x0c0f)  // DMA status and control
        {
            int result = 0;
            if(amstrad_plus_irq_cause == 0x00)
                result |= 0x40;
            if(amstrad_plus_irq_cause == 0x02)
                result |= 0x20;
            if(amstrad_plus_irq_cause == 0x04)
                result |= 0x10;
            if(amstrad_plus_irq_cause == 0x06)
                result |= 0x80;
            return result;
        }
*/
	return amstrad_plus_asic_ram[offset+0x2000];
}


/* used for loading snapshot only ! */
static void AmstradCPC_PALWrite(running_machine *machine, int data)
{
	if ((data & 0x0c0)==0x0c0)
	{
		amstrad_GateArray_RamConfiguration = data;
		AmstradCPC_GA_SetRamConfiguration(machine);
	}
}


/* -------------------
   -  the Gate Array -
   -------------------
The gate array is controlled by I/O. The recommended I/O port address is &7Fxx.
The gate array is selected when bit 15 of the I/O port address is set to "0" and bit 14 of the I/O port address is set to "1".
The values of the other bits are ignored.
However, to avoid conflict with other devices in the system, these bits should be set to "1".

The function to be performed is selected by writing data to the Gate-Array, bit 7 and 6 of the data define the function selected (see table below).
It is not possible to read from the Gate-Array.

Bit 7 Bit 6 Function
0     0     Select pen
0     1     Select colour for selected pen
1     0     Select screen mode, rom configuration and interrupt control
1     1     Ram Memory Management (note 1)

Note 1 : This function is not available in the Gate-Array, but is performed by a device at the same I/O port address location. In the CPC464,CPC664 and KC compact, this function is performed in a memory-expansion (e.g. Dk'Tronics 64K Ram Expansion), if this expansion is not present then the function is not available. In the CPC6128, this function is performed by a PAL located on the main PCB, or a memory-expansion. In the 464+ and 6128+ this function is performed by the ASIC or a memory expansion. Please read the document on Ram Management for more information.*/


static void amstrad_GateArray_write(running_machine *machine, UINT8 dataToGateArray)
{
/* Get Bit 7 and 6 of the dataToGateArray = Gate Array function selected */
	switch ((dataToGateArray & 0xc0)>>6) 
	{
/* Pen selection
   -------------
Bit Value Function        Bit Value Function
5   x     not used        5   x     not used
4   1     Select border   4   0     Select pen
3   x     | ignored       3   x     | Pen Number
2   x     |               2   x       |
1   x     |               1   x     |
0   x     |               0   x     |
*/
  	case 0x00:
		/* Select Border Number, get b4 */
		/* if b4 = 0 : Select Pen Number, get b3-b0 */
		gate_array.pen_selected = ( dataToGateArray & 0x10 ) ? 0x10 : ( dataToGateArray & 0x0f );
		break;

/* Colour selection
   ----------------
Even though there is provision for 32 colours, only 27 are possible.
The remaining colours are duplicates of those already in the colour palette.

Bit Value Function
5   x     not used
4   x     | Colour number
3   x     |
2   x     |
1   x     |
0   x     |*/
	case 0x01:
		video_screen_update_partial( machine->primary_screen, video_screen_get_vpos( machine->primary_screen ) );
		amstrad_vh_update_colour( gate_array.pen_selected, (dataToGateArray & 0x1F));
		break;

/* Select screen mode and rom configuration
   ----------------------------------------
Bit Value Function
5   x     not used
4   x     Interrupt generation control
3   1     Upper rom area disable or 0 Upper rom area enable
2   1     Lower rom area disable or 0 Lower rom area enable
1   x     | Mode selection
0   x     |

Screen mode selection : The settings for bits 1 and 0 and the corresponding screen mode are given in the table below.
-----------------------
b1 b0 Screen mode
0  0  Mode 0, 160x200 resolution, 16 colours
0  1  Mode 1, 320x200 resolution, 4 colours
1  0  Mode 2, 640x200 resolution, 2 colours
1  1  Mode 3, 160x200 resolution, 4 colours (note 1)

This mode is not official. From the combinations possible, we can see that 4 modes can be defined, although the Amstrad only has 3. Mode 3 is similar to mode 0, because it has the same resolution, but it is limited to only 4 colours.
Mode changing is synchronised with HSYNC. If the mode is changed, it will take effect from the next HSYNC.

Rom configuration selection :
-----------------------------
Bit 2 is used to enable or disable the lower rom area. The lower rom area occupies memory addressess &0000-&3fff and is used to access the operating system rom. When the lower rom area is is enabled, reading from &0000-&3FFF will return data in the rom. When a value is written to &0000-&3FFF, it will be written to the ram underneath the rom. When it is disabled, data read from &0000-&3FFF will return the data in the ram.
Similarly, bit 3 controls enabling or disabling of the upper rom area. The upper rom area occupies memory addressess &C000-&FFFF and is BASIC or any expansion roms which may be plugged into a rom board/box. See the document on upper rom selection for more details. When the upper rom area enabled, reading from &c000-&ffff, will return data in the rom. When data is written to &c000-&FFFF, it will be written to the ram at the same address as the rom. When the upper rom area is disabled, and data is read from &c000-&ffff the data returned will be the data in the ram.

Bit 4 controls the interrupt generation. It can be used to delay interrupts.*/
	case 0x02:
		/* If bit 5 is enabled on a CPC Plus/GX4000 when the ASIC is unlocked, sets the lower ROM position and cart bank
           b5 = 1, b4b3 = RAM position for lower ROM area and if the ASIC registers are visible at &4000,
           b2b1b0 = cartridge bank to read from lower ROM area (0-7 only) */
		if ( amstrad_system_type == SYSTEM_PLUS || amstrad_system_type == SYSTEM_GX4000 )
		{
			if ( asic.enabled && (dataToGateArray & 0x20) )
			{
				asic.rmr2 = dataToGateArray;
			}
			else
			{
				gate_array.mrer = dataToGateArray;
			}
		}
		else
		{
			gate_array.mrer = dataToGateArray;
		}

		/* If bit 4 of the "Select screen mode and rom configuration" register of the Gate-Array is set to "1"
		 then the interrupt request is cleared and the 6-bit counter is reset to "0".  */
		if ( gate_array.mrer & 0x10 )
		{
			amstrad_CRTC_HS_Counter = 0;
			cputag_set_input_line(machine, "maincpu", 0, CLEAR_LINE);
		}

		/* b3b2 != 0 then change the state of upper or lower rom area and rethink memory */
		amstrad_setLowerRom(machine);
		amstrad_setUpperRom(machine);

		/* b1b0 mode */
		video_screen_update_partial( machine->primary_screen, video_screen_get_vpos( machine->primary_screen ) );
		amstrad_vh_update_mode();

		break;

/* Ram Memory Management
     ---------------------
This function is not available in the Gate-Array, but is performed by a device at the same I/O port address location.
In the CPC464,CPC664 and KC compact, this function is performed in a memory-expansion (e.g. Dk'Tronics 64K Ram Expansion), if this expansion is not present then the function is not available.
In the CPC6128, this function is performed by a PAL located on the main PCB, or a memory-expansion.
In the 464+ and 6128+ this function is performed by the ASIC or a memory expansion.
*/
	case 0x03:
		amstrad_GateArray_RamConfiguration = dataToGateArray;
		break;

	default:
		break;
	}
}


static WRITE8_HANDLER( aleste_msx_mapper )
{
	int page = (offset & 0x0300) >> 8;
	int ramptr = (data & 0x1f) * 0x4000;
	int rampage = data & 0x1f;
	int function = (data & 0xc0) >> 6;

	// It is assumed that functions are all mapped to each port &7cff-&7fff, and b8 and b9 are only used for RAM bank location
	switch(function)
	{
	case 0:  // Pen select (same as Gate Array?)
		amstrad_GateArray_write(space->machine, data);
		break;
	case 1:  // Colour select (6-bit palette)
		aleste_vh_update_colour( gate_array.pen_selected, data & 0x3f );
		break;
	case 2:  // Screen mode, Upper/Lower ROM select
		amstrad_GateArray_write(space->machine, data);
		break;
	case 3: // RAM banks
		switch(page)
		{
		case 0:  /* 0x0000 - 0x3fff */
			memory_set_bankptr(space->machine,1,mess_ram+ramptr);
			memory_set_bankptr(space->machine,2,mess_ram+ramptr+0x2000);
			memory_set_bankptr(space->machine,9,mess_ram+ramptr);
			memory_set_bankptr(space->machine,10,mess_ram+ramptr+0x2000);
			Aleste_RamBanks[0] = mess_ram+ramptr;
			aleste_active_page[0] = data;
			logerror("RAM: RAM location 0x%06x (page %02x) mapped to 0x0000\n",ramptr,rampage);
			break;
		case 1:  /* 0x4000 - 0x7fff */
			memory_set_bankptr(space->machine,3,mess_ram+ramptr);
			memory_set_bankptr(space->machine,4,mess_ram+ramptr+0x2000);
			memory_set_bankptr(space->machine,11,mess_ram+ramptr);
			memory_set_bankptr(space->machine,12,mess_ram+ramptr+0x2000);
			Aleste_RamBanks[1] = mess_ram+ramptr;
			aleste_active_page[1] = data;
			logerror("RAM: RAM location 0x%06x (page %02x) mapped to 0x4000\n",ramptr,rampage);
			break;
		case 2:  /* 0x8000 - 0xbfff */
			memory_set_bankptr(space->machine,5,mess_ram+ramptr);
			memory_set_bankptr(space->machine,6,mess_ram+ramptr+0x2000);
			memory_set_bankptr(space->machine,13,mess_ram+ramptr);
			memory_set_bankptr(space->machine,14,mess_ram+ramptr+0x2000);
			Aleste_RamBanks[2] = mess_ram+ramptr;
			aleste_active_page[2] = data;
			logerror("RAM: RAM location 0x%06x (page %02x) mapped to 0x8000\n",ramptr,rampage);
			break;
		case 3:  /* 0xc000 - 0xffff */
			memory_set_bankptr(space->machine,7,mess_ram+ramptr);
			memory_set_bankptr(space->machine,8,mess_ram+ramptr+0x2000);
			memory_set_bankptr(space->machine,15,mess_ram+ramptr);
			memory_set_bankptr(space->machine,16,mess_ram+ramptr+0x2000);
			Aleste_RamBanks[3] = mess_ram+ramptr;
			aleste_active_page[3] = data;
			logerror("RAM: RAM location 0x%06x (page %02x) mapped to 0xc000\n",ramptr,rampage);
			break;
		}
		break;
	}
}


/* CRTC Differences
   ----------------
The following tables list the functions that can be accessed for each type:

Type 0
b1 b0 Function Read/Write
0  0  Select internal 6845 register Write Only
0  1  Write to selected internal 6845 register Write Only
1  0  - -
1  1  Read from selected internal 6845 register Read only

Type 1
b1 b0 Function Read/Write
0  0  Select internal 6845 register Write Only
0  1  Write to selected internal 6845 register Write Only
1  0  Read Status Register Read Only
1  1  Read from selected internal 6845 register Read only

Type 2
b1 b0 Function Read/Write
0  0  Select internal 6845 register Write Only
0  1  Write to selected internal 6845 register Write Only
1  0  - -
1  1  Read from selected internal 6845 register Read only

Type 3 and 4
b1 b0 Function Read/Write
0  0  Select internal 6845 register Write Only
0  1  Write to selected internal 6845 register Write Only
1  0  Read from selected internal 6845 register Read Only
1  1  Read from selected internal 6845 register Read only
*/

/* I/O port allocation
   -------------------

Many thanks to Mark Rison for providing the original information. Thankyou to Richard Wilson for his discoveries concerning RAM management I/O decoding.

This document will explain the decoding of the I/O ports. The port address is not decoded fully which means a hardware device can be accessed through more than one address, in addition, using some addressess can access more than one element of the hardware at the same time. The CPC IN/OUT design differs from the norm in that port numbers are defined using 16 bits, as opposed to the traditional 8 bits.

IN r,(C)/OUT (C),r instructions: Bits b15-b8 come from the B register, bits b7-b0 come from "r"
IN A,(n)/OUT (n),A instructions: Bits b15-b8 come from the A register, bits b7-b0 come from "n"
Listed below are the internal hardware devices and the bit fields to which they respond. In the table:

"-" means this bit is ignored,
"0" means the bit must be set to "0" for the hardware device to respond,
"1" means the bit must be set to "1" for the hardware device to respond.
"r1" and "r0" mean a bit used to define a register

Hardware device       Read/Write Port bits
                                 b15 b14 b13 b12 b11 b10 b9  b8  b7  b6  b5  b4  b3  b2  b1  b0
Gate-Array            Write Only 0   1   -   -   -   -   -   -   -   -   -   -   -   -   -   -
RAM Configuration     Write Only 0   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -
CRTC                  Read/Write -   0   -   -   -   -   r1  r0  -   -   -   -   -   -   -   -
ROM select            Write only -   -   0   -   -   -   -   -   -   -   -   -   -   -   -   -
Printer port          Write only -   -   -   0   -   -   -   -   -   -   -   -   -   -   -   -
8255 PPI              Read/Write -   -   -   -   0   -   r1  r0  -   -   -   -   -   -   -   -
Expansion Peripherals Read/Write -   -   -   -   -   0   -   -   -   -   -   -   -   -   -   -

*/

READ8_HANDLER ( amstrad_cpc_io_r )
{
	const device_config *fdc = devtag_get_device(space->machine, "nec765");
	const device_config *mc6845 = devtag_get_device(space->machine, "mc6845" );

	unsigned char data = 0xFF;
	unsigned int r1r0 = (unsigned int)((offset & 0x0300) >> 8);
//	m6845_personality_t crtc_type;
	int page;

//	crtc_type = input_port_read_safe(space->machine, "crtc", 0);
//	m6845_set_personality(crtc_type);

	if(aleste_mode & 0x04)
	{
		if ((offset & (1<<15)) == 0)  // Aleste Mapper is readable?
		{
			page = (offset & 0x0300) >> 8;
			data = aleste_active_page[page];
			return data;
		}
	}

	/* if b14 = 0 : CRTC Read selected */
	if ((offset & (1<<14)) == 0)
	{
		switch(r1r0) 
		{
		case 0x02:
			data = mc6845_status_r( mc6845, 0 );
//			/* CRTC Type 1 : Read Status Register
//               CRTC Type 3 or 4 : Read from selected internal 6845 register */
//			switch(crtc_type) {
//			case M6845_PERSONALITY_UM6845R:
//				data = amstrad_CRTC_CR; /* Read Status Register */
//				break;
//			case M6845_PERSONALITY_AMS40489:
//			case M6845_PERSONALITY_PREASIC:
//				data = m6845_register_r(0);
//				break;
//			default:
//				break;
//			}
			break;
		case 0x03:
			/* All CRTC type : Read from selected internal 6845 register Read only */
			data = mc6845_register_r( mc6845, 0 );
			break;
		}
	}

/* if b11 = 0 : 8255 PPI Read selected - bits 9 and 8 then define the PPI function access as shown below:

b9 b8 | PPI Function Read/Write status
0  0  | Port A data  Read/Write
0  1  | Port B data  Read/Write
1  0  | Port C data  Read/Write
1  1  | Control      Write Only
*/
	if ((offset & (1<<11)) == 0)
	{
		if (r1r0 < 0x03 )
			data = i8255a_r(devtag_get_device(space->machine, "ppi8255" ), r1r0);
	}

/* if b10 = 0 : Expansion Peripherals Read selected

bits b7-b5 are used to select an expansion peripheral. Again, this is done by resetting the peripheral's bit:
Bit | Device
b7  | FDC
b6  | Reserved (was it ever used?)
b5  | Serial port

In the case of the FDC, bits b8 and b0 are used to select the specific mode of operation; all the other bits (b9,b4-b1) are ignored:
b8 b0 Function Read/Write state
0 0 FDD motor control Write Only
0 1 not used N/A
1 0 Main Status register of FDC (MSR) Read Only
1 1 Data register of FDC Read/Write

If b10 is reset but none of b7-b5 are reset, user expansion peripherals are selected.
The exception is the case where none of b7-b0 are reset (i.e. port &FBFF), which causes the expansion peripherals to reset.
 */
	if ( amstrad_system_type != SYSTEM_GX4000 )
	{
 		if ( ( offset & (1<<10) ) == 0 )
		{
			if ( ( offset & (1<<10) ) == 0 )
			{
				int b8b0 = ( ( offset & (1<<8) ) >> (8 - 1) ) | ( offset & 0x01 );

				switch (b8b0)
				{
  				case 0x02:
  					data = nec765_status_r(fdc, 0);
  					break;
  				case 0x03:
  					data = nec765_data_r(fdc, 0);
  					break;
  				default:
  					break;
				}
			}
		}
	}
	return data;
}


// Handler for checking the ASIC unlocking sequence
static void amstrad_plus_seqcheck(int data)
{
	static int prev_data;

	if(data == 0 && prev_data != 0)
	{
		asic.seqptr = 0;  // non-zero value followed by zero will sync the locking mechanism
	}
	if(data == asic_unlock_seq[asic.seqptr])
	{
		asic.seqptr++;
		if ( asic.seqptr == 14 && asic.enabled )
		{
			logerror("SYS: ASIC locked\n");
			asic.enabled = 0;
		}
		if ( asic.seqptr >= 15 )  // end of sequence
		{
			logerror("SYS: ASIC unlocked\n");
			asic.enabled = 1;
		}
	}
	prev_data = data;
}


/* Offset handler for write */
WRITE8_HANDLER ( amstrad_cpc_io_w )
{
	const device_config *fdc = devtag_get_device(space->machine, "nec765");
	const device_config *mc6845 = devtag_get_device(space->machine, "mc6845");

	static int printer_bit8_selected = FALSE;

	if ((offset & (1<<15)) == 0)
	{
		if(aleste_mode & 0x04) // Aleste mode
		{
			aleste_msx_mapper(space, offset, data);
		}
		else
		{
			/* if b15 = 0 and b14 = 1 : Gate-Array Write Selected*/
			if ((offset & (1<<14)) != 0)
	   			amstrad_GateArray_write(space->machine, data);

			/* if b15 = 0 : RAM Configuration Write Selected*/
			AmstradCPC_GA_SetRamConfiguration(space->machine);
		}
	}

	/*The Gate-Array and CRTC can't be selected simultaneously, which would otherwise cause potential display corruption.*/
	/* if b14 = 0 : CRTC Write Selected*/
	if ((offset & (1<<14)) == 0)
	{
		switch ((offset & 0x0300) >> 8) // r1r0
		{
  		case 0x00:		/* Select internal 6845 register Write Only */
			mc6845_address_w( mc6845, 0, data );
			if ( amstrad_system_type == SYSTEM_PLUS || amstrad_system_type == SYSTEM_GX4000 )
				amstrad_plus_seqcheck(data);

			/* printer port d7 */
			if (data == 0x0c && amstrad_system_type == SYSTEM_PLUS)
				printer_bit8_selected = TRUE;

			break;
		case 0x01:		/* Write to selected internal 6845 register Write Only */
			video_screen_update_partial( space->machine->primary_screen, video_screen_get_vpos( space->machine->primary_screen ) );
			mc6845_register_w( mc6845, 0, data );

			/* printer port bit 8 */
			if (printer_bit8_selected && amstrad_system_type == SYSTEM_PLUS)
			{
				const device_config *printer = devtag_get_device(space->machine, "centronics");
				centronics_d7_w(printer, BIT(data, 3));
				printer_bit8_selected = FALSE;
			}

			break;
		default:
			break;
		}
	}

	/* ROM select before GateArrayWrite ?*/
	/* b13 = 0 : ROM select Write Selected*/
	if ((offset & (1<<13)) == 0)
	{
		gate_array.upper_bank = data;
		amstrad_setUpperRom( space->machine );
	}

	/* b12 = 0 : Printer port Write Selected*/
	if(amstrad_system_type != SYSTEM_GX4000)
	{
		if ((offset & (1<<12)) == 0)
		{
			const device_config *printer = devtag_get_device(space->machine, "centronics");

			/* CPC has a 7-bit data port, bit 8 is the STROBE signal */
			centronics_data_w(printer, 0, data & 0x7f);
			centronics_strobe_w(printer, BIT(data, 7));
		}
	}

	/* if b11 = 0 : 8255 PPI Write selected - bits 9 and 8 then define the PPI function access as shown below:
	b9 b8 | PPI Function Read/Write status
	0  0  | Port A data  Read/Write
	0  1  | Port B data  Read/Write
	1  0  | Port C data  Read/Write
	1  1  | Control      Write Only
	*/
	if ((offset & (1<<11)) == 0)
	{
		unsigned int Index = ((offset & 0x0300) >> 8);

		i8255a_w(devtag_get_device(space->machine, "ppi8255" ), Index, data);
	}

	/* if b10 = 0 : Expansion Peripherals Write selected */
	if ((offset & (1<<10)) == 0)
	{
/* bits b7-b5 are used to select an expansion peripheral. This is done by resetting the peripheral's bit:
Bit | Device
b7  | FDC
b6  | Reserved (was it ever used?)
b5  | Serial port

In the case of the FDC, bits b8 and b0 are used to select the specific mode of operation;
all the other bits (b9,b4-b1) are ignored:
b8 b0 Function Read/Write state
0 0 FDD motor control Write Only
0 1 not used N/A
1 0 Main Status register of FDC (MSR) Read Only
1 1 Data register of FDC Read/Write

If b10 is reset but none of b7-b5 are reset, user expansion peripherals are selected.
The exception is the case where none of b7-b0 are reset (i.e. port &FBFF), which causes the expansion peripherals to reset.
*/
		if(amstrad_system_type != SYSTEM_GX4000)
		{
			if ((offset & (1<<7)) == 0)
			{
				unsigned int b8b0 = ((offset & 0x0100) >> (8 - 1)) | (offset & 0x01);

				switch (b8b0)
				{
				case 0x00:
					/* FDC Motor Control - Bit 0 defines the state of the FDD motor:
					 * "1" the FDD motor will be active.
					 * "0" the FDD motor will be in-active.*/
					floppy_drive_set_motor_state(image_from_devtype_and_index(space->machine, IO_FLOPPY, 0), (data & 0x01));
					floppy_drive_set_motor_state(image_from_devtype_and_index(space->machine, IO_FLOPPY, 1), (data & 0x01));
					floppy_drive_set_ready_state(image_from_devtype_and_index(space->machine, IO_FLOPPY, 0), 1,1);
					floppy_drive_set_ready_state(image_from_devtype_and_index(space->machine, IO_FLOPPY, 1), 1,1);
				  break;

				case 0x03: /* Write Data register of FDC */
					nec765_data_w(fdc, 0,data);
					break;

				default:
					break;
				}
			}
		}
	}

	/*	Aleste Extend Port:
		D0 - VRAM Bank 0/1 (64K)
		D1 - MODE 0-Norm ,1-High resolution
		D2 - MAPMOD 0-Amstrad ,1-Yamaha mapper
		D3 - MAP Page 0/1 (256K)
		D4 - 580WI53
		D5 - 0-AY8910 ,1-512WI1 */
	if(offset == 0xfabf)
	{
		aleste_mode = data;
		logerror("EXTEND: Port &FABF write 0x%02x\n",data);
		mc6845_set_clock( mc6845, ( aleste_mode & 0x02 ) ? ( XTAL_16MHz / 8 ) : ( XTAL_16MHz / 16 ) );
	}

	multiface_io_write(space, offset, data);
}


/* load CPCEMU style snapshots */
static void amstrad_handle_snapshot(running_machine *machine, unsigned char *pSnapshot)
{
	const address_space* space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	const device_config *mc6845 = devtag_get_device(space->machine, "mc6845" );
	const device_config *ay8910 = devtag_get_device(machine, "ay");
	int RegData;
	int i;

	/* init Z80 */
	RegData = (pSnapshot[0x011] & 0x0ff) | ((pSnapshot[0x012] & 0x0ff)<<8);
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_AF, RegData);

	RegData = (pSnapshot[0x013] & 0x0ff) | ((pSnapshot[0x014] & 0x0ff)<<8);
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_BC, RegData);

	RegData = (pSnapshot[0x015] & 0x0ff) | ((pSnapshot[0x016] & 0x0ff)<<8);
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_DE, RegData);

	RegData = (pSnapshot[0x017] & 0x0ff) | ((pSnapshot[0x018] & 0x0ff)<<8);
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_HL, RegData);

	RegData = (pSnapshot[0x019] & 0x0ff) ;
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_R, RegData);

	RegData = (pSnapshot[0x01a] & 0x0ff);
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_I, RegData);

	if ((pSnapshot[0x01b] & 1)==1)
	{
		cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_IFF1, 1);
	}
	else
	{
		cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_IFF1, 0);
	}

	if ((pSnapshot[0x01c] & 1)==1)
	{
		cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_IFF2, 1);
	}
	else
	{
		cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_IFF2, 0);
	}

	RegData = (pSnapshot[0x01d] & 0x0ff) | ((pSnapshot[0x01e] & 0x0ff)<<8);
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_IX, RegData);

	RegData = (pSnapshot[0x01f] & 0x0ff) | ((pSnapshot[0x020] & 0x0ff)<<8);
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_IY, RegData);

	RegData = (pSnapshot[0x021] & 0x0ff) | ((pSnapshot[0x022] & 0x0ff)<<8);
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_SP, RegData);
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), REG_GENSP, RegData);

	RegData = (pSnapshot[0x023] & 0x0ff) | ((pSnapshot[0x024] & 0x0ff)<<8);

	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_PC, RegData);
//	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), REG_SP, RegData);

	RegData = (pSnapshot[0x025] & 0x0ff);
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_IM, RegData);

	RegData = (pSnapshot[0x026] & 0x0ff) | ((pSnapshot[0x027] & 0x0ff)<<8);
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_AF2, RegData);

	RegData = (pSnapshot[0x028] & 0x0ff) | ((pSnapshot[0x029] & 0x0ff)<<8);
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_BC2, RegData);

	RegData = (pSnapshot[0x02a] & 0x0ff) | ((pSnapshot[0x02b] & 0x0ff)<<8);
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_DE2, RegData);

	RegData = (pSnapshot[0x02c] & 0x0ff) | ((pSnapshot[0x02d] & 0x0ff)<<8);
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_HL2, RegData);

	/* init GA */
	for (i=0; i<17; i++)
	{
		amstrad_GateArray_write(machine, i);

		amstrad_GateArray_write(machine, ((pSnapshot[0x02f + i] & 0x01f) | 0x040));
	}

	amstrad_GateArray_write(machine, pSnapshot[0x02e] & 0x01f);

	amstrad_GateArray_write(machine, ((pSnapshot[0x040] & 0x03f) | 0x080));

	AmstradCPC_PALWrite(machine, ((pSnapshot[0x041] & 0x03f) | 0x0c0));

	/* init CRTC */
	for (i=0; i<18; i++)
	{
		mc6845_address_w( mc6845, 0, i );
		mc6845_register_w( mc6845, 0, pSnapshot[0x043+i] & 0xff );
	}

	mc6845_address_w( mc6845, 0, i );

	/* upper rom selection */
	gate_array.upper_bank = pSnapshot[0x055];

	/* PPI */
	i8255a_w(devtag_get_device(machine, "ppi8255"),3,pSnapshot[0x059] & 0x0ff);

	i8255a_w(devtag_get_device(machine, "ppi8255"),0,pSnapshot[0x056] & 0x0ff);
	i8255a_w(devtag_get_device(machine, "ppi8255"),1,pSnapshot[0x057] & 0x0ff);
	i8255a_w(devtag_get_device(machine, "ppi8255"),2,pSnapshot[0x058] & 0x0ff);

	/* PSG */
	for (i=0; i<16; i++)
	{
		ay8910_address_w(ay8910, 0, i);
		ay8910_data_w(ay8910, 0, pSnapshot[0x05b + i] & 0x0ff);
	}

	ay8910_address_w(ay8910, 0, pSnapshot[0x05a]);

	{
		int MemSize;
		int MemorySize;

		MemSize = (pSnapshot[0x06b] & 0x0ff) | ((pSnapshot[0x06c] & 0x0ff)<<8);

		if (MemSize==128)
		{
			MemorySize = 128*1024;
		}
		else
		{
			MemorySize = 64*1024;
		}

		memcpy(mess_ram, &pSnapshot[0x0100], MemorySize);
	}
	amstrad_rethinkMemory(machine);
}


/* sets up for a machine reset
All hardware is reset and the firmware is completely initialized
Once all tables and jumpblocks have been set up,
control is passed to the default entry in rom 0*/
static void amstrad_reset_machine(running_machine *machine)
{
	/* enable lower rom (OS rom) */
	amstrad_GateArray_write(machine, 0x089);

	/* set ram config 0 */
	amstrad_GateArray_write(machine, 0x0c0);

  // Get manufacturer name and TV refresh rate from PCB link (dipswitch for mess emulation)
	ppi_port_inputs[amstrad_ppi_PortB] = (((input_port_read(machine, "solder_links")&MANUFACTURER_NAME)<<1) | (input_port_read(machine, "solder_links")&TV_REFRESH_RATE));

	if ( amstrad_system_type == SYSTEM_PLUS || amstrad_system_type == SYSTEM_GX4000 )
		memset(amstrad_plus_asic_ram,0,16384);  // clear ASIC RAM

	multiface_reset();
}


/*------------------
  - Rethink Memory -
  ------------------*/
static void amstrad_rethinkMemory(running_machine *machine)
{
	/* the following is used for banked memory read/writes and for setting up
     * opcode and opcode argument reads */

	/* bank 0 - 0x0000..0x03fff */
	amstrad_setLowerRom(machine);

	/* bank 1 - 0x04000..0x07fff */
	if ( amstrad_system_type == SYSTEM_CPC || amstrad_system_type == SYSTEM_ALESTE || asic.enabled == 0 )
	{
		if(aleste_mode & 0x04)
		{
			memory_set_bankptr(machine,3, Aleste_RamBanks[1]);
			memory_set_bankptr(machine,4, Aleste_RamBanks[1]+0x2000);
			/* bank 2 - 0x08000..0x0bfff */
			memory_set_bankptr(machine,5, Aleste_RamBanks[2]);
			memory_set_bankptr(machine,6, Aleste_RamBanks[2]+0x2000);
		}
		else
		{
			memory_set_bankptr(machine,3, AmstradCPC_RamBanks[1]);
			memory_set_bankptr(machine,4, AmstradCPC_RamBanks[1]+0x2000);
			/* bank 2 - 0x08000..0x0bfff */
			memory_set_bankptr(machine,5, AmstradCPC_RamBanks[2]);
			memory_set_bankptr(machine,6, AmstradCPC_RamBanks[2]+0x2000);
		}
	}
	else
		amstrad_setLowerRom(machine);

	/* bank 3 - 0x0c000..0x0ffff */
	amstrad_setUpperRom(machine);

	/* other banks */
	if(aleste_mode & 0x04)
	{
		memory_set_bankptr(machine,9, Aleste_RamBanks[0]);
		memory_set_bankptr(machine,10, Aleste_RamBanks[0]+0x2000);
		memory_set_bankptr(machine,11, Aleste_RamBanks[1]);
		memory_set_bankptr(machine,12, Aleste_RamBanks[1]+0x2000);
		memory_set_bankptr(machine,13, Aleste_RamBanks[2]);
		memory_set_bankptr(machine,14, Aleste_RamBanks[2]+0x2000);
		memory_set_bankptr(machine,15, Aleste_RamBanks[3]);
		memory_set_bankptr(machine,16, Aleste_RamBanks[3]+0x2000);
	}
	else
	{
		memory_set_bankptr(machine,9, AmstradCPC_RamBanks[0]);
		memory_set_bankptr(machine,10, AmstradCPC_RamBanks[0]+0x2000);
		memory_set_bankptr(machine,11, AmstradCPC_RamBanks[1]);
		memory_set_bankptr(machine,12, AmstradCPC_RamBanks[1]+0x2000);
		memory_set_bankptr(machine,13, AmstradCPC_RamBanks[2]);
		memory_set_bankptr(machine,14, AmstradCPC_RamBanks[2]+0x2000);
		memory_set_bankptr(machine,15, AmstradCPC_RamBanks[3]);
		memory_set_bankptr(machine,16, AmstradCPC_RamBanks[3]+0x2000);
	}

	/* multiface hardware enabled? */
	if (multiface_hardware_enabled(machine)) 
	{
		multiface_rethink_memory(machine);
	}
}


static void kccomp_reset_machine(running_machine *machine)
{
	/* enable lower rom (OS rom) */
	amstrad_GateArray_write(machine, 0x089);

	/* set ram config 0 */
	amstrad_GateArray_write(machine, 0x0c0);
}


VIDEO_EOF( amstrad )
{
	if (input_port_read_safe(machine, "multiface", 0) & 0x02)
	{
		multiface_stop(machine);
	}
}


/* ---------------------------------------
   - 27.05.2004 - PSG function selection -
   ---------------------------------------
The databus of the PSG is connected to PPI Port A.
Data is read from/written to the PSG through this port.

The PSG function, defined by the BC1,BC2 and BDIR signals, is controlled by bit 7 and bit 6 of PPI Port C.

PSG function selection:
-----------------------
Function

BDIR = PPI Port C Bit 7 and BC1 = PPI Port C Bit 6

PPI Port C Bit | PSG Function
BDIR BC1       |
0    0         | Inactive
0    1         | Read from selected PSG register. When function is set, the PSG will make the register data available to PPI Port A
1    0         | Write to selected PSG register. When set, the PSG will take the data at PPI Port A and write it into the selected PSG register
1    1         | Select PSG register. When set, the PSG will take the data at PPI Port A and select a register
*/
/* PSG function selected */
static unsigned char amstrad_Psg_FunctionSelected;
static void update_psg(running_machine *machine)
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	const device_config *ay8910 = devtag_get_device(machine, "ay");
	
	if(aleste_mode & 0x20)  // RTC selected
	{
		switch(aleste_rtc_function)
		{
		case 0x02:  // AS
			mc146818_port_w(space, 0,ppi_port_outputs[amstrad_ppi_PortA]);
			break;
		case 0x04:  // DS write
			mc146818_port_w(space, 1,ppi_port_outputs[amstrad_ppi_PortA]);
			break;
		case 0x05:  // DS read
			ppi_port_inputs[amstrad_ppi_PortA] = mc146818_port_r(space, 1);
			break;
		}
		return;
	}
  switch (amstrad_Psg_FunctionSelected)
  {
  	case 0:
		{/* Inactive */
		} break;
  	case 1:
		{/* b6 = 1 ? : Read from selected PSG register and make the register data available to PPI Port A */
  			ppi_port_inputs[amstrad_ppi_PortA] = ay8910_r(ay8910, 0);
  		}
		break;
  	case 2:
		{/* b7 = 1 ? : Write to selected PSG register and write data to PPI Port A */
  			ay8910_data_w(ay8910, 0, ppi_port_outputs[amstrad_ppi_PortA]);
  		}
		break;
  	case 3:
		{/* b6 and b7 = 1 ? : The register will now be selected and the user can read from or write to it.  The register will remain selected until another is chosen.*/
  			ay8910_address_w(ay8910, 0, ppi_port_outputs[amstrad_ppi_PortA]);
			prev_reg = ppi_port_outputs[amstrad_ppi_PortA];
  		}
		break;
  	default: {
    } break;
	}
}


/* Read/Write 8255 PPI port A (connected to AY-3-8912 databus) */
READ8_DEVICE_HANDLER ( amstrad_ppi_porta_r )
{
	update_psg(device->machine);
	return ppi_port_inputs[amstrad_ppi_PortA];
}


WRITE8_DEVICE_HANDLER ( amstrad_ppi_porta_w )
{
	ppi_port_outputs[amstrad_ppi_PortA] = data;
	update_psg(device->machine);
}


/* - Read PPI Port B -
   -------------------
Bit Description
7   Cassette read data
6   Parallel/Printer port ready signal ("1" = not ready, "0" = Ready)
5   /EXP signal on expansion port (note 6)
4   50/60 Hz (link on PCB. For this MESS driver I have used the dipswitch feature) (note 5)
3   | PCB links to define manufacturer name. For this MESS driver I have used the dipswitch feature. (note 1) (note 4)
2   | (note 2)
1   | (note 3)
0   VSYNC State from 6845. "1" = VSYNC active, "0" = VSYNC inactive

Note:

1 On CPC464,CPC664,CPC6128 and GX4000 this is LK3 on the PCB. On the CPC464+ and CPC6128+ this is LK103 on the PCB. On the KC compact this is "1".
2 On CPC464,CPC664,CPC6128 and GX4000 this is LK2 on the PCB. On the CPC464+ and CPC6128+ this is LK102 on the PCB. On the KC compact this is "0".
3 On CPC464,CPC664,CPC6128 and GX4000 this is LK1 on the PCB. On the CPC464+ and CPC6128+ this is LK101 on the PCB. On the KC compact this is /TEST signal from the expansion port.
4 On the CPC464,CPC664,CPC6128,CPC464+,CPC6128+ and GX4000 bits 3,2 and 1 define the manufacturer name. See below to see the options available. The manufacturer name is defined on the PCB and cannot be changed through software.
5 On the CPC464,CPC664,CPC6128,CPC464+,CPC6128+ and GX4000 bit 4 defines the Screen refresh frequency. "1" = 50 Hz, "0" = 60 Hz. This is defined on the PCB and cannot be changed with software. On the KC compact bit 4 is "1"
6 This bit is connected to /EXP signal on the expansion port.
  On the KC Compact this bit is used to define bit 7 of the printer data.
  On the CPC, it is possible to use this bit to define bit 7 of the printer data, so a 8-bit printer port is made, with a hardware modification,
  On the CPC this can be used by a expansion device to report it's presence. "1" = device connected, "0" = device not connected. This is not always used by all expansion devices.
*/

READ8_DEVICE_HANDLER (amstrad_ppi_portb_r)
{
	int data = 0;
/* Set b7 with cassette tape input */
	if(amstrad_system_type != SYSTEM_GX4000)
	{
		if (cassette_input(devtag_get_device(device->machine, "cassette" )) > 0.03) 
		{
			data |= (1<<7);
		}
	}
/* Set b6 with Parallel/Printer port ready */
	if(amstrad_system_type != SYSTEM_GX4000)
	{
		const device_config *printer = devtag_get_device(device->machine, "centronics");
		data |= centronics_busy_r(printer) << 6;
	}
/* Set b4-b1 50Hz/60Hz state and manufacturer name defined by links on PCB */
	data |= (ppi_port_inputs[amstrad_ppi_PortB] & 0x1e);

/*  Set b0 with VSync state from the CRTC */
	data |= amstrad_CRTC_VS;

	if(aleste_mode & 0x04)
	{
		if(aleste_fdc_int == 0)
			data &= ~0x02;
		else
			data |= 0x02;
	}

	return data;
}


/* 26-May-2005 - PPI Port C
   -----------------------
Bit Description  Usage
7   PSG BDIR     | PSG function selection
6   PSG BC1      |
5                Cassette Write data
4                Cassette Motor Control set bit to "1" for motor on, or "0" for motor off
3                | Keyboard line Select keyboard line to be scanned (0-15)
2                |
1                |
0                |*/

/* previous_ppi_portc_w value */
static int previous_ppi_portc_w;

WRITE8_DEVICE_HANDLER ( amstrad_ppi_portc_w )
{
	int changed_data;

	previous_ppi_portc_w = ppi_port_outputs[amstrad_ppi_PortC];
/* Write the data to Port C */
	changed_data = previous_ppi_portc_w^data;

	ppi_port_outputs[amstrad_ppi_PortC] = data;

/* get b7 and b6 (PSG Function Selected */
	amstrad_Psg_FunctionSelected = ((data & 0xC0)>>6);

	/* MC146818 function */
	aleste_rtc_function = data & 0x07;

	/* Perform PSG function */
	update_psg(device->machine);

	/* b5 Cassette Write data */
	if(amstrad_system_type != SYSTEM_GX4000)
	{
		if ((changed_data & 0x20) != 0) 
		{
			cassette_output(devtag_get_device(device->machine, "cassette" ),
				((data & 0x20) ? -1.0 : +1.0));
		}
	}

	/* b4 Cassette Motor Control */
	if(amstrad_system_type != SYSTEM_GX4000)
	{
		if ((changed_data & 0x10) != 0)
		{
			cassette_change_state(devtag_get_device(device->machine, "cassette" ),
				((data & 0x10) ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED),
				CASSETTE_MASK_MOTOR);
		}
	}
}


/* Additional notes for the AY-3-8912 in the CPC design
Port A of the AY-3-8912 is connected to the keyboard.
The data for a selected keyboard line can be read through Port A, as long as it is defined as input.
The operating system and I believe all programs assume this port has been defined as input. (NWC has found a bug in the Multiface 2 software. The Multiface does not reprogram the input/output state of the AY-3-8912's registers, therefore if port A is programmed as output, the keyboard will be unresponsive and it will not be possible to use the Multiface functions.)
When port B is defined as input (bit 7 of register 7 is set to "0"), a read of this port will return &FF.
*/

/* read PSG port A */
READ8_HANDLER ( amstrad_psg_porta_read )
{
	/* Read CPC Keyboard
   If keyboard matrix line 11-14 are selected, the byte is always &ff.
   After testing on a real CPC, it is found that these never change, they always return &FF. */

	static const char *const keynames[] = { "keyboard_row_0", "keyboard_row_1", "keyboard_row_2", "keyboard_row_3", "keyboard_row_4",
										"keyboard_row_5", "keyboard_row_6", "keyboard_row_7", "keyboard_row_8", "keyboard_row_9",
										"keyboard_row_10" };

	if ( ( ppi_port_outputs[amstrad_ppi_PortC] & 0x0F ) > 10)
	{
		return 0xFF;
	}
	else
	{
		if(aleste_mode == 0x08 && ( ppi_port_outputs[amstrad_ppi_PortC] & 0x0F ) == 10)
			return 0xff;

		return input_port_read_safe(space->machine, keynames[ppi_port_outputs[amstrad_ppi_PortC] & 0x0F], 0) & 0xFF;
	}
}


/* called when cpu acknowledges int */
/* reset top bit of interrupt line counter */
/* this ensures that the next interrupt is no closer than 32 lines */
static IRQ_CALLBACK(amstrad_cpu_acknowledge_int)
{
	// DMA interrupts can be automatically cleared if bit 0 of &6805 is set to 0
	if( asic.enabled && amstrad_plus_irq_cause != 0x06 && asic.dma_clear & 0x01)
	{
		logerror("IRQ: Not cleared, IRQ was called by DMA [%i]\n",amstrad_plus_irq_cause);
		amstrad_plus_asic_ram[0x2c0f] &= ~0x80;  // not a raster interrupt, so this bit is reset
		return (amstrad_plus_asic_ram[0x2805] & 0xf8) | amstrad_plus_irq_cause;
	}
	cputag_set_input_line(device->machine,"maincpu", 0, CLEAR_LINE);
	amstrad_CRTC_HS_Counter &= 0x1F;
	if ( asic.enabled )
	{
		if(amstrad_plus_irq_cause == 6)  // bit 7 is set "if last interrupt acknowledge cycle was caused by a raster interrupt"
			amstrad_plus_asic_ram[0x2c0f] |= 0x80;
		else
		{
			amstrad_plus_asic_ram[0x2c0f] &= ~0x80;
			amstrad_plus_asic_ram[0x2c0f] &= (0x40 >> amstrad_plus_irq_cause/2);
		}
		return (amstrad_plus_asic_ram[0x2805] & 0xf8) | amstrad_plus_irq_cause;
	}
	return 0xFF;
}


/* the following timings have been measured! */
static const UINT8 amstrad_cycle_table_op[256] = {
	 4, 12,  8,  8,  4,  4,  8,  4,  4, 12,  8,  8,  4,  4,  8,  4,
	12, 12,  8,  8,  4,  4,  8,  4, 12, 12,  8,  8,  4,  4,  8,  4,
	 8, 12, 20,  8,  4,  4,  8,  4,  8, 12, 20,  8,  4,  4,  8,  4,
	 8, 12, 16,  8, 12, 12, 12,  4,  8, 12, 16,  8,  4,  4,  8,  4,
	 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
	 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
	 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
	 8,  8,  8,  8,  8,  8,  4,  8,  4,  4,  4,  4,  4,  4,  8,  4,
	 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
	 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
	 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
	 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
	 8, 12, 12, 12, 12, 16,  8, 16,  8, 12, 12,  4, 12, 20,  8, 16,
	 8, 12, 12, 12, 12, 16,  8, 16,  8,  4, 12, 12, 12,  4,  8, 16,
	 8, 12, 12, 24, 12, 16,  8, 16,  8,  4, 12,  4, 12,  4,  8, 16,
	 8, 12, 12,  4, 12, 16,  8, 16,  8,  8, 12,  4, 12,  4,  8, 16
};

static const UINT8 amstrad_cycle_table_cb[256]=
{
	 4,  4,  4,  4,  4,  4, 12,  4,  4,  4,  4,  4,  4,  4, 12,  4,
	 4,  4,  4,  4,  4,  4, 12,  4,  4,  4,  4,  4,  4,  4, 12,  4,
	 4,  4,  4,  4,  4,  4, 12,  4,  4,  4,  4,  4,  4,  4, 12,  4,
	 4,  4,  4,  4,  4,  4, 12,  4,  4,  4,  4,  4,  4,  4, 12,  4,
	 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
	 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
	 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
	 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
	 4,  4,  4,  4,  4,  4, 12,  4,  4,  4,  4,  4,  4,  4, 12,  4,
	 4,  4,  4,  4,  4,  4, 12,  4,  4,  4,  4,  4,  4,  4, 12,  4,
	 4,  4,  4,  4,  4,  4, 12,  4,  4,  4,  4,  4,  4,  4, 12,  4,
	 4,  4,  4,  4,  4,  4, 12,  4,  4,  4,  4,  4,  4,  4, 12,  4,
	 4,  4,  4,  4,  4,  4, 12,  4,  4,  4,  4,  4,  4,  4, 12,  4,
	 4,  4,  4,  4,  4,  4, 12,  4,  4,  4,  4,  4,  4,  4, 12,  4,
	 4,  4,  4,  4,  4,  4, 12,  4,  4,  4,  4,  4,  4,  4, 12,  4,
	 4,  4,  4,  4,  4,  4, 12,  4,  4,  4,  4,  4,  4,  4, 12,  4
};

static const UINT8 amstrad_cycle_table_ed[256]=
{
	 4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	 4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	 4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	 4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	12, 12, 12, 20,  4, 12,  4,  8, 12, 12, 12, 20,  4, 12,  4,  8,
	12, 12, 12, 20,  4, 12,  4,  8, 12, 12, 12, 20,  4, 12,  4,  8,
	12, 12, 12, 20,  4, 12,  4, 16, 12, 12, 12, 20,  4, 12,  4, 16,
	12, 12, 12, 20,  4, 12,  4,  4, 12, 12, 12, 20,  4, 12,  4,  4,
	 4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	 4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	16, 12, 16, 16,  4,  4,  4,  4, 16, 12, 16, 16,  4,  4,  4,  4,
	16, 12, 16, 16,  4,  4,  4,  4, 16, 12, 16, 16,  4,  4,  4,  4,
	 4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	 4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	 4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	 4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4
};

static const UINT8 amstrad_cycle_table_xy[256]=
{
	 4, 12,  8,  8,  4,  4,  8,  4,  4, 12,  8,  8,  4,  4,  8,  4,
	12, 12,  8,  8,  4,  4,  8,  4, 12, 12,  8,  8,  4,  4,  8,  4,
	 8, 12, 20,  8,  4,  4,  8,  4,  8, 12, 20,  8,  4,  4,  8,  4,
	 8, 12, 16,  8, 20, 20, 20,  4,  8, 12, 16,  8,  4,  4,  8,  4,
	 4,  4,  4,  4,  4,  4, 16,  4,  4,  4,  4,  4,  4,  4, 16,  4,
	 4,  4,  4,  4,  4,  4, 16,  4,  4,  4,  4,  4,  4,  4, 16,  4,
	 4,  4,  4,  4,  4,  4, 16,  4,  4,  4,  4,  4,  4,  4, 16,  4,
	16, 16, 16, 16, 16, 16,  4, 16,  4,  4,  4,  4,  4,  4, 16,  4,
	 4,  4,  4,  4,  4,  4, 16,  4,  4,  4,  4,  4,  4,  4, 16,  4,
	 4,  4,  4,  4,  4,  4, 16,  4,  4,  4,  4,  4,  4,  4, 16,  4,
	 4,  4,  4,  4,  4,  4, 16,  4,  4,  4,  4,  4,  4,  4, 16,  4,
	 4,  4,  4,  4,  4,  4, 16,  4,  4,  4,  4,  4,  4,  4, 16,  4,
	 8, 12, 12, 12, 12, 16,  8, 16,  8, 12, 12,  4, 12, 20,  8, 16,
	 8, 12, 12, 12, 12, 16,  8, 16,  8,  4, 12, 12, 12,  4,  8, 16,
	 8, 12, 12, 24, 12, 16,  8, 16,  8,  4, 12,  4, 12,  4,  8, 16,
	 8, 12, 12,  4, 12, 16,  8, 16,  8,  8, 12,  4, 12,  4,  8, 16
};

static const UINT8 amstrad_cycle_table_xycb[256]=
{
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20
};

static const UINT8 amstrad_cycle_table_ex[256]=
{
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 4,  0,  0,  0,  0,  0,  0,  0,  4,  0,  0,  0,  0,  0,  0,  0,
	 4,  0,  0,  0,  0,  0,  0,  0,  4,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 4,  8,  4,  4,  0,  0,  0,  0,  4,  8,  4,  4,  0,  0,  0,  0,
	 8,  0,  0,  0,  8,  0,  0,  0,  8,  0,  0,  0,  8,  0,  0,  0,
	 8,  0,  0,  0,  8,  0,  0,  0,  8,  0,  0,  0,  8,  0,  0,  0,
	 8,  0,  0,  0,  8,  0,  0,  0,  8,  0,  0,  0,  8,  0,  0,  0,
	 8,  0,  0,  0,  8,  0,  0,  0,  8,  0,  0,  0,  8,  0,  0,  0
};

static void amstrad_common_init(running_machine *machine)
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	aleste_mode = 0;

	gate_array.mrer = 0;
	amstrad_GateArray_RamConfiguration = 0;
	amstrad_CRTC_HS_Counter = 2;

	memory_install_read8_handler(space, 0x0000, 0x1fff, 0, 0, SMH_BANK(1));
	memory_install_read8_handler(space, 0x2000, 0x3fff, 0, 0, SMH_BANK(2));

	memory_install_read8_handler(space, 0x4000, 0x5fff, 0, 0, SMH_BANK(3));
	memory_install_read8_handler(space, 0x6000, 0x7fff, 0, 0, SMH_BANK(4));

	memory_install_read8_handler(space, 0x8000, 0x9fff, 0, 0, SMH_BANK(5));
	memory_install_read8_handler(space, 0xa000, 0xbfff, 0, 0, SMH_BANK(6));

	memory_install_read8_handler(space, 0xc000, 0xdfff, 0, 0, SMH_BANK(7));
	memory_install_read8_handler(space, 0xe000, 0xffff, 0, 0, SMH_BANK(8));

	memory_install_write8_handler(space, 0x0000, 0x1fff, 0, 0, SMH_BANK(9));
	memory_install_write8_handler(space, 0x2000, 0x3fff, 0, 0, SMH_BANK(10));

	memory_install_write8_handler(space, 0x4000, 0x5fff, 0, 0, SMH_BANK(11));
	memory_install_write8_handler(space, 0x6000, 0x7fff, 0, 0, SMH_BANK(12));

	memory_install_write8_handler(space, 0x8000, 0x9fff, 0, 0, SMH_BANK(13));
	memory_install_write8_handler(space, 0xa000, 0xbfff, 0, 0, SMH_BANK(14));

	memory_install_write8_handler(space, 0xc000, 0xdfff, 0, 0, SMH_BANK(15));
	memory_install_write8_handler(space, 0xe000, 0xffff, 0, 0, SMH_BANK(16));

	device_reset(cputag_get_cpu(space->machine, "maincpu"));
	if ( amstrad_system_type == SYSTEM_CPC || amstrad_system_type == SYSTEM_ALESTE )
		cpu_set_input_line_vector(cputag_get_cpu(machine, "maincpu"), 0, 0xff);
	else
		cpu_set_input_line_vector(cputag_get_cpu(machine, "maincpu"), 0, 0x00);

	if(amstrad_system_type != SYSTEM_GX4000)
	{
		floppy_drive_set_geometry(image_from_devtype_and_index(machine, IO_FLOPPY, 0),  FLOPPY_DRIVE_SS_40);
		floppy_drive_set_geometry(image_from_devtype_and_index(machine, IO_FLOPPY, 1),  FLOPPY_DRIVE_SS_40);
	}

	/* The opcode timing in the Amstrad is different to the opcode
    timing in the core for the Z80 CPU.

    The Amstrad hardware issues a HALT for each memory fetch.
    This has the effect of stretching the timing for Z80 opcodes,
    so that they are all multiple of 4 T states long. All opcode
    timings are a multiple of 1us in length. */

	/* Using the cool code Juergen has provided, I will override
    the timing tables with the values for the amstrad */
	z80_set_cycle_tables(cputag_get_cpu(machine, "maincpu"),
		(void*)amstrad_cycle_table_op,
		(void*)amstrad_cycle_table_cb,
		(void*)amstrad_cycle_table_ed,
		(void*)amstrad_cycle_table_xy,
		(void*)amstrad_cycle_table_xycb,
		(void*)amstrad_cycle_table_ex);

	/* Juergen is a cool dude! */
	cpu_set_irq_callback(cputag_get_cpu(machine, "maincpu"), amstrad_cpu_acknowledge_int);
}


MACHINE_RESET( amstrad )
{
	int i;
	UINT8 *rom = memory_region(machine, "maincpu");

	amstrad_system_type = SYSTEM_CPC;

	for (i=0; i<256; i++)
	{
		Amstrad_ROM_Table[i] = &rom[0x014000];
	}

	Amstrad_ROM_Table[7] = &rom[0x018000];
	amstrad_common_init(machine);
	amstrad_reset_machine(machine);
//	amstrad_init_palette(machine);

	multiface_init(machine);

}


MACHINE_START( plus )
{
	amstrad_plus_asic_ram = memory_region(machine, "user1");  // 16kB RAM for ASIC, memory-mapped registers.
}


MACHINE_RESET( plus )
{
	int i;
	UINT8 *rom = memory_region(machine, "maincpu");

	amstrad_system_type = SYSTEM_PLUS;

	for (i=0; i<128; i++)  // fill ROM table
	{
		Amstrad_ROM_Table[i] = &rom[0x4000];  // BASIC in system cart
	}
	for(i=128;i<160;i++)
	{
		Amstrad_ROM_Table[i] = &rom[(i-128)*0x4000];
	}
	Amstrad_ROM_Table[7] = &rom[0xc000];  // AMSDOS in system cart

	asic.enabled = 0;
	asic.seqptr = 0;
	asic.pri = 0;  // disable PRI
	asic.dma_status = 0;  // disable all DMA channels
	asic.dma_addr[0] = 0;
	asic.dma_prescaler[0] = 0;
	asic.dma_addr[1] = 0;
	asic.dma_prescaler[1] = 0;
	asic.dma_addr[2] = 0;
	asic.dma_prescaler[2] = 0;
	asic.dma_clear = 1;  // by default, DMA interrupts must be cleared by writing to the DSCR (&6c0f)
	amstrad_plus_irq_cause = 6;

	amstrad_common_init(machine);
	amstrad_reset_machine(machine);
	amstrad_plus_asic_ram[0x2805] = 0x01;  // interrupt vector is undefined at startup, except that bit 0 is always 1.
	AmstradCPC_GA_SetRamConfiguration(machine);
	amstrad_GateArray_write(machine, 0x081); // Epyx World of Sports requires upper ROM to be enabled by default

	//  multiface_init();
}


MACHINE_RESET( gx4000 )
{
	int i;
	UINT8 *rom = memory_region(machine, "maincpu");

	amstrad_system_type = SYSTEM_GX4000;

	for (i=0; i<128; i++)  // fill ROM table
	{
		Amstrad_ROM_Table[i] = &rom[0x4000];
	}
	for(i=128;i<160;i++)
	{
		Amstrad_ROM_Table[i] = &rom[(i-128)*0x4000];
	}
	Amstrad_ROM_Table[7] = &rom[0xc000];

	asic.enabled = 0;
	asic.seqptr = 0;
	asic.pri = 0;  // disable PRI
	asic.dma_status = 0;  // disable all DMA channels
	asic.dma_addr[0] = 0;
	asic.dma_prescaler[0] = 0;
	asic.dma_addr[1] = 0;
	asic.dma_prescaler[1] = 0;
	asic.dma_addr[2] = 0;
	asic.dma_prescaler[2] = 0;
	asic.dma_clear = 1;  // by default, DMA interrupts must be cleared by writing to the DSCR (&6c0f)
	amstrad_plus_irq_cause = 6;

	amstrad_common_init(machine);
	amstrad_reset_machine(machine);
	amstrad_plus_asic_ram[0x2805] = 0x01;  // interrupt vector is undefined at startup, except that bit 0 is always 1.
	AmstradCPC_GA_SetRamConfiguration(machine);
	amstrad_GateArray_write(machine, 0x081); // Epyx World of Sports requires upper ROM to be enabled by default
	//  multiface_init();
}


MACHINE_RESET( kccomp )
{
	int i;
	UINT8 *rom = memory_region(machine, "maincpu");

	amstrad_system_type = SYSTEM_CPC;

	for (i=0; i<256; i++)
	{
		Amstrad_ROM_Table[i] = &rom[0x014000];
	}

	amstrad_common_init(machine);
	kccomp_reset_machine(machine);

	/* bit 1 = /TEST. When 0, KC compact will enter data transfer
    sequence, where another system using the expansion port signals
    DATA2,DATA1, /STROBE and DATA7 can transfer 256 bytes of program.
    When the program has been transfered, it will be executed. This
    is not supported in the driver */
	/* bit 3,4 are tied to +5V, bit 2 is tied to 0V */
	ppi_port_inputs[amstrad_ppi_PortB] = (1<<4) | (1<<3) | 2;
}


MACHINE_RESET( aleste )
{
	int i;
	UINT8 *rom = memory_region(machine, "maincpu");

	amstrad_system_type = SYSTEM_ALESTE;

	for (i=0; i<256; i++)
	{
		Amstrad_ROM_Table[i] = &rom[0x014000];
	}

	Amstrad_ROM_Table[3] = &rom[0x01c000];  // MSX-DOS / BIOS
	Amstrad_ROM_Table[7] = &rom[0x018000];  // AMSDOS
	amstrad_common_init(machine);
	amstrad_reset_machine(machine);

	floppy_drive_set_geometry(image_from_devtype_and_index(machine, IO_FLOPPY, 0),  FLOPPY_DRIVE_DS_80);
	floppy_drive_set_geometry(image_from_devtype_and_index(machine, IO_FLOPPY, 1),  FLOPPY_DRIVE_DS_80);
}


/* load snapshot */
SNAPSHOT_LOAD(amstrad)
{
	UINT8 *snapshot;

	/* get file size */
	if (snapshot_size < 8)
		return INIT_FAIL;

	snapshot = malloc(snapshot_size);
	if (!snapshot)
		return INIT_FAIL;

	/* read whole file */
	image_fread(image, snapshot, snapshot_size);

	if (memcmp(snapshot, "MV - SNA", 8))
	{
		free(snapshot);
		return INIT_FAIL;
	}

	amstrad_handle_snapshot(image->machine, snapshot);
	free(snapshot);
	return INIT_PASS;
}


DEVICE_IMAGE_LOAD(amstrad_plus_cartridge)
{
	// load CPC Plus / GX4000 cartridge image
	// Format is RIFF: RIFF header chunk contains "AMS!"
	// Chunks should be 16k, but may vary
	// Chunks labeled 'cb00' represent Cartridge block 0, and is loaded to &0000-&3fff
	//                'cb01' represent Cartridge block 1, and is loaded to &4000-&7fff
	//                ... and so on.

	unsigned char header[12];  // RIFF chunk
	char chunkid[4];  // chunk ID (4 character code - cb00, cb01, cb02... upto cb31 (max 512kB), other chunks are ignored)
	char chunklen[4];  // chunk length (always little-endian)
	int chunksize;  // chunk length, calcaulated from the above
	int ramblock;  // 16k RAM block chunk is to be loaded in to
	int result;
	unsigned int bytes_to_read;  // total bytes to read, as mame_feof doesn't react to EOF without trying to go past it.
	unsigned char* mem = memory_region(image->machine, "maincpu");

	logerror("IMG: loading CPC+ cartridge file\n");
	// load RIFF chunk
	result = image_fread(image,header,12);
	if(result != 12)
	{
		logerror("IMG: failed to read from cart image\n");
		return INIT_FAIL;
	}
	if(strncmp((char *)header,"RIFF",4) != 0)
	{
		// not an RIFF format file, assume raw binary (*.bin)
		image_fseek(image,0,SEEK_SET);
		ramblock = 0;
		while(!image_feof(image))
		{
			result = image_fread(image,mem+(0x4000*ramblock),0x4000);
			if(result < 0x4000)
			{
				logerror("BIN: block %i loaded is smaller than 16kB in size\n",ramblock);
				return INIT_FAIL;
			}
			ramblock++;
		}
	}
	else
	{
		// Is RIFF format (*.cpr)
		if(strncmp((char*)(header+8),"AMS!",4) != 0)
		{
			logerror("CPR: not an Amstrad CPC cartridge image\n");
			return INIT_FAIL;
		}

		bytes_to_read = header[4] + (header[5] << 8) + (header[6] << 16)+ (header[7] << 24);
		bytes_to_read -= 4;  // account for AMS! header
		logerror("CPR: Data to read: %i bytes\n",bytes_to_read);
		// read some chunks
		while(bytes_to_read > 0)
		{
			result = image_fread(image,chunkid,4);
			if(result != 4)
			{
				logerror("CPR: failed to read from cart image\n");
				return INIT_FAIL;
			}
			bytes_to_read -= result;
			result = image_fread(image,chunklen,4);
			if(result != 4)
			{
				logerror("CPR: failed to read from cart image\n");
				return INIT_FAIL;
			}
			bytes_to_read -= result;
			// calculate little-endian value, just to be sure
			chunksize = chunklen[0] + (chunklen[1] << 8) + (chunklen[2] << 16) + (chunklen[3] << 24);

			if(strncmp(chunkid,"cb",2) == 0)
			{
				// load chunk into RAM
				// find out what block this is
				ramblock = (chunkid[2] - 0x30) * 10;
				ramblock += chunkid[3] - 0x30;
				logerror("CPR: Loading chunk into RAM block %i ['%4s']\n",ramblock,chunkid);
				if(ramblock >= 0 && ramblock < 32)
				{
					// clear RAM block
					memset(mem+(0x4000*ramblock),0,0x4000);

					// load block into ROM area
					if(chunksize > 16384)
						chunksize = 16384;
					result = image_fread(image,mem+(0x4000*ramblock),chunksize);
					if(result != chunksize)
					{
						logerror("CPR: Read %i-byte chunk, expected %i bytes\n",result,chunksize);
						return INIT_FAIL;
					}
					bytes_to_read -= chunksize;
					logerror("CPR: Loaded %i-byte chunk into RAM block %i\n",result,ramblock);
				}
			}
			else
			{
				logerror("CPR: Unknown chunk '%4s', skipping %i bytes\n",chunkid,chunksize);
				if(chunksize != 0)
				{
					image_fseek(image,chunksize,SEEK_CUR);
					bytes_to_read -= chunksize;
				}
			}
		}
	}


	return INIT_PASS;
}


/*static DEVICE_IMAGE_LOAD( aleste )
{
	if (device_load_basicdsk_floppy(image)==INIT_PASS)
	{
		basicdsk_set_geometry(image, 80, 2, 9, 512, 0x01, 0, FALSE);
		return INIT_PASS;
	}
	return INIT_FAIL;
}*/
