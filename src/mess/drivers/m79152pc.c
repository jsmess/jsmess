/***************************************************************************

        Mera-Elzab 79152pc

        This is terminal

        29/12/2011 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"

class m79152pc_state : public driver_device
{
public:
	m79152pc_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

};

static ADDRESS_MAP_START(m79152pc_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( m79152pc_io , AS_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( m79152pc )
INPUT_PORTS_END


static MACHINE_RESET(m79152pc)
{
}

static VIDEO_START( m79152pc )
{
}

static SCREEN_UPDATE( m79152pc )
{
    return 0;
}

static MACHINE_CONFIG_START( m79152pc, m79152pc_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MCFG_CPU_PROGRAM_MAP(m79152pc_mem)
    MCFG_CPU_IO_MAP(m79152pc_io)

    MCFG_MACHINE_RESET(m79152pc)

    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(640, 480)
    MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MCFG_SCREEN_UPDATE(m79152pc)

    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(black_and_white)

    MCFG_VIDEO_START(m79152pc)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( m79152pc )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "left.bin", 0x0000, 0x4000, CRC(8cd677fc) SHA1(7ad28f3ba984383f24a36639ca27fc1eb5a5d002))
	ROM_REGION( 0x1800, "gfx", ROMREGION_ERASEFF )
	ROM_LOAD( "right.bin", 0x0000, 0x1000, CRC(93f83fdc) SHA1(e8121b3d175c46c02828f43ec071a7d9c62e7c26))
	ROM_LOAD( "char.bin",  0x1000, 0x0800, CRC(da3792a5) SHA1(b4a4f0d61d8082b7909a346a5b01494c53cf8d05))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( ????, m79152pc,  0,       0,	m79152pc,	m79152pc,	 0,   "Mera-Elzab",   "79152pc",		GAME_NOT_WORKING | GAME_NO_SOUND)

