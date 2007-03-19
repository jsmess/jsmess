
/****************************************************************************

	Bally Astrocade Driver

	09/23/98 - Added sound, added player 2 pot					FMP
			   Added MWA8_ROM to fix Star Fortress problem
			   Added cartridge support

	08/02/98 - First release based on original wow.c in MAME	FMP
			   Added palette generation based on a function
			   Fixed collision detection
                           Fixed shifter operation
                           Fixed clock speed
                           Fixed Interrupt Rate and handling
                           (No Light pen support yet)
                           (No sound yet)

        Original header follows, some comments don't apply      FMP

 ****************************************************************************/

 /****************************************************************************

   Bally Astrocade style games

   02.02.98 - New IO port definitions				MJC
              Dirty Rectangle handling
              Sparkle Circuit for Gorf
              errorlog output conditional on MAME_DEBUG

   03/04 98 - Extra Bases driver 				ATJ
	      	  Wow word driver

 ****************************************************************************/

#include "driver.h"
#include "sound/astrocde.h"
#include "video/generic.h"
#include "includes/astrocde.h"
#include "devices/cartslot.h"

/****************************************************************************
 * Bally Astrocade
 ****************************************************************************/

ADDRESS_MAP_START( astrocade_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_READWRITE(MRA8_ROM, astrocade_magicram_w)
	AM_RANGE(0x1000, 0x3fff) AM_ROM /* Star Fortress writes in here?? */
	AM_RANGE(0x4000, 0x4fff) AM_READWRITE(MRA8_RAM, astrocade_videoram_w) AM_BASE(&astrocade_videoram) AM_SIZE(&videoram_size) /* ASG */
ADDRESS_MAP_END

ADDRESS_MAP_START( astrocade_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x07) AM_MIRROR(0xff00) AM_WRITE(astrocade_colour_register_w)
	AM_RANGE(0x08, 0x08) AM_MIRROR(0xff00) AM_READWRITE(astrocade_intercept_r, astrocade_mode_w)
	AM_RANGE(0x09, 0x09) AM_MIRROR(0xff00) AM_WRITE(astrocade_colour_split_w)
	AM_RANGE(0x0a, 0x0a) AM_MIRROR(0xff00) AM_WRITE(astrocade_vertical_blank_w)
	AM_RANGE(0x0b, 0x0b) AM_MIRROR(0xff00) AM_WRITE(astrocade_colour_block_w)
	AM_RANGE(0x0c, 0x0c) AM_MIRROR(0xff00) AM_WRITE(astrocade_magic_control_w)
	AM_RANGE(0x0d, 0x0d) AM_MIRROR(0xff00) AM_WRITE(astrocade_interrupt_vector_w)
	AM_RANGE(0x0e, 0x0e) AM_MIRROR(0xff00) AM_READWRITE(astrocade_video_retrace_r, astrocade_interrupt_enable_w)
	AM_RANGE(0x0f, 0x0f) AM_MIRROR(0xff00) AM_WRITE(astrocade_interrupt_w)
	/*AM_RANGE(0x0f, 0x0f) AM_MIRROR(0xff00) AM_READ(astrocade_horiz_r)*/
	AM_RANGE(0x10, 0x10) AM_MIRROR(0xff00) AM_READ(input_port_0_r)
	AM_RANGE(0x11, 0x11) AM_MIRROR(0xff00) AM_READ(input_port_1_r)
	AM_RANGE(0x12, 0x12) AM_MIRROR(0xff00) AM_READ(input_port_2_r)
	AM_RANGE(0x13, 0x13) AM_MIRROR(0xff00) AM_READ(input_port_3_r)
	AM_RANGE(0x14, 0x14) AM_MIRROR(0xff00) AM_READ(input_port_4_r)
	AM_RANGE(0x15, 0x15) AM_MIRROR(0xff00) AM_READ(input_port_5_r)
	AM_RANGE(0x16, 0x16) AM_MIRROR(0xff00) AM_READ(input_port_6_r)
	AM_RANGE(0x17, 0x17) AM_MIRROR(0xff00) AM_READ(input_port_7_r)
	AM_RANGE(0x10, 0x17) AM_MIRROR(0xff00) AM_WRITE(astrocade_sound1_w) /* Sound Stuff */
	AM_RANGE(0x1c, 0x1c) AM_MIRROR(0xff00) AM_READ(input_port_8_r)
	AM_RANGE(0x1d, 0x1d) AM_MIRROR(0xff00) AM_READ(input_port_9_r)
	AM_RANGE(0x1e, 0x1e) AM_MIRROR(0xff00) AM_READ(input_port_10_r)
	AM_RANGE(0x1f, 0x1f) AM_MIRROR(0xff00) AM_READ(input_port_11_r)
	AM_SPACE(0x18, 0xff) AM_WRITE(astrocade_soundblock1_w)
	AM_RANGE(0x19, 0x19) AM_MIRROR(0xff00) AM_WRITE(astrocade_magic_expand_color_w)
ADDRESS_MAP_END

INPUT_PORTS_START( astrocde )
	PORT_START /* IN0 */	/* Player 1 Handle */
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_PLAYER(1) PORT_8WAY
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_PLAYER(1) PORT_8WAY
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_PLAYER(1) PORT_8WAY
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_PLAYER(1)
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* IN1 */	/* Player 2 Handle */
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_PLAYER(2) PORT_8WAY
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_PLAYER(2) PORT_8WAY
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_PLAYER(2) PORT_8WAY
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_PLAYER(2)
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* IN2 */	/* Player 3 Handle */

	PORT_START /* IN3 */	/* Player 4 Handle */

	PORT_START /* IN4 */	/* Keypad Column 0 (right) */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("%   \xC3\xB7         [   ]   LIST") PORT_CODE(KEYCODE_O)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("/   x     J   K   L   NEXT") PORT_CODE(KEYCODE_SLASH_PAD)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("x   -     V   W   X   IF") PORT_CODE(KEYCODE_ASTERISK)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("-   +     &   @   *   GOTO") PORT_CODE(KEYCODE_MINUS_PAD)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("+   =     #   %   :   PRINT") PORT_CODE(KEYCODE_PLUS_PAD)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("=   WORDS Shift") PORT_CODE(KEYCODE_ENTER_PAD)
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* IN5 */	/* Keypad Column 1 */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("\xE2\x86\x93   HALT              RUN") PORT_CODE(KEYCODE_PGDN)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CH  9     G   H   I   STEP") PORT_CODE(KEYCODE_H)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9   6     S   T   U   RND") PORT_CODE(KEYCODE_9)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("6   3     \xE2\x86\x91   .   \xE2\x86\x93   BOX") PORT_CODE(KEYCODE_6)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3   ERASE (   ;   )") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(".   BLUE Shift") PORT_CODE(KEYCODE_STOP)
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* IN6 */	/* Keypad Column 2 */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("\xE2\x86\x91   PAUSE     /   \\") PORT_CODE(KEYCODE_PGUP)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("MS  8     D   E   F   TO") PORT_CODE(KEYCODE_S)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("8   5     P   Q   R   RETN") PORT_CODE(KEYCODE_8)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("5   2     \xE2\x86\x90   '   \xE2\x86\x92   LINE") PORT_CODE(KEYCODE_5)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2   0     <   \"   >   INPUT") PORT_CODE(KEYCODE_2)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0   RED Shift") PORT_CODE(KEYCODE_0)
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* IN7 */	/* Keypad Column 3 (left) */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("C   GO                +10") PORT_CODE(KEYCODE_C)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("MR  7     A   B   C   FOR") PORT_CODE(KEYCODE_R)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("7   4     M   N   O   GOSB") PORT_CODE(KEYCODE_7)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("4   1     Y   Z   !   CLEAR") PORT_CODE(KEYCODE_4)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("1   SPACE $   ,   ?") PORT_CODE(KEYCODE_1)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CE  GREEN Shift") PORT_CODE(KEYCODE_E)
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* IN8 */	/* Player 1 Knob */
#if 0
	PORT_BIT( 0xff, 0x00, IPT_PADDLE ) PORT_SENSITIVITY(25) PORT_KEYDELTA(0) PORT_MINMAX(255,KEYCODE_X) PORT_CODE_DEC(KEYCODE_Z) PORT_CODE_INC(0) PORT_CODE_DEC(0) PORT_CODE_INC(4)

	PORT_START /* IN9 */	/* Player 2 Knob */
	PORT_BIT( 0xff, 0x00, IPT_PADDLE ) PORT_SENSITIVITY(25) PORT_KEYDELTA(0) PORT_MINMAX(255,KEYCODE_N) PORT_CODE_DEC(KEYCODE_M) PORT_CODE_INC(0) PORT_CODE_DEC(0) PORT_CODE_INC(4)
