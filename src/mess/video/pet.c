/***************************************************************************

  commodore pet discrete video circuit

  PeT mess@utanet.at

***************************************************************************/

#include "driver.h"
#include "includes/pet.h"


void pet_vh_init (running_machine *machine)
{
	UINT8 *gfx = memory_region(machine, "gfx1");
	int i;

	/* inversion logic on board */
	for (i = 0; i < 0x400; i++)
	{
		gfx[0x800+i] = gfx[0x400+i];
		gfx[0x400+i] = gfx[i]^0xff;
		gfx[0xc00+i] = gfx[0x800+i]^0xff;
	}
}

void pet80_vh_init (running_machine *machine)
{
	UINT8 *gfx = memory_region(machine, "gfx1");
	int i;

	/* inversion logic on board */
	for (i = 0; i < 0x400; i++) {
		gfx[0x800+i] = gfx[0x400+i];
		gfx[0x400+i] = gfx[i]^0xff;
		gfx[0x0c00+i] = gfx[0x800+i]^0xff;
	}
	// I assume the hardware logic is not displaying line 8 and 9 of char
	// I draw it like lines would be 8-15 are black!
	for (i=511; i>=0; i--) {
		memcpy(gfx+i*16, gfx+i*8, 8);
		memset(gfx+i*16+8, 0, 8);
	}
}

void superpet_vh_init (running_machine *machine)
{
	UINT8 *gfx = memory_region(machine, "gfx1");
	int i;

	for (i=0; i<0x400; i++) {
		gfx[0x1000+i]=gfx[0x800+i];
		gfx[0x1800+i]=gfx[0xc00+i];
		gfx[0x1c00+i]=gfx[0x1800+i]^0xff;
		gfx[0x1400+i]=gfx[0x1000+i]^0xff;
		gfx[0x800+i]=gfx[0x400+i];
		gfx[0xc00+i]=gfx[0x800+i]^0xff;
		gfx[0x400+i]=gfx[i]^0xff;
	}
	// I assume the hardware logic is not displaying line 8 and 9 of char
	// I draw it like lines 8-15 are black!
	for (i=1023; i>=0; i--) {
		memcpy(gfx+i*16, gfx+i*8, 8);
		memset(gfx+i*16+8, 0, 8);
	}
}

//  commodore pet discrete video circuit
VIDEO_UPDATE( pet )
{
	int x, y, i;

	for (y=0, i=0; y<25;y++)
	{
		for (x=0;x<40;x++, i++)
		{
			drawgfx_opaque(bitmap, NULL,screen->machine->gfx[pet_font],
					videoram[i], 0, 0, 0, 8*x,8*y);
		}
	}
	return 0;
}


MC6845_UPDATE_ROW( pet40_update_row )
{
	int i;

	for( i = 0; i < x_count; i++ ) {
		drawgfx_opaque( bitmap, cliprect, device->machine->gfx[pet_font], videoram[(ma+i)&0x3ff], 0, 0, 0, 8 * i, y-ra );
	}
}

MC6845_UPDATE_ROW( pet80_update_row )
{
	int i;

	for( i = 0; i < x_count; i++ ) {
		drawgfx_opaque( bitmap, cliprect, device->machine->gfx[pet_font], videoram[((ma+i)<<1)&0x7ff], 0, 0, 0, 16 * i, y-ra );
		drawgfx_opaque( bitmap, cliprect, device->machine->gfx[pet_font], videoram[(((ma+i)<<1)+1)&0x7ff], 0, 0, 0, 16 * i + 8, y-ra );
	}
}

WRITE_LINE_DEVICE_HANDLER( pet_display_enable_changed )
{
}

