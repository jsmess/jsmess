/***************************************************************************

Atari Triple Hunt video emulation

***************************************************************************/

#include "driver.h"
#include "triplhnt.h"

extern void triplhnt_hit_callback(int);

UINT8* triplhnt_playfield_ram;
UINT8* triplhnt_hpos_ram;
UINT8* triplhnt_vpos_ram;
UINT8* triplhnt_code_ram;
UINT8* triplhnt_orga_ram;

int triplhnt_sprite_zoom;
int triplhnt_sprite_bank;

static mame_bitmap* helper;
static tilemap* bg_tilemap;


static UINT32 get_memory_offset(UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows)
{
	return num_cols * row + col;
}


static void get_tile_info(int tile_index)
{
	int code = triplhnt_playfield_ram[tile_index] & 0x3f;

	SET_TILE_INFO(2, code, code == 0x3f ? 1 : 0, 0)
}


VIDEO_START( triplhnt )
{
	helper = auto_bitmap_alloc(Machine->screen[0].width, Machine->screen[0].height, Machine->screen[0].format);

	bg_tilemap = tilemap_create(get_tile_info, get_memory_offset, 0, 16, 16, 16, 16);

	return 0;
}


static void triplhnt_draw_sprites(mame_bitmap* bitmap, const rectangle* cliprect)
{
	int i;

	int hit_line = 999;
	int hit_code = 999;

	for (i = 0; i < 16; i++)
	{
		rectangle rect;

		int j = (triplhnt_orga_ram[i] & 15) ^ 15;

		/* software sorts sprites by x and stores order in orga RAM */

		int hpos = triplhnt_hpos_ram[j] ^ 255;
		int vpos = triplhnt_vpos_ram[j] ^ 255;
		int code = triplhnt_code_ram[j] ^ 255;

		if (hpos == 255)
		{
			continue;
		}

		/* sprite placement might be wrong */

		if (triplhnt_sprite_zoom)
		{
			rect.min_x = hpos - 16;
			rect.min_y = 196 - vpos;
			rect.max_x = rect.min_x + 63;
			rect.max_y = rect.min_y + 63;
		}
		else
		{
			rect.min_x = hpos - 16;
			rect.min_y = 224 - vpos;
			rect.max_x = rect.min_x + 31;
			rect.max_y = rect.min_y + 31;
		}

		/* render sprite to auxiliary bitmap */

		drawgfx(helper, Machine->gfx[triplhnt_sprite_zoom],
			2 * code + triplhnt_sprite_bank, 0, code & 8, 0,
			rect.min_x, rect.min_y, cliprect, TRANSPARENCY_NONE, 0);

		if (rect.min_x < cliprect->min_x)
			rect.min_x = cliprect->min_x;
		if (rect.min_y < cliprect->min_y)
			rect.min_y = cliprect->min_y;
		if (rect.max_x > cliprect->max_x)
			rect.max_x = cliprect->max_x;
		if (rect.max_y > cliprect->max_y)
			rect.max_y = cliprect->max_y;

		/* check for collisions and copy sprite */

		{
			int x;
			int y;

			for (x = rect.min_x; x <= rect.max_x; x++)
			{
				for (y = rect.min_y; y <= rect.max_y; y++)
				{
					pen_t a = read_pixel(helper, x, y);
					pen_t b = read_pixel(bitmap, x, y);

					if (a == 2 && b == 7)
					{
						hit_code = j;
						hit_line = y;
					}

					if (a != 1)
					{
						plot_pixel(bitmap, x, y, a);
					}
				}
			}
		}
	}

	if (hit_line != 999 && hit_code != 999)
	{
		timer_set(cpu_getscanlinetime(hit_line), hit_code, triplhnt_hit_callback);
	}
}


VIDEO_UPDATE( triplhnt )
{
	tilemap_mark_all_tiles_dirty(bg_tilemap);

	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);

	triplhnt_draw_sprites(bitmap, cliprect);

	discrete_sound_w(TRIPLHNT_BEAR_ROAR_DATA, triplhnt_playfield_ram[0xfa] & 15);
	discrete_sound_w(TRIPLHNT_SHOT_DATA, triplhnt_playfield_ram[0xfc] & 15);
	return 0;
}
