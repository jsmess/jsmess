/***************************************************************************

  PeT mess@utanet.at

***************************************************************************/

#include "driver.h"
#include "deprecat.h"
#include "includes/cbmb.h"
#include "video/mc6845.h"


static int cbmb_font=0;
static mc6845_t *mc6845;

VIDEO_START( cbmb_crtc ) {
	mc6845 = devtag_get_token(machine, MC6845, "crtc");
}

VIDEO_UPDATE( cbmb_crtc ) {
	mc6845_update(mc6845, bitmap, cliprect);
	return 0;
}

void cbm600_vh_init(void)
{
	UINT8 *gfx = memory_region(REGION_GFX1);
	int i;

	/* inversion logic on board */
	for (i=0; i<0x800; i++) {
		gfx[0x1000+i]=gfx[0x800+i];
		gfx[0x1800+i]=gfx[0x1000+i]^0xff;
		gfx[0x800+i]=gfx[i]^0xff;
	}
}

void cbm700_vh_init(void)
{
	UINT8 *gfx = memory_region(REGION_GFX1);
	int i;
	for (i=0; i<0x800; i++) {
		gfx[0x1000+i]=gfx[0x800+i];
		gfx[0x1800+i]=gfx[0x1000+i]^0xff;
		gfx[0x800+i]=gfx[i]^0xff;
	}
}

VIDEO_START( cbm700 )
{
	int i;

	mc6845 = devtag_get_token(machine, MC6845, "crtc");

    /* remove pixel column 9 for character codes 0 - 175 and 224 - 255 */
	for( i = 0; i < 256; i++)
	{
//		if( i < 176 || i > 223 )
		{
			int y;
			for( y = 0; y < machine->gfx[0]->height; y++ ) {
				machine->gfx[0]->gfxdata[(i * machine->gfx[0]->height + y) * machine->gfx[0]->width + 8] = 0;
				machine->gfx[1]->gfxdata[(i * machine->gfx[1]->height + y) * machine->gfx[1]->width + 8] = 0;
			}
		}
	}
}

void cbmb_vh_set_font(int font)
{
	cbmb_font=font;
}

MC6845_UPDATE_ROW( cbm600_update_row )
{
	int i;

	for( i = 0; i < x_count; i++ ) {
		if ( i == cursor_x ) {
			plot_box( bitmap, machine->gfx[cbmb_font]->width * i, y, machine->gfx[cbmb_font]->width, 1, machine->pens[1] );
		} else {
			drawgfx( bitmap, machine->gfx[cbmb_font], videoram[(ma+i )& 0x7ff], 0, 0, 0, machine->gfx[cbmb_font]->width * i, y-ra, cliprect, TRANSPARENCY_NONE, 0 );
		}
	}
}

MC6845_UPDATE_ROW( cbm700_update_row )
{
	int i;

	for( i = 0; i < x_count; i++ ) {
		if ( i == cursor_x ) {
			plot_box( bitmap, machine->gfx[cbmb_font]->width * i, y, machine->gfx[cbmb_font]->width, 1, machine->pens[1] );
		} else {
			drawgfx( bitmap, machine->gfx[cbmb_font], videoram[(ma+i) & 0x7ff], 0, 0, 0, machine->gfx[cbmb_font]->width * i, y-ra, cliprect, TRANSPARENCY_NONE, 0 );
		}
	}
}

void cbmb_display_enable_changed(running_machine *machine, mc6845_t *mc6845, int display_enabled)
{
}

