/*

Ohio Scientific Superboard II Model 600

PCB Layout
----------

OHIO SCIENTIFIC MODEL 600 REV D

|-------------------------------------------------------------------------------------------|
|					2114	2114															|
|									LS138				CN1									|
|		IC1			2114	2114															|
|									LS04			LS75	LS75							|
|					2114	2114															|
|									LS138													|
|					2114	2114					LS125	LS125							|
|									LS138													|
| -					2114	2114					8T28	8T28							|
| |																							|
|C|					2114	2114															|
|N|																							|
|3|					2114	2114						6502								|
| |																							|
| -					2114	2114															|
|																							|
|							2114	8T28				ROM0								|
|			LS163	LS157					LS174											|
|							2114	8T28				ROM1								|
|	CA3130	LS163	LS157					LS02											|
|							2114	8T28				ROM2								|
|	LS14	LS163	LS157					LS04											|
|									SW1					ROM3								|
|	7417	7404	LS20	ROM5	7408	LS139											|
|														ROM4								|
| -	74123	7474	LS163	LS165	7474													|
|C|																							|
|N|			7476	7400	LS157	LS93	LS04		6850								|
|2|																							|
| -	7403	74123	3.7MHz	LS86	LS163	LS20									CN4		|
|-------------------------------------------------------------------------------------------|

Notes:
    All IC's shown.

    ROM0-5  - 
	6502	- 
	6850	- Asynchronous Communications Interface Adapter
	8T28	- 4-Bit Bidirectional Bus Transceiver
	CA3130	- Operational Amplifier
	CN1		- OSI-48 bus connector
	CN2		- 
	CN3		- 
	CN4		- sound connector
	SW1		- 

*/

/*

Compukit UK101

PCB Layout
----------

|-------------------------------------------------------------------------------------------|
|					2114	2114															|
|									LS138				CN1									|
|		IC1			2114	2114															|
|									7404			LS75	LS75							|
|					2114	2114															|
|									LS138													|
|					2114	2114					LS125	LS125							|
|									LS138													|
| -					2114*	2114*					8T28*	8T28*							|
| |																							|
|C|					2114*	2114*															|
|N|		CN4																					|
|3|					2114*	2114*						6502								|
| |																							|
| -					2114*	2114*															|
|																							|
|							2114	8T28				ROM0								|
|			74163	LS157																	|
|							2114	8T28				ROM1								|
|	IC66'	74163	LS157					7402											|
|														ROM2								|
|	IC67'	74163	LS157					7404											|
|														ROM3								|
|	IC68'	7404	7420	ROM5			LS139											|
|														ROM4								|
| -	LS123	7474	74163	74165	LS123													|
|C|																							|
|N|			7476	7400			7493	7404		6850								|
|2|																							|
| -	7403	LS123	8MHz			74163	LS20											|
|-------------------------------------------------------------------------------------------|

Notes:
    All IC's shown.

    ROM0-5  - 
	6502	- 
	6850	- Asynchronous Communications Interface Adapter
	8T28	- 4-Bit Bidirectional Bus Transceiver
	CN1		- OSI-48 Bus Connector
	CN2		- 
	CN3		- 
	CN4		- UHF Modulator Output
	*		- present when 8KB of RAM installed
	'		- present when cassette option installed

*/

/*

Ohio Scientific Single Sided Floppy Interface

PCB Layout
----------

OSI 470 REV B

|-------------------------------------------------------------------|
|																	|
|												8T26	8T26		|
|																	|
|		7417														|
|																	|
|						6820				6850		7400  U2C	|
|		7417														|
|																	|
|																	|
|		7417												7404	|
|																	|
|																	|
|		7417														|
|																	|
|					7400	7404	74123	74123	7410			|
|	4MHz															|
|																	|
|																	|
|	7400	7493	74390	74390	74390	7404	7430	7404	|
|																	|
|																	|
|					U6A												|
|																	|
|-------------------------------------------------------------------|

Notes:
    All IC's shown.

	6850	- Asynchronous Communications Interface Adapter
	6820	- Peripheral Interface Adapter
	8T26	- 4-Bit Bidirectional Bus Transceiver
	U2C		- PROTO?
	U6A		- PROTO?

*/

