/*********************************
 Wild Arrow - Meyco Games 1982

 Preliminary driver by
        Tomasz Slanina
        Pierpaolo Prazzoli

Wild Arrow (c) 1981 Meyco Games

CPU: 8080A

RAM: 411A (x48)

XTal: 20.0


Probably missing sound board.

**********************************/
#include "driver.h"


static UINT8 *vram1;
static UINT8 *vram2;
static UINT8 *vram3;

static void putpixel(int offset)
{
		int i,x,y,col,d1,d2,d3;

		d1=vram1[offset];
		d2=vram2[offset];
		d3=vram3[offset];

		y = offset / 32;
		col = 8 * (offset % 32);
		x = col;

		for (i = 0; i < 8; i++)
		{
			plot_pixel(tmpbitmap, x, y, Machine->pens[ ((d1&0x80)>>7)|((d2&0x80)>>6)|((d3&0x80)>>5) ]);
			x++;
			d1 <<= 1;
			d2 <<= 1;
			d3 <<= 1;
		}
}

static WRITE8_HANDLER( videoram1_w )
{
	vram1[offset]=data;
	putpixel(offset);
}

static WRITE8_HANDLER( videoram2_w )
{
	vram2[offset]=data;
	putpixel(offset);
}


static WRITE8_HANDLER( videoram3_w )
{
	vram3[offset]=data;
	putpixel(offset);
}


static READ8_HANDLER( videoram1_r )
{
	return vram1[offset];
}

static READ8_HANDLER( videoram2_r )
{
	return vram2[offset];
}

static READ8_HANDLER( videoram3_r )
{
	return vram3[offset];
}


static READ8_HANDLER( unk_r )
{
	return (rand()&0xf)|(input_port_2_r(0)&0xf0)|0x80;
}


static ADDRESS_MAP_START( memory_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0x2fff) AM_ROM
	AM_RANGE(0x3800, 0x3800) AM_READ(input_port_0_r)
	AM_RANGE(0x4000, 0x43ff) AM_RAM
	AM_RANGE(0x4400, 0x5fff) AM_READWRITE(videoram1_r, videoram1_w) AM_BASE(&vram1)

	AM_RANGE(0x6000, 0x60ff) AM_RAM
	AM_RANGE(0x6400, 0x7fff) AM_READWRITE(videoram2_r, videoram2_w) AM_BASE(&vram2)

	AM_RANGE(0x8000, 0x80ff) AM_RAM
	AM_RANGE(0x8400, 0x9fff) AM_READWRITE(videoram3_r, videoram3_w) AM_BASE(&vram3)

	AM_RANGE(0xa000, 0xa0ff) AM_RAM
	AM_RANGE(0xcd00, 0xcd1f) AM_RAM

	AM_RANGE(0xf000, 0xf000) AM_READ(input_port_1_r) AM_WRITENOP
	AM_RANGE(0xf001, 0xf003) AM_WRITENOP
	AM_RANGE(0xf004, 0xf004) AM_READ(unk_r) AM_WRITENOP
	AM_RANGE(0xf005, 0xf00f) AM_READ(input_port_3_r)
	AM_RANGE(0xf006, 0xf006) AM_WRITENOP
	AM_RANGE(0xf0f0, 0xf0ff) AM_WRITENOP
ADDRESS_MAP_END

INPUT_PORTS_START( wldarrow )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_DIPNAME( 0x04, 0x00, "Color" ) /* ??? */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "0-0" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "0-1" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "0-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "0-3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "0-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "0-5" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "0-6" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "0-7" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_DIPNAME( 0x01, 0x01, "1-0" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "1-1" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "1-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "1-3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "1-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "1-5" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "1-6" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_DIPNAME( 0x01, 0x01, "2-0" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "2-1" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "2-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "2-3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "2-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "2-5" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "2-6" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

static MACHINE_DRIVER_START( wldarrow )
	MDRV_CPU_ADD(8080, 2000000)
	MDRV_CPU_PROGRAM_MAP(memory_map,0)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
	MDRV_PALETTE_LENGTH(8)

	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(generic_bitmapped)
MACHINE_DRIVER_END

ROM_START( wldarrow )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "a1-v48.8k",    0x0000, 0x0800, CRC(05dd8056) SHA1(556ca28d090cbf1855618ba40fc631523bdfadd5) )
	ROM_LOAD( "a2-v48.7k",    0x0800, 0x0800, CRC(37df3acf) SHA1(a7f7f54af533dd8231bb20c526c053dd99e74863) )
	ROM_LOAD( "a3-v48.6k",    0x1000, 0x0800, CRC(1295cee2) SHA1(61b260eb907ee4bbf1460277d09e3205c1f6d8a0) )
	ROM_LOAD( "a4-v48.5k",    0x1800, 0x0800, CRC(5562614e) SHA1(7cb04d76e987944d385d40515396fc27ba00ae83) )
