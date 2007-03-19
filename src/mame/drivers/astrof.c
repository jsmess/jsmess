/*
    Driver For DECO   ASTRO FIGHTER/TOMAHAWK 777

    Initial Version

    Lee Taylor 28/11/1997

*** Astro Battle added by HIGHWAYMAN with help from Reip.

2 sets, 1 may be a bad set, or they may simply be different - dont know yet.
protection involves the eprom datalines being routed through an 8bit x 256byte prom.
it *may* have a more complex palette, it needs to be investigated when i get more time.


    Astro Fighter Sets:

    The differences are minor. From newest to oldest:

    Main Set: 16Kbit ROMs
              Green/Hollow empty fuel bar.
              60 points for every bomb destroyed.

    Set 2:    8Kbit ROMs
              Blue/Solid empty fuel bar.
              60 points for every bomb destroyed.

    Set 3:    8Kbit ROMs
              Blue/Solid empty fuel bar.
              300 points for every seven bombs destroyed.


To Do!!
       Figure out the correct vblank interval. The one I'm using seems to be
       ok for Astro Fighter, but the submarine in Tomahawk flickers.
       Maybe the video rate should 57FPS as the Burger Time games?

       Rotation Support

Also....
        I know there must be at least one other rom set for Astro Fighter
        I have played one that stoped between waves to show the next enemy
*/

#include "driver.h"
#include "sound/samples.h"
#include "includes/astrof.h"

static int abattle_count;

static READ8_HANDLER( shoot_r )
{
	/* not really sure about this */
	return mame_rand(Machine) & 8;
}
static READ8_HANDLER( abattle_coin_prot_r )
{
	if(abattle_count < 0x100)
	{
		abattle_count++;
		return 7;
	}
	else
	{
		abattle_count = 0;
		return 0;
	}
}

static READ8_HANDLER( afire_coin_prot_r )
{
	if(!abattle_count)
	{
		abattle_count++;
		return 7;
	}
	else
	{
		abattle_count = 0;
		return 0;
	}
}

static ADDRESS_MAP_START( astrof_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_RAM AM_MIRROR(0x3c00)
	AM_RANGE(0x4000, 0x5fff) AM_RAM AM_WRITE(astrof_videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x8003, 0x8003) AM_WRITE(MWA8_RAM) AM_BASE(&astrof_color)
	AM_RANGE(0x8004, 0x8004) AM_WRITE(astrof_video_control1_w)
	AM_RANGE(0x8005, 0x8005) AM_WRITE(astrof_video_control2_w)
	AM_RANGE(0x8006, 0x8006) AM_WRITE(astrof_sample1_w)
	AM_RANGE(0x8007, 0x8007) AM_WRITE(astrof_sample2_w)
	AM_RANGE(0xa000, 0xa000) AM_READ(input_port_0_r)
	AM_RANGE(0xa001, 0xa001) AM_READ(input_port_1_r)	/* IN1 */
	AM_RANGE(0xd000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( tomahawk_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_RAM
	AM_RANGE(0x4000, 0x5fff) AM_RAM AM_WRITE(tomahawk_videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x8003, 0x8003) AM_WRITE(MWA8_RAM) AM_BASE(&astrof_color)
	AM_RANGE(0x8004, 0x8004) AM_WRITE(astrof_video_control1_w)
	AM_RANGE(0x8005, 0x8005) AM_WRITE(tomahawk_video_control2_w)
	AM_RANGE(0x8006, 0x8006) AM_WRITE(MWA8_NOP)                        // Sound triggers
	AM_RANGE(0x8007, 0x8007) AM_WRITE(MWA8_RAM) AM_BASE(&tomahawk_protection)
	AM_RANGE(0xa000, 0xa000) AM_READ(input_port_0_r)
	AM_RANGE(0xa001, 0xa001) AM_READ(input_port_1_r)	/* IN1 */
	AM_RANGE(0xa003, 0xa003) AM_READ(tomahawk_protection_r)
	AM_RANGE(0xd000, 0xffff) AM_ROM
ADDRESS_MAP_END



/***************************************************************************

  These games don't have VBlank interrupts.
  Interrupts are still used by the game: but they are related to coin
  slots.

***************************************************************************/
static INTERRUPT_GEN( astrof_interrupt )
{
	if (readinputportbytag("FAKE") & 1)	/* Coin */
		cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
}

static MACHINE_RESET( abattle )
{
	abattle_count = 0;
}


INPUT_PORTS_START( astrof )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
/* Player 1 Controls */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
/* Player 2 Controls */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )

	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
/* 0x0c gives 2 Coins/1 Credit */

	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "3000" )
	PORT_DIPSETTING(    0x10, "5000" )
	PORT_DIPSETTING(    0x20, "7000" )
	PORT_DIPSETTING(    0x30, DEF_STR( None ) )

	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Hard ) )

	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START_TAG("FAKE")
	/* The coin slots are not memory mapped. Coin insertion causes a NMI. */
	/* This fake input port is used by the interrupt */
	/* handler to be notified of coin insertions. We use IMPULSE to */
	/* trigger exactly one interrupt, without having to check when the */
	/* user releases the key. */
	/* The cabinet selector is not memory mapped, but just disables the */
	/* screen flip logic */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
