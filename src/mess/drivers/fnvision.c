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

	- hook up the PIA properly so keys can be read

*/

#include "driver.h"
#include "inputx.h"
#include "devices/cartslot.h"
#include "machine/6821pia.h"
#include "sound/sn76496.h"
#include "video/tms9928a.h"

/* Read/Write Handlers */

static int keylatch;

static WRITE8_HANDLER( pia_porta_w )
{
	keylatch = ~data & 0x0f;
}

static READ8_HANDLER( pia_portb_r )
{
	switch (keylatch)
	{
	case 1:
		return readinputportbytag("PIAA0");
	case 2:
		return readinputportbytag("PIAA1");
	case 4:
		return readinputportbytag("PIAA2");
	case 8:
		return readinputportbytag("PIAA3");
	}

	return 0xff;
}

static READ8_HANDLER( pia_ca1_r )
{
	return 1;
}

static READ8_HANDLER( pia_ca2_r )
{
	return 1;
}

static READ8_HANDLER( pia_cb1_r )
{
	return 1;
}

static WRITE8_HANDLER( pia_ca2_w )
{
}

static WRITE8_HANDLER( pia_cb2_w )
{
}

/* Memory Map */

static ADDRESS_MAP_START( crvision_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_RAM AM_MIRROR(0x0c00)
//	AM_RANGE(0x1000, 0x1003) AM_READWRITE(pia_0_r, pia_0_w)
	AM_RANGE(0x1000, 0x1003) AM_WRITE(pia_0_w) // HACK to enable booting
	AM_RANGE(0x1002, 0x1002) AM_READ(pia_portb_r) // HACK to enable booting
	AM_RANGE(0x1003, 0x1003) AM_READ(input_port_4_r) // HACK to enable booting
	AM_RANGE(0x2000, 0x2000) AM_READ(TMS9928A_vram_r)
	AM_RANGE(0x2001, 0x2001) AM_READ(TMS9928A_register_r)
	AM_RANGE(0x3000, 0x3000) AM_WRITE(TMS9928A_vram_w)
	AM_RANGE(0x3001, 0x3001) AM_WRITE(TMS9928A_register_w)
	AM_RANGE(0x4000, 0x7fff) AM_ROMBANK(2)
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK(1)
	AM_RANGE(0xc000, 0xc7ff) AM_ROM AM_MIRROR(0x3800)
ADDRESS_MAP_END

/* Input Ports */

INPUT_PORTS_START( crvision )
	PORT_START_TAG("PIAA0")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x0c, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("1") PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("P1 Button 2 / CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL) // CNTL

	PORT_START_TAG("PIAA1")
	PORT_BIT( 0x30, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT( 0x60, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x28, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x48, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x50, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x18, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT( 0x0c, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT( 0x14, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x24, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x44, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT( 0x09, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("BACKSPACE") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x11, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x21, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT( 0x41, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x03, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x05, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("P1 Button 1 / SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT)
	PORT_BIT( 0x0a, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT( 0x12, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT( 0x22, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x42, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT( 0x06, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	
	PORT_START_TAG("PIAA2")
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_PLAYER(2)
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(2)
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_PLAYER(2)
	PORT_BIT( 0x0c, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_PLAYER(2)
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("P2 Button 2 / TAB") PORT_CODE(KEYCODE_TAB) PORT_CHAR(9) PORT_PLAYER(2)

	PORT_START_TAG("PIAA3")
	PORT_BIT( 0x06, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT( 0x42, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0x22, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x12, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR('@')
	PORT_BIT( 0x0a, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("P2 Button 1 / - =") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('=') PORT_PLAYER(2)
	PORT_BIT( 0x05, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT( 0x03, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT( 0x41, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT( 0x21, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT( 0x11, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT( 0x09, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER) PORT_CHAR('\r')
	PORT_BIT( 0x44, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT( 0x24, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT( 0x14, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT( 0x0c, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0x18, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT( 0x50, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT( 0x48, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT( 0x28, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x60, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0x30, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')

	PORT_START // HACK to enable booting
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

/* Machine Interface */

static INTERRUPT_GEN( crvision_int )
{
    TMS9928A_interrupt();
}

static void crvision_vdp_interrupt(int state)
{
	cpunum_set_input_line(0, INPUT_LINE_IRQ0, state);
}

static const TMS9928a_interface tms9918_intf =
{
	TMS99x8,
	0x4000,
	0, 0,
	crvision_vdp_interrupt
};

static const TMS9928a_interface tms9929_intf =
{
	TMS9929,
	0x4000,
	0, 0,
	crvision_vdp_interrupt
};

static const pia6821_interface crvision_pia_intf =
{
	0,				// input A
	pia_portb_r,	// input B
	pia_ca1_r,		// input CA1 (+5V)
	pia_cb1_r,		// input CB1 (SN76489 pin 4 (READY))
	pia_ca2_r,		// input CA2 (+5V)
	0,				// input CB2
	pia_porta_w,	// output A
	SN76496_0_w,	// output B
	pia_ca2_w,		// output CA2
	pia_cb2_w,		// output CB2 (SN76489 pin 6 (_CE))
	0,				// irq A
	0				// irq B
};

static MACHINE_START( crvision )
{
	TMS9928A_configure(&tms9918_intf);
	pia_config(0, &crvision_pia_intf);

	return 0;
}

static MACHINE_START( fnvision )
{
	TMS9928A_configure(&tms9929_intf);
	pia_config(0, &crvision_pia_intf);

	return 0;
}

static MACHINE_RESET( crvision )
{
	pia_reset();
}

/* Machine Driver */

static MACHINE_DRIVER_START( crvision )
	// basic machine hardware
	MDRV_CPU_ADD(M6502, 2000000)
	MDRV_CPU_PROGRAM_MAP(crvision_map, 0)
	MDRV_CPU_VBLANK_INT(crvision_int, 1)
	MDRV_SCREEN_REFRESH_RATE(10738635.0/2/342/262)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_START( crvision )
	MDRV_MACHINE_RESET( crvision )

    // video hardware
	MDRV_IMPORT_FROM(tms9928a)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SN76489, 2000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( fnvision )
	MDRV_IMPORT_FROM( crvision )
	MDRV_MACHINE_START( fnvision )
	MDRV_SCREEN_REFRESH_RATE(10738635.0/2/342/313)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( crvision )
    ROM_REGION( 0x10000, REGION_CPU1, 0 )
    ROM_LOAD( "crvision.rom", 0xc000, 0x0800, CRC(c3c590c6) SHA1(5ac620c529e4965efb5560fe824854a44c983757) )
ROM_END

ROM_START( fnvision )
    ROM_REGION( 0x10000, REGION_CPU1, 0 )
    ROM_LOAD( "funboot.rom", 0xc000, 0x0800, CRC(05602697) SHA1(c280b20c8074ba9abb4be4338b538361dfae517f) )
ROM_END

/* System Configuration */

static DEVICE_LOAD( crvision_cart )
{
	int size = image_length(image);

	switch (size)
	{
	case 0x1000: // 4K
		image_fread(image, memory_region(REGION_CPU1) + 0x9000, 0x1000);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x9fff, 0, 0x2000, MRA8_BANK1);
		break;

	case 0x1800: // 6K
		image_fread(image, memory_region(REGION_CPU1) + 0x9000, 0x1000);
		image_fread(image, memory_region(REGION_CPU1) + 0x8800, 0x0800);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x9fff, 0, 0x2000, MRA8_BANK1);
		break;

	case 0x2000: // 8K
		image_fread(image, memory_region(REGION_CPU1) + 0x8000, 0x2000);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x9fff, 0, 0x2000, MRA8_BANK1);
		break;

	case 0x2800: // 10K
		image_fread(image, memory_region(REGION_CPU1) + 0x8000, 0x2000);
		image_fread(image, memory_region(REGION_CPU1) + 0x5800, 0x0800);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x9fff, 0, 0x2000, MRA8_BANK1);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x5fff, 0, 0x2000, MRA8_BANK2);
		break;

	case 0x3000: // 12K
		image_fread(image, memory_region(REGION_CPU1) + 0x8000, 0x2000);
		image_fread(image, memory_region(REGION_CPU1) + 0x5000, 0x1000);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x9fff, 0, 0x2000, MRA8_BANK1);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x5fff, 0, 0x2000, MRA8_BANK2);
		break;

	case 0x4000: // 16K
		image_fread(image, memory_region(REGION_CPU1) + 0xa000, 0x2000);
		image_fread(image, memory_region(REGION_CPU1) + 0x8000, 0x2000);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xbfff, 0, 0, MRA8_BANK1);
		break;

	case 0x4800: // 18K
		image_fread(image, memory_region(REGION_CPU1) + 0xa000, 0x2000);
		image_fread(image, memory_region(REGION_CPU1) + 0x8000, 0x2000);
		image_fread(image, memory_region(REGION_CPU1) + 0x4800, 0x0800);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x8fff, 0, 0, MRA8_BANK1);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x4fff, 0, 0x3000, MRA8_BANK2);
		break;

	default:
		return INIT_FAIL;
	}

	memory_configure_bank(1, 0, 1, memory_region(REGION_CPU1) + 0x8000, 0);
	memory_set_bank(1, 0);

	memory_configure_bank(2, 0, 1, memory_region(REGION_CPU1) + 0x4000, 0);
	memory_set_bank(2, 0);

	return INIT_PASS;
}

static void crvision_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;
		case DEVINFO_INT_MUST_BE_LOADED:				info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_crvision_cart; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "rom"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START( crvision )
	CONFIG_DEVICE(crvision_cartslot_getinfo)
SYSTEM_CONFIG_END

/* System Drivers */

/*    YEAR	NAME	  PARENT	COMPAT	MACHINE		INPUT		INIT	CONFIG      COMPANY				FULLNAME */
CONS( 1981, crvision, 0,		0,		crvision,	crvision,	0,		crvision,	"Video Technology", "creatiVision (NTSC)", GAME_NOT_WORKING )
CONS( 1983, fnvision, crvision, 0,		fnvision,	crvision,	0,		crvision,	"Video Technology", "FunVision (PAL)", GAME_NOT_WORKING )
