/*

Master Boy

-- It might be impossible to emulate this, unless the external data can somehow be manipulated into dumping the internal rom.

Produttore  Gaelco
N.revisione rev A - Italian
CPU
1x HD647180X0CP6-1M1R (main)(on a small piggyback)
1x SAA1099P (sound)
1x OKI5205 (sound)
1x crystal resonator POE400B (close to sound)
1x oscillator 24.000000MHz (close to main)
ROMs
1x M27C2001 (1)
4x AM27C010 (2,5,6,7)
2x TMS27C512 (3,4)

*/

#include "driver.h"

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0bfff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0bfff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END


INPUT_PORTS_START( mastboy )
INPUT_PORTS_END

static const gfx_layout tiles8x8_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 24,28,16,20,8,12,0,4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static const gfx_layout tiles8x8_layout_2 =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0,4,8,12,16,20,24,28 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};


static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles8x8_layout, 0, 16 },
	{ REGION_GFX2, 0, &tiles8x8_layout_2, 0, 16 },
	{ -1 }
};

VIDEO_START(mastboy)
{
	return 0;
}

VIDEO_UPDATE(mastboy)
{

	return 0;
}


static MACHINE_DRIVER_START( mastboy )
	MDRV_CPU_ADD_TAG("main", Z180, 8000000)	/* HD647180X0CP6-1M1R */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
//  MDRV_CPU_IO_MAP(readport,writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)


	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_SCREEN_VISIBLE_AREA(0, 256-1, 16, 256-16-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x100)

	MDRV_VIDEO_START(mastboy)
	MDRV_VIDEO_UPDATE(mastboy)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
MACHINE_DRIVER_END



ROM_START( mastboy )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "hd647180x0cp6.cpu", 0x00000, 0x10000, NO_DUMP ) // game code is internal to the CPU!

	ROM_REGION( 0x70000, REGION_GFX1, 0 ) /* these 3 are close together, might be accessed directly by the cpu / gfx / sound chips.. */
	ROM_LOAD( "01.bin", 0x00000, 0x40000, CRC(36755831) SHA1(706fba5fc765502774643bfef8a3c9d2c01eb01b) ) // 99% gfx
	ROM_LOAD( "02.bin", 0x40000, 0x20000, CRC(69cf6b7c) SHA1(a7bdc62051d09636dcd54db102706a9b42465e63) ) // data of some kind.. (for cpu?)
	ROM_LOAD( "03.bin", 0x60000, 0x10000, CRC(5020a37f) SHA1(8bc75623232f3ab457b47d5af6cd1c3fb24c0d0e) ) // sound data? (+ 1 piece of) 1ST AND 2ND HALF IDENTICAL

	ROM_REGION( 0x10000, REGION_GFX2, 0 ) /* this is further away, and a different format to the above gfx */
	ROM_LOAD( "04.bin", 0x00000, 0x10000, CRC(565932f4) SHA1(4b184aa445b5671072031ad4a2ccb13868d6d3a4) )

	ROM_REGION( 0x20000*6, REGION_USER1, 0 ) /* question data - 6 sockets */
	ROM_LOAD( "05.bin", 0x00000, 0x20000, CRC(394cb674) SHA1(1390c666772f1e1e2da8866b960a3d24dc660e68) )
	ROM_LOAD( "06.bin", 0x20000, 0x20000, CRC(aace7120) SHA1(5655b56a7c241bc7908081088042601174c0a0b2) )
	ROM_LOAD( "07.bin", 0x40000, 0x20000, CRC(6618b002) SHA1(79942350da335a3362b6fc43527b6568ce134ceb) )
	ROM_LOAD( "08.bin", 0x60000, 0x20000, CRC(6a4870dd) SHA1(f8ca94a5bc4ba3f512767901e4ae3579c2c6355a) )
ROM_END

ROM_START( mastboyi )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "hd647180x0cp6.cpu", 0x00000, 0x10000, NO_DUMP ) // game code is internal to the CPU!

	ROM_REGION( 0x70000, REGION_GFX1, 0 ) /* these 3 are close together, might be accessed directly by the cpu / gfx / sound chips.. */
	ROM_LOAD( "1-mem-c.ic75", 0x00000, 0x40000, CRC(7c7b1cc5) SHA1(73ad7bdb61d1f99ce09ef3a5a3ae0f1e72364eee) ) // 99% gfx
	ROM_LOAD( "2-mem-b.ic76", 0x40000, 0x20000, CRC(87015c18) SHA1(a16bf2707ce847da0923662796195b75719a6d77) ) // data of some kind.. (for cpu?)
	ROM_LOAD( "3-mem-a.ic77", 0x60000, 0x10000, CRC(3ee33282) SHA1(26371e3bb436869461e9870409b69aa9fb1845d6) ) // sound data? (+ 1 piece of) 1ST AND 2ND HALF IDENTICAL

	ROM_REGION( 0x10000, REGION_GFX2, 0 ) /* this is further away, and a different format to the above gfx */
	ROM_LOAD( "4.ic91", 0x00000, 0x10000, CRC(858d7b27) SHA1(b0ddf49df5665003f3616d67f7fc27408433483b) )

	ROM_REGION( 0x20000*6, REGION_USER1, 0 ) /* question data - 6 sockets */
	ROM_LOAD( "5-rom.ic95", 0x00000, 0x20000, CRC(adc07f12) SHA1(2e0b46ac5884ad459bc354f56ff384ff1932f147) )
	ROM_LOAD( "6-rom.ic96", 0x20000, 0x20000, CRC(2c52cb1e) SHA1(d58f21c09bd3983497f74ab6c5a37977d9e30f0c) )
	ROM_LOAD( "7-rom.ic97", 0x40000, 0x20000, CRC(7818408f) SHA1(2a69688b6cda5baf2a45966dd86f10b2fcd54b66) )
ROM_END

GAME( 1991, mastboy,  0,          mastboy, mastboy, 0, ROT0, "Gaelco", "Master Boy (Spanish, PCB Rev A)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 1991, mastboyi, mastboy,    mastboy, mastboy, 0, ROT0, "Gaelco", "Master Boy (Italian, PCB Rev A)", GAME_NO_SOUND|GAME_NOT_WORKING )