INPUT_PORTS_END

INPUT_PORTS_START( abattle )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
/* Player 1 Controls */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
/* Player 2 Controls */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_COCKTAIL

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )

	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
/* 0x0c gives 2 Coins/1 Credit */

	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "5000" )
	PORT_DIPSETTING(    0x10, "7000" )
	PORT_DIPSETTING(    0x20, "10000" )
	PORT_DIPSETTING(    0x30, DEF_STR( None ) )

	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Hard ) )

	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START_TAG("FAKE")
	/* The coin slots are not memory mapped. Coin insertion causes a NMI. */
	/* This fake input port is used by the interrupt */
	/* handler to be notified of coin insertions. We use IMPULSE to */
	/* trigger exactly one interrupt, without having to check when the */
	/* user releases the key. */
	/* The cabinet selector is not memory mapped, but just disables the */
	/* screen flip logic */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
INPUT_PORTS_END

INPUT_PORTS_START( tomahawk )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )

	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
/* 0x0c gives 2 Coins/1 Credit */

	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "5000" )
	PORT_DIPSETTING(    0x10, "7000" )
	PORT_DIPSETTING(    0x20, "10000" )
	PORT_DIPSETTING(    0x30, DEF_STR( None ) )

	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Difficulty ) )  /* Only on Tomahawk 777 */
	PORT_DIPSETTING(    0x00, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Hard ) )

	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START_TAG("FAKE")
	/* The coin slots are not memory mapped. Coin insertion causes a NMI. */
	/* This fake input port is used by the interrupt */
	/* handler to be notified of coin insertions. We use IMPULSE to */
	/* trigger exactly one interrupt, without having to check when the */
	/* user releases the key. */
	/* The cabinet selector is not memory mapped, but just disables the */
	/* screen flip logic */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
INPUT_PORTS_END


