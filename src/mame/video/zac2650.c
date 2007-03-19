/*************************************************************/
/*                                                           */
/* Zaccaria/Zelco S2650 based games video                    */
/*                                                           */
/*************************************************************/

#include "driver.h"

UINT8 *s2636ram;
static mame_bitmap *spritebitmap;

static UINT8 dirtychar[256>>3];
static int CollisionBackground;
static int CollisionSprite;

static tilemap *bg_tilemap;


/**************************************************************/
/* The S2636 is a standard sprite chip used by several boards */
/* Emulation of this chip may be moved into a seperate unit   */
/* once it's workings are fully understood.                   */
/**************************************************************/

WRITE8_HANDLER( tinvader_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE8_HANDLER( zac_s2636_w )
{
	if (s2636ram[offset] != data)
    {
		s2636ram[offset] = data;
        dirtychar[offset>>3] = 1;
    }
}

READ8_HANDLER( zac_s2636_r )
{
	if(offset!=0xCB) return s2636ram[offset];
    else return CollisionSprite;
}

READ8_HANDLER( tinvader_port_0_r )
{
	return input_port_0_r(0) - CollisionBackground;
}

/*****************************************/
/* Check for Collision between 2 sprites */
/*****************************************/

int SpriteCollision(int first,int second)
{
	int Checksum=0;
	int x,y;

    if((s2636ram[first * 0x10 + 10] < 0xf0) && (s2636ram[second * 0x10 + 10] < 0xf0))
    {
    	int fx     = (s2636ram[first * 0x10 + 10] * 4)-22;
        int fy     = (s2636ram[first * 0x10 + 12] * 3)+3;
		int expand = (first==1) ? 2 : 1;

        /* Draw first sprite */

	    drawgfx(spritebitmap,Machine->gfx[expand],
			    first * 2,
			    0,
			    0,0,
			    fx,fy,
			    0, TRANSPARENCY_NONE, 0);

        /* Get fingerprint */

	    for (x = fx; x < fx + Machine->gfx[expand]->width; x++)
	    {
		    for (y = fy; y < fy + Machine->gfx[expand]->height; y++)
            {
			    if ((x < Machine->screen[0].visarea.min_x) ||
			        (x > Machine->screen[0].visarea.max_x) ||
			        (y < Machine->screen[0].visarea.min_y) ||
			        (y > Machine->screen[0].visarea.max_y))
			    {
				    continue;
			    }

        	    Checksum += read_pixel(spritebitmap, x, y);
            }
	    }

        /* Blackout second sprite */

	    drawgfx(spritebitmap,Machine->gfx[1],
			    second * 2,
			    1,
			    0,0,
			    (s2636ram[second * 0x10 + 10] * 4)-22,(s2636ram[second * 0x10 + 12] * 3) + 3,
			    0, TRANSPARENCY_PEN, 0);

        /* Remove fingerprint */

	    for (x = fx; x < fx + Machine->gfx[expand]->width; x++)
	    {
		    for (y = fy; y < fy + Machine->gfx[expand]->height; y++)
            {
			    if ((x < Machine->screen[0].visarea.min_x) ||
			        (x > Machine->screen[0].visarea.max_x) ||
			        (y < Machine->screen[0].visarea.min_y) ||
			        (y > Machine->screen[0].visarea.max_y))
			    {
				    continue;
			    }

        	    Checksum -= read_pixel(spritebitmap, x, y);
            }
	    }

        /* Zero bitmap */

	    drawgfx(spritebitmap,Machine->gfx[expand],
			    first * 2,
			    1,
			    0,0,
			    fx,fy,
			    0, TRANSPARENCY_NONE, 0);
    }

	return Checksum;
}

static void get_bg_tile_info(int tile_index)
{
	int code = videoram[tile_index];

	SET_TILE_INFO(0, code, 0, 0)
}

VIDEO_START( tinvader )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows,
		TILEMAP_OPAQUE, 24, 24, 32, 32);

	spritebitmap = auto_bitmap_alloc(Machine->screen[0].width,Machine->screen[0].height,Machine->screen[0].format);
	tmpbitmap = auto_bitmap_alloc(Machine->screen[0].width,Machine->screen[0].height,Machine->screen[0].format);

	return 0;
}

