/*************************************************************************

    drivers/advision.c

    Driver for the Entex Adventure Vision

**************************************************************************/

/*

    TODO:

    - Turtles music is monotonous
    - convert to discrete sound
    - screen pincushion distortion

*/

#include "driver.h"
#include "includes/advision.h"
#include "cpu/mcs48/mcs48.h"
#include "cpu/cop400/cop400.h"
#include "devices/cartslot.h"
#include "sound/dac.h"

/* Memory Maps */

static ADDRESS_MAP_START( program_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_ROMBANK(1)
	AM_RANGE(0x0400, 0x0fff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0xff) AM_READWRITE(advision_extram_r, advision_extram_w)
	AM_RANGE(MCS48_PORT_P1, MCS48_PORT_P1) AM_READWRITE(advision_controller_r, advision_bankswitch_w)
	AM_RANGE(MCS48_PORT_P2, MCS48_PORT_P2) AM_WRITE(advision_av_control_w)
	AM_RANGE(MCS48_PORT_T1, MCS48_PORT_T1) AM_READ(advision_vsync_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(COP400_PORT_L, COP400_PORT_L) AM_READ(advision_sound_cmd_r)
	AM_RANGE(COP400_PORT_G, COP400_PORT_G) AM_WRITE(advision_sound_g_w)
	AM_RANGE(COP400_PORT_D, COP400_PORT_D) AM_WRITE(advision_sound_d_w)
	AM_RANGE(COP400_PORT_SIO, COP400_PORT_SIO) AM_NOP
	AM_RANGE(COP400_PORT_SK, COP400_PORT_SK) AM_NOP
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( advision )
    PORT_START("joystick")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON4 )       PORT_PLAYER(1)
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 )       PORT_PLAYER(1)
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 )       PORT_PLAYER(1)
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )       PORT_PLAYER(1)
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(1) PORT_8WAY
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(1) PORT_8WAY
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_8WAY
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(1) PORT_8WAY
INPUT_PORTS_END

/* Machine Driver */

static COP400_INTERFACE( advision_cop411_interface )
{
	COP400_CKI_DIVISOR_4,
	COP400_CKO_RAM_POWER_SUPPLY, // ??? or not connected
	COP400_MICROBUS_DISABLED
};

static MACHINE_DRIVER_START( advision )
	MDRV_DRIVER_DATA(advision_state)

	/* basic machine hardware */
	MDRV_CPU_ADD(I8048_TAG, I8048, XTAL_11MHz)
	MDRV_CPU_PROGRAM_MAP(program_map)
	MDRV_CPU_IO_MAP(io_map)

	MDRV_CPU_ADD(COP411_TAG, COP411, 52631*16) // COP411L-KCN/N
	MDRV_CPU_CONFIG(advision_cop411_interface)
	MDRV_CPU_IO_MAP(sound_io_map)

	MDRV_MACHINE_START(advision)
	MDRV_MACHINE_RESET(advision)

    /* video hardware */
	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_REFRESH_RATE(8*15)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(320, 200)
	MDRV_SCREEN_VISIBLE_AREA(0, 320-1, 0, 200-1)
	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT(advision)

	MDRV_VIDEO_START(advision)
	MDRV_VIDEO_UPDATE(advision)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD("dac", DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	/* cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("bin")
	MDRV_CARTSLOT_MANDATORY
MACHINE_DRIVER_END

/* ROMs */

ROM_START( advision )
	ROM_REGION( 0x1000, I8048_TAG, 0 )
	ROM_CART_LOAD( "cart", 0x0000, 0x1000, ROM_NOMIRROR | ROM_FULLSIZE )

	ROM_REGION( 0x400, "bios", 0 )
    ROM_LOAD( "avbios.u5", 0x000, 0x400, CRC(279e33d1) SHA1(bf7b0663e9125c9bfb950232eab627d9dbda8460) )

	ROM_REGION( 0x200, COP411_TAG, 0 )
	ROM_LOAD( "avsound.u8", 0x000, 0x200, CRC(81e95975) SHA1(8b6f8c30dd3e9d8e43f1ea20fba2361b383790eb) )
ROM_END

/* Game Driver */

/*    YEAR  NAME        PARENT  COMPAT  MACHINE   INPUT     INIT        CONFIG      COMPANY                 FULLNAME            FLAGS */
CONS( 1982, advision,	0,		0,		advision, advision,	0,			0,	"Entex Electronics",	"Adventure Vision", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
