#include "driver.h"
#include "deprecat.h"
#include "devices/basicdsk.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "devices/printer.h"
#include "machine/centroni.h"
#include "includes/serial.h"
#include "machine/msm8251.h"
#include "machine/8255ppi.h"
#include "machine/nec765.h"
#include "sound/sn76496.h"
#include "video/tms9928a.h"

/*

    TODO:

    - SG-1000 pause button (NMI vector 0x66)
    - SC-3000 reset key
    - SC-3000 cassette
    - SF-7000 serial comms
    - SP-400 serial printer

*/

/* Terebi Oekaki (TV Draw) */

/*

    Address Access  Bits
                    7       6   5   4   3   2   1   0
    $6000   W       -       -   -   -   -   -   -   AXIS
    $8000   R       BUSY    -   -   -   -   -   -   PRESS
    $A000   R/W     DATA

    AXIS: write 0 to select X axis, 1 to select Y axis.
    BUSY: reads 1 when graphic board is busy sampling position, else 0.
    PRESS: reads 0 when pen is touching graphic board, else 1.
    DATA: when pen is touching graphic board, return 8-bit sample position for currently selected axis (X is in the 0-255 range, Y in the 0-191 range). Else, return 0.

*/

static int tvdraw_data;

static WRITE8_HANDLER( tvdraw_axis_w )
{
	if (data & 0x01)
	{
		tvdraw_data = readinputportbytag("TVDRAW_X");

		if (tvdraw_data < 4) tvdraw_data = 4;
		if (tvdraw_data > 251) tvdraw_data = 251;
	}
	else
	{
		tvdraw_data = readinputportbytag("TVDRAW_Y") + 32;
	}
}

static READ8_HANDLER( tvdraw_status_r )
{
	return readinputportbytag("TVDRAW_PEN");
}

static READ8_HANDLER( tvdraw_data_r )
{
	return tvdraw_data;
}

static READ8_HANDLER( sg1000_joysel_r )
{
	return 0x80;
}

/* Memory Maps */

// SG-1000

static ADDRESS_MAP_START( sg1000_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_RAMBANK(1)
	AM_RANGE(0xc000, 0xffff) AM_RAMBANK(2)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sg1000_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x7f, 0x7f) AM_WRITE(SN76496_0_w)
	AM_RANGE(0xbe, 0xbe) AM_READWRITE(TMS9928A_vram_r, TMS9928A_vram_w)
	AM_RANGE(0xbf, 0xbf) AM_READWRITE(TMS9928A_register_r, TMS9928A_register_w)
	AM_RANGE(0xdc, 0xdc) AM_READ(input_port_0_r)
	AM_RANGE(0xdd, 0xdd) AM_READ(input_port_1_r)
	AM_RANGE(0xde, 0xde) AM_READ(sg1000_joysel_r) AM_WRITENOP
	AM_RANGE(0xdf, 0xdf) AM_NOP
ADDRESS_MAP_END

// SC-3000

static ADDRESS_MAP_START( sc3000_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_RAMBANK(1)
	AM_RANGE(0xc000, 0xffff) AM_RAMBANK(2)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sc3000_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x7f, 0x7f) AM_WRITE(SN76496_0_w)
	AM_RANGE(0xbe, 0xbe) AM_READWRITE(TMS9928A_vram_r, TMS9928A_vram_w)
	AM_RANGE(0xbf, 0xbf) AM_READWRITE(TMS9928A_register_r, TMS9928A_register_w)
	AM_RANGE(0xdc, 0xdf) AM_READWRITE(ppi8255_0_r, ppi8255_0_w)
ADDRESS_MAP_END

// SF-7000

