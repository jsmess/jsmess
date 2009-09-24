/******************************************************************************


    Philips CD-I
    -------------------

    Preliminary MAME driver by Roberto Fresca, David Haywood & Angelo Salese
    MESS improvements by Harmony


*******************************************************************************


    *** Hardware Notes ***

    These are actually the specs of the Philips CD-i console.

    Identified:

    - CPU:  1x Philips SCC 68070 CCA84 (16 bits Microprocessor, PLCC) @ 15 MHz
    - VSC:  1x Philips SCC 66470 CAB (Video and System Controller, QFP)

    - Crystals:   1x 30.0000 MHz.
                  1x 19.6608 MHz.


*******************************************************************************

STATUS:

BIOSes will run until attempting an OS call using the methods described here:
http://www.icdia.co.uk/microware/tech/tech_2.pdf

TODO:

-Proper handling of the 68070 (68k with 32 address lines instead of 24)
 & handle the extra features properly (UART,DMA,Timers etc.)

-Proper emulation of the 66470 and/or MCD212 Video Chip (still many unhandled features)

-Inputs;

-Unknown sound chip (it's an ADPCM with eight channels);

-Many unknown memory maps;

*******************************************************************************/


#define CLOCK_A	XTAL_30MHz
#define CLOCK_B	XTAL_19_6608MHz

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "sound/2413intf.h"
#include "devices/chd_cd.h"

static UINT16 *ram;
static UINT16 *ram2;

#define VERBOSE_LEVEL	(5)

INLINE void verboselog(running_machine *machine, int n_level, const char *s_fmt, ...)
{
	if( VERBOSE_LEVEL >= n_level )
	{
		va_list v;
		char buf[ 32768 ];
		va_start( v, s_fmt );
		vsprintf( buf, s_fmt, v );
		va_end( v );
		logerror( "%08x: %s", cpu_get_pc(cputag_get_cpu(machine, "maincpu")), buf );
	}
}

/*************************
*     Video Hardware     *
*************************/

typedef struct
{
	UINT8 csrr;
	UINT16 csrw;
	UINT16 dcr;
	UINT16 vsr;
	UINT16 ddr;
	UINT16 dcp;
	UINT8 clut[256];
	UINT8 image_coding_method;
	UINT8 transparency_control;
	UINT8 plane_order;
	UINT8 clut_bank;
	UINT8 transparent_color_a;
	UINT8 reserved0;
	UINT8 transparent_color_b;
	UINT8 mask_color_a;
	UINT8 reserved1;
	UINT8 mask_color_b;
	UINT8 dyuv_abs_start_a;
	UINT8 dyuv_abs_start_b;
	UINT8 reserved2;
	UINT8 cursor_position;
	UINT8 cursor_control;
	UINT8 cursor_pattern[16];
	UINT8 region_control[8];
	UINT8 backdrop_color;
	UINT8 mosaic_hold_a;
	UINT8 mosaic_hold_b;
	UINT8 weight_factor_a;
	UINT8 weight_factor_b;
} mcd212_channel_t;

typedef struct
{
	mcd212_channel_t channel[2];
	emu_timer *scan_timer;
} mcd212_t;

mcd212_t mcd212;

#define MCD212_DCR_DE			0x8000	// Display Enable
#define MCD212_DCR_CF			0x4000	// Crystal Frequency
#define MCD212_DCR_FD			0x2000	// Frame Duration
#define MCD212_DCR_SM			0x1000	// Scan Mode
#define MCD212_DCR_CM			0x0800	// Color Mode Ch.1/2
#define MCD212_DCR_ICA			0x0200	// ICA Enable Ch.1/2
#define MCD212_DCR_DCA			0x0100	// DCA Enable Ch.1/2

#define MCD212_DDR_FT			0x0300	// Display File Type
#define MCD212_DDR_FT_BMP		0x0000	// Bitmap
#define MCD212_DDR_FT_BMP2		0x0100	// Bitmap (alt.)
#define MCD212_DDR_FT_RLE		0x0200	// Run-Length Encoded
#define MCD212_DDR_FT_MOSAIC	0x0300	// Mosaic

static READ16_HANDLER(mcd212_r)
{
	switch(offset)
	{
		case 0x00/2:
		case 0x10/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "mcd212_r: Status Register %d: %02x & %04x\n", (1 - (offset / 8)) + 1, mcd212.channel[1 - (offset / 8)].csrr, mem_mask);
				mcd212.channel[1 - (offset / 8)].csrr ^= 0x80; // Simulate drawing / not drawing
				return mcd212.channel[1 - (offset / 8)].csrr;
			}
			break;
		case 0x02/2:
		case 0x12/2:
			verboselog(space->machine, 2, "mcd212_r: Display Command Register %d: %04x & %04x\n", (1 - (offset / 8)) + 1, mcd212.channel[1 - (offset / 8)].dcr, mem_mask);
			return mcd212.channel[1 - (offset / 8)].dcr;
			break;
		case 0x04/2:
		case 0x14/2:
			verboselog(space->machine, 2, "mcd212_r: Video Start Register %d: %04x & %04x\n", (1 - (offset / 8)) + 1, mcd212.channel[1 - (offset / 8)].vsr, mem_mask);
			return mcd212.channel[1 - (offset / 8)].vsr;
			break;
		case 0x08/2:
		case 0x18/2:
			verboselog(space->machine, 2, "mcd212_r: Display Decoder Register %d: %04x & %04x\n", (1 - (offset / 8)) + 1, mcd212.channel[1 - (offset / 8)].ddr, mem_mask);
			return mcd212.channel[1 - (offset / 8)].ddr;
			break;
		case 0x0a/2:
		case 0x1a/2:
			verboselog(space->machine, 2, "mcd212_r: DCA Pointer Register %d: %04x & %04x\n", (1 - (offset / 8)) + 1, mcd212.channel[1 - (offset / 8)].dcp, mem_mask);
			return mcd212.channel[1 - (offset / 8)].dcp;
			break;
		default:
			verboselog(space->machine, 2, "mcd212_r: Unknown Register %d & %04x\n", (1 - (offset / 8)) + 1, mem_mask);
			break;
	}

	return 0;
}

static WRITE16_HANDLER(mcd212_w)
{
	switch(offset)
	{
		case 0x00/2:
		case 0x10/2:
			verboselog(space->machine, 2, "mcd212_w: Status Register %d: %02x & %04x\n", (1 - (offset / 8)) + 1, data, mem_mask);
			COMBINE_DATA(&mcd212.channel[1 - (offset / 8)].csrw);
			break;
		case 0x02/2:
		case 0x12/2:
			verboselog(space->machine, 2, "mcd212_w: Display Command Register %d: %04x & %04x\n", (1 - (offset / 8)) + 1, data, mem_mask);
			COMBINE_DATA(&mcd212.channel[1 - (offset / 8)].dcr);
			break;
		case 0x04/2:
		case 0x14/2:
			verboselog(space->machine, 2, "mcd212_w: Video Start Register %d: %04x & %04x\n", (1 - (offset / 8)) + 1, data, mem_mask);
			COMBINE_DATA(&mcd212.channel[1 - (offset / 8)].vsr);
			break;
		case 0x08/2:
		case 0x18/2:
			verboselog(space->machine, 2, "mcd212_w: Display Decoder Register %d: %04x & %04x\n", (1 - (offset / 8)) + 1, data, mem_mask);
			COMBINE_DATA(&mcd212.channel[1 - (offset / 8)].ddr);
			break;
		case 0x0a/2:
		case 0x1a/2:
			verboselog(space->machine, 2, "mcd212_w: DCA Pointer Register %d: %04x & %04x\n", (1 - (offset / 8)) + 1, data, mem_mask);
			COMBINE_DATA(&mcd212.channel[1 - (offset / 8)].dcp);
			break;
		default:
			verboselog(space->machine, 2, "mcd212_w: Unknown Register %d: %04x & %04x\n", (1 - (offset / 8)) + 1, data, mem_mask);
			break;
	}
}

