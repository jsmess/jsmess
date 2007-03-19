/*
DJ Boy (c)1989 Kanako

Hardware has many similarities to Airbusters.

You must manually reset (F3) after booting up to proceed.
There's likely an unemulated hardware watchdog.

Video hardware and sound should both be mostly correct.
There's still some missing protection and/or exotic CPU communication.

Self Test:
press button#3 to advance past color pattern
buttons 1,2,3 are used to select and play sound/music


- CPU0 manages sprites, which are also used to display text
        irq (0x10) - timing/watchdog
        irq (0x30) - processes sprites
        nmi: wakes up this cpu

- CPU1 manages the protection device, palette, and tilemap(s)
        nmi: resets this cpu
        irq: game update
        additional protection at d8xx?

- CPU2 manages sound chips
        irq: update music
        nmi: handle sound command

    The protection device provides an API to poll dipswitches and inputs.
    It is probably involved with the memory range 0xd800..0xd8ff, which CPU2 reads.
    It handles coin input and coinage internally.
    The real game shouts "DJ Boy!" every time a credit is inserted.


Genre: Scrolling Fighter
Orientation: Horizontal
Type: Raster: Standard Resolution
CRT: Color
Conversion Class: JAMMA
Number of Simultaneous Players: 2
Maximum number of Players: 2
Gameplay: Joint
Control Panel Layout: Multiple Player
Joystick: 8-way
Buttons: 3 - Punch, Kick, Jump
Sound: Amplified Mono (one channel) - Stereo sound is available
through a 4-pin header (voice of Wolfman Jack!!)


                     BS-65  6116
                     BS-101 6116
6264                 780C-2
BS-005
BS-004               6264
16mhz
12mhz                                   beast
        41101
BS-003                BS-203    6295    2203
BS-000  pandora       780C-2    6295
BS-001
BS-002  4464 4464     BS-100          6264
BS07    4464 4464     BS-64           BS-200
*/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "sound/2203intf.h"
#include "sound/okim6295.h"

/* public functions from video/djboy.h */
extern void djboy_set_videoreg( UINT8 data );
extern WRITE8_HANDLER( djboy_scrollx_w );
extern WRITE8_HANDLER( djboy_scrolly_w );
extern WRITE8_HANDLER( djboy_videoram_w );
extern WRITE8_HANDLER( djboy_paletteram_w );
extern VIDEO_START( djboy );
extern VIDEO_UPDATE( djboy );

static UINT8 *sharedram;
static READ8_HANDLER( sharedram_r )	{ return sharedram[offset]; }
static WRITE8_HANDLER( sharedram_w )	{ sharedram[offset] = data; }

/******************************************************************************/

static WRITE8_HANDLER( trigger_nmi_on_cpu0 )
{
	cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
}

static WRITE8_HANDLER( cpu0_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	logerror( "cpu1_bankswitch( 0x%02x )\n", data );
	if( data < 4 )
	{
		RAM = &RAM[0x2000 * data];
	}
	else
	{
		RAM = &RAM[0x10000 + 0x2000 * (data-4)];
	}
	memory_set_bankptr(1,RAM);
}

/******************************************************************************/

static WRITE8_HANDLER( cpu1_bankswitch_w )
{
	UINT8 *RAM = memory_region(REGION_CPU2);
	djboy_set_videoreg( data );
	switch( data&0xf )
	{
	/* bs65.5y */
	case 0x00: memory_set_bankptr(2,&RAM[0x00000]); break;
	case 0x01: memory_set_bankptr(2,&RAM[0x04000]); break;
	case 0x02: memory_set_bankptr(2,&RAM[0x10000]); break;
	case 0x03: memory_set_bankptr(2,&RAM[0x14000]); break;

	/* bs101.6w */
	case 0x08: memory_set_bankptr(2,&RAM[0x18000]); break;
	case 0x09: memory_set_bankptr(2,&RAM[0x1c000]); break;
	case 0x0a: memory_set_bankptr(2,&RAM[0x20000]); break;
	case 0x0b: memory_set_bankptr(2,&RAM[0x24000]); break;
	case 0x0c: memory_set_bankptr(2,&RAM[0x28000]); break;
	case 0x0d: memory_set_bankptr(2,&RAM[0x2c000]); break;
	case 0x0e: memory_set_bankptr(2,&RAM[0x30000]); break;
	case 0x0f: memory_set_bankptr(2,&RAM[0x34000]); break;

	default:
		break;
	}
}