/*

	TODO:

	- fix uk101 video to 64x16
	- floppy PIA is actually a 6820
	- break key
	- power on reset
	- Superboard II revisions A/C/D
	- uk101 medium resolution graphics
	- uk101 ay-3-8910 sound
	- cassette
	- faster cassette
	- floppy
	- wemon?

*/

#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "includes/osi.h"
#include "machine/6850acia.h"
#include "machine/6821pia.h"
#include "sound/discrete.h"
#include "sound/beep.h"
#include "devices/flopdrv.h"
#include "formats/basicdsk.h"
#include "devices/cassette.h"

/* Sound */

static const discrete_dac_r1_ladder osi600_dac =
{
	4,			// size of ladder
	{ 180, 180, 180, 180, 0, 0, 0, 0 }, // R68, R69, R70, R71
	5,			// 5V
	RES_K(1),	// R67
	0,			// no rGnd
	0			// no cFilter
};

static DISCRETE_SOUND_START( osi600_discrete_interface )
	DISCRETE_INPUT_DATA(NODE_01)

	DISCRETE_DAC_R1(NODE_02, NODE_01, DEFAULT_TTL_V_LOGIC_1, &osi600_dac)
	DISCRETE_CRFILTER(NODE_03, 1, NODE_02, 1.0/(1.0/RES_K(1)+1.0/180+1.0/180+1.0/180+1.0/180), CAP_U(0.1))
	DISCRETE_OUTPUT(NODE_03, 100)
	DISCRETE_GAIN(NODE_04, NODE_03, 32767.0/5)
	DISCRETE_OUTPUT(NODE_04, 100)
DISCRETE_SOUND_END

static const discrete_dac_r1_ladder osi600c_dac =
{
	8,			// size of ladder
	{ RES_K(68), RES_K(33), RES_K(16), RES_K(8.2), RES_K(3.9), RES_K(2), RES_K(1), 510 }, // R73, R71, R70, R67, R68, R69, R75, R74
	5,			// 5V
	510,		// R86
	0,			// no rGnd
	CAP_U(33)	// C63
};

static DISCRETE_SOUND_START( osi600c_discrete_interface )
	DISCRETE_INPUT_DATA(NODE_01)
	DISCRETE_INPUT_LOGIC(NODE_10)

	DISCRETE_DAC_R1(NODE_02, NODE_01, DEFAULT_TTL_V_LOGIC_1, &osi600_dac)
	DISCRETE_ONOFF(NODE_03, NODE_10, NODE_02)
	DISCRETE_OUTPUT(NODE_03, 100)
DISCRETE_SOUND_END

/* Keyboard */

static READ8_HANDLER( osi600_keyboard_r )
{
	osi_state *state = space->machine->driver_data;

	static const char *const keynames[] = { "ROW0", "ROW1", "ROW2", "ROW3", "ROW4", "ROW5", "ROW6", "ROW7" };

	UINT8 data = 0xff;
	int bit;

	for (bit = 0; bit < 8; bit++)
	{
		if (!BIT(state->keylatch, bit)) data &= input_port_read(space->machine, keynames[bit]);
	}

	return data;
}

static WRITE8_HANDLER( osi600_keyboard_w )
{
	const device_config *discrete = devtag_get_device(space->machine, "discrete");
	osi_state *state = space->machine->driver_data;

	state->keylatch = data;

	discrete_sound_w(discrete, NODE_01, (data >> 2) & 0x0f);
}

static WRITE8_HANDLER( uk101_keyboard_w )
{
	osi_state *state = space->machine->driver_data;

	state->keylatch = data;
}