INLINE void mcd212_set_register(running_machine *machine, int channel, UINT8 reg, UINT32 value)
{
	switch(reg)
	{
		case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87: // CLUT 0 - 63
		case 0x88: case 0x89: case 0x8a: case 0x8b: case 0x8c: case 0x8d: case 0x8e: case 0x8f:
		case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97:
		case 0x98: case 0x99: case 0x9a: case 0x9b: case 0x9c: case 0x9d: case 0x9e: case 0x9f:
		case 0xa0: case 0xa1: case 0xa2: case 0xa3: case 0xa4: case 0xa5: case 0xa6: case 0xa7:
		case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad: case 0xae: case 0xaf:
		case 0xb0: case 0xb1: case 0xb2: case 0xb3: case 0xb4: case 0xb5: case 0xb6: case 0xb7:
		case 0xb8: case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbe: case 0xbf:
			verboselog(machine, 6, "          %04xxxxx: CLUT[%d] = %08x\n", channel * 0x20, mcd212.channel[channel].clut_bank * 0x40 + (reg - 0x80), value );
			mcd212.channel[channel].clut[mcd212.channel[channel].clut_bank * 0x40 + (reg - 0x80)] = value;
			break;
		case 0xc0: // Image Coding Method
			if(channel == 0)
			{
				verboselog(machine, 6, "          %04xxxxx: Image Coding Method = %08x\n", channel * 0x20, value );
				mcd212.channel[channel].image_coding_method = value;
			}
			break;
		case 0xc1: // Transparency Control
			if(channel == 0)
			{
				verboselog(machine, 6, "          %04xxxxx: Transparency Control = %08x\n", channel * 0x20, value );
				mcd212.channel[channel].transparency_control = value;
			}
			break;
		case 0xc2: // Plane Order
			if(channel == 0)
			{
				verboselog(machine, 6, "          %04xxxxx: Plane Order = %08x\n", channel * 0x20, value & 7);
				mcd212.channel[channel].plane_order = value & 0x00000007;
			}
			break;
		case 0xc3: // CLUT Bank Register
			verboselog(machine, 6, "          %04xxxxx: CLUT Bank Register = %08x\n", channel * 0x20, value & 3);
			mcd212.channel[channel].clut_bank = value & 0x00000003;
			break;
		case 0xc4: // Transparent Color A
			if(channel == 0)
			{
				verboselog(machine, 6, "          %04xxxxx: Transparent Color A = %08x\n", channel * 0x20, value );
				mcd212.channel[channel].transparent_color_a = value;
			}
			break;
		case 0xc6: // Transparent Color B
			if(channel == 1)
			{
				verboselog(machine, 6, "          %04xxxxx: Transparent Color B = %08x\n", channel * 0x20, value );
				mcd212.channel[channel].transparent_color_b = value;
			}
			break;
		case 0xc7: // Mask Color A
			if(channel == 0)
			{
				verboselog(machine, 6, "          %04xxxxx: Mask Color A = %08x\n", channel * 0x20, value );
				mcd212.channel[channel].mask_color_a = value;
			}
			break;
		case 0xc9: // Mask Color B
			if(channel == 1)
			{
				verboselog(machine, 6, "          %04xxxxx: Mask Color B = %08x\n", channel * 0x20, value );
				mcd212.channel[channel].mask_color_b = value;
			}
			break;
		case 0xca: // Delta YUV Absolute Start Value A
			if(channel == 0)
			{
				verboselog(machine, 6, "          %04xxxxx: Delta YUV Absolute Start Value A = %08x\n", channel * 0x20, value );
				mcd212.channel[channel].dyuv_abs_start_a = value;
			}
			break;
		case 0xcb: // Delta YUV Absolute Start Value B
			if(channel == 1)
			{
				verboselog(machine, 6, "          %04xxxxx: Delta YUV Absolute Start Value B = %08x\n", channel * 0x20, value );
				mcd212.channel[channel].dyuv_abs_start_b = value;
			}
			break;
		case 0xcd: // Cursor Position
			if(channel == 0)
			{
				verboselog(machine, 6, "          %04xxxxx: Cursor Position = %08x\n", channel * 0x20, value );
				mcd212.channel[channel].cursor_position = value;
			}
			break;
		case 0xce: // Cursor Control
			if(channel == 0)
			{
				verboselog(machine, 6, "          %04xxxxx: Cursor Control = %08x\n", channel * 0x20, value );
				mcd212.channel[channel].cursor_control = value;
			}
			break;
		case 0xcf: // Cursor Pattern
			if(channel == 0)
			{
				verboselog(machine, 6, "          %04xxxxx: Cursor Pattern[%d] = %04x\n", channel * 0x20, (value >> 16) & 0x000f, value & 0x0000ffff);
				mcd212.channel[channel].cursor_pattern[(value >> 16) & 0x000f] = value & 0x0000ffff;
			}
			break;
		case 0xd0: // Region Control 0-7
		case 0xd1:
		case 0xd2:
		case 0xd3:
		case 0xd4:
		case 0xd5:
		case 0xd6:
		case 0xd7:
			verboselog(machine, 6, "          %04xxxxx: Region Control %d = %08x\n", channel * 0x20, reg & 7, value );
			mcd212.channel[channel].region_control[reg & 7] = value;
			break;
		case 0xd8: // Backdrop Color
			if(channel == 0)
			{
				verboselog(machine, 6, "          %04xxxxx: Backdrop Color = %08x\n", channel * 0x20, value );
				mcd212.channel[channel].backdrop_color = value;
			}
			break;
		case 0xd9: // Mosaic Pixel Hold Factor A
			if(channel == 0)
			{
				verboselog(machine, 6, "          %04xxxxx: Mosaic Pixel Hold Factor A = %08x\n", channel * 0x20, value );
				mcd212.channel[channel].mosaic_hold_a = value;
			}
			break;
		case 0xda: // Mosaic Pixel Hold Factor B
			if(channel == 1)
			{
				verboselog(machine, 6, "          %04xxxxx: Mosaic Pixel Hold Factor B = %08x\n", channel * 0x20, value );
				mcd212.channel[channel].mosaic_hold_b = value;
			}
			break;
		case 0xdb: // Weight Factor A
			if(channel == 0)
			{
				verboselog(machine, 6, "          %04xxxxx: Weight Factor A = %08x\n", channel * 0x20, value );
				mcd212.channel[channel].weight_factor_a = value;
			}
			break;
		case 0xdc: // Weight Factor B
			if(channel == 1)
			{
				verboselog(machine, 6, "          %04xxxxx: Weight Factor B = %08x\n", channel * 0x20, value );
				mcd212.channel[channel].weight_factor_b = value;
			}
			break;
	}
}

INLINE void mcd212_set_vsr(int channel, UINT32 value)
{
	mcd212.channel[channel].vsr = value & 0x0000ffff;
	mcd212.channel[channel].dcr &= 0xffc0;
	mcd212.channel[channel].dcr |= (value >> 16) & 0x003f;
}

INLINE UINT32 mcd212_get_vsr(int channel)
{
	return ((mcd212.channel[channel].dcr & 0x3f) << 16) | mcd212.channel[channel].vsr;
}

INLINE void mcd212_set_dcp(int channel, UINT32 value)
{
	mcd212.channel[channel].dcp = value & 0x0000ffff;
	mcd212.channel[channel].ddr &= 0xffc0;
	mcd212.channel[channel].ddr |= (value >> 16) & 0x003f;
}

