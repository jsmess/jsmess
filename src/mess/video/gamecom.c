#include "emu.h"
#include "includes/gamecom.h"

#define Y_PIXELS 200

static int scanline;
static unsigned int base_address;
static emu_timer *scanline_timer;

static TIMER_CALLBACK( gamecom_scanline ) {
	UINT8 * RAM = memory_region(machine, "maincpu");
	// draw line
	if ( scanline == 0 ) {
		base_address = ( RAM[SM8521_LCDC] & 0x40 ) ? 0x2000 : 0x0000;
	}
	if ( ~RAM[SM8521_LCDC] & 0x80 ) {
		rectangle rec;
		rec.min_x = 0;
		rec.max_x = Y_PIXELS - 1;
		rec.min_y = rec.max_y = scanline;
		bitmap_fill( machine->generic.tmpbitmap, &rec , 0);
		return;
	} else {
		UINT8 *line = &gamecom_vram[ base_address + 40 * scanline ];
		int	pal[4];
		int	i;

		switch( RAM[SM8521_LCDC] & 0x30 ) {
		case 0x00:
			pal[0] = 4;
			pal[1] = 3;
			pal[2] = 2;
			pal[3] = 0;
			break;
		case 0x10:
			pal[0] = 4;
			pal[1] = 3;
			pal[2] = 1;
			pal[3] = 0;
			break;
		case 0x20:
			pal[0] = 4;
			pal[1] = 3;
			pal[2] = 1;
			pal[3] = 0;
			break;
		case 0x30:
			pal[0] = 4;
			pal[1] = 2;
			pal[2] = 1;
			pal[3] = 0;
			break;
		}
		for( i = 0; i < 40; i++ ) {
			UINT8 p = line[i];
			*BITMAP_ADDR16(machine->generic.tmpbitmap, i * 4 + 0, scanline) = pal[ ( p >> 6 ) & 3 ];
			*BITMAP_ADDR16(machine->generic.tmpbitmap, i * 4 + 1, scanline) = pal[ ( p >> 4 ) & 3 ];
			*BITMAP_ADDR16(machine->generic.tmpbitmap, i * 4 + 2, scanline) = pal[ ( p >> 2 ) & 3 ];
			*BITMAP_ADDR16(machine->generic.tmpbitmap, i * 4 + 3, scanline) = pal[ ( p      ) & 3 ];
		}
	}

	scanline = ( scanline + 1 ) % Y_PIXELS;
}

VIDEO_START( gamecom )
{
	VIDEO_START_CALL( generic_bitmapped );
	scanline_timer = timer_alloc( machine, gamecom_scanline, NULL );
	timer_adjust_periodic( scanline_timer, machine->primary_screen->time_until_pos(0, 0 ), 0, machine->primary_screen->scan_period() );

}