static WRITE8_HANDLER( osi600_ctrl_w )
{
	/*

		bit		signal			description

		0		_32				screen size (0=32x32, 1=64x16)
		1		COLOR EN		color enable
		2		BK0			
		3		BK1			
		4		DAC DISABLE		DAC sound enable
		5		
		6		
		7		

	*/

	const device_config *discrete = devtag_get_device(space->machine, "discrete");
	osi_state *state = space->machine->driver_data;

	state->_32 = BIT(data, 0);
	state->coloren = BIT(data, 1);

	discrete_sound_w(discrete, NODE_10, BIT(data, 4));
}

static WRITE8_HANDLER( osi630_ctrl_w )
{
	const device_config *speaker = devtag_get_device(space->machine, "beep");
	/*

		bit		description

		0		AC control enable
		1		tone generator enable
		2		modem select (0 = printer, 1 = modem)
		3		
		4		
		5		
		6		
		7		

	*/

	beep_set_state(speaker, BIT(data, 1));
}

static WRITE8_HANDLER( osi630_sound_w )
{
	const device_config *speaker = devtag_get_device(space->machine, "beep");
	if (data) beep_set_frequency(speaker, 49152/data);
}

/* Disk Drive */

/*
	C000 FLOPIN			FLOPPY DISK STATUS PORT

	BIT	FUNCTION
	0	DRIVE 0 READY (0 IF READY)
	1	TRACK 0 (0 IF AT TRACK 0)
	2	FAULT (0 IF FAULT)
	3
	4	DRIVE 1 READY (0 IF READY)
	5	WRITE PROTECT (0 IF WRITE PROTECT)
	6	DRIVE SELECT (1 = A OR C, 0 = B OR D)
	7	INDEX (0 IF AT INDEX HOLE)

	C002 FLOPOT			FLOPPY DISK CONTROL PORT

	BIT	FUNCTION
	0	WRITE ENABLE (0 ALLOWS WRITING)
	1	ERASE ENABLE (0 ALLOWS ERASING)
		ERASE ENABLE IS ON 200us AFTER WRITE IS ON
		ERASE ENABLE IS OFF 530us AFTER WRITE IS OFF
	2	STEP BIT : INDICATES DIRECTION OF STEP (WAIT 10us FIRST)
		0 INDICATES STEP TOWARD 76
		1 INDICATES STEP TOWARD 0
	3	STEP (TRANSITION FROM 1 TO 0)
		MUST HOLD AT LEAST 10us, MIN 8us BETWEEN
	4	FAULT RESET (0 RESETS)
	5	SIDE SELECT (1 = A OR B, 0 = C OR D)
	6	LOW CURRENT (0 FOR TRKS 43-76, 1 FOR TRKS 0-42)
	7	HEAD LOAD (0 TO LOAD : MUST WAIT 40ms AFTER)

	C010 ACIA			DISK CONTROLLER ACIA STATUS PORT
	C011 ACIAIO			DISK CONTROLLER ACIA I/O PORT
*/

static void osi470_index_callback(const device_config *controller, const device_config *img, int state)
{
	osi_state *driver_state = img->machine->driver_data;

	driver_state->fdc_index = state;
}

static READ8_DEVICE_HANDLER( osi470_pia_a_r )
{

	/*

		bit		description

		0		_READY DRIVE 1
		1		_TRACK 00
		2		_FAULT
		3		_SECTOR
		4		_READY DRIVE 2
		5		_WRITE PROTECT
		6		
		7		_INDEX

	*/

	osi_state *state = device->machine->driver_data;

	return (state->fdc_index << 7);
}

static WRITE8_DEVICE_HANDLER( osi470_pia_a_w )
{
	/*

		bit		description

		0		
		1		
		2		
		3		
		4		
		5		
		6		drive select
		7		

	*/
}

