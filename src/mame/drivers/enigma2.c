/********************************************************************

Enigma 2 (c) Zilec Electronics

driver by Pierpaolo Prazzoli and Tomasz Slanina

Two sets:

Enigma2 (1981)
 2xZ80 + AY8910

Enigma2a (1984?)
 Conversion applied to a Taito Space Invaders Part II board set. Bootleg ?

Writes to ROM/unmapped area = only durning gfx scrolling
                      (enigma and zilec logo, big spaceships)
                        Probably a bit buggy game code
TODO:
 - enigma2  - More accurate starfield emulation (screen flipping , blinking freq measured at 0.001 kHz)
 - enigma2a - bad sound ROM

*********************************************************************/

#include "driver.h"
#include "sound/ay8910.h"

static int sndlatch;
static int prevdata=0;
static int protdata=0;
static int flipscreen;
static int blink_cnt=0;

static READ8_HANDLER( dipsw_r )
{
	if(protdata!=0xff)
		return protdata^0x88;
	else
		return readinputport(2);
}

static WRITE8_HANDLER(sound_w)
{
	if((!(data&4)) && (prevdata&4)) // 0->1
		sndlatch=((sndlatch<<1)|((data&1)^1))&0xff;

	if(data&2)
		cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);

	prevdata=data;
}

static READ8_HANDLER(sound_r)
{
	return BITSWAP8(sndlatch,0,1,2,3,4,5,6,7);
}

static WRITE8_HANDLER(protection_w)
{
	protdata=data;
}

static WRITE8_HANDLER(videoctrl_w)
{
	if((data & 0x20) == 0x20 && (readinputport(2) & 0x20) == 0x20)
		flipscreen = 1;
	else
		flipscreen = 0;
}

static WRITE8_HANDLER(videoctrl2a_w)
{
	int i, old_flip;

	old_flip = flipscreen;

	if((data & 0x20) == 0x20 && (readinputport(2) & 0x20) == 0x20)
		flipscreen = 1;
	else
		flipscreen = 0;

	if(old_flip != flipscreen)
	{
		/* redraw screen */
		for(i=0;i<0x1c00;i++)
			videoram_w(i,videoram[i]);
	}
}

WRITE8_HANDLER( enigma2a_videoram_w )
{
	int i,x,y,col;
	videoram[offset]=data;
	y = offset / 32;
	col = 8 * (offset % 32);
	x = 255 - col;
	for (i = 0; i < 8; i++)
	{
		if(!flipscreen)
			plot_pixel(tmpbitmap, x, 255 - y, Machine->pens[(data&0x80)?1:0]);
		else
			plot_pixel(tmpbitmap, 255+15-x, y+31, Machine->pens[(data&0x80)?1:0]);
		x++;
		data <<= 1;
	}
}

/* unknown reads - protection ? mirrors ? */

static READ8_HANDLER( unknown_r1 )
{
	if( activecpu_get_pc() == 0x7e5 )
		return 0xaa;
	else
		return 0xf4;
}

static READ8_HANDLER( unknown_r2 )
{
	return 0x38;
}

static READ8_HANDLER( unknown_r3 )
{
	return 0xaa;
}

static READ8_HANDLER( unknown_r4 )
{
	return 0x38;
}

static READ8_HANDLER( enigma2_player2_r )
{
	if(flipscreen)
		return input_port_1_r(0);
	else
		return input_port_0_r(0);
}

/* Enigma 2 */

static ADDRESS_MAP_START( main_cpu, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0x23ff) AM_RAM
	AM_RANGE(0x2400, 0x3fff) AM_RAM AM_BASE(&videoram)
	AM_RANGE(0x4000, 0x4fff) AM_ROM
	AM_RANGE(0x5001, 0x5001) AM_READ(dipsw_r)
	AM_RANGE(0x5002, 0x5002) AM_READ(unknown_r1)
	AM_RANGE(0x5803, 0x5803) AM_WRITE(sound_w)
	AM_RANGE(0x5805, 0x5805) AM_WRITE(videoctrl_w)
	AM_RANGE(0x5806, 0x5806) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x5035, 0x5035) AM_READ(unknown_r2)
	AM_RANGE(0x5051, 0x5051) AM_READ(unknown_r3)
	AM_RANGE(0x5079, 0x5079) AM_READ(unknown_r4)
	AM_RANGE(0x5801, 0x5801) AM_READ(input_port_0_r)
	AM_RANGE(0x5802, 0x5802) AM_READ(enigma2_player2_r)
ADDRESS_MAP_END

/* Enigma 2a */