INLINE UINT32 mcd212_get_dcp(int channel)
{
	return ((mcd212.channel[channel].ddr & 0x3f) << 16) | mcd212.channel[channel].dcp;
}

INLINE void mcd212_set_display_parameters(int channel, UINT8 value)
{
	mcd212.channel[channel].ddr &= 0xf0ff;
	mcd212.channel[channel].ddr |= (value & 0x0f) << 8;
	mcd212.channel[channel].dcr &= 0xf7ff;
	mcd212.channel[channel].dcr |= (value & 0x10) << 7;
}

static void mcd212_process_ica(running_machine *machine, int channel)
{
	UINT16 *ica = channel ? ram2 : ram;
	UINT32 addr = 0x000400/2;
	UINT32 cmd = 0;
	while(1)
	{
		UINT8 stop = 0;
		cmd = ica[addr++] << 16;
		cmd |= ica[addr++];
		switch((cmd & 0xff000000) >> 24)
		{
			case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:	// STOP
			case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f:
				verboselog(machine, 6, "%08x: %08x: ICA: STOP\n", addr * 2 + channel * 0x200000, cmd );
				stop = 1;
				break;
			case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17: // NOP
			case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
				verboselog(machine, 6, "%08x: %08x: ICA: NOP\n", addr * 2 + channel * 0x200000, cmd );
				break;
			case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27: // RELOAD DCP
			case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f:
				verboselog(machine, 6, "%08x: %08x: ICA: RELOAD DCP\n", addr * 2 + channel * 0x200000, cmd );
				mcd212_set_dcp(channel, cmd & 0x001fffff);
				break;
			case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37: // RELOAD DCP and STOP
			case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f:
				verboselog(machine, 6, "%08x: %08x: ICA: RELOAD DCP and STOP\n", addr * 2 + channel * 0x200000, cmd );
				mcd212_set_dcp(channel, cmd & 0x001fffff);
				stop = 1;
				break;
			case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47: // RELOAD ICA
			case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f:
				verboselog(machine, 6, "%08x: %08x: ICA: RELOAD ICA\n", addr * 2 + channel * 0x200000, cmd );
				addr = (cmd & 0x001fffff) / 2;
				break;
			case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57: // RELOAD VSR and STOP
			case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5e: case 0x5f:
				verboselog(machine, 6, "%08x: %08x: ICA: RELOAD VSR and STOP\n", addr * 2 + channel * 0x200000, cmd );
				mcd212_set_vsr(channel, cmd & 0x001fffff);
				stop = 1;
				break;
			case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67: // INTERRUPT
			case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c: case 0x6d: case 0x6e: case 0x6f:
				verboselog(machine, 5, "%08x: %08x: ICA: INTERRUPT\n", addr * 2 + channel * 0x200000, cmd );
				mcd212.channel[1].csrr |= 1 << (2 - channel);
				// TODO
				// mcd212_check_interrupts
				break;
			case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77: // RELOAD DISPLAY PARAMETERS
			case 0x78: case 0x79: case 0x7a: case 0x7b: case 0x7c: case 0x7d: case 0x7e: case 0x7f:
				verboselog(machine, 6, "%08x: %08x: ICA: RELOAD DISPLAY PARAMETERS\n", addr * 2 + channel * 0x200000, cmd );
				mcd212_set_display_parameters(channel, cmd & 0x1f);
				break;
			default:
				mcd212_set_register(machine, channel, cmd >> 24, cmd & 0x00ffffff);
				break;
		}
		if(stop)
		{
			break;
		}
	}
}

static void mcd212_process_dca(running_machine *machine, int channel)
{
	UINT16 *dca = channel ? ram2 : ram;
	UINT32 addr = mcd212_get_dcp(channel) / 2;
	UINT32 cmd = 0;
	while(1)
	{
		UINT8 stop = 0;
		cmd = dca[addr++] << 16;
		cmd |= dca[addr++];
		switch((cmd & 0xff000000) >> 24)
		{
			case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:	// STOP
			case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f:
				verboselog(machine, 6, "%08x: %08x: DCA: STOP\n", addr * 2 + channel * 0x200000, cmd );
				stop = 1;
				break;
			case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17: // NOP
			case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
				verboselog(machine, 6, "%08x: %08x: DCA: NOP\n", addr * 2 + channel * 0x200000, cmd );
				break;
			case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27: // RELOAD DCP
			case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f:
				verboselog(machine, 6, "%08x: %08x: DCA: RELOAD DCP (NOP)\n", addr * 2 + channel * 0x200000, cmd );
				break;
			case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37: // RELOAD DCP and STOP
			case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f:
				verboselog(machine, 6, "%08x: %08x: DCA: RELOAD DCP and STOP\n", addr * 2 + channel * 0x200000, cmd );
				mcd212_set_dcp(channel, cmd & 0x001fffff);
				stop = 1;
				break;
			case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47: // RELOAD VSR
			case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f:
				verboselog(machine, 6, "%08x: %08x: DCA: RELOAD VSR\n", addr * 2 + channel * 0x200000, cmd );
				mcd212_set_vsr(channel, cmd & 0x001fffff);
				break;
			case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57: // RELOAD VSR and STOP
			case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5e: case 0x5f:
				verboselog(machine, 6, "%08x: %08x: DCA: RELOAD VSR and STOP\n", addr * 2 + channel * 0x200000, cmd );
				mcd212_set_vsr(channel, cmd & 0x001fffff);
				stop = 1;
				break;
			case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67: // INTERRUPT
			case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c: case 0x6d: case 0x6e: case 0x6f:
				verboselog(machine, 5, "%08x: %08x: DCA: INTERRUPT\n", addr * 2 + channel * 0x200000, cmd );
				mcd212.channel[1].csrr |= 1 << (2 - channel);
				// TODO
				// mcd212_check_interrupts
				break;
			case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77: // RELOAD DISPLAY PARAMETERS
			case 0x78: case 0x79: case 0x7a: case 0x7b: case 0x7c: case 0x7d: case 0x7e: case 0x7f:
				verboselog(machine, 6, "%08x: %08x: DCA: RELOAD DISPLAY PARAMETERS\n", addr * 2 + channel * 0x200000, cmd );
				mcd212_set_display_parameters(channel, cmd & 0x1f);
				break;
			default:
				mcd212_set_register(machine, channel, cmd >> 24, cmd & 0x00ffffff);
				break;
		}
		if(stop)
		{
			break;
		}
	}
}

static void mcd212_process_vsr(running_machine *machine, int channel, UINT32 *pixels)
{
	UINT8 *data = channel ? (UINT8*)ram2 : (UINT8*)ram;
	UINT32 vsr = mcd212_get_vsr(channel);
	UINT8 done = 0;
	int x = 0;

	while(!done)
	{
		UINT8 byte = data[vsr++ ^ 3];
		switch(mcd212.channel[channel].ddr & MCD212_DDR_FT)
		{
			case MCD212_DDR_FT_BMP:
			case MCD212_DDR_FT_BMP2:
				verboselog(machine, 0, "Unsupported display mode: Bitmap\n" );
				done = 1;
				break;
			case MCD212_DDR_FT_RLE:
				if(mcd212.channel[channel].dcr & MCD212_DCR_CM)
				{
					// 4-bit RLE
					verboselog(machine, 0, "Unsupported display mode: 4-bit RLE\n" );
					done = 1;
				}
				else
				{
					// 8-bit RLE
					if(byte & 0x80)
					{
						// Run length
						UINT8 length = data[vsr++ ^ 3];
						if(!length)
						{
							// Go to the end of the line
							for(; x < 640; x++)
							{
								fflush(stdout);
								pixels[x] = mcd212.channel[0].clut[byte & 0x7f];
							}
							done = 1;
						}
						else
						{
							int end = x + length;
							for(; x < end; x++)
							{
								fflush(stdout);
								pixels[x] = mcd212.channel[0].clut[byte & 0x7f];
							}
						}
					}
					else
					{
						fflush(stdout);
						// Single pixel
						pixels[x++] = mcd212.channel[0].clut[byte & 0x7f];
					}
				}
				break;
			case MCD212_DDR_FT_MOSAIC:
				verboselog(machine, 0, "Unsupported display mode: Mosaic\n" );
				done = 1;
				break;
		}
	}
}

