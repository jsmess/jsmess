/***************************************************************************

        Ondra driver by Miodrag Milanovic

        08/09/2008 Preliminary driver.

****************************************************************************/


#include "emu.h"
#include "includes/ondra.h"
#include "devices/messram.h"



VIDEO_START( ondra )
{
	ondra_state *state = machine->driver_data<ondra_state>();
	state->video_enable = 0;
}

VIDEO_UPDATE( ondra )
{
	ondra_state *state = screen->machine->driver_data<ondra_state>();
	UINT8 code1,code2;
	int y, x, b;
	int Vaddr = 0x2800;

	if (state->video_enable==1) {
		for (x = 0; x < 40; x++)
		{
			for (y = 127; y >=0; y--)
			{
				code1 = messram_get_ptr(screen->machine->device("messram"))[0xd700 + Vaddr + 0x80];
				code2 = messram_get_ptr(screen->machine->device("messram"))[0xd700 + Vaddr + 0x00];
				for (b = 0; b < 8; b++)
				{
					*BITMAP_ADDR16(bitmap, 2*y, x*8+b) =  ((code1 << b) & 0x80) ? 1 : 0;
					*BITMAP_ADDR16(bitmap, 2*y+1,   x*8+b) =  ((code2 << b) & 0x80) ? 1 : 0;
				}
				Vaddr++;
			}
			Vaddr = (Vaddr - 128) - 256;
		}
	}
	return 0;
}

