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

/*
 *  Main CPU: 0xfd30 - 0xfd34
 *  Analog colour palette (FM-77AV and later only)
 *  All ports are write-only.
 * 
 *  fd30: colour select(?) high 4 bits (LC11-LC8)
 *  fd31: colour select(?) low 8 bits (LC7-LC0)
 *  fd32: blue level (4 bits)
 *  fd33: red level (4 bits)
 *  fd34: green level (4 bits) 
 */
WRITE8_HANDLER( fm77av_analog_palette_w )
{
	int val;
	
	switch(offset)
	{
		case 0:
			val = ((data & 0x0f) << 8) | (fm7_video.fm77av_pal_selected & 0x00ff);
			fm7_video.fm77av_pal_selected = val;
			break;
		case 1:
			val = data | (fm7_video.fm77av_pal_selected & 0x0f00);
			fm7_video.fm77av_pal_selected = val;
			break;
		case 2:
			fm7_video.fm77av_pal_b[fm7_video.fm77av_pal_selected] = (data & 0x0f) << 4;
			palette_set_color(space->machine,fm7_video.fm77av_pal_selected+8,
				MAKE_RGB(fm7_video.fm77av_pal_r[fm7_video.fm77av_pal_selected],
				fm7_video.fm77av_pal_g[fm7_video.fm77av_pal_selected],
				fm7_video.fm77av_pal_b[fm7_video.fm77av_pal_selected]));
			break;
		case 3:
			fm7_video.fm77av_pal_r[fm7_video.fm77av_pal_selected] = (data & 0x0f) << 4;
			palette_set_color(space->machine,fm7_video.fm77av_pal_selected+8,
				MAKE_RGB(fm7_video.fm77av_pal_r[fm7_video.fm77av_pal_selected],
				fm7_video.fm77av_pal_g[fm7_video.fm77av_pal_selected],
				fm7_video.fm77av_pal_b[fm7_video.fm77av_pal_selected]));
			break;
		case 4:
			fm7_video.fm77av_pal_g[fm7_video.fm77av_pal_selected] = (data & 0x0f) << 4;
			palette_set_color(space->machine,fm7_video.fm77av_pal_selected+8,
				MAKE_RGB(fm7_video.fm77av_pal_r[fm7_video.fm77av_pal_selected],
				fm7_video.fm77av_pal_g[fm7_video.fm77av_pal_selected],
				fm7_video.fm77av_pal_b[fm7_video.fm77av_pal_selected]));
			break;
	}
}


/*
 *   Sub CPU: 0xd430 - BUSY/NMI/Bank register (FM77AV series only)
 * 
 *   On read:  bit 7 - 0 if in VBlank
 *             bit 4 - busy(0)/ready(1)
 *             bit 2 - VSync status (1 if active?)
 *             bit 0 - RESET
 */
READ8_HANDLER( fm77av_video_flags_r )
{
	UINT8 ret = 0xff;
	
	if(video_screen_get_vblank(space->machine->primary_screen))
		ret &= ~0x80;
	
	return ret;
}

/*
 *  Main CPU: port 0xfd12 
 *  Sub mode status register  (FM-77AV or later)
 *  bit 6 (R/W) - Video mode width(?) 0=640 (default) 1=320.
 *  bit 1 (R/O) - DISPTMG status (0=blank)
 *  bit 0 (R/O) - VSync status (1=sync?)
 */
READ8_HANDLER( fm77av_sub_modestatus_r )
{
	UINT8 ret = 0x00;
	
	ret |= 0xbc;
	ret |= (fm7_video.modestatus & 0x40);
	if(!video_screen_get_vblank(space->machine->primary_screen))
		ret |= 0x02;
	
	return ret;
}

WRITE8_HANDLER( fm77av_sub_modestatus_w )
{
	fm7_video.modestatus = data & 0x40;
}

/*
 *  Main CPU: port 0xfd13
 *  Sub Bank select register
 * 
 *  bits 1 and 0 select which subsys ROM to be banked into sub CPU space
 *  on the FM-77AV40 and later, bit 2 can also selected to bank in sub monitor RAM.
 */
WRITE8_HANDLER( fm77av_sub_bank_w )
{
	UINT8* RAM = memory_region(space->machine,"sub");
	UINT8* ROM;
	
	fm7_video.subrom = data & 0x03;
	switch (data & 0x03)
	{
		case 0x00:  // Type C, 640x200 (as used on the FM-7)
			ROM = memory_region(space->machine,"subsys_c");
			memory_set_bankptr(space->machine,20,ROM);
			memory_set_bankptr(space->machine,21,ROM+0x800);
			break;
		case 0x01:  // Type A, 640x200
			ROM = memory_region(space->machine,"subsys_a");
			memory_set_bankptr(space->machine,20,RAM+0xd800);
			memory_set_bankptr(space->machine,21,ROM);
			break;
		case 0x02:  // Type B, 320x200
			ROM = memory_region(space->machine,"subsys_b");
			memory_set_bankptr(space->machine,20,RAM+0xd800);
			memory_set_bankptr(space->machine,21,ROM);
			break;
		case 0x03:  // CG Font?
			ROM = memory_region(space->machine,"subsyscg");
			memory_set_bankptr(space->machine,20,RAM+0xd800);
			memory_set_bankptr(space->machine,21,ROM);
			break;
	}
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

