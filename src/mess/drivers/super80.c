/* Super80.c written by Robbbert, 2005-2009. See the MESS wiki for documentation. */

#include "driver.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "sound/wave.h"
#include "machine/z80pio.h"
#include "devices/snapquik.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "devices/z80bin.h"
#include "sound/speaker.h"
#include "machine/ctronics.h"
#include "video/mc6845.h"
#include "super80.h"


static const device_config *super80_z80pio;
static const device_config *super80_speaker;
static const device_config *super80_cassette;
static const device_config *super80_printer;

UINT8 *pcgram;
UINT8 super80v_vid_col=1;			// 0 = color ram ; 1 = video ram
UINT8 super80v_rom_pcg=1;			// 0 = prom ; 1 = pcg
UINT8 super80_mhz=2;	/* state of bit 2 of port F0 */

#define MASTER_CLOCK			(XTAL_12MHz)
#define PIXEL_CLOCK			(MASTER_CLOCK/2)
#define HTOTAL				(384)
#define HBEND				(0)
#define HBSTART				(256)
#define VTOTAL				(240)
#define VBEND				(0)
#define VBSTART				(160)

#define SUPER80V_SCREEN_WIDTH		(560)
#define SUPER80V_SCREEN_HEIGHT		(300)
#define SUPER80V_DOTS			(7)


/**************************** PIO ******************************************************************************/

static UINT8 keylatch;

/* This activates when Control + C + 4 pressed */
static void super80_pio_interrupt(const device_config *device, int state)
{
	cputag_set_input_line(device->machine, "maincpu", 0, state );
}

static WRITE8_DEVICE_HANDLER( pio_port_a_w )
{
	keylatch = data;
};

static READ8_DEVICE_HANDLER( pio_port_b_r )
{
	static const char *const keynames[] = { "LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7" };

	int bit;
	UINT8 data = 0xff;

	for (bit = 0; bit < 8; bit++)
	{
		if (!BIT(keylatch, bit)) data &= input_port_read(device->machine, keynames[bit]);
	}

	return data;
};

static const z80pio_interface super80_pio_intf =
{
	DEVCB_LINE(super80_pio_interrupt),		/* callback when change interrupt status */
	DEVCB_NULL,
	DEVCB_HANDLER(pio_port_b_r),
	DEVCB_HANDLER(pio_port_a_w),
	DEVCB_NULL,
	DEVCB_NULL,			/* portA ready active callback (not used in super80) */
	DEVCB_NULL			/* portB ready active callback (not used in super80) */
};


static const z80_daisy_chain super80_daisy_chain[] =
{
	{ "z80pio" },
	{ NULL }
};

/**************************** CASSETTE ROUTINES *****************************************************************/

static void super80_cassette_motor( running_machine *machine, UINT8 data )
{
	if (data)
		cassette_change_state(super80_cassette, CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);
	else
		cassette_change_state(super80_cassette, CASSETTE_MOTOR_ENABLED, CASSETTE_MASK_MOTOR);

	/* does user want to hear the sound? */
	if (input_port_read(machine, "CONFIG") & 8)
		cassette_change_state(super80_cassette, CASSETTE_SPEAKER_ENABLED, CASSETTE_MASK_SPEAKER);
	else
		cassette_change_state(super80_cassette, CASSETTE_SPEAKER_MUTED, CASSETTE_MASK_SPEAKER);
}

/********************************************* TIMER ************************************************/

static UINT8 cass_data[]={ 0, 0, 0, 0 };

	/* this timer runs at 200khz and does 2 jobs:  
	1. Scan the keyboard and present the results to the pio  
	2. Emulate the 2 chips in the cassette input circuit  

	Reasons why it is necessary:  
	1. The real z80pio is driven by the cpu clock and is capable of independent actions.  
	MAME does not support this at all. If the interrupt key sequence is entered, the  
	computer can be reset out of a hung state by the operator.  
	2. This "emulates" U79 CD4046BCN PLL chip and U1 LM311P op-amp. U79 converts a frequency to a voltage,  
	and U1 amplifies that voltage to digital levels. U1 has a trimpot connected, to set the midpoint.

	The MDS homebrew input circuit consists of 2 op-amps followed by a D-flipflop.
	My "read-any-system" cassette circuit was a CA3140 op-amp, the smarts being done in software.

	bit 0 = original system (U79 and U1)
	bit 1 = MDS fast system
	bit 2 = CA3140 */

static TIMER_CALLBACK( super80_timer )
{
	UINT8 cass_ws=0;

	cass_data[1]++;
	cass_ws = (cassette_input(super80_cassette) > +0.03) ? 4 : 0;

	if (cass_ws != cass_data[0])
	{
		if (cass_ws) cass_data[3] ^= 2;						// the MDS flipflop
		cass_data[0] = cass_ws;
		cass_data[2] = ((cass_data[1] < 0x40) ? 1 : 0) | cass_ws | cass_data[3];
		cass_data[1] = 0;
	}

	z80pio_p_w(super80_z80pio,1,pio_port_b_r(super80_z80pio,0));
}

/*************************************** PRINTER ********************************************************/

/* The Super80 had an optional I/O card that plugged into the S-100 slot. The card had facility for running
	an 8-bit Centronics printer, and a serial device at 300, 600, or 1200 baud. The I/O address range
	was selectable via 4 dipswitches. The serial parameters (baud rate, parity, stop bits, etc) was
	chosen with more dipswitches. Regretably, no parameters could be set by software. Currently, the
	Centronics printer is emulated; the serial side of things may be done later, as will the dipswitches.

	The most commonly used I/O range is DC-DE (DC = centronics, DD = serial data, DE = serial control
	All the home-brew roms use this, except for super80e which uses BC-BE (which we don't support yet). */