static void mcd212_draw_scanline(running_machine *machine, int y)
{
	bitmap_t *bitmap = tmpbitmap;
	UINT32 plane_a[768];
	//UINT32 plane_b[768];
	UINT32 *scanline = BITMAP_ADDR32(bitmap, y, 0);
	int x;
	mcd212_process_vsr(machine, 0, plane_a);
	//mcd212_process_vsr(machine, 1, plane_b);
	for(x = 0; x < 768; x++)
	{
		scanline[x] = plane_a[x];
	}
}

TIMER_CALLBACK( mcd212_perform_scan )
{
	int scanline = video_screen_get_vpos(machine->primary_screen);
	if(mcd212.channel[0].dcr & MCD212_DCR_DE)
	{
		if(scanline == 0)
		{
			// Process ICA
			int index = 0;
			for(index = 0; index < 2; index++)
			{
				if(mcd212.channel[index].dcr & MCD212_DCR_ICA)
				{
					mcd212_process_ica(machine, index);
				}
			}
		}
		else if(scanline >= 22)
		{
			int index = 0;
			// Process VSR
			mcd212_draw_scanline(machine, scanline);
			// Process DCA
			for(index = 0; index < 2; index++)
			{
				if(mcd212.channel[index].dcr & MCD212_DCR_DCA)
				{
					mcd212_process_dca(machine, index);
				}
			}
		}
	}
	timer_adjust_oneshot(mcd212.scan_timer, video_screen_get_time_until_pos(machine->primary_screen, ( scanline + 1 ) % 262, 0), 0);
}

static VIDEO_START(cdi)
{
	VIDEO_START_CALL(generic_bitmapped);
	mcd212.channel[0].csrr = 0x00;
	mcd212.channel[1].csrr = 0x00;
	mcd212.channel[0].clut_bank = 0;
	mcd212.channel[1].clut_bank = 0;
	mcd212.scan_timer = timer_alloc(machine, mcd212_perform_scan, 0);
	timer_adjust_oneshot(mcd212.scan_timer, video_screen_get_time_until_pos(machine->primary_screen, 0, 0), 0);
}

/*************************
* Memory map information *
*************************/

typedef struct
{
	UINT8 reserved0;
	UINT8 data_register;
	UINT8 reserved1;
	UINT8 address_register;
	UINT8 reserved2;
	UINT8 status_register;
	UINT8 reserved3;
	UINT8 control_register;
	UINT8 reserved;
	UINT8 clock_control_register;
} scc68070_i2c_regs_t;

#define ISR_MST		0x80	// Master
#define ISR_TRX		0x40	// Transmitter
#define ISR_BB		0x20	// Busy
#define ISR_PIN		0x10	// No Pending Interrupt
#define ISR_AL		0x08	// Arbitration Lost
#define ISR_AAS		0x04	// Addressed As Slave
#define ISR_AD0		0x02	// Address Zero
#define ISR_LRB		0x01	// Last Received Bit

typedef struct
{
	UINT8 reserved0;
	UINT8 mode_register;
	UINT8 reserved1;
	UINT8 status_register;
	UINT8 reserved2;
	UINT8 clock_select;
	UINT8 reserved3;
	UINT8 command_register;
	UINT8 reserved4;
	UINT8 transmit_holding_register;
	UINT8 reserved5;
	UINT8 receive_holding_register;
} scc68070_uart_regs_t;

#define UMR_OM			0xc0
#define UMR_OM_NORMAL	0x00
#define UMR_OM_ECHO		0x40
#define UMR_OM_LOOPBACK	0x80
#define UMR_OM_RLOOP	0xc0
#define UMR_TXC			0x10
#define UMR_PC			0x08
#define UMR_P			0x04
#define UMR_SB			0x02
#define UMR_CL			0x01

#define USR_RB			0x80
#define USR_FE			0x40
#define USR_PE			0x20
#define USR_OE			0x10
#define USR_TXEMT		0x08
#define USR_TXRDY		0x04
#define USR_RXRDY		0x01

typedef struct
{
	UINT8 timer_status_register;
	UINT8 timer_control_register;
	UINT16 reload_register;
	UINT16 timer0;
	UINT16 timer1;
	UINT16 timer2;
	emu_timer* timer0_timer;
} scc68070_timer_regs_t;

#define TSR_OV0			0x80
#define TSR_MA1			0x40
#define TSR_CAP1		0x20
#define TSR_OV1			0x10
#define TSR_MA2			0x08
#define TSR_CAP2		0x04
#define TSR_OV2			0x02

#define TCR_E1			0xc0
#define TCR_E1_NONE		0x00
#define TCR_E1_RISING	0x40
#define TCR_E1_FALLING	0x80
#define TCR_E1_BOTH		0xc0
#define TCR_M1			0x30
#define TCR_M1_NONE		0x00
#define TCR_M1_MATCH	0x10
#define TCR_M1_CAPTURE	0x20
#define TCR_M1_COUNT	0x30
#define TCR_E2			0x0c
#define TCR_E2_NONE		0x00
#define TCR_E2_RISING	0x04
#define TCR_E2_FALLING	0x08
#define TCR_E2_BOTH		0x0c
#define TCR_M2			0x03
#define TCR_M2_NONE		0x00
#define TCR_M2_MATCH	0x01
#define TCR_M2_CAPTURE	0x02
#define TCR_M2_COUNT	0x03

typedef struct
{
	UINT8 channel_status;
	UINT8 channel_error;

	UINT8 reserved0[2];

	UINT8 device_control;
	UINT8 operation_control;
	UINT8 sequence_control;
	UINT8 channel_control;

	UINT8 reserved1[3];

	UINT8 transfer_counter;

	UINT32 memory_address_counter;

	UINT8 reserved2[4];

	UINT32 device_address_counter;

	UINT8 reserved3[40];
} scc68070_dma_channel_t;

#define CSR_COC			0x80
#define CSR_NDT			0x20
#define CSR_ERR			0x10
#define CSR_CA			0x08

#define CER_EC			0x1f
#define CER_NONE		0x00
#define CER_TIMING		0x02
#define CER_BUSERR_MEM	0x09
#define CER_BUSERR_DEV	0x0a
#define CER_SOFT_ABORT	0x11

#define DCR1_ERM		0x80
#define DCR1_DT			0x30

#define DCR2_ERM		0x80
#define DCR2_DT			0x30
#define DCR2_DS			0x08

#define OCR_D			0x80
#define OCR_D_M2D		0x00
#define OCR_D_D2M		0x80
#define OCR_OS			0x30
#define OCR_OS_BYTE		0x00
#define OCR_OS_WORD		0x10

#define SCR2_MAC		0x0c
#define SCR2_MAC_NONE	0x00
#define SCR2_MAC_INC	0x04
#define SCR2_DAC		0x03
#define SCR2_DAC_NONE	0x00
#define SCR2_DAC_INC	0x01

