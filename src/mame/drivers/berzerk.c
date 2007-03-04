/***************************************************************************

 Berzerk Driver by Zsolt Vasvari
 Sound Driver by Alex Judd

***************************************************************************/

#include "driver.h"
#include "includes/berzerk.h"
#include "includes/exidy.h"
#include "sound/s14001a.h"

static ADDRESS_MAP_START( berzerk_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_ROM
	AM_RANGE(0x0800, 0x09ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x1000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0x5fff) AM_READWRITE(MRA8_RAM,berzerk_videoram_w) AM_BASE(&videoram) AM_SHARE(1)
	AM_RANGE(0x6000, 0x7fff) AM_READWRITE(MRA8_RAM,berzerk_magicram_w) AM_SHARE(1)
	AM_RANGE(0x8000, 0x87ff) AM_READWRITE(MRA8_RAM,berzerk_colorram_w) AM_BASE(&colorram)
ADDRESS_MAP_END


static ADDRESS_MAP_START( frenzy_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0x5fff) AM_READWRITE(MRA8_RAM,berzerk_videoram_w) AM_BASE(&videoram) AM_SHARE(1)
	AM_RANGE(0x6000, 0x7fff) AM_READWRITE(MRA8_RAM,berzerk_magicram_w) AM_SHARE(1)
	AM_RANGE(0x8000, 0x87ff) AM_READWRITE(MRA8_RAM,berzerk_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0xc000, 0xcfff) AM_ROM
	AM_RANGE(0xf800, 0xf9ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
ADDRESS_MAP_END


static ADDRESS_MAP_START( port_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x40, 0x47) AM_READWRITE(berzerk_sound_r,berzerk_sound_w) /* First sound board */
	AM_RANGE(0x48, 0x48) AM_READ(input_port_0_r)
	AM_RANGE(0x49, 0x49) AM_READ(input_port_1_r)
	AM_RANGE(0x4a, 0x4a) AM_READ(input_port_2_r)
	AM_RANGE(0x4b, 0x4b) AM_WRITE(berzerk_magicram_control_w)
	AM_RANGE(0x4c, 0x4c) AM_READWRITE(berzerk_nmi_enable_r,berzerk_nmi_enable_w)
	AM_RANGE(0x4d, 0x4d) AM_READWRITE(berzerk_nmi_disable_r,berzerk_nmi_disable_w)
	AM_RANGE(0x4e, 0x4e) AM_READ(berzerk_port_4e_r)
	AM_RANGE(0x4f, 0x4f) AM_WRITE(berzerk_irq_enable_w)
	AM_RANGE(0x50, 0x57) AM_WRITE(MWA8_NOP) /* Second sound board but not used */
	AM_RANGE(0x60, 0x60) AM_READ(input_port_4_r)
	AM_RANGE(0x61, 0x61) AM_READ(input_port_5_r)
	AM_RANGE(0x62, 0x62) AM_READ(input_port_6_r)
	AM_RANGE(0x63, 0x63) AM_READ(input_port_7_r)
	AM_RANGE(0x64, 0x64) AM_READ(input_port_8_r)
	AM_RANGE(0x65, 0x65) AM_READ(input_port_9_r)
	AM_RANGE(0x66, 0x66) AM_READ(berzerk_led_off_r)
	AM_RANGE(0x67, 0x67) AM_READ(berzerk_led_on_r)
ADDRESS_MAP_END


#define COINAGE(CHUTE) \
	PORT_DIPNAME( 0x0f, 0x00, "Coin "#CHUTE ) \
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x0d, DEF_STR( 4C_3C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x0e, DEF_STR( 4C_5C ) ) \
	PORT_DIPSETTING(    0x0a, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(    0x0f, DEF_STR( 4C_7C ) ) \
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(    0x0b, DEF_STR( 2C_5C ) ) \
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(    0x0c, DEF_STR( 2C_7C ) ) \
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) ) \
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) ) \
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_7C ) ) \
	PORT_DIPSETTING(    0x07, "1 Coin/10 Credits" ) \
	PORT_DIPSETTING(    0x08, "1 Coin/14 Credits" ) \
	PORT_BIT( 0xf0, IP_ACTIVE_LOW,  IPT_UNUSED )


