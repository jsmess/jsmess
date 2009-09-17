/***************************************************************************

    JR-200

    12/05/2009 Skeleton driver.

	http://www.armchairarcade.com/neo/node/1598

****************************************************************************/

/*

	TODO:

	- MN1544 4-bit CPU core and ROM dump

*/

#include "driver.h"
#include "cpu/m6800/m6800.h"

static UINT8 *textram;

static READ8_HANDLER( test1_r )
{
	if (debug_global_input_code_pressed(KEYCODE_Q)) return 0x50;
return 0x50;
}

static READ8_HANDLER( test2_r )
{
	if (debug_global_input_code_pressed(KEYCODE_Q)) return 0x01;
return 0x01;
}

static READ8_HANDLER( test3_r )
{
	if (debug_global_input_code_pressed(KEYCODE_Q)) return 0x10;
return 0x10;
}

/*
	0000-3fff RAM
	4000-4fff RAM ( 4k expansion)
	4000-7fff RAM (16k expansion)
	4000-bfff RAM (32k expansion)
	c100-c3FF text code
	c500-c7ff text color
	ca00      border color
*/ 

static ADDRESS_MAP_START(jr200_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x000c) AM_RAM
	AM_RANGE(0x000d, 0x000d) AM_RAM
	AM_RANGE(0x000e, 0x3fff) AM_RAM
	AM_RANGE(0xa000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xc7ff) AM_RAM AM_BASE(&textram)
	AM_RANGE(0xc801, 0xc801) AM_RAM AM_READ(test1_r)
	AM_RANGE(0xc81c, 0xc81c) AM_RAM AM_READ(test2_r)
	AM_RANGE(0xc81d, 0xc81d) AM_RAM AM_READ(test3_r)
	AM_RANGE(0xe000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( jr200_io, ADDRESS_SPACE_IO, 8)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( jr200 )
INPUT_PORTS_END


static MACHINE_RESET(jr200)
{
}

static VIDEO_START( jr200 )
{
}

static VIDEO_UPDATE( jr200 )
{
	int i,j;
	const gfx_element *gfx = screen->machine->gfx[0];

	for (i = 0; i < 0x02ff; i += 0x20)
		for (j = 0; j < 0x20; j++)
		{
			UINT8 code = textram[0x0100 + i + j];
			UINT8 col_bg = (textram[0x0500 + i + j] >> 3) & 0x07;
			UINT8 col_fg = (textram[0x0500 + i + j] >> 0) & 0x07;

			if (1==0) drawgfx_transpen(bitmap, cliprect, gfx,
					0, col_bg,
					0, 0,
					j * 8, (i >> 5) * 8, 15);
			drawgfx_transpen(bitmap, cliprect, gfx,
					code, col_fg,
					0, 0,
					j * 8, (i >> 5) * 8, 15);
		}
	return 0;
}

static const gfx_layout tiles8x8_layout =
{
	8,8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static GFXDECODE_START( jr200 )
	GFXDECODE_ENTRY( "char", 0, tiles8x8_layout, 0, 0x100 )
GFXDECODE_END


static INTERRUPT_GEN( jr200_irq )
{
	cpu_set_input_line(device, 0, HOLD_LINE);

}

static INTERRUPT_GEN( jr200_nmi )
{
	cpu_set_input_line(device, INPUT_LINE_NMI, PULSE_LINE);
}

static MACHINE_DRIVER_START( jr200 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", M6802, XTAL_4MHz) /* MN1800A */
	MDRV_CPU_PROGRAM_MAP(jr200_mem)
	MDRV_CPU_IO_MAP(jr200_io)
MDRV_CPU_VBLANK_INT("screen", jr200_irq)
MDRV_CPU_PERIODIC_INT(jr200_nmi,60)
/*
	MDRV_CPU_ADD("mn1544", MN1544, ?)
*/
	MDRV_MACHINE_RESET(jr200)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 192)
	MDRV_SCREEN_VISIBLE_AREA(0, 256-1, 0, 192-1)

	MDRV_GFXDECODE(jr200)
	MDRV_PALETTE_LENGTH(255)

	MDRV_VIDEO_START(jr200)
	MDRV_VIDEO_UPDATE(jr200)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(jr200)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( jr200 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "basic.rom",  0xa000, 0x2000, CRC(cc53eb52) SHA1(910927b98a8338ba072173d79613422a8cb796da) )
	ROM_LOAD( "jr200u.bin", 0xe000, 0x2000, CRC(37ca3080) SHA1(17d3fdedb4de521da7b10417407fa2b61f01a77a) )

	ROM_REGION( 0x10000, "mn1544", ROMREGION_ERASEFF )
	ROM_LOAD( "mn1544.bin",  0x0000, 0x0400, NO_DUMP )

	ROM_REGION( 0x0800, "char", ROMREGION_ERASEFF )
	ROM_LOAD( "char.rom", 0x0000, 0x0800, CRC(cb641624) SHA1(6fe890757ebc65bbde67227f9c7c490d8edd84f2) )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( 1982, jr200,  0,       0, 	jr200, 	jr200, 	 0,  	  jr200,  	 "National",   "JR-200",		GAME_NOT_WORKING )