static int prot_busy_count;
#define PROT_OUTPUT_BUFFER_SIZE 8
static UINT8 prot_output_buffer[PROT_OUTPUT_BUFFER_SIZE];
static int prot_available_data_count;
static int prot_offs; /* internal state */
static UINT8 prot_ram[0x80]; /* internal RAM */

static enum
{
	ePROT_NORMAL,
	ePROT_WRITE_BYTES,
	ePROT_WRITE_BYTE,
	ePROT_READ_BYTES,
	ePROT_WAIT_DSW1_WRITEBACK,
	ePROT_WAIT_DSW2_WRITEBACK,
	ePROT_STORE_PARAM
} prot_mode;

static void
ProtectionOut( int i, UINT8 data )
{
	if( prot_available_data_count == i )
	{
		prot_output_buffer[prot_available_data_count++] = data;
	}
	else
	{
		logerror( "prot_output_buffer overflow!\n" );
		exit(1);
	}
}

static void
OutputProtectionState( int i, int type )
{ /* 0..e */
	int dat = 0x82;
	ProtectionOut( i, dat );
}

static void
CommonProt( int i, int type )
{
	ProtectionOut( i++, 1/*credits*/ );
	ProtectionOut( i++, readinputport(0) ); /* COIN/START */
	OutputProtectionState( i, type );
}

static WRITE8_HANDLER( prot_data_w )
{
	prot_busy_count = 1;

	logerror( "0x%04x: prot_w(0x%02x)\n", activecpu_get_pc(), data );

	if( prot_mode == ePROT_WAIT_DSW1_WRITEBACK )
	{
		logerror( "[DSW1_WRITEBACK]\n" );
		ProtectionOut( 0, readinputport(4) ); /* DSW2 */
		prot_mode = ePROT_WAIT_DSW2_WRITEBACK;
	}
	else if( prot_mode == ePROT_WAIT_DSW2_WRITEBACK )
	{
		logerror( "[DSW2_WRITEBACK]\n" );
		prot_mode = ePROT_STORE_PARAM;
		prot_offs = 0;
	}
	else if( prot_mode == ePROT_STORE_PARAM )
	{
		logerror( "prot param[%d]: 0x%02x\n", prot_offs, data );
		prot_offs++;
		if( prot_offs == 8 )
		{
			prot_mode = ePROT_NORMAL;
		}
	}
	else if( prot_mode == ePROT_WRITE_BYTE )
	{ /* pc == 0x79cd */
		prot_ram[(prot_offs++)&0x7f] = data;
		prot_mode = ePROT_WRITE_BYTES;
	}
	else
	{
		switch( data )
		{
		case 0x00:
			if( prot_mode == ePROT_WRITE_BYTES )
			{ /* next byte is data to write to internal prot RAM */
				prot_mode = ePROT_WRITE_BYTE;
			}
			else if( prot_mode == ePROT_READ_BYTES )
			{ /* request next byte of internal prot RAM */
				ProtectionOut( 0, prot_ram[(prot_offs++)&0x7f] );
			}
			else
			{
				logerror( "UNEXPECTED PREFIX!\n" );
			}
			break;

		case 0x01: // pc=7389
			OutputProtectionState( 0, 0x01 );
			break;

		case 0x02:
			CommonProt( 0,0x02 );
			break;

		case 0x03: /* prepare for memory write to protection device ram (pc == 0x7987) */ // -> 0x02
			logerror( "[WRITE BYTES]\n" );
			prot_mode = ePROT_WRITE_BYTES;
			prot_offs = 0;
			break;

		case 0x04:
			ProtectionOut( 0,0 ); // ?
			ProtectionOut( 1,0 ); // ?
			ProtectionOut( 2,0 ); // ?
			ProtectionOut( 3,0 ); // ?
			CommonProt(    4,0x04 );
			break;

		case 0x05: /* 0x71f4 */
			ProtectionOut( 0,readinputport(1) ); // to $42
			ProtectionOut( 1,0 ); // ?
			ProtectionOut( 2,readinputport(2) ); // to $43
			ProtectionOut( 3,0 ); // ?
			ProtectionOut( 4,0 ); // ?
			CommonProt(    5,0x05 );
			break;

		case 0x07:
			CommonProt( 0,0x07 );
			break;

		case 0x08: /* pc == 0x727a */
			ProtectionOut( 0,readinputport(0) ); /* COIN/START */
			ProtectionOut( 1,readinputport(1) ); /* JOY1 */
			ProtectionOut( 2,readinputport(2) ); /* JOY2 */
			ProtectionOut( 3,readinputport(3) ); /* DSW1 */
			ProtectionOut( 4,readinputport(4) ); /* DSW2 */
			CommonProt(    5, 0x08 );
			break;

		case 0x09:
			ProtectionOut( 0,0 ); // ?
			ProtectionOut( 1,0 ); // ?
			ProtectionOut( 2,0 ); // ?
			CommonProt(    3, 0x09 );
			break;

		case 0x0a:
			CommonProt( 0,0x0a );
			break;

		case 0x0c:
			CommonProt( 1,0x0c );
			break;

		case 0x0d:
			CommonProt( 2,0x0d );
			break;

		case 0xfe: /* prepare for memory read from protection device ram (pc == 0x79ee, 0x7a3f) */
			if( prot_mode == ePROT_WRITE_BYTES )
			{
				prot_mode = ePROT_READ_BYTES;
				logerror( "[READ BYTES]\n" );
			}
			else
			{
				prot_mode = ePROT_WRITE_BYTES;
				logerror( "[WRITE BYTES*]\n" );
			}
			prot_offs = 0;
			break;

		case 0xff: /* read DSW (pc == 0x714d) */
			ProtectionOut( 0,readinputport(3) ); /* DSW1 */
			prot_mode = ePROT_WAIT_DSW1_WRITEBACK;
			break;

		case 0x92:
		case 0x97:
		case 0x9a:
		case 0xa3:
		case 0xa5:
		case 0xa9:
		case 0xad:
		case 0xb0:
		case 0xb3:
		case 0xb7:
			//ix+$60
			break;

		default:
			logerror( "UNKNOWN PROT_W!\n" );
			exit(1);
			break;
		}
	}
} /* prot_data_w */

