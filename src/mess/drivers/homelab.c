/***************************************************************************

        Homelab driver by Miodrag Milanovic

        31/08/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "cpu/z80/z80.h"
#include "includes/homelab.h"

/* Address maps */
static ADDRESS_MAP_START(homelab2_mem, ADDRESS_SPACE_PROGRAM, 8)
    AM_RANGE( 0x0000, 0x07ff ) AM_ROM  // ROM 1 
    AM_RANGE( 0x0800, 0x0fff ) AM_ROM  // ROM 2 
    AM_RANGE( 0x1000, 0x17ff ) AM_ROM  // ROM 3
    AM_RANGE( 0x1800, 0x1fff ) AM_ROM  // ROM 4
    AM_RANGE( 0x2000, 0x27ff ) AM_ROM  // ROM 5
    AM_RANGE( 0x2800, 0x2fff ) AM_ROM  // ROM 6
		AM_RANGE( 0x3000, 0x37ff ) AM_ROM  // Empty
    AM_RANGE( 0x4000, 0x7fff ) AM_RAM
    AM_RANGE( 0x8000, 0xbfff ) AM_NOP
    AM_RANGE( 0xc000, 0xc3ff ) AM_RAM AM_MIRROR(0x3c00) // Video RAM 1K    
ADDRESS_MAP_END

static ADDRESS_MAP_START(homelab3_mem, ADDRESS_SPACE_PROGRAM, 8)
    AM_RANGE( 0x0000, 0x3fff ) AM_ROM 
    AM_RANGE( 0x4000, 0x7fff ) AM_RAM
    AM_RANGE( 0x8000, 0xdfff ) AM_NOP
    AM_RANGE( 0xe000, 0xefff ) AM_RAM // Keyboard
    AM_RANGE( 0xf000, 0xf7ff ) AM_RAM AM_MIRROR(0x0800)// Video RAM
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( homelab )
	PORT_START("LINE0")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Delete") PORT_CODE(KEYCODE_BACKSPACE)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Enter") PORT_CODE(KEYCODE_ENTER)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CTL") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("GR") PORT_CODE(KEYCODE_LALT) PORT_CODE(KEYCODE_RALT)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT)
INPUT_PORTS_END

/* Machine driver */
static MACHINE_DRIVER_START( homelab )
/* basic machine hardware */
	MDRV_CPU_ADD("main", Z80, 3000000)
	MDRV_CPU_PROGRAM_MAP(homelab2_mem, 0)
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)

	/* video hardware */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(40*8, 25*8)
	MDRV_SCREEN_VISIBLE_AREA(0, 40*8-1, 0, 25*8-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT( black_and_white )

	MDRV_VIDEO_START( homelab )
	MDRV_VIDEO_UPDATE( homelab )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( homelab3 )
/* basic machine hardware */
	MDRV_IMPORT_FROM(homelab)
  MDRV_CPU_MODIFY("main")     
	MDRV_CPU_PROGRAM_MAP(homelab3_mem, 0)
	
	MDRV_SCREEN_MODIFY("main")
	MDRV_SCREEN_SIZE(80*8, 25*8)
	MDRV_SCREEN_VISIBLE_AREA(0, 80*8-1, 0, 25*8-1)
	MDRV_VIDEO_UPDATE( homelab3 )
	
MACHINE_DRIVER_END

/* ROM definition */

ROM_START( homelab2 )
    ROM_REGION( 0x10000, "main", ROMREGION_ERASEFF )
    ROM_LOAD( "hl2_1.rom", 0x0000, 0x0800, CRC(205365F7) )
    ROM_LOAD( "hl2_2.rom", 0x0800, 0x0800, CRC(696AF3C1) )
    ROM_LOAD( "hl2_3.rom", 0x1000, 0x0800, CRC(69E57E8C) )
  	ROM_LOAD( "hl2_4.rom", 0x1800, 0x0800, CRC(97CBBE74) )
		ROM_REGION(0x0800, "gfx1",0)
		ROM_LOAD ("hl2.chr", 0x0000, 0x0800, CRC(2E669D40))
ROM_END

ROM_START( homelab3 )
    ROM_REGION( 0x10000, "main", ROMREGION_ERASEFF )
    ROM_LOAD( "hl3_1.rom", 0x0000, 0x1000, CRC(6B90A8EA) )
    ROM_LOAD( "hl3_2.rom", 0x1000, 0x1000, CRC(BCAC3C24) )
    ROM_LOAD( "hl3_3.rom", 0x2000, 0x1000, CRC(AB1B4AB0) )
  	ROM_LOAD( "hl3_4.rom", 0x3000, 0x1000, CRC(BF67EFF9) )
		ROM_REGION(0x0800, "gfx1",0)
		ROM_LOAD ("hl3.chr", 0x0000, 0x0800, CRC(F58EE39B))
ROM_END

ROM_START( homelab4 )
    ROM_REGION( 0x10000, "main", ROMREGION_ERASEFF )
    ROM_LOAD( "hl4_1.rom", 0x0000, 0x1000, CRC(A549B2D4) )
    ROM_LOAD( "hl4_2.rom", 0x1000, 0x1000, CRC(151D33E8) )
    ROM_LOAD( "hl4_3.rom", 0x2000, 0x1000, CRC(39571AB1) )
  	ROM_LOAD( "hl4_4.rom", 0x3000, 0x1000, CRC(F4B77CA2) )
		ROM_REGION(0x0800, "gfx1",0)
		ROM_LOAD ("hl4.chr", 0x0000, 0x0800, CRC(F58EE39B))
ROM_END

/* Driver */

/*    YEAR  NAME   PARENT  COMPAT  MACHINE  INPUT   INIT  CONFIG COMPANY                 FULLNAME   FLAGS */
COMP( 1982, homelab2,   0	,0, 	homelab, 	homelab, 	homelab, NULL,  "", "Homelab 2 / Aircomp 16",		 GAME_NOT_WORKING)
COMP( 1983, homelab3,   homelab2,0, 	homelab3, 	homelab, 	homelab, NULL,  "", "Homelab 3",		 GAME_NOT_WORKING)
COMP( 1984, homelab4,   homelab2,0, 	homelab3, 	homelab, 	homelab, NULL,  "", "Homelab 4",		 GAME_NOT_WORKING)
