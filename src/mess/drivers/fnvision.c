/*

	FUNVISION TECHNICAL INFO
	========================

	CHIPS
	=====

	2MHZ 6502A cpu
	2 x 2114 (1K RAM)
	9929 Video chip with 16kb vram
	76489 sound chip
	6821 PIA

	MEMORY MAP
	==========

	0000 - 0FFF     1K RAM repeated 4 times
	1000 - 1FFF     PIA (only 4 addresses)
	2000 - 2FFF     Read VDP chip (only occupies 2 addresses)
	3000 - 3FFF     Write VDP chip (only occupies 2 addresses)
	4000 - 7FFF     2nd cartridge ROM area
	8000 - BFFF     1st cartridge ROM area 
	C000 - FFFF     2K boot ROM repeated 8 times.

	Video
	=====

	Most code writes to $3000 and $3001 and reads from $2000 and $2001. Go read 
	a manual for the 9929/9918 for how it all works.

	Sound
	=====

	The 76489 is wired into port B of the PIA. You just write single bytes to it.
	Its a relatively slow access device so this is undoubtedly why they wired it to
	the PIA rather than straight to the data bus. I think they configure the PIA to
	automatically send a strobe signal when you write to $1002.

	Keyboard
	========

	The keyboard is the weirdest thing imaginable. Visually it consists of the two
	hand controllers slotted into the top of the console. The left joystick and
	24 keys on one side and the right joystick and another 24 keys on the other.
	The basic layout of the keys is:

	Left controller
	---------------

	 1     2     3     4     5     6
	ctrl   Q     W     E     R     T
	 <-    A     S     D     F     G
	shft   Z     X     C     V     B

	Right controller
	----------------

	 7     8     9     0     :     -
	 Y     U     I     O     P    Enter
	 H     J     K     L     ;     ->
	 N     M     ,     .     /    space

	The left controller is wired to the PIA pins pa0, pa1 and pb0-7
	The right controller is wired to the PIA pins pa2, pa3 and pb0-7

	The basic key scanning routine sets pa0-3 high then sends pa0 low, then pa1
	low and so forth and each time it reads back a value into pb0-7. You might ask
	the question 'How do they read 48 keys and two 8(16?) position joysticks using
	a 4 x 8 key matrix?'. Somehow when you press a key and the appropriate 
	strobe is low, two of the 8 input lines are sent low instead of 1. I have
	no idea how they do this. This allows them to read virtually all 24 keys on
	one controller by just sending one strobe low. The strobes go something like:

		PA? low          keys
		-------          ----
		PA3             right hand keys
		PA2             right joystick
		PA1             left hand keys
		PA0             left joystick

	An example of a key press is the 'Y' key. Its on the right controller so is
	scanned when PA3 is low. It will return 11111010 ($FA).

	There are some keys that don't follow the setup above. These are the '1', Left
	arrow, space and right arrow. They are all scanned as part of the corresponding
	joystick strobe. eg. '1' is detected by sending PA0 low and reading back a 
	11110011 ($F3). Also some keys are effectively the same as the fire buttons
	on the controllers. Left shift and control act like the fire buttons on the 
	left controller and right arrow and '-' act the same as the fire buttons on the
	right controller.

	---

	MESS Driver by Curt Coder
	Based on the FunnyMu emulator by ???

	---

	TODO:

	- inputs
	- only astro pinball & chopper rescue carts work

*/

#include "driver.h"
#include "inputx.h"
#include "video/tms9928a.h"
#include "machine/6821pia.h"
#include "sound/sn76496.h"
#include "devices/cartslot.h"