static READ8_HANDLER( prot_data_r )
{ /* port#4 */
	UINT8 data = 0x00;
	if( prot_available_data_count )
	{
		int i;
		data = prot_output_buffer[0];
		prot_available_data_count--;
		for( i=0; i<prot_available_data_count; i++ )
		{
			prot_output_buffer[i] = prot_output_buffer[i+1];
		}
	}
	else
	{
		logerror( "prot_r: data expected!\n" );
	}
	logerror( "0x%04x: prot_r() == 0x%02x\n", activecpu_get_pc(), data );
	return data;
} /* prot_data_r */

static READ8_HANDLER( prot_status_r )
{ /* port 0xc */
	UINT8 result = 0;
//  logerror( "0x%04x: prot_status_r\n", activecpu_get_pc() );
	if( prot_busy_count )
	{
		prot_busy_count--;
		result |= 1<<3; /* 0x8 */
	}
	if( !prot_available_data_count )
	{
		result |= 1<<2; /* 0x4 */
	}
	return result;
}

/******************************************************************************/

static WRITE8_HANDLER( trigger_nmi_on_sound_cpu2 )
{
	soundlatch_w(0,data);
	cpunum_set_input_line(2, INPUT_LINE_NMI, PULSE_LINE);
}

static WRITE8_HANDLER( cpu2_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU3);

	if( data<3 )
	{
		RAM = &RAM[0x04000 * data];
	}
	else
	{
		RAM = &RAM[0x10000 + 0x4000*(data-3)];
	}
	memory_set_bankptr(3,RAM);
}

/******************************************************************************/

