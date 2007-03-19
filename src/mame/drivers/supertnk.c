/***************************************************************************

Super Tank
(c) 1981 Video Games GmbH

Reverse-engineering and MAME driver by Norbert Kehrer (December 2003).

****************************************************************************


Hardware:
---------

CPU     TMS9980 (Texas Instruments)
Sound chip  AY-3-8910


Memory map:
-----------

>0000 - >07ff   Fixed ROM
>0800 - >17ff   Bank-switched ROM
>1800 - >1bff   RAM
>1efc       Input port 1
>1efd       Input port 2
>1efe       Read:  DIP switch port 1
        Write: Control port of AY-3-8910 sound chip
>1eff       Read: DIP switch port 2
        Write: Data port of AY-3-8910
>2000 - >3fff   Video RAM (CPU view of it, in reality there are 24 KB on the video board)


Input ports:
------------

Input port 0, mapped to memory address 0x1efc:
7654 3210
0          Player 2 Right
 0         Player 2 Left
  0        Player 2 Down
   0       Player 2 Up
     0     Player 1 Right
      0    Player 1 Left
       0   Player 1 Down
        0  Player 1 Up

Input port 1, mapped to memory address 0x1efd:
7654 3210
0          Player 2 Fire
 0         Player 1 Fire
  0        ??
   0       ??
     0     Start 1 player game
      0    Start 2 players game
       0   Coin (strobe)
        0  ??


DIP switch ports:
-----------------

DIP switch port 1, mapped to memory address 0x1efe:
7654 3210
0          Not used (?)
 0         Not used (?)
  0        Tanks per player: 1 = 5 tanks, 0 = 3 tanks
   0       Extra tank: 1 = at 10,000, 0 = at 15,000 pts.
     000   Coinage
        0  Not used (?)

DIP switch port 2, mapped to memory address 0x1eff:
7654 3210
1          ??
 1         ??
  1        ??
   1       ??
     1     ??
      1    ??
       1   ??
        1  ??


CRU lines:
----------

>400    Select bitplane for writes into video RAM (bit 1)
>401    Select bitplane for writes into video RAM (bit 0)
>402    ROM bank selector (bit 1)
>404    ROM bank selector (bit 0)
>406    Interrupt acknowledge (clears interrupt line)
>407    Watchdog reset (?)
>b12    Unknown, maybe some special-hardware sound effect or lights blinking (?)
>b13    Unknown, maybe some special-hardware sound effect or lights blinking (?)

***************************************************************************/


#include "driver.h"
#include "sound/ay8910.h"

static int supertnk_rom_bank;
static int supertnk_video_bitplane;

static UINT8 *supertnk_videoram;



PALETTE_INIT( supertnk )
{
	int i;

	for (i = 0;i < 0x20;i++)
	{
		palette_set_color(machine, i, pal1bit(*color_prom >> 2), pal1bit(*color_prom >> 5), pal1bit(*color_prom >> 6));
		color_prom++;
	}
}



VIDEO_START( supertnk )
{
	supertnk_videoram = auto_malloc(0x6000);	/* allocate physical video RAM */

	memset(supertnk_videoram, 0, 0x6000);

	return video_start_generic_bitmapped(machine);
}



WRITE8_HANDLER( supertnk_videoram_w )
{
	int x, y, i, col, col0, col1, col2;

	if (supertnk_video_bitplane > 2)
	{
		supertnk_videoram[0x0000 + offset] =
		supertnk_videoram[0x2000 + offset] =
		supertnk_videoram[0x4000 + offset] = 0;
	}
	else
		supertnk_videoram[0x2000 * supertnk_video_bitplane + offset] = data;


	x = (offset % 32) * 8 ;
	y = (offset / 32);

	for (i=0; i<8; i++)
	{
		col0 = (supertnk_videoram[0x0000 + offset] >> (7-i)) & 0x01;
		col1 = (supertnk_videoram[0x2000 + offset] >> (7-i)) & 0x01;
		col2 = (supertnk_videoram[0x4000 + offset] >> (7-i)) & 0x01;
		col = ( (col0 << 2) | (col1 << 1) | (col2 << 0) );

		plot_pixel(tmpbitmap, x+i, y, Machine->pens[col]);
	}
}