static MACHINE_DRIVER_START( astrof )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6502, 10595000/16)	/* 0.66 MHz */
	MDRV_CPU_PROGRAM_MAP(astrof_map,0)
	MDRV_CPU_VBLANK_INT(astrof_interrupt,1)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(TIME_IN_USEC(3400))

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_SCREEN_VISIBLE_AREA(8, 256-1-8, 8, 256-1-8)
	MDRV_PALETTE_LENGTH(16)

	MDRV_PALETTE_INIT(astrof)
	MDRV_VIDEO_START(astrof)
	MDRV_VIDEO_UPDATE(astrof)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD_TAG("samples", SAMPLES, 0)
	MDRV_SOUND_CONFIG(astrof_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( abattle )
	MDRV_IMPORT_FROM(astrof)
	MDRV_MACHINE_RESET(abattle)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( tomahawk )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(astrof)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(tomahawk_map,0)

	/* video hardware */
	MDRV_PALETTE_LENGTH(32)

	/* sound hardware */
	MDRV_SOUND_REPLACE("samples", SAMPLES, 0)
	MDRV_SOUND_CONFIG(tomahawk_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( astrof )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "afii.6",       0xd000, 0x0800, CRC(d6cd13a4) SHA1(359b00b02f4256f1138c8526214c6a34d2e5b47a) )
	ROM_LOAD( "afii.5",       0xd800, 0x0800, CRC(6fd3c4df) SHA1(73aad03e2588ac9f249d5751eb4a7c7cd12fd3b9) )
	ROM_LOAD( "afii.4",       0xe000, 0x0800, CRC(9612dae3) SHA1(8ee1797c212e06c381972b7b555f240ff317d75d) )
	ROM_LOAD( "afii.3",       0xe800, 0x0800, CRC(5a0fef42) SHA1(92a575abdf17bbb5ed6bc67479049523a985aa75) )
	ROM_LOAD( "afii.2",       0xf000, 0x0800, CRC(69f8a4fc) SHA1(9f9a935f19187640018009ade92f8993912ef6c2) )
	ROM_LOAD( "afii.1",       0xf800, 0x0800, CRC(322c09d2) SHA1(89723e3d998ff9cb9b174bca4b072b412b290c04) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "astrf.clr",    0x0000, 0x0020, CRC(61329fd1) SHA1(15782d8757d4dda5a8b97815e94c90218f0e08dd) )
ROM_END

ROM_START( astrof2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "kei2",         0xd000, 0x0400, CRC(9f0bd355) SHA1(45db9229dcd8bbd366ff13c683625c3d1c175598) )
	ROM_LOAD( "keii",         0xd400, 0x0400, CRC(71f229f0) SHA1(be426360567066df01fb428dc5cd2d6ef01a4cf7) )
	ROM_LOAD( "kei0",         0xd800, 0x0400, CRC(88114f7c) SHA1(e64ae3cac92d2a3c02edc8e81c88d5d275e89293) )
	ROM_LOAD( "af579.08",     0xdc00, 0x0400, CRC(9793c124) SHA1(ae0352ed13fa21a00181669e92f9e66c938e4843) )
	ROM_LOAD( "ke8",          0xe000, 0x0400, CRC(08e44b12) SHA1(0e156fff081ae74321597eca1a02920bfc464651) )
	ROM_LOAD( "ke7",          0xe400, 0x0400, CRC(8a42d62c) SHA1(f5c0043be113c88f87deee3a2acd7d778a569e4f) )
	ROM_LOAD( "ke6",          0xe800, 0x0400, CRC(3e9aa743) SHA1(5f473afee7a416bb6f4e658cf8e46f8362ae3bba) )
	ROM_LOAD( "ke5",          0xec00, 0x0400, CRC(712a4557) SHA1(66a19378782c3911b8740ca25451ce84e1096fd0) )
	ROM_LOAD( "ke4",          0xf000, 0x0400, CRC(ad06f306) SHA1(d6ab7cba97658a46a63846a203eb89d9fc367e4f) )
	ROM_LOAD( "ke3",          0xf400, 0x0400, CRC(680b91b4) SHA1(004fd0f6564c19277632adec42bcf1054d043e4a) )
	ROM_LOAD( "ke2",          0xf800, 0x0400, CRC(2c4cab1a) SHA1(3171764a17f2c5fda39f0b32ccce60bc107d306e) )
	ROM_LOAD( "af583.00",     0xfc00, 0x0400, CRC(f699dda3) SHA1(e595cb93df40f64f7521afa51a879d53e1d04126) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "astrf.clr",    0x0000, 0x0020, CRC(61329fd1) SHA1(15782d8757d4dda5a8b97815e94c90218f0e08dd) )
ROM_END

ROM_START( astrof3 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "kei2",         0xd000, 0x0400, CRC(9f0bd355) SHA1(45db9229dcd8bbd366ff13c683625c3d1c175598) )
	ROM_LOAD( "keii",         0xd400, 0x0400, CRC(71f229f0) SHA1(be426360567066df01fb428dc5cd2d6ef01a4cf7) )
	ROM_LOAD( "kei0",         0xd800, 0x0400, CRC(88114f7c) SHA1(e64ae3cac92d2a3c02edc8e81c88d5d275e89293) )
	ROM_LOAD( "ke9",          0xdc00, 0x0400, CRC(29cbaea6) SHA1(da29e8156218884195b16839be9ad1e98a8348ac) )
	ROM_LOAD( "ke8",          0xe000, 0x0400, CRC(08e44b12) SHA1(0e156fff081ae74321597eca1a02920bfc464651) )
	ROM_LOAD( "ke7",          0xe400, 0x0400, CRC(8a42d62c) SHA1(f5c0043be113c88f87deee3a2acd7d778a569e4f) )
	ROM_LOAD( "ke6",          0xe800, 0x0400, CRC(3e9aa743) SHA1(5f473afee7a416bb6f4e658cf8e46f8362ae3bba) )
	ROM_LOAD( "ke5",          0xec00, 0x0400, CRC(712a4557) SHA1(66a19378782c3911b8740ca25451ce84e1096fd0) )
	ROM_LOAD( "ke4",          0xf000, 0x0400, CRC(ad06f306) SHA1(d6ab7cba97658a46a63846a203eb89d9fc367e4f) )
	ROM_LOAD( "ke3",          0xf400, 0x0400, CRC(680b91b4) SHA1(004fd0f6564c19277632adec42bcf1054d043e4a) )
	ROM_LOAD( "ke2",          0xf800, 0x0400, CRC(2c4cab1a) SHA1(3171764a17f2c5fda39f0b32ccce60bc107d306e) )
	ROM_LOAD( "kei",          0xfc00, 0x0400, CRC(fce4718d) SHA1(3a313328609f6bef644a2d906d8ca74c5d52058b) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "astrf.clr",    0x0000, 0x0020, CRC(61329fd1) SHA1(15782d8757d4dda5a8b97815e94c90218f0e08dd) )
ROM_END

ROM_START( abattle )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "10405-b.bin",  0xd000, 0x0400, CRC(9ba57987) SHA1(becf89b7d474f86839f13f9be5502c91491e8584) )
	ROM_LOAD( "10405-a.bin",  0xd400, 0x0400, CRC(3fbbeeba) SHA1(1c9f519a0797f90524adf187b0761f150db0828d) )
	ROM_LOAD( "10405-9.bin",  0xd800, 0x0400, CRC(354cf432) SHA1(138956ea8064eba0dcd8b2f175d4981b689a2077) )
	ROM_LOAD( "10405-8.bin",  0xdc00, 0x0400, CRC(4cee0c8b) SHA1(98bfdda9d2d368db16d6e9090536b09d8337c0e5) )
	ROM_LOAD( "10405-4.bin",  0xe000, 0x0400, CRC(9cb477f3) SHA1(6866264aa8d0479cee237a00e4a919e3981144a5) )
	ROM_LOAD( "10405-6.bin",  0xe400, 0x0400, CRC(272de8f1) SHA1(e917b3b8bb96fedacd6d5cb3d1c30977818f2e85) )
	ROM_LOAD( "10405-5.bin",  0xe800, 0x0400, CRC(ff25acaa) SHA1(5cb360c556c9b36039ae05702e6900b82fe5676b) )
	ROM_LOAD( "10405-3.bin",  0xec00, 0x0400, CRC(6edf202d) SHA1(a4cab2f10a99e0a4b1c571168e17cbee1d18cf06) )
	ROM_LOAD( "10405-7.bin",  0xf000, 0x0400, CRC(02a35ad9) SHA1(d54afff13f8d5a6544dda49c766a147fa0172cfa) )
	ROM_LOAD( "10405-1.bin",  0xf400, 0x0400, CRC(c68f6657) SHA1(a38c24670fcbbf7844ca15f918efcb467bae7bef) )
	ROM_LOAD( "10405-2.bin",  0xf800, 0x0400, CRC(b206deda) SHA1(9ab52920c06ed6beb38bc7f97ffd00e8ad46c17d) )
	ROM_LOAD( "10405-0.bin",  0xfc00, 0x0400, CRC(c836a152) SHA1(418b64d50bb2f849b1e7177c7bf2fdd0cc99e079) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "8f-clr.bin",   0x0000, 0x0100, CRC(3bf3ccb0) SHA1(d61d19d38045f42a9adecf295e479fee239bed48) )

	ROM_REGION( 0x0100, REGION_USER1, 0 )	/* decryption table */
	ROM_LOAD( "2h-prot.bin",  0x0000, 0x0100, CRC(a6bdd18c) SHA1(438bfc543730afdb531204585f17a68ddc03ded0) )