static WRITE8_DEVICE_HANDLER( osi470_pia_b_w )
{
	/*

		bit		description

		0		_WRITE ENABLE
		1		_ERASE ENABLE
		2		_STEP IN
		3		_STEP
		4		_FAULT RESET
		5		side select
		6		_LOW CURRENT
		7		_HEAD LOAD

	*/
}

static WRITE8_DEVICE_HANDLER( osi470_pia_cb2_w )
{
}

static const pia6821_interface osi470_pia_intf =
{
	DEVCB_HANDLER(osi470_pia_a_r),
	DEVCB_NULL, // read8_machine_func in_b_func,
	DEVCB_NULL, // read8_machine_func in_ca1_func,
	DEVCB_NULL, // read8_machine_func in_cb1_func,
	DEVCB_NULL, // read8_machine_func in_ca2_func,
	DEVCB_NULL, // read8_machine_func in_cb2_func,
	DEVCB_HANDLER(osi470_pia_a_w),
	DEVCB_HANDLER(osi470_pia_b_w),
	DEVCB_NULL, // write8_machine_func out_ca2_func,
	DEVCB_HANDLER(osi470_pia_cb2_w),
	DEVCB_NULL, // void (*irq_a_func)(int state),
	DEVCB_NULL, // void (*irq_b_func)(int state),
};

static const pia6821_interface pia_dummy_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

/* Memory Maps */