static ADDRESS_MAP_START( sf7000_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READWRITE(MRA8_BANK1, MWA8_BANK2)
	AM_RANGE(0x4000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sf7000_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x7f, 0x7f) AM_WRITE(SN76496_0_w)
	AM_RANGE(0xbe, 0xbe) AM_READWRITE(TMS9928A_vram_r, TMS9928A_vram_w)
	AM_RANGE(0xbf, 0xbf) AM_READWRITE(TMS9928A_register_r, TMS9928A_register_w)
	AM_RANGE(0xdc, 0xdf) AM_READWRITE(ppi8255_0_r, ppi8255_0_w)
	AM_RANGE(0xe0, 0xe0) AM_READ(nec765_status_r)
	AM_RANGE(0xe1, 0xe1) AM_READWRITE(nec765_data_r, nec765_data_w)
	AM_RANGE(0xe4, 0xe7) AM_READWRITE(ppi8255_1_r, ppi8255_1_w)
	AM_RANGE(0xe8, 0xe8) AM_READWRITE(msm8251_data_r, msm8251_data_w)
	AM_RANGE(0xe9, 0xe9) AM_READWRITE(msm8251_status_r, msm8251_control_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( tvdraw )
	PORT_START_TAG("TVDRAW_X")
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_CROSSHAIR(X, 1.0, 0.0, 0) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START_TAG("TVDRAW_Y")
	PORT_BIT( 0xff, 0x60, IPT_LIGHTGUN_Y ) PORT_CROSSHAIR(Y, 1.0, 0.0, 0) PORT_MINMAX(0, 191) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START_TAG("TVDRAW_PEN")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Pen")
INPUT_PORTS_END

static INPUT_PORTS_START( sg1000 )
	PORT_START_TAG("PA7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)

	PORT_START_TAG("PB7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PAUSE")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START ) PORT_NAME("PAUSE")

	PORT_INCLUDE( tvdraw )
INPUT_PORTS_END

static INPUT_PORTS_START( sk1100 )
	PORT_START_TAG("PA0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("ENG DIER'S") PORT_CODE(KEYCODE_RALT) PORT_CHAR(UCHAR_MAMEKEY(RALT))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('I')

	PORT_START_TAG("PA1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SPC") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('O')

	PORT_START_TAG("PA2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("HOME CLR") PORT_CODE(KEYCODE_HOME) PORT_CHAR(UCHAR_MAMEKEY(HOME))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('P')

	PORT_START_TAG("PA3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("INS DEL") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PI") PORT_CODE(KEYCODE_EQUALS)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('@') PORT_CHAR('`')

	PORT_START_TAG("PA4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("DOWN") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR('[') PORT_CHAR('{')

	PORT_START_TAG("PA5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("LEFT") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CR") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PA6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RIGHT") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("UP") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PA7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)

	PORT_START_TAG("PB0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0x07, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PB1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x07, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PB2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT( 0x07, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PB3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT( 0x07, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PB4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH2) PORT_CHAR('^')
	PORT_BIT( 0x07, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PB5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("YEN") PORT_CODE(KEYCODE_TILDE)
	PORT_BIT( 0x06, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("FUNC") PORT_CODE(KEYCODE_TAB)

	PORT_START_TAG("PB6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("BREAK") PORT_CODE(KEYCODE_ESC) PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("GRAPH") PORT_CODE(KEYCODE_LALT) PORT_CHAR(UCHAR_MAMEKEY(LALT))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)

	PORT_START_TAG("PB7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)

	PORT_START_TAG("RESET")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RESET") PORT_CODE(KEYCODE_F10)
INPUT_PORTS_END

static INPUT_PORTS_START( sc3000 )
	PORT_INCLUDE( sk1100 )
	PORT_INCLUDE( tvdraw )
INPUT_PORTS_END

static INPUT_PORTS_START( sf7000 )
	PORT_INCLUDE( sk1100 )

	PORT_START_TAG("BAUD")
	PORT_CONFNAME( 0x05, 0x05, "Baud rate")
	PORT_CONFSETTING( 0x00, "9600 baud" )
	PORT_CONFSETTING( 0x01, "4800 baud" )
	PORT_CONFSETTING( 0x02, "2400 baud" )
	PORT_CONFSETTING( 0x03, "1200 baud" )
	PORT_CONFSETTING( 0x04, "600 baud" )
	PORT_CONFSETTING( 0x05, "300 baud" )
INPUT_PORTS_END

/* Machine Interfaces */

// SG-1000

static INTERRUPT_GEN( sg1000_int )
{
    TMS9928A_interrupt();
}

static void sg1000_vdp_interrupt(int state)
{
	cpunum_set_input_line_and_vector(Machine, 0, INPUT_LINE_IRQ0, state, 0x38);
}

static const TMS9928a_interface tms9928a_interface =
{
	TMS99x8A,
	0x4000,
	0, 0,
	sg1000_vdp_interrupt
};

static MACHINE_START( sg1000 )
{
	TMS9928A_configure(&tms9928a_interface);
}

// SC-3000

static int keylatch;

static READ8_HANDLER( sc3000_ppi8255_a_r )
{
	/*
        Signal  Description

        PA0     Keyboard input
        PA1     Keyboard input
        PA2     Keyboard input
        PA3     Keyboard input
        PA4     Keyboard input
        PA5     Keyboard input
        PA6     Keyboard input
        PA7     Keyboard input
    */

	return readinputport(keylatch);
}

static READ8_HANDLER( sc3000_ppi8255_b_r )
{
	/*
        Signal  Description

        PB0     Keyboard input
        PB1     Keyboard input
        PB2     Keyboard input
        PB3     Keyboard input
        PB4     /CONT input from cartridge terminal B-11
        PB5     FAULT input from printer
        PB6     BUSY input from printer
        PB7     Cassette tape input
    */

	return readinputport(keylatch + 8) | 0x70;
}

static WRITE8_HANDLER( sc3000_ppi8255_c_w )
{
	/*
        Signal  Description

        PC0     Keyboard raster output
        PC1     Keyboard raster output
        PC2     Keyboard raster output
        PC3     not connected
        PC4     Cassette tape output
        PC5     DATA to printer
        PC6     /RESET to printer
        PC7     /FEED to printer
    */

	keylatch = data & 0x07;
}

static const ppi8255_interface sc3000_ppi8255_intf =
{
	1, 						// 1 chip
	{ sc3000_ppi8255_a_r },	// Port A read
	{ sc3000_ppi8255_b_r },	// Port B read
	{ NULL },				// Port C read
	{ NULL },				// Port A write
	{ NULL },				// Port B write
	{ sc3000_ppi8255_c_w },	// Port C write
};

static MACHINE_START( sc3000 )
{
	TMS9928A_configure(&tms9928a_interface);
	ppi8255_init(&sc3000_ppi8255_intf);
}

// SF-7000

static int sf7000_fdc_int;
static int sf7000_fdc_index;

static READ8_HANDLER( sf7000_ppi8255_a_r )
{
	/*
        Signal  Description

        PA0     INT from FDC
        PA1     BUSY from Centronics printer
        PA2     INDEX from FDD
        PA3
        PA4
        PA5
        PA6
        PA7
    */

	int centronics_handshake = centronics_read_handshake(1);
	int busy = 0;

	if ((centronics_handshake & CENTRONICS_NOT_BUSY) == 0)
	{
		busy = 0x02;
	}

	return (sf7000_fdc_index << 2) | busy | sf7000_fdc_int;
}

static WRITE8_HANDLER( sf7000_ppi8255_b_w )
{
	/*
        Signal  Description

        PB0     Data output to Centronics printer
        PB1     Data output to Centronics printer
        PB2     Data output to Centronics printer
        PB3     Data output to Centronics printer
        PB4     Data output to Centronics printer
        PB5     Data output to Centronics printer
        PB6     Data output to Centronics printer
        PB7     Data output to Centronics printer
    */

	centronics_write_data(1, data);
}

static WRITE8_HANDLER( sf7000_ppi8255_c_w )
{
	/*
        Signal  Description

        PC0     /INUSE signal to FDD
        PC1     /MOTOR ON signal to FDD
        PC2     TC signal to FDC
        PC3     RESET signal to FDC
        PC4     not connected
        PC5     not connected
        PC6     /ROM SEL (switch between IPL ROM and RAM)
        PC7     /STROBE to Centronics printer
    */

	floppy_drive_set_motor_state(image_from_devtype_and_index(IO_FLOPPY, 0), (data & 0x02) ? 0 : 1);
	floppy_drive_set_ready_state(image_from_devtype_and_index(IO_FLOPPY, 0), 1, 0);

	nec765_set_tc_state(data & 0x04);

	if (data & 0x08)
	{
		nec765_reset(0);
	}

	memory_set_bank(1, (data & 0x40) >> 6);

	centronics_write_handshake(1, (data & 0x80) ? 0 : CENTRONICS_STROBE, CENTRONICS_STROBE);
}

static const ppi8255_interface sf7000_ppi8255_intf =
{
	2, 											// 2 chips
	{ sc3000_ppi8255_a_r, sf7000_ppi8255_a_r },	// Port A read
	{ sc3000_ppi8255_b_r, NULL },				// Port B read
	{ NULL, NULL },								// Port C read
	{ NULL, NULL },								// Port A write
	{ NULL, sf7000_ppi8255_b_w },				// Port B write
	{ sc3000_ppi8255_c_w, sf7000_ppi8255_c_w },	// Port C write
};

/* callback for /INT output from FDC */
static void sf7000_fdc_interrupt(int state)
{
	sf7000_fdc_int = state;
}

static void sf7000_fdc_index_callback(mess_image *img, int state)
{
	sf7000_fdc_index = state;
}

static const struct nec765_interface sf7000_nec765_interface =
{
	sf7000_fdc_interrupt,
	NULL
};

static const struct msm8251_interface sf7000_uart_interface =
{
	NULL,
	NULL,
	NULL
};

static const CENTRONICS_CONFIG sf7000_centronics_config[1] = {
	{
		PRINTER_IBM,
		NULL
	}
};

static MACHINE_START( sf7000 )
{
	TMS9928A_configure(&tms9928a_interface);
	ppi8255_init(&sf7000_ppi8255_intf);
	nec765_init(&sf7000_nec765_interface, NEC765A);
	floppy_drive_set_index_pulse_callback(image_from_devtype_and_index(IO_FLOPPY, 0), sf7000_fdc_index_callback);
	msm8251_init(&sf7000_uart_interface);
	centronics_config(1, sf7000_centronics_config);

	memory_configure_bank(1, 0, 1, memory_region(REGION_CPU1), 0);
	memory_configure_bank(1, 1, 1, mess_ram, 0);
	memory_configure_bank(2, 0, 1, mess_ram, 0);
}

static MACHINE_RESET( sf7000 )
{
	memory_set_bank(1, 0);
	memory_set_bank(2, 0);
}

/* Machine Drivers */

static MACHINE_DRIVER_START( sg1000 )
	// basic machine hardware
	MDRV_CPU_ADD(Z80, XTAL_10_738635MHz/3)
	MDRV_CPU_PROGRAM_MAP(sg1000_map, 0)
	MDRV_CPU_IO_MAP(sg1000_io_map, 0)
	MDRV_CPU_VBLANK_INT(sg1000_int, 1)

	MDRV_MACHINE_START( sg1000 )

    // video hardware
	MDRV_IMPORT_FROM(tms9928a)
	MDRV_SCREEN_MODIFY("main")
	MDRV_SCREEN_REFRESH_RATE(10738635.0/2/342/262)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SN76489, XTAL_10_738635MHz/3)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( sc3000 )
	// basic machine hardware
	MDRV_CPU_ADD(Z80, XTAL_10_738635MHz/3)
	MDRV_CPU_PROGRAM_MAP(sc3000_map, 0)
	MDRV_CPU_IO_MAP(sc3000_io_map, 0)
	MDRV_CPU_VBLANK_INT(sg1000_int, 1)

	MDRV_MACHINE_START( sc3000 )

    // video hardware
	MDRV_IMPORT_FROM(tms9928a)
	MDRV_SCREEN_MODIFY("main")
	MDRV_SCREEN_REFRESH_RATE(10738635.0/2/342/262)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SN76489, XTAL_10_738635MHz/3)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( sf7000 )
	// basic machine hardware
	MDRV_CPU_ADD(Z80, XTAL_10_738635MHz/3)
	MDRV_CPU_PROGRAM_MAP(sf7000_map, 0)
	MDRV_CPU_IO_MAP(sf7000_io_map, 0)
	MDRV_CPU_VBLANK_INT(sg1000_int, 1)

	MDRV_MACHINE_START( sf7000 )
	MDRV_MACHINE_RESET( sf7000 )

    // video hardware
	MDRV_IMPORT_FROM(tms9928a)
	MDRV_SCREEN_MODIFY("main")
	MDRV_SCREEN_REFRESH_RATE(10738635.0/2/342/262)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SN76489, XTAL_10_738635MHz/3)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( sg1000 )
    ROM_REGION( 0x10000, REGION_CPU1, ROMREGION_ERASE00 )
ROM_END

ROM_START( sg1000m2 )
    ROM_REGION( 0x10000, REGION_CPU1, ROMREGION_ERASE00 )
ROM_END

ROM_START( sc3000 )
    ROM_REGION( 0x10000, REGION_CPU1, ROMREGION_ERASE00 )
ROM_END

ROM_START( sf7000 )
    ROM_REGION( 0x10000, REGION_CPU1, 0 )
    ROM_LOAD( "ipl.rom", 0x0000, 0x2000, CRC(d76810b8) SHA1(77339a6db2593aadc638bed77b8e9bed5d9d87e3) )
ROM_END

/* System Configuration */

static void sg1000_map_cartridge_memory(UINT8 *ptr, int size)
{
	if (size == 40 * 1024)
	{
		memory_install_read8_handler (0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x9fff, 0, 0, MRA8_BANK1);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x9fff, 0, 0, MWA8_UNMAP);
		memory_configure_bank(1, 0, 1, memory_region(REGION_CPU1) + 0x8000, 0);
		memory_set_bank(1, 0);
	}
	else if (size == 48 * 1024)
	{
		memory_install_read8_handler (0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xbfff, 0, 0, MRA8_BANK1);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xbfff, 0, 0, MWA8_UNMAP);
		memory_configure_bank(1, 0, 1, memory_region(REGION_CPU1) + 0x8000, 0);
		memory_set_bank(1, 0);
	}
	else if (!strncmp("annakmn", (const char *)&ptr[0x13b3], 7))
	{
		// Terebi Oekaki

		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x6000, 0, 0, &tvdraw_axis_w);
		memory_install_read8_handler (0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x8000, 0, 0, &tvdraw_status_r);
		memory_install_read8_handler (0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xa000, 0, 0, &tvdraw_data_r);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xa000, 0, 0, MWA8_NOP);
	}
	else if (!strncmp("ASCII 1986", (const char *)&ptr[0x1cc3], 10))
	{
		// The Castle

		memory_install_read8_handler (0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x9fff, 0, 0, MRA8_BANK1);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x9fff, 0, 0, MWA8_BANK1);
	}
}

static DEVICE_LOAD( sg1000_cart )
{
	int size = image_length(image);
	UINT8 *ptr = memory_region(REGION_CPU1);

	if (image_fread(image, ptr, size ) != size)
	{
		return INIT_FAIL;
	}

	sg1000_map_cartridge_memory(ptr, size);

	memory_install_read8_handler (0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xc3ff, 0, 0x3c00, MRA8_BANK2);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xc3ff, 0, 0x3c00, MWA8_BANK2);

	return INIT_PASS;
}

static void sg1000_cartslot_getinfo( const mess_device_class *devclass, UINT32 state, union devinfo *info )
{
	switch( state )
	{
		case MESS_DEVINFO_INT_COUNT:							info->i = 1; break;
		case MESS_DEVINFO_INT_MUST_BE_LOADED:				info->i = 1; break;
		case MESS_DEVINFO_PTR_LOAD:							info->load = device_load_sg1000_cart; break;
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "sg"); break;

		default:										cartslot_device_getinfo( devclass, state, info ); break;
	}
}