ROM_END

ROM_START( abattle2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "10405-b.bin",  0xd000, 0x0400, CRC(9ba57987) SHA1(becf89b7d474f86839f13f9be5502c91491e8584) )
	ROM_LOAD( "10405-a.bin",  0xd400, 0x0400, CRC(3fbbeeba) SHA1(1c9f519a0797f90524adf187b0761f150db0828d) )
	ROM_LOAD( "10405-9.bin",  0xd800, 0x0400, CRC(354cf432) SHA1(138956ea8064eba0dcd8b2f175d4981b689a2077) )
	ROM_LOAD( "10405-8.bin",  0xdc00, 0x0400, CRC(4cee0c8b) SHA1(98bfdda9d2d368db16d6e9090536b09d8337c0e5) )
	ROM_LOAD( "sidam-4.bin",  0xe000, 0x0400, CRC(f6998053) SHA1(f1a868e68db1ca89c54ee179aa4c922ec49b686b) )
	ROM_LOAD( "10405-6.bin",  0xe400, 0x0400, CRC(272de8f1) SHA1(e917b3b8bb96fedacd6d5cb3d1c30977818f2e85) )
	ROM_LOAD( "sidam-5.bin",  0xe800, 0x0400, CRC(6ddd78ff) SHA1(2fdf3fd145446f174293818aa81463097227361e) )
	ROM_LOAD( "10405-3.bin",  0xec00, 0x0400, CRC(6edf202d) SHA1(a4cab2f10a99e0a4b1c571168e17cbee1d18cf06) )
	ROM_LOAD( "10405-7.bin",  0xf000, 0x0400, CRC(02a35ad9) SHA1(d54afff13f8d5a6544dda49c766a147fa0172cfa) )
	ROM_LOAD( "10405-1.bin",  0xf400, 0x0400, CRC(c68f6657) SHA1(a38c24670fcbbf7844ca15f918efcb467bae7bef) )
	ROM_LOAD( "10405-2.bin",  0xf800, 0x0400, CRC(b206deda) SHA1(9ab52920c06ed6beb38bc7f97ffd00e8ad46c17d) )
	ROM_LOAD( "10405-0.bin",  0xfc00, 0x0400, CRC(c836a152) SHA1(418b64d50bb2f849b1e7177c7bf2fdd0cc99e079) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "8f-clr.bin",   0x0000, 0x0100, CRC(3bf3ccb0) SHA1(d61d19d38045f42a9adecf295e479fee239bed48) )

	ROM_REGION( 0x0100, REGION_USER1, 0 )	/* decryption table */
	ROM_LOAD( "2h-prot.bin",  0x0000, 0x0100, CRC(a6bdd18c) SHA1(438bfc543730afdb531204585f17a68ddc03ded0) )
