/***************************************************************************

Skeleton driver file for Super A'Can console.

The system unit contains a reset button.

Controllers:
- 4 directional buttons
- A, B, X, Y, buttons
- Start, select buttons
- L, R shoulder buttons

f00000 - bit 15 is probably vblank bit.

Known unemulated graphical effects:
- The green blob of the A'Can logo should slide out from beneath the word
  "A'Can"
- The A'Can logo should have a scrolling ROZ layer beneath it
- Sprites

***************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "cpu/m6502/m6502.h"
#include "devices/cartslot.h"

static UINT16 supracan_m6502_reset;
static UINT8 *supracan_soundram;
static UINT16 *supracan_soundram_16;
static UINT16 supracan_video_regs[256];
static emu_timer *supracan_video_timer;
static UINT16 *supracan_charram, *supracan_sprcharram;
static UINT16 *supracan_vram;
static UINT16 *supracan_rram;
static UINT16 *supracan_rozpal;

static READ16_HANDLER( supracan_unk1_r );
static WRITE16_HANDLER( supracan_soundram_w );
static WRITE16_HANDLER( supracan_sound_w );
static READ16_HANDLER( supracan_video_r );
static WRITE16_HANDLER( supracan_video_w );
static WRITE16_HANDLER( supracan_char_w );

#define VERBOSE_LEVEL	(99)

#define ENABLE_VERBOSE_LOG (1)

#if ENABLE_VERBOSE_LOG
INLINE void verboselog(running_machine *machine, int n_level, const char *s_fmt, ...)
{
	if( VERBOSE_LEVEL >= n_level )
	{
		va_list v;
		char buf[ 32768 ];
		va_start( v, s_fmt );
		vsprintf( buf, s_fmt, v );
		va_end( v );
		logerror( "%06x: %s", cpu_get_pc(cputag_get_cpu(machine, "maincpu")), buf );
	}
}
#else
#define verboselog(x,y,z,...)
#endif

static VIDEO_START( supracan )
{

}

static VIDEO_UPDATE( supracan )
{
	int x,y,count;
	const gfx_element *gfx = screen->machine->gfx[0];

	count = 0;

	for (y=0;y<64;y++)
	{
		for (x=0;x<32;x += 8)
		{
			static UINT16 dot_data;

			/* FIXME: likely to be clutted */
			dot_data = supracan_rram[count] ^ 0xffff;

			*BITMAP_ADDR16(bitmap, y, 128+x+0) = ((dot_data >> 14) & 0x0003) + 0x20;
			*BITMAP_ADDR16(bitmap, y, 128+x+1) = ((dot_data >> 12) & 0x0003) + 0x20;
			*BITMAP_ADDR16(bitmap, y, 128+x+2) = ((dot_data >> 10) & 0x0003) + 0x20;
			*BITMAP_ADDR16(bitmap, y, 128+x+3) = ((dot_data >>  8) & 0x0003) + 0x20;
			*BITMAP_ADDR16(bitmap, y, 128+x+4) = ((dot_data >>  6) & 0x0003) + 0x20;
			*BITMAP_ADDR16(bitmap, y, 128+x+5) = ((dot_data >>  4) & 0x0003) + 0x20;
			*BITMAP_ADDR16(bitmap, y, 128+x+6) = ((dot_data >>  2) & 0x0003) + 0x20;
			*BITMAP_ADDR16(bitmap, y, 128+x+7) = ((dot_data >>  0) & 0x0003) + 0x20;
			count++;
		}
	}

	count = 0;

	for (y=0;y<16;y++)
	{
		for (x=0;x<16;x++)
		{
			int tile;

			tile = (supracan_vram[count] & 0x00ff);
			drawgfx_opaque(bitmap,cliprect,gfx,tile,0,0,0,x*8,y*8);
			count++;
		}
	}

	return 0;
}

typedef struct
{
	UINT32 source;
	UINT32 dest;
	UINT16 count;
	UINT16 control;
} _acan_dma_regs_t;

static _acan_dma_regs_t acan_dma_regs;

