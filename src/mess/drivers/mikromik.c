#include "driver.h"
#include "includes/mikromik.h"
#include "formats/basicdsk.h"
#include "devices/flopdrv.h"
#include "cpu/i8085/i8085.h"
#include "machine/8237dma.h"
#include "machine/i8212.h"
#include "machine/upd765.h"
#include "machine/pit8253.h"
#include "machine/upd7201.h"
#include "video/i8275.h"
#include "video/upd7220.h"
#include "sound/speaker.h"
#include "devices/messram.h"

/*

	TODO:

	- add HRTC/VRTC output to i8275
	- NEC uPD7220 GDC
	- accurate video timing
	- floppy DRQ during RECALL = 0
	- PCB layout
	- NEC uPD7201 MPSC

*/

/*

	Nokia Elektroniikka pj

	Controller ILC 9534
	FDC-Interface ILC 9530

	Parts:

	6,144 MHz xtal (CPU clock)
	18,720 MHz xtal (pixel clock)
	16 MHz xtal (FDC clock)
	Intel 8085AP (CPU)
	Intel 8253-5P (PIT)
	Intel 8275P (CRTC)
	Intel 8212P (I/OP)
	Intel 8237A-5P (DMAC)
	NEC uPD7220C (GDC)
	NEC uPD7201P (MPSC=uart)
	NEC uPD765 (FDC)
	TMS4116-15 (16Kx4 DRAM)*4 = 32KB Video RAM for 7220
	2164-6P (64Kx1 DRAM)*8 = 64KB Work RAM

	DMA channels:

	0	CRT
	1	MPSC transmit
	2	MPSC receive
	3	FDC

	Interrupts:

	INTR	MPSC INT
	RST5.5	FDC IRQ
	RST6.5	8212 INT
	RST7.5	DMA EOP

*/

/* Read/Write Handlers */

static WRITE8_HANDLER( ls259_w )
{
	mm1_state *state = space->machine->driver_data;
	const address_space *program = cputag_get_address_space(space->machine, I8085A_TAG, ADDRESS_SPACE_PROGRAM);
	int d = BIT(data, 0);

	switch (offset)
	{
	case 0: /* IC24 A8 */
		//logerror("IC24 A8 %u\n", d);
		memory_set_bank(space->machine, "bank1", d);

		if (d)
			memory_install_readwrite_bank(program, 0x0000, 0x0fff, 0, 0, "bank1");
		else
			memory_install_read_bank(program, 0x0000, 0x0fff, 0, 0, "bank1");
			memory_unmap_write(program, 0x0000, 0x0fff, 0, 0);
		break;

	case 1: /* RECALL */
		//logerror("RECALL %u\n", d);
		state->recall = d;
		upd765_reset_w(state->upd765, d);
		break;

	case 2: /* _RV28/RX21 */
		state->rx21 = d;
		break;

	case 3: /* _TX21 */
		state->tx21 = d;
		break;

	case 4: /* _RCL */
		state->rcl = d;
		break;

	case 5: /* _INTC */
		state->intc = d;
		break;

	case 6: /* LLEN */
		//logerror("LLEN %u\n", d);
		state->llen = d;
		break;

	case 7: /* MOTOR ON */
		//logerror("MOTOR %u\n", d);
		floppy_drive_set_motor_state(floppy_get_device(space->machine, 0), d);
		floppy_drive_set_ready_state(floppy_get_device(space->machine, 0), d, 1);
		floppy_drive_set_motor_state(floppy_get_device(space->machine, 1), d);
		floppy_drive_set_ready_state(floppy_get_device(space->machine, 1), d, 1);

		if (input_port_read(space->machine, "T5")) upd765_ready_w(state->upd765, d);
		break;
	}
}

/* Memory Maps */