READ8_HANDLER( supertnk_videoram_r )
{
	if (supertnk_video_bitplane < 3)
		return supertnk_videoram[0x2000 * supertnk_video_bitplane + offset];
	else
		return 0;
}


VIDEO_UPDATE( supertnk )
{
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->screen[0].visarea,TRANSPARENCY_NONE,0);
	return 0;
}



WRITE8_HANDLER( supertnk_intack )
{
	cpunum_set_input_line(0, 0, CLEAR_LINE);
}



WRITE8_HANDLER( supertnk_bankswitch_w )
{
	int bankaddress;
	UINT8 *ROM = memory_region(REGION_CPU1);
	switch (offset)
	{
		case 0:
			supertnk_rom_bank &= 0x02;
			supertnk_rom_bank |= (data & 0x01);
			break;
		case 2:
			supertnk_rom_bank &= 0x01;
			supertnk_rom_bank |= ((data & 0x01) << 1);
			break;
	}
	bankaddress = 0x4000 + supertnk_rom_bank * 0x1000;
	memory_set_bankptr(1,&ROM[bankaddress]);
}



WRITE8_HANDLER( supertnk_set_video_bitplane )
{
	switch (offset)
	{
		case 0:
			supertnk_video_bitplane &= 0x02;
			supertnk_video_bitplane |= (data & 0x01);
			break;
		case 1:
			supertnk_video_bitplane &= 0x01;
			supertnk_video_bitplane |= ((data & 0x01) << 1);
			break;
	}
}



INTERRUPT_GEN( supertnk_interrupt )
{
	/* On a TMS9980, a 6 on the interrupt bus means a level 4 interrupt */
	cpunum_set_input_line_and_vector(0, 0, ASSERT_LINE, 6);
}



static ADDRESS_MAP_START( supertnk_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_READ(MRA8_ROM)			/* Fixed ROM */
	AM_RANGE(0x0800, 0x17ff) AM_READ(MRA8_BANK1)			/* Banked ROM */
	AM_RANGE(0x1800, 0x1bff) AM_READ(MRA8_RAM)
	AM_RANGE(0x2000, 0x3fff) AM_READ(supertnk_videoram_r)	/* Video RAM */
	AM_RANGE(0x1efc, 0x1efc) AM_READ(input_port_0_r)		/* Input ports */
	AM_RANGE(0x1efd, 0x1efd) AM_READ(input_port_1_r)
	AM_RANGE(0x1efe, 0x1efe) AM_READ(input_port_2_r)		/* DIP switch ports */
	AM_RANGE(0x1eff, 0x1eff) AM_READ(input_port_3_r)
ADDRESS_MAP_END



static ADDRESS_MAP_START( supertnk_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x17ff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x1800, 0x1bff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1efe, 0x1efe) AM_WRITE(AY8910_control_port_0_w)	/* Sound chip control port */
	AM_RANGE(0x1eff, 0x1eff) AM_WRITE(AY8910_write_port_0_w)	/* Sound chip data port */
	AM_RANGE(0x2000, 0x3fff) AM_WRITE(supertnk_videoram_w)	/* Video RAM */
ADDRESS_MAP_END



static ADDRESS_MAP_START( supertnk_writeport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x000, 0x000) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x400, 0x401) AM_WRITE(supertnk_set_video_bitplane)
	AM_RANGE(0x402, 0x404) AM_WRITE(supertnk_bankswitch_w)
	AM_RANGE(0x406, 0x406) AM_WRITE(supertnk_intack)
	AM_RANGE(0x407, 0x407) AM_WRITE(watchdog_reset_w)
ADDRESS_MAP_END