ROM_END

ROM_START( afire )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "b.bin",        0xd000, 0x0400, CRC(16ad2bcc) SHA1(e7f55d17ee18afbb045cd0fd8d3ffc0c8300130a) )
	ROM_LOAD( "a.bin",        0xd400, 0x0400, CRC(ce8b6e4f) SHA1(b85ab709d80324df5d2c4b0dbbc5e6aeb4003077) )
	ROM_LOAD( "9.bin",        0xd800, 0x0400, CRC(e0f45b07) SHA1(091e1ea4b3726888dc488bb01e0bd4e588eccae5) )
	ROM_LOAD( "8.bin",        0xdc00, 0x0400, CRC(85b96728) SHA1(dbbfbc085f19184d861c42a0307f95f9105a677b) )
	ROM_LOAD( "4.bin",        0xe000, 0x0400, CRC(271f90ad) SHA1(fe41a0f35d30d38fc21ac19982899d93cbd292f0) )
	ROM_LOAD( "6.bin",        0xe400, 0x0400, CRC(568efbfe) SHA1(ef39f0fc4c030fc7f688515415aedeb4c039b73a) )
	ROM_LOAD( "5.bin",        0xe800, 0x0400, CRC(1c0b298a) SHA1(61677f8f402679fcfbb9fb12f9dfde7b6e1cdd1c) )
	ROM_LOAD( "3.bin",        0xec00, 0x0400, CRC(2938c641) SHA1(c8655a8218818c12eca0f00a361412e4946f8b5c) )
	ROM_LOAD( "7.bin",        0xf000, 0x0400, CRC(912c8fe1) SHA1(1ae1eb13858d39200386f59c3381eef2699e4647) )
	ROM_LOAD( "1.bin",        0xf400, 0x0400, CRC(0ef045d8) SHA1(c41b284ccdf5da3a5e9b4732324b3d61440ce9db) )
	ROM_LOAD( "2.bin",        0xf800, 0x0400, CRC(d4ea2760) SHA1(57c9a4d21fbb28019fcd2f60c0424b3c9ae1055c) )
	ROM_LOAD( "0.bin",        0xfc00, 0x0400, CRC(fe695575) SHA1(b12587a4de624ab712ed6336bd2eb69b12bde563) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "astrf.clr",    0x0000, 0x0020, CRC(61329fd1) SHA1(15782d8757d4dda5a8b97815e94c90218f0e08dd) )
ROM_END

/* This is a newer revision of "Astro Combat" (most probably manufactured by Sidam),
   with correct spelling for FUEL and the main boss sporting "CB". */