static void sc3000_map_cartridge_memory(UINT8 *ptr)
{
	if (!strncmp("SC-3000 BASIC Level 3 ver 1.0", (const char *)&ptr[0x6a20], 29))
	{
		// SC-3000 BASIC Level III

		memory_install_read8_handler (0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xbfff, 0, 0, MRA8_BANK1);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xbfff, 0, 0, MWA8_BANK1);

		memory_install_read8_handler (0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xffff, 0, 0, MRA8_BANK2);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xffff, 0, 0, MWA8_BANK2);
	}
	else if (!strncmp("PIANO", (const char *)&ptr[0x0841], 5))
	{
		// Sega SC-3000 Music Editor

		memory_install_read8_handler (0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x9fff, 0, 0, MRA8_BANK1);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x9fff, 0, 0, MWA8_BANK1);

		memory_install_read8_handler (0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xc7ff, 0, 0x3800, MRA8_BANK2);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xc7ff, 0, 0x3800, MWA8_BANK2);
	}
	else
	{
		// regular cartridges

		memory_install_read8_handler (0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xc7ff, 0, 0x3800, MRA8_BANK2);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xc7ff, 0, 0x3800, MWA8_BANK2);
	}
}

static DEVICE_LOAD( sc3000_cart )
{
	int size = image_length(image);
	UINT8 *ptr = memory_region(REGION_CPU1);

	if (image_fread(image, ptr, size ) != size)
	{
		return INIT_FAIL;
	}

	sg1000_map_cartridge_memory(ptr, size);
	sc3000_map_cartridge_memory(ptr);

	return INIT_PASS;
}