static MACHINE_DRIVER_START( supertnk )

	/* basic machine hardware */
	MDRV_CPU_ADD(TMS9980, 2598750) /* ? to which frequency is the 20.79 Mhz crystal mapped down? */
	MDRV_CPU_PROGRAM_MAP(supertnk_readmem,supertnk_writemem)
	MDRV_CPU_IO_MAP(0,supertnk_writeport)
	MDRV_CPU_VBLANK_INT(supertnk_interrupt,1)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(32)

	MDRV_PALETTE_INIT(supertnk)
	MDRV_VIDEO_START(supertnk)
	MDRV_VIDEO_UPDATE(supertnk)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 2000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END



INPUT_PORTS_START( supertnk )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x0e, 0x02, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x0c, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x0a, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0x08, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "at 15,000 points" )
	PORT_DIPSETTING(	0x10, "at 10,000 points" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING( 	0x00, "3" )
	PORT_DIPSETTING(    	0x20, "5" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
INPUT_PORTS_END



ROM_START( supertnk )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for TMS9980 code - normal and banked ROM */
	ROM_LOAD( "supertan.2d",	0x0000, 0x0800, CRC(1656a2c1) SHA1(1d49945aed105003a051cfbf646af7a4be1b7e86) )
	ROM_LOAD( "supertnk.3d",	0x4800, 0x0800, CRC(8b023a9a) SHA1(1afdc8d75f2ca04153bac20c0e3e123e2a7acdb7) )
		ROM_CONTINUE(		0x4000, 0x0800)
	ROM_LOAD( "supertnk.4d",	0x5800, 0x0800, CRC(b8249e5c) SHA1(ef4bb714b0c1b97890a067f05fc50ab3426ce37f) )
		ROM_CONTINUE(		0x5000, 0x0800)
	ROM_LOAD( "supertnk.8d",	0x6800, 0x0800, CRC(d8175a4f) SHA1(cba7b426773ac86c81a9eac81087a2db268cd0f9) )
		ROM_CONTINUE(		0x6000, 0x0800)
	ROM_LOAD( "supertnk.9d",	0x7800, 0x0800, CRC(a34a494a) SHA1(9b7f0560e9d569ee25eae56f31886d50a3153dcc) )
		ROM_CONTINUE(		0x7000, 0x0800)

	ROM_REGION( 0x0060, REGION_PROMS, 0 ) /* color PROM */
	ROM_LOAD( "supertnk.clr",	0x0000, 0x0020, CRC(9ae1faee) SHA1(19de4bb8bc389d98c8f8e35c755fad96e1a6a0cd) )
 	/* unknown */
	ROM_LOAD( "supertnk.s",		0x0020, 0x0020, CRC(91722fcf) SHA1(f77386014b459cc151d2990ac823b91c04e8d319) )
	/* unknown */
	ROM_LOAD( "supertnk.t",		0x0040, 0x0020, CRC(154390bd) SHA1(4dc0fd7bd8999d2670c8d93aaada835d2a84d4db) )
ROM_END


DRIVER_INIT( supertnk ){
	/* decode the TMS9980 ROMs */
	UINT8 *pMem = memory_region( REGION_CPU1 );
	UINT8 raw, code;
	int i;
	for( i=0; i<0x8000; i++ )
	{
		raw = pMem[i];
		code = 0;
		if( raw&0x01 ) code |= 0x80;
		if( raw&0x02 ) code |= 0x40;
		if( raw&0x04 ) code |= 0x20;
		if( raw&0x08 ) code |= 0x10;
		if( raw&0x10 ) code |= 0x08;
		if( raw&0x20 ) code |= 0x04;
		if( raw&0x40 ) code |= 0x02;
		if( raw&0x80 ) code |= 0x01;
		pMem[i] = code;
	};

	supertnk_rom_bank = 0;
	supertnk_video_bitplane = 0;
}



/*          rom       parent     machine   inp       init */
GAME( 1981, supertnk,  0,        supertnk, supertnk, supertnk, ROT90, "Video Games GmbH", "Super Tank", 0 )