ROM_START( acombat )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "b.bin",        0xd000, 0x0400, CRC(16ad2bcc) SHA1(e7f55d17ee18afbb045cd0fd8d3ffc0c8300130a) )
	ROM_LOAD( "a.bin",        0xd400, 0x0400, CRC(ce8b6e4f) SHA1(b85ab709d80324df5d2c4b0dbbc5e6aeb4003077) )
	ROM_LOAD( "9.bin",        0xd800, 0x0400, CRC(e0f45b07) SHA1(091e1ea4b3726888dc488bb01e0bd4e588eccae5) )
	ROM_LOAD( "8.bin",        0xdc00, 0x0400, CRC(85b96728) SHA1(dbbfbc085f19184d861c42a0307f95f9105a677b) )
	ROM_LOAD( "4.bin",        0xe000, 0x0400, CRC(271f90ad) SHA1(fe41a0f35d30d38fc21ac19982899d93cbd292f0) )
	ROM_LOAD( "6.bin",        0xe400, 0x0400, CRC(568efbfe) SHA1(ef39f0fc4c030fc7f688515415aedeb4c039b73a) )
	ROM_LOAD( "5.bin",        0xe800, 0x0400, CRC(1c0b298a) SHA1(61677f8f402679fcfbb9fb12f9dfde7b6e1cdd1c) )
	ROM_LOAD( "3.bin",        0xec00, 0x0400, CRC(2938c641) SHA1(c8655a8218818c12eca0f00a361412e4946f8b5c) )
	ROM_LOAD( "7.bin",        0xf000, 0x0400, CRC(912c8fe1) SHA1(1ae1eb13858d39200386f59c3381eef2699e4647) )
	ROM_LOAD( "1a",           0xf400, 0x0400, CRC(7193f999) SHA1(13ddeddb1f22cae973102203ab4917b1407b6401) )
	ROM_LOAD( "2a",           0xf800, 0x0400, CRC(3b6ccbbe) SHA1(f9cf023557ee769bcb92df808628a39630b258f2) )
	ROM_LOAD( "0a",           0xfc00, 0x0400, CRC(355da937) SHA1(e50f364372120926d062203bd476ff68ab3bb5cf) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "astrf.clr",    0x0000, 0x0020, CRC(61329fd1) SHA1(15782d8757d4dda5a8b97815e94c90218f0e08dd) )
ROM_END

/* It is on older revision of "Astro Combat" (most probably manufactured by Sidam),
   with incorrect spelling for fuel as FLUEL and the main boss sporting "PZ" */
ROM_START( acombato )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "b.bin",        0xd000, 0x0400, CRC(16ad2bcc) SHA1(e7f55d17ee18afbb045cd0fd8d3ffc0c8300130a) )
	ROM_LOAD( "a.bin",        0xd400, 0x0400, CRC(ce8b6e4f) SHA1(b85ab709d80324df5d2c4b0dbbc5e6aeb4003077) )
	ROM_LOAD( "9.bin",        0xd800, 0x0400, CRC(e0f45b07) SHA1(091e1ea4b3726888dc488bb01e0bd4e588eccae5) )
	ROM_LOAD( "8.bin",        0xdc00, 0x0400, CRC(85b96728) SHA1(dbbfbc085f19184d861c42a0307f95f9105a677b) )
	ROM_LOAD( "4.bin",        0xe000, 0x0400, CRC(271f90ad) SHA1(fe41a0f35d30d38fc21ac19982899d93cbd292f0) )
	ROM_LOAD( "6.bin",        0xe400, 0x0400, CRC(568efbfe) SHA1(ef39f0fc4c030fc7f688515415aedeb4c039b73a) )
	ROM_LOAD( "5.bin",        0xe800, 0x0400, CRC(1c0b298a) SHA1(61677f8f402679fcfbb9fb12f9dfde7b6e1cdd1c) )
	ROM_LOAD( "3.bin",        0xec00, 0x0400, CRC(2938c641) SHA1(c8655a8218818c12eca0f00a361412e4946f8b5c) )
	ROM_LOAD( "7.bin",        0xf000, 0x0400, CRC(912c8fe1) SHA1(1ae1eb13858d39200386f59c3381eef2699e4647) )
	ROM_LOAD( "1a",           0xf400, 0x0400, CRC(7193f999) SHA1(13ddeddb1f22cae973102203ab4917b1407b6401) )
	ROM_LOAD( "2a",           0xf800, 0x0400, CRC(3b6ccbbe) SHA1(f9cf023557ee769bcb92df808628a39630b258f2) )
	ROM_LOAD( "0",            0xfc00, 0x0400, CRC(c4f3eaad) SHA1(51f03f35c45ac00a7f38fd97386be92bcb562ca2) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "astrf.clr",    0x0000, 0x0020, CRC(61329fd1) SHA1(15782d8757d4dda5a8b97815e94c90218f0e08dd) )
ROM_END