/**************************** I/O PORTS *****************************************************************/

static READ8_HANDLER( super80_dc_r )
{
	UINT8 data=0x7f;

	/* bit 7 = printer busy
	0 = printer is not busy */

	data |= centronics_busy_r(super80_printer) << 7;

	return data;
}

static READ8_HANDLER( super80_f2_r )
{
	UINT8 data = input_port_read(space->machine, "DSW") & 0xf0;	// dip switches on pcb
	data |= cass_data[2];			// bit 0 = output of U1, bit 1 = MDS cass state, bit 2 = current wave_state
	data |= 0x08;				// bit 3 - not used
	return data;
}

static WRITE8_HANDLER( super80_dc_w )
{
	/* hardware strobe driven from port select, bit 7..0 = data */
	centronics_strobe_w(super80_printer, 1);
	centronics_data_w(super80_printer, 0, data);
	centronics_strobe_w(super80_printer, 0);
}

static UINT8 last_data;

static WRITE8_HANDLER( super80_f0_w )
{
	UINT8 bits = data ^ last_data;

	if (bits & 0x20) set_led_status(2,(data & 32) ? 0 : 1);		/* bit 5 - LED - scroll lock led is used */
	speaker_level_w(super80_speaker, (data & 8) ? 0 : 1);				/* bit 3 - speaker */
	super80_mhz = (data & 4) ? 1 : 2;				/* bit 2 - video on/off */
	if (bits & 2) super80_cassette_motor(space->machine, data & 2 ? 1 : 0);	/* bit 1 - cassette motor */
	if (bits & 1) cassette_output(super80_cassette, (data & 1) ? -1.0 : +1.0);	/* bit 0 - cass out */

	last_data = data;
}

static WRITE8_HANDLER( super80r_f0_w )
{
	UINT8 bits = data ^ last_data;

	if (bits & 0x20) set_led_status(2,(data & 32) ? 0 : 1);		/* bit 5 - LED - scroll lock led is used */
	speaker_level_w(super80_speaker, (data & 8) ? 0 : 1);				/* bit 3 - speaker */
	if (bits & 2) super80_cassette_motor(space->machine, data & 2 ? 1 : 0);	/* bit 1 - cassette motor */
	if (bits & 1) cassette_output(super80_cassette, (data & 1) ? -1.0 : +1.0);	/* bit 0 - cass out */

	last_data = data;
}

static WRITE8_HANDLER( super80v_f0_w )
{
	UINT8 bits = data ^ last_data;

	if (bits & 0x20) set_led_status(2,(data & 32) ? 0 : 1);		/* bit 5 - LED - scroll lock led is used */
	super80v_rom_pcg = data & 0x10;					/* bit 4 - bankswitch gfx rom or pcg */
	speaker_level_w(super80_speaker, (data & 8) ? 0 : 1);				/* bit 3 - speaker */
	super80v_vid_col = data & 4;					/* bit 2 - bankswitch video or colour ram */
	if (bits & 2) super80_cassette_motor(space->machine, data & 2 ? 1 : 0);	/* bit 1 - cassette motor */
	if (bits & 1) cassette_output(super80_cassette, (data & 1) ? -1.0 : +1.0);	/* bit 0 - cass out */

	last_data = data;
}

static READ8_HANDLER( super80_f8_r ) { return z80pio_d_r (super80_z80pio,0); }
static READ8_HANDLER( super80_f9_r ) { return z80pio_c_r (super80_z80pio,0); }
static READ8_HANDLER( super80_fa_r ) { return z80pio_d_r (super80_z80pio,1); }
static READ8_HANDLER( super80_fb_r ) { return z80pio_c_r (super80_z80pio,1); }
static WRITE8_HANDLER( super80_f8_w ) { z80pio_d_w(super80_z80pio,0, data); }
static WRITE8_HANDLER( super80_f9_w ) { z80pio_c_w(super80_z80pio,0, data); }
static WRITE8_HANDLER( super80_fa_w ) { z80pio_d_w(super80_z80pio,1, data); }
static WRITE8_HANDLER( super80_fb_w ) { z80pio_c_w(super80_z80pio,1, data); }

/**************************** MEMORY AND I/O MAPPINGS *****************************************************************/

/* A read_byte or write_byte to unmapped memory crashes MESS, and UNMAP doesnt fix it.
	This makes the H and E monitor commands show FF */
static READ8_HANDLER( super80_read_ff ) { return 0xff; }

