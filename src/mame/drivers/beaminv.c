/***************************************************************************

    Tekunon Kougyou Beam Invader hardware

    driver by Zsolt Vasvari

    Games supported:
        * Beam Invader (2 sets)

    Known issues:
        * Port 0 might be a analog port select


Stephh's notes (based on the games Z80 code and some tests) :

  - The min/max values for the controllers might not be accurate, but I have no infos at all.
    So I put the min/max values from what I see in the Z80 code (see below).
  - Is the visible area correct ? The invaders and the ship don't reach the left part of the screen !

1) 'beaminv'

  - Routine to handle the analog inputs at 0x0521.
    Contents from 0x3400 (IN2) is compared with contents from 0x1d25 (value in RAM).
    Contents from 0x3400 is not limited but contents from 0x1d25 range is the following :
      . player 1 : min = 0x1c - max = 0xd1
      . player 2 : min = 0x2d - max = 0xe2
    This is why sometimes the ship moves even if you don't do anything !
  - Screen flipping is internally handled (no specific write to memory or out to a port).
  - I can't tell if controller select is handled with a out to port 0 but I haven't found
    any other write to memory or out to another port.
  - Player's turn is handled by multiple reads from 0x1839 in RAM :
      . 1 player  game : [0x1839] = 0x00
      . 2 players game : [0x1839] = 0xaa (player 1) or 0x55 (player 2)
  - Credits are stored at address 0x1837 (BCD coded, range 0x00-0x99)

2) 'beaminva'

  - Routine to handle the analog inputs at 0x04bd.
    Contents from 0x3400 (IN2) is compared with contents from 0x1d05 (value in RAM).
    Contents from 0x3400 is limited to range 0x35-0x95 but contents from 0x1d05 range is the following :
      . player 1 : min = 0x1c - max = 0xd1
      . player 2 : min = 0x2d - max = 0xe2
    This is why sometimes the ship moves even if you don't do anything !
  - Screen flipping is internally handled (no specific write to memory or out to a port).
  - I can't tell if controller select is handled with a out to port 0 but I haven't found
    any other write to memory or out to another port.
  - Player's turn is handled by multiple reads from 0x1838 in RAM :
      . 1 player  game : [0x1838] = 0x00
      . 2 players game : [0x1838] = 0xaa (player 1) or 0x55 (player 2)
  - Credits are stored at address 0x1836 (BCD coded, range 0x00-0x99)

***************************************************************************/

#include "driver.h"
#include "beaminv.h"


/****************************************************************
 *
 *  Special port handler - doesn't warrant its own 'machine file
 *
 ****************************************************************/

int beaminv_controller_select;

static WRITE8_HANDLER( beaminv_controller_select_w )
{
	/* 1 player game : 0x01 - 2 players game : 0x01 (player 1) or 0x02 (player 2) */
	beaminv_controller_select = data;
}


static READ8_HANDLER( beaminv_input_port_2_r )
{
	return (readinputport(2 + (beaminv_controller_select - 1)));
}

static READ8_HANDLER( beaminv_input_port_3_r )
{
	return (readinputportbytag("IN3") & 0xfe) | ((cpu_getscanline() >> 7) & 0x01);
}


/*************************************
 *
 *  Memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x17ff) AM_READ(MRA8_ROM)
	AM_RANGE(0x1800, 0x1fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x2400, 0x2400) AM_READ(input_port_0_r)
	AM_RANGE(0x2800, 0x28ff) AM_READ(input_port_1_r)
	AM_RANGE(0x3400, 0x3400) AM_READ(beaminv_input_port_2_r)
	AM_RANGE(0x3800, 0x3800) AM_READ(beaminv_input_port_3_r)
	AM_RANGE(0x4000, 0x5fff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x17ff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x1800, 0x1fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x4000, 0x5fff) AM_WRITE(beaminv_videoram_w) AM_BASE(&videoram)
ADDRESS_MAP_END


/*************************************
 *
 *  Port handlers
 *
 *************************************/