ROM_START( sstarbtl )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "b.bin",        0xd000, 0x0400, CRC(16ad2bcc) SHA1(e7f55d17ee18afbb045cd0fd8d3ffc0c8300130a) )
	ROM_LOAD( "a.rom",        0xd400, 0x0400, CRC(5a75891d) SHA1(71cde93a219ec3735cead7ec89f77bc8b11bfc64) )
	ROM_LOAD( "9.rom",        0xd800, 0x0400, CRC(de3f8063) SHA1(77b89ef0b356316e463d7575c037069d0c14a850) )
	ROM_LOAD( "8.bin",        0xdc00, 0x0400, CRC(85b96728) SHA1(dbbfbc085f19184d861c42a0307f95f9105a677b) )
	ROM_LOAD( "4.bin",        0xe000, 0x0400, CRC(271f90ad) SHA1(fe41a0f35d30d38fc21ac19982899d93cbd292f0) )
	ROM_LOAD( "6.bin",        0xe400, 0x0400, CRC(568efbfe) SHA1(ef39f0fc4c030fc7f688515415aedeb4c039b73a) )
	ROM_LOAD( "5.rom",        0xe800, 0x0400, CRC(4202b7f8) SHA1(c9d153323bdc0c99f4987895d1fba1ebf3bc7f2d) )
	ROM_LOAD( "3.bin",        0xec00, 0x0400, CRC(2938c641) SHA1(c8655a8218818c12eca0f00a361412e4946f8b5c) )
	ROM_LOAD( "7.rom",        0xf000, 0x0400, CRC(76990bf1) SHA1(e0d8e2015401d1190fc8cd9dac3e20a4a54cdc02) )
	ROM_LOAD( "1.rom",        0xf400, 0x0400, CRC(c72dd542) SHA1(08b6aab4c53dac77c6e0af21bae3fed4facef7ef) )
	ROM_LOAD( "2a",           0xf800, 0x0400, CRC(3b6ccbbe) SHA1(f9cf023557ee769bcb92df808628a39630b258f2) )
	ROM_LOAD( "0.rom",        0xfc00, 0x0400, CRC(b31ed075) SHA1(faaa21c9b62deb36dcc4805b38ef55db63fb854a) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "astrf.clr",    0x0000, 0x0020, CRC(61329fd1) SHA1(15782d8757d4dda5a8b97815e94c90218f0e08dd) )
ROM_END

ROM_START( tomahawk )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "l8-1",         0xdc00, 0x0400, CRC(7c911661) SHA1(3fc75bb0e6a89d41d76f82eeb0fde7d33809dddf) )
	ROM_LOAD( "l7-1",         0xe000, 0x0400, CRC(adeffb69) SHA1(8ff7ada883825a8b56cae3368ce377228922ab1d) )
	ROM_LOAD( "l6-1",         0xe400, 0x0400, CRC(9116e59d) SHA1(22a6d410fff8534b3aa7eb2ed0a8c096c890acf5) )
	ROM_LOAD( "l5-1",         0xe800, 0x0400, CRC(01e4c7c4) SHA1(fbb37539d08284bae6454cd57650e8507a88acdb) )
	ROM_LOAD( "l4-1",         0xec00, 0x0400, CRC(d9f69cb0) SHA1(d6a2dcaf867f33068e7d7ad7a3faf62a360456a6) )
	ROM_LOAD( "l3-1",         0xf000, 0x0400, CRC(7ce7183f) SHA1(949c7b696fe215b68af450299c91e90fb27b0141) )
	ROM_LOAD( "l2-1",         0xf400, 0x0400, CRC(43fea29d) SHA1(6890311440089a16d2e4d502855670723df41e16) )
	ROM_LOAD( "l1-1",         0xf800, 0x0400, CRC(f2096ba9) SHA1(566f6d49cdacb5e39c40eb3773640270ef5f272c) )
	ROM_LOAD( "l0-1",         0xfc00, 0x0400, CRC(42edbc28) SHA1(bab1fe8591509783dfdd4f53b9159263b9201970) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "t777.clr",     0x0000, 0x0020, CRC(d6a528fd) SHA1(5fc08252a2d7c5405f601efbfb7d84bec328d733) )
ROM_END

