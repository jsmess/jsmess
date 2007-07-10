/* kan_pand.c Kaneko Pandora Sprite Chip
  GFX processor - PX79C480FP-3 (KANEKO, Pandora-Chip)

  This emulates the Kaneko Pandora Sprite Chip
   which is found on several Kaneko boards.

   there several bootleg variants of this chip,
   these are emulated in kan_panb.c instead.

   Original Games using this Chip

   Snow Bros
   Air Buster
   DJ Boy
   Heavy Unit

   The SemiCom games are also using this because
   their bootleg chip appears to function in an
   identical way.

   The chip appears to be an 8-bit chip, and
   when used on 16-bit CPUs only the MSB or LSB
   of the data lines are connected.  Address Lines
   also appear to be swapped around on one of the
   hookups.

*/

#include "driver.h"

static UINT8* pandora_spriteram;
static UINT8 pandora_region;

void pandora_update(running_machine *machine, mame_bitmap *bitmap, const rectangle *cliprect)
{

	int sx=0, sy=0, x=0, y=0, offs;


	/*
     * Sprite Tile Format
     * ------------------
     *
     * Byte | Bit(s)   | Use
     * -----+-76543210-+----------------
     *  0-2 | -------- | unused
     *  3   | xxxx.... | Palette Bank
     *  3   | .......x | XPos - Sign Bit
     *  3   | ......x. | YPos - Sign Bit
     *  3   | .....x.. | Use Relative offsets
     *  4   | xxxxxxxx | XPos
     *  5   | xxxxxxxx | YPos
     *  6   | xxxxxxxx | Sprite Number (low 8 bits)
     *  7   | ....xxxx | Sprite Number (high 4 bits)
     *  7   | x....... | Flip Sprite Y-Axis
     *  7   | .x...... | Flip Sprite X-Axis
     */

	for (offs = 0;offs < 0x1000;offs += 8)
	{
		int dx = pandora_spriteram[offs+4];
		int dy = pandora_spriteram[offs+5];
		int tilecolour = pandora_spriteram[offs+3];
		int attr = pandora_spriteram[offs+7];
		int flipx =   attr & 0x80;
		int flipy =  (attr & 0x40) << 1;
		int tile  = ((attr & 0x3f) << 8) + (pandora_spriteram[offs+6] & 0xff);

		if (tilecolour & 1) dx = -1 - (dx ^ 0xff);
		if (tilecolour & 2) dy = -1 - (dy ^ 0xff);
		if (tilecolour & 4)
		{
			x += dx;
			y += dy;
		}
		else
		{
			x = dx;
			y = dy;
		}

		if (x > 511) x &= 0x1ff;
		if (y > 511) y &= 0x1ff;

		if (flip_screen)
		{
			sx = 240 - x;
			sy = 240 - y;
			flipx = !flipx;
			flipy = !flipy;
		}
		else
		{
			sx = x;
			sy = y;
		}

		drawgfx(bitmap,machine->gfx[pandora_region],
				tile,
				(tilecolour & 0xf0) >> 4,
				flipx, flipy,
				sx,sy,
				&machine->screen[0].visarea,TRANSPARENCY_PEN,0);
	}
}


void pandora_start(UINT8 region)
{
	pandora_region = region;
	pandora_spriteram = auto_malloc(0x1000);
}


WRITE8_HANDLER ( pandora_spriteram_w )
{
	// it's either hooked up oddly on this, or on snowbros
	offset = BITSWAP16(offset,  15,14,13,12, 11,   7,6,5,4,3,2,1,0,   10,9,8  );

	if (offset>=0x1000)
	{
		logerror("pandora_spriteram_w write past spriteram, offset %04x %02x\n",offset,data);
		return;
	}
	pandora_spriteram[offset] = data;
}

READ8_HANDLER( pandora_spriteram_r )
{
	// it's either hooked up oddly on this, or on snowbros
	offset = BITSWAP16(offset,  15,14,13,12, 11,  7,6,5,4,3,2,1,0,  10,9,8  );

	if (offset>=0x1000)
	{
		logerror("pandora_spriteram_r read past spriteram, offset %04x\n",offset );
		return 0x00;
	}
	return pandora_spriteram[offset];
}

WRITE16_HANDLER( pandora_spriteram_LSB_w )
{
	if (ACCESSING_LSB)
	{
		pandora_spriteram[offset] = data&0xff;
	}
}

READ16_HANDLER( pandora_spriteram_LSB_r )
{
	return pandora_spriteram[offset];
}

WRITE16_HANDLER( pandora_spriteram_MSB_w )
{
	if (ACCESSING_MSB)
	{
		pandora_spriteram[offset] = (data>>8)&0xff;
	}
}

READ16_HANDLER( pandora_spriteram_MSB_r )
{
	return pandora_spriteram[offset]<<8;
}
