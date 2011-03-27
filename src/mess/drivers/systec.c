/***************************************************************************

        Systec Z80

        More data :
            http://www.hd64180-cpm.de/html/systecz80.html

        30/08/2010 Skeleton driver

        Systec Platine 1

        SYSTEC 155.1L
        EPROM 2764 CP/M LOADER 155 / 9600 Baud
        Z8400APS CPU
        Z8420APS PIO
        Z8430APS CTC
        Z8470APS DART

        Systec Platine 2

        SYSTEC 100.3B
        MB8877A FDC Controller
        FDC9229BT SMC 8608
        Z8410APS DMA
        Z8420APS PIO

        MB8877A Compatible FD1793
****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"


class systec_state : public driver_device
{
public:
	systec_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};


static ADDRESS_MAP_START(systec_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( systec_io, AS_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( systec )
INPUT_PORTS_END

static MACHINE_RESET(systec)
{
}

static VIDEO_START( systec )
{
}

static SCREEN_UPDATE( systec )
{
	return 0;
}

static MACHINE_CONFIG_START( systec, systec_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu", Z80, XTAL_16MHz / 4)
    MCFG_CPU_PROGRAM_MAP(systec_mem)
	MCFG_CPU_IO_MAP(systec_io)

    MCFG_MACHINE_RESET(systec)

    /* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(256, 192) /* border size not accurate */
	MCFG_SCREEN_VISIBLE_AREA(0, 256 - 1, 0, 192 - 1)
    MCFG_SCREEN_UPDATE(systec)

    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(black_and_white)

    MCFG_VIDEO_START(systec)
MACHINE_CONFIG_END


/* ROM definition */
ROM_START( systec )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "systec.bin",   0x0000, 0x2000, CRC(967108ab) SHA1(a414db032ca7db0f9fdbe22aa68a099a93efb593))
ROM_END

/* Driver */

/*   YEAR  NAME    PARENT  COMPAT   MACHINE  INPUT  INIT        COMPANY   FULLNAME       FLAGS */
COMP( 19??, systec,  0,       0,	systec, 	systec, 	 0, 	  "Systec",   "Systec Z80",		GAME_NOT_WORKING | GAME_NO_SOUND)