static ADDRESS_MAP_START( mm1_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_RAMBANK("bank1")
	AM_RANGE(0x1000, 0xfeff) AM_RAM
	AM_RANGE(0xff00, 0xff0f) AM_MIRROR(0x80) AM_DEVREADWRITE(I8237_TAG, i8237_r, i8237_w)
//	AM_RANGE(0xff10, 0xff13) AM_MIRROR(0x8c) AM_DEVREADWRITE(UPD7201_TAG, upd7201_cd_ba_r, upd7201_cd_ba_w)
    AM_RANGE(0xff20, 0xff21) AM_MIRROR(0x8e) AM_DEVREADWRITE(I8275_TAG, i8275_r, i8275_w)
	AM_RANGE(0xff30, 0xff33) AM_MIRROR(0x8c) AM_DEVREADWRITE(I8253_TAG, pit8253_r, pit8253_w)
	AM_RANGE(0xff40, 0xff40) AM_MIRROR(0x8f) AM_DEVREADWRITE(I8212_TAG, i8212_r, i8212_w)
	AM_RANGE(0xff50, 0xff50) AM_MIRROR(0x8e) AM_DEVREAD(UPD765_TAG, upd765_status_r)
	AM_RANGE(0xff51, 0xff51) AM_MIRROR(0x8e) AM_DEVREADWRITE(UPD765_TAG, upd765_data_r, upd765_data_w)
	AM_RANGE(0xff60, 0xff67) AM_MIRROR(0x88) AM_WRITE(ls259_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mm1m6_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_IMPORT_FROM(mm1_map)
	AM_RANGE(0xff70, 0xff71) AM_MIRROR(0x8e) AM_DEVREADWRITE(UPD7220_TAG, upd7220_r, upd7220_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mm1_upd7220_map, 0, 16 )
	AM_RANGE(0x00000, 0x07fff) AM_RAM
	AM_RANGE(0x08000, 0x3ffff) AM_UNMAP // wrong
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( mm1 )
	PORT_START("ROW0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('\"')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F1") PORT_CODE(KEYCODE_F1) PORT_CHAR(UCHAR_MAMEKEY(F1))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("TAB") PORT_CODE(KEYCODE_TAB) PORT_CHAR('\t')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F2") PORT_CODE(KEYCODE_F2) PORT_CHAR(UCHAR_MAMEKEY(F2))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')

	PORT_START("ROW1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F3") PORT_CODE(KEYCODE_F3) PORT_CHAR(UCHAR_MAMEKEY(F3))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F4") PORT_CODE(KEYCODE_F4) PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')

	PORT_START("ROW2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F5") PORT_CODE(KEYCODE_F5) PORT_CHAR(UCHAR_MAMEKEY(F5))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F6") PORT_CODE(KEYCODE_F6) PORT_CHAR(UCHAR_MAMEKEY(F6))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')

	PORT_START("ROW3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('/')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F7") PORT_CODE(KEYCODE_F7) PORT_CHAR(UCHAR_MAMEKEY(F7))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR(';')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F8") PORT_CODE(KEYCODE_F8) PORT_CHAR(UCHAR_MAMEKEY(F8))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')

	PORT_START("ROW4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')

	PORT_START("ROW5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('+') PORT_CHAR('?')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xC2\xB4 '") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR(0x00B4) PORT_CHAR('`')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x96")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR(':')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC) PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('-') PORT_CHAR('_')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xC3\xA5 \xC3\x85") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR(0x00E5) PORT_CHAR(0x00C5)

	PORT_START("ROW6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH2) PORT_CHAR('<') PORT_CHAR('>')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad .") PORT_CODE(KEYCODE_DEL_PAD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CODE(KEYCODE_ENTER_PAD) PORT_CHAR(13)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR('~') PORT_CHAR('^')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x93") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("LF")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('@') PORT_CHAR('*')

	PORT_START("ROW7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 8") PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 9") PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x92") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 2") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 6") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("DEL") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 3") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 5") PORT_CODE(KEYCODE_5_PAD)

	PORT_START("ROW8")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CAPS LOCK") PORT_CODE(KEYCODE_CAPSLOCK)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 7") PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x91") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xC3\xA4 \xC3\x84") PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(0x00E4) PORT_CHAR(0x00C4)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 4") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 1") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 0") PORT_CODE(KEYCODE_0_PAD)

	PORT_START("ROW9")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR('=')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F9") PORT_CODE(KEYCODE_F9) PORT_CHAR(UCHAR_MAMEKEY(F9))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xC3\xB6 \xC3\x96") PORT_CODE(KEYCODE_COLON) PORT_CHAR(0x00F6) PORT_CHAR(0x00D6)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F10") PORT_CODE(KEYCODE_F10) PORT_CHAR(UCHAR_MAMEKEY(F10))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')

	PORT_START("SPECIAL")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Left SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Right SHIFT") PORT_CODE(KEYCODE_RSHIFT)

	PORT_START("T5")
	PORT_CONFNAME( 0x01, 0x00, "Floppy Drive Type")
	PORT_CONFSETTING( 0x00, "640 KB" )
	PORT_CONFSETTING( 0x01, "160/320 KB" )
