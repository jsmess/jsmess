/***************************************************************************

Koi Koi

Basic hw is...
z80 (possibly xtal/4)
ay3-8910 (possibly xtal/8)
15.468xtal
1 dsw (8)
2kx8 SRAM (x2)

***************************************************************************/

#include "driver.h"

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x2fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x6000, 0x67ff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x2fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x6000, 0x67ff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END



INPUT_PORTS_START( koikoi )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY

INPUT_PORTS_END


static const gfx_layout tilelayout =
{
	8, 8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(0,3), RGN_FRAC(1,3), RGN_FRAC(2,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &tilelayout,      0, 32 },
	{ -1 } /* end of array */

};

VIDEO_START(koikoi)
{
	return 0;
}

VIDEO_UPDATE(koikoi)
{
	return 0;
}

#define KOIKOI_CRYSTAL 15468000

static MACHINE_DRIVER_START( koikoi )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,KOIKOI_CRYSTAL/4)	/* ?? */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	//MDRV_CPU_IO_MAP(readport,writeport)
	//MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 30*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16)
	MDRV_COLORTABLE_LENGTH(4*32)

	//MDRV_PALETTE_INIT(mrjong)
	MDRV_VIDEO_START(koikoi)
	MDRV_VIDEO_UPDATE(koikoi)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/


ROM_START( koikoi )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* code */
	ROM_LOAD( "ic56", 0x0000, 0x1000, CRC(bdc68f9d) SHA1(c45fbc95abb37f750acc1d9f3b35ad0f41af097d) )
	ROM_LOAD( "ic55", 0x1000, 0x1000, CRC(fe09248a) SHA1(c192795678068e387bd406f5cd1c5aba5f5ef66a) )
	ROM_LOAD( "ic54", 0x2000, 0x1000, CRC(925fc57c) SHA1(4c79df92b6617fe84e61359c8e6e3b907b138777) )

	ROM_REGION( 0x3000, REGION_GFX1, 0 )	/* gfx */
	ROM_LOAD( "ic33", 0x0000, 0x1000, CRC(9e4d563b) SHA1(63664dcffc2eb198a161c73131b95a66b2067424) )
	ROM_LOAD( "ic26", 0x1000, 0x1000, CRC(79cb1e93) SHA1(4d08b3d88727b437673f7a51d47396f19bbc3caa) )
	ROM_LOAD( "ic18", 0x2000, 0x1000, CRC(c209362d) SHA1(0620c19fe72e8407db0f487b6413c5d45ac8046c) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )	/* color? */
	ROM_LOAD( "prom.ic23", 0x000, 0x100,  CRC(f1d169a6) SHA1(5ee4b1dfe61e8b97a90cc113ba234298189f1a73) )
ROM_END



GAME( 1983, koikoi,   0,      koikoi, koikoi, 0, ROT0, "Kiwako", "Koi Koi", GAME_NOT_WORKING|GAME_NO_SOUND )