#define CCR_SO			0x80
#define CCR_SA			0x10
#define CCR_INE			0x08
#define CCR_IPL			0x07

typedef struct
{
	scc68070_dma_channel_t channel[2];
} scc68070_dma_regs_t;

typedef struct
{
	UINT16 attr;
	UINT16 length;
	UINT8  undefined;
	UINT8  segment;
	UINT16 base;
} scc68070_mmu_desc_t;

typedef struct
{
	UINT8 status;
	UINT8 control;

	UINT8 reserved[0x3e];

	scc68070_mmu_desc_t desc[8];
} scc68070_mmu_regs_t;

typedef struct
{
	UINT16 lir;
	scc68070_i2c_regs_t i2c;
	scc68070_uart_regs_t uart;
	scc68070_timer_regs_t timers;
	UINT8 picr1;
	UINT8 picr2;
	scc68070_dma_regs_t dma;
	scc68070_mmu_regs_t mmu;
} scc68070_regs_t;

scc68070_regs_t scc68070_regs;

static void scc68070_set_timer_callback(int channel)
{
	UINT32 compare = 0;
	attotime period;
	switch(channel)
	{
		case 0:
			compare = 0x10000 - scc68070_regs.timers.timer0;
			period = attotime_mul(ATTOTIME_IN_HZ(1000000), compare);
            timer_adjust_oneshot(scc68070_regs.timers.timer0_timer, period, 0);
			break;
		default:
			fatalerror( "Unsupported timer channel to scc68070_set_timer_callback!\n" );
	}
}

TIMER_CALLBACK( scc68070_timer0_callback )
{
	scc68070_regs.timers.timer0 = scc68070_regs.timers.reload_register;
	if(scc68070_regs.picr1 & 7)
	{
		scc68070_regs.timers.timer_status_register |= TSR_OV0;
		//printf( "Pulling line %d\n", 1 + ((scc68070_regs.picr1 & 7) - 1) );
		cputag_set_input_line(machine, "maincpu", M68K_IRQ_1 + ((scc68070_regs.picr1 & 7) - 1), ASSERT_LINE);
	}
	scc68070_set_timer_callback(0);
}