static ADDRESS_MAP_START( super80_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_RAMBANK(1) AM_REGION("maincpu", 0x0000)
	AM_RANGE(0x4000, 0xbfff) AM_RAM AM_REGION("maincpu", 0x4000)
	AM_RANGE(0xc000, 0xefff) AM_ROM
	AM_RANGE(0xf000, 0xffff) AM_READWRITE(super80_read_ff, SMH_NOP)
ADDRESS_MAP_END

static ADDRESS_MAP_START( super80m_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_RAMBANK(1) AM_REGION("maincpu", 0x0000)
	AM_RANGE(0x4000, 0xbfff) AM_RAM AM_REGION("maincpu", 0x4000)
	AM_RANGE(0xc000, 0xefff) AM_ROM
	AM_RANGE(0xf000, 0xffff) AM_RAM AM_REGION("maincpu", 0xf000)
ADDRESS_MAP_END

static ADDRESS_MAP_START( super80v_map, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_RAMBANK(1)
	AM_RANGE(0x4000, 0xbfff) AM_RAM
	AM_RANGE(0xc000, 0xefff) AM_ROM
	AM_RANGE(0xf000, 0xf7ff) AM_READWRITE(super80v_low_r, super80v_low_w) AM_BASE(&pcgram)
	AM_RANGE(0xf800, 0xffff) AM_READWRITE(super80v_high_r, super80v_high_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( super80_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0xdc, 0xdc) AM_READWRITE(super80_dc_r, super80_dc_w)
	AM_RANGE(0xe0, 0xe0) AM_MIRROR(0x14) AM_WRITE(super80_f0_w)
	AM_RANGE(0xe1, 0xe1) AM_MIRROR(0x14) AM_WRITE(super80_f1_w)
	AM_RANGE(0xe2, 0xe2) AM_MIRROR(0x14) AM_READ(super80_f2_r)
	AM_RANGE(0xf8, 0xf8) AM_MIRROR(0x04) AM_READWRITE(super80_f8_r,super80_f8_w)
	AM_RANGE(0xf9, 0xf9) AM_MIRROR(0x04) AM_READWRITE(super80_f9_r,super80_f9_w)
	AM_RANGE(0xfa, 0xfa) AM_MIRROR(0x04) AM_READWRITE(super80_fa_r,super80_fa_w)
	AM_RANGE(0xfb, 0xfb) AM_MIRROR(0x04) AM_READWRITE(super80_fb_r,super80_fb_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( super80r_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x10, 0x10) AM_WRITE(super80v_10_w)
	AM_RANGE(0x11, 0x11) AM_DEVREAD("crtc", mc6845_register_r)
	AM_RANGE(0x11, 0x11) AM_WRITE(super80v_11_w)
	AM_RANGE(0xdc, 0xdc) AM_READWRITE(super80_dc_r, super80_dc_w)
	AM_RANGE(0xe0, 0xe0) AM_MIRROR(0x14) AM_WRITE(super80r_f0_w)
	AM_RANGE(0xe2, 0xe2) AM_MIRROR(0x14) AM_READ(super80_f2_r)
	AM_RANGE(0xf8, 0xf8) AM_MIRROR(0x04) AM_READWRITE(super80_f8_r,super80_f8_w)
	AM_RANGE(0xf9, 0xf9) AM_MIRROR(0x04) AM_READWRITE(super80_f9_r,super80_f9_w)
	AM_RANGE(0xfa, 0xfa) AM_MIRROR(0x04) AM_READWRITE(super80_fa_r,super80_fa_w)
	AM_RANGE(0xfb, 0xfb) AM_MIRROR(0x04) AM_READWRITE(super80_fb_r,super80_fb_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( super80v_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x10, 0x10) AM_WRITE(super80v_10_w)
	AM_RANGE(0x11, 0x11) AM_DEVREAD("crtc", mc6845_register_r)
	AM_RANGE(0x11, 0x11) AM_WRITE(super80v_11_w)
	AM_RANGE(0xdc, 0xdc) AM_READWRITE(super80_dc_r, super80_dc_w)
	AM_RANGE(0xe0, 0xe0) AM_MIRROR(0x14) AM_WRITE(super80v_f0_w)
	AM_RANGE(0xe2, 0xe2) AM_MIRROR(0x14) AM_READ(super80_f2_r)
	AM_RANGE(0xf8, 0xf8) AM_MIRROR(0x04) AM_READWRITE(super80_f8_r,super80_f8_w)
	AM_RANGE(0xf9, 0xf9) AM_MIRROR(0x04) AM_READWRITE(super80_f9_r,super80_f9_w)
	AM_RANGE(0xfa, 0xfa) AM_MIRROR(0x04) AM_READWRITE(super80_fa_r,super80_fa_w)
	AM_RANGE(0xfb, 0xfb) AM_MIRROR(0x04) AM_READWRITE(super80_fb_r,super80_fb_w)
ADDRESS_MAP_END

/**************************** DIPSWITCHES, KEYBOARD, HARDWARE CONFIGURATION ****************************************/

static INPUT_PORTS_START( super80 )
	PORT_START("DSW")
	PORT_BIT( 0xf, 0xf, IPT_UNUSED )
	PORT_DIPNAME( 0x10, 0x00, "Switch A") PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x10, DEF_STR(Off))
	PORT_DIPSETTING(    0x00, DEF_STR(On))
	PORT_DIPNAME( 0x20, 0x20, "Switch B") PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(    0x20, DEF_STR(Off))
	PORT_DIPSETTING(    0x00, DEF_STR(On))
	PORT_DIPNAME( 0x40, 0x40, "Switch C") PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x40, DEF_STR(Off))
	PORT_DIPSETTING(    0x00, DEF_STR(On))
	PORT_DIPNAME( 0x80, 0x00, "Switch D") PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x80, DEF_STR(Off))
	PORT_DIPSETTING(    0x00, DEF_STR(On))

	PORT_START("LINE0")	/* line 0 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("@ `") PORT_CODE(KEYCODE_TILDE) PORT_CHAR('@') PORT_CHAR('`') PORT_CHAR(0x00)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H) PORT_CHAR('H') PORT_CHAR('H') PORT_CHAR(0x08)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P) PORT_CHAR('P') PORT_CHAR('P') PORT_CHAR(0x10)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X) PORT_CHAR('X') PORT_CHAR('X') PORT_CHAR(0x18)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1 !") PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9 )") PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("REPT") PORT_CODE(KEYCODE_LALT)
	PORT_START("LINE1")	/* line 1 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A) PORT_CHAR('A') PORT_CHAR('A') PORT_CHAR(0x01)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I) PORT_CHAR('I') PORT_CHAR('I') PORT_CHAR(0x09)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q) PORT_CHAR('Q') PORT_CHAR('Q') PORT_CHAR(0x11)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y) PORT_CHAR('Y') PORT_CHAR('Y') PORT_CHAR(0x19)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2 \"") PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('\"')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(": *") PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Backspace") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(0x08)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_START("LINE2")	/* line 2 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B) PORT_CHAR('B') PORT_CHAR('B') PORT_CHAR(0x02)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J) PORT_CHAR('J') PORT_CHAR('J') PORT_CHAR(0x0a)	// port_char 0x0a is hijacked to 0x0d
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R) PORT_CHAR('R') PORT_CHAR('R') PORT_CHAR(0x12)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z) PORT_CHAR('Z') PORT_CHAR('Z') PORT_CHAR(0x1a)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3 #") PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("; +") PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("TAB") PORT_CODE(KEYCODE_TAB) PORT_CHAR(0x09)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(Fire)") PORT_CODE(KEYCODE_INSERT) PORT_CHAR(UCHAR_MAMEKEY(INSERT))
	PORT_START("LINE3")	/* line 3 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C) PORT_CHAR('C') PORT_CHAR('C') PORT_CHAR(0x03)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K) PORT_CHAR('K') PORT_CHAR('K') PORT_CHAR(0x0b)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S) PORT_CHAR('S') PORT_CHAR('S') PORT_CHAR(0x13)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("[ {") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[') PORT_CHAR('{') PORT_CHAR(0x1b)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4 $") PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(", <") PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LINEFEED") PORT_CODE(KEYCODE_ENTER_PAD) PORT_CHAR(0x0a)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_START("LINE4")	/* line 4 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D) PORT_CHAR('D') PORT_CHAR('D') PORT_CHAR(0x04)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L) PORT_CHAR('L') PORT_CHAR('L') PORT_CHAR(0x0c)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T) PORT_CHAR('T') PORT_CHAR('T') PORT_CHAR(0x14)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\\ |")PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\') PORT_CHAR('|') PORT_CHAR(0x1c)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5 %") PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("BRK") PORT_CODE(KEYCODE_NUMLOCK) PORT_CHAR(0x03)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(0x0d)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(Right)") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_START("LINE5")	/* line 5 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E) PORT_CHAR('E') PORT_CHAR('E') PORT_CHAR(0x05)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M) PORT_CHAR('M') PORT_CHAR('M') PORT_CHAR(0x0d)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U) PORT_CHAR('U') PORT_CHAR('U') PORT_CHAR(0x15)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("] }") PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']') PORT_CHAR('}') PORT_CHAR(0x1d)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6 &") PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(". >") PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC) PORT_CHAR(0x1b)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(Left)") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_START("LINE6")	/* line 6 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F) PORT_CHAR('F') PORT_CHAR('F') PORT_CHAR(0x06)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N) PORT_CHAR('N') PORT_CHAR('N') PORT_CHAR(0x0e)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V) PORT_CHAR('V') PORT_CHAR('V') PORT_CHAR(0x16)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("^ ~") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('^') PORT_CHAR('~') PORT_CHAR(0x1e)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7 \'") PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/ ?") PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DEL") PORT_CODE(KEYCODE_DEL) PORT_CHAR(0x7f) PORT_CHAR(0x7f) PORT_CHAR(0x1f)	// natural kbd, press ctrl-backspace to DEL
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(Down)") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_START("LINE7")	/* line 7 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G) PORT_CHAR('G') PORT_CHAR('G') PORT_CHAR(0x07)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O) PORT_CHAR('O') PORT_CHAR('O') PORT_CHAR(0x0f)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W) PORT_CHAR('W') PORT_CHAR('W') PORT_CHAR(0x17)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("- =") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8 (") PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR(' ')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LOCK") PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(0x80)				// port_char doesn't work, no equivalent key
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(Up)") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))

	/* Enhanced options not available on real hardware */
	PORT_START("CONFIG")
	PORT_CONFNAME( 0x01, 0x01, "Autorun on Quickload")
	PORT_CONFSETTING(    0x00, DEF_STR(No))
	PORT_CONFSETTING(    0x01, DEF_STR(Yes))
	PORT_CONFNAME( 0x02, 0x02, "2 MHz always")
	PORT_CONFSETTING(    0x02, DEF_STR(No))
	PORT_CONFSETTING(    0x00, DEF_STR(Yes))
	PORT_CONFNAME( 0x04, 0x04, "Screen on always")
	PORT_CONFSETTING(    0x04, DEF_STR(No))
	PORT_CONFSETTING(    0x00, DEF_STR(Yes))
	PORT_CONFNAME( 0x08, 0x08, "Cassette Speaker")
	PORT_CONFSETTING(    0x08, DEF_STR(On))
	PORT_CONFSETTING(    0x00, DEF_STR(Off))
INPUT_PORTS_END

static INPUT_PORTS_START( super80d )
	PORT_START("DSW")
	PORT_BIT( 0xf, 0xf, IPT_UNUSED )
	PORT_DIPNAME( 0x10, 0x00, "Switch A") PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x10, DEF_STR(Off))
	PORT_DIPSETTING(    0x00, DEF_STR(On))
	PORT_DIPNAME( 0x20, 0x20, "Switch B") PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(    0x20, DEF_STR(Off))
	PORT_DIPSETTING(    0x00, DEF_STR(On))
	PORT_DIPNAME( 0x40, 0x40, "Switch C") PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x40, DEF_STR(Off))
	PORT_DIPSETTING(    0x00, DEF_STR(On))
	PORT_DIPNAME( 0x80, 0x00, "Switch D") PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x80, DEF_STR(Off))
	PORT_DIPSETTING(    0x00, DEF_STR(On))

	PORT_START("LINE0")	/* line 0 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("@ `") PORT_CODE(KEYCODE_TILDE) PORT_CHAR('@') PORT_CHAR('`') PORT_CHAR(0x00)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H) PORT_CHAR('H') PORT_CHAR('h') PORT_CHAR(0x08)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P) PORT_CHAR('P') PORT_CHAR('p') PORT_CHAR(0x10)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X) PORT_CHAR('X') PORT_CHAR('x') PORT_CHAR(0x18)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1 !") PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9 )") PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("REPT") PORT_CODE(KEYCODE_LALT)
	PORT_START("LINE1")	/* line 1 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A) PORT_CHAR('A') PORT_CHAR('a') PORT_CHAR(0x01)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I) PORT_CHAR('I') PORT_CHAR('i') PORT_CHAR(0x09)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q) PORT_CHAR('Q') PORT_CHAR('q') PORT_CHAR(0x11)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y) PORT_CHAR('Y') PORT_CHAR('y') PORT_CHAR(0x19)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2 \"") PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('\"')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(": *") PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Backspace") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(0x08)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_START("LINE2")	/* line 2 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B) PORT_CHAR('B') PORT_CHAR('b') PORT_CHAR(0x02)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J) PORT_CHAR('J') PORT_CHAR('j') PORT_CHAR(0x0a)	// port_char 0x0a is hijacked to 0x0d
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R) PORT_CHAR('R') PORT_CHAR('r') PORT_CHAR(0x12)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z) PORT_CHAR('Z') PORT_CHAR('z') PORT_CHAR(0x1a)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3 #") PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("; +") PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("TAB") PORT_CODE(KEYCODE_TAB) PORT_CHAR(0x09)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(Fire)") PORT_CODE(KEYCODE_INSERT) PORT_CHAR(UCHAR_MAMEKEY(INSERT))
	PORT_START("LINE3")	/* line 3 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C) PORT_CHAR('C') PORT_CHAR('c') PORT_CHAR(0x03)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K) PORT_CHAR('K') PORT_CHAR('k') PORT_CHAR(0x0b)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S) PORT_CHAR('S') PORT_CHAR('s') PORT_CHAR(0x13)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("[ {") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[') PORT_CHAR('{') PORT_CHAR(0x1b)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4 $") PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(", <") PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LINEFEED") PORT_CODE(KEYCODE_ENTER_PAD) PORT_CHAR(0x0a)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_START("LINE4")	/* line 4 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D) PORT_CHAR('D') PORT_CHAR('d') PORT_CHAR(0x04)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L) PORT_CHAR('L') PORT_CHAR('l') PORT_CHAR(0x0c)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T) PORT_CHAR('T') PORT_CHAR('t') PORT_CHAR(0x14)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\\ |")PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\') PORT_CHAR('|') PORT_CHAR(0x1c)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5 %") PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("BRK") PORT_CODE(KEYCODE_NUMLOCK) PORT_CHAR(0x03)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(0x0d)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(Right)") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_START("LINE5")	/* line 5 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E) PORT_CHAR('E') PORT_CHAR('e') PORT_CHAR(0x05)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M) PORT_CHAR('M') PORT_CHAR('m') PORT_CHAR(0x0d)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U) PORT_CHAR('U') PORT_CHAR('u') PORT_CHAR(0x15)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("] }") PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']') PORT_CHAR('}') PORT_CHAR(0x1d)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6 &") PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(". >") PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC) PORT_CHAR(0x1b)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(Left)") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_START("LINE6")	/* line 6 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F) PORT_CHAR('F') PORT_CHAR('f') PORT_CHAR(0x06)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N) PORT_CHAR('N') PORT_CHAR('n') PORT_CHAR(0x0e)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V) PORT_CHAR('V') PORT_CHAR('v') PORT_CHAR(0x16)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("^ ~") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('^') PORT_CHAR('~') PORT_CHAR(0x1e)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7 \'") PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/ ?") PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DEL") PORT_CODE(KEYCODE_DEL) PORT_CHAR(0x7f) PORT_CHAR(0x5f) PORT_CHAR(0x1f)	// natural kbd, press ctrl-backspace to DEL
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(Down)") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_START("LINE7")	/* line 7 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G) PORT_CHAR('G') PORT_CHAR('g') PORT_CHAR(0x07)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O) PORT_CHAR('O') PORT_CHAR('o') PORT_CHAR(0x0f)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W) PORT_CHAR('W') PORT_CHAR('w') PORT_CHAR(0x17)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("- =") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8 (") PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR(' ')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LOCK") PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(0x80)				// port_char doesn't work, no equivalent key
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(Up)") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))

	/* Enhanced options not available on real hardware */
	PORT_START("CONFIG")
	PORT_CONFNAME( 0x01, 0x01, "Autorun on Quickload")
	PORT_CONFSETTING(    0x00, DEF_STR(No))
	PORT_CONFSETTING(    0x01, DEF_STR(Yes))
	PORT_CONFNAME( 0x02, 0x02, "2 MHz always")
	PORT_CONFSETTING(    0x02, DEF_STR(No))
	PORT_CONFSETTING(    0x00, DEF_STR(Yes))
	PORT_CONFNAME( 0x04, 0x04, "Screen on always")
	PORT_CONFSETTING(    0x04, DEF_STR(No))
	PORT_CONFSETTING(    0x00, DEF_STR(Yes))
	PORT_CONFNAME( 0x08, 0x08, "Cassette Speaker")
	PORT_CONFSETTING(    0x08, DEF_STR(On))
	PORT_CONFSETTING(    0x00, DEF_STR(Off))
