/***************************************************************************

    Atari Avalanche hardware

    driver by Mike Balfour

    Games supported:
        * Avalanche

    Known issues:
        * none at this time

****************************************************************************

    Memory Map:
                    0000-1FFF               RAM
                    2000-2FFF       R       INPUTS
                    3000-3FFF       W       WATCHDOG
                    4000-4FFF       W       OUTPUTS
                    5000-5FFF       W       SOUND LEVEL
                    6000-7FFF       R       PROGRAM ROM
                    8000-DFFF               UNUSED
                    E000-FFFF               PROGRAM ROM (Remapped)

    If you have any questions about how this driver works, don't hesitate to
    ask.  - Mike Balfour (mab22@po.cwru.edu)

***************************************************************************/

#include "driver.h"
#include "sound/discrete.h"
#include "avalnche.h"

#include "avalnche.lh"



/*************************************
 *
 *  Palette generation
 *
 *************************************/

static PALETTE_INIT( avalnche )
{
	/* 2 colors in the palette: black & white */
	palette_set_color(machine,0,0x00,0x00,0x00);
	palette_set_color(machine,1,0xff,0xff,0xff);
}



/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(15) )
	AM_RANGE(0x0000, 0x1fff) AM_READWRITE(MRA8_RAM, avalnche_videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size) /* RAM SEL */
	AM_RANGE(0x2000, 0x2fff) AM_READ(avalnche_input_r) /* INSEL */
	AM_RANGE(0x3000, 0x3fff) AM_WRITE(MWA8_NOP) /* WATCHDOG */
	AM_RANGE(0x4000, 0x4fff) AM_WRITE(avalnche_output_w) /* OUTSEL */
	AM_RANGE(0x5000, 0x5fff) AM_WRITE(avalnche_noise_amplitude_w) /* SOUNDLVL */
	AM_RANGE(0x6000, 0x7fff) AM_ROM /* ROM1-ROM2 */
ADDRESS_MAP_END



/*************************************
 *
 *  Port definitions
 *
 *************************************/

INPUT_PORTS_START( avalnche )
	PORT_START_TAG("IN0")
	PORT_BIT (0x03, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Spare */
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPSETTING(    0x30, DEF_STR( German ) )
	PORT_DIPSETTING(    0x20, DEF_STR( French ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Spanish ) )
	PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START_TAG("IN1")
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_DIPNAME( 0x04, 0x04, "Allow Extended Play" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x00, "Lives/Extended Play" )
	PORT_DIPSETTING(    0x00, "3/450 points" )
	PORT_DIPSETTING(    0x08, "5/750 points" )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* SLAM */
	PORT_SERVICE( 0x20, IP_ACTIVE_HIGH)
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 )	/* Serve */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )	/* VBLANK */

	PORT_START_TAG("IN2")
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(0x40,0xb7) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_CENTERDELTA(0)
INPUT_PORTS_END


INPUT_PORTS_START( cascade )
	PORT_START_TAG("IN0")
	PORT_BIT (0x03, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Spare */
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPSETTING(    0x30, DEF_STR( German ) )
	PORT_DIPSETTING(    0x20, DEF_STR( French ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Italian ) )
	PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START_TAG("IN1")
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_DIPNAME( 0x04, 0x04, "Allow Extended Play" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x00, "Extended Play" )
	PORT_DIPSETTING(    0x00, "1500 points" )
	PORT_DIPSETTING(    0x08, "2500 points" )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* SLAM */
	PORT_SERVICE( 0x20, IP_ACTIVE_HIGH)
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 )	/* Serve */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )	/* VBLANK */

	PORT_START_TAG("IN2")
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(0x40,0xb7) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_CENTERDELTA(0)
INPUT_PORTS_END


/*************************************
 *
 *  Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( avalnche )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502,12096000/16)	   /* clock input is the "2H" signal divided by two */
	MDRV_CPU_PROGRAM_MAP(main_map,0)
	MDRV_CPU_VBLANK_INT(avalnche_interrupt,8)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 32*8-1, 2*8, 32*8-1)
	MDRV_PALETTE_LENGTH(2)

	MDRV_PALETTE_INIT(avalnche)
	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(avalnche)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(DISCRETE, 0)
	MDRV_SOUND_CONFIG(avalnche_discrete_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( avalnche )
	ROM_REGION( 0x8000, REGION_CPU1, 0 ) /* 32k for code */
	ROM_LOAD_NIB_HIGH( "30612.d2",     	0x6800, 0x0800, CRC(3f975171) SHA1(afe680865da97824f1ebade4c7a2ba5d7ee2cbab) )
	ROM_LOAD_NIB_LOW ( "30615.d3",     	0x6800, 0x0800, CRC(3e1a86b4) SHA1(3ff4cffea5b7a32231c0996473158f24c3bbe107) )
	ROM_LOAD_NIB_HIGH( "30613.e2",     	0x7000, 0x0800, CRC(47a224d3) SHA1(9feb7444a2e5a3d90a4fe78ae5d23c3a5039bfaa) )
	ROM_LOAD_NIB_LOW ( "30616.e3",     	0x7000, 0x0800, CRC(f620f0f8) SHA1(7802b399b3469fc840796c3145b5f63781090956) )
	ROM_LOAD_NIB_HIGH( "30611.c2",     	0x7800, 0x0800, CRC(0ad07f85) SHA1(5a1a873b14e63dbb69ee3686ba53f7ca831fe9d0) )
	ROM_LOAD_NIB_LOW ( "30614.c3",     	0x7800, 0x0800, CRC(a12d5d64) SHA1(1647d7416bf9266d07f066d3797bda943e004d24) )
ROM_END

ROM_START( cascade )
	ROM_REGION( 0x8000, REGION_CPU1, 0 ) /* 32k for code */
	ROM_LOAD( "10005.1a",     	0x6800, 0x0400, CRC(54774594) SHA1(03e013b563675fb8a30bd69836466a353db9bfc7) )
	ROM_LOAD( "10005.1b",     	0x6c00, 0x0400, CRC(fd9575ad) SHA1(ed0a1343d3c0456d458561256d5ee966b6c213f4) )
	ROM_LOAD( "10005.2a",     	0x7000, 0x0400, CRC(12857c75) SHA1(e42fdee70bd19d6f60e88f106a49dbbd68c591cd) )
	ROM_LOAD( "10005.2b",     	0x7400, 0x0400, CRC(26361698) SHA1(587cc6f0dc068a74aac41c2cb3336d70d2dd91e5) )
	ROM_LOAD( "10005.3a",     	0x7800, 0x0400, CRC(d1f422ff) SHA1(65ecbf0a22ba340d6a1768ed029030bac9c19e0f) )
	ROM_LOAD( "10005.3b",     	0x7c00, 0x0400, CRC(bb243d96) SHA1(3a387a8c50cd9b0db37d12b94dc9e260892dbf21) )
ROM_END



/*************************************
 *
 *  Game drivers
 *
 *************************************/

GAMEL( 1978, avalnche, 0,        avalnche, avalnche, 0, ROT0, "Atari", "Avalanche", 0, layout_avalnche )
GAMEL( 1978, cascade,  avalnche, avalnche, cascade,  0, ROT0, "Sidam", "Cascade", 0, layout_avalnche )
