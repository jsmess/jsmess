/******************************************************************************
*
*  Fidelity Electronics Z80 based board driver
*  By Kevin 'kevtris' Horton and Jonathan Gevaryahu AKA Lord Nightmare
*
*  All detailed RE work done by Kevin 'kevtris' Horton
*
*  TODO:
*  * Inputs: 4x4 keypad matrix
*  * LED displays (four 7seg, two leds, currently is using terminal as a hack so it doesn't crash due to the screenless system bug)
     The four arrays are populated with the proper data, all thats needed is a callback to draw said data to screen or to artwork
*  * Figure out why it says the first speech line twice; it shouldn't.
*  * Get rom locations from pcb
*  * Add sensory chess challenger to driver, similar hardware.
*
***********************************************************************

Voice Chess Challenger
----------------------

The CPU is a Z80 running at 4MHz.  The TSI chip runs at around 25KHz, using a
470K / 100pf RC network.  This system is very very basic, and is composed of just
the Z80, 4 ROMs, the TSI chip, and an 8255.


The Z80's interrupt inputs are all pulled to VCC, so no interrupts are used.

Reset is connected to a power-on reset circuit and a button on the keypad (marked RE).

The TSI chip connects to a 4K ROM marked VCC-ENGL.  All of the Voice Chess Challengers
use this same ROM  (three).  The later chess boards use a slightly different part
number, but the contents are identical.

Memory map:
-----------
0000-0FFF: 4K ROM 101-32103
1000-1FFF: 4K ROM VCC2
2000-3FFF: 4K ROM VCC3 [LN edit: should be 2000-2FFF]
4000-5FFF: 1K RAM (2114 * 2)
6000-FFFF: empty

I/O map:
--------
00-FF: 8255 port chip [LN edit: 00-03, mirrored over the 00-FF range; program accesses F4-F7]


8255 connections:
-----------------

PA.0 - segment G, TSI A0
PA.1 - segment F, TSI A1
PA.2 - segment E, TSI A2
PA.3 - segment D, TSI A3
PA.4 - segment C, TSI A4
PA.5 - segment B, TSI A5
PA.6 - segment A, language latch Data
PA.7 - TSI START line, language latch clock (see below)

PB.0 - dot commons
PB.1 - NC
PB.2 - digit 0, bottom dot
PB.3 - digit 1, top dot
PB.4 - digit 2
PB.5 - digit 3
PB.6 - enable language switches (see below)
PB.7 - TSI DONE line

(button rows pulled up to 5V thru 2.2K resistors)
PC.0 - button row 0, german language jumper
PC.1 - button row 1, french language jumper
PC.2 - button row 2, spanish language jumper
PC.3 - button row 3, special language jumper
PC.4 - button column A
PC.5 - button column B
PC.6 - button column C
PC.7 - button column D


language switches:
------------------

When PB.6 is pulled low, the language switches can be read.  There are four.
They connect to the button rows.  When enabled, the row(s) will read low if
the jumper is present.  English only VCC's do not have the 367 or any pads stuffed.
The jumpers are labelled: french, german, spanish, and special.


language latch:
---------------

There's an unstuffed 7474 on the board that connects to PA.6 and PA.7.  It allows
one to latch the state of A12 to the speech ROM.  The english version has the chip
missing, and a jumper pulling "A12" to ground.  This line is really a negative
enable.

To make the VCC multi-language, one would install the 74367 (note: it must be a 74367
or possibly a 74LS367.  A 74HC367 would not work since they rely on the input current
to keep the inputs pulled up), solder a piggybacked ROM to the existing english
speech ROM, and finally install a 7474 dual flipflop.

This way, the game can then detect which secondary language is present, and then it can
automatically select the correct ROM(s).  I have to test wether it will do automatic
determination and give you a language option on power up or something.

***********************************************************************

Sensory Chess Challenger
add me

******************************************************************************/

/* Core includes */
#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/i8255a.h"
#include "sound/s14001a.h"
//#include "dectalk.lh" //  hack to avoid screenless system crash
#include "machine/terminal.h"

/* Defines */

/* Components */

static struct
{
UINT8 data[8]; // what is this for? to suppress the scalar initializer warning?
UINT8 led_7seg_data[4]; // data for the 4x 7seg leds, bits are 0bxABCDEFG
UINT8 led_data; // data for the two individual leds, in 0bxxxx12xx format
UINT8 led_selected; // 5 bit selects for 7 seg leds and for common other leds, bits are (7seg leds are 0 1 2 3, common other leds are C) 0bxx3210xc
UINT8 columnlatch;
} fidelz80={ {0}};

/* Devices */

/* I8255 Device */

/*static READ8_DEVICE_HANDLER( vcc_porta_r )
{
    return 0;
};*/

static READ8_DEVICE_HANDLER( vcc_portb_r )
{
	device = device->machine->device("speech");
	if (s14001a_bsy_r(device) != 0)
		return 0x80;
	else
		return 0;
};

static READ8_DEVICE_HANDLER( vcc_portc_r )
{
	return 0xFF; // TODO: emulate button input
};

static WRITE8_DEVICE_HANDLER( vcc_porta_w )
{
	device = device->machine->device("speech");
	s14001a_set_volume(device, 15); // hack, s14001a core should assume a volume of 15 unless otherwise stated...
	s14001a_reg_w(device, data & 0x3f);
	s14001a_rst_w(device, (data & 0x80)>>7);
	/* handle digits below */
	if (fidelz80.led_selected & 0x4)
		fidelz80.led_7seg_data[0] = data&0x7F;
	if (fidelz80.led_selected & 0x8)
		fidelz80.led_7seg_data[1] = data&0x7F;
	if (fidelz80.led_selected & 0x10)
		fidelz80.led_7seg_data[2] = data&0x7F;
	if (fidelz80.led_selected & 0x20)
		fidelz80.led_7seg_data[3] = data&0x7F;
};