INPUT_PORTS_END

static INPUT_PORTS_START( super80m )
	PORT_INCLUDE( super80d )

	PORT_MODIFY("CONFIG")

	/* Enhanced options not available on real hardware */
	PORT_CONFNAME( 0x10, 0x10, "Swap CharGens")
	PORT_CONFSETTING(    0x10, DEF_STR(No))
	PORT_CONFSETTING(    0x00, DEF_STR(Yes))
	PORT_CONFNAME( 0x60, 0x40, "Colour")
	PORT_CONFSETTING(    0x60, "MonoChrome")
	PORT_CONFSETTING(    0x40, "Green")
	PORT_CONFSETTING(    0x00, "Composite")
	PORT_CONFSETTING(    0x20, "RGB")
INPUT_PORTS_END

static INPUT_PORTS_START( super80v )
	PORT_INCLUDE( super80m )

	PORT_MODIFY("CONFIG")
	PORT_BIT( 0x16, 0x16, IPT_UNUSED )
INPUT_PORTS_END

static INPUT_PORTS_START( super80r )
	PORT_INCLUDE( super80v )

	PORT_MODIFY("CONFIG")
	PORT_BIT( 0x16, 0x16, IPT_UNUSED )
	PORT_CONFNAME( 0x60, 0x40, "Colour")
	PORT_CONFSETTING(    0x60, "MonoChrome")
	PORT_CONFSETTING(    0x40, "Green")
