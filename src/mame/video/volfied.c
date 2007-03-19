#include "driver.h"
#include "video/taitoic.h"

static mame_bitmap* pixel_layer = 0;

static UINT16* video_ram = NULL;

static UINT16 video_ctrl = 0;
static UINT16 video_mask = 0;

static UINT8* line_dirty = NULL;

static void mark_all_dirty(void)
{
	memset(line_dirty, 1, 256);
}


/******************************************************
          INITIALISATION AND CLEAN-UP
******************************************************/

VIDEO_START( volfied )
{
	pixel_layer = auto_bitmap_alloc(Machine->screen[0].width, Machine->screen[0].height, Machine->screen[0].format);

	line_dirty = auto_malloc(256);

	video_ram = auto_malloc(0x40000 * sizeof (UINT16));

	state_save_register_global_pointer(video_ram, 0x40000);
	state_save_register_global(video_ctrl);
	state_save_register_global(video_mask);

	state_save_register_func_postload (mark_all_dirty);

	return PC090OJ_vh_start(0, 0, 0, 0);
}


/*******************************************************
          READ AND WRITE HANDLERS
*******************************************************/

READ16_HANDLER( volfied_video_ram_r )
{
	return video_ram[offset];
}

WRITE16_HANDLER( volfied_video_ram_w )
{
	mem_mask |= ~video_mask;

	line_dirty[(offset >> 9) & 0xff] = 1;

	COMBINE_DATA(&video_ram[offset]);
}

WRITE16_HANDLER( volfied_video_ctrl_w )
{
	if (ACCESSING_LSB && (data & 1) != (video_ctrl & 1))
	{
		mark_all_dirty();    /* screen switch */
	}

	COMBINE_DATA(&video_ctrl);
}

READ16_HANDLER( volfied_video_ctrl_r )
{
	/* Could this be some kind of hardware collision detection? If bit 6 is
       set the game will check for collisions with the large enemy, whereas
       bit 5 does the same for small enemies. Bit 7 is also used although
       its purpose is unclear. This register is usually read during a VBI
       and stored in work RAM for later use. */

	return 0x60;
}

WRITE16_HANDLER( volfied_video_mask_w )
{
	COMBINE_DATA(&video_mask);
}

WRITE16_HANDLER( volfied_sprite_ctrl_w )
{
	PC090OJ_sprite_ctrl = (data & 0x3c) >> 2;
}


/*******************************************************
                SCREEN REFRESH
*******************************************************/

static void refresh_pixel_layer(void)
{
	int x, y;

	/*********************************************************

    VIDEO RAM has 2 screens x 256 rows x 512 columns x 16 bits

    x---------------  select image
    -x--------------  ?             (used for 3-D corners)
    --x-------------  ?             (used for 3-D walls)
    ---xxxx---------  image B
    -------xxx------  palette index bits #8 to #A
    ----------x-----  ?
    -----------x----  ?
    ------------xxxx  image A

    '3d' corners & walls are made using unknown bits for each
    line the player draws.  However, on the pcb they just
    appear as solid black. Perhaps it was prototype code that
    was turned off at some stage.

    *********************************************************/

	UINT16* p = video_ram;

	if (video_ctrl & 1)
	{
		p += 0x20000;
	}

	for (y = 0; y < Machine->screen[0].height; y++)
	{
		if (line_dirty[y])
		{
			for (x = 1; x < Machine->screen[0].width + 1; x++) // Hmm, 1 pixel offset is needed to align properly with sprites
			{
				int color = (p[x] << 2) & 0x700;

				if (p[x] & 0x8000)
				{
					color |= 0x800 | ((p[x] >> 9) & 0xf);

					if (p[x] & 0x2000)
					{
						color &= ~0xf;	  /* hack */
					}
				}
				else
				{
					color |= p[x] & 0xf;
				}

				plot_pixel(pixel_layer, x - 1, y, Machine->pens[color]);
			}

			line_dirty[y] = 0;
		}

		p += 512;
	}
}

VIDEO_UPDATE( volfied )
{
	fillbitmap(priority_bitmap, 0, cliprect);

	refresh_pixel_layer();

	copybitmap(bitmap, pixel_layer, 0, 0, 0, 0, cliprect, TRANSPARENCY_NONE, 0);

	PC090OJ_draw_sprites(bitmap, cliprect, 0);
	return 0;
}
