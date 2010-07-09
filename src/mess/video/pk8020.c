/***************************************************************************

        PK-8020 driver by Miodrag Milanovic

        18/07/2008 Preliminary driver.

****************************************************************************/

#include "emu.h"
#include "includes/pk8020.h"
#include "devices/messram.h"

VIDEO_START( pk8020 )
{
}

VIDEO_UPDATE( pk8020 )
{
	int y, x, b, j;
	UINT8 *gfx = memory_region(screen->machine, "gfx1");

	for (y = 0; y < 16; y++)
	{
		for (x = 0; x < 64; x++)
		{
			UINT8 chr = messram_get_ptr(screen->machine->device("messram"))[x +(y*64) + 0x40000];
			UINT8 attr= messram_get_ptr(screen->machine->device("messram"))[x +(y*64) + 0x40400];
			for (j = 0; j < 16; j++) {
				UINT32 addr = 0x10000 + x + ((y*16+j)*64) + (pk8020_video_page * 0xC000);
				UINT8 code1 = messram_get_ptr(screen->machine->device("messram"))[addr];
				UINT8 code2 = messram_get_ptr(screen->machine->device("messram"))[addr + 0x4000];
				UINT8 code3 = messram_get_ptr(screen->machine->device("messram"))[addr + 0x8000];
				UINT8 code4 = gfx[((chr<<4) + j) + (pk8020_font*0x1000)];
				if(attr) code4 ^= 0xff;
				for (b = 0; b < 8; b++)
				{
					UINT8 col = (((code4 >> b) & 0x01) ? 0x08 : 0x00);
					col |= (((code3 >> b) & 0x01) ? 0x04 : 0x00);
					col |= (((code2 >> b) & 0x01) ? 0x02 : 0x00);
					col |= (((code1 >> b) & 0x01) ? 0x01 : 0x00);
					*BITMAP_ADDR16(bitmap, (y*16)+j, x*8+(7-b)) =  col;
				}
			}
		}
	}
	return 0;
}

PALETTE_INIT( pk8020 )
{
	int i;
	for(i=0;i<16;i++) {
		palette_set_color( machine, i, MAKE_RGB(i*0x10,i*0x10,i*0x10) );
	}
}
