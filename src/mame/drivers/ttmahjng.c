/***************************************************************************


 ***************************************************************************/

#include "driver.h"
#include "sound/ay8910.h"

extern unsigned char *ttmahjng_sharedram;
extern unsigned char *ttmahjng_videoram1;
extern unsigned char *ttmahjng_videoram2;
extern size_t ttmahjng_videoram_size;

PALETTE_INIT( ttmahjng );
VIDEO_START( ttmahjng );
WRITE8_HANDLER( ttmahjng_out0_w );
WRITE8_HANDLER( ttmahjng_out1_w );
WRITE8_HANDLER( ttmahjng_videoram1_w );
WRITE8_HANDLER( ttmahjng_videoram2_w );
READ8_HANDLER( ttmahjng_videoram1_r );
READ8_HANDLER( ttmahjng_videoram2_r );
WRITE8_HANDLER( ttmahjng_sharedram_w );
READ8_HANDLER( ttmahjng_sharedram_r );
VIDEO_UPDATE( ttmahjng );


static int psel;
static WRITE8_HANDLER( input_port_matrix_w )
{
	psel = data;
}

static READ8_HANDLER( input_port_matrix_r )
{
	int	cdata;

	cdata = 0;
	switch (psel)
	{
		case	1:
			cdata = readinputport(2);
			break;
		case	2:
			cdata = readinputport(3);
			break;
		case	4:
			cdata = readinputport(4);
			break;
		case	8:
			cdata = readinputport(5);
			break;
		default:
			break;
	}
	return cdata;
}


static ADDRESS_MAP_START( cpu1_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x43ff) AM_READ(ttmahjng_sharedram_r)
	AM_RANGE(0x4800, 0x4800) AM_READ(input_port_0_r)
	AM_RANGE(0x5000, 0x5000) AM_READ(input_port_1_r)
	AM_RANGE(0x5800, 0x5800) AM_READ(input_port_matrix_r)
	AM_RANGE(0x7838, 0x7838) AM_READ(MRA8_NOP)
	AM_RANGE(0x7859, 0x7859) AM_READ(MRA8_NOP)
	AM_RANGE(0x8000, 0xbfff) AM_READ(ttmahjng_videoram1_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( cpu1_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x43ff) AM_WRITE(ttmahjng_sharedram_w) AM_BASE(&ttmahjng_sharedram)
	AM_RANGE(0x4800, 0x4800) AM_WRITE(ttmahjng_out0_w)
	AM_RANGE(0x5000, 0x5000) AM_WRITE(ttmahjng_out1_w)
	AM_RANGE(0x5800, 0x5800) AM_WRITE(input_port_matrix_w)
	AM_RANGE(0x5f3e, 0x5f3e) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x6800, 0x6800) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x6900, 0x6900) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x8000, 0xbfff) AM_WRITE(ttmahjng_videoram1_w) AM_BASE(&ttmahjng_videoram1) AM_SIZE(&ttmahjng_videoram_size)
ADDRESS_MAP_END