INPUT_PORTS_START( berzerk )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x1c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x60, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )

	PORT_START      /* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x7e, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* Collision */

	PORT_START      /* IN4 */
	PORT_BIT(    0x01, 0x00, IPT_DIPSWITCH_NAME ) PORT_NAME("Input Test Mode") PORT_CODE(KEYCODE_F2) PORT_TOGGLE
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_BIT(    0x02, 0x00, IPT_DIPSWITCH_NAME ) PORT_NAME("Crosshair Pattern") PORT_CODE(KEYCODE_F4) PORT_TOGGLE
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_BIT( 0x3c, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPSETTING(    0x40, DEF_STR( German ) )
	PORT_DIPSETTING(    0x80, DEF_STR( French ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Spanish ) )

	PORT_START      /* IN5 */
	PORT_BIT(    0x03, 0x00, IPT_DIPSWITCH_NAME ) PORT_NAME("Color Test") PORT_CODE(KEYCODE_F5) PORT_TOGGLE
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x03, DEF_STR( On ) )
	PORT_BIT( 0x3c, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0xc0, "5000 and 10000" )
	PORT_DIPSETTING(    0x40, "5000" )
	PORT_DIPSETTING(    0x80, "10000" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )

	PORT_START      /* IN6 */
	COINAGE(3)

	PORT_START      /* IN7 */
	COINAGE(2)

	PORT_START      /* IN8 */
	COINAGE(1)

	PORT_START      /* IN9 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_BIT( 0x7e, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Stats") PORT_CODE(KEYCODE_F1)
INPUT_PORTS_END

INPUT_PORTS_START( frenzy )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x3c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x60, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )

	PORT_START      /* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x7e, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* Collision */

	PORT_START      /* IN4 */
	PORT_DIPNAME( 0x0f, 0x03, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x01, "1000" )
	PORT_DIPSETTING(    0x02, "2000" )
	PORT_DIPSETTING(    0x03, "3000" )
	PORT_DIPSETTING(    0x04, "4000" )
	PORT_DIPSETTING(    0x05, "5000" )
	PORT_DIPSETTING(    0x06, "6000" )
	PORT_DIPSETTING(    0x07, "7000" )
	PORT_DIPSETTING(    0x08, "8000" )
	PORT_DIPSETTING(    0x09, "9000" )
	PORT_DIPSETTING(    0x0a, "10000" )
	PORT_DIPSETTING(    0x0b, "11000" )
	PORT_DIPSETTING(    0x0c, "12000" )
	PORT_DIPSETTING(    0x0d, "13000" )
	PORT_DIPSETTING(    0x0e, "14000" )
	PORT_DIPSETTING(    0x0f, "15000" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_BIT( 0x30, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPSETTING(    0x40, DEF_STR( German ) )
	PORT_DIPSETTING(    0x80, DEF_STR( French ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Spanish ) )

	PORT_START      /* IN5 */
	PORT_BIT( 0x03, IP_ACTIVE_HIGH, IPT_UNUSED )  /* Bit 0 does some more hardware tests */
	PORT_BIT(    0x04, 0x00, IPT_DIPSWITCH_NAME ) PORT_NAME("Input Test Mode") PORT_CODE(KEYCODE_F2) PORT_TOGGLE
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_BIT(    0x08, 0x00, IPT_DIPSWITCH_NAME ) PORT_NAME("Crosshair Pattern") PORT_CODE(KEYCODE_F4) PORT_TOGGLE
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	/* The following 3 ports use all 8 bits, but I didn't feel like adding all 256 values :-) */
	PORT_START      /* IN6 */
	PORT_DIPNAME( 0x0f, 0x01, "Coins/Credit B" )
	/*PORT_DIPSETTING(    0x00, "0" )    Can't insert coins  */
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x05, "5" )
	PORT_DIPSETTING(    0x06, "6" )
	PORT_DIPSETTING(    0x07, "7" )
	PORT_DIPSETTING(    0x08, "8" )
	PORT_DIPSETTING(    0x09, "9" )
	PORT_DIPSETTING(    0x0a, "10" )
	PORT_DIPSETTING(    0x0b, "11" )
	PORT_DIPSETTING(    0x0c, "12" )
	PORT_DIPSETTING(    0x0d, "13" )
	PORT_DIPSETTING(    0x0e, "14" )
	PORT_DIPSETTING(    0x0f, "15" )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH,  IPT_UNUSED )

	PORT_START      /* IN7 */
	PORT_DIPNAME( 0x0f, 0x01, "Coins/Credit A" )
	/*PORT_DIPSETTING(    0x00, "0" )    Can't insert coins  */
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x05, "5" )
	PORT_DIPSETTING(    0x06, "6" )
	PORT_DIPSETTING(    0x07, "7" )
	PORT_DIPSETTING(    0x08, "8" )
	PORT_DIPSETTING(    0x09, "9" )
	PORT_DIPSETTING(    0x0a, "10" )
	PORT_DIPSETTING(    0x0b, "11" )
	PORT_DIPSETTING(    0x0c, "12" )
	PORT_DIPSETTING(    0x0d, "13" )
	PORT_DIPSETTING(    0x0e, "14" )
	PORT_DIPSETTING(    0x0f, "15" )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH,  IPT_UNUSED )

	PORT_START      /* IN8 */
	PORT_DIPNAME( 0x0f, 0x01, "Coin Multiplier" )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x05, "5" )
	PORT_DIPSETTING(    0x06, "6" )
	PORT_DIPSETTING(    0x07, "7" )
	PORT_DIPSETTING(    0x08, "8" )
	PORT_DIPSETTING(    0x09, "9" )
	PORT_DIPSETTING(    0x0a, "10" )
	PORT_DIPSETTING(    0x0b, "11" )
	PORT_DIPSETTING(    0x0c, "12" )
	PORT_DIPSETTING(    0x0d, "13" )
	PORT_DIPSETTING(    0x0e, "14" )
	PORT_DIPSETTING(    0x0f, "15" )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH,  IPT_UNUSED )

	PORT_START      /* IN9 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x7e, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Stats") PORT_CODE(KEYCODE_F1)
INPUT_PORTS_END

static struct S14001A_interface berzerk_s14001a_interface =
{
	REGION_SOUND1				/* where to find the voice data */
};