INPUT_PORTS_END

/* Video */

static PALETTE_INIT( mm1 )
{
	palette_set_color(machine, 0, RGB_BLACK); /* black */
	palette_set_color_rgb(machine, 1, 0x00, 0xc0, 0x00); /* green */
	palette_set_color_rgb(machine, 2, 0x00, 0xff, 0x00); /* bright green */
}

static I8275_DISPLAY_PIXELS( crtc_display_pixels )
{
	mm1_state *state = device->machine->driver_data;

	UINT8 romdata = state->char_rom[(charcode << 4) | linecount];

	int d0 = BIT(romdata, 0);
	int d7 = BIT(romdata, 7);
	int gpa0 = BIT(gpa, 0);
	int llen = state->llen;
	int i;

	UINT8 data = (romdata << 1) | (d7 & d0);

	for (i = 0; i < 8; i++)
	{
		int qh = BIT(data, i);
		int video_in = ((((d7 & llen) | !vsp) & !gpa0) & qh) | lten;
		int compl_in = rvv;
		int hlt_in = hlgt;

		int color = hlt_in ? 2 : (video_in ^ compl_in);

		*BITMAP_ADDR16(device->machine->generic.tmpbitmap, y, x + i) = color;
	}
}

static const i8275_interface mm1_i8275_intf =
{
	SCREEN_TAG,
	8,
	0,
	DEVCB_DEVICE_LINE(I8237_TAG, i8237_dreq0_w),
	DEVCB_NULL,
	crtc_display_pixels
};

static UPD7220_DISPLAY_PIXELS( hgdc_display_pixels )
{
	int i;

	for (i = 0; i < 16; i++)
	{
		if (BIT(data, i)) *BITMAP_ADDR16(bitmap, y, x + i) = 1;
	}
}

