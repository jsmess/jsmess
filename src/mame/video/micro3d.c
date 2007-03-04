/*====================================================================*/
/*                         'micro3d.c'                                */
/*               Microprose 3D Hardware MAME driver                   */
/*                        Video Emulation                             */
/*                     Philip J Bennett 2004                          */
/*====================================================================*/

#include "driver.h"
#include "cpu/tms34010/tms34010.h"
#include "cpu/tms34010/34010ops.h"
#include "includes/micro3d.h"


UINT16 dpytap, dudate, dumask;
//extern UINT16 *m68681_base;

#if 0
extern struct {

UINT16 MR1A;
UINT16 MR2A;
UINT16 SRA;
UINT16 CSRA;
UINT16 CRA;
UINT16 RBA;
UINT16 TBA;
UINT16 IPCR;
UINT16 ACR;
UINT16 ISR;
UINT16 IMR;
UINT16 CUR;
UINT16 CTUR;
UINT16 CLR;
UINT16 CTLR;
UINT16 MR1B;
UINT16 MR2B;
UINT16 SRB;
UINT16 CSRB;
UINT16 CRB;
UINT16 RBB;
UINT16 TBB;
UINT16 IVR;
UINT16 IPORT;
UINT16 OPCR;
UINT16 OPR;

int MRA_ptr;
int MRB_ptr;

}M68681;
#endif

extern UINT8 ti_uart[8];


void changecolor_BBBBBRRRRRGGGGGG(pen_t color,int data)
{
	palette_set_color(Machine,color,pal5bit(data >> 6),pal5bit(data >> 1),pal5bit(data >> 11));
}

WRITE16_HANDLER( paletteram16_BBBBBRRRRRGGGGGG_word_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	changecolor_BBBBBRRRRRGGGGGG(offset,paletteram16[offset]);
}

VIDEO_START(micro3d)
{
	return 0;
}



VIDEO_UPDATE( micro3d )
{

UINT16 *asrc;
//UINT16 dudate, dumask;
int x,y;
static UINT32 offset=0x1e00;

/*
static int bank=0;

       offset = (bank ? 0x1e00 : 0x37a00);
       bank=!bank;
*/

/*

if(keyboard_pressed(KEYCODE_F1))
{
  UINT8 transmit=0;
                mame_pause(Machine, 1);

                do
            {
        InputCode code;

                  update_video_and_audio();

        if (input_ui_pressed(IPT_UI_CANCEL))
            break;

        code = code_read_async();
        if (code == KEYCODE_F1) code = code_read_async();

        if (code != CODE_NONE)
        {
            if (code >= KEYCODE_A && code <= KEYCODE_Z)
            transmit = code+97;
            if (code >= KEYCODE_0 && code <= KEYCODE_9)
            transmit = code+22;
                        if (code == KEYCODE_ENTER)
                        transmit = 13;
                        if (code == KEYCODE_MINUS)
                        transmit = 63;
                        if (code == KEYCODE_OPENBRACE)
                        transmit = 91;
                        if (code == KEYCODE_CLOSEBRACE)
                        transmit = 93;
                        if (code == KEYCODE_COMMA)
                        transmit = 46;
                        if (code == KEYCODE_SPACE)
                        transmit = 32;

        }
    }
    while (!transmit);


    ti_uart[0]=transmit;
    ti_uart[2]|=2;

      M68681.RBA=transmit<<8;
      M68681.SRA|=0x100;

    mame_pause(Machine, 0);
    return 0;
}
*/

	/* If the display is blanked, fill with black */
	if (tms34010_io_display_blanked(0))
	{
		fillbitmap(bitmap, Machine->pens[0], cliprect);
		return 0;
	}

        /* fetch current scanline advance and column offset values */
	cpuintrf_push_context(0);
	dudate = (tms34010_io_register_r(REG_DPYCTL, 0) & 0x03fc) << 4;
	dumask = dudate - 1;
	cpuintrf_pop_context();

	/* compute the offset */
	offset = (dpyadr << 4);

	/* adjust for when DPYADR was written */
//  if (cliprect->min_y > dpyadrscan)
//      offset += (cliprect->min_y - dpyadrscan) * dudate;
//mame_printf_debug("DPYADR = %04X DPYTAP = %04X (%d-%d)\n", dpyadr, dpytap, cliprect->min_y, cliprect->max_y);

// 7-bits per pixel for the TI 2D, it would seem.
// Bit 8 selects 2D or 3D.

	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT16 scanline[576];                             // Was 577
                asrc = &micro3d_sprite_vram[offset + (512 * y)];     // Correct - there are 1024 bytes per line

			for (x = 0; x < 576; x+=2)               //Number of pixels.
			{
				UINT16 adata = *asrc++;
				if(adata&0x0080)
                         	{
                         	        scanline[x] = ((adata & 0x007f))+0xf00;
                         	}
                         	else
                         	{
                                        scanline[x] = 0;           // Blank - for 3D
                         	}
                                if(adata&0x8000)
                         	{
                         	        scanline[x+1] = ((adata & 0x7f00)>>8)+0xf00;
                         	}
                         	else
                         	{
                         	        scanline[x+1] = 0;         // Blank - for 3D
                         	}
                        }
		/* Draw the scanline */
		draw_scanline16(bitmap, cliprect->min_x, y, cliprect->max_x - cliprect->min_x, &scanline[cliprect->min_x], Machine->pens, -1);
	}
	return 0;
}
