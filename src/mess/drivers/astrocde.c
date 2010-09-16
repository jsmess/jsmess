/****************************************************************************

    Bally Astrocade consumer hardware
    driver by Nicola Salmoria, Mike Coates, Frank Palazzolo, Aaron Giles,
    Dirk Best

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "includes/astrocde.h"
#include "sound/astrocde.h"
#include "devices/cartslot.h"


/*************************************
 *
 *  Memory maps
 *
 *************************************/

static ADDRESS_MAP_START( astrocade_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_ROM AM_WRITE(astrocade_funcgen_w)
	AM_RANGE(0x1000, 0x3fff) AM_ROM /* Star Fortress writes in here?? */
	AM_RANGE(0x4000, 0x4fff) AM_RAM AM_BASE_GENERIC(videoram) /* ASG */
ADDRESS_MAP_END


static ADDRESS_MAP_START( astrocade_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x1f) AM_MIRROR(0xff00) AM_MASK(0xffff) AM_READWRITE(astrocade_data_chip_register_r, astrocade_data_chip_register_w)
ADDRESS_MAP_END


/*************************************
 *
 *  Input ports
 *
 *
 *  The Astrocade has ports for four hand controllers.  Each controller has a
 *  knob on top that can be simultaneously pushed as an eight-way joystick and
 *  twisted as a paddle, in addition to a trigger button.  The knob can twist
 *  through about 270 degrees, registering 256 unique positions.  It does not
 *  autocenter.  When selecting options on the menu, twisting the knob to the
 *  right gives lower numbers, and twisting to the left gives larger numbers.
 *  Paddle games like Clowns have more intuitive behavior -- twisting to the
 *  right moves the character right.
 *
 *  There is a 24-key keypad on the system itself (6 rows, 4 columns).  It is
 *  labeled for the built-in calculator, but overlays were released for other
 *  programs, the most popular being the BASIC cartridges, which allowed a
 *  large number of inputs by making the bottom row shift buttons.  The labels
 *  below first list the calculator key, then the BASIC keys in the order of no
 *  shift, GREEN shift, RED shift, BLUE shift, WORDS shift.
 *
 *************************************/

static INPUT_PORTS_START( astrocde )
	PORT_START("P1HANDLE")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP)    PORT_PLAYER(1) PORT_8WAY
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN)  PORT_PLAYER(1) PORT_8WAY
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT)  PORT_PLAYER(1) PORT_8WAY
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_BUTTON1)        PORT_PLAYER(1)
	PORT_BIT(0xe0, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("P2HANDLE")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP)    PORT_PLAYER(2) PORT_8WAY
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN)  PORT_PLAYER(2) PORT_8WAY
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT)  PORT_PLAYER(2) PORT_8WAY
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_BUTTON1)        PORT_PLAYER(2)
	PORT_BIT(0xe0, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("P3HANDLE")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP)    PORT_PLAYER(3) PORT_8WAY
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN)  PORT_PLAYER(3) PORT_8WAY
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT)  PORT_PLAYER(3) PORT_8WAY
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_PLAYER(3) PORT_8WAY
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_BUTTON1)        PORT_PLAYER(3)
	PORT_BIT(0xe0, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("P4HANDLE")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP)    PORT_PLAYER(4) PORT_8WAY
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN)  PORT_PLAYER(4) PORT_8WAY
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT)  PORT_PLAYER(4) PORT_8WAY
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_PLAYER(4) PORT_8WAY
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_BUTTON1)        PORT_PLAYER(4)
	PORT_BIT(0xe0, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("KEYPAD0")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("%   \xC3\xB7         [   ]   LIST") PORT_CODE(KEYCODE_O)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("/   x     J   K   L   NEXT") PORT_CODE(KEYCODE_SLASH_PAD)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("x   -     V   W   X   IF") PORT_CODE(KEYCODE_ASTERISK)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("-   +     &   @   *   GOTO") PORT_CODE(KEYCODE_MINUS_PAD)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("+   =     #   %   :   PRINT") PORT_CODE(KEYCODE_PLUS_PAD)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("=   WORDS Shift") PORT_CODE(KEYCODE_ENTER_PAD)
	PORT_BIT(0xc0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("KEYPAD1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("\xE2\x86\x93   HALT              RUN") PORT_CODE(KEYCODE_PGDN)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("CH  9     G   H   I   STEP") PORT_CODE(KEYCODE_H)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("9   6     S   T   U   RND") PORT_CODE(KEYCODE_9)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("6   3     \xE2\x86\x91   .   \xE2\x86\x93   BOX") PORT_CODE(KEYCODE_6)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("3   ERASE (   ;   )") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME(".   BLUE Shift") PORT_CODE(KEYCODE_STOP)
	PORT_BIT(0xc0, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("KEYPAD2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("\xE2\x86\x91   PAUSE     /   \\") PORT_CODE(KEYCODE_PGUP)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("MS  8     D   E   F   TO") PORT_CODE(KEYCODE_S)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("8   5     P   Q   R   RETN") PORT_CODE(KEYCODE_8)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("5   2     \xE2\x86\x90   '   \xE2\x86\x92   LINE") PORT_CODE(KEYCODE_5)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("2   0     <   \"   >   INPUT") PORT_CODE(KEYCODE_2)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("0   RED Shift") PORT_CODE(KEYCODE_0)
	PORT_BIT(0xc0, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("KEYPAD3")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("C   GO                +10") PORT_CODE(KEYCODE_C)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("MR  7     A   B   C   FOR") PORT_CODE(KEYCODE_R)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("7   4     M   N   O   GOSB") PORT_CODE(KEYCODE_7)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("4   1     Y   Z   !   CLEAR") PORT_CODE(KEYCODE_4)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("1   SPACE $   ,   ?") PORT_CODE(KEYCODE_1)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("CE  GREEN Shift") PORT_CODE(KEYCODE_E)
	PORT_BIT(0xc0, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("P1_KNOB")
	PORT_BIT(0xff, 0x00, IPT_PADDLE) PORT_INVERT PORT_SENSITIVITY(85) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_MINMAX(0,255) PORT_CODE_DEC(KEYCODE_Z) PORT_CODE_INC(KEYCODE_X) PORT_PLAYER(1)

	PORT_START("P2_KNOB")
	PORT_BIT(0xff, 0x00, IPT_PADDLE) PORT_INVERT PORT_SENSITIVITY(85) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_MINMAX(0,255) PORT_CODE_DEC(KEYCODE_N) PORT_CODE_INC(KEYCODE_M) PORT_PLAYER(2)

	PORT_START("P3_KNOB")
	PORT_BIT(0xff, 0x00, IPT_PADDLE) PORT_INVERT PORT_SENSITIVITY(85) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_MINMAX(0,255) PORT_CODE_DEC(KEYCODE_Q) PORT_CODE_INC(KEYCODE_W) PORT_PLAYER(3)

	PORT_START("P4_KNOB")
	PORT_BIT(0xff, 0x00, IPT_PADDLE) PORT_INVERT PORT_SENSITIVITY(85) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_MINMAX(0,255) PORT_CODE_DEC(KEYCODE_Y) PORT_CODE_INC(KEYCODE_U) PORT_PLAYER(4)
INPUT_PORTS_END


/*************************************
 *
 *  Machine drivers
 *
 *************************************/

static DEVICE_IMAGE_LOAD( astrocde_cart )
{
	UINT32 size;

	if (image.software_entry() == NULL)
	{
		size = image.length();

		if (image.fread( memory_region(image.device().machine, "maincpu") + 0x2000, size) != size)
		{
			image.seterror(IMAGE_ERROR_UNSPECIFIED, "Unable to fully read from file");
			return IMAGE_INIT_FAIL;
		}

	}
	else
	{
		size = image.get_software_region_length("rom");
		memcpy(memory_region(image.device().machine, "maincpu") + 0x2000, image.get_software_region("rom"), size);
	}

	return IMAGE_INIT_PASS;
}

static MACHINE_CONFIG_START( astrocde, driver_device )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, ASTROCADE_CLOCK/4)        /* 1.789 MHz */
	MDRV_CPU_PROGRAM_MAP(astrocade_mem)
	MDRV_CPU_IO_MAP(astrocade_io)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_RAW_PARAMS(ASTROCADE_CLOCK, 455, 0, 352, 262, 0, 240)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)

	MDRV_PALETTE_LENGTH(512)
	MDRV_PALETTE_INIT(astrocde)

	MDRV_VIDEO_START(astrocde)
	MDRV_VIDEO_UPDATE(astrocde)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("astrocade1", ASTROCADE, ASTROCADE_CLOCK/4)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("bin")
	MDRV_CARTSLOT_INTERFACE("astrocde_cart")
	MDRV_CARTSLOT_LOAD(astrocde_cart)

	/* Software lists */
	MDRV_SOFTWARE_LIST_ADD("cart_list","astrocde")
MACHINE_CONFIG_END


/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( astrocde )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "astro.bin",  0x0000, 0x2000, CRC(ebc77f3a) SHA1(b902c941997c9d150a560435bf517c6a28137ecc) )
ROM_END

ROM_START( astrocdl )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ballyhlc.bin",  0x0000, 0x2000, CRC(d7c517ba) SHA1(6b2bef5d970e54ed204549f58ba6d197a8bfd3cc) )
ROM_END

ROM_START( astrocdw )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "bioswhit.bin",  0x0000, 0x2000, CRC(6eb53e79) SHA1(d84341feec1a0a0e8aa6151b649bc3cf6ef69fbf) )
ROM_END

/*************************************
 *
 *  Driver initialization
 *
 *************************************/

static DRIVER_INIT( astrocde )
{
	astrocade_video_config = AC_SOUND_PRESENT | AC_LIGHTPEN_INTS;
}


/*************************************
 *
 *  Driver definitions
 *
 *************************************/

/*    YEAR  NAME      PARENT    COMPAT    MACHINE   INPUT     INIT      COMPANY                FULLNAME                     FLAGS */
CONS( 1978, astrocde, 0,        0,        astrocde, astrocde, astrocde, "Bally Manufacturing", "Bally Professional Arcade", GAME_SUPPORTS_SAVE )
CONS( 1977, astrocdl, astrocde, 0,        astrocde, astrocde, astrocde, "Bally Manufacturing", "Bally Home Library Computer", GAME_SUPPORTS_SAVE )
CONS( 1977, astrocdw, astrocde, 0,        astrocde, astrocde, astrocde, "Bally Manufacturing", "Bally Computer System", GAME_SUPPORTS_SAVE )
