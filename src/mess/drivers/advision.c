/*************************************************************************

    drivers/advision.c

    Driver for the Entex Adventure Vision

**************************************************************************/

/* Core includes */
#include "driver.h"
#include "includes/advision.h"

/* Components */
#include "cpu/i8039/i8039.h"
#include "cpu/cop400/cop400.h"

/* Devices */
#include "devices/cartslot.h"



#define MAIN_CLOCK  XTAL_11MHz



/*************************************
 *
 *  Memory maps
 *
 *************************************/

static ADDRESS_MAP_START( advision_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_READWRITE(MRA8_BANK1, MWA8_ROM)
	AM_RANGE(0x0400, 0x0fff) AM_ROM
	AM_RANGE(0x2000, 0x23ff) AM_RAM	/* MAINRAM four banks */
ADDRESS_MAP_END


static ADDRESS_MAP_START( advision_sound_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_ROM
ADDRESS_MAP_END


/*
    8048 Ports:
    
    P1  Bit 0..1  - RAM bank select
        Bit 3..7  - Keypad input

    P2  Bit 0..3  - A8-A11
        Bit 4..7  - Sound control/Video write address

    T1  Mirror sync pulse
*/

static ADDRESS_MAP_START( advision_ports, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0xff)         AM_READWRITE(advision_MAINRAM_r, advision_MAINRAM_w)
	AM_RANGE(I8039_p1, I8039_p1) AM_READWRITE(advision_controller_r, advision_bankswitch_w)
	AM_RANGE(I8039_p2, I8039_p2) AM_WRITE(advision_av_control_w)
	AM_RANGE(I8039_t1, I8039_t1) AM_READ(advision_gett1)
ADDRESS_MAP_END


static ADDRESS_MAP_START( advision_sound_ports, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0xff)                   AM_RAM
	AM_RANGE(COP400_PORT_L, COP400_PORT_L) AM_READ(advision_getL)
	AM_RANGE(COP400_PORT_G, COP400_PORT_G) AM_WRITE(advision_putG)
	AM_RANGE(COP400_PORT_D, COP400_PORT_D) AM_WRITE(advision_putD)
ADDRESS_MAP_END



/*************************************
 *
 *  Input ports
 *
 *************************************/

static INPUT_PORTS_START( advision )
    PORT_START_TAG("joystick")      /* IN0 */
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON4 )       PORT_PLAYER(1)
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 )       PORT_PLAYER(1)
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 )       PORT_PLAYER(1)
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )       PORT_PLAYER(1)
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(1) PORT_8WAY
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(1) PORT_8WAY
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_8WAY
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(1) PORT_8WAY
INPUT_PORTS_END



/*************************************
 *
 *  Machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( advision )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", I8048, MAIN_CLOCK/15)
	MDRV_CPU_PROGRAM_MAP(advision_mem, 0)
	MDRV_CPU_IO_MAP(advision_ports, 0)

	MDRV_CPU_ADD(COP411, 52631)
	MDRV_CPU_PROGRAM_MAP(advision_sound_mem, 0)
	MDRV_CPU_IO_MAP(advision_sound_ports, 0)

	MDRV_SCREEN_REFRESH_RATE(8*15)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	MDRV_MACHINE_RESET( advision )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(320, 200)
	MDRV_SCREEN_VISIBLE_AREA(0, 320-1, 0, 200-1)
	MDRV_PALETTE_LENGTH(8+2)
	MDRV_COLORTABLE_LENGTH(8*2)
	MDRV_PALETTE_INIT(advision)

	MDRV_VIDEO_START(advision)
	MDRV_VIDEO_UPDATE(advision)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( advision )
	ROM_REGION( 0x2800, REGION_CPU1, 0 )
    ROM_LOAD( "avbios.rom", 0x1000, 0x0400, CRC(279e33d1) SHA1(bf7b0663e9125c9bfb950232eab627d9dbda8460) )
	ROM_CART_LOAD( 0, "bin", 0x0000, 0x1000, ROM_NOMIRROR | ROM_FULLSIZE )
	ROM_REGION( 0x0400, REGION_CPU2, 0 )
	ROM_LOAD( "avsound.bin", 0x0000, 0x0200, CRC(81e95975) SHA1(8b6f8c30dd3e9d8e43f1ea20fba2361b383790eb) )
ROM_END



/*************************************
 *
 *  System config
 *
 *************************************/

SYSTEM_CONFIG_START( advision )
	CONFIG_DEVICE( cartslot_device_getinfo )
SYSTEM_CONFIG_END



/*************************************
 *
 *  Game drivers
 *
 *************************************/

/*    YEAR  NAME        PARENT  COMPAT  MACHINE   INPUT     INIT        CONFIG      COMPANY   FULLNAME */
CONS( 1982, advision,	0,		0,		advision, advision,	advision,	advision,	"Entex",  "Adventure Vision", 0 )