INPUT_PORTS_END


/**************************** BASIC MACHINE CONSTRUCTION ***********************************************************/

static TIMER_CALLBACK( super80_halfspeed )
{
	static UINT8 int_sw=0;
	UINT8 go_fast = 0;
	if ((super80_mhz == 2) || (!(input_port_read(machine, "CONFIG") & 2)))	/* bit 2 of port F0 is low, OR user turned on config switch */
		go_fast++;

	/* code to slow down computer to 1 MHz by halting cpu on every second frame */
	if (!go_fast)
	{
		if (!int_sw)
			cputag_set_input_line(machine, "maincpu", INPUT_LINE_HALT, ASSERT_LINE);	// if going, stop it

		int_sw++;
		if (int_sw > 1)
		{
			cputag_set_input_line(machine, "maincpu", INPUT_LINE_HALT, CLEAR_LINE);		// if stopped, start it
			int_sw = 0;
		}
	}
	else
	{
		if (int_sw < 8)								// @2MHz, reset just once
		{
			cputag_set_input_line(machine, "maincpu", INPUT_LINE_HALT, CLEAR_LINE);
			int_sw = 8;							// ...not every time
		}
	}
}


/* after the first 4 bytes have been read from ROM, switch the ram back in */
static TIMER_CALLBACK( super80_reset )
{
	memory_set_bank(machine, 1, 0);
}

