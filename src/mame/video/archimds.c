/***************************************************************************

	Acorn Archimedes VIDC (VIDeo Controller) emulation

***************************************************************************/

#include "emu.h"
#include "includes/archimds.h"

VIDEO_START( archimds_vidc )
{
}

VIDEO_UPDATE( archimds_vidc )
{
	int xstart,ystart,xend,yend;

	/* border color */
	bitmap_fill(bitmap, cliprect, screen->machine->pens[0x10]);

	/* display area x/y */
	xstart = vidc_regs[VIDC_HDSR];
	ystart = vidc_regs[VIDC_VDSR];
	xend = vidc_regs[VIDC_HDER];
	yend = vidc_regs[VIDC_VDER];

	/* disable the screen if display params are invalid */
	if(xstart <= xend || ystart <= yend)
		return 0;

	{
		int count;
		int x,y,xi;
		UINT8 pen;
		static UINT8 *vram = memory_region(screen->machine,"vram");

		count = 0;

		switch(vidc_bpp_mode)
		{
			case 0:
			{
				for(y=0;y<400;y++)
				{
					for(x=0;x<640;x+=8)
					{
						pen = vram[count];

						for(xi=0;xi<8;xi++)
						{
							if ((x+xi) <= screen->visible_area().max_x && (y) <= screen->visible_area().max_y &&
							    (x+xi) <= xend && (y) <= yend)
								*BITMAP_ADDR32(bitmap, y+ystart, x+xi+xstart) = screen->machine->pens[(pen>>(xi))&0x1];
						}

						count++;
					}
				}
			}
			break;
		}
	}

	return 0;
}