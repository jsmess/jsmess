/***************************************************************************

        Sharp PC-E220

        16/11/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "devices/messram.h"

static WRITE8_HANDLER( rom_bank_w)
{
	UINT8 bank2 = data & 0x07; // bits 0,1,2
	UINT8 bank1 = data & 0x70; // bits 4,5,6

	memory_set_bankptr(space->machine, 3, memory_region(space->machine, "user1") + 0x4000 * bank1);
	memory_set_bankptr(space->machine, 4, memory_region(space->machine, "user1") + 0x4000 * bank2);
}

static WRITE8_HANDLER( ram_bank_w)
{
	const address_space *space_prg = cputag_get_address_space(space->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT8 bank = BIT(data,2);
	memory_install_write8_handler(space_prg, 0x0000, 0x3fff, 0, 0, SMH_BANK(1));

	memory_set_bankptr(space->machine, 1, messram_get_ptr(devtag_get_device(space->machine, "messram"))+0x0000+bank*0x8000);
	memory_set_bankptr(space->machine, 2, messram_get_ptr(devtag_get_device(space->machine, "messram"))+0x4000+bank*0x8000);
}

static ADDRESS_MAP_START(pce220_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_RAMBANK(1)
	AM_RANGE(0x4000, 0x7fff) AM_RAMBANK(2)
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK(3)
	AM_RANGE(0xc000, 0xffff) AM_ROMBANK(4)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pce220_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x19, 0x19) AM_WRITE(rom_bank_w)
	AM_RANGE(0x1b, 0x1b) AM_WRITE(ram_bank_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( pce220 )
INPUT_PORTS_END


static MACHINE_RESET(pce220)
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	memory_install_write8_handler(space, 0x0000, 0x3fff, 0, 0, SMH_UNMAP);
	memory_set_bankptr(machine, 1, memory_region(machine, "user1") + 0x0000);
}

static VIDEO_START( pce220 )
{
}

static VIDEO_UPDATE( pce220 )
{
    return 0;
}

static MACHINE_DRIVER_START( pce220 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, 3072000 ) // CMOS-SC7852
    MDRV_CPU_PROGRAM_MAP(pce220_mem)
    MDRV_CPU_IO_MAP(pce220_io)

    MDRV_MACHINE_RESET(pce220)

    /* video hardware */
	// 4 lines x 24 characters, resp. 144 x 32 pixel
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(144, 32)
    MDRV_SCREEN_VISIBLE_AREA(0, 144-1, 0, 32-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(pce220)
    MDRV_VIDEO_UPDATE(pce220)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("64K") // 32K internal + 32K external card
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( pce220 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_REGION( 0x20000, "user1", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS( 0, "v1", "v 0.1")
	ROM_SYSTEM_BIOS( 1, "v2", "v 0.2")
	ROM_LOAD( "bank0.bin",      0x0000, 0x4000, CRC(1fa94d11) SHA1(24c54347dbb1423388360a359aa09db47d2057b7))
	ROM_LOAD( "bank1.bin",      0x4000, 0x4000, CRC(0f9864b0) SHA1(6b7301c96f1a865e1931d82872a1ed5d1f80644e))
	ROM_LOAD( "bank2.bin",      0x8000, 0x4000, CRC(1625e958) SHA1(090440600d461aa7efe4adbf6e975aa802aabeec))
	ROM_LOAD( "bank3.bin",      0xc000, 0x4000, CRC(ed9a57f8) SHA1(58087dc64103786a40325c0a1e04bd88bfd6da57))
	ROM_LOAD( "bank4.bin",     0x10000, 0x4000, CRC(e37665ae) SHA1(85f5c84f69f79e7ac83b30397b2a1d9629f9eafa))
	ROMX_LOAD( "bank5.bin",     0x14000, 0x4000, CRC(6b116e7a) SHA1(b29f5a070e846541bddc88b5ee9862cc36b88eee),ROM_BIOS(2))
	ROMX_LOAD( "bank5_0.1.bin", 0x14000, 0x4000, CRC(13c26eb4) SHA1(b9cd0efd6b195653b9610e20ad8aab541824a689),ROM_BIOS(1))
	ROMX_LOAD( "bank6.bin",     0x18000, 0x4000, CRC(4fbfbd18) SHA1(e5aab1df172dcb94aa90e7d898eacfc61157ff15),ROM_BIOS(2))
	ROMX_LOAD( "bank6_0.1.bin", 0x18000, 0x4000, CRC(e2cda7a6) SHA1(01b1796d9485fde6994cb5afbe97514b54cfbb3a),ROM_BIOS(1))
	ROMX_LOAD( "bank7.bin",     0x1c000, 0x4000, CRC(5e98b5b6) SHA1(f22d74d6a24f5929efaf2983caabd33859232a94),ROM_BIOS(2))
	ROMX_LOAD( "bank7_0.1.bin", 0x1c000, 0x4000, CRC(d8e821b2) SHA1(18245a75529d2f496cdbdc28cdf40def157b20c0),ROM_BIOS(1))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT CONFIG COMPANY   FULLNAME       FLAGS */
COMP( 1991, pce220,  0,       0, 	pce220, 	pce220,  0,   0,  	 "Sharp",   "PC-E220",		GAME_NOT_WORKING)

