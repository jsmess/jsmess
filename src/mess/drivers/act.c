/***************************************************************************

        ACT Apricot series

        07/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/i86/i86.h"


static ADDRESS_MAP_START(act_mem, ADDRESS_SPACE_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000,0xf7fff) AM_RAM
	AM_RANGE(0xf8000,0xfffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( act_io , ADDRESS_SPACE_IO, 16)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( act )
INPUT_PORTS_END


static MACHINE_RESET(act)
{
}

static VIDEO_START( act )
{
}

static VIDEO_UPDATE( act )
{
	return 0;
}

static MACHINE_DRIVER_START( act )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", I8086, 4670000)
	MDRV_CPU_PROGRAM_MAP(act_mem)
	MDRV_CPU_IO_MAP(act_io)

	MDRV_MACHINE_RESET(act)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_SCREEN_VISIBLE_AREA(0, 512-1, 0, 256-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(act)
	MDRV_VIDEO_UPDATE(act)
MACHINE_DRIVER_END


/* ROM definition */
// not sure the ROMs are loaded correctly
ROM_START( aprixi )
	ROM_REGION( 0x100000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "lo_ve007.u11", 0xfc000, 0x2000, CRC(e74e14d1) SHA1(569133b0266ce3563b21ae36fa5727308797deee) )	// Labelled LO Ve007 03.04.84
	ROM_LOAD16_BYTE( "hi_ve007.u9",  0xfc001, 0x2000, CRC(b04fb83e) SHA1(cc2b2392f1b4c04bb6ec8ee26f8122242c02e572) )	// Labelled HI Ve007 03.04.84
ROM_END

ROM_START( aprif1 )
	ROM_REGION( 0x100000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "lo_f1_1.6.8f",  0xf8000, 0x4000, CRC(be018be2) SHA1(80b97f5b2111daf112c69b3f58d1541a4ba69da0) )	// Labelled F1 - LO Vr. 1.6
	ROM_LOAD16_BYTE( "hi_f1_1.6.10f", 0xf8001, 0x4000, CRC(bbba77e2) SHA1(e62bed409eb3198f4848f85fccd171cd0745c7c0) )	// Labelled F1 - HI Vr. 1.6
ROM_END

ROM_START( aprif10 )
	ROM_REGION( 0x100000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "lo_f10_3.1.1.8f",  0xf8000, 0x4000, CRC(bfd46ada) SHA1(0a36ef379fa9af7af9744b40c167ce6e12093485) )	// Labelled LO-FRange Vr3.1.1
	ROM_LOAD16_BYTE( "hi_f10_3.1.1.10f", 0xf8001, 0x4000, CRC(67ad5b3a) SHA1(a5ececb87476a30167cf2a4eb35c03aeb6766601) )	// Labelled HI-FRange Vr3.1.1
ROM_END

ROM_START( aprifp )
	ROM_REGION( 0x100000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "lo_fp_3.1.ic20", 0xf8000, 0x4000, CRC(0572add2) SHA1(c7ab0e5ced477802e37f9232b5673f276b8f5623) )	// Labelled 11212721 F97E PORT LO VR 3.1
	ROM_LOAD16_BYTE( "hi_fp_3.1.ic9",  0xf8001, 0x4000, CRC(3903674b) SHA1(8418682dcc0c52416d7d851760fea44a3cf2f914) )	// Labelled 11212721 BD2D PORT HI VR 3.1
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT MACHINE INPUT   INIT  CONFIG COMPANY  FULLNAME                 FLAGS */
COMP( 1984, aprixi,    0,    0,     act,    act,    0,    0,     "ACT",   "Apricot Xi",            GAME_NOT_WORKING)
COMP( 1984, aprif1,    0,    0,     act,    act,    0,    0,     "ACT",   "Apricot F1",            GAME_NOT_WORKING)
COMP( 1985, aprif10,   0,    0,     act,    act,    0,    0,     "ACT",   "Apricot F10",           GAME_NOT_WORKING)
COMP( 1984, aprifp,    0,    0,     act,    act,    0,    0,     "ACT",   "Apricot Portable / FP", GAME_NOT_WORKING)