static MACHINE_RESET( super80 )
{
	timer_set(machine, ATTOTIME_IN_USEC(10), NULL, 0, super80_reset);
	memory_set_bank(machine, 1, 1);
	super80_z80pio = devtag_get_device(machine, "z80pio");
	super80_speaker = devtag_get_device(machine, "speaker");
	super80_cassette = devtag_get_device(machine, "cassette");
	super80_printer = devtag_get_device(machine, "centronics");
}

static const cassette_config super80_cassette_config =
{
	cassette_default_formats,
	NULL,
	CASSETTE_STOPPED | CASSETTE_MOTOR_DISABLED | CASSETTE_SPEAKER_ENABLED
};


static DEVICE_IMAGE_LOAD( super80_cart )
{
	image_fread(image, memory_region(image->machine, "maincpu") + 0xc000, 0x3000);

	return INIT_PASS;
}

static const mc6845_interface super80v_crtc = {
	"screen",			/* name of screen */
	SUPER80V_DOTS,			/* number of dots per character */
	NULL,
	super80v_update_row,		/* handler to display a scanline */
	NULL,
	NULL,
	NULL,
	NULL
};

static MACHINE_DRIVER_START( super80_cartslot )
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("rom")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_LOAD(super80_cart)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( super80 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, MASTER_CLOCK/6)		/* 2 MHz */
	MDRV_CPU_PROGRAM_MAP(super80_map)
	MDRV_CPU_IO_MAP(super80_io)
	MDRV_CPU_CONFIG(super80_daisy_chain)

	MDRV_MACHINE_RESET( super80 )

	MDRV_Z80PIO_ADD( "z80pio", super80_pio_intf )

	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(48.8)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_RAW_PARAMS(PIXEL_CLOCK, HTOTAL, HBEND, HBSTART, VTOTAL, VBEND, VBSTART)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(super80)
	MDRV_VIDEO_UPDATE(super80)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_WAVE_ADD("wave", "cassette")
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	/* printer */
	MDRV_CENTRONICS_ADD("centronics", standard_centronics)

	/* quickload */
	MDRV_Z80BIN_QUICKLOAD_ADD("quickload", default, 1)

	/* cassette */
	MDRV_CASSETTE_ADD( "cassette", super80_cassette_config )

	/* cartridge */
	MDRV_IMPORT_FROM(super80_cartslot)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( super80d )
	MDRV_IMPORT_FROM(super80)
	MDRV_VIDEO_UPDATE(super80d)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( super80e )
	MDRV_IMPORT_FROM(super80)
	MDRV_VIDEO_UPDATE(super80e)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( super80m )
	MDRV_IMPORT_FROM(super80)
	MDRV_CPU_MODIFY("maincpu")
	MDRV_CPU_PROGRAM_MAP(super80m_map)

	MDRV_PALETTE_LENGTH(16)
	MDRV_PALETTE_INIT(super80m)
	MDRV_VIDEO_EOF(super80m)
	MDRV_VIDEO_UPDATE(super80m)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( super80v )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, MASTER_CLOCK/6)		/* 2 MHz */
	MDRV_CPU_PROGRAM_MAP(super80v_map)
	MDRV_CPU_IO_MAP(super80v_io)
	MDRV_CPU_CONFIG(super80_daisy_chain)

	MDRV_MACHINE_RESET( super80 )

	MDRV_Z80PIO_ADD( "z80pio", super80_pio_intf )

	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(SUPER80V_SCREEN_WIDTH, SUPER80V_SCREEN_HEIGHT)
	MDRV_SCREEN_VISIBLE_AREA(0, SUPER80V_SCREEN_WIDTH-1, 0, SUPER80V_SCREEN_HEIGHT-1)
	MDRV_PALETTE_LENGTH(16)
	MDRV_PALETTE_INIT(super80m)

	MDRV_MC6845_ADD("crtc", MC6845, MASTER_CLOCK / SUPER80V_DOTS, super80v_crtc)

	MDRV_VIDEO_START(super80v)
	MDRV_VIDEO_EOF(super80m)
	MDRV_VIDEO_UPDATE(super80v)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_WAVE_ADD("wave", "cassette")
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	/* printer */
	MDRV_CENTRONICS_ADD("centronics", standard_centronics)

	/* quickload */
	MDRV_Z80BIN_QUICKLOAD_ADD("quickload", default, 1)

	/* cassette */
	MDRV_CASSETTE_ADD( "cassette", super80_cassette_config )

	/* cartridge */
	MDRV_IMPORT_FROM(super80_cartslot)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( super80r )
	MDRV_IMPORT_FROM(super80v)
	MDRV_CPU_MODIFY("maincpu")
	MDRV_CPU_IO_MAP(super80r_io)
