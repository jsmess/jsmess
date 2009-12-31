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


static READ16_HANDLER( supracan_unk1_r );
static WRITE16_HANDLER( supracan_soundram_w );
static WRITE16_HANDLER( supracan_sound_w );


static ADDRESS_MAP_START( supracan_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE( 0x000000, 0x07ffff ) AM_ROM AM_REGION( "cart", 0 )
	AM_RANGE( 0xe80300, 0xe80301 ) AM_READ( supracan_unk1_r )
	AM_RANGE( 0xe88400, 0xe88c87 ) AM_RAM	/* Unknown, some data gets copied here during boot */
	AM_RANGE( 0xe8f000, 0xe8ffff ) AM_RAM_WRITE( supracan_soundram_w ) AM_BASE( &supracan_soundram_16 )
	AM_RANGE( 0xe90000, 0xe9001f ) AM_WRITE( supracan_sound_w )
	AM_RANGE( 0xf00000, 0xf001ff ) AM_RAM	/* Unknown */
	AM_RANGE( 0xf00200, 0xf003ff ) AM_RAM	/* Unknown, some data gets copied here during boot */
	AM_RANGE( 0xf40000, 0xf43fff ) AM_RAM	/* Unknown, some data gets copied here during boot */
	AM_RANGE( 0xf44000, 0xf4401f ) AM_RAM	/* Unknown */
	AM_RANGE( 0xf44200, 0xf443ff ) AM_RAM	/* Unknown, some data gets copied here during boot. Possibly a tilemap?? */
	AM_RANGE( 0xf44400, 0xf445ff ) AM_RAM	/* Unknown, some data gets copied here during boot */
	AM_RANGE( 0xfc0000, 0xfcffff ) AM_RAM	/* System work ram */
	AM_RANGE( 0xff8000, 0xffffff ) AM_RAM	/* System work ram */
ADDRESS_MAP_END


static ADDRESS_MAP_START( supracan_sound_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE( 0x0000, 0x01ff ) AM_RAM
	AM_RANGE( 0xf000, 0xffff ) AM_RAM AM_BASE( &supracan_soundram )
ADDRESS_MAP_END


static INPUT_PORTS_START( supracan )
INPUT_PORTS_END


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


static MACHINE_RESET( supracan )
{
	cputag_set_input_line(machine, "soundcpu", INPUT_LINE_HALT, ASSERT_LINE);
}


static MACHINE_DRIVER_START( supracan )
	MDRV_CPU_ADD( "maincpu", M68000, XTAL_10_738635MHz )		/* Correct frequency unknown */
	MDRV_CPU_PROGRAM_MAP( supracan_mem )

	MDRV_CPU_ADD( "soundcpu", M6502, XTAL_3_579545MHz )		/* TODO: Verfiy actual clock */
	MDRV_CPU_PROGRAM_MAP( supracan_sound_mem )

	MDRV_MACHINE_RESET( supracan )

	MDRV_SCREEN_ADD( "screen", LCD )
	MDRV_SCREEN_FORMAT( BITMAP_FORMAT_RGB32 )
	MDRV_SCREEN_SIZE( 320, 240 )
	MDRV_SCREEN_VISIBLE_AREA( 0, 319, 0, 239 )
	MDRV_SCREEN_REFRESH_RATE( 60 )

	MDRV_CARTSLOT_ADD( "cart" )
	MDRV_CARTSLOT_EXTENSION_LIST( "bin" )
	MDRV_CARTSLOT_MANDATORY
	MDRV_CARTSLOT_LOAD( supracan_cart )
MACHINE_DRIVER_END


ROM_START( supracan )
	ROM_REGION( 0x80000, "cart", ROMREGION_ERASEFF )
ROM_END


/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT     INIT    COMPANY                  FULLNAME        FLAGS */
CONS( 1995, supracan,   0,      0,      supracan,   supracan, 0,      "Funtech Entertainment", "Super A'Can",  GAME_NOT_WORKING )