static ADDRESS_MAP_START( osi600_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_RAMBANK(1)
	AM_RANGE(0xa000, 0xbfff) AM_ROM
	AM_RANGE(0xd000, 0xd3ff) AM_RAM AM_BASE_MEMBER(osi_state, video_ram)
	AM_RANGE(0xdf00, 0xdf00) AM_READWRITE(osi600_keyboard_r, osi600_keyboard_w)
	AM_RANGE(0xf000, 0xf000) AM_DEVREADWRITE("acia_0", acia6850_stat_r, acia6850_ctrl_w)
	AM_RANGE(0xf001, 0xf001) AM_DEVREADWRITE("acia_0", acia6850_data_r, acia6850_data_w)
	AM_RANGE(0xf800, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( uk101_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_RAMBANK(1)
	AM_RANGE(0xa000, 0xbfff) AM_ROM
	AM_RANGE(0xd000, 0xd3ff) AM_RAM AM_BASE_MEMBER(osi_state, video_ram)
	AM_RANGE(0xdf00, 0xdf00) AM_MIRROR(0x03ff) AM_READWRITE(osi600_keyboard_r, uk101_keyboard_w)
	AM_RANGE(0xf000, 0xf000) AM_MIRROR(0x00fe) AM_DEVREADWRITE("acia_0", acia6850_stat_r, acia6850_ctrl_w)
	AM_RANGE(0xf001, 0xf001) AM_MIRROR(0x00fe) AM_DEVREADWRITE("acia_0", acia6850_data_r, acia6850_data_w)
	AM_RANGE(0xf800, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( c1p_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x4fff) AM_RAMBANK(1)
	AM_RANGE(0xa000, 0xbfff) AM_ROM
	AM_RANGE(0xc704, 0xc707) AM_DEVREADWRITE("pia_1", pia6821_r, pia6821_w)
	AM_RANGE(0xc708, 0xc70b) AM_DEVREADWRITE("pia_2", pia6821_r, pia6821_w)
	AM_RANGE(0xc70c, 0xc70f) AM_DEVREADWRITE("pia_3", pia6821_r, pia6821_w)
	AM_RANGE(0xd000, 0xd3ff) AM_RAM AM_BASE_MEMBER(osi_state, video_ram)
	AM_RANGE(0xd400, 0xd7ff) AM_RAM AM_BASE_MEMBER(osi_state, color_ram)
	AM_RANGE(0xd800, 0xd800) AM_WRITE(osi600_ctrl_w)
	AM_RANGE(0xdf00, 0xdf00) AM_READWRITE(osi600_keyboard_r, osi600_keyboard_w)
	AM_RANGE(0xf000, 0xf000) AM_DEVREADWRITE("acia_0", acia6850_stat_r, acia6850_ctrl_w)
	AM_RANGE(0xf001, 0xf001) AM_DEVREADWRITE("acia_0", acia6850_data_r, acia6850_data_w)
	AM_RANGE(0xf7c0, 0xf7c0) AM_WRITE(osi630_sound_w)
	AM_RANGE(0xf7e0, 0xf7e0) AM_WRITE(osi630_ctrl_w)
	AM_RANGE(0xf800, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( c1pmf_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x4fff) AM_RAMBANK(1)
	AM_RANGE(0xa000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xc003) AM_DEVREADWRITE("pia_0", pia6821_r, pia6821_w) // FDC
	AM_RANGE(0xc010, 0xc010) AM_DEVREADWRITE("acia_1", acia6850_stat_r, acia6850_ctrl_w)
	AM_RANGE(0xc011, 0xc011) AM_DEVREADWRITE("acia_1", acia6850_data_r, acia6850_data_w)
	AM_RANGE(0xc704, 0xc707) AM_DEVREADWRITE("pia_1", pia6821_r, pia6821_w)
	AM_RANGE(0xc708, 0xc70b) AM_DEVREADWRITE("pia_2", pia6821_r, pia6821_w)
	AM_RANGE(0xc70c, 0xc70f) AM_DEVREADWRITE("pia_3", pia6821_r, pia6821_w)
	AM_RANGE(0xd000, 0xd3ff) AM_RAM AM_BASE_MEMBER(osi_state, video_ram)
	AM_RANGE(0xd400, 0xd7ff) AM_RAM AM_BASE_MEMBER(osi_state, color_ram)
	AM_RANGE(0xd800, 0xd800) AM_WRITE(osi600_ctrl_w)
	AM_RANGE(0xdf00, 0xdf00) AM_READWRITE(osi600_keyboard_r, osi600_keyboard_w)
	AM_RANGE(0xf000, 0xf000) AM_DEVREADWRITE("acia_0", acia6850_stat_r, acia6850_ctrl_w)
	AM_RANGE(0xf001, 0xf001) AM_DEVREADWRITE("acia_0", acia6850_data_r, acia6850_data_w)
	AM_RANGE(0xf7c0, 0xf7c0) AM_WRITE(osi630_sound_w)
	AM_RANGE(0xf7e0, 0xf7e0) AM_WRITE(osi630_ctrl_w)
	AM_RANGE(0xf800, 0xffff) AM_ROM
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( osi600 )
	PORT_START("ROW0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SHIFT LOCK") PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK)) PORT_TOGGLE
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RIGHT SHIFT") PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LEFT SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_TAB) PORT_CHAR(27)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("REPEAT") PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\')

	PORT_START("ROW1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')

	PORT_START("ROW2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_X) PORT_CHAR('X')

	PORT_START("ROW3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_S) PORT_CHAR('S')

	PORT_START("ROW4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_W) PORT_CHAR('W')

	PORT_START("ROW5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LINE FEED") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR(10)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')

	PORT_START("ROW6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RUB OUT") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')

	PORT_START("ROW7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('\"')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
INPUT_PORTS_END

static INPUT_PORTS_START( uk101 )
	PORT_INCLUDE(osi600)

	PORT_MODIFY("ROW0")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC) PORT_CHAR(27)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_TILDE) PORT_CHAR('~')

	PORT_MODIFY("ROW5")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\')
INPUT_PORTS_END

/* Machine Start */

static READ_LINE_DEVICE_HANDLER( cassette_rx )
{
	osi_state *driver_state = device->machine->driver_data;

	return (cassette_input(driver_state->cassette) > 0.0) ? 1 : 0;
}

static WRITE_LINE_DEVICE_HANDLER( cassette_tx )
{
	osi_state *driver_state = device->machine->driver_data;

	cassette_output(driver_state->cassette, state ? +1.0 : -1.0);
}

static ACIA6850_INTERFACE( osi600_acia_intf )
{
	X1/32,
	X1/32,
	DEVCB_LINE(cassette_rx),
	DEVCB_LINE(cassette_tx),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

static ACIA6850_INTERFACE( uk101_acia_intf )
{
	500000, //
	500000, //
	DEVCB_LINE(cassette_rx),
	DEVCB_LINE(cassette_tx),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

static ACIA6850_INTERFACE( osi470_acia_intf )
{
	0,				// clocked in from the floppy drive
	XTAL_4MHz/8,	// 250 kHz
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

static MACHINE_START( osi600 )
{
	osi_state *state = machine->driver_data;

	const address_space *program = cputag_get_address_space(machine, M6502_TAG, ADDRESS_SPACE_PROGRAM);

	/* find devices */
	state->cassette = devtag_get_device(machine, CASSETTE_TAG);

	/* configure RAM banking */
	memory_configure_bank(machine, 1, 0, 1, memory_region(machine, M6502_TAG), 0);
	memory_set_bank(machine, 1, 0);

	switch (mess_ram_size)
	{
	case 4*1024:
		memory_install_readwrite8_handler(program, 0x0000, 0x0fff, 0, 0, SMH_BANK(1), SMH_BANK(1));
		memory_install_readwrite8_handler(program, 0x1000, 0x1fff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		break;

	case 8*1024:
		memory_install_readwrite8_handler(program, 0x0000, 0x1fff, 0, 0, SMH_BANK(1), SMH_BANK(1));
		break;
	}

	/* register for state saving */
	state_save_register_global(machine, state->keylatch);
	state_save_register_global_pointer(machine, state->video_ram, OSI600_VIDEORAM_SIZE);
}

static MACHINE_START( c1p )
{
	osi_state *state = machine->driver_data;

	const address_space *program = cputag_get_address_space(machine, M6502_TAG, ADDRESS_SPACE_PROGRAM);

	/* find devices */
	state->cassette = devtag_get_device(machine, CASSETTE_TAG);

	/* configure RAM banking */
	memory_configure_bank(machine, 1, 0, 1, memory_region(machine, M6502_TAG), 0);
	memory_set_bank(machine, 1, 0);

	switch (mess_ram_size)
	{
	case 8*1024:
		memory_install_readwrite8_handler(program, 0x0000, 0x1fff, 0, 0, SMH_BANK(1), SMH_BANK(1));
		memory_install_readwrite8_handler(program, 0x2000, 0x4fff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		break;

	case 20*1024:
		memory_install_readwrite8_handler(program, 0x0000, 0x4fff, 0, 0, SMH_BANK(1), SMH_BANK(1));
		break;
	}

	/* register for state saving */
	state_save_register_global(machine, state->keylatch);
	state_save_register_global(machine, state->_32);
	state_save_register_global(machine, state->coloren);
	state_save_register_global_pointer(machine, state->video_ram, OSI600_VIDEORAM_SIZE);
	state_save_register_global_pointer(machine, state->color_ram, OSI630_COLORRAM_SIZE);
}

static MACHINE_START( c1pmf )
{
	MACHINE_START_CALL(c1p);

	/* set floppy index hole callback */
	floppy_drive_set_index_pulse_callback(floppy_get_device(machine, 0), osi470_index_callback);
}

static FLOPPY_OPTIONS_START(osi)
	FLOPPY_OPTION(osi, "img", "OSI disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([36])
		SECTORS([10])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([0]))
FLOPPY_OPTIONS_END

static const floppy_config osi_floppy_config =
{
	FLOPPY_DRIVE_SS_40,
	FLOPPY_OPTIONS_NAME(osi),
	DO_NOT_KEEP_GEOMETRY
};

/* Machine Drivers */

static MACHINE_DRIVER_START( osi600 )
	MDRV_DRIVER_DATA(osi_state)

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502_TAG, M6502, X1/4) // .98304 MHz
	MDRV_CPU_PROGRAM_MAP(osi600_mem)

	MDRV_MACHINE_START(osi600)

    /* video hardware */
	MDRV_IMPORT_FROM(osi600_video)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("discrete", DISCRETE, 0)
	MDRV_SOUND_CONFIG_DISCRETE(osi600_discrete_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* cassette ACIA */
	MDRV_ACIA6850_ADD("acia_0", osi600_acia_intf)

	/* cassette */
	MDRV_CASSETTE_ADD("cassette", default_cassette_config)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( uk101 )
	MDRV_DRIVER_DATA(osi_state)

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502_TAG, M6502, UK101_X1/8) // 1 MHz
	MDRV_CPU_PROGRAM_MAP(uk101_mem)

	MDRV_MACHINE_START(osi600)

    /* video hardware */
	MDRV_IMPORT_FROM(uk101_video)

	/* cassette ACIA */
	MDRV_ACIA6850_ADD("acia_0", uk101_acia_intf)

	/* cassette */
	MDRV_CASSETTE_ADD("cassette", default_cassette_config)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( c1p )
	MDRV_DRIVER_DATA(osi_state)

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502_TAG, M6502, X1/4) // .98304 MHz
	MDRV_CPU_PROGRAM_MAP(c1p_mem)

	MDRV_MACHINE_START(c1p)

    /* video hardware */
	MDRV_IMPORT_FROM(osi630_video)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("discrete", DISCRETE, 0)
	MDRV_SOUND_CONFIG_DISCRETE(osi600c_discrete_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
	MDRV_SOUND_ADD("beep", BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MDRV_PIA6821_ADD( "pia_1", pia_dummy_intf )
	MDRV_PIA6821_ADD( "pia_2", pia_dummy_intf )
	MDRV_PIA6821_ADD( "pia_3", pia_dummy_intf )

	/* cassette ACIA */
	MDRV_ACIA6850_ADD("acia_0", osi600_acia_intf)

	/* cassette */
	MDRV_CASSETTE_ADD("cassette", default_cassette_config)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( c1pmf )
	MDRV_IMPORT_FROM(c1p)

	MDRV_CPU_MODIFY(M6502_TAG)
	MDRV_CPU_PROGRAM_MAP(c1pmf_mem)

	MDRV_MACHINE_START(c1pmf)

	MDRV_PIA6821_ADD( "pia_0", osi470_pia_intf )

	/* floppy ACIA */
	MDRV_ACIA6850_ADD("acia_1", osi470_acia_intf)
	
	MDRV_FLOPPY_DRIVE_ADD(FLOPPY_0, osi_floppy_config)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( sb2m600b )
	ROM_REGION( 0x10000, M6502_TAG, 0 )
	ROM_LOAD( "basus01.u9",  0xa000, 0x0800, CRC(f4f5dec0) SHA1(b41bf24b4470b6e969d32fe48d604637276f846e) )
	ROM_LOAD( "basus02.u10", 0xa800, 0x0800, CRC(0039ef6a) SHA1(1397f0dc170c16c8e0c7d02e63099e986e86385b) )
	ROM_LOAD( "basus03.u11", 0xb000, 0x0800, CRC(ca25f8c1) SHA1(f5e8ee93a5e0656657d0cc60ef44e8a24b8b0a80) )
	ROM_LOAD( "basus04.u12", 0xb800, 0x0800, CRC(8ee6030e) SHA1(71f210163e4268cba2dd78a97c4d8f5dcebf980e) )
	ROM_LOAD( "monde01.u13", 0xf800, 0x0800, CRC(95a44d2e) SHA1(4a0241c4015b94c436d0f0f58b3dd9d5207cd847) )

	ROM_REGION( 0x800, "chargen",0)
	ROM_LOAD( "chgsup2.u41", 0x0000, 0x0800, CRC(735f5e0a) SHA1(87c6271497c5b00a974d905766e91bb965180594) )
ROM_END

#define rom_c1p rom_sb2m600b
#define rom_c1pmf rom_sb2m600b

ROM_START( uk101 )
	ROM_REGION( 0x10000, M6502_TAG, 0 )
	ROM_LOAD( "basuk01.ic9",   0xa000, 0x0800, CRC(9d3caa92) SHA1(b2c3d1af0c4f3cead1dbd44aaf5a11680880f772) )
	ROM_LOAD( "basus02.ic10",  0xa800, 0x0800, CRC(0039ef6a) SHA1(1397f0dc170c16c8e0c7d02e63099e986e86385b) )
	ROM_LOAD( "basuk03.ic11",  0xb000, 0x0800, CRC(0d011242) SHA1(54bd33522a5d1991086eeeff3a4f73c026be45b6) )
	ROM_LOAD( "basuk04.ic12",  0xb800, 0x0800, CRC(667223e8) SHA1(dca78be4b98317413376d69119942d692e39575a) )
	ROM_LOAD( "monuk02.ic13",  0xf800, 0x0800, CRC(04ac5822) SHA1(2bbbcd0ca18103fd68afcf64a7483653b925d83e) )

	ROM_REGION( 0x800, "chargen", 0 )
	ROM_LOAD( "chguk101.ic41", 0x0000, 0x0800, CRC(fce2c84a) SHA1(baa66a7a48e4d62282671ef53abfaf450b888b70) )
ROM_END

/* Driver Initialization */

static TIMER_CALLBACK( setup_beep )
{
	const device_config *speaker = devtag_get_device(machine, "beep");
	beep_set_state(speaker, 0);
	beep_set_frequency(speaker, 300);
}

static DRIVER_INIT( c1p )
{
	timer_set(machine, attotime_zero, NULL, 0, setup_beep);
}

/* System Configuration */

static SYSTEM_CONFIG_START( osi600 )
	CONFIG_RAM_DEFAULT( 4 * 1024 )
	CONFIG_RAM		  ( 8 * 1024 )
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START( uk101 )
	CONFIG_RAM_DEFAULT( 4 * 1024 )
	CONFIG_RAM		  ( 8 * 1024 )
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START( c1p )
	CONFIG_RAM_DEFAULT( 8 * 1024 )
	CONFIG_RAM		  (20 * 1024 )
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START( c1pmf )
	CONFIG_RAM_DEFAULT(20 * 1024 )
SYSTEM_CONFIG_END

/* System Drivers */

//    YEAR  NAME		PARENT		COMPAT	MACHINE		INPUT		INIT	CONFIG		COMPANY            FULLNAME
COMP( 1978, sb2m600b,	0,			0,		osi600,		osi600,		0, 		osi600,		"Ohio Scientific", "Superboard II Model 600 (Rev. B)", 0)
//COMP( 1980, sb2m600c,	0,			0,		osi600c,	osi600,		0, 		osi600,		"Ohio Scientific", "Superboard II Model 600 (Rev. C)", 0)
COMP( 1980, c1p,		sb2m600b,	0,		c1p,		osi600,		c1p,	c1p,		"Ohio Scientific", "Challenger 1P Series 2", GAME_NOT_WORKING)
COMP( 1980, c1pmf,		sb2m600b,	0,		c1pmf,		osi600,		c1p,	c1pmf,		"Ohio Scientific", "Challenger 1P MF Series 2", GAME_NOT_WORKING)
COMP( 1979,	uk101,		sb2m600b,	0,		uk101,		uk101,		0, 		uk101,		"Compukit",        "UK101", GAME_NOT_WORKING)