static void sc3000_cartslot_getinfo( const mess_device_class *devclass, UINT32 state, union devinfo *info )
{
	switch( state )
	{
		case MESS_DEVINFO_INT_COUNT:							info->i = 1; break;
		case MESS_DEVINFO_INT_MUST_BE_LOADED:				info->i = 1; break;
		case MESS_DEVINFO_PTR_LOAD:							info->load = device_load_sc3000_cart; break;
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "sg,sc"); break;

		default:										cartslot_device_getinfo( devclass, state, info ); break;
	}
}

static void sc3000_cassette_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		case MESS_DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}

static void sc3000_printer_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		case MESS_DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										printer_device_getinfo(devclass, state, info); break;
	}
}

static void sf7000_printer_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		case MESS_DEVINFO_INT_COUNT:							info->i = 2; break;

		default:										printer_device_getinfo(devclass, state, info); break;
	}
}

static DEVICE_LOAD( sf7000_floppy )
{
	if (image_has_been_created(image))
		return INIT_FAIL;

	if (image_length(image) == 256*16*40)
	{
		if (device_load_basicdsk_floppy(image) == INIT_PASS)
		{
			/* image, tracks, sides, sectors per track, sector length, first sector id, offset of track 0, track skipping */
			basicdsk_set_geometry(image, 40, 1, 16, 256, 1, 0, FALSE);

			return INIT_PASS;
		}
	}

	return INIT_FAIL;
}

