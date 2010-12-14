#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "machine/abc99.h"
#include "devices/messram.h"


class abc1600_state : public driver_device
{
public:
	abc1600_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};


#define M68008_TAG "m68008"

static ADDRESS_MAP_START( abc1600_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x03fff) AM_ROM
	AM_RANGE(0x04000, 0xfffff) AM_RAM
ADDRESS_MAP_END

static INPUT_PORTS_START( abc1600 )
INPUT_PORTS_END

static VIDEO_UPDATE( abc1600 )
{
	return 0;
}

static MACHINE_CONFIG_START( abc1600, abc1600_state )
	/* basic machine hardware */
	MDRV_CPU_ADD(M68008_TAG, M68008, 8000000)
	MDRV_CPU_PROGRAM_MAP(abc1600_mem)

	/* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(1024, 768)
    MDRV_SCREEN_VISIBLE_AREA(0, 1024-1, 0, 768-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_UPDATE(abc1600)

	/* sound hardware */

	/* devices */
	MDRV_ABC99_ADD(ABC99_TAG)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("1M")
MACHINE_CONFIG_END

ROM_START( abc1600 )
	ROM_REGION( 0x4000, M68008_TAG, 0 )
	ROM_LOAD( "boot 6490356-04.1f", 0x0000, 0x4000, CRC(9372f6f2) SHA1(86f0681f7ef8dd190b49eda5e781881582e0c2a4) )

	ROM_REGION( 0x2000, "wrmsk", 0 )
	ROM_LOAD( "wrmskl 6490362-01.1g", 0x0000, 0x1000, CRC(bc737538) SHA1(80e2c3757eb7f713018808d6e41ebef612425028) )
	ROM_LOAD( "wrmskh 6490363-01.1j", 0x1000, 0x1000, CRC(6b7c9f0b) SHA1(7155a993adcf08a5a8a2f22becf9fd66fda698be) )

	ROM_REGION( 0x200, "shinf", 0 )
	ROM_LOAD( "shinf 6490361-01.1f", 0x0000, 0x0200, CRC(20260f8f) SHA1(29bf49c64e7cc7592e88cde2768ac57c7ce5e085) )

	ROM_REGION( 0x40, "drmsk", 0 )
	ROM_LOAD( "drmskl 6490359-01.1k", 0x0000, 0x0020, CRC(6e71087c) SHA1(0acf67700d6227f4b315cf8fb0fb31c0e7fb9496) )
	ROM_LOAD( "drmskh 6490358-01.1k", 0x0020, 0x0020, CRC(a4a9a9dc) SHA1(d8575c0335d6021cbb5f7bcd298b41c35294a80a) )

	ROM_REGION( 2459, "plds", 0 ) // TODO convert to binary with jedutil
	ROM_LOAD( "drmsk 6490360-01.jed", 0x0000, 2459, CRC(22eca315) SHA1(0302fc9828fa148c7a657b615494c22a006ebd2e) )
	ROM_LOAD( "1020 6490349-01.8b",   0x0000, 2459, CRC(44e5217a) SHA1(e54e353a8e117222b03d0b03842b0b2cf467e876) )
	ROM_LOAD( "1021 6490350-01.5d",   0x0000, 2459, CRC(e48cb95f) SHA1(5988074aea3baf16f731f956d11492d74eaffddb) )
	ROM_LOAD( "1022 6490351-01.17e",  0x0000, 2459, CRC(8a3cb5d5) SHA1(4ac9f033a7b098afd014527b7bcb19d0f1a3d717) )
	ROM_LOAD( "1023 6490352-01.11e",  0x0000, 2459, CRC(97280a0e) SHA1(2be0cfa68cf3cc81c92152bee9cb8ad3efbf2225) )
	ROM_LOAD( "1024 6490353-01.12e",  0x0000, 2451, CRC(20fbae6d) SHA1(481a964eaa2662abfa383df56fc7a66685894bba) )
	ROM_LOAD( "1025 6490354-01.6e",   0x0000, 2451, CRC(9794dbab) SHA1(76bc104875d85dbb2ba6cae2143812d5a4d509b0) )
ROM_END

/*    YEAR  NAME      PARENT  COMPAT  MACHINE   INPUT     INIT  COMPANY     FULLNAME     FLAGS */
COMP( 1985, abc1600, 0,      0,      abc1600, abc1600, 0,    "Luxor", "ABC 1600", GAME_NOT_WORKING | GAME_NO_SOUND )
