/***************************************************************************

      Camputers Lynx

      05/2009 Skeleton driver.

      The Lynx was an 8-bit British home computer that was first released 
      in early 1983 as a 48 kB model. 
      The designer of the Lynx was John Shireff and several models were 
      available with 48 kB, 96 kB (from Sep 1983) or 128 kB RAM (from Dec 
      1983). It was possible reach 192 kB with RAM expansions on-board.

      The machine was based around a Z80A CPU clocked at 4 MHz, and featured 
      a Motorola 6845 as video controller. It was possible to run CP/M with 
      the optional 5.25" floppy disk-drive on the 96 kB and 128 kB models.
      Approximately 30,000 Lynx units were sold world-wide.

      Camputers ceased trading in June 1984. Anston Technology took over in 
      November the same year and a re-launch was planned but never happened. 

      In June 1986, Anston sold everything - hardware, design rights and 
      thousands of cassettes - to the National Lynx User Group. The group 
      planned to produce a Super-Lynx but was too busy supplying spares and 
      technical information to owners of existing models, and the project never 
      came into being.

      Hardware info:
       - CPU: Zilog Z80A 4 MHz
       - CO-PROCESSOR: Motorola 6845 (CRT controller)
       - RAM: 48 kb, 96 kb or 128 kb depending on models (max. 192 kb)
       - ROM: 16 kb (48K version), 24 KB (96K and 128K versions)
       - TEXT MODES: 40 x 24, 80 x 24
       - GRAPHIC MODES: 256 x 248, 512 x 480
       - SOUND: one voice beeper

      Lynx 128 Memory Map

              | 0000    2000    4000    6000    8000    a000    c000     e000
              | 1fff    3fff    5fff    7fff    9fff    bfff    dfff     ffff
     -------------------------------------------------------------------------
              |                      |                        |       |
      Bank 0  |      BASIC ROM       |    Not Available       |  Ext  |  Ext
              |                      |                        |  ROM1 |  ROM2
     -------------------------------------------------------------------------
              |                      |   
      Bank 1  |        STORE         |           Workspace RAM
              |                      |    
     -------------------------------------------------------------------------
              |               |               |               |
      Bank 2  |       RED     |     BLUE      |     GREEN     |     Alt
              |               |               |               |    Green
     -------------------------------------------------------------------------
              |   
      Bank 3  |              Available for Video Expansion
              |   
     -------------------------------------------------------------------------
              |         
      Bank 4  |              Available for User RAM Expansion
              |       


****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "video/mc6845.h"

static const device_config *mc6845;

static ADDRESS_MAP_START( camplynx_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000,0x3fff) AM_ROM	/* for Lynx 48k only */
	AM_RANGE(0x4000,0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( camplynx_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x86, 0x86) AM_DEVWRITE("crtc", mc6845_address_w)
	AM_RANGE(0x87, 0x87) AM_DEVREADWRITE("crtc", mc6845_register_r,mc6845_register_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( camplynx )
INPUT_PORTS_END


static MACHINE_RESET( camplynx )
{
}

static MC6845_UPDATE_ROW( camplynx_update_row )
{
}

static VIDEO_START( camplynx )
{
	mc6845 = devtag_get_device(machine, "crtc");
}

static VIDEO_UPDATE( camplynx )
{
	mc6845_update(mc6845, bitmap, cliprect);
	return 0;
}


static const mc6845_interface camplynx_crtc6845_interface = {
	"screen",
	8 /*?*/,
	NULL,
	camplynx_update_row,
	NULL,
	NULL,
	NULL,
	NULL
};


static MACHINE_DRIVER_START( camplynx )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, XTAL_4MHz)
	MDRV_CPU_PROGRAM_MAP(camplynx_mem)
	MDRV_CPU_IO_MAP(camplynx_io)

	MDRV_MACHINE_RESET(camplynx)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_MC6845_ADD("crtc", MC6845, XTAL_12MHz / 8 /*? dot clock divided by dots per char */, camplynx_crtc6845_interface)

	MDRV_VIDEO_START(camplynx)
	MDRV_VIDEO_UPDATE(camplynx)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( camplynx )
	ROM_REGION( 0x4000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "lynx48-1.rom", 0x0000, 0x2000, CRC(56feec44) SHA1(7ded5184561168e159a30fa8e9d3fde5e52aa91a) )
	ROM_LOAD( "lynx48-2.rom", 0x2000, 0x2000, CRC(d894562e) SHA1(c08a78ecb4eb05baa4c52488fce3648cd2688744) )
ROM_END

ROM_START( camply96 )
	ROM_REGION( 0x8000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "lynx96-1.rom", 0x0000, 0x2000, CRC(56feec44) SHA1(7ded5184561168e159a30fa8e9d3fde5e52aa91a) )
	ROM_LOAD( "lynx96-2.rom", 0x2000, 0x2000, CRC(d894562e) SHA1(c08a78ecb4eb05baa4c52488fce3648cd2688744) )
	ROM_LOAD( "lynx96-3.rom", 0x4000, 0x2000, CRC(21f11709) SHA1(f86a729b01de286197c550974f7825c12815a4f4) )
	ROM_LOAD( "dosrom.rom", 0x6000, 0x2000, CRC(011e106a) SHA1(e77f0ca99790551a7122945f3194516b2390fb69) )
ROM_END

ROM_START( camply128 )
	ROM_REGION( 0x8000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "lynx128-1.rom", 0x0000, 0x2000, CRC(65d292ce) SHA1(36567c2fbd9cf72f758e8cb80c21cb4d82040752) )
	ROM_LOAD( "lynx128-2.rom", 0x2000, 0x2000, CRC(23288773) SHA1(e12a7ebea3fae5eb375c03e848dbb81070d9d189) )
	ROM_LOAD( "lynx128-3.rom", 0x4000, 0x2000, CRC(9827b9e9) SHA1(1092367b2af51c72ce9be367179240d692aeb131) )
	ROM_LOAD( "dosrom.rom", 0x6000, 0x2000, CRC(011e106a) SHA1(e77f0ca99790551a7122945f3194516b2390fb69) )
ROM_END


static SYSTEM_CONFIG_START(camp48)
	CONFIG_RAM_DEFAULT(48 * 1024)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(camp96)
	CONFIG_RAM_DEFAULT(96 * 1024)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(camp128)
	CONFIG_RAM_DEFAULT(128 * 1024)
SYSTEM_CONFIG_END


/* Driver */
/*    YEAR  NAME       PARENT     COMPAT   MACHINE    INPUT     INIT  CONFIG    COMPANY       FULLNAME     FLAGS */
COMP( 1983, camplynx,  0,         0,       camplynx,  camplynx, 0,    camp48,   "Camputers",  "Lynx 48",   GAME_NOT_WORKING)
COMP( 1983, camply96,  camplynx,  0,       camplynx,  camplynx, 0,    camp96,   "Camputers",  "Lynx 96",   GAME_NOT_WORKING)
COMP( 1983, camply128, camplynx,  0,       camplynx,  camplynx, 0,    camp128,  "Camputers",  "Lynx 128",  GAME_NOT_WORKING)