static UPD7220_INTERFACE( mm1_upd7220_intf )
{
	SCREEN_TAG,
	hgdc_display_pixels,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

static VIDEO_START( mm1 )
{
	mm1_state *state = machine->driver_data;

	/* find memory regions */
	state->char_rom = memory_region(machine, "chargen");

	VIDEO_START_CALL(generic_bitmapped);
}

static VIDEO_UPDATE( mm1 )
{
	mm1_state *state = screen->machine->driver_data;

	/* text */
	i8275_update(state->i8275, bitmap, cliprect);
	copybitmap(bitmap, screen->machine->generic.tmpbitmap, 0, 0, 0, 0, cliprect);

	/* graphics */
	upd7220_update(state->upd7220, bitmap, cliprect);

	return 0;
}

static const gfx_layout tiles8x16_layout =
{
	8, 16,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{  0*8,  1*8,  2*8,  3*8,  4*8,  5*8,  6*8,  7*8,
	   8*8,  9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	8*16
};

static GFXDECODE_START( mm1 )
	GFXDECODE_ENTRY( "chargen", 0, tiles8x16_layout, 0, 0x100 )
GFXDECODE_END

/* 8212 Interface */

static READ8_DEVICE_HANDLER( kb_r )
{
	mm1_state *state = device->machine->driver_data;

	return state->keydata;
}

static I8212_INTERFACE( mm1_i8212_intf )
{
	DEVCB_CPU_INPUT_LINE(I8085A_TAG, I8085_RST65_LINE),
	DEVCB_HANDLER(kb_r),
	DEVCB_NULL
};

/* 8237 Interface */

static WRITE_LINE_DEVICE_HANDLER( dma_hrq_changed )
{
	cputag_set_input_line(device->machine, I8085A_TAG, INPUT_LINE_HALT, state ? ASSERT_LINE : CLEAR_LINE);

	/* Assert HLDA */
	i8237_hlda_w(device, state);
}

static READ8_DEVICE_HANDLER( mpsc_dack_r )
{
	mm1_state *state = device->machine->driver_data;

	/* clear data request */
	i8237_dreq2_w(state->i8237, CLEAR_LINE);

	return upd7201_dtra_r(device);
}

static WRITE8_DEVICE_HANDLER( mpsc_dack_w )
{
	mm1_state *state = device->machine->driver_data;

	upd7201_hai_w(device, data);

	/* clear data request */
	i8237_dreq1_w(state->i8237, CLEAR_LINE);
}

static WRITE_LINE_DEVICE_HANDLER( tc_w )
{
	mm1_state *driver_state = device->machine->driver_data;

	if (!driver_state->dack3)
	{
		/* floppy terminal count */
		upd765_tc_w(driver_state->upd765, !state);
	}

	driver_state->tc = !state;

	cputag_set_input_line(device->machine, I8085A_TAG, I8085_RST75_LINE, state);
}

static WRITE_LINE_DEVICE_HANDLER( dack3_w )
{
	mm1_state *driver_state = device->machine->driver_data;

	driver_state->dack3 = state;

	if (!driver_state->dack3)
	{
		/* floppy terminal count */
		upd765_tc_w(driver_state->upd765, driver_state->tc);
	}
}

static I8237_INTERFACE( mm1_dma8237_intf )
{
	DEVCB_LINE(dma_hrq_changed),
	DEVCB_LINE(tc_w),
	DEVCB_MEMORY_HANDLER(I8085A_TAG, PROGRAM, memory_read_byte),
	DEVCB_MEMORY_HANDLER(I8085A_TAG, PROGRAM, memory_write_byte),
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_DEVICE_HANDLER(UPD7201_TAG, mpsc_dack_r), DEVCB_DEVICE_HANDLER(UPD765_TAG, upd765_dack_r) },
	{ DEVCB_DEVICE_HANDLER(I8275_TAG, i8275_dack_w), DEVCB_DEVICE_HANDLER(UPD7201_TAG, mpsc_dack_w), DEVCB_NULL, DEVCB_DEVICE_HANDLER(UPD765_TAG, upd765_dack_w) },
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_LINE(dack3_w) }
};

/* uPD765 Interface */

static UPD765_DMA_REQUEST( drq_w )
{
	mm1_state *driver_state = device->machine->driver_data;

	i8237_dreq3_w(driver_state->i8237, state);
}

static const upd765_interface mm1_upd765_intf =
{
	DEVCB_CPU_INPUT_LINE(I8085A_TAG, I8085_RST55_LINE),
	drq_w,
	NULL,
	UPD765_RDY_PIN_NOT_CONNECTED,
	{ FLOPPY_0, FLOPPY_1, NULL, NULL }
};

/* 8253 Interface */

static PIT8253_OUTPUT_CHANGED( itxc_w )
{
	mm1_state *driver_state = device->machine->driver_data;

	if (!driver_state->intc)
	{
		upd7201_txca_w(driver_state->upd7201, state);
	}
}

static PIT8253_OUTPUT_CHANGED( irxc_w )
{
	mm1_state *driver_state = device->machine->driver_data;

	if (!driver_state->intc)
	{
		upd7201_rxca_w(driver_state->upd7201, state);
	}
}

static PIT8253_OUTPUT_CHANGED( auxc_w )
{
	mm1_state *driver_state = device->machine->driver_data;

	upd7201_txcb_w(driver_state->upd7201, state);
	upd7201_rxcb_w(driver_state->upd7201, state);
}