static WRITE8_DEVICE_HANDLER( vcc_portb_w )
{
	if (data&0x1) // common for two leds
	{
		fidelz80.led_data = (data&0xc);
	}
	fidelz80.led_selected = (data&0x3d);
	// ignoring the language switch enable for now, is bit 0x40
};

static WRITE8_DEVICE_HANDLER( vcc_portc_w )
{
	fidelz80.columnlatch = (data&0x80);
};

static I8255A_INTERFACE( vcc_ppi8255_intf )
{
	DEVCB_NULL, // only bit 6 is readable (and only sometimes) and I'm not emulating the language latch unless needed
	DEVCB_HANDLER(vcc_portb_r), // bit 7 is readable and is the done line from the s14001a
	DEVCB_HANDLER(vcc_portc_r), // bits 0,1,2,3 are readable, have to do with input
	DEVCB_HANDLER(vcc_porta_w), // display segments and s14001a lines
	DEVCB_HANDLER(vcc_portb_w), // display digits and led dots
	DEVCB_HANDLER(vcc_portc_w), // bits 4,5,6,7 are writable, have to do with input
};

static MACHINE_RESET( fidelz80 )
{
	// data for the 4x 7seg leds, bits are 0bxABCDEFG
	fidelz80.led_7seg_data[0] = 0;
	fidelz80.led_7seg_data[1] = 0;
	fidelz80.led_7seg_data[2] = 0;
	fidelz80.led_7seg_data[3] = 0;
	fidelz80.led_data = 0;
	fidelz80.led_selected = 0;
	fidelz80.columnlatch = 0;
}



/******************************************************************************
 Address Maps
******************************************************************************/

static ADDRESS_MAP_START(vcc_z80_mem, ADDRESS_SPACE_PROGRAM, 8)
    ADDRESS_MAP_UNMAP_HIGH
    AM_RANGE(0x0000, 0x0fff) AM_ROM // 4k rom
	AM_RANGE(0x1000, 0x1fff) AM_ROM // 4k rom
	AM_RANGE(0x2000, 0x2fff) AM_ROM // 4k rom
	AM_RANGE(0x4000, 0x43ff) AM_RAM AM_MIRROR(0x1c00) // 1k ram (2114*2) mirrored 8 times
ADDRESS_MAP_END

static ADDRESS_MAP_START(vcc_z80_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x03) AM_MIRROR(0xFC) AM_DEVREADWRITE("vcc_ppi8255", i8255a_r, i8255a_w) // 8255 i/o chip
ADDRESS_MAP_END


/******************************************************************************
 Input Ports
******************************************************************************/
static INPUT_PORTS_START( fidelz80 )
	PORT_INCLUDE(generic_terminal)
INPUT_PORTS_END

/******************************************************************************
 Machine Drivers
******************************************************************************/

static WRITE8_DEVICE_HANDLER( null_kbd_put )
{
}

static GENERIC_TERMINAL_INTERFACE( dectalk_terminal_intf )
{
	DEVCB_HANDLER(null_kbd_put)
};

static MACHINE_CONFIG_START( vcc, driver_device )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu", Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(vcc_z80_mem)
    MDRV_CPU_IO_MAP(vcc_z80_io)
    MDRV_QUANTUM_TIME(HZ(60))
    MDRV_MACHINE_RESET(fidelz80)

    /* video hardware */
    //MDRV_DEFAULT_LAYOUT(layout_dectalk) // hack to avoid screenless system crash
	MDRV_FRAGMENT_ADD( generic_terminal )
	MDRV_GENERIC_TERMINAL_ADD(TERMINAL_TAG,dectalk_terminal_intf)

	/* other hardware */
	MDRV_I8255A_ADD("vcc_ppi8255", vcc_ppi8255_intf)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speech", S14001A, 25000) // around 25khz
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

MACHINE_CONFIG_END



/******************************************************************************
 ROM Definitions
******************************************************************************/

ROM_START(vcc)

    ROM_REGION(0x10000, "maincpu", 0)
    ROM_LOAD("101-32103.bin", 0x0000, 0x1000, CRC(257BB5AB) SHA1(F7589225BB8E5F3EAC55F23E2BD526BE780B38B5)) // 32014.VCC???
    ROM_LOAD("vcc2.bin", 0x1000, 0x1000, CRC(F33095E7) SHA1(692FCAB1B88C910B74D04FE4D0660367AEE3F4F0))
    ROM_LOAD("vcc3.bin", 0x2000, 0x1000, CRC(624F0CD5) SHA1(7C1A4F4497FE5882904DE1D6FECF510C07EE6FC6))

    ROM_REGION(0x2000, "speech", 0)
    ROM_LOAD("vcc-engl.bin", 0x0000, 0x1000, CRC(F35784F9) SHA1(348E54A7FA1E8091F89AC656B4DA22F28CA2E44D))
ROM_END



/******************************************************************************
 Drivers
******************************************************************************/

/*    YEAR  NAME        PARENT      COMPAT  MACHINE     INPUT   INIT      COMPANY                     FULLNAME                                                    FLAGS */
COMP( 1982, vcc,        0,          0,      vcc,   fidelz80, 0,      "Fidelity Electronics",   "Talking Chess Challenger (model VCC)", GAME_NOT_WORKING )

