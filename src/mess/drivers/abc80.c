/*
	abc80.c

	MESS Driver by Curt Coder

	Luxor ABC 80
	------------
	(c) 1978 Luxor Datorer AB, Sweden

	CPU:			Z80 @ 3 MHz
	ROM:			32 KB
	RAM:			16 KB, 1 KB frame buffer
	CRTC:			74S264
	Resolution:		240x240
	Colors:			2

	Luxor ABC 800C
	--------------
	(c) 1981 Luxor Datorer AB, Sweden

	CPU:			Z80 @ 3 MHz
	ROM:			32 KB
	RAM:			16 KB, 1 KB frame buffer, 16 KB high-resolution videoram (800C/HR)
	CRTC:			6845
	Resolution:		240x240
	Colors:			8

	Luxor ABC 800M
	--------------
	(c) 1981 Luxor Datorer AB, Sweden

	CPU:			Z80 @ 3 MHz
	ROM:			32 KB
	RAM:			16 KB, 2 KB frame buffer, 16 KB high-resolution videoram (800M/HR)
	CRTC:			6845
	Resolution:		480x240, 240x240 (HR)
	Colors:			2

	Luxor ABC 802
	-------------
	(c) 1983 Luxor Datorer AB, Sweden

	CPU:			Z80 @ 3 MHz
	ROM:			32 KB
	RAM:			16 KB, 2 KB frame buffer, 16 KB ram-floppy
	CRTC:			6845
	Resolution:		480x240
	Colors:			2

	Luxor ABC 806
	-------------
	(c) 1983 Luxor Datorer AB, Sweden

	CPU:			Z80 @ 3 MHz
	ROM:			32 KB
	RAM:			32 KB, 4 KB frame buffer, 128 KB scratch pad ram
	CRTC:			6845
	Resolution:		240x240, 256x240, 512x240
	Colors:			8

	http://www.devili.iki.fi/Computers/Luxor/
	http://hem.passagen.se/mani/abc/
*/

#include "driver.h"
#include "video/generic.h"
#include "includes/crtc6845.h"
#include "includes/centroni.h"
#include "devices/basicdsk.h"
#include "devices/cassette.h"
#include "devices/printer.h"
#include "machine/z80ctc.h"
#include "machine/z80pio.h"
#include "machine/z80sio.h"
#include "sound/sn76477.h"

static tilemap *bg_tilemap;

/* vidhrdw */

