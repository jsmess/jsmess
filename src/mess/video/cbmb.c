/***************************************************************************

  PeT mess@utanet.at

***************************************************************************/

#include "driver.h"
#include "includes/cbmb.h"
#include "video/crtc6845.h"


static int cbmb_font=0;

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

	VIDEO_START_CALL(generic);
}

void cbmb_vh_set_font(int font)
{
	cbmb_font=font;
}

void cbm600_update_row(mame_bitmap *bitmap, const rectangle *cliprect, UINT16 ma,
					   UINT8 ra, UINT16 y, UINT8 x_count, void *param) {
	int i;

	for( i = 0; i < x_count; i++ ) {
//		if ( ma + i == cursor_ma ) {
//			plot_box( bitmap, Machine->gfx[cbmb_font]->width * i, y, Machine->gfx[cbmb_font]->width, 1, Machine->pens[1] );
//		} else {
			drawgfx( bitmap, Machine->gfx[cbmb_font], videoram[(ma+i )& 0x7ff], 0, 0, 0, Machine->gfx[cbmb_font]->width * i, y-ra, cliprect, TRANSPARENCY_NONE, 0 );
//		}
	}
}

void cbm700_update_row(mame_bitmap *bitmap, const rectangle *cliprect, UINT16 ma,
					   UINT8 ra, UINT16 y, UINT8 x_count, void *param) {
	int i;

	for( i = 0; i < x_count; i++ ) {
//		if ( ma + i == cursor_ma ) {
//			plot_box( bitmap, Machine->gfx[cbmb_font]->width * i, y, Machine->gfx[cbmb_font]->width, 1, Machine->pens[1] );
//		} else {
			drawgfx( bitmap, Machine->gfx[cbmb_font], videoram[(ma+i) & 0x7ff], 0, 0, 0, Machine->gfx[cbmb_font]->width * i, y-ra, cliprect, TRANSPARENCY_NONE, 0 );
//		}
	}
}