static const struct pit8253_config mm1_pit8253_intf =
{
	{
		{
			XTAL_6_144MHz/2/2,
			itxc_w
		}, {
			XTAL_6_144MHz/2/2,
			irxc_w
		}, {
			XTAL_6_144MHz/2/2,
			auxc_w
		}
	}
};

/* uPD7201 Interface */

static WRITE_LINE_DEVICE_HANDLER( drq2_w )
{
	mm1_state *driver_state = device->machine->driver_data;

	if (state) i8237_dreq2_w(driver_state->i8237, ASSERT_LINE);
}

static WRITE_LINE_DEVICE_HANDLER( drq1_w )
{
	mm1_state *driver_state = device->machine->driver_data;

	if (state) i8237_dreq1_w(driver_state->i8237, ASSERT_LINE);
}

static UPD7201_INTERFACE( mm1_upd7201_intf )
{
	DEVCB_NULL,					/* interrupt */
	{
		{
			0,					/* receive clock */
			0,					/* transmit clock */
			DEVCB_LINE(drq2_w),	/* receive DRQ */
			DEVCB_LINE(drq1_w),	/* transmit DRQ */
			DEVCB_NULL,			/* receive data */
			DEVCB_NULL,			/* transmit data */
			DEVCB_NULL,			/* clear to send */
			DEVCB_NULL,			/* data carrier detect */
			DEVCB_NULL,			/* ready to send */
			DEVCB_NULL,			/* data terminal ready */
			DEVCB_NULL,			/* wait */
			DEVCB_NULL			/* sync output */
		}, {
			0,					/* receive clock */
			0,					/* transmit clock */
			DEVCB_NULL,			/* receive DRQ */
			DEVCB_NULL,			/* transmit DRQ */
			DEVCB_NULL,			/* receive data */
			DEVCB_NULL,			/* transmit data */
			DEVCB_NULL,			/* clear to send */
			DEVCB_LINE_GND,		/* data carrier detect */
			DEVCB_NULL,			/* ready to send */
			DEVCB_NULL,			/* data terminal ready */
			DEVCB_NULL,			/* wait */
			DEVCB_NULL			/* sync output */
		}
	}
};

/* 8085A Interface */

static READ_LINE_DEVICE_HANDLER( dsra_r )
{
	return 1;
}

static I8085_CONFIG( mm1_i8085_config )
{
	DEVCB_NULL,			/* STATUS changed callback */
	DEVCB_NULL,			/* INTE changed callback */
	DEVCB_LINE(dsra_r),	/* SID changed callback (8085A only) */
	DEVCB_DEVICE_LINE(SPEAKER_TAG, speaker_level_w)	/* SOD changed callback (8085A only) */
};

/* Keyboard */

static TIMER_DEVICE_CALLBACK( kbclk_tick )
{
	mm1_state *state = timer->machine->driver_data;
	static const char *const keynames[] = { "ROW0", "ROW1", "ROW2", "ROW3", "ROW4", "ROW5", "ROW6", "ROW7", "ROW8", "ROW9" };

	UINT8 data = input_port_read(timer->machine, keynames[state->drive]);
	UINT8 special = input_port_read(timer->machine, "SPECIAL");
	int ctrl = BIT(special, 0);
	int shift = BIT(special, 2) & BIT(special, 1);
	UINT8 keydata = 0xff;

	if (!BIT(data, state->sense))
	{
		/* get key data from PROM */
		keydata = state->key_rom[(ctrl << 8) | (shift << 7) | (state->drive << 3) | (state->sense)];
	}

	if (state->keydata != keydata)
	{
		/* latch key data */
		state->keydata = keydata;

		if (keydata != 0xff)
		{
			/* strobe in key data */
			i8212_stb_w(state->i8212, 1);
			i8212_stb_w(state->i8212, 0);
		}
	}

	if (keydata == 0xff)
	{
		/* increase scan counters */
		state->sense++;

		if (state->sense == 8)
		{
			state->sense = 0;
			state->drive++;

			if (state->drive == 10)
			{
				state->drive = 0;
			}
		}
	}
}