static ADDRESS_MAP_START( fnvision_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_RAM AM_MIRROR(0x0c00)
//	AM_RANGE(0x1000, 0x1003) AM_READWRITE(pia_0_r, pia_0_w)
	AM_RANGE(0x1002, 0x1002) AM_WRITE(SN76496_0_w)		// HACK
	AM_RANGE(0x1003, 0x1003) AM_READ(input_port_0_r)	// HACK
	AM_RANGE(0x2000, 0x2000) AM_READ(TMS9928A_vram_r)
	AM_RANGE(0x2001, 0x2001) AM_READ(TMS9928A_register_r)
	AM_RANGE(0x3000, 0x3000) AM_WRITE(TMS9928A_vram_w)
	AM_RANGE(0x3001, 0x3001) AM_WRITE(TMS9928A_register_w)
	AM_RANGE(0x4000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xc7ff) AM_ROM AM_MIRROR(0x3800)
ADDRESS_MAP_END

INPUT_PORTS_START( fnvision )
    PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

/*
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1) PORT_CHAR('1')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2) PORT_CHAR('2')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3) PORT_CHAR('3')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4) PORT_CHAR('4')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5) PORT_CHAR('5')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6) PORT_CHAR('6')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W) PORT_CHAR('W')

	PORT_START	// IN
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G) PORT_CHAR('G')

	PORT_START	// IN
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL)
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(left)") PORT_CODE(KEYCODE_LEFT)
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT)

	PORT_START	// IN
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xb0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	// IN
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7) PORT_CHAR('7')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9) PORT_CHAR('9')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(":") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR(':')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U) PORT_CHAR('U')

    PORT_START	// IN
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(";") PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(';')

    PORT_START	// IN
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(",") PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(".") PORT_CODE(KEYCODE_STOP) PORT_CHAR('.')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/") PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER) PORT_CHAR('\r')
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(right)") PORT_CODE(KEYCODE_RIGHT)
	PORT_BIT( 0x0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(space)") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')

    PORT_START	// IN
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP)		PORT_PLAYER(2)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT)	PORT_PLAYER(2)
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN)	PORT_PLAYER(2)
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT)	PORT_PLAYER(2)
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1)			PORT_PLAYER(2)
    PORT_BIT( 0xb0, IP_ACTIVE_LOW, IPT_UNUSED )
*/
INPUT_PORTS_END



static INTERRUPT_GEN( fnvision_int )
{
    TMS9928A_interrupt();
}

static void fnvision_vdp_interrupt(int state)
{
	static int last_state;

    // only if it goes up
	if (state && !last_state)
	{
		cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
	}
	last_state = state;
}

static const TMS9928a_interface tms9928a_interface =
{
	TMS9929,
	0x4000,
	0, 0,
	fnvision_vdp_interrupt
};

static int pa;

static READ8_HANDLER( pia0_portb_r )
{
	switch (pa)
	{
	case 0:
		return readinputport(0);
	case 1:
		return readinputport(1);
	case 2:
		return readinputport(2);
	case 3:
		return readinputport(3);
	}

	return 0xff;
}

static WRITE8_HANDLER( pia0_porta_w )
{
	pa = ~data;
	logerror("%u",data);
}

static const pia6821_interface pia0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, pia0_portb_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ pia0_porta_w, SN76496_0_w, 0, 0,
	/*irqs   : A/B             */ 0, 0,
};

static MACHINE_START( fnvision )
{
	TMS9928A_configure(&tms9928a_interface);
	pia_config(0, PIA_STANDARD_ORDERING, &pia0_intf);
	return 0;
}

static MACHINE_RESET( fnvision )
{
	pia_reset();
}

static MACHINE_DRIVER_START( fnvision )
	// basic machine hardware
	MDRV_CPU_ADD(M6502, 2000000)	// ???
	MDRV_CPU_PROGRAM_MAP(fnvision_map, 0)
	MDRV_CPU_VBLANK_INT(fnvision_int, 1)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_START( fnvision )
	MDRV_MACHINE_RESET( fnvision )

    // video hardware
	MDRV_IMPORT_FROM(tms9928a)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SN76496, 2000000)	/* 2 MHz */
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END

ROM_START( fnvision )
    ROM_REGION( 0x10000, REGION_CPU1, 0 )
    ROM_LOAD( "funboot.rom", 0xc000, 0x0800, CRC(05602697) SHA1(c280b20c8074ba9abb4be4338b538361dfae517f) )
ROM_END

int device_load_fnvision_cart(mess_image *image)
{
	/*

		Cartridge Image format
		======================

		- The first 16K is read into 8000 - BFFF. If the cart file is less than 16k
		then the cartridge is read into 8000, then replicated to fill up the 16k
		(eg. a 4k cartridge file is written to 8000 - 8FFF, then replicated at
		9000-9FFF, A000-AFFF and B000-BFFF).
		- The next 16k is read into 4000 - 7FFF. If this extra bit is less than 16k,
		then the data is replicated throughout 4000 - 7FFF.

		For example, an 18k cartridge dump has its first 16k read and written into
		memory at 8000-BFFF. The remaining 2K is written into 4000-47FF, then 
		replicated 8 times to appear at 4800, 5000, 5800, 6000, 6800, 7000 and 7800.

	*/

	int size = image_fread(image, memory_region(REGION_CPU1) + 0x8000, 0x4000);

	switch (size)
	{
	case 0x1000:
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x8fff, 0, 0x3000, MRA8_ROM);
		break;
	case 0x2000:
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x9fff, 0, 0x2000, MRA8_ROM);
		break;
	case 0x4000:
		size = image_fread(image, memory_region(REGION_CPU1) + 0x4000, 0x4000);

		switch (size)
		{
		case 0x0800:
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x47ff, 0, 0x3800, MRA8_ROM);
			break;
		case 0x1000:
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x4fff, 0, 0x3000, MRA8_ROM);
			break;
		case 0x2000:
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x5fff, 0, 0x2000, MRA8_ROM);
			break;
		}
		break;
	}

	return 0;
}

static void fnvision_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;
		case DEVINFO_INT_MUST_BE_LOADED:				info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_fnvision_cart; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "rom"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START( fnvision )
	CONFIG_DEVICE(fnvision_cartslot_getinfo)
SYSTEM_CONFIG_END

//    YEAR	NAME	  PARENT COMPAT MACHINE INPUT INIT COMPANY FULLNAME
CONS( 1981, fnvision, 0, 0, fnvision, fnvision, 0, fnvision, "Video Technology", "FunVision", GAME_NOT_WORKING )