MACHINE_DRIVER_END

static void driver_init_common( running_machine *machine )
{
	UINT8 *RAM = memory_region(machine, "maincpu");
	memory_configure_bank(machine, 1, 0, 2, &RAM[0x0000], 0xc000);
	timer_pulse(machine, ATTOTIME_IN_HZ(200000),NULL,0,super80_timer);	/* timer for keyboard and cassette */
}

static DRIVER_INIT( super80 )
{
	timer_pulse(machine, ATTOTIME_IN_HZ(100),NULL,0,super80_halfspeed);	/* timer for 1MHz slowdown */
	driver_init_common(machine);
}

static DRIVER_INIT( super80v )
{
	pcgram = memory_region(machine, "maincpu")+0xf000;
	videoram = memory_region(machine, "maincpu")+0x18000;
	colorram = memory_region(machine, "maincpu")+0x1C000;
	driver_init_common(machine);
}


/**************************** ROMS *****************************************************************/

ROM_START( super80 )
	ROM_REGION(0x10000, "maincpu", 0)
	ROM_LOAD("super80.u26",   0xc000, 0x1000, CRC(6a6a9664) SHA1(2c4fcd943aa9bf7419d58fbc0e28ffb89ef22e0b) )
	ROM_LOAD("super80.u33",   0xd000, 0x1000, CRC(cf8020a8) SHA1(2179a61f80372cd49e122ad3364773451531ae85) )
	ROM_LOAD("super80.u42",   0xe000, 0x1000, CRC(a1c6cb75) SHA1(d644ca3b399c1a8902f365c6095e0bbdcea6733b) )
	ROM_FILL( 0xf000, 0x1000, 0xff)	/* This makes the screen show the FF character when O F1 F0 entered */

	ROM_REGION(0x0400, "gfx1", 0)	// 2513 prom
	ROM_LOAD("super80.u27",   0x0000, 0x0400, CRC(d1e4b3c6) SHA1(3667b97c6136da4761937958f281609690af4081) )
ROM_END

ROM_START( super80d )
	ROM_REGION(0x10000, "maincpu", 0)
	ROM_SYSTEM_BIOS(0, "super80d", "V2.2")
	ROMX_LOAD("super80d.u26", 0xc000, 0x1000, CRC(cebd2613) SHA1(87b94cc101a5948ce590211c68272e27f4cbe95a), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "super80f", "MDS (original)")
	ROMX_LOAD("super80f.u26", 0xc000, 0x1000, CRC(d39775f0) SHA1(b47298ee028924612e9728bb2debd0f47399add7), ROM_BIOS(2))
	ROM_SYSTEM_BIOS(2, "super80g", "MDS (upgraded)")
	ROMX_LOAD("super80g.u26", 0xc000, 0x1000, CRC(7386f507) SHA1(69d7627033d62bd4e886ccc136e89f1524d38f47), ROM_BIOS(3))
	ROM_LOAD("super80.u33",	  0xd000, 0x1000, CRC(cf8020a8) SHA1(2179a61f80372cd49e122ad3364773451531ae85) )
	ROM_LOAD("super80.u42",	  0xe000, 0x1000, CRC(a1c6cb75) SHA1(d644ca3b399c1a8902f365c6095e0bbdcea6733b) )
	ROM_FILL( 0xf000, 0x1000, 0xff)

	ROM_REGION(0x0800, "gfx1", 0)	// 2716 eprom
	ROM_LOAD("super80d.u27",  0x0000, 0x0800, CRC(cb4c81e2) SHA1(8096f21c914fa76df5d23f74b1f7f83bd8645783) )
ROM_END