static void sf7000_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		case MESS_DEVINFO_INT_COUNT:							info->i = 1; break;
		case MESS_DEVINFO_PTR_LOAD:							info->load = device_load_sf7000_floppy; break;
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "sf7"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}

static DEVICE_LOAD( sf7000_serial )
{
	/* filename specified */
	if (serial_device_load(image)==INIT_PASS)
	{
		/* setup transmit parameters */
		serial_device_setup(image, 9600 >> readinputportbytag("BAUD"), 8, 1, SERIAL_PARITY_NONE);

		/* connect serial chip to serial device */
		msm8251_connect_to_serial_device(image);

		serial_device_set_protocol(image, SERIAL_PROTOCOL_NONE);

		/* and start transmit */
		serial_device_set_transmit_state(image, 1);

		return INIT_PASS;
	}

	return INIT_FAIL;
}

static void sf7000_serial_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* serial */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_TYPE:							info->i = IO_SERIAL; break;
		case MESS_DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_INIT:							info->init = serial_device_init; break;
		case MESS_DEVINFO_PTR_LOAD:							info->load = device_load_sf7000_serial; break;
		case MESS_DEVINFO_PTR_UNLOAD:						info->unload = serial_device_unload; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "txt"); break;
	}
}

SYSTEM_CONFIG_START( sg1000 )
	CONFIG_DEVICE(sg1000_cartslot_getinfo)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START( sc3000 )
	CONFIG_DEVICE(sc3000_cartslot_getinfo)
	CONFIG_DEVICE(sc3000_cassette_getinfo)
	CONFIG_DEVICE(sc3000_printer_getinfo)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START( sf7000 )
	CONFIG_RAM_DEFAULT	(64 * 1024)
	CONFIG_DEVICE(sc3000_cassette_getinfo)
	CONFIG_DEVICE(sf7000_printer_getinfo)
	CONFIG_DEVICE(sf7000_floppy_getinfo)
	CONFIG_DEVICE(sf7000_serial_getinfo)
SYSTEM_CONFIG_END

/* System Drivers */

/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT       INIT    CONFIG      COMPANY   FULLNAME */
CONS( 1983,	sg1000,		0,		0,		sg1000,		sg1000,		0,		sg1000,		"Sega",	"SG-1000", 0 )
COMP( 1983,	sc3000,		0,		0,		sc3000,		sc3000,		0,		sc3000,		"Sega",	"SC-3000", 0 )
COMP( 1983,	sf7000,		sc3000, 0,		sf7000,		sf7000,		0,		sf7000,		"Sega",	"SC-3000/Super Control Station SF-7000", 0 )
CONS( 1984,	sg1000m2,	sg1000,	0,		sc3000,		sc3000,		0,		sg1000,		"Sega",	"SG-1000 Mark II", 0 )
//COMP( 1983,   omv,        sg1000, 0,      omv,        omv,        0,      omv,        "Tsukuda Original", "Othello Multivision", GAME_NOT_WORKING )