static READ16_HANDLER( scc68070_periphs_r )
{
	switch(offset)
	{
		// Interupts: 80001001
		case 0x1000/2: // LIR priority level
			return scc68070_regs.lir;

		// I2C interface: 80002001 to 80002009
		case 0x2000/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: I2C Data Register: %04x & %04x\n", scc68070_regs.i2c.data_register, mem_mask);
			}
			return scc68070_regs.i2c.data_register;
		case 0x2002/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: I2C Address Register: %04x & %04x\n", scc68070_regs.i2c.address_register, mem_mask);
			}
			return scc68070_regs.i2c.address_register;
		case 0x2004/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: I2C Status Register: %04x & %04x\n", scc68070_regs.i2c.status_register, mem_mask);
			}
			return scc68070_regs.i2c.status_register;
		case 0x2006/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: I2C Control Register: %04x & %04x\n", scc68070_regs.i2c.control_register, mem_mask);
			}
			return scc68070_regs.i2c.control_register;
		case 0x2008/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: I2C Clock Control Register: %04x & %04x\n", scc68070_regs.i2c.clock_control_register, mem_mask);
			}
			return scc68070_regs.i2c.clock_control_register;

		// UART interface: 80002011 to 8000201b
		case 0x2010/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: UART Mode Register: %04x & %04x\n", scc68070_regs.uart.mode_register, mem_mask);
			}
			return scc68070_regs.uart.mode_register;
		case 0x2012/2:
			if(ACCESSING_BITS_0_7)
			{
//				verboselog(space->machine, 2, "scc68070_periphs_r: UART Status Register: %04x & %04x\n", scc68070_regs.uart.status_register, mem_mask);
			}
			return scc68070_regs.uart.status_register | USR_TXRDY | USR_RXRDY;
		case 0x2014/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: UART Clock Select: %04x & %04x\n", scc68070_regs.uart.clock_select, mem_mask);
			}
			return scc68070_regs.uart.clock_select;
		case 0x2016/2:
			if(ACCESSING_BITS_0_7)
			{
//				verboselog(space->machine, 2, "scc68070_periphs_r: UART Command Register: %02x & %04x\n", scc68070_regs.uart.command_register, mem_mask);
			}
			return scc68070_regs.uart.command_register;
		case 0x2018/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: UART Transmit Holding Register: %02x & %04x\n", scc68070_regs.uart.transmit_holding_register, mem_mask);
			}
			return scc68070_regs.uart.transmit_holding_register;
		case 0x201a/2:
			if(ACCESSING_BITS_0_7)
			{
//				verboselog(space->machine, 2, "scc68070_periphs_r: UART Receive Holding Register: %02x & %04x\n", scc68070_regs.uart.receive_holding_register, mem_mask);
			}
			return scc68070_regs.uart.receive_holding_register;

		// Timers: 80002020 to 80002029
		case 0x2020/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: Timer Status Register: %02x & %04x\n", scc68070_regs.timers.timer_status_register, mem_mask);
			}
			if(ACCESSING_BITS_8_15)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: Timer Control Register: %02x & %04x\n", scc68070_regs.timers.timer_control_register, mem_mask);
			}
			return (scc68070_regs.timers.timer_control_register << 8) | scc68070_regs.timers.timer_status_register;
			break;
		case 0x2022/2:
			verboselog(space->machine, 2, "scc68070_periphs_r: Timer Reload Register: %04x & %04x\n", scc68070_regs.timers.reload_register, mem_mask);
			return scc68070_regs.timers.reload_register;
		case 0x2024/2:
			verboselog(space->machine, 2, "scc68070_periphs_r: Timer 0: %04x & %04x\n", scc68070_regs.timers.timer0, mem_mask);
			return scc68070_regs.timers.timer0;
		case 0x2026/2:
			verboselog(space->machine, 2, "scc68070_periphs_r: Timer 1: %04x & %04x\n", scc68070_regs.timers.timer1, mem_mask);
			return scc68070_regs.timers.timer1;
		case 0x2028/2:
			verboselog(space->machine, 2, "scc68070_periphs_r: Timer 2: %04x & %04x\n", scc68070_regs.timers.timer2, mem_mask);
			return scc68070_regs.timers.timer2;

		// PICR1: 80002045
		case 0x2044/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: Peripheral Interrupt Control Register 1: %02x & %04x\n", scc68070_regs.picr1, mem_mask);
			}
			return scc68070_regs.picr1;

		// PICR2: 80002047
		case 0x2046/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: Peripheral Interrupt Control Register 2: %02x & %04x\n", scc68070_regs.picr2, mem_mask);
			}
			return scc68070_regs.picr2;

		// DMA controller: 80004000 to 8000406d
		case 0x4000/2:
		case 0x4040/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: DMA(%d) Error Register: %04x & %04x\n", (offset - 0x2000) / 32, scc68070_regs.dma.channel[(offset - 0x2000) / 32].channel_error, mem_mask);
			}
			if(ACCESSING_BITS_8_15)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: DMA(%d) Status Register: %04x & %04x\n", (offset - 0x2000) / 32, scc68070_regs.dma.channel[(offset - 0x2000) / 32].channel_status, mem_mask);
			}
			return (scc68070_regs.dma.channel[(offset - 0x2000) / 32].channel_status << 8) | scc68070_regs.dma.channel[(offset - 0x2000) / 32].channel_error;
			break;
		case 0x4004/2:
		case 0x4044/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: DMA(%d) Operation Control Register: %02x & %04x\n", (offset - 0x2000) / 32, scc68070_regs.dma.channel[(offset - 0x2000) / 32].operation_control, mem_mask);
			}
			if(ACCESSING_BITS_8_15)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: DMA(%d) Device Control Register: %02x & %04x\n", (offset - 0x2000) / 32, scc68070_regs.dma.channel[(offset - 0x2000) / 32].device_control, mem_mask);
			}
			return (scc68070_regs.dma.channel[(offset - 0x2000) / 32].device_control << 8) | scc68070_regs.dma.channel[(offset - 0x2000) / 32].operation_control;
		case 0x4006/2:
		case 0x4046/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: DMA(%d) Channel Control Register: %02x & %04x\n", (offset - 0x2000) / 32, scc68070_regs.dma.channel[(offset - 0x2000) / 32].channel_control, mem_mask);
			}
			if(ACCESSING_BITS_8_15)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: DMA(%d) Sequence Control Register: %02x & %04x\n", (offset - 0x2000) / 32, scc68070_regs.dma.channel[(offset - 0x2000) / 32].sequence_control, mem_mask);
			}
			return (scc68070_regs.dma.channel[(offset - 0x2000) / 32].sequence_control << 8) | scc68070_regs.dma.channel[(offset - 0x2000) / 32].channel_control;
		case 0x400a/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: DMA(%d) Memory Transfer Counter Low: %02x & %04x\n", (offset - 0x2000) / 32, scc68070_regs.dma.channel[(offset - 0x2000) / 32].transfer_counter, mem_mask);
			}
			if(ACCESSING_BITS_8_15)
			{
				verboselog(space->machine, 0, "scc68070_periphs_r: DMA(%d) Memory Transfer Counter High (invalid): %04x\n", (offset - 0x2000) / 32, mem_mask);
			}
			return scc68070_regs.dma.channel[(offset - 0x2000) / 32].transfer_counter;
		case 0x400c/2:
		case 0x404c/2:
			verboselog(space->machine, 2, "scc68070_periphs_r: DMA(%d) Memory Address Counter (High Word): %04x & %04x\n", (offset - 0x2000) / 32, (scc68070_regs.dma.channel[(offset - 0x2000) / 32].memory_address_counter >> 16), mem_mask);
			return (scc68070_regs.dma.channel[(offset - 0x2000) / 32].memory_address_counter >> 16);
		case 0x400e/2:
		case 0x404e/2:
			verboselog(space->machine, 2, "scc68070_periphs_r: DMA(%d) Memory Address Counter (Low Word): %04x & %04x\n", (offset - 0x2000) / 32, scc68070_regs.dma.channel[(offset - 0x2000) / 32].device_address_counter, mem_mask);
			return scc68070_regs.dma.channel[(offset - 0x2000) / 32].memory_address_counter;
		case 0x4014/2:
		case 0x4054/2:
			verboselog(space->machine, 2, "scc68070_periphs_r: DMA(%d) Device Address Counter (High Word): %04x & %04x\n", (offset - 0x2000) / 32, (scc68070_regs.dma.channel[(offset - 0x2000) / 32].device_address_counter >> 16), mem_mask);
			return (scc68070_regs.dma.channel[(offset - 0x2000) / 32].device_address_counter >> 16);
		case 0x4016/2:
		case 0x4056/2:
			verboselog(space->machine, 2, "scc68070_periphs_r: DMA(%d) Device Address Counter (Low Word): %04x & %04x\n", (offset - 0x2000) / 32, scc68070_regs.dma.channel[(offset - 0x2000) / 32].device_address_counter, mem_mask);
			return scc68070_regs.dma.channel[(offset - 0x2000) / 32].device_address_counter;

		// MMU: 80008000 to 8000807f
		case 0x8000/2:	// Status / Control register
			if(ACCESSING_BITS_0_7)
			{	// Control
				verboselog(space->machine, 2, "scc68070_periphs_r: MMU Control: %02x & %04x\n", scc68070_regs.mmu.control, mem_mask);
				return scc68070_regs.mmu.control;
			}	// Status
			else
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: MMU Status: %02x & %04x\n", scc68070_regs.mmu.status, mem_mask);
				return scc68070_regs.mmu.status;
			}
			break;
		case 0x8040/2:
		case 0x8048/2:
		case 0x8050/2:
		case 0x8058/2:
		case 0x8060/2:
		case 0x8068/2:
		case 0x8070/2:
		case 0x8078/2:	// Attributes (SD0-7)
			verboselog(space->machine, 2, "scc68070_periphs_r: MMU descriptor %d attributes: %04x & %04x\n", (offset - 0x4020) / 4, scc68070_regs.mmu.desc[(offset - 0x4020) / 4].attr, mem_mask);
			return scc68070_regs.mmu.desc[(offset - 0x4020) / 4].attr;
		case 0x8042/2:
		case 0x804a/2:
		case 0x8052/2:
		case 0x805a/2:
		case 0x8062/2:
		case 0x806a/2:
		case 0x8072/2:
		case 0x807a/2:	// Segment Length (SD0-7)
			verboselog(space->machine, 2, "scc68070_periphs_r: MMU descriptor %d length: %04x & %04x\n", (offset - 0x4020) / 4, scc68070_regs.mmu.desc[(offset - 0x4020) / 4].length, mem_mask);
			return scc68070_regs.mmu.desc[(offset - 0x4020) / 4].length;
		case 0x8044/2:
		case 0x804c/2:
		case 0x8054/2:
		case 0x805c/2:
		case 0x8064/2:
		case 0x806c/2:
		case 0x8074/2:
		case 0x807c/2:	// Segment Number (SD0-7, A0=1 only)
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: MMU descriptor %d segment: %02x & %04x\n", (offset - 0x4020) / 4, scc68070_regs.mmu.desc[(offset - 0x4020) / 4].segment, mem_mask);
				return scc68070_regs.mmu.desc[(offset - 0x4020) / 4].segment;
			}
			break;
		case 0x8046/2:
		case 0x804e/2:
		case 0x8056/2:
		case 0x805e/2:
		case 0x8066/2:
		case 0x806e/2:
		case 0x8076/2:
		case 0x807e/2:	// Base Address (SD0-7)
			verboselog(space->machine, 2, "scc68070_periphs_r: MMU descriptor %d base: %04x & %04x\n", (offset - 0x4020) / 4, scc68070_regs.mmu.desc[(offset - 0x4020) / 4].base, mem_mask);
			return scc68070_regs.mmu.desc[(offset - 0x4020) / 4].base;
	}

	return 0;
}