/* Floppy Configuration */

static FLOPPY_OPTIONS_START( mm1 )
	FLOPPY_OPTION( mm1_640kb, "dsk", "Nokia MikroMikko 1 640KB disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([80])
		SECTORS([8]) /* 3:1 sector skew (1,4,7,2,5,8,3,6) */
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

static FLOPPY_OPTIONS_START( mm2 )
	FLOPPY_OPTION( mm2_360kb, "dsk", "Nokia MikroMikko 2 360KB disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([40])
		SECTORS([9])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([1]))

	FLOPPY_OPTION( mm2_720kb, "dsk", "Nokia MikroMikko 2 720KB disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([40])
		SECTORS([18])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

static const floppy_config mm1_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(mm1),
	DO_NOT_KEEP_GEOMETRY
};

/* Machine Initialization */

static MACHINE_START( mm1 )
{
	mm1_state *state = machine->driver_data;
	const address_space *program = cputag_get_address_space(machine, I8085A_TAG, ADDRESS_SPACE_PROGRAM);

	/* look up devices */
	state->i8212 = devtag_get_device(machine, I8212_TAG);
	state->i8237 = devtag_get_device(machine, I8237_TAG);
	state->i8275 = devtag_get_device(machine, I8275_TAG);
	state->upd765 = devtag_get_device(machine, UPD765_TAG);
	state->upd7201 = devtag_get_device(machine, UPD7201_TAG);
	state->upd7220 = devtag_get_device(machine, UPD7220_TAG);
	state->speaker = devtag_get_device(machine, SPEAKER_TAG);

	/* find memory regions */
	state->key_rom = memory_region(machine, "keyboard");

	/* setup memory banking */
	memory_install_read_bank(program, 0x0000, 0x0fff, 0, 0, "bank1");
	memory_unmap_write(program, 0x0000, 0x0fff, 0, 0);
	memory_configure_bank(machine, "bank1", 0, 1, memory_region(machine, "bios"), 0);
	memory_configure_bank(machine, "bank1", 1, 1, messram_get_ptr(devtag_get_device(machine, "messram")), 0);
	memory_set_bank(machine, "bank1", 0);

	/* register for state saving */
	state_save_register_global(machine, state->sense);
	state_save_register_global(machine, state->drive);
	state_save_register_global(machine, state->llen);
	state_save_register_global(machine, state->intc);
	state_save_register_global(machine, state->rx21);
	state_save_register_global(machine, state->tx21);
	state_save_register_global(machine, state->rcl);
	state_save_register_global(machine, state->recall);
	state_save_register_global(machine, state->dack3);
}

static MACHINE_RESET( mm1 )
{
	mm1_state *state = machine->driver_data;
	const address_space *program = cputag_get_address_space(machine, I8085A_TAG, ADDRESS_SPACE_PROGRAM);
	int i;

	/* reset LS259 */
	for (i = 0; i < 8; i++) ls259_w(program, i, 0);

	/* set FDC ready */
	if (!input_port_read(machine, "T5")) upd765_ready_w(state->upd765, 1);

	/* reset FDC */
	upd765_reset_w(state->upd765, 1);
	upd765_reset_w(state->upd765, 0);
}

/* Machine Drivers */

static MACHINE_DRIVER_START( mm1 )
	MDRV_DRIVER_DATA(mm1_state)

	/* basic system hardware */
	MDRV_CPU_ADD(I8085A_TAG, 8085A, XTAL_6_144MHz)
	MDRV_CPU_PROGRAM_MAP(mm1_map)
	MDRV_CPU_CONFIG(mm1_i8085_config)

	MDRV_MACHINE_START(mm1)
	MDRV_MACHINE_RESET(mm1)

	MDRV_TIMER_ADD_PERIODIC("kbclk", kbclk_tick, HZ(2500)) //HZ(XTAL_6_144MHz/2/8))

	/* video hardware */
	MDRV_SCREEN_ADD( SCREEN_TAG, RASTER )
	MDRV_SCREEN_REFRESH_RATE( 50 )
	MDRV_SCREEN_FORMAT( BITMAP_FORMAT_INDEXED16 )
	MDRV_SCREEN_SIZE( 800, 400 )
	MDRV_SCREEN_VISIBLE_AREA( 0, 800-1, 0, 400-1 )
	//MDRV_SCREEN_RAW_PARAMS(XTAL_18_720MHz, ...)

	MDRV_GFXDECODE(mm1)
	MDRV_PALETTE_LENGTH(3)
	MDRV_PALETTE_INIT(mm1)

	MDRV_VIDEO_START(mm1)
	MDRV_VIDEO_UPDATE(mm1)

	MDRV_I8275_ADD(I8275_TAG, mm1_i8275_intf)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SPEAKER_TAG, SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* peripheral hardware */
	MDRV_I8212_ADD(I8212_TAG, mm1_i8212_intf)
	MDRV_I8237_ADD(I8237_TAG, XTAL_6_144MHz/2, mm1_dma8237_intf)
	MDRV_PIT8253_ADD(I8253_TAG, mm1_pit8253_intf)
	MDRV_UPD765A_ADD(UPD765_TAG, /* XTAL_16MHz/2/2, */ mm1_upd765_intf)
	MDRV_UPD7201_ADD(UPD7201_TAG, XTAL_6_144MHz/2, mm1_upd7201_intf)

	MDRV_FLOPPY_2_DRIVES_ADD(mm1_floppy_config)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("64K")
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mm1m6 )
	MDRV_IMPORT_FROM(mm1)

	/* basic system hardware */
	MDRV_CPU_MODIFY(I8085A_TAG)
	MDRV_CPU_PROGRAM_MAP(mm1m6_map)

	/* video hardware */
	MDRV_UPD7220_ADD(UPD7220_TAG, XTAL_18_720MHz/8, mm1_upd7220_intf, mm1_upd7220_map)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( mm1m6 )
	ROM_REGION( 0x4000, "bios", 0 ) /* BIOS */
	ROM_LOAD( "9081b.ic2", 0x0000, 0x2000, CRC(2955feb3) SHA1(946a6b0b8fb898be3f480c04da33d7aaa781152b) )

	/*

        Address Decoder PROM outputs

        D0      IOEN/_MEMEN
        D1      RAMEN
        D2      not connected
        D3      _CE4
        D4      _CE0
        D5      _CE1
        D6      _CE2
        D7      _CE3

    */
	ROM_REGION( 0x200, "address", 0 ) /* address decoder */
	ROM_LOAD( "720793a.ic24", 0x0000, 0x0200, CRC(deea87a6) SHA1(8f19e43252c9a0b1befd02fc9d34fe1437477f3a) )

	ROM_REGION( 0x200, "keyboard", 0 ) /* keyboard encoder */
	ROM_LOAD( "mmi6349-1j.bin", 0x0000, 0x0200, CRC(4ab3bf03) SHA1(925c9ee22db13566416cdbc505c03d4116ff8d5f) )

	ROM_REGION( 0x1000, "chargen", 0 ) /* character generator */
	ROM_LOAD( "6807b.ic61", 0x0000, 0x1000, CRC(32b36220) SHA1(8fe7a181badea3f7e656dfaea21ee9e4c9baf0f1) )
ROM_END

#define rom_mm1m7 rom_mm1m6

/* System Drivers */

//    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT       INIT    CONFIG    COMPANY           FULLNAME                FLAGS
COMP( 1981, mm1m6,		0,		0,		mm1m6,		mm1,		0, 		0,	  "Nokia Data",		"MikroMikko 1 M6",		GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
COMP( 1981, mm1m7,		mm1m6,	0,		mm1m6,		mm1,		0, 		0,	  "Nokia Data",		"MikroMikko 1 M7",		GAME_NOT_WORKING )