static struct CustomSound_interface custom_interface =
{
	berzerk_sh_start
};


static MACHINE_DRIVER_START( berzerk )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 10000000/4)        /* 2.5 MHz */
	MDRV_CPU_PROGRAM_MAP(berzerk_map,0)
	MDRV_CPU_IO_MAP(port_map,0)
	MDRV_CPU_VBLANK_INT(berzerk_interrupt,8)

	MDRV_MACHINE_RESET(berzerk)
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(TIME_IN_USEC(2500)  /* Needs to be long enough so 2 of the 8 */)
								/* interrupts fall inside the VBLANK */
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_SCREEN_VISIBLE_AREA(0, 256-1, 32, 256-1)

	MDRV_PALETTE_LENGTH(16)

	MDRV_PALETTE_INIT(berzerk)
	MDRV_VIDEO_START(berzerk)
	MDRV_VIDEO_UPDATE(generic_bitmapped)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(S14001A, 5000000)	/* CPU clock divided by 16 divided by a programmable TTL setup */
	MDRV_SOUND_CONFIG(berzerk_s14001a_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(custom_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( frenzy )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(berzerk)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(frenzy_map,0)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( berzerk )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "1c-0",         0x0000, 0x0800, CRC(ca566dbc) SHA1(fae2647f12f1cd82826db61b53b116a5e0c9f995) )
	ROM_LOAD( "1d-1",         0x1000, 0x0800, CRC(7ba69fde) SHA1(69af170c4a39a3494dcd180737e5c87b455f9203) )
	ROM_LOAD( "3d-2",         0x1800, 0x0800, CRC(a1d5248b) SHA1(a0b7842f6a5f86c16d80d78e7012c78b3ea11d1d) )
	ROM_LOAD( "5d-3",         0x2000, 0x0800, CRC(fcaefa95) SHA1(07f849aa39f1e3db938187ffde4a46a588156ddc) )
	ROM_LOAD( "6d-4",         0x2800, 0x0800, CRC(1e35b9a0) SHA1(5a5e549ec0e4803ab2d1eac6b3e7171aedf28244) )
	ROM_LOAD( "5c-5",         0x3000, 0x0800, CRC(c8c665e5) SHA1(e9eca4b119549e0061384abf52327c14b0d56624) )
	ROM_FILL( 0x3800, 0x0800, 0xff )

	ROM_REGION( 0x01000, REGION_SOUND1, 0 ) /* voice data */
	ROM_LOAD( "1c",           0x0000, 0x0800, CRC(2cfe825d) SHA1(f12fed8712f20fa8213f606c4049a8144bfea42e) )	/* VSU-1000 board */
	ROM_LOAD( "2c",           0x0800, 0x0800, CRC(d2b6324e) SHA1(20a6611ad6ec19409ac138bdae7bdfaeab6c47cf) )
ROM_END

ROM_START( berzerk1 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "rom0.1c",      0x0000, 0x0800, CRC(5b7eb77d) SHA1(8de488e279036fe40d6fb4c0dde16075309342fd) )
	ROM_LOAD( "rom1.1d",      0x1000, 0x0800, CRC(e58c8678) SHA1(a11f08448b457d690b270512c9f02fcf1e41d9e0) )
	ROM_LOAD( "rom2.3d",      0x1800, 0x0800, CRC(705bb339) SHA1(845191df90cd7d80f8fed3d2b69305301d921549) )
	ROM_LOAD( "rom3.5d",      0x2000, 0x0800, CRC(6a1936b4) SHA1(f1635e9d2f25514c35559d2a247c3bc4b4034c19) )
	ROM_LOAD( "rom4.6d",      0x2800, 0x0800, CRC(fa5dce40) SHA1(b3a3ee52bf65bbb3a20f905d3e4ebdf6871dcb5d) )
	ROM_LOAD( "rom5.5c",      0x3000, 0x0800, CRC(2579b9f4) SHA1(890f0237afbb194166eae88c98de81989f408548) )
	ROM_FILL( 0x3800, 0x0800, 0xff )

	ROM_REGION( 0x01000, REGION_SOUND1, 0 ) /* voice data */
	ROM_LOAD( "1c",           0x0000, 0x0800, CRC(2cfe825d) SHA1(f12fed8712f20fa8213f606c4049a8144bfea42e) )	/* VSU-1000 board */
	ROM_LOAD( "2c",           0x0800, 0x0800, CRC(d2b6324e) SHA1(20a6611ad6ec19409ac138bdae7bdfaeab6c47cf) )
ROM_END

ROM_START( frenzy )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "1c-0",         0x0000, 0x1000, CRC(abdd25b8) SHA1(e6a3ab826b51b2c6ddd63d55681848fccad800dd) )
	ROM_LOAD( "1d-1",         0x1000, 0x1000, CRC(536e4ae8) SHA1(913385c43b8902d3d3ad2194a3137e19e61c6573) )
	ROM_LOAD( "3d-2",         0x2000, 0x1000, CRC(3eb9bc9b) SHA1(1e43e76ae0606a6d41d9006005d6001bdee48694) )
	ROM_LOAD( "5d-3",         0x3000, 0x1000, CRC(e1d3133c) SHA1(2af4a9bc2b29735a548ae770f872127bc009cc42) )
	ROM_LOAD( "6d-4",         0xc000, 0x1000, CRC(5581a7b1) SHA1(1f633c1c29d3b64f701c601feba26da66a6c6f23) )

	ROM_REGION( 0x01000, REGION_SOUND1, 0 ) /* voice data */
	ROM_LOAD( "1c",           0x0000, 0x0800, CRC(2cfe825d) SHA1(f12fed8712f20fa8213f606c4049a8144bfea42e) )	/* VSU-1000 board */
	ROM_LOAD( "2c",           0x0800, 0x0800, CRC(d2b6324e) SHA1(20a6611ad6ec19409ac138bdae7bdfaeab6c47cf) )
