/***************************************************************************

Skeleton driver file for Super A'Can console.

The system unit contains a reset button.

Controllers:
- 4 directional buttons
- A, B, X, Y, buttons
- Start, select buttons
- L, R shoulder buttons

f00000 - bit 15 is probably vblank bit.

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
static UINT16 *supracan_charram, *supracan_vram;

static READ16_HANDLER( supracan_unk1_r );
static WRITE16_HANDLER( supracan_soundram_w );
static WRITE16_HANDLER( supracan_sound_w );
static READ16_HANDLER( supracan_video_r );
static WRITE16_HANDLER( supracan_video_w );
static WRITE16_HANDLER( supracan_char_w );

static VIDEO_START( supracan )
{

}

static VIDEO_UPDATE( supracan )
{
	int x,y,count;
	const gfx_element *gfx = screen->machine->gfx[0];

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

static ADDRESS_MAP_START( supracan_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE( 0x000000, 0x07ffff ) AM_ROM AM_REGION( "cart", 0 )
	AM_RANGE( 0xe80204, 0xe80205 ) AM_READ( supracan_unk1_r )
	AM_RANGE( 0xe80300, 0xe80301 ) AM_READ( supracan_unk1_r )
	AM_RANGE( 0xe88400, 0xe88c87 ) AM_RAM	/* Unknown, some data gets copied here during boot */
	AM_RANGE( 0xe8f000, 0xe8ffff ) AM_RAM_WRITE( supracan_soundram_w ) AM_BASE( &supracan_soundram_16 )
	AM_RANGE( 0xe90000, 0xe9001f ) AM_WRITE( supracan_sound_w )
	AM_RANGE( 0xf00000, 0xf001ff ) AM_READWRITE( supracan_video_r, supracan_video_w )
	AM_RANGE( 0xf00200, 0xf003ff ) AM_RAM_WRITE(paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE_GENERIC(paletteram)
	AM_RANGE( 0xf40000, 0xf43fff ) AM_RAM_WRITE(supracan_char_w) AM_BASE(&supracan_charram)	/* Unknown, some data gets copied here during boot */
	AM_RANGE( 0xf44000, 0xf441ff ) AM_RAM	/* Unknown */
	AM_RANGE( 0xf44200, 0xf443ff ) AM_RAM AM_BASE(&supracan_vram)	/* Unknown, some data gets copied here during boot. Possibly a tilemap?? */
	AM_RANGE( 0xf44400, 0xf445ff ) AM_RAM	/* Unknown, some data gets copied here during boot. Looks like the Functech logo is here */
	AM_RANGE( 0xf44600, 0xf44fff ) AM_RAM	/* Unknown, stuff gets written here. Zoom table?? */
	AM_RANGE( 0xfc0000, 0xfcffff ) AM_RAM	/* System work ram */
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

		gfx_element_mark_dirty(space->machine->gfx[0], offset/32);
	}
}

static INPUT_PORTS_START( supracan )
INPUT_PORTS_END


static PALETTE_INIT( supracan )
{
	#if 0
	int i, r, g, b;

	for( i = 0; i < 32768; i++ )
	{
		/* TODO: Verfiy that we are taking the proper bits for r, g, and b. */
		r = (i & 0x1f) << 3;
		g = ((i >> 5) & 0x1f) << 3;
		b = ((i >> 10) & 0x1f) << 3;
		palette_set_color_rgb( machine, i, r, g, b );
	}
	#endif
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


static GFXDECODE_START( supracan )
	GFXDECODE_ENTRY( "ram_gfx", 0, supracan_gfxlayout,   0, 1 )
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
ROM_END


/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT     INIT    COMPANY                  FULLNAME        FLAGS */
CONS( 1995, supracan,   0,      0,      supracan,   supracan, 0,      "Funtech Entertainment", "Super A'Can",  GAME_NOT_WORKING )


