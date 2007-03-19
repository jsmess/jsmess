/*

 Two Minute Drill

 Taito

 Half Video, Half Mechanical?

*/

/*

TWO MINUTE DRILL - Taito 1993?


No idea what this game is... I do not have the pinout




See pic for more details





 Brief hardware overview:
 ------------------------

 Main processor   - 68000 16Mhz

 Sound            - Yamaha YM2610B



 Taito custom ICs - TC0400YSC
                  - TC0260DAR
                  - TC0630FDP
                  - TC0510NI0

DAC               -26.6860Mhz
          -32.0000Mhz



See pic for more details


*/

#include "driver.h"

VIDEO_START( drill )
{
	return 0;
}

VIDEO_UPDATE( drill )
{
	return 0;
}

READ16_HANDLER( drill_unk_r )
{
	return mame_rand(Machine);
}

static ADDRESS_MAP_START( drill_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x200000, 0x20ffff) AM_RAM
	AM_RANGE(0x400000, 0x43ffff) AM_RAM // video stuff

	AM_RANGE(0x500000, 0x501fff) AM_RAM AM_WRITE(paletteram16_RRRRGGGGBBBBRGBx_word_w) AM_BASE(&paletteram16)

	AM_RANGE(0x700000, 0x70000f) AM_READ(drill_unk_r) AM_WRITE(MWA16_NOP)

ADDRESS_MAP_END



INPUT_PORTS_START( drill )
INPUT_PORTS_END


static const gfx_layout drill_layout =
{
	16,16,
	RGN_FRAC(1,2),
	6,
	{ 0,1,2,3 , RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+1 }, // order may be wrong
	{ 20, 16, 28, 24, 4, 0, 12, 8,        52, 48, 60, 56, 36, 32, 40, 44 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64, 8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	16*64
};



static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &drill_layout,  0x0, 256  },
	{ -1 } /* end of array */
};

void drill_interrupt5(int x)
{
	cpunum_set_input_line(0,5,HOLD_LINE);
}

static INTERRUPT_GEN( drill_interrupt )
{
	timer_set(TIME_IN_CYCLES(5000,0),0,drill_interrupt5);
	cpunum_set_input_line(0, 4, HOLD_LINE);
}


static MACHINE_DRIVER_START( drill )
	MDRV_CPU_ADD_TAG("main", M68000, 16000000 ) // ?
	MDRV_CPU_PROGRAM_MAP(drill_map,0)
	MDRV_CPU_VBLANK_INT(drill_interrupt,1)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(8*8, 48*8-1, 2*8, 30*8-1)
	MDRV_PALETTE_LENGTH(0x1000)

	MDRV_VIDEO_START(drill)
	MDRV_VIDEO_UPDATE(drill)
MACHINE_DRIVER_END


ROM_START( 2mindril )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "d58-38.ic11", 0x00000, 0x40000, CRC(c58e8e4f) SHA1(648db679c3bfb5de1cd6c1b1217773a2fe56f11b) )
	ROM_LOAD16_BYTE( "d58-37.ic9",  0x00001, 0x40000, CRC(19e5cc3c) SHA1(04ac0eef893c579fe90d91d7fd55c5741a2b7460) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "d58-11.ic31", 0x000000, 0x200000,  CRC(dc26d58d) SHA1(cffb18667da18f5367b02af85a2f7674dd61ae97) )

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_ERASE00 )
	ROM_LOAD32_WORD( "d58-09.ic28", 0x000000, 0x200000, CRC(d8f6a86a) SHA1(d6b2ec309e21064574ee63e025ae4716b1982a98) )
	ROM_LOAD32_WORD( "d58-08.ic27", 0x000002, 0x200000, CRC(9f5a3f52) SHA1(7b696bd823819965b974c853cebc1660750db61e) )

	ROM_REGION( 0x400000, REGION_GFX2, 0 )
	ROM_LOAD32_WORD( "d58-10.ic29", 0x000000, 0x200000, CRC(74c87e08) SHA1(f39b3a64f8338ccf5ca6eb76cee92a10fe0aad8f) )
ROM_END

DRIVER_INIT( drill )
{
	// rearrange gfx roms to something we can decode, two of the roms form 4bpp of the graphics, the third forms another 2bpp but is in a different format
	UINT32 *src    = (UINT32*)memory_region       ( REGION_GFX2 );
	UINT32 *dst    = (UINT32*)memory_region       ( REGION_GFX1 );// + 0x400000;
	int i;


	for (i=0; i< 0x400000/4; i++)
	{
		UINT32 dat1 = src[i];
	    dat1 = BITSWAP32(dat1, 3, 11, 19, 27, 2, 10, 18, 26, 1, 9, 17, 25, 0, 8, 16, 24, 7, 15, 23, 31, 6, 14, 22, 30, 5, 13, 21, 29, 4, 12, 20, 28 );
		dst[(0x400000/4)+i] = dat1;
	}
}

GAME( 1993, 2mindril,    0,        drill,    drill,    drill, ROT0,  "Taito", "Two Minute Drill", GAME_NO_SOUND|GAME_NOT_WORKING )