static WRITE8_HANDLER( abc80_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

static unsigned short colortable_abc80[] =
{
	0x00, 0x01,
	0x01, 0x00,
};

static unsigned short colortable_abc800c[] =
{
	0x00, 0x00,
	0x00, 0x01,
	0x00, 0x02,
	0x00, 0x03,
	0x00, 0x04,
	0x00, 0x05,
	0x00, 0x06,
	0x00, 0x07,
	// this is wrong
};

static PALETTE_INIT( abc80 )
{
	palette_set_color(machine,  0, 0x00, 0x00, 0x00); // black
	palette_set_color(machine,  1, 0xff, 0xff, 0xff); // white

	memcpy(colortable, colortable_abc80, sizeof(colortable_abc80));
}

static PALETTE_INIT( abc800m )
{
	palette_set_color(machine,  0, 0x00, 0x00, 0x00); // black
	palette_set_color(machine,  1, 0xff, 0xff, 0x00); // yellow (really white, but blue signal is disconnected from monitor)

	memcpy(colortable, colortable_abc80, sizeof(colortable_abc80));
}

static PALETTE_INIT( abc800c )	// probably wrong
{
	palette_set_color(machine, 0, 0x00, 0x00, 0x00); // black
	palette_set_color(machine, 1, 0x00, 0x00, 0xff); // blue
	palette_set_color(machine, 2, 0xff, 0x00, 0x00); // red
	palette_set_color(machine, 3, 0xff, 0x00, 0xff); // magenta
	palette_set_color(machine, 4, 0x00, 0xff, 0x00); // green
	palette_set_color(machine, 5, 0x00, 0xff, 0xff); // cyan
	palette_set_color(machine, 6, 0xff, 0xff, 0x00); // yellow
	palette_set_color(machine, 7, 0xff, 0xff, 0xff); // white

	memcpy(colortable, colortable_abc800c, sizeof(colortable_abc800c));
}

static void abc80_get_tile_info(int tile_index)
{
	int attr = videoram[tile_index];
	int bank = 0;	// TODO: bank 1 is graphics mode, add a [40][25] array to support it, also to videoram_w
	int code = attr & 0x7f;
	int color = (attr & 0x80) ? 1 : 0;

	SET_TILE_INFO(bank, code, color, 0)
}

static UINT32 abc80_tilemap_scan( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	return ((row & 0x07) << 7) + (row >> 3) * num_cols + col;
}

VIDEO_START( abc80 )
{
	bg_tilemap = tilemap_create(abc80_get_tile_info, abc80_tilemap_scan, 
		TILEMAP_OPAQUE, 6, 10, 40, 24);

	return 0;
}

VIDEO_START( abc800m )
{
	bg_tilemap = tilemap_create(abc80_get_tile_info, tilemap_scan_rows, 
		TILEMAP_OPAQUE, 6, 10, 80, 24);

	return 0;
}

static void abc800c_get_tile_info(int tile_index)
{
	int code = videoram[tile_index];
	int color = 1;						// WRONG!

	SET_TILE_INFO(0, code, color, 0)
}

VIDEO_START( abc800c )
{
	bg_tilemap = tilemap_create(abc800c_get_tile_info, tilemap_scan_rows,
		TILEMAP_OPAQUE, 6, 10, 40, 24);

	return 0;
}

VIDEO_UPDATE( abc80 )
{
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	return 0;
}

/* Read/Write Handlers */

static READ8_HANDLER( pling_r )
{
	return 0;
}

// IN/OUT

static READ8_HANDLER( in_r )
{
	return 0xff;
}

static WRITE8_HANDLER( out_w )
{
}

// HR

static WRITE8_HANDLER( hrs_w )
{
}

static WRITE8_HANDLER( hrc_w )
{
}

// STROBE

static READ8_HANDLER( strobe_r )
{
	return 0;
}

static WRITE8_HANDLER( strobe_w )
{
}

// ABC 806

static READ8_HANDLER( bankswitch_r )
{
	return 0;
}

static WRITE8_HANDLER( bankswitch_w )
{
}

static READ8_HANDLER( attribute_r )
{
	return 0;
}

static WRITE8_HANDLER( attribute_w )
{
}

static READ8_HANDLER( sync_r )
{
	return 0;
}

static WRITE8_HANDLER( fgctlprom_w )
{
}

static WRITE8_HANDLER( ram_ctrl_w )
{
}

/* CTC */

static READ8_HANDLER( ctc_r )
{
	return 0;
}

static WRITE8_HANDLER( ctc_w )
{
}


static READ8_HANDLER( status_r )
{
	int i;

	//return 0xff;

	for (i = 0; i < 64; i++)
		if (~readinputport(i / 8) & (1 << (i & 0x07)))
			return i + 0x80 + 64;

	return 0;
}

static READ8_HANDLER( io_reset_r )
{
	// reset i/o devices
	return 0;
}

/* ABC 80 - Sound */

/*
  Bit Name     Description
   0  SYSENA   1 On, 0 Off (inverted)
   1  EXTVCO   00 High freq, 01 Low freq
   2  VCOSEL   10 SLF cntrl, 11 SLF ctrl
   3  MIXSELB  000 VCO, 001 Noise, 010 SLF
   4  MIXSELA  011 VCO+Noise, 100 SLF+Noise, 101 SLF+VCO
   5  MIXSELC  110 SLF+VCO+Noise, 111 Quiet
   6  ENVSEL2  00 VCO, 01 Rakt igenom
   7  ENVSEL1  10 Monovippa, 11 VCO alt.pol.
*/

static void abc80_sound_w(UINT8 data)
{
	// TODO: beep sound won't stop after it starts

	SN76477_enable_w(0, ~data & 0x01);

	SN76477_vco_voltage_w(0, (data & 0x02) ? 2.5 : 0);
	SN76477_vco_w(0, (data & 0x04) ? 1 : 0);

	SN76477_mixer_a_w(0, (data & 0x10) ? 1 : 0);
	SN76477_mixer_b_w(0, (data & 0x08) ? 1 : 0);
	SN76477_mixer_c_w(0, (data & 0x20) ? 1 : 0);

	SN76477_envelope_1_w(0, (data & 0x80) ? 1 : 0);
	SN76477_envelope_2_w(0, (data & 0x40) ? 1 : 0);
}

static READ8_HANDLER( keyboard_r )
{
	return 0;
}

/* ABC 80x - CRTC 6845 */

static READ8_HANDLER( crtc_r )
{
	return 0;
}

static WRITE8_HANDLER( crtc_w )
{
	static int reg;

	switch (offset)
	{
	case 0:
		reg = data;
		break;
	case 1:
		//crtc6845_0_port_w(reg, data);
		break;
	}
}

/* ABC 80 - Keyboard */

static const UINT8 abc80_keycodes[7][8] = {
{0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38}, 
{0x39, 0x30, 0x2B, 0x60, 0x3C, 0x71, 0x77, 0x65}, 
{0x72, 0x74, 0x79, 0x75, 0x69, 0x6F, 0x70, 0x7D}, 
{0x7E, 0x0D, 0x61, 0x73, 0x64, 0x66, 0x67, 0x68}, 
{0x6A, 0x6B, 0x6C, 0x7C, 0x7B, 0x27, 0x08, 0x7A}, 
{0x78, 0x63, 0x76, 0x62, 0x6E, 0x6D, 0x2C, 0x2E}, 
{0x2D, 0x09, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00}
};

static void abc80_keyboard_scan(void)
{
	UINT8 keycode = 0;
	UINT8 data;
	int irow, icol;

	for (irow = 0; irow < 7; irow++)
	{
		data = readinputport(irow);
		if (data != 0)
		{
			UINT8 ibit = 1;
			for (icol = 0; icol < 8; icol++)
			{
				if (data & ibit)
					keycode = abc80_keycodes[irow][icol];
				ibit <<= 1;
			}			
		}
	}
	
	/* shift, upper case */
//	if (readinputport(7) & 0x07)
	
	/* ctrl */
//	if (readinputport(7) & 0x08)
	
	if (keycode != 0)
		z80pio_d_w(0, 0, keycode | 0x80);
}

// PIO

static UINT8 abc80_pio_r(UINT16 offset)
{
	switch (offset)
	{
	case 0:
	{
		UINT8 data;
		data = z80pio_d_r(0, 0);
//		z80pio_d_w(0, 0, data & ~0x80);
		return data;
	}
	case 1:
		return z80pio_c_r(0, 0);
	case 2:
		return z80pio_d_r(0, 1);
	case 3:
		return z80pio_c_r(0, 1);
	}

	return 0xff;
}

static void abc80_pio_w(UINT16 offset, UINT8 data)
{
	switch (offset)
	{
	case 0:
		z80pio_d_w(0, 0, data);
		break;
	case 1:
		z80pio_c_w(0, 0, data);
		break;
	case 2:
		z80pio_d_w(0, 1, data);
		break;
	case 3:
		z80pio_c_w(0, 1, data);
		break;
	}
}

// SIO/2

static READ8_HANDLER( sio2_r )
{
	switch (offset)
	{
	case 0:
	case 1:
	case 2:
	case 3:
		break;
	}

	return 0;
}

static WRITE8_HANDLER( sio2_w )
{
	switch (offset)
	{
	case 0:
		break;
	case 1:
		break;
	case 2:
		break;
	case 3:
		break;
	}
}

// DART

/*
typedef struct
{
	UINT8 latch;
	UINT8 wreg[8];
	UINT8 rreg[3];	
} typDART;

static typDART abc80_dart[2];
*/

static READ8_HANDLER( dart_r )
{
	switch (offset)
	{
	case 0:
	case 1:
	case 2:
	case 3:
		break;
	}

	return 0;
}

static WRITE8_HANDLER( dart_w )
{
	switch (offset)
	{
	case 0:
		break;
	case 1:
		break;
	case 2:
		break;
	case 3:
		break;
	}
}

//

static WRITE8_HANDLER( drive_select_w )
{
	/*
		36		ABC 850/852/856		10/20/60MB Hard Disk
		44		ABC 832/834/850		2x640KB Floppy Disk
		45		ABC 830				2x160KB Floppy Disk
		46		ABC 838				2x1MB Floppy Disk
	*/
}

// ABC55/77 keyboard

static READ8_HANDLER( abc77_keyboard_r )
{
	return 0;
}

static WRITE8_HANDLER( abc77_keyboard_w )
{
}

static WRITE8_HANDLER( abc77_keyboard_status_w )
{
}

static READ8_HANDLER( abc80_io_r )
{
	if (offset & 0x10)
		return abc80_pio_r(offset & 0x03);

	switch (offset)
	{
	case 0x00: /* ABC Bus - Data in */
		break;
	case 0x01: /* ABC Bus - Status in */
		break;
	case 0x07: /* ABC Bus - Reset */
		break;
	}
	return 0xff;
}

static WRITE8_HANDLER( abc80_io_w )
{
	if (offset & 0x10)
	{
		abc80_pio_w(offset & 0x03, data);
		return;
	}

	switch (offset)
	{
	case 0x00: /* ABC Bus - Data out */
		break;
	case 0x01: /* ABC Bus - Channel select */
		break;
	case 0x02: /* ABC Bus - Command C1 */
		break;
	case 0x03: /* ABC Bus - Command C2 */
		break;
	case 0x04: /* ABC Bus - Command C3 */
		break;
	case 0x05: /* ABC Bus - Command C4 */
		break;
	case 0x06: /* SN76477 Sound */
		abc80_sound_w(data);
		break;
	}
}

/* Memory Maps */

// ABC 80

static ADDRESS_MAP_START( abc80_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM // ROM Basic
//	AM_RANGE(0x6000, 0x6fff) AM_ROM // ROM Floppy
//	AM_RANGE(0x7800, 0x7bff) AM_ROM // ROM Printer
	AM_RANGE(0x7c00, 0x7fff) AM_RAM AM_WRITE(abc80_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x8000, 0xbfff) AM_RAM	// RAM Expanded
	AM_RANGE(0xc000, 0xffff) AM_RAM	// RAM Internal
ADDRESS_MAP_END

static ADDRESS_MAP_START( abc80_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0xff) AM_WRITE(abc80_io_w)
	AM_RANGE(0x00, 0xff) AM_READ(abc80_io_r)
/*
	AM_RANGE(0x06, 0x06) AM_WRITE(sound_w)
	AM_RANGE(0x07, 0x07) AM_READ(io_reset_r)
	AM_RANGE(0x38, 0x38) AM_READ(status_r)
	AM_RANGE(0x38, 0x3b) AM_WRITE(pio_w)
*/	
ADDRESS_MAP_END

// ABC 800

static ADDRESS_MAP_START( abc800_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x77ff) AM_ROM
	AM_RANGE(0x7800, 0x7fff) AM_RAM AM_WRITE(abc80_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x8000, 0xbfff) AM_RAM	// Work RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( abc800_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x02) AM_READ(in_r)
	AM_RANGE(0x05, 0x05) AM_READ(pling_r)
	AM_RANGE(0x00, 0x05) AM_WRITE(out_w)
	AM_RANGE(0x06, 0x06) AM_WRITE(hrs_w)
	AM_RANGE(0x07, 0x07) AM_READWRITE(io_reset_r, hrc_w)
	AM_RANGE(0x20, 0x23) AM_READWRITE(dart_r, dart_w)
	AM_RANGE(0x30, 0x32) AM_WRITE(ram_ctrl_w)
	AM_RANGE(0x31, 0x31) AM_READ(crtc_r)
	AM_RANGE(0x38, 0x39) AM_WRITE(crtc_w)
	AM_RANGE(0x40, 0x43) AM_READWRITE(sio2_r, sio2_w)
	AM_RANGE(0x50, 0x53) AM_READWRITE(ctc_r, ctc_w)
	AM_RANGE(0x80, 0xff) AM_READWRITE(strobe_r, strobe_w)
ADDRESS_MAP_END

// ABC 55/77 keyboard

static ADDRESS_MAP_START( abc77_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x000, 0xfff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( abc77_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_WRITE(abc77_keyboard_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(abc77_keyboard_r, abc77_keyboard_status_w)
ADDRESS_MAP_END

// HR high resolution graphics card

static ADDRESS_MAP_START( hr_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x7800, 0x7fff) AM_RAM
ADDRESS_MAP_END

// ABC 802

static ADDRESS_MAP_START( abc802_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x77ff) AM_ROM
	AM_RANGE(0x7800, 0x7fff) AM_RAM AM_WRITE(abc80_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x8000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( abc802_io_map, ADDRESS_SPACE_IO, 8 )
/*	AM_RANGE(0x00, 0x02) AM_READ(in_r)
	AM_RANGE(0x00, 0x05) AM_WRITE(out_w)
	AM_RANGE(0x06, 0x06) AM_WRITE(hrs_w)
	AM_RANGE(0x07, 0x07) AM_READWRITE(io_reset_r, hrc_w)
	AM_RANGE(0x20, 0x23) AM_READWRITE(dart_r, dart_w)
	AM_RANGE(0x31, 0x31) AM_READ(crtc_r)
	AM_RANGE(0x32, 0x35) AM_READWRITE(sio2_r, sio2_w)
	AM_RANGE(0x38, 0x39) AM_WRITE(crtc_w)
	AM_RANGE(0x60, 0x63) AM_READWRITE(ctc_r, ctc_w)
	AM_RANGE(0x80, 0xff) AM_READWRITE(strobe_r, strobe_w)*/
ADDRESS_MAP_END

// ABC 806

static ADDRESS_MAP_START( abc806_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x77ff) AM_ROM
	AM_RANGE(0x7800, 0x7fff) AM_RAM AM_WRITE(abc80_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x8000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( abc806_io_map, ADDRESS_SPACE_IO, 8 )
/*	AM_RANGE(0x00, 0x02) AM_READ(in_r)
	AM_RANGE(0x00, 0x05) AM_WRITE(out_w)
	AM_RANGE(0x06, 0x06) AM_WRITE(hrs_w)
	AM_RANGE(0x07, 0x07) AM_READWRITE(io_reset_r, hrc_w)
	AM_RANGE(0x20, 0x23) AM_READWRITE(dart_r, dart_w)
	AM_RANGE(0x31, 0x31) AM_READ(crtc_r)
	AM_RANGE(0x34, 0x34) AM_READWRITE(bankswitch_r, bankswitch_w)
	AM_RANGE(0x35, 0x35) AM_READWRITE(attribute_r, attribute_w)
	AM_RANGE(0x37, 0x37) AM_READWRITE(sync_r, fgctlprom_w)
	AM_RANGE(0x38, 0x39) AM_WRITE(crtc_w)
	AM_RANGE(0x40, 0x41) AM_READWRITE(sio2_r, sio2_w)
	AM_RANGE(0x60, 0x63) AM_READWRITE(ctc_r, ctc_w)
	AM_RANGE(0x80, 0xff) AM_READWRITE(strobe_r, strobe_w)*/
ADDRESS_MAP_END

/* Input Ports */

INPUT_PORTS_START( abc77 )
	PORT_START_TAG("ROW0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START_TAG("ROW1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START_TAG("ROW2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START_TAG("ROW3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START_TAG("ROW4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START_TAG("ROW5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START_TAG("ROW6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START_TAG("ROW7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START_TAG("ROW8")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START_TAG("ROW9")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START_TAG("ROW10")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START_TAG("ROW11")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x01, 0x01, "Keyboard Program" )
	PORT_DIPSETTING(    0x00, "Internal (8048)" )
	PORT_DIPSETTING(    0x01, "External PROM" )
	PORT_DIPNAME( 0x02, 0x00, "Character Set" )
	PORT_DIPSETTING(    0x00, "Swedish" )
	PORT_DIPSETTING(    0x02, "US ASCII" )
	PORT_DIPNAME( 0x04, 0x04, "External PROM" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, "Keyboard Type" )
	PORT_DIPSETTING(    0x00, "Danish" )
	PORT_DIPSETTING(    0x10, DEF_STR( French ) )
	PORT_DIPSETTING(    0x08, DEF_STR( German ) )
	PORT_DIPSETTING(    0x18, DEF_STR( Spanish ) )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END

INPUT_PORTS_START( abc80 )
 PORT_START /* 0 */
  PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("1 !") PORT_CODE(KEYCODE_1)
  PORT_BIT (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2 \"") PORT_CODE(KEYCODE_2)
  PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3 #") PORT_CODE(KEYCODE_3)
  PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("4 \xC2\xA4") PORT_CODE(KEYCODE_4)
  PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("5 %") PORT_CODE(KEYCODE_5)
  PORT_BIT (0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("6 &") PORT_CODE(KEYCODE_6)
  PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("7 /") PORT_CODE(KEYCODE_7)
  PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("8 (") PORT_CODE(KEYCODE_8)
 PORT_START /* 1 */
  PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9 )") PORT_CODE(KEYCODE_9)
  PORT_BIT (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0 =") PORT_CODE(KEYCODE_0)
  PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("+ ?") PORT_CODE(KEYCODE_MINUS)
  PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("\xC3\xA9 \xC3\x89") PORT_CODE(KEYCODE_EQUALS)
  PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("< >") PORT_CODE(KEYCODE_BACKSLASH)
  PORT_BIT (0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("q Q") PORT_CODE(KEYCODE_Q)
  PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("w W") PORT_CODE(KEYCODE_W)
  PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("e E") PORT_CODE(KEYCODE_E)
 PORT_START /* 2 */
  PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("r R") PORT_CODE(KEYCODE_R)
  PORT_BIT (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("t T") PORT_CODE(KEYCODE_T)
  PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("y Y") PORT_CODE(KEYCODE_Y)
  PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("u U") PORT_CODE(KEYCODE_U)
  PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("i I") PORT_CODE(KEYCODE_I)
  PORT_BIT (0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("o O") PORT_CODE(KEYCODE_O)
  PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("p P") PORT_CODE(KEYCODE_P)
  PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("\xC3\xA5 \xC3\x85") PORT_CODE(KEYCODE_OPENBRACE)
 PORT_START /* 3 */
  PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("\xC3\xBC \xC3\x9C") PORT_CODE(KEYCODE_CLOSEBRACE)
  PORT_BIT (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER)
  PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("a A") PORT_CODE(KEYCODE_A)
  PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("s S") PORT_CODE(KEYCODE_S)
  PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("d D") PORT_CODE(KEYCODE_D)
  PORT_BIT (0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("f F") PORT_CODE(KEYCODE_F)
  PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("g G") PORT_CODE(KEYCODE_G)
  PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("h H") PORT_CODE(KEYCODE_H)
 PORT_START /* 4 */
  PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("j J") PORT_CODE(KEYCODE_J)
  PORT_BIT (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("l L") PORT_CODE(KEYCODE_L)
  PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("\xC3\xB6 \xC3\x96") PORT_CODE(KEYCODE_COLON)
  PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("\xC3\xA4 \xC3\x84") PORT_CODE(KEYCODE_QUOTE)
  PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("' *") PORT_CODE(KEYCODE_BACKSLASH)
  PORT_BIT (0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("LEFT") PORT_CODE(KEYCODE_LEFT)
  PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("z Z") PORT_CODE(KEYCODE_Z)
  PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("x X") PORT_CODE(KEYCODE_X)
 PORT_START /* 5 */
  PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("c C") PORT_CODE(KEYCODE_C)
  PORT_BIT (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("v V") PORT_CODE(KEYCODE_V)
  PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("b B") PORT_CODE(KEYCODE_B)
  PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("n N") PORT_CODE(KEYCODE_N)
  PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("m M") PORT_CODE(KEYCODE_M)
  PORT_BIT (0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(", ;") PORT_CODE(KEYCODE_COMMA)
  PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(". :") PORT_CODE(KEYCODE_STOP)
  PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("- _") PORT_CODE(KEYCODE_SLASH)
 PORT_START /* 6 */
  PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RIGHT") PORT_CODE(KEYCODE_RIGHT)
  PORT_BIT (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE)
  PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_UNUSED)   
  PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_UNUSED)   
  PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_UNUSED)   
  PORT_BIT (0x20, IP_ACTIVE_HIGH, IPT_UNUSED)   
  PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_UNUSED)   
  PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_UNUSED)   
 PORT_START /* 7 */
  PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("LEFT SHIFT") PORT_CODE(KEYCODE_LSHIFT)
  PORT_BIT (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RIGHT SHIFT") PORT_CODE(KEYCODE_RSHIFT)
  PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("UPPER CASE") PORT_CODE(KEYCODE_CAPSLOCK)
  PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL)
  PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_UNUSED)   
  PORT_BIT (0x20, IP_ACTIVE_HIGH, IPT_UNUSED)   
  PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_UNUSED)   
  PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_UNUSED)   
INPUT_PORTS_END

INPUT_PORTS_START( abc800 )
	PORT_INCLUDE(abc77)
INPUT_PORTS_END

INPUT_PORTS_START( abc802 )
	PORT_INCLUDE(abc77)
INPUT_PORTS_END

INPUT_PORTS_START( abc806 )
	PORT_INCLUDE(abc77)
INPUT_PORTS_END

/* Graphics Layouts */

static gfx_layout charlayout_abc80 =
{
	6, 10,
	128,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8 },
	10*8
};

static gfx_layout charlayout_abc800m =
{
	6, 10,
	128,
	1,
	{ 0 },
	{ 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8 },
	16*8
};

/* Graphics Decode Info */

static gfx_decode gfxdecodeinfo_abc80[] =
{
	{ REGION_GFX1, 0,     &charlayout_abc80, 0, 2 },	// normal characters
	{ REGION_GFX1, 0x500, &charlayout_abc80, 0, 2 },	// graphics characters
	{ -1 }
};

static gfx_decode gfxdecodeinfo_abc800m[] =
{
	{ REGION_GFX1, 0, &charlayout_abc800m, 0, 2 },
	{ -1 }
};

static gfx_decode gfxdecodeinfo_abc802[] =
{
	{ REGION_GFX1, 0,     &charlayout_abc800m, 0, 2 },
	{ REGION_GFX1, 0x800, &charlayout_abc800m, 0, 2 },
	{ -1 }
};

/* Z80 Interfaces */

static void abc80_pio_interrupt( int state )
{
	logerror("pio irq state: %02x\n",state);
//	cpu_set_irq_line(0, 3, state);
//	cpunum_set_input_line (0, 0, HOLD_LINE);

	cpunum_set_input_line(0, 0, (state ? HOLD_LINE : CLEAR_LINE));
//	cpu_set_irq_line(0, Z80_IRQ_STATE, state ? ASSERT_LINE : CLEAR_LINE);
}

/*
	PIO Channel A

	0  R	Keyboard
	1  R
	2  R
	3  R
	4  R
	5  R
	6  R
	7  R	Strobe

	PIO Channel B
	
	0  R	RS-232C RxD
	1  R	RS-232C _CTS
	2  R	RS-232C _DCD
	3  W	RS-232C TxD
	4  W	RS-232C _RTS
	5  W	Cassette Memory Control Relay
	6  W	Cassette Memory 1
	7  R	Cassette Memory 5
*/

static z80pio_interface abc80_pio_interface = 
{
	abc80_pio_interrupt,
	NULL,
	NULL
};

/* Sound Interfaces */

static struct SN76477interface sn76477_interface =
{
	RES_K(47),		//  4  noise_res		R26 47k
	RES_K(330),		//  5  filter_res		R24 330k
	CAP_P(390),		//  6  filter_cap		C52 390p
	RES_K(47),		//  7  decay_res		R23 47k
	CAP_U(10),		//  8  attack_decay_cap		C50 10u/35V
	RES_K(2.2),		// 10  attack_res		R21 2.2k
	RES_K(33),		// 11  amplitude_res		R19 33k
	RES_K(10),		// 12  feedback_res		R18 10k
	0,			// 16  vco_voltage		0V or 2.5V
	CAP_N(10) ,		// 17  vco_cap			C48 10n
	RES_K(100),		// 18  vco_res			R20 100k
	0,			// 19  pitch_voltage		N/C
	RES_K(220),		// 20  slf_res			R22 220k
	CAP_U(1),		// 21  slf_cap			C51 1u/35V
	CAP_U(0.1),		// 23  oneshot_cap		C53 0.1u
	RES_K(330)		// 24  oneshot_res		R25 330k
};

/* Interrupt Generators */

INTERRUPT_GEN( abc80_nmi_interrupt )
{
	abc80_keyboard_scan();
	cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
}

/*static INTERRUPT_GEN( abc800_interrupt )
{
	//cpu_set_irq_line(0, 0, HOLD_LINE);
//	if (~readinputport(0))
		//cpu_set_irq_line_and_vector(0, 0, PULSE_LINE, 26);
	//	cpu_set_irq_line(0, 0, ASSERT_LINE);
//	else
		//cpu_set_irq_line(0, 0, CLEAR_LINE);

	cpu_set_irq_line(0, 1, PULSE_LINE);
}*/

/* Machine Initialization */

static MACHINE_START( abc80 )
{
	z80pio_init(0, &abc80_pio_interface);
	return 0;
}

/* Machine Drivers */

static MACHINE_DRIVER_START( abc80 )
	// basic machine hardware
	MDRV_CPU_ADD(Z80, 11980800/2/2)	// 2.9952 MHz
	MDRV_CPU_PROGRAM_MAP(abc80_map, 0)
	MDRV_CPU_IO_MAP(abc80_io_map, 0)

	MDRV_CPU_PERIODIC_INT(abc80_nmi_interrupt, TIME_IN_HZ(50))

	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_START(abc80)

	// video hardware
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(40*6, 24*10)
	MDRV_SCREEN_VISIBLE_AREA(0, 40*6-1, 0, 24*10-1)
	MDRV_GFXDECODE(gfxdecodeinfo_abc80)
	MDRV_PALETTE_LENGTH(2)
	MDRV_COLORTABLE_LENGTH(2*2)

	MDRV_PALETTE_INIT(abc80)
	MDRV_VIDEO_START(abc80)
	MDRV_VIDEO_UPDATE(abc80)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SN76477, 0)
	MDRV_SOUND_CONFIG(sn76477_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( abc800m )
	// basic machine hardware
	MDRV_CPU_ADD_TAG("main", Z80, 12000000/2/2)	// 3 MHz
	MDRV_CPU_PROGRAM_MAP(abc800_map, 0)
	MDRV_CPU_IO_MAP(abc800_io_map, 0)
	//MDRV_CPU_VBLANK_INT(abc800_interrupt, 1)

	//MDRV_CPU_ADD(I8035, 4608000) // 4.608 MHz, keyboard cpu
	//MDRV_CPU_PROGRAM_MAP(abc77_map, 0)
	//MDRV_CPU_IO_MAP(abc77_io_map, 0)

	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	// video hardware
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(80*6, 24*10)
	MDRV_SCREEN_VISIBLE_AREA(0, 80*6-1, 0, 24*10-1)
	MDRV_GFXDECODE(gfxdecodeinfo_abc800m)
	MDRV_PALETTE_LENGTH(2)
	MDRV_COLORTABLE_LENGTH(2*2)

	MDRV_PALETTE_INIT(abc800m)
	MDRV_VIDEO_START(abc800m)
	MDRV_VIDEO_UPDATE(abc80)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( abc800c )
	MDRV_IMPORT_FROM(abc800m)
	
	MDRV_SCREEN_SIZE(40*6, 24*10)
	MDRV_SCREEN_VISIBLE_AREA(0, 40*6-1, 0, 24*10-1)
	MDRV_PALETTE_LENGTH(8)
	MDRV_COLORTABLE_LENGTH(16)	// WRONG!

	MDRV_PALETTE_INIT(abc800c)
	MDRV_VIDEO_START(abc800c)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( abc802 )
	MDRV_IMPORT_FROM(abc800c)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(abc802_map, 0)
	MDRV_CPU_IO_MAP(abc802_io_map, 0)
	
	MDRV_GFXDECODE(gfxdecodeinfo_abc802)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( abc806 )
	MDRV_IMPORT_FROM(abc800c)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(abc806_map, 0)
	MDRV_CPU_IO_MAP(abc806_io_map, 0)
MACHINE_DRIVER_END

/* Devices */

int device_load_abc80_floppy(mess_image *image)
{
	int size, tracks, heads, sectors;

	if (image_has_been_created(image))
		return INIT_FAIL;

	size = image_length (image);
	switch (size)
	{
	case 80*1024: /* Scandia Metric FD2 */
		tracks = 40;
		heads = 1;
		sectors = 8;
		break;
	case 160*1024: /* ABC 830 */
		tracks = 40;
		heads = 1;
		sectors = 16;
		break;
	case 640*1024: /* ABC 832/834 */
		tracks = 80;
		heads = 2;
		sectors = 16;
		break;
	case 1001*1024: /* ABC 838 */
		tracks = 77;
		heads = 2;
		sectors = 26;
		break;
	default:
		return INIT_FAIL;
	}

	if (device_load_basicdsk_floppy(image)==INIT_PASS)
	{
		/* sector id's 0-9 */
		/* drive, tracks, heads, sectors per track, sector length, dir_sector, dir_length, first sector id */
		basicdsk_set_geometry(image, tracks, heads, sectors, 256, 0, 0, FALSE);
		return INIT_PASS;
	}

	return INIT_FAIL;
}

/* ROMs */

ROM_START( abc80 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "1.a2", 0x0000, 0x1000, CRC(e2afbf48) SHA1(9883396edd334835a844dcaa792d29599a8c67b9) )
	ROM_LOAD( "2.a3", 0x1000, 0x1000, CRC(d224412a) SHA1(30968054bba7c2aecb4d54864b75a446c1b8fdb1) )
	ROM_LOAD( "3.a4", 0x2000, 0x1000, CRC(1502ba5b) SHA1(5df45909c2c4296e5701c6c99dfaa9b10b3a729b) )
	ROM_LOAD( "4.a5", 0x3000, 0x1000, CRC(bc8860b7) SHA1(28b6cf7f5a4f81e017c2af091c3719657f981710) )

	ROM_REGION( 0x0a00, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "chargen", 0x0000, 0x0a00, CRC(9e064e91) SHA1(354783c8f2865f73dc55918c9810c66f3aca751f) )

	ROM_REGION( 0x400, REGION_PROMS, 0 )
	ROM_LOAD( "82s129.e7", 0x0000, 0x0080, NO_DUMP ) // 256x4 address decoder PROM
	ROM_LOAD( "82s129.j3", 0x0080, 0x0080, NO_DUMP ) // 256x4 chargen 74S263 column address PROM
	ROM_LOAD( "82s129.k5", 0x0100, 0x0080, NO_DUMP ) // 256x4 horizontal sync PROM
	ROM_LOAD( "82s131.k1", 0x0180, 0x0100, NO_DUMP ) // 512x4 chargen 74S263 row address PROM
	ROM_LOAD( "82s131.k2", 0x0280, 0x0100, NO_DUMP ) // 512x4 vertical sync, videoram PROM
ROM_END

ROM_START( abc800c )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "abcc.1m",    0x0000, 0x1000, NO_DUMP )
	ROM_LOAD( "abc1-12.1l", 0x1000, 0x1000, CRC(1e99fbdc) SHA1(ec6210686dd9d03a5ed8c4a4e30e25834aeef71d) )
	ROM_LOAD( "abc2-12.1k", 0x2000, 0x1000, CRC(ac196ba2) SHA1(64fcc0f03fbc78e4c8056e1fa22aee12b3084ef5) )
	ROM_LOAD( "abc3-12.1j", 0x3000, 0x1000, CRC(3ea2b5ee) SHA1(5a51ac4a34443e14112a6bae16c92b5eb636603f) )
	ROM_LOAD( "abc4-12.2m", 0x4000, 0x1000, CRC(695cb626) SHA1(9603ce2a7b2d7b1cbeb525f5493de7e5c1e5a803) )
	ROM_LOAD( "abc5-12.2l", 0x5000, 0x1000, CRC(b4b02358) SHA1(95338efa3b64b2a602a03bffc79f9df297e9534a) )
	ROM_LOAD( "abc6-13.2k", 0x6000, 0x1000, CRC(6fa71fb6) SHA1(b037dfb3de7b65d244c6357cd146376d4237dab6) )
	ROM_LOAD( "abc7-21.2j", 0x7000, 0x1000, CRC(fd137866) SHA1(3ac914d90db1503f61397c0ea26914eb38725044) )

	ROM_REGION( 0x1000, REGION_CPU2, 0 )
	ROM_LOAD( "keyboard.z10", 0x0000, 0x0800, NO_DUMP ) // 2716 ABC55/77 keyboard controller Swedish EPROM
	ROM_LOAD( "keyboard.z14", 0x0800, 0x0800, NO_DUMP ) // 2716 ABC55/77 keyboard controller non-Swedish EPROM

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "vuc-se.7c",  0x0000, 0x1000, NO_DUMP )
ROM_END

ROM_START( abc800m )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "abcm.1m",    0x0000, 0x1000, CRC(f85b274c) SHA1(7d0f5639a528d8d8130a22fe688d3218c77839dc) )
	ROM_LOAD( "abc1-12.1l", 0x1000, 0x1000, CRC(1e99fbdc) SHA1(ec6210686dd9d03a5ed8c4a4e30e25834aeef71d) )
	ROM_LOAD( "abc2-12.1k", 0x2000, 0x1000, CRC(ac196ba2) SHA1(64fcc0f03fbc78e4c8056e1fa22aee12b3084ef5) )
	ROM_LOAD( "abc3-12.1j", 0x3000, 0x1000, CRC(3ea2b5ee) SHA1(5a51ac4a34443e14112a6bae16c92b5eb636603f) )
	ROM_LOAD( "abc4-12.2m", 0x4000, 0x1000, CRC(695cb626) SHA1(9603ce2a7b2d7b1cbeb525f5493de7e5c1e5a803) )
	ROM_LOAD( "abc5-12.2l", 0x5000, 0x1000, CRC(b4b02358) SHA1(95338efa3b64b2a602a03bffc79f9df297e9534a) )
	ROM_LOAD( "abc6-13.2k", 0x6000, 0x1000, CRC(6fa71fb6) SHA1(b037dfb3de7b65d244c6357cd146376d4237dab6) )
	ROM_LOAD( "abc7-21.2j", 0x7000, 0x1000, CRC(fd137866) SHA1(3ac914d90db1503f61397c0ea26914eb38725044) )

	ROM_REGION( 0x1000, REGION_CPU2, 0 )
	ROM_LOAD( "keyboard.z10", 0x0000, 0x0800, NO_DUMP ) // 2716 ABC55/77 keyboard controller Swedish EPROM
	ROM_LOAD( "keyboard.z14", 0x0800, 0x0800, NO_DUMP ) // 2716 ABC55/77 keyboard controller non-Swedish EPROM

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "vum-se.7c",  0x0000, 0x0800, CRC(f9152163) SHA1(997313781ddcbbb7121dbf9eb5f2c6b4551fc799) )
ROM_END

ROM_START( abc802 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "abc02-11.9f",  0x0000, 0x2000, CRC(b86537b2) SHA1(4b7731ef801f9a03de0b5acd955f1e4a1828355d) )
	ROM_LOAD( "abc12-11.11f", 0x2000, 0x2000, CRC(3561c671) SHA1(f12a7c0fe5670ffed53c794d96eb8959c4d9f828) )
	ROM_LOAD( "abc22-11.12f", 0x4000, 0x2000, CRC(8dcb1cc7) SHA1(535cfd66c84c0370fd022d6edf702d3d1ad1b113) )
//	ROM_LOAD( "abc32-21.14f", 0x6000, 0x2000, CRC(57050b98) SHA1(b977e54d1426346a97c98febd8a193c3e8259574) ) // v1.9
	ROM_LOAD( "abc32-31.14f", 0x6000, 0x2000, CRC(fc8be7a8) SHA1(a1d4cb45cf5ae21e636dddfa70c99bfd2050ad60) ) // v2.0

	ROM_REGION( 0x1000, REGION_CPU2, 0 )
	ROM_LOAD( "keyboard.z10", 0x0000, 0x0800, NO_DUMP ) // 2716 ABC55/77 keyboard controller Swedish EPROM
	ROM_LOAD( "keyboard.z14", 0x0800, 0x0800, NO_DUMP ) // 2716 ABC55/77 keyboard controller non-Swedish EPROM

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "abct2-11.3g",  0x0000, 0x2000, CRC(e21601ee) SHA1(2e838ebd7692e5cb9ba4e80fe2aa47ea2584133a) )
ROM_END

ROM_START( abc806 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "abc06-11.1m",  0x0000, 0x1000, CRC(27083191) SHA1(9b45592273a5673e4952c6fe7965fc9398c49827) )
	ROM_LOAD( "abc16-11.1l",  0x1000, 0x1000, CRC(eb0a08fd) SHA1(f0b82089c5c8191fbc6a3ee2c78ce730c7dd5145) )
	ROM_LOAD( "abc26-11.1k",  0x2000, 0x1000, CRC(97a95c59) SHA1(013bc0a2661f4630c39b340965872bf607c7bd45) )
	ROM_LOAD( "abc36-11.1j",  0x3000, 0x1000, CRC(b50e418e) SHA1(991a59ed7796bdcfed310012b2bec50f0b8df01c) )
	ROM_LOAD( "abc46-11.2m",  0x4000, 0x1000, CRC(17a87c7d) SHA1(49a7c33623642b49dea3d7397af5a8b9dde8185b) )
	ROM_LOAD( "abc56-11.2l",  0x5000, 0x1000, CRC(b4b02358) SHA1(95338efa3b64b2a602a03bffc79f9df297e9534a) )
	ROM_LOAD( "abc66-31.2k",  0x6000, 0x1000, CRC(a2e38260) SHA1(0dad83088222cb076648e23f50fec2fddc968883) )
	ROM_LOAD( "abc76-11.2j",  0x7000, 0x1000, CRC(3eb5f6a1) SHA1(02d4e38009c71b84952eb3b8432ad32a98a7fe16) )

	ROM_REGION( 0x1000, REGION_CPU2, 0 )
	ROM_LOAD( "keyboard.z10", 0x0000, 0x0800, NO_DUMP ) // 2716 ABC55/77 keyboard controller Swedish EPROM
	ROM_LOAD( "keyboard.z14", 0x0800, 0x0800, NO_DUMP ) // 2716 ABC55/77 keyboard controller non-Swedish EPROM

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "char.7c",  0x0000, 0x1000, CRC(b17c51c5) SHA1(e466e80ec989fbd522c89a67d274b8f0bed1ff72) )
ROM_END

static void abc80_printer_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* printer */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										printer_device_getinfo(devclass, state, info); break;
	}
}

#if 0
static void abc80_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	// cassette
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_CASSETTE_FORMATS:				info->p = (void *) abc80_cassette_formats; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}
#endif

static void abc80_floppy_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_abc80_floppy; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "dsk"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}

