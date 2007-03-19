/***************************************************************************

  video.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "includes/vicdual.h"



unsigned char *vicdual_characterram;
static unsigned char dirtycharacter[256];

static int palette_bank;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  The VIC dual game board has one 32x8 palette PROM. The color code is taken
  from the three most significant bits of the character code, plus two
  additional palette bank bits.
  The palette PROM is connected to the RGB output this way:

  bit 7 -- 22 ohm resistor  -- RED   \
        -- 22 ohm resistor  -- BLUE  |  foreground
        -- 22 ohm resistor  -- GREEN /
        -- Unused
        -- 22 ohm resistor  -- RED   \
        -- 22 ohm resistor  -- BLUE  |  background
        -- 22 ohm resistor  -- GREEN /
  bit 0 -- Unused

***************************************************************************/
PALETTE_INIT( vicdual )
{
	int i;
	/* for b&w games we'll use the Head On PROM */
	static unsigned char bw_color_prom[] =
	{
		/* for b/w games, let's use the Head On PROM */
		0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,
		0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,
	};


	if (color_prom == 0) color_prom = bw_color_prom;

	for (i = 0;i < Machine->drv->total_colors / 2;i++)
	{
		/* background components */
		palette_set_color(machine,2*i,pal1bit(color_prom[i] >> 3),pal1bit(color_prom[i] >> 1),pal1bit(color_prom[i] >> 2));

		/* foreground components */
		palette_set_color(machine,2*i+1,pal1bit(color_prom[i] >> 7),pal1bit(color_prom[i] >> 5),pal1bit(color_prom[i] >> 6));
	}

	palette_bank = 0;

	{
		/* Heiankyo Alien doesn't write to port 0x40, it expects it to default to 3 */
		if (strcmp(Machine->gamedrv->name, "heiankyo") == 0)
			palette_bank = 3;

		/* and many others expect it to default to 1 */
		if (strcmp(Machine->gamedrv->name, "invinco") == 0 ||
				strcmp(Machine->gamedrv->name, "digger") == 0 ||
				strcmp(Machine->gamedrv->name, "tranqgun") == 0)
			palette_bank = 1;
	}
}



WRITE8_HANDLER( vicdual_characterram_w )
{
	if (vicdual_characterram[offset] != data)
	{
		dirtycharacter[(offset / 8) & 0xff] = 1;

		vicdual_characterram[offset] = data;
	}
}

READ8_HANDLER( vicdual_characterram_r )
{
	return vicdual_characterram[offset];
}

WRITE8_HANDLER( vicdual_palette_bank_w )
{
	if (palette_bank != (data & 3))
	{
		palette_bank = data & 3;
		memset(dirtybuffer,1,videoram_size);
	}
}



/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
VIDEO_UPDATE( vicdual )
{
	int offs;


	if (get_vh_global_attribute_changed())
	{
		memset(dirtybuffer,1,videoram_size);
	}

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int charcode;


		charcode = videoram[offs];

		if (dirtybuffer[offs] || dirtycharacter[charcode])
		{
			int sx,sy;


			/* decode modified characters */
			if (dirtycharacter[charcode] == 1)
			{
				decodechar(Machine->gfx[0],charcode,vicdual_characterram,Machine->drv->gfxdecodeinfo[0].gfxlayout);
				dirtycharacter[charcode] = 2;
			}


			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;

			drawgfx(tmpbitmap,Machine->gfx[0],
					charcode,
					(charcode >> 5) + 8 * palette_bank,
					0,0,
					8*sx,8*sy,
					&Machine->screen[0].visarea,TRANSPARENCY_NONE,0);

		}
	}
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->screen[0].visarea,TRANSPARENCY_NONE,0);


	for (offs = 0;offs < 256;offs++)
	{
		if (dirtycharacter[offs] == 2) dirtycharacter[offs] = 0;
	}
	return 0;
}
