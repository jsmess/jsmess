/*
 * 
 *  Fujitsu Micro 7 Video functions
 * 
 */

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "includes/fm7.h"

struct fm7_video_flags fm7_video;

extern UINT8* fm7_video_ram;
extern emu_timer* fm7_subtimer;

/*
 * Main CPU: Sub-CPU interface (port 0xfd05)
 * 
 * Read:
 *   bit 7: Sub-CPU busy (or halted)
 *   bit 0: EXTDET (?)
 * Write:
 *   bit 7: Sub-CPU halt
 *   bit 6: Sub-CPU cancel IRQ
 */

READ8_HANDLER( fm7_subintf_r )
{
	UINT8 ret = 0x00;
	
	if(fm7_video.sub_busy != 0 || fm7_video.sub_halt != 0)
		ret |= 0x80;
		
	ret |= 0x7e;
	//ret |= 0x01; // EXTDET (not implemented yet)
	
	return ret;
}

WRITE8_HANDLER( fm7_subintf_w )
{
	fm7_video.sub_halt = data & 0x80;
	if(data & 0x80)
		fm7_video.sub_busy = data & 0x80;

	cputag_set_input_line(space->machine,"sub",INPUT_LINE_HALT,(data & 0x80) ? ASSERT_LINE : CLEAR_LINE);
	if(data & 0x40)
		cputag_set_input_line(space->machine,"sub",M6809_IRQ_LINE,ASSERT_LINE);
	//popmessage("Sub CPU Interface write: %02x\n",data);
}

READ8_HANDLER( fm7_sub_busyflag_r )
{
	if(fm7_video.sub_halt == 0)
		fm7_video.sub_busy = 0x00;
	return 0x00;
}

WRITE8_HANDLER( fm7_sub_busyflag_w )
{
	fm7_video.sub_busy = 0x80;
}

/*
 * Sub-CPU port 0xd402
 *   Read-only: Acknowledge Cancel IRQ
 */
READ8_HANDLER( fm7_cancel_ack )
{
	cputag_set_input_line(space->machine,"sub",M6809_IRQ_LINE,CLEAR_LINE);
	return 0x00;
}

/*
 * Reading from 0xd404 (sub-CPU) causes an "Attention" FIRQ on the main CPU 
 */
READ8_HANDLER( fm7_attn_irq_r )
{
	fm7_video.attn_irq = 1;
	cputag_set_input_line(space->machine,"maincpu",M6809_FIRQ_LINE,ASSERT_LINE);
	return 0xff;
}

/*
 *  Sub CPU: I/O port 0xd409
 * 
 *  On read, enables VRAM access
 *  On write, disables VRAM access
 */
READ8_HANDLER( fm7_vram_access_r )
{
	fm7_video.vram_access = 1;
	return 0xff;
}

WRITE8_HANDLER( fm7_vram_access_w )
{
	fm7_video.vram_access = 0;
}

READ8_HANDLER( fm7_vram_r )
{
	int offs;
	
	offs = (offset & 0xc000) | ((offset + fm7_video.vram_offset) & 0x3fff);
	return fm7_video_ram[offs];
}

WRITE8_HANDLER( fm7_vram_w )
{
	int offs;
	
	offs = (offset & 0xc000) | ((offset + fm7_video.vram_offset) & 0x3fff);
	if(fm7_video.vram_access != 0)
		fm7_video_ram[offs] = data;
}

/*
 *  Sub CPU: I/O port 0xd408
 * 
 *  On read, enables the CRT display
 *  On write, disables the CRT display
 */
READ8_HANDLER( fm7_crt_r )
{
	fm7_video.crt_enable = 1;
	return 0xff;
}

WRITE8_HANDLER( fm7_crt_w )
{
	fm7_video.crt_enable = 0;
}

/*
 *  Sub CPU: I/O ports 0xd40e - 0xd40f
 * 
 *  0xd40e: bits 0-6 - offset in bytes (high byte) (bit 6 is used for 400 line video only)
 *  0xd40f: bits 0-7 - offset in bytes (low byte)
 */
WRITE8_HANDLER( fm7_vram_offset_w )
{
	UINT16 new_offset = 0;
	
	switch(offset)
	{
		case 0:
			new_offset = (data << 8) | (fm7_video.vram_offset & 0x00ff); 
			break;
		case 1:  // low 5 bits are used on FM-77AV and later only
			new_offset = (fm7_video.vram_offset & 0xff00) | (data & 0xe0);
			break;
	}
	
	fm7_video.vram_offset = new_offset;
}

/*
 *  Main CPU: I/O ports 0xfd38-0xfd3f
 *  Colour palette.
 *  Each port represents one of eight colours.  Palette is 3-bit.
 *  bit 2 = Green
 *  bit 1 = Red
 *  bit 0 = Blue
 */
READ8_HANDLER( fm7_palette_r )
{
	return fm7_video.fm7_pal[offset];
}

WRITE8_HANDLER( fm7_palette_w )
{
	UINT8 r = 0,g = 0,b = 0;
	
	if(data & 0x04)
		g = 0xff;
	if(data & 0x02)
		r = 0xff;
	if(data & 0x01)
		b = 0xff;
		
	palette_set_color(space->machine,offset,MAKE_RGB(r,g,b));
	fm7_video.fm7_pal[offset] = data & 0x07;
}

VIDEO_START( fm7 )
{
}

VIDEO_UPDATE( fm7 )
{
    UINT8 code_r,code_g,code_b;
    UINT8 col;
    int y, x, b;

	if(fm7_video.crt_enable == 0)
		return 0;

    for (y = 0; y < 200; y++)
    {
	    for (x = 0; x < 80; x++)
	    {
            code_r = fm7_video_ram[0x8000 + ((y*80 + x + fm7_video.vram_offset) & 0x3fff)];
            code_g = fm7_video_ram[0x4000 + ((y*80 + x + fm7_video.vram_offset) & 0x3fff)];
            code_b = fm7_video_ram[0x0000 + ((y*80 + x + fm7_video.vram_offset) & 0x3fff)];
            for (b = 0; b < 8; b++)
            {
                col = (((code_r >> b) & 0x01) ? 4 : 0) + (((code_g >> b) & 0x01) ? 2 : 0) + (((code_b >> b) & 0x01) ? 1 : 0);
                *BITMAP_ADDR16(bitmap, y,  x*8+(7-b)) =  col;
            }
        }
    }
	return 0;
}

static const rgb_t fm7_initial_palette[8] = {
	MAKE_RGB(0x00, 0x00, 0x00), // 0
	MAKE_RGB(0x00, 0x00, 0xff), // 1
	MAKE_RGB(0xff, 0x00, 0x00), // 2
	MAKE_RGB(0xff, 0x00, 0xff), // 3
	MAKE_RGB(0x00, 0xff, 0x00), // 4
	MAKE_RGB(0x00, 0xff, 0xff), // 5
	MAKE_RGB(0xff, 0xff, 0x00), // 6
	MAKE_RGB(0xff, 0xff, 0xff), // 7
};

PALETTE_INIT( fm7 )
{
	int x;
	
	palette_set_colors(machine, 0, fm7_initial_palette, ARRAY_LENGTH(fm7_initial_palette));
	for(x=0;x<8;x++)
		fm7_video.fm7_pal[x] = x;
}