static WRITE16_HANDLER( scc68070_periphs_w )
{
	switch(offset)
	{
		// Interupts: 80001001
		case 0x1000/2: // LIR priority level
			COMBINE_DATA(&scc68070_regs.lir);
			break;

		// I2C interface: 80002001 to 80002009
		case 0x2000/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: I2C Data Register: %04x & %04x\n", data, mem_mask);
				scc68070_regs.i2c.data_register = data & 0x00ff;
			}
			break;
		case 0x2002/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: I2C Address Register: %04x & %04x\n", data, mem_mask);
				scc68070_regs.i2c.address_register = data & 0x00ff;
			}
			break;
		case 0x2004/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: I2C Status Register: %04x & %04x\n", data, mem_mask);
				scc68070_regs.i2c.status_register = data & 0x00ff;
			}
			break;
		case 0x2006/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: I2C Control Register: %04x & %04x\n", data, mem_mask);
				scc68070_regs.i2c.control_register = data & 0x00ff;
			}
			break;
		case 0x2008/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: I2C Clock Control Register: %04x & %04x\n", data, mem_mask);
				scc68070_regs.i2c.clock_control_register = data & 0x00ff;
			}
			break;

		// UART interface: 80002011 to 8000201b
		case 0x2010/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: UART Mode Register: %04x & %04x\n", data, mem_mask);
				scc68070_regs.uart.mode_register = data & 0x00ff;
			}
			break;
		case 0x2012/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: UART Status Register: %04x & %04x\n", data, mem_mask);
				scc68070_regs.uart.status_register = data & 0x00ff;
			}
			break;
		case 0x2014/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: UART Clock Select: %04x & %04x\n", data, mem_mask);
				scc68070_regs.uart.clock_select = data & 0x00ff;
			}
			break;
		case 0x2016/2:
			if(ACCESSING_BITS_0_7)
			{
//				verboselog(space->machine, 2, "scc68070_periphs_w: UART Command Register: %04x & %04x\n", data, mem_mask);
				scc68070_regs.uart.command_register = data & 0x00ff;
			}
			break;
		case 0x2018/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: UART Transmit Holding Register: %04x & %04x\n", data, mem_mask);
				if(data >= 0x20 && data < 0x7f)
				{
					printf( "%c", data & 0x00ff );
				}
				scc68070_regs.uart.transmit_holding_register = data & 0x00ff;
			}
			break;
		case 0x201a/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: UART Receive Holding Register: %04x & %04x\n", data, mem_mask);
				scc68070_regs.uart.receive_holding_register = data & 0x00ff;
			}
			break;

		// Timers: 80002020 to 80002029
		case 0x2020/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: Timer Status Register: %04x & %04x\n", data, mem_mask);
				scc68070_regs.timers.timer_status_register &= ~(data & 0x00ff);
				if(scc68070_regs.picr1 & 7)
				{
					cputag_set_input_line(space->machine, "maincpu", M68K_IRQ_1 + ((scc68070_regs.picr1 & 7) - 1), CLEAR_LINE);
				}
			}
			if(ACCESSING_BITS_8_15)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: Timer Control Register: %04x & %04x\n", data, mem_mask);
				scc68070_regs.timers.timer_control_register = data >> 8;
			}
			break;
		case 0x2022/2:
			verboselog(space->machine, 2, "scc68070_periphs_w: Timer Reload Register: %04x & %04x\n", data, mem_mask);
			COMBINE_DATA(&scc68070_regs.timers.reload_register);
			break;
		case 0x2024/2:
			verboselog(space->machine, 2, "scc68070_periphs_w: Timer 0: %04x & %04x\n", data, mem_mask);
			COMBINE_DATA(&scc68070_regs.timers.timer0);
			scc68070_set_timer_callback(0);
			break;
		case 0x2026/2:
			verboselog(space->machine, 2, "scc68070_periphs_w: Timer 1: %04x & %04x\n", data, mem_mask);
			COMBINE_DATA(&scc68070_regs.timers.timer1);
			break;
		case 0x2028/2:
			verboselog(space->machine, 2, "scc68070_periphs_w: Timer 2: %04x & %04x\n", data, mem_mask);
			COMBINE_DATA(&scc68070_regs.timers.timer2);
			break;

		// PICR1: 80002045
		case 0x2044/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: Peripheral Interrupt Control Register 1: %04x & %04x\n", data, mem_mask);
				scc68070_regs.picr1 = data & 0x00ff;
			}
			break;

		// PICR2: 80002047
		case 0x2046/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: Peripheral Interrupt Control Register 2: %04x & %04x\n", data, mem_mask);
				scc68070_regs.picr2 = data & 0x00ff;
			}
			break;

		// DMA controller: 80004000 to 8000406d
		case 0x4000/2:
		case 0x4040/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: DMA(%d) Error (invalid): %04x & %04x\n", (offset - 0x2000) / 32, data, mem_mask);
			}
			if(ACCESSING_BITS_8_15)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: DMA(%d) Status: %04x & %04x\n", (offset - 0x2000) / 32, data, mem_mask);
				scc68070_regs.dma.channel[(offset - 0x2000) / 32].channel_status = data >> 8;
			}
			break;
		case 0x4004/2:
		case 0x4044/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: DMA(%d) Operation Control Register: %04x & %04x\n", (offset - 0x2000) / 32, data, mem_mask);
				scc68070_regs.dma.channel[(offset - 0x2000) / 32].operation_control = data >> 8;
			}
			if(ACCESSING_BITS_8_15)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: DMA(%d) Device Control Register: %04x & %04x\n", (offset - 0x2000) / 32, data, mem_mask);
				scc68070_regs.dma.channel[(offset - 0x2000) / 32].device_control = data >> 8;
			}
			break;
		case 0x4006/2:
		case 0x4046/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: DMA(%d) Channel Control Register: %04x & %04x\n", (offset - 0x2000) / 32, data, mem_mask);
				scc68070_regs.dma.channel[(offset - 0x2000) / 32].channel_control = data >> 8;
			}
			if(ACCESSING_BITS_8_15)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: DMA(%d) Sequence Control Register: %04x & %04x\n", (offset - 0x2000) / 32, data, mem_mask);
				scc68070_regs.dma.channel[(offset - 0x2000) / 32].sequence_control = data >> 8;
			}
			break;
		case 0x400a/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: DMA(%d) Memory Transfer Counter Low: %04x & %04x\n", (offset - 0x2000) / 32, data, mem_mask);
				scc68070_regs.dma.channel[(offset - 0x2000) / 32].transfer_counter = data >> 8;
			}
			if(ACCESSING_BITS_8_15)
			{
				verboselog(space->machine, 0, "scc68070_periphs_w: DMA(%d) Memory Transfer Counter High (invalid): %04x & %04x\n", (offset - 0x2000) / 32, data, mem_mask);
			}
			break;
		case 0x400c/2:
		case 0x404c/2:
			verboselog(space->machine, 2, "scc68070_periphs_w: DMA(%d) Memory Address Counter (High Word): %04x & %04x\n", (offset - 0x2000) / 32, data, mem_mask);
			scc68070_regs.dma.channel[(offset - 0x2000) / 32].memory_address_counter &= mem_mask << 16;
			scc68070_regs.dma.channel[(offset - 0x2000) / 32].memory_address_counter |= data << 16;
			break;
		case 0x400e/2:
		case 0x404e/2:
			verboselog(space->machine, 2, "scc68070_periphs_w: DMA(%d) Memory Address Counter (Low Word): %04x & %04x\n", (offset - 0x2000) / 32, data, mem_mask);
			scc68070_regs.dma.channel[(offset - 0x2000) / 32].memory_address_counter &= (0xffff0000) | mem_mask;
			scc68070_regs.dma.channel[(offset - 0x2000) / 32].memory_address_counter |= data;
			break;
		case 0x4014/2:
		case 0x4054/2:
			verboselog(space->machine, 2, "scc68070_periphs_w: DMA(%d) Device Address Counter (High Word): %04x & %04x\n", (offset - 0x2000) / 32, data, mem_mask);
			scc68070_regs.dma.channel[(offset - 0x2000) / 32].device_address_counter &= mem_mask << 16;
			scc68070_regs.dma.channel[(offset - 0x2000) / 32].device_address_counter |= data << 16;
			break;
		case 0x4016/2:
		case 0x4056/2:
			verboselog(space->machine, 2, "scc68070_periphs_w: DMA(%d) Device Address Counter (Low Word): %04x & %04x\n", (offset - 0x2000) / 32, data, mem_mask);
			scc68070_regs.dma.channel[(offset - 0x2000) / 32].device_address_counter &= (0xffff0000) | mem_mask;
			scc68070_regs.dma.channel[(offset - 0x2000) / 32].device_address_counter |= data;
			break;

		// MMU: 80008000 to 8000807f
		case 0x8000/2:	// Status / Control register
			if(ACCESSING_BITS_0_7)
			{	// Control
				verboselog(space->machine, 2, "scc68070_periphs_w: MMU Control: %04x & %04x\n", data, mem_mask);
				scc68070_regs.mmu.control = data & 0x00ff;
			}	// Status
			else
			{
				verboselog(space->machine, 0, "scc68070_periphs_w: MMU Status (invalid): %04x & %04x\n", data, mem_mask);
			}
			break;
		case 0x8040/2:
		case 0x8048/2:
		case 0x8050/2:
		case 0x8058/2:
		case 0x8060/2:
		case 0x8068/2:
		case 0x8070/2:
		case 0x8078/2:	// Attributes (SD0-7)
			verboselog(space->machine, 2, "scc68070_periphs_w: MMU descriptor %d attributes: %04x & %04x\n", (offset - 0x4020) / 4, data, mem_mask);
			COMBINE_DATA(&scc68070_regs.mmu.desc[(offset - 0x4020) / 4].attr);
			break;
		case 0x8042/2:
		case 0x804a/2:
		case 0x8052/2:
		case 0x805a/2:
		case 0x8062/2:
		case 0x806a/2:
		case 0x8072/2:
		case 0x807a/2:	// Segment Length (SD0-7)
			verboselog(space->machine, 2, "scc68070_periphs_w: MMU descriptor %d length: %04x & %04x\n", (offset - 0x4020) / 4, data, mem_mask);
			COMBINE_DATA(&scc68070_regs.mmu.desc[(offset - 0x4020) / 4].length);
			break;
		case 0x8044/2:
		case 0x804c/2:
		case 0x8054/2:
		case 0x805c/2:
		case 0x8064/2:
		case 0x806c/2:
		case 0x8074/2:
		case 0x807c/2:	// Segment Number (SD0-7, A0=1 only)
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: MMU descriptor %d segment: %04x & %04x\n", (offset - 0x4020) / 4, data, mem_mask);
				scc68070_regs.mmu.desc[(offset - 0x4020) / 4].segment = data & 0x00ff;
			}
			break;
		case 0x8046/2:
		case 0x804e/2:
		case 0x8056/2:
		case 0x805e/2:
		case 0x8066/2:
		case 0x806e/2:
		case 0x8076/2:
		case 0x807e/2:	// Base Address (SD0-7)
			verboselog(space->machine, 2, "scc68070_periphs_w: MMU descriptor %d base: %04x & %04x\n", (offset - 0x4020) / 4, data, mem_mask);
			COMBINE_DATA(&scc68070_regs.mmu.desc[(offset - 0x4020) / 4].base);
			break;
	}
}