static ADDRESS_MAP_START( main2a_cpu, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0x23ff) AM_RAM
	AM_RANGE(0x2400, 0x3fff) AM_READ(videoram_r) AM_WRITE(enigma2a_videoram_w) AM_BASE(&videoram) AM_BASE(&videoram)
	AM_RANGE(0x4000, 0x4fff) AM_ROM
	AM_RANGE(0x5001, 0x5001) AM_READ(dipsw_r)
	AM_RANGE(0x5002, 0x5002) AM_READ(unknown_r1)
	AM_RANGE(0x5035, 0x5035) AM_READ(unknown_r2)
	AM_RANGE(0x5051, 0x5051) AM_READ(unknown_r3)
	AM_RANGE(0x5079, 0x5079) AM_READ(unknown_r4)
ADDRESS_MAP_END

static ADDRESS_MAP_START( portmap, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_READ(input_port_0_r)
	AM_RANGE(0x02, 0x02) AM_READ(enigma2_player2_r)
	AM_RANGE(0x03, 0x03) AM_WRITE(sound_w)
	AM_RANGE(0x05, 0x05) AM_WRITE(videoctrl2a_w)
	AM_RANGE(0x06, 0x06) AM_WRITE(MWA8_NOP)
ADDRESS_MAP_END


/* sound cpu */
static ADDRESS_MAP_START( sound_cpu, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_ROM
	AM_RANGE(0x8000, 0x83ff) AM_RAM
	AM_RANGE(0xa000, 0xa000) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0xa001, 0xa001) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0xa002, 0xa002) AM_READ(AY8910_read_port_0_r)
ADDRESS_MAP_END

PALETTE_INIT( enigma2 )
{
	palette_set_color(machine,0,0,0,0);
	palette_set_color(machine,1,0,0xff,0);
	palette_set_color(machine,2,0,0,0xff);
	palette_set_color(machine,3,0,0xff,0xff);
	palette_set_color(machine,4,0xff,0,0);
	palette_set_color(machine,5,0xff,0xff,0);
	palette_set_color(machine,6,0xff,0,0xff);
	palette_set_color(machine,7,0xff,0xff,0xff);
}

VIDEO_UPDATE( enigma2 )
{
	int i,x,y,col,offs,data;
	fillbitmap(bitmap,Machine->pens[0],&Machine->screen[0].visarea);
	blink_cnt++;

	/* starfield */
	for(y=0;y<32;y++)
		for(x=0;x<32;x++)
		{
		 	offs = y * 32 + x + 1024 * ((blink_cnt>>3)&1);
			plot_pixel(bitmap,x*8,255-y*8,Machine->pens[memory_region(REGION_USER1)[offs]]);
		}

	/* gfx */
	for(offs=0;offs<0x1c00;offs++)
	{
		y = offs / 32;
		col = 8 * (offs % 32);
		x = 255 - col;

		data=videoram[offs];

		for (i = 0; i < 8; i++)
		{
			if( data & 0x80 )
			{
				if(!flipscreen)
					plot_pixel(bitmap, x, 255 - y, memory_region(REGION_PROMS)[((y+32) >> 3 << 5) + (col >> 3)] & 0x07);
				else
					plot_pixel(bitmap, 255+15-x, y+31, memory_region(REGION_PROMS)[((35-((y+32) >> 3)) << 5) + (31-(col >> 3))+0x400] & 0x07);
			}

			x++;
			data <<= 1;
		}
	}
	return 0;
}

INPUT_PORTS_START( enigma2 )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x1c, 0x00, "Skill level" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x14, "6" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x40, 0x40, "Number of invaders" )
	PORT_DIPSETTING(    0x40, "16" )
	PORT_DIPSETTING(    0x00, "32" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
INPUT_PORTS_END

INPUT_PORTS_START( enigma2a )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x1c, 0x00, "Skill level" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x14, "6" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
INPUT_PORTS_END


INTERRUPT_GEN( enigma2_interrupt )
{
	int vector = video_screen_get_vblank(0) ? 0xcf : 0xd7;
    cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, vector);
}


static struct AY8910interface ay8910_interface =
{
	sound_r,
	0,
	0,
	protection_w
};