static WRITE16_HANDLER( supracan_dma_w )
{
	int i;
	switch(offset)
	{
		case 0x00/2: // Source address MSW
			acan_dma_regs.source &= 0x0000ffff;
			acan_dma_regs.source |= data << 16;
			break;
		case 0x02/2: // Source address LSW
			acan_dma_regs.source &= 0xffff0000;
			acan_dma_regs.source |= data;
			break;
		case 0x04/2: // Destination address MSW
			acan_dma_regs.dest &= 0x0000ffff;
			acan_dma_regs.dest |= data << 16;
			break;
		case 0x06/2: // Destination address LSW
			acan_dma_regs.dest &= 0xffff0000;
			acan_dma_regs.dest |= data;
			break;
		case 0x08/2: // Byte count
			acan_dma_regs.count = data;
			break;
		case 0x0a/2: // Control
			//if(acan_dma_regs.dest != 0xf00200)
			//	printf("%08x %08x %02x %04x\n",acan_dma_regs.source,acan_dma_regs.dest,acan_dma_regs.count + 1,data);
			if(data & 0x8800)
			{
				verboselog(space->machine, 0, "supracan_dma_w: Kicking off a DMA from %08x to %08x, %d bytes\n", acan_dma_regs.source, acan_dma_regs.dest, acan_dma_regs.count + 1);
				for(i = 0; i <= acan_dma_regs.count; i++)
				{
					if(data & 0x1000)
						memory_write_word(space, acan_dma_regs.dest+=2, memory_read_word(space, acan_dma_regs.source+=2));
					else
						memory_write_byte(space, acan_dma_regs.dest++, memory_read_byte(space, acan_dma_regs.source++));
				}
			}
			else
			{
				verboselog(space->machine, 0, "supracan_dma_w: Unknown DMA kickoff value of %04x (other regs %08x, %08x, %d)\n", data, acan_dma_regs.source, acan_dma_regs.dest, acan_dma_regs.count + 1);
			}
			break;
	}
}

static WRITE16_HANDLER( supracan_sprchar_w )
{
	COMBINE_DATA(&supracan_sprcharram[offset]);

	{
		UINT8 *gfx = memory_region(space->machine, "spr_gfx");

		gfx[offset*2+0] = (supracan_sprcharram[offset] & 0xff00) >> 8;
		gfx[offset*2+1] = (supracan_sprcharram[offset] & 0x00ff) >> 0;

		gfx_element_mark_dirty(space->machine->gfx[1], offset/32);
	}
}