static READ16_HANDLER( magic_r )
{
	return 0x1234;
}

static READ16_HANDLER( magic_2_r )
{
	return 0xf6;
}

static ADDRESS_MAP_START( cdi_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000000, 0x000fffff) AM_RAM AM_SHARE(1) AM_BASE(&ram)
	AM_RANGE(0x00200000, 0x002fffff) AM_RAM AM_BASE(&ram2)
	AM_RANGE(0x00301400, 0x00301403) AM_READ( magic_r )
	AM_RANGE(0x00310004, 0x00310007) AM_READ( magic_2_r )
	AM_RANGE(0x00320000, 0x0032ffff) AM_RAM
	AM_RANGE(0x00400000, 0x0047ffff) AM_ROM AM_REGION("maincpu", 0)
	AM_RANGE(0x004fffe0, 0x004fffff) AM_READWRITE(mcd212_r, mcd212_w)
	AM_RANGE(0x00500000, 0x0057ffff) AM_RAM
	AM_RANGE(0x00600000, 0x00ffffff) AM_NOP
	AM_RANGE(0x80000000, 0x8000807f) AM_READWRITE(scc68070_periphs_r, scc68070_periphs_w)
ADDRESS_MAP_END


/*************************
*      Input ports       *
*************************/

static INPUT_PORTS_START( cdi )
INPUT_PORTS_END


static MACHINE_RESET( cdi )
{
	UINT16 *src    = (UINT16*)memory_region( machine, "maincpu" );
	UINT16 *dst    = ram;
	memcpy (dst, src, 0x80000);
	device_reset(cputag_get_cpu(machine, "maincpu"));

	scc68070_regs.timers.timer0_timer = timer_alloc(machine, scc68070_timer0_callback, 0);
}

/*************************
*    Machine Drivers     *
*************************/

//static INTERRUPT_GEN( cdi_irq )
//{
//  if(input_code_pressed(KEYCODE_Z))
//      cpu_set_input_line(device->machine->firstcpu, 1, HOLD_LINE);
//  ram[0x2004/2]^=0xffff;
//}

static MACHINE_DRIVER_START( cdi )
	MDRV_CPU_ADD("maincpu", SCC68070, CLOCK_A/2)	/* SCC-68070 CCA84 datasheet */
	MDRV_CPU_PROGRAM_MAP(cdi_mem)
 	//MDRV_CPU_VBLANK_INT("screen", cdi_irq) /* no interrupts? (it erases the vectors..) */

	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_SIZE(768, 262)
	MDRV_SCREEN_VISIBLE_AREA(0, 639-1, 22, 262-1) //dynamic resolution,TODO

	MDRV_PALETTE_LENGTH(0x100)

	MDRV_VIDEO_START(cdi)
	MDRV_VIDEO_UPDATE(generic_bitmapped)

	MDRV_MACHINE_RESET(cdi)

	//MDRV_SPEAKER_STANDARD_MONO("mono")
	//MDRV_SOUND_ADD("ym", YM2413, CLOCK_A/12)
	//MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_CDROM_ADD( "cdrom" )
MACHINE_DRIVER_END

/*************************
*        Rom Load        *
*************************/

ROM_START( cdi )
	ROM_REGION(0x80000, "maincpu", 0)
	ROM_SYSTEM_BIOS( 0, "mcdi200", "Magnavox CD-i 200" )
	ROMX_LOAD( "cdi200.rom", 0x000000, 0x80000, CRC(40c4e6b9) SHA1(d961de803c89b3d1902d656ceb9ce7c02dccb40a), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "cdi220", "Philips CD-i 220" )
	ROMX_LOAD( "cdi220.rom", 0x000000, 0x80000, CRC(40c4e6b9) SHA1(d961de803c89b3d1902d656ceb9ce7c02dccb40a), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "pcdi490", "Philips CD-i 490" )
	ROMX_LOAD( "cdi490.rom", 0x000000, 0x80000, CRC(e115f45b) SHA1(f71be031a5dfa837de225081b2ddc8dcb74a0552), ROM_BIOS(3) )
	ROM_SYSTEM_BIOS( 3, "pcdi910m", "Philips CD-i 910" )
	ROMX_LOAD( "cdi910.rom", 0x000000, 0x80000,  CRC(8ee44ed6) SHA1(3fcdfa96f862b0cb7603fb6c2af84cac59527b05), ROM_BIOS(4) )
	/* Bad dump? */
	/* ROM_SYSTEM_BIOS( 4, "cdi205", "Philips CD-i 205" ) */
	/* ROMX_LOAD( "cdi205.rom", 0x000000, 0x7fc00, CRC(79ab41b2) SHA1(656890bbd3115b235bc8c6b1cf29a99e9db37c1b), ROM_BIOS(5) ) */
ROM_END

/*************************
*      Game driver(s)    *
*************************/

/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT   INIT    CONFIG  COMPANY     FULLNAME   FLAGS */
CONS( 1991, cdi,        0,      0,      cdi,        0,      0,      0,      "Philips",  "CD-i",   GAME_NO_SOUND | GAME_NOT_WORKING )