static void tinvader_draw_sprites( mame_bitmap *bitmap )
{
	int offs;

    /* -------------------------------------------------------------- */
    /* There seems to be a strange setup with this board, in that it  */
    /* appears that the S2636 runs from a different clock than the    */
    /* background generator, When the program maps sprite position to */
    /* character position it only has 6 pixels of sprite for 8 pixels */
    /* of character.                                                  */
    /* -------------------------------------------------------------- */
    /* n.b. The original has several graphic glitches as well, so it  */
    /* does not seem to be a fault of the emulation!                  */
    /* -------------------------------------------------------------- */

    CollisionBackground = 0;	/* Read from 0x1e80 bit 7 */

	// for collision detection checking
	copybitmap(tmpbitmap,bitmap,0,0,0,0,&Machine->screen[0].visarea,TRANSPARENCY_NONE,0);

    for(offs=0;offs<0x50;offs+=0x10)
    {
    	if((s2636ram[offs+10]<0xF0) && (offs!=0x30))
		{
            int spriteno = (offs / 8);
			int expand   = ((s2636ram[0xc0] & (spriteno*2))!=0) ? 2 : 1;
            int bx       = (s2636ram[offs+10] * 4) - 22;
            int by       = (s2636ram[offs+12] * 3) + 3;
            int x,y;

            if(dirtychar[spriteno])
            {
            	/* 16x8 version */
	   			decodechar(Machine->gfx[1],spriteno,s2636ram,Machine->drv->gfxdecodeinfo[1].gfxlayout);

                /* 16x16 version */
   				decodechar(Machine->gfx[2],spriteno,s2636ram,Machine->drv->gfxdecodeinfo[2].gfxlayout);

                dirtychar[spriteno] = 0;
            }

            /* Sprite->Background collision detection */
			drawgfx(bitmap,Machine->gfx[expand],
				    spriteno,
					1,
				    0,0,
				    bx,by,
				    0, TRANSPARENCY_PEN, 0);

	        for (x = bx; x < bx + Machine->gfx[expand]->width; x++)
	        {
		        for (y = by; y < by + Machine->gfx[expand]->height; y++)
                {
			        if ((x < Machine->screen[0].visarea.min_x) ||
			            (x > Machine->screen[0].visarea.max_x) ||
			            (y < Machine->screen[0].visarea.min_y) ||
			            (y > Machine->screen[0].visarea.max_y))
			        {
				        continue;
			        }

        	        if (read_pixel(bitmap, x, y) != read_pixel(tmpbitmap, x, y))
        	        {
                    	CollisionBackground = 0x80;
				        break;
			        }
                }
	        }

			drawgfx(bitmap,Machine->gfx[expand],
				    spriteno,
					0,
				    0,0,
				    bx,by,
				    0, TRANSPARENCY_PEN, 0);
        }
    }

    /* Sprite->Sprite collision detection */
    CollisionSprite = 0;
//  if(SpriteCollision(0,1)) CollisionSprite |= 0x20;   /* Not Used */
    if(SpriteCollision(0,2)) CollisionSprite |= 0x10;
    if(SpriteCollision(0,4)) CollisionSprite |= 0x08;
    if(SpriteCollision(1,2)) CollisionSprite |= 0x04;
    if(SpriteCollision(1,4)) CollisionSprite |= 0x02;
//  if(SpriteCollision(2,4)) CollisionSprite |= 0x01;   /* Not Used */
}

VIDEO_UPDATE( tinvader )
{
	tilemap_draw(bitmap, &Machine->screen[0].visarea, bg_tilemap, 0, 0);
	tinvader_draw_sprites(bitmap);
	return 0;
}