ROM_END

/*
   The original / prototype version of moon war appears to run on Frenzy hardware, however the only board found
   had been stripped for parts, leaving only the sound ROMs.

   The more common version of Moon War runs on modified Super Cobra (scobra.c) hardware and is often called
   'Moon War 2' because it is the second version, and many of the PCBs are labeled as such
*/
ROM_START( moonwarp )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "code_roms",         0x0000, 0x1000, NO_DUMP )

	ROM_REGION( 0x01000, REGION_SOUND1, 0 ) /* voice data */
	ROM_LOAD( "moonwar.1c.bin",           0x0000, 0x0800, CRC(9e9a653f) SHA1(cf49a38ef343ace271ba1e5dde38bd8b9c0bd876) )	/* VSU-1000 board */
	ROM_LOAD( "moonwar.2c.bin",           0x0800, 0x0800, CRC(73fd988d) SHA1(08a2aeb4d87eee58e38e4e3f749a95f2308aceb0) )
ROM_END


GAME( 1980, berzerk,  0,       berzerk, berzerk, 0, ROT0, "Stern", "Berzerk (set 1)", 0 )
GAME( 1980, berzerk1, berzerk, berzerk, berzerk, 0, ROT0, "Stern", "Berzerk (set 2)", 0 )
GAME( 1982, frenzy,   0,       frenzy,  frenzy,  0, ROT0, "Stern", "Frenzy", 0 )
GAME( 1981, moonwarp, 0,       frenzy,  frenzy,  0, ROT0, "Stern", "Moon War (prototype on Frenzy hardware)", GAME_NOT_WORKING )