static ADDRESS_MAP_START( writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(beaminv_controller_select_w) /* to be confirmed */
ADDRESS_MAP_END


/*************************************
 *
 *  Port definitions
 *
 *************************************/

INPUT_PORTS_START( beaminv )
	PORT_START                /* IN0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Bonus_Life ) )       /* value at 0x183b in RAM - code at 0x01c8 */
	PORT_DIPSETTING(    0x00, "1000" )
	PORT_DIPSETTING(    0x04, "2000" )
	PORT_DIPSETTING(    0x08, "3000" )
	PORT_DIPSETTING(    0x0c, "4000" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x00, "Faster Bombs At" )           /* table at 0x1777 in ROM - code at 0x16ac */
	PORT_DIPSETTING(    0x00, "49 Enemies" )
	PORT_DIPSETTING(    0x20, "39 Enemies" )
	PORT_DIPSETTING(    0x40, "29 Enemies" )
	PORT_DIPSETTING(    0x60, "Never" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START                /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START                /* IN2 for player 1 */
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(20) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_PLAYER(1)
	PORT_START                /* FAKE IN2 for player 2 */
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(20) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_PLAYER(2)

	PORT_START_TAG("IN3")     /* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SPECIAL )  /* should be V128, using VBLANK slows game down */
	PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( beaminva )
	PORT_START                /* IN0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Bonus_Life ) )       /* value at 0x183a in RAM - code at 0x01bf */
	PORT_DIPSETTING(    0x00, "1500" )
	PORT_DIPSETTING(    0x04, "2000" )
	PORT_DIPSETTING(    0x08, "2500" )
	PORT_DIPSETTING(    0x0c, "3000" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x00, "Faster Bombs At" )           /* table at 0x1777 in ROM - code at 0x166d */
	PORT_DIPSETTING(    0x00, "44 Enemies" )
	PORT_DIPSETTING(    0x20, "39 Enemies" )
	PORT_DIPSETTING(    0x40, "34 Enemies" )
	PORT_DIPSETTING(    0x40, "29 Enemies" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START                /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START                /* IN2 for player 1 */
	PORT_BIT( 0xff, 0x65, IPT_PADDLE ) PORT_MINMAX(0x35,0x95) PORT_SENSITIVITY(20) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_PLAYER(1)
	PORT_START                /* FAKE IN2 for player 2 */
	PORT_BIT( 0xff, 0x65, IPT_PADDLE ) PORT_MINMAX(0x35,0x95) PORT_SENSITIVITY(20) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_PLAYER(2)

	PORT_START_TAG("IN3")     /* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SPECIAL )  /* should be V128, using VBLANK slows game down */
	PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


/*************************************
 *
 *  Machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( beaminv )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 2000000)	/* 2 MHz ? */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_IO_MAP(0,writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,2)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(TIME_IN_USEC(0))

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_SCREEN_VISIBLE_AREA(16, 223, 16, 247)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(generic_bitmapped)
MACHINE_DRIVER_END


/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( beaminv )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "0a", 0x0000, 0x0400, CRC(17503086) SHA1(18c789216e5c4330dba3eeb24919dae636bf803d) )
	ROM_LOAD( "1a", 0x0400, 0x0400, CRC(aa9e1666) SHA1(050e2bd169f1502f49b7e6f5f2df9dac0d8107aa) )
	ROM_LOAD( "2a", 0x0800, 0x0400, CRC(ebaa2fc8) SHA1(b4ff1e1bdfe9efdc08873bba2f0a30d24678f9d8) )
	ROM_LOAD( "3a", 0x0c00, 0x0400, CRC(4f62c2e6) SHA1(4bd7d5e4f18d250003c7d771f1cdab08d699a765) )
	ROM_LOAD( "4a", 0x1000, 0x0400, CRC(3eebf757) SHA1(990eebda80ec52b7e3a36912c6e9230cd97f9f25) )
	ROM_LOAD( "5a", 0x1400, 0x0400, CRC(ec08bc1f) SHA1(e1df6704298e470a77158740c275fdca105e8f69) )
ROM_END

ROM_START( beaminva )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "rom_0", 0x0000, 0x0400, CRC(67e100dd) SHA1(5f58e2ed3da14c48f7c382ee6091a59caf8e0609) )
	ROM_LOAD( "rom_1", 0x0400, 0x0400, CRC(442bbe98) SHA1(0e0382d4f6491629449759747019bd453a458b66) )
	ROM_LOAD( "rom_2", 0x0800, 0x0400, CRC(5d5d2f68) SHA1(e363f9445bbba1492188efe1830cae96f6078878) )
	ROM_LOAD( "rom_3", 0x0c00, 0x0400, CRC(527906b8) SHA1(9bda7da653db64246597ca386adab4cbab319189) )
	ROM_LOAD( "rom_4", 0x1000, 0x0400, CRC(920bb3f0) SHA1(3b9897d31c551e0b9193f775a6be65376b4a8c34) )
	ROM_LOAD( "rom_5", 0x1400, 0x0400, CRC(3f6980e4) SHA1(cb73cbc474677e6e302cb3842f32923ef2cdc98d) )
ROM_END


/*************************************
 *
 *  Game drivers
 *
 *************************************/

GAME( 19??, beaminv,  0      ,  beaminv,  beaminv,  0, ROT0, "Tekunon Kougyou", "Beam Invader (set 1)", GAME_NO_SOUND)
GAME( 1979, beaminva, beaminv,  beaminv,  beaminva, 0, ROT0, "Tekunon Kougyou", "Beam Invader (set 2)", GAME_NO_SOUND) // what's the real title ?