static ADDRESS_MAP_START( cpu0_am, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0xb000, 0xbfff) AM_READ(MRA8_RAM) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram)
	AM_RANGE(0xc000, 0xdfff) AM_READ(MRA8_BANK1)
	AM_RANGE(0xe000, 0xefff) AM_READ(MRA8_RAM) AM_WRITE(MWA8_RAM) AM_BASE(&sharedram)
	AM_RANGE(0xf000, 0xf7ff) AM_READ(MRA8_RAM) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xf800, 0xffff) AM_READ(MRA8_RAM) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( cpu0_port_am, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(cpu0_bankswitch_w)
ADDRESS_MAP_END

/******************************************************************************/

static ADDRESS_MAP_START( cpu1_am, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_BANK2)
	AM_RANGE(0xc000, 0xcfff) AM_READ(MRA8_RAM) AM_WRITE(djboy_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0xd000, 0xd3ff) AM_READ(MRA8_RAM) AM_WRITE(djboy_paletteram_w) AM_BASE(&paletteram)
	AM_RANGE(0xd400, 0xd7ff) AM_READ(MRA8_RAM) AM_WRITE(MWA8_RAM) /* workram */
	AM_RANGE(0xd800, 0xd8ff) AM_READ(MRA8_RAM) AM_WRITE(MWA8_RAM) /* shared! */
	AM_RANGE(0xe000, 0xffff) AM_READ(sharedram_r) AM_WRITE(sharedram_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( cpu1_port_am, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(cpu1_bankswitch_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(trigger_nmi_on_sound_cpu2)
	AM_RANGE(0x04, 0x04) AM_READ(prot_data_r) AM_WRITE(prot_data_w)
	AM_RANGE(0x06, 0x06) AM_WRITE(djboy_scrolly_w)
	AM_RANGE(0x08, 0x08) AM_WRITE(djboy_scrollx_w)
	AM_RANGE(0x0a, 0x0a) AM_WRITE(trigger_nmi_on_cpu0)
	AM_RANGE(0x0c, 0x0c) AM_READ(prot_status_r)
ADDRESS_MAP_END

/******************************************************************************/

static ADDRESS_MAP_START( cpu2_am, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_BANK3)
	AM_RANGE(0xc000, 0xdfff) AM_READ(MRA8_RAM) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( cpu2_port_am, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(cpu2_bankswitch_w)
	AM_RANGE(0x02, 0x02) AM_READ(YM2203_status_port_0_r) AM_WRITE(YM2203_control_port_0_w)
	AM_RANGE(0x03, 0x03) AM_READ(YM2203_read_port_0_r) AM_WRITE(YM2203_write_port_0_w)
	AM_RANGE(0x04, 0x04) AM_READ(soundlatch_r)
	AM_RANGE(0x06, 0x06) AM_READ(OKIM6295_status_0_r) AM_WRITE(OKIM6295_data_0_w)
	AM_RANGE(0x07, 0x07) AM_READ(OKIM6295_status_1_r) AM_WRITE(OKIM6295_data_1_w)
ADDRESS_MAP_END

/******************************************************************************/

static const gfx_layout tile_layout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{
		0*4,1*4,2*4,3*4,4*4,5*4,6*4,7*4,
		8*32+0*4,8*32+1*4,8*32+2*4,8*32+3*4,8*32+4*4,8*32+5*4,8*32+6*4,8*32+7*4
	},
	{
		0*32,1*32,2*32,3*32,4*32,5*32,6*32,7*32,
		16*32+0*32,16*32+1*32,16*32+2*32,16*32+3*32,16*32+4*32,16*32+5*32,16*32+6*32,16*32+7*32
	},
	4*8*32
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tile_layout, 0x000, 16 }, /* foreground tiles? */
	{ REGION_GFX2, 0, &tile_layout, 0x100, 16 }, /* sprite */
	{ REGION_GFX3, 0, &tile_layout, 0x000, 16 }, /* background tiles */
	{ -1 }
};

/******************************************************************************/

static INTERRUPT_GEN( djboy_interrupt )
{
	/* CPU1 uses interrupt mode 2.
     * For now, just alternate the two interrupts.  It isn't known what triggers them
     */
	static int addr = 0xff;
	addr ^= 0x02;
	cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, addr);
}

static MACHINE_DRIVER_START( djboy )
	MDRV_CPU_ADD(Z80,6000000) /* ? */
	MDRV_CPU_PROGRAM_MAP(cpu0_am,0)
	MDRV_CPU_IO_MAP(cpu0_port_am,0)
	MDRV_CPU_VBLANK_INT(djboy_interrupt,2)

	MDRV_CPU_ADD(Z80,6000000) /* ? */
	MDRV_CPU_PROGRAM_MAP(cpu1_am,0)
	MDRV_CPU_IO_MAP(cpu1_port_am,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 6000000) /* ? */
	MDRV_CPU_PROGRAM_MAP(cpu2_am,0)
	MDRV_CPU_IO_MAP(cpu2_port_am,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_SCREEN_VISIBLE_AREA(0, 256-1, 16, 256-16-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x200)

	MDRV_VIDEO_START(djboy)
	MDRV_VIDEO_UPDATE(djboy)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 3000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.80)

	MDRV_SOUND_ADD(OKIM6295, 8000000/4)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1_pin7low)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_ADD(OKIM6295, 8000000/4)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1_pin7low)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