static ADDRESS_MAP_START( supracan_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE( 0x000000, 0x07ffff ) AM_ROM AM_REGION( "cart", 0 )
	AM_RANGE( 0xe80204, 0xe80205 ) AM_READ( supracan_unk1_r )
	AM_RANGE( 0xe80300, 0xe80301 ) AM_READ( supracan_unk1_r )
	AM_RANGE( 0xe88400, 0xe8efff ) AM_RAM	/* sample table + sample data */
	AM_RANGE( 0xe8f000, 0xe8ffff ) AM_RAM_WRITE( supracan_soundram_w ) AM_BASE( &supracan_soundram_16 )
	AM_RANGE( 0xe90000, 0xe9001f ) AM_WRITE( supracan_sound_w )
	AM_RANGE( 0xe90020, 0xe9002b ) AM_WRITE( supracan_dma_w )
	AM_RANGE( 0xf00000, 0xf001ff ) AM_READWRITE( supracan_video_r, supracan_video_w )
	AM_RANGE( 0xf00200, 0xf003ff ) AM_RAM_WRITE(paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE_GENERIC(paletteram)
	AM_RANGE( 0xf40000, 0xf43fff ) AM_RAM_WRITE(supracan_char_w) AM_BASE(&supracan_charram)	/* Unknown, some data gets copied here during boot */
	AM_RANGE( 0xf44000, 0xf441ff ) AM_RAM AM_BASE(&supracan_rozpal) /* Unknown */
	AM_RANGE( 0xf44200, 0xf443ff ) AM_RAM AM_BASE(&supracan_vram)	/* Foreground tilemap layer */
	AM_RANGE( 0xf44400, 0xf445ff ) AM_RAM AM_BASE(&supracan_rram)	/* ROZ layer tiles */
	AM_RANGE( 0xf44600, 0xf44fff ) AM_RAM	/* Unknown, stuff gets written here. Zoom table?? */
	AM_RANGE( 0xfc0000, 0xfcffff ) AM_RAM_WRITE( supracan_sprchar_w ) AM_BASE( &supracan_sprcharram )	/* System work ram */
	AM_RANGE( 0xff8000, 0xffffff ) AM_RAM	/* System work ram */
ADDRESS_MAP_END


static ADDRESS_MAP_START( supracan_sound_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE( 0x0000, 0x03ff ) AM_RAM
	AM_RANGE( 0xf000, 0xffff ) AM_RAM AM_BASE( &supracan_soundram )
ADDRESS_MAP_END

static WRITE16_HANDLER( supracan_char_w )
{
	COMBINE_DATA(&supracan_charram[offset]);

	{
		UINT8 *gfx = memory_region(space->machine, "ram_gfx");

		gfx[offset*2+0] = (supracan_charram[offset] & 0xff00) >> 8;
		gfx[offset*2+1] = (supracan_charram[offset] & 0x00ff) >> 0;

		gfx_element_mark_dirty(space->machine->gfx[0], offset/16);
	}
}

static INPUT_PORTS_START( supracan )
INPUT_PORTS_END


static PALETTE_INIT( supracan )
{
	// Used for debugging purposes for now
	//#if 0
	int i, r, g, b;

	for( i = 0; i < 32768; i++ )
	{
		r = (i & 0x1f) << 3;
		g = ((i >> 5) & 0x1f) << 3;
		b = ((i >> 10) & 0x1f) << 3;
		palette_set_color_rgb( machine, i, r, g, b );
	}
	//#endif
}


static WRITE16_HANDLER( supracan_soundram_w )
{
	supracan_soundram_16[offset] = data;

	supracan_soundram[offset*2 + 1] = data & 0xff;
	supracan_soundram[offset*2 + 0] = data >> 8;
}


static READ16_HANDLER( supracan_unk1_r )
{
	/* No idea what hardware this is connected to. */
	return 0xffff;
}


static WRITE16_HANDLER( supracan_sound_w )
{
	switch ( offset )
	{
	case 0x001c/2:	/* Sound cpu control. Bit 0 tied to sound cpu RESET line */
		if ( data & 0x01 )
		{
			if ( ! supracan_m6502_reset )
			{
				/* Reset and enable the sound cpu */
				cputag_set_input_line(space->machine, "soundcpu", INPUT_LINE_HALT, CLEAR_LINE);
				cputag_reset( space->machine, "soundcpu" );
			}
		}
		else
		{
			/* Halt the sound cpu */
			cputag_set_input_line(space->machine, "soundcpu", INPUT_LINE_HALT, ASSERT_LINE);
		}
		supracan_m6502_reset = data & 0x01;
		break;
	default:
		logerror("supracan_sound_w: writing %04x to unknown address %08x\n", data, 0xe90000 + offset * 2 );
		break;
	}
}


static READ16_HANDLER( supracan_video_r )
{
	UINT16 data = supracan_video_regs[offset];

	switch(offset)
	{
		case 0: // VBlank flag
			break;
		default:
			verboselog(space->machine, 0, "supracan_video_r: Unknown register: %08x (%04x & %04x)\n", 0xf0000 + (offset << 1), data, mem_mask);
			break;
	}

	return data;
}


static TIMER_CALLBACK( supracan_video_callback )
{
	int vpos = video_screen_get_vpos(machine->primary_screen);

	switch( vpos )
	{
	case 0:
		supracan_video_regs[0] &= 0x7fff;
		break;
	case 240:
		supracan_video_regs[0] |= 0x8000;
		break;
	}

	timer_adjust_oneshot( supracan_video_timer, video_screen_get_time_until_pos( machine->primary_screen, ( vpos + 1 ) & 0xff, 0 ), 0 );
}


static WRITE16_HANDLER( supracan_video_w )
{
	switch(offset)
	{
		default:
			verboselog(space->machine, 0, "supracan_video_w: Unknown register: %08x = %04x & %04x\n", 0xf0000 + (offset << 1), data, mem_mask);
			break;
	}
	supracan_video_regs[offset] = data;
}


static DEVICE_IMAGE_LOAD( supracan_cart )
{
	UINT8 *cart = memory_region( image->machine, "cart" );
	int size = image_length( image );

	if ( size != 0x80000 )
	{
		image_seterror( image, IMAGE_ERROR_UNSPECIFIED, "Unsupported cartridge size" );
		return INIT_FAIL;
	}

	if ( image_fread( image, cart, size ) != size )
	{
		image_seterror( image, IMAGE_ERROR_UNSPECIFIED, "Unable to fully read from file" );
		return INIT_FAIL;
	}

	return INIT_PASS;
}


static MACHINE_START( supracan )
{
	supracan_video_timer = timer_alloc( machine, supracan_video_callback, NULL );
}


static MACHINE_RESET( supracan )
{
	cputag_set_input_line(machine, "soundcpu", INPUT_LINE_HALT, ASSERT_LINE);
	timer_adjust_oneshot( supracan_video_timer, video_screen_get_time_until_pos( machine->primary_screen, 0, 0 ), 0 );
}

static const gfx_layout supracan_gfxlayout =
{
	8,8,
	RGN_FRAC(1,1),
	8,
	{ STEP8(0,1) },
	{ STEP8(0,8) },
	{ STEP8(0,8*8) },
	8*8*8
};

static const gfx_layout supracan_sprlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	8*32
};

static GFXDECODE_START( supracan )
	GFXDECODE_ENTRY( "ram_gfx",  0, supracan_gfxlayout,   0, 1 )
	GFXDECODE_ENTRY( "spr_gfx",  0, supracan_sprlayout,   0, 2 )
GFXDECODE_END

static MACHINE_DRIVER_START( supracan )
	MDRV_CPU_ADD( "maincpu", M68000, XTAL_10_738635MHz )		/* Correct frequency unknown */
	MDRV_CPU_PROGRAM_MAP( supracan_mem )
	MDRV_CPU_VBLANK_INT("screen", irq7_line_hold)

	MDRV_CPU_ADD( "soundcpu", M6502, XTAL_3_579545MHz )		/* TODO: Verfiy actual clock */
	MDRV_CPU_PROGRAM_MAP( supracan_sound_mem )

	MDRV_MACHINE_START( supracan )
	MDRV_MACHINE_RESET( supracan )

	MDRV_SCREEN_ADD( "screen", RASTER )
	MDRV_SCREEN_FORMAT( BITMAP_FORMAT_INDEXED16 )
	MDRV_SCREEN_RAW_PARAMS(XTAL_10_738635MHz/2, 348, 0, 320, 256, 0, 240 )	/* No idea if this is correct */

	MDRV_PALETTE_LENGTH( 32768 )
	MDRV_PALETTE_INIT( supracan )
	MDRV_GFXDECODE( supracan )

	MDRV_CARTSLOT_ADD( "cart" )
	MDRV_CARTSLOT_EXTENSION_LIST( "bin" )
	MDRV_CARTSLOT_MANDATORY
	MDRV_CARTSLOT_LOAD( supracan_cart )

	MDRV_VIDEO_START( supracan )
	MDRV_VIDEO_UPDATE( supracan )
MACHINE_DRIVER_END


ROM_START( supracan )
	ROM_REGION( 0x80000, "cart", ROMREGION_ERASEFF )

	ROM_REGION( 0x4000, "ram_gfx", ROMREGION_ERASEFF )

	ROM_REGION( 0x8000, "spr_gfx", ROMREGION_ERASEFF )

ROM_END


/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT     INIT    COMPANY                  FULLNAME        FLAGS */
CONS( 1995, supracan,   0,      0,      supracan,   supracan, 0,      "Funtech Entertainment", "Super A'Can",  GAME_NOT_WORKING )