ROM_END

/*

Meyco Games, unknown PCB, 1981

NOTE TO WHOEVER IS MAME DEVELOPER FOR THIS DRIVER:  This is obviously part of a multiple board set.  The other PCB board(s) were unfound.  If/when the game is identified, could you please let me know what it is?

Dumper notes:

-Board is approximately 16.5"x14.25", with no daughterboards or satellite add-on boards.

-Board has 8 pin connector and 50 pin connector with no keying pins (see diagram)

-Board has an 20.000MHz crystal @ C4 (see diagram below).

-No dip switchs or sound chips on this board.

-Board has 32 uPD411 RAM chips.

-The 40 pin CPU is missing, Guru feels it will be an 8080 CPU.

-Dump consists of:
--6x2516 @ K3, K4, K5, K6, K7, K8.

   K      J     H       G      F             E      D      C       B       A
  |------------------------------------------------------------------------------|
 1|                                                                              |
 2|                                                                              |
 3|ROM.3k                                    DS0026CN                            |
 4|ROM.4k                                           20.000                      *|
 5|ROM.5k                      N8T26AN                                           |
 6|ROM.6k               C      898-1-R 1.5K  N8T26AN                            &|
 7|ROM.7k               P      N8T26AN                                          &|
 8|ROM.8k               U      N8T26AN                                          &|
 9|                                                                             &|
10|                                                                              |
11|                                                                              |
12|uPD411 uPD411 uPD411 uPD411 uPD411        uPD411 uPD411 uPD411                |
13|uPD411 uPD411 uPD411 uPD411 uPD411        uPD411 uPD411 uPD411                |
14|uPD411 uPD411 uPD411 uPD411 uPD411        uPD411 uPD411 uPD411                |
15|uPD411 uPD411 uPD411 uPD411 uPD411        uPD411 uPD411 uPD411                |
  |------------------------------------------------------------------------------|

* = 8 pin connector
& = 50 pin connector (assuming to other half of PCB)

*/

ROM_START( unkmeyco )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "tms2516.k8",   0x0000, 0x0800, CRC(2e5fc31e) SHA1(5ea01298051bc51250f67305ac8a65b0b94c120f) )
	ROM_LOAD( "tms2516.k7",   0x0800, 0x0800, CRC(baaf874e) SHA1(b7bb476ef873102979ad3252d19a26ee3a31d933) )
	ROM_LOAD( "tms2516.k6",   0x1000, 0x0800, CRC(a0e13c41) SHA1(17f78f91dae64c39f1a39a0b99a081af1d3bed47) )
	ROM_LOAD( "tms2516.k5",   0x1800, 0x0800, CRC(530a48fd) SHA1(e539962d19d884794ece2e426423f6b33d54058d) )
	ROM_LOAD( "tms2516.k4",   0x2000, 0x0800, CRC(bb1bd38a) SHA1(90256991eb1d030dd72e7e6f8d1a7cce22340b42) )
	ROM_LOAD( "tms2516.k3",   0x2800, 0x0800, CRC(30904dc8) SHA1(c82276aa0eb8f48d136ad8c15dd309c9b880c294) )
ROM_END

DRIVER_INIT( wldarrow )
{
	int i;
	for( i = 0; i < 0x3000; i++ )
	{
		memory_region(REGION_CPU1)[i]^=0xff;
	}
}

GAME( 1982, wldarrow,  0,		wldarrow, wldarrow, wldarrow, ROT0, "Meyco Games", "Wild Arrow", GAME_NO_SOUND | GAME_NOT_WORKING)
GAME( 198?, unkmeyco,  0,		wldarrow, wldarrow, wldarrow, ROT0, "Meyco Games", "<unknown> Meyco game", GAME_NO_SOUND | GAME_NOT_WORKING)