ROM_START( super80e )
	ROM_REGION(0x10000, "maincpu", 0)
	ROM_LOAD("super80e.u26",  0xc000, 0x1000, CRC(bdc668f8) SHA1(3ae30b3cab599fca77d5e461f3ec1acf404caf07) )
	ROM_LOAD("super80.u33",	  0xd000, 0x1000, CRC(cf8020a8) SHA1(2179a61f80372cd49e122ad3364773451531ae85) )
	ROM_LOAD("super80.u42",	  0xe000, 0x1000, CRC(a1c6cb75) SHA1(d644ca3b399c1a8902f365c6095e0bbdcea6733b) )
	ROM_FILL( 0xf000, 0x1000, 0xff)

	ROM_REGION(0x1000, "gfx1", 0)	// 2732 eprom
	ROM_LOAD("super80e.u27",  0x0000, 0x1000, CRC(ebe763a7) SHA1(ffaa6d6a2c5dacc5a6651514e6707175a32e83e8) )
ROM_END

ROM_START( super80m )
	ROM_REGION(0x10000, "maincpu", 0)
	ROM_LOAD("s80-8r0.u26",	  0xc000, 0x1000, CRC(48d410d8) SHA1(750d984abc013a3344628300288f6d1ba140a95f) )
	ROM_LOAD("s80-8r0.u33",   0xd000, 0x1000, CRC(9765793e) SHA1(4951b127888c1f3153004cc9fb386099b408f52c) )
	ROM_LOAD("s80-8r0.u42",   0xe000, 0x1000, CRC(5f65d94b) SHA1(fe26b54dec14e1c4911d996c9ebd084a38dcb691) )
#if 0
	/* Temporary patch to fix crash when lprinting a tab */
	ROM_FILL(0xcc44,1,0x46)
	ROM_FILL(0xcc45,1,0xc5)
	ROM_FILL(0xcc46,1,0x06)
	ROM_FILL(0xcc47,1,0x20)
	ROM_FILL(0xcc48,1,0xcd)
	ROM_FILL(0xcc49,1,0xc7)
	ROM_FILL(0xcc4a,1,0xcb)
	ROM_FILL(0xcc4b,1,0xc1)
	ROM_FILL(0xcc4c,1,0x10)
	ROM_FILL(0xcc4d,1,0xf7)
	ROM_FILL(0xcc4e,1,0x00)
	ROM_FILL(0xcc4f,1,0x00)
#endif
	ROM_REGION(0x1800, "gfx1", 0)
	ROM_LOAD("super80e.u27",  0x0000, 0x1000, CRC(ebe763a7) SHA1(ffaa6d6a2c5dacc5a6651514e6707175a32e83e8) )
	ROM_LOAD("super80d.u27",  0x1000, 0x0800, CRC(cb4c81e2) SHA1(8096f21c914fa76df5d23f74b1f7f83bd8645783) )
ROM_END

ROM_START( super80r )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_SYSTEM_BIOS(0, "super80r", "MCE (original)")
	ROMX_LOAD("super80r.u26", 0xc000, 0x1000, CRC(01bb6406) SHA1(8e275ecf5141b93f86e45ff8a735b965ea3e8d44), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "super80s", "MCE (upgraded)")
	ROMX_LOAD("super80s.u26", 0xc000, 0x1000, CRC(3e29d307) SHA1(b3f4667633e0a4eb8577e39b5bd22e1f0bfbc0a9), ROM_BIOS(2))

	ROM_LOAD("super80.u33",	  0xd000, 0x1000, CRC(cf8020a8) SHA1(2179a61f80372cd49e122ad3364773451531ae85) )
	ROM_LOAD("super80.u42",	  0xe000, 0x1000, CRC(a1c6cb75) SHA1(d644ca3b399c1a8902f365c6095e0bbdcea6733b) )
	ROM_LOAD("s80hmce.ic24",  0xf000, 0x0800, CRC(a6488a1e) SHA1(7ba613d70a37a6b738dcd80c2bb9988ff1f011ef) )
ROM_END

ROM_START( super80v )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD("s80-v37v.u26",  0xc000, 0x1000, CRC(01e0c0dd) SHA1(ef66af9c44c651c65a21d5bda939ffa100078c08) )
	ROM_LOAD("s80-v37.u33",   0xd000, 0x1000, CRC(812ad777) SHA1(04f355bea3470a7d9ea23bb2811f6af7d81dc400) )
	ROM_LOAD("s80-v37.u42",   0xe000, 0x1000, CRC(e02e736e) SHA1(57b0264c805da99234ab5e8e028fca456851a4f9) )
	ROM_LOAD("s80hmce.ic24",  0xf000, 0x0800, CRC(a6488a1e) SHA1(7ba613d70a37a6b738dcd80c2bb9988ff1f011ef) )
ROM_END

/*    YEAR  NAME      PARENT COMPAT MACHINE INPUT     INIT       CONFIG   COMPANY       FULLNAME */
COMP( 1981, super80,  0,       0, super80,  super80,  super80,  0, "Dick Smith Electronics","Super-80 (V1.2)" , 0)
COMP( 1981, super80d, super80, 0, super80d, super80d, super80,  0, "Dick Smith Electronics","Super-80 (V2.2)" , 0)
COMP( 1981, super80e, super80, 0, super80e, super80d, super80,  0, "Dick Smith Electronics","Super-80 (El Graphix 4)" , 0)
COMP( 1981, super80m, super80, 0, super80m, super80m, super80,  0, "Dick Smith Electronics","Super-80 (8R0)" , 0)
COMP( 1981, super80r, super80, 0, super80r, super80r, super80v, 0, "Dick Smith Electronics","Super-80 (with VDUEB)" , 0)
COMP( 1981, super80v, super80, 0, super80v, super80v, super80v, 0, "Dick Smith Electronics","Super-80 (with enhanced VDUEB)" , 0)