static MACHINE_DRIVER_START( enigma2 )
	MDRV_CPU_ADD_TAG("main", Z80, 2500000)
	MDRV_CPU_PROGRAM_MAP(main_cpu,0)
	MDRV_CPU_VBLANK_INT(enigma2_interrupt,2)

	MDRV_CPU_ADD(Z80, 2500000)
	/* audio CPU */
	MDRV_CPU_VBLANK_INT(irq0_line_hold,8)
	MDRV_CPU_PROGRAM_MAP(sound_cpu,0)


	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_SCREEN_VISIBLE_AREA(2*8, 32*8-1, 4*8, 32*8-1)
	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT(enigma2)

	MDRV_VIDEO_UPDATE(enigma2)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 2500000/2)
	MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( enigma2a )
	MDRV_CPU_ADD_TAG("main", 8080, 2000000)
	MDRV_CPU_PROGRAM_MAP(main2a_cpu,0)
	MDRV_CPU_IO_MAP(portmap,0)
	MDRV_CPU_VBLANK_INT(enigma2_interrupt,2)

	MDRV_CPU_ADD(Z80, 2500000)
	/* audio CPU */
	MDRV_CPU_VBLANK_INT(irq0_line_hold,8)
	MDRV_CPU_PROGRAM_MAP(sound_cpu,0)

	MDRV_SCREEN_REFRESH_RATE(60)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_SCREEN_VISIBLE_AREA(2*8, 32*8-1, 4*8, 32*8-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(generic_bitmapped)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 2500000/2)
	MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

ROM_START( enigma2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "1.5d",         0x0000, 0x0800, CRC(499749de) SHA1(401928ff41d3b4cbb68e6ad3bf3be4a10ae1781f) )
	ROM_LOAD( "2.7d",         0x0800, 0x0800, CRC(173c1329) SHA1(3f1ad46d0e58ab236e4ff2b385d09fbf113627da) )
	ROM_LOAD( "3.8d",         0x1000, 0x0800, CRC(c7d3e6b1) SHA1(43f7c3a02b46747998260d5469248f21714fe12b) )
	ROM_LOAD( "4.10d",        0x1800, 0x0800, CRC(c6a7428c) SHA1(3503f09856655c5973fb89f60d1045fe41012aa9) )
	ROM_LOAD( "5.11d",   	  0x4000, 0x0800, CRC(098ac15b) SHA1(cce28a2540a9eabb473391fff92895129ae41751) )
	ROM_LOAD( "6.13d",   	  0x4800, 0x0800, CRC(240a9d4b) SHA1(ca1c69fafec0471141ce1254ddfaef54fecfcbf0) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "enigma2.s",    0x0000, 0x1000, CRC(68fd8c54) SHA1(69996d5dfd996f0aacb26e397bef314204a2a88a) )

	 /* Color Map */
	ROM_REGION( 0x800, REGION_PROMS, 0 )
	ROM_LOAD( "7.11f",        0x0000, 0x0800, CRC(409b5aad) SHA1(1b774a70f725637458ed68df9ed42476291b0e43) )

	/* Star Map */
	ROM_REGION( 0x800, REGION_USER1, 0 )
	ROM_LOAD( "8.13f",        0x0000, 0x0800, CRC(e9cb116d) SHA1(41da4f46c5614ec3345c233467ebad022c6b0bf5) )
ROM_END


ROM_START( enigma2a )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "36_en1.bin",   0x0000, 0x0800, CRC(15f44806) SHA1(4a2f7bc91d4edf7a069e0865d964371c97af0a0a) )
	ROM_LOAD( "35_en2.bin",   0x0800, 0x0800, CRC(e841072f) SHA1(6ab02fd9fdeac5ab887cd25eee3d6b70ab01f849) )
	ROM_LOAD( "34_en3.bin",   0x1000, 0x0800, CRC(43d06cf4) SHA1(495af05d54c0325efb67347f691e64d194645d85) )
	ROM_LOAD( "33_en4.bin",   0x1800, 0x0800, CRC(8879a430) SHA1(c97f44bef3741eef74e137d2459e79f1b3a90457) )
	ROM_LOAD( "5.11d",        0x4000, 0x0800, CRC(098ac15b) SHA1(cce28a2540a9eabb473391fff92895129ae41751) )
	ROM_LOAD( "6.13d",   	  0x4800, 0x0800, CRC(240a9d4b) SHA1(ca1c69fafec0471141ce1254ddfaef54fecfcbf0) )


	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "sound.bin",    0x0000, 0x0800, BAD_DUMP CRC(5f092d3c) SHA1(17c70f6af1b5560a45e6b1bdb330a98b27570fe9) )
ROM_END


static DRIVER_INIT(enigma2)
{
	int i;
	for(i=0;i<0x2000;i++)
		memory_region(REGION_CPU2)[i]=BITSWAP8(memory_region(REGION_CPU2)[i],4,5,6,0,7,1,3,2);
}

GAME( 1981, enigma2,  0,	   enigma2,  enigma2,  enigma2, ROT90, "GamePlan (Zilec Electronics license)", "Enigma 2", 0 )
GAME( 1984, enigma2a, enigma2, enigma2a, enigma2a, enigma2, ROT90, "Zilec Electronics", "Enigma 2 (Space Invaders Hardware)", 0 )