#else
	PORT_BIT( 0xff, 0x00, IPT_PADDLE) PORT_SENSITIVITY(85) PORT_KEYDELTA(20) PORT_MINMAX(0,255) PORT_CODE_DEC(KEYCODE_X) PORT_CODE_INC(KEYCODE_Z) PORT_PLAYER(1)

	PORT_START /* IN9 */	/* Player 2 Knob */
	PORT_BIT( 0xff, 0x00, IPT_PADDLE) PORT_SENSITIVITY(85) PORT_KEYDELTA(20) PORT_MINMAX(0,255) PORT_CODE_DEC(KEYCODE_M) PORT_CODE_INC(KEYCODE_N) PORT_PLAYER(2)
#endif
	PORT_START /* IN10 */	/* Player 3 Knob */

	PORT_START /* IN11 */	/* Player 4 Knob */

INPUT_PORTS_END



static MACHINE_DRIVER_START( astrocde )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 1789000)        /* 1.789 Mhz */
	MDRV_CPU_PROGRAM_MAP(astrocade_mem, 0)
	MDRV_CPU_IO_MAP(astrocade_io, 0)
	MDRV_CPU_VBLANK_INT(astrocade_interrupt,256)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(320, 204)
	MDRV_SCREEN_VISIBLE_AREA(0, 320-1, 0, 204-1)
	MDRV_PALETTE_LENGTH(8*32)
	MDRV_COLORTABLE_LENGTH(8)
	MDRV_PALETTE_INIT( astrocade )

	MDRV_VIDEO_START( generic )
	MDRV_VIDEO_UPDATE( generic_bitmapped )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(ASTROCADE, 1789773)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END


ROM_START( astrocde )
    ROM_REGION( 0x10000, REGION_CPU1, 0 )
    ROM_LOAD( "astro.bin",  0x0000, 0x2000, CRC(ebc77f3a) SHA1(b902c941997c9d150a560435bf517c6a28137ecc))
ROM_END

ROM_START( astrocdw )
    ROM_REGION( 0x10000, REGION_CPU1, 0 )
    ROM_LOAD( "bioswhit.bin",  0x0000, 0x2000, CRC(6eb53e79) SHA1(d84341feec1a0a0e8aa6151b649bc3cf6ef69fbf))
ROM_END

static void astrocde_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_astrocade_rom; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "bin"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(astrocde)
	CONFIG_DEVICE(astrocde_cartslot_getinfo)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME      PARENT	COMPAT	  MACHINE   INPUT     INIT	CONFIG		COMPANY			FULLNAME */
CONS( 1978, astrocde, 0,	0,	  astrocde, astrocde, 0,	astrocde,	"Bally Manufacturing", "Bally Professional Arcade", 0)
CONS( 1977, astrocdw, astrocde, 0, astrocde, astrocde, 0,        astrocde,       "Bally Manufacturing", "Bally Computer System", 0)