ROM_START( tomahaw5 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "thawk.l8",     0xdc00, 0x0400, CRC(b01dab4b) SHA1(d8b4266359a3b18d649f539fad8dce4d73cec412) )
	ROM_LOAD( "thawk.l7",     0xe000, 0x0400, CRC(3a6549e8) SHA1(2ba622d78596c72998784432cf8fbbe733c50ce5) )
	ROM_LOAD( "thawk.l6",     0xe400, 0x0400, CRC(863e47f7) SHA1(e8e48560c217025796be20f51c50ec276dba3eb5) )
	ROM_LOAD( "thawk.l5",     0xe800, 0x0400, CRC(de0183bc) SHA1(7cb8d013750c8fb423ab2759443f805bc8440d53) )
	ROM_LOAD( "thawk.l4",     0xec00, 0x0400, CRC(11e9c7ea) SHA1(9dbdce7d518891aa8b08dca50d4e8aaec89cc038) )
	ROM_LOAD( "thawk.l3",     0xf000, 0x0400, CRC(ec44d388) SHA1(7dda9db5ce2271988e9316dacf4b6ccbb72f50c9) )
	ROM_LOAD( "thawk.l2",     0xf400, 0x0400, CRC(dc0a0f54) SHA1(8e5c94706768ffafaba96382f2e757ecb825799f) )
	ROM_LOAD( "thawk.l1",     0xf800, 0x0400, CRC(1d9dab9c) SHA1(54dd91164db0489bd5984f10d4f0254184302ae4) )
	ROM_LOAD( "thawk.l0",     0xfc00, 0x0400, CRC(d21a1eba) SHA1(ce9ad7a1a3b069ef4eb8b5ce569e52c488a224f2) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "t777.clr",     0x0000, 0x0020, CRC(d6a528fd) SHA1(5fc08252a2d7c5405f601efbfb7d84bec328d733) )
ROM_END

static DRIVER_INIT( abattle )
{
	/* use the protection prom to decrypt the roms */
	UINT8 *rom = memory_region(REGION_CPU1);
	UINT8 *prom = memory_region(REGION_USER1);
	int i;

	for(i = 0xd000; i < 0x10000; i++)
	{
		rom[i] = prom[rom[i]];
	}

	/* set up protection handlers */
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa003, 0xa003, 0, 0, shoot_r);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa004, 0xa004, 0, 0, abattle_coin_prot_r);
}

static DRIVER_INIT( afire )
{
	UINT8 *rom = memory_region(REGION_CPU1);
	int i;

	for(i = 0xd000; i < 0x10000; i++)
		rom[i] = rom[i] ^ 0xff;

	/* set up protection handlers */
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa003, 0xa003, 0, 0, shoot_r);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa004, 0xa004, 0, 0, afire_coin_prot_r);
}

static DRIVER_INIT( sstarbtl )
{
	UINT8 *rom = memory_region(REGION_CPU1);
	int i;

	for(i = 0xd000; i < 0x10000; i++)
		rom[i] = rom[i] ^ 0xff;

	/* set up protection handlers */
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa003, 0xa003, 0, 0, shoot_r);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa004, 0xa004, 0, 0, abattle_coin_prot_r);
}

GAME( 1979, astrof,   0,        astrof,   astrof,   0,		 ROT90, "Data East",   "Astro Fighter (set 1)", 0 )
GAME( 1979, astrof2,  astrof,   astrof,   astrof,   0,		 ROT90, "Data East",   "Astro Fighter (set 2)", 0 )
GAME( 1979, astrof3,  astrof,   astrof,   astrof,   0,		 ROT90, "Data East",   "Astro Fighter (set 3)", 0 )
GAME( 1979, abattle,  astrof,	abattle,  abattle,  abattle, ROT90, "Sidam",	   "Astro Battle (set 1)", 0 )
GAME( 1979, abattle2, astrof,	abattle,  abattle,  abattle, ROT90, "Sidam",	   "Astro Battle (set 2)", 0 )
GAME( 1979, afire,    astrof,   abattle,  abattle,  afire,   ROT90, "Rene Pierre", "Astro Fire", 0 )
GAME( 1979, acombat,  astrof,   abattle,  abattle,  afire,   ROT90, "bootleg",     "Astro Combat (newer, CB)", 0 )
GAME( 1979, acombato, astrof,   abattle,  abattle,  afire,   ROT90, "bootleg",     "Astro Combat (older, PZ)", 0 )
GAME( 1979, sstarbtl, astrof,   abattle,  abattle,  sstarbtl,ROT90, "bootleg",     "Super Star Battle", 0 )
GAME( 1980, tomahawk, 0,        tomahawk, tomahawk, 0,       ROT90, "Data East",   "Tomahawk 777 (Revision 1)", GAME_NO_SOUND )
GAME( 1980, tomahaw5, tomahawk, tomahawk, tomahawk, 0,       ROT90, "Data East",   "Tomahawk 777 (Revision 5)", GAME_NO_SOUND )