ROM_START( djboy )
	ROM_REGION( 0x48000, REGION_CPU1, 0 )
	ROM_LOAD( "bs64.4b",  0x00000, 0x08000, CRC(b77aacc7) SHA1(78100d4695738a702f13807526eb1bcac759cce3) )
	ROM_CONTINUE( 0x10000, 0x18000 )
	ROM_LOAD( "bs100.4d", 0x28000, 0x20000, CRC(081e8af8) SHA1(3589dab1cf31b109a40370b4db1f31785023e2ed) )

	ROM_REGION( 0x38000, REGION_CPU2, 0 )
	ROM_LOAD( "bs65.5y",  0x00000, 0x08000, CRC(0f1456eb) SHA1(62ed48c0d71c1fabbb3f6ada60381f57f692cef8) )
	ROM_CONTINUE( 0x10000, 0x08000 )
	ROM_LOAD( "bs101.6w", 0x18000, 0x20000, CRC(a7c85577) SHA1(8296b96d5f69f6c730b7ed77fa8c93496b33529c) )

	ROM_REGION( 0x24000, REGION_CPU3, 0 ) /* sound */
	ROM_LOAD( "bs200.8c", 0x00000, 0x0c000, CRC(f6c19e51) SHA1(82193f71122df07cce0a7f057a87b89eb2d587a1) )
	ROM_CONTINUE( 0x10000, 0x14000 )

	ROM_REGION( 0x10000, REGION_GFX1, 0 ) /* foreground tiles?  alt sprite bank? */
	ROM_LOAD( "bs07.1b", 0x000000, 0x10000, CRC(d9b7a220) SHA1(ba3b528d50650c209c986268bb29b42ff1276eb2) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD( "bs000.1h", 0x000000, 0x80000, CRC(be4bf805) SHA1(a73c564575fe89d26225ca8ec2d98b6ac319ac18) )
	ROM_LOAD( "bs001.1f", 0x080000, 0x80000, CRC(fdf36e6b) SHA1(a8762458dfd5201304247c113ceb85e96e33d423) )
	ROM_LOAD( "bs002.1d", 0x100000, 0x80000, CRC(c52fee7f) SHA1(bd33117f7a57899fd4ec0a77413107edd9c44629) )
	ROM_LOAD( "bs003.1k", 0x180000, 0x80000, CRC(ed89acb4) SHA1(611af362606b73cd2cf501678b463db52dcf69c4) )

	ROM_REGION( 0x100000, REGION_GFX3, 0 ) /* background */
	ROM_LOAD( "bs004.1s", 0x000000, 0x80000, CRC(2f1392c3) SHA1(1bc3030b3612766a02133eef0b4d20013c0495a4) )
	ROM_LOAD( "bs005.1u", 0x080000, 0x80000, CRC(46b400c4) SHA1(35f4823364bbff1fc935994498d462bbd3bc6044) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* OKI-M6295 samples */
	ROM_LOAD( "bs203.5j", 0x000000, 0x40000, CRC(805341fb) SHA1(fb94e400e2283aaa806814d5a39d6196457dc822) )
ROM_END

ROM_START( djboyj )
	ROM_REGION( 0x48000, REGION_CPU1, 0 )
	ROM_LOAD( "bs12.4b",  0x00000, 0x08000, CRC(0971523e) SHA1(f90cd02cedf8632f4b651de7ea75dc8c0e682f6e) )
	ROM_CONTINUE( 0x10000, 0x18000 )
	ROM_LOAD( "bs100.4d", 0x28000, 0x20000, CRC(081e8af8) SHA1(3589dab1cf31b109a40370b4db1f31785023e2ed) )

	ROM_REGION( 0x38000, REGION_CPU2, 0 )
	ROM_LOAD( "bs13.5y",  0x00000, 0x08000, CRC(5c3f2f96) SHA1(bb7ee028a2d8d3c76a78a29fba60bcc36e9399f5) )
	ROM_CONTINUE( 0x10000, 0x08000 )
	ROM_LOAD( "bs101.6w", 0x18000, 0x20000, CRC(a7c85577) SHA1(8296b96d5f69f6c730b7ed77fa8c93496b33529c) )

	ROM_REGION( 0x24000, REGION_CPU3, 0 ) /* sound */
	ROM_LOAD( "bs200.8c", 0x00000, 0x0c000, CRC(f6c19e51) SHA1(82193f71122df07cce0a7f057a87b89eb2d587a1) )
	ROM_CONTINUE( 0x10000, 0x14000 )

	ROM_REGION( 0x10000, REGION_GFX1, 0 ) /* foreground tiles?  alt sprite bank? */
	ROM_LOAD( "bsxx.1b", 0x000000, 0x10000, CRC(22c8aa08) SHA1(5521c9d73b4ee82a2de1992d6edc7ef62788ad72) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD( "bs000.1h", 0x000000, 0x80000, CRC(be4bf805) SHA1(a73c564575fe89d26225ca8ec2d98b6ac319ac18) )
	ROM_LOAD( "bs001.1f", 0x080000, 0x80000, CRC(fdf36e6b) SHA1(a8762458dfd5201304247c113ceb85e96e33d423) )
	ROM_LOAD( "bs002.1d", 0x100000, 0x80000, CRC(c52fee7f) SHA1(bd33117f7a57899fd4ec0a77413107edd9c44629) )
	ROM_LOAD( "bs003.1k", 0x180000, 0x80000, CRC(ed89acb4) SHA1(611af362606b73cd2cf501678b463db52dcf69c4) )

	ROM_REGION( 0x100000, REGION_GFX3, 0 ) /* background */
	ROM_LOAD( "bs004.1s", 0x000000, 0x80000, CRC(2f1392c3) SHA1(1bc3030b3612766a02133eef0b4d20013c0495a4) )
	ROM_LOAD( "bs005.1u", 0x080000, 0x80000, CRC(46b400c4) SHA1(35f4823364bbff1fc935994498d462bbd3bc6044) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* OKI-M6295 samples */
	ROM_LOAD( "bs-204.5j", 0x000000, 0x40000, CRC(510244f0) SHA1(afb502d46d268ad9cd209ae1da72c50e4e785626) )
ROM_END



INPUT_PORTS_START( djboy )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* labeled "TEST" in self test */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) /* punch */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) /* kick */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) /* jump */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Service_Mode ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) ) /* coin mode? */
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x0c, 0x0c, "Bonus" )
	PORT_DIPSETTING(    0x0c, "10k,30k,50k,70k,90k" )
	PORT_DIPSETTING(    0x08, "10k,20k,30k,40k,50k,60k,70k,80k,90k" )
	PORT_DIPSETTING(    0x04, "20k,50k" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPSETTING(    0x30, "5" )
	PORT_DIPSETTING(    0x10, "7" )
	PORT_DIPSETTING(    0x00, "9" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Stero Sound" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/*     YEAR, NAME,  PARENT, MACHINE, INPUT, INIT, MNTR,  COMPANY, FULLNAME, FLAGS */
GAME( 1989, djboy,  0,      djboy,   djboy, 0,    ROT0, "Sammy / Williams [Kaneko]", "DJ Boy", GAME_NOT_WORKING ) // Sammy & Williams logos in FG ROM
GAME( 1989, djboyj, djboy,  djboy,   djboy, 0,    ROT0, "Sega [Kaneko]", "DJ Boy (Japan)", GAME_NOT_WORKING ) // Sega logo in FG ROM
