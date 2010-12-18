/***************************************************************************
 *  Microtan 65
 *
 *  video hardware
 *
 *  Juergen Buchmueller <pullmoll@t-online.de>, Jul 2000
 *
 *  Thanks go to Geoff Macdonald <mail@geoff.org.uk>
 *  for his site http:://www.geo255.redhotant.com
 *  and to Fabrice Frances <frances@ensica.fr>
 *  for his site http://www.ifrance.com/oric/microtan.html
 *
 ***************************************************************************/

#include "emu.h"
#include "includes/microtan.h"


UINT8 microtan_chunky_graphics = 0;
UINT8 *microtan_chunky_buffer = NULL;


WRITE8_HANDLER( microtan_videoram_w )
{
	microtan_state *state = space->machine->driver_data<microtan_state>();
	UINT8 *videoram = state->videoram;
	if ((videoram[offset] != data) || (state->chunky_buffer[offset] != state->chunky_graphics))
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(state->bg_tilemap, offset);
		state->chunky_buffer[offset] = state->chunky_graphics;
	}
}

static TILE_GET_INFO(get_bg_tile_info)
{
	microtan_state *state = machine->driver_data<microtan_state>();
	UINT8 *videoram = state->videoram;
	int gfxn = state->chunky_buffer[tile_index];
	int code = videoram[tile_index];

	SET_TILE_INFO(gfxn, code, 0, 0);
}

VIDEO_START( microtan )
{
	microtan_state *state = machine->driver_data<microtan_state>();
	state->bg_tilemap = tilemap_create(machine, get_bg_tile_info, tilemap_scan_rows,
		8, 16, 32, 16);

	state->chunky_buffer = auto_alloc_array(machine, UINT8, 0x200);
	memset(state->chunky_buffer, 0, 0x200);
	state->chunky_graphics = 0;
}

VIDEO_UPDATE( microtan )
{
	microtan_state *state = screen->machine->driver_data<microtan_state>();
	tilemap_draw(bitmap, cliprect, state->bg_tilemap, 0, 0);
	return 0;
}