/* System Configuration */

SYSTEM_CONFIG_START( abc80 )
	CONFIG_RAM_DEFAULT	( 16 * 1024)
	CONFIG_RAM		( 32 * 1024)
	CONFIG_DEVICE(abc80_printer_getinfo)
//	CONFIG_DEVICE(abc80_cassette_getinfo)
	CONFIG_DEVICE(abc80_floppy_getinfo)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START( abc800 )
	CONFIG_RAM_DEFAULT	( 16 * 1024)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START( abc802 )
	CONFIG_RAM_DEFAULT	( 32 * 1024)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START( abc806 )
	CONFIG_RAM_DEFAULT	(160 * 1024)
	CONFIG_RAM		(544 * 1024)
SYSTEM_CONFIG_END

/* Drivers */

/*    YEAR	NAME  PARENT  COMPAT MACHINE  INPUT	INIT CONFIG	 COMPANY             FULLNAME */
COMP( 1978, abc80,   0,       0, abc80,   abc80,  0, abc80,  "Luxor Datorer AB", "ABC 80", GAME_NOT_WORKING )
COMP( 1981, abc800c, 0,       0, abc800c, abc800, 0, abc800, "Luxor Datorer AB", "ABC 800C/HR", GAME_NOT_WORKING )
COMP( 1981, abc800m, abc800c, 0, abc800m, abc800, 0, abc800, "Luxor Datorer AB", "ABC 800M/HR", GAME_NOT_WORKING )
COMP( 1983, abc802,  0,       0, abc802,  abc802, 0, abc802, "Luxor Datorer AB", "ABC 802", GAME_NOT_WORKING )
COMP( 1983, abc806,  0,       0, abc806,  abc806, 0, abc806, "Luxor Datorer AB", "ABC 806", GAME_NOT_WORKING )