static ADDRESS_MAP_START( cpu2_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x43ff) AM_READ(ttmahjng_sharedram_r)
	AM_RANGE(0x8000, 0xbfff) AM_READ(ttmahjng_videoram2_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( cpu2_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x43ff) AM_WRITE(ttmahjng_sharedram_w)
	AM_RANGE(0x8000, 0xbfff) AM_WRITE(ttmahjng_videoram2_w) AM_BASE(&ttmahjng_videoram2)
ADDRESS_MAP_END


INPUT_PORTS_START( ttmahjng )
	PORT_START      /* IN0 */
	PORT_DIPNAME( 0x01, 0x00, "Unknown 01" )
	PORT_DIPSETTING(    0x00, "00" )
	PORT_DIPSETTING(    0x01, "01" )
	PORT_DIPNAME( 0x02, 0x00, "Unknown 02" )
	PORT_DIPSETTING(    0x00, "00" )
	PORT_DIPSETTING(    0x02, "02" )
	PORT_DIPNAME( 0x04, 0x00, "Unknown 04" )
	PORT_DIPSETTING(    0x00, "00" )
	PORT_DIPSETTING(    0x04, "04" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown 08" )
	PORT_DIPSETTING(    0x00, "00" )
	PORT_DIPSETTING(    0x08, "08" )
	PORT_DIPNAME( 0x10, 0x00, "Unknown 10" )
	PORT_DIPSETTING(    0x00, "00" )
	PORT_DIPSETTING(    0x10, "10" )
	PORT_DIPNAME( 0x20, 0x00, "Unknown 20" )
	PORT_DIPSETTING(    0x00, "00" )
	PORT_DIPSETTING(    0x20, "20" )
	PORT_DIPNAME( 0x40, 0x00, "Unknown 40" )
	PORT_DIPSETTING(    0x00, "00" )
	PORT_DIPSETTING(    0x40, "40" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown 80" )
	PORT_DIPSETTING(    0x00, "00" )
	PORT_DIPSETTING(    0x80, "80" )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN2 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_MAHJONG_A )
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_MAHJONG_E )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_MAHJONG_I )
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_MAHJONG_M )
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_MAHJONG_KAN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )		// START2?
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN3 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_MAHJONG_B )
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_MAHJONG_F )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_MAHJONG_J )
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_MAHJONG_N )
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_MAHJONG_REACH )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )		// START1?
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN4 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_MAHJONG_C )
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_MAHJONG_G )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_MAHJONG_K )
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_MAHJONG_CHI )
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_MAHJONG_RON )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN5 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_MAHJONG_D )
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_MAHJONG_H )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_MAHJONG_L )
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_MAHJONG_PON )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


static MACHINE_DRIVER_START( ttmahjng )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,2500000)
	MDRV_CPU_PROGRAM_MAP(cpu1_readmem,cpu1_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 2500000)	/* 10MHz / 4 = 2.5MHz */
	MDRV_CPU_PROGRAM_MAP(cpu2_readmem,cpu2_writemem)

	MDRV_SCREEN_REFRESH_RATE(57)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_SCREEN_VISIBLE_AREA(0, 256-1, 0, 256-1)
	MDRV_PALETTE_LENGTH(8)

	MDRV_PALETTE_INIT(ttmahjng)
	MDRV_VIDEO_START(ttmahjng)
	MDRV_VIDEO_UPDATE(ttmahjng)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 10000000/8)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( ttmahjng )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "ju04", 0x0000, 0x1000, CRC(fe7c693a) SHA1(be0630557e0bcd9ec2e9542cc4a4d947889ec57a) )
	ROM_LOAD( "ju05", 0x1000, 0x1000, CRC(985723d3) SHA1(9d7499c48cfc242875a95d01459b8f3252ea41bc) )
	ROM_LOAD( "ju06", 0x2000, 0x1000, CRC(2cd69bc8) SHA1(a0a55c972291d043da9f76faf551dba790d5d103) )
	ROM_LOAD( "ju07", 0x3000, 0x1000, CRC(30e8ec63) SHA1(9c6a2b5e436b5e469c15f04c557839b6f07eb22e) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 ) /* color proms */
	/* The upper 128 bytes are 0's, used by the hardware to blank the display */
	ROM_LOAD( "ju03", 0x0000, 0x0100, CRC(27d47624) SHA1(ee04ce8043216be8b91413b546479419fca2b917) )
	ROM_LOAD( "ju09", 0x0100, 0x0100, CRC(27d47624) SHA1(ee04ce8043216be8b91413b546479419fca2b917) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the second CPU */
	ROM_LOAD( "ju01", 0x0000, 0x0800, CRC(0f05ca3c) SHA1(6af547b2ec4f69069b4ad62d96d109ec0105dd8b) )
	ROM_LOAD( "ju02", 0x0800, 0x0800, CRC(c1ffeceb) SHA1(18cf337ef2c9b51f1e9e4f08743225755c4ff420) )
	ROM_LOAD( "ju08", 0x1000, 0x0800, CRC(2dcc76b5) SHA1(1732bcf5492dda34425681e7f28775ad7a5e04af) )
ROM_END


GAME( 1981, ttmahjng, 0, ttmahjng, ttmahjng, 0, ROT0, "Taito", "Mahjong", 0 )
