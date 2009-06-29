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
extern UINT8 fm7_type;

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
	UINT16 page = 0x0000;
	
	if(offset < 0x4000 && (fm7_video.multi_page & 0x01))
		return 0xff;
	if((offset < 0x8000 && offset >=0x4000) && (fm7_video.multi_page & 0x02))
		return 0xff;
	if((offset < 0xc000 && offset >=0x8000) && (fm7_video.multi_page & 0x04))
		return 0xff;
	
	if(fm7_video.active_video_page != 0)
		page = 0xc000;

	offs = (offset & 0xc000) | ((offset + fm7_video.vram_offset) & 0x3fff);
	return fm7_video_ram[offs + page];
}

WRITE8_HANDLER( fm7_vram_w )
{
	int offs;
	UINT16 page = 0x0000;
	
	if(offset < 0x4000 && (fm7_video.multi_page & 0x01))
		return;
	if((offset < 0x8000 && offset >=0x4000) && (fm7_video.multi_page & 0x02))
		return;
	if((offset < 0xc000 && offset >=0x8000) && (fm7_video.multi_page & 0x04))
		return;
		
	if(fm7_video.active_video_page != 0)
		page = 0xc000;

	offs = (offset & 0xc000) | ((offset + fm7_video.vram_offset) & 0x3fff);
	if(fm7_video.vram_access != 0)
		fm7_video_ram[offs+page] = data;
}

// not pretty, but it should work.
WRITE8_HANDLER( fm7_vram_banked_w )
{
	int offs;
	UINT16 page = 0x0000;
	
	if(offset < 0x4000 && (fm7_video.multi_page & 0x01))
		return;
	if((offset < 0x8000 && offset >=0x4000) && (fm7_video.multi_page & 0x02))
		return;
	if((offset < 0xc000 && offset >=0x8000) && (fm7_video.multi_page & 0x04))
		return;
		
	if(fm7_video.active_video_page != 0)
		page = 0xc000;

	offs = (offset & 0xc000) | ((offset + fm7_video.vram_offset) & 0x3fff);
	fm7_video_ram[offs+page] = data;
}

READ8_HANDLER( fm7_vram0_r )
{
	return fm7_vram_r(space,offset);
}

READ8_HANDLER( fm7_vram1_r )
{
	return fm7_vram_r(space,offset+0x1000);
}

READ8_HANDLER( fm7_vram2_r )
{
	return fm7_vram_r(space,offset+0x2000);
}

READ8_HANDLER( fm7_vram3_r )
{
	return fm7_vram_r(space,offset+0x3000);
}

READ8_HANDLER( fm7_vram4_r )
{
	return fm7_vram_r(space,offset+0x4000);
}

READ8_HANDLER( fm7_vram5_r )
{
	return fm7_vram_r(space,offset+0x5000);
}

READ8_HANDLER( fm7_vram6_r )
{
	return fm7_vram_r(space,offset+0x6000);
}

READ8_HANDLER( fm7_vram7_r )
{
	return fm7_vram_r(space,offset+0x7000);
}

READ8_HANDLER( fm7_vram8_r )
{
	return fm7_vram_r(space,offset+0x8000);
}

READ8_HANDLER( fm7_vram9_r )
{
	return fm7_vram_r(space,offset+0x9000);
}

READ8_HANDLER( fm7_vramA_r )
{
	return fm7_vram_r(space,offset+0xa000);
}

READ8_HANDLER( fm7_vramB_r )
{
	return fm7_vram_r(space,offset+0xb000);
}

WRITE8_HANDLER( fm7_vram0_w )
{
	fm7_vram_banked_w(space,offset,data);
}

WRITE8_HANDLER( fm7_vram1_w )
{
	fm7_vram_banked_w(space,offset+0x1000,data);
}

WRITE8_HANDLER( fm7_vram2_w )
{
	fm7_vram_banked_w(space,offset+0x2000,data);
}

WRITE8_HANDLER( fm7_vram3_w )
{
	fm7_vram_banked_w(space,offset+0x3000,data);
}

WRITE8_HANDLER( fm7_vram4_w )
{
	fm7_vram_banked_w(space,offset+0x4000,data);
}

WRITE8_HANDLER( fm7_vram5_w )
{
	fm7_vram_banked_w(space,offset+0x5000,data);
}

WRITE8_HANDLER( fm7_vram6_w )
{
	fm7_vram_banked_w(space,offset+0x6000,data);
}

WRITE8_HANDLER( fm7_vram7_w )
{
	fm7_vram_banked_w(space,offset+0x7000,data);
}

WRITE8_HANDLER( fm7_vram8_w )
{
	fm7_vram_banked_w(space,offset+0x8000,data);
}

WRITE8_HANDLER( fm7_vram9_w )
{
	fm7_vram_banked_w(space,offset+0x9000,data);
}

WRITE8_HANDLER( fm7_vramA_w )
{
	fm7_vram_banked_w(space,offset+0xa000,data);
}

WRITE8_HANDLER( fm7_vramB_w )
{
	fm7_vram_banked_w(space,offset+0xb000,data);
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
			if(fm7_type == SYS_FM7)
				new_offset = (fm7_video.vram_offset & 0xff00) | (data & 0xe0);
			else
			{
				if(fm7_video.fine_offset != 0)
					new_offset = (fm7_video.vram_offset & 0xff00) | (data & 0xff);
				else
					new_offset = (fm7_video.vram_offset & 0xff00) | (data & 0xe0);
			}
			break;
	}	
	fm7_video.vram_offset = new_offset;
}

/*
 *  Main CPU: port 0xfd37
 *  Multipage
 * 	Port is write-only
 * 
 * 	bits 6-4: VRAM planes to display (G,R,B) (1=disable)
 *  bits 2-0: VRAM CPU access (G,R,B) (1=disable)
 */
WRITE8_HANDLER( fm7_multipage_w )
{
	fm7_video.multi_page = data & 0x77;
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
 * 
 * 	 On write: bits 1-0 - CGROM select (maps to 0xd800)
 * 	           bit 2 - fine offset enable (enables OA4-OA0 bits in VRAM offset)
 *             bit 5 - active VRAM page
 *             bit 6 - display VRAM page
 *             bit 7 - NMI mask register (1=mask)
 */
READ8_HANDLER( fm77av_video_flags_r )
{
	UINT8 ret = 0xff;
	
	if(video_screen_get_vblank(space->machine->primary_screen))
		ret &= ~0x80;
	
	if(!fm7_video.sub_reset)
		ret &= ~0x01;
		
	return ret;
}

WRITE8_HANDLER( fm77av_video_flags_w )
{
	UINT8* RAM = memory_region(space->machine,"subsyscg");

	fm7_video.cgrom = data & 0x03;
	memory_set_bankptr(space->machine,20,RAM+(fm7_video.cgrom*0x800));
	fm7_video.fine_offset = data & 0x04;
	fm7_video.active_video_page = data & 0x20;
	fm7_video.display_video_page = data & 0x40;
	fm7_video.nmi_mask = data & 0x80;
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
	rectangle rect;
	fm7_video.modestatus = data & 0x40;
	if(data & 0x40)
	{
		rect.min_x = rect.min_y = 0;
		rect.max_x = 320-1;
		rect.max_y = 200-1;
		video_screen_configure(space->machine->primary_screen,320,200,&rect,HZ_TO_ATTOSECONDS(60));
	}
	else
	{
		rect.min_x = rect.min_y = 0;
		rect.max_x = 640-1;
		rect.max_y = 200-1;
		video_screen_configure(space->machine->primary_screen,640,200,&rect,HZ_TO_ATTOSECONDS(60));
	}
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
//	UINT8* RAM = memory_region(space->machine,"sub");
	UINT8* ROM;
	static int prev;
	
	if((data & 0x03) == (prev & 0x03))
		return;
	
	fm7_video.subrom = data & 0x03;
	switch (data & 0x03)
	{
		case 0x00:  // Type C, 640x200 (as used on the FM-7)
			ROM = memory_region(space->machine,"subsys_c");
		//	memory_set_bankptr(space->machine,20,ROM);
			memory_set_bankptr(space->machine,21,ROM+0x800);
			logerror("VID: Sub ROM Type C selected\n");
			break;
		case 0x01:  // Type A, 640x200
			ROM = memory_region(space->machine,"subsys_a");
		//	memory_set_bankptr(space->machine,20,RAM+0xd800);
			memory_set_bankptr(space->machine,21,ROM);
			logerror("VID: Sub ROM Type A selected\n");
			break;
		case 0x02:  // Type B, 320x200
			ROM = memory_region(space->machine,"subsys_b");
		//	memory_set_bankptr(space->machine,20,RAM+0xd800);
			memory_set_bankptr(space->machine,21,ROM);
			logerror("VID: Sub ROM Type B selected\n");
			break;
		case 0x03:  // CG Font?
			ROM = memory_region(space->machine,"subsyscg");
		//	memory_set_bankptr(space->machine,20,RAM+0xd800);
			memory_set_bankptr(space->machine,21,ROM);
			logerror("VID: Sub ROM CG selected\n");
			break;
	}
	// reset sub CPU, set busy flag, set reset flag
	cputag_set_input_line(space->machine,"sub",INPUT_LINE_RESET,PULSE_LINE);
	fm7_video.sub_busy = 0x80;
	fm7_video.sub_reset = 1;
	prev = data;
}

VIDEO_START( fm7 )
{
	fm7_video.vram_access = 0;
	fm7_video.crt_enable = 0;
	fm7_video.vram_offset = 0x0000;
	fm7_video.sub_reset = 0;
	fm7_video.multi_page = 0;
	fm7_video.subrom = 0;
	fm7_video.cgrom = 0;
	fm7_video.fine_offset = 0;
	fm7_video.nmi_mask = 0;
	fm7_video.active_video_page = 0;
	fm7_video.display_video_page = 0;
}

VIDEO_UPDATE( fm7 )
{
    UINT8 code_r = 0,code_g = 0,code_b = 0;
	UINT8 code_r2 = 0,code_g2 = 0,code_b2 = 0;
	UINT8 code_r3 = 0,code_g3 = 0,code_b3 = 0;
	UINT8 code_r4 = 0,code_g4 = 0,code_b4 = 0;
    UINT16 col;
    int y, x, b;
    UINT16 page = 0x0000;
    
    if(fm7_video.display_video_page != 0)
    	page = 0xc000;

	if(fm7_video.crt_enable == 0)
		return 0;

	if(fm7_video.modestatus & 0x40)  // 320x200 mode
	{
	    for (y = 0; y < 200; y++)
	    {
		    for (x = 0; x < 40; x++)
		    {
		    	if(!(fm7_video.multi_page & 0x40))
		    	{
	            	code_r = fm7_video_ram[0x8000 + ((y*40 + x + fm7_video.vram_offset) & 0x1fff)];
	            	code_r2 = fm7_video_ram[0xa000 + ((y*40 + x + fm7_video.vram_offset) & 0x1fff)];
	            	code_r3 = fm7_video_ram[0x14000 + ((y*40 + x + fm7_video.vram_offset) & 0x1fff)];
	            	code_r4 = fm7_video_ram[0x16000 + ((y*40 + x + fm7_video.vram_offset) & 0x1fff)];
		    	}
		    	if(!(fm7_video.multi_page & 0x20))
		    	{
	    	        code_g = fm7_video_ram[0x4000 + ((y*40 + x + fm7_video.vram_offset) & 0x1fff)];
	    	        code_g2 = fm7_video_ram[0x6000 + ((y*40 + x + fm7_video.vram_offset) & 0x1fff)];
	            	code_g3 = fm7_video_ram[0x10000 + ((y*40 + x + fm7_video.vram_offset) & 0x1fff)];
	            	code_g4 = fm7_video_ram[0x12000 + ((y*40 + x + fm7_video.vram_offset) & 0x1fff)];
		    	}
		    	if(!(fm7_video.multi_page & 0x10))
		    	{
		            code_b = fm7_video_ram[0x0000 + ((y*40 + x + fm7_video.vram_offset) & 0x1fff)];
		            code_b2 = fm7_video_ram[0x2000 + ((y*40 + x + fm7_video.vram_offset) & 0x1fff)];
	            	code_b3 = fm7_video_ram[0xc000 + ((y*40 + x + fm7_video.vram_offset) & 0x1fff)];
	            	code_b4 = fm7_video_ram[0xe000 + ((y*40 + x + fm7_video.vram_offset) & 0x1fff)];
		    	}
	            for (b = 0; b < 8; b++)
	            {
	            	col = (((code_b >> b) & 0x01) ? 8 : 0) | (((code_b2 >> b) & 0x01) ? 4 : 0) | (((code_b3 >> b) & 0x01) ? 2 : 0) | (((code_b4 >> b) & 0x01) ? 1 : 0); 
	            	col |= (((code_g >> b) & 0x01) ? 128 : 0) | (((code_g2 >> b) & 0x01) ? 64 : 0) | (((code_g3 >> b) & 0x01) ? 32 : 0) | (((code_g4 >> b) & 0x01) ? 16 : 0);
	            	col |= (((code_r >> b) & 0x01) ? 2048 : 0) | (((code_r2 >> b) & 0x01) ? 1024 : 0) | (((code_r3 >> b) & 0x01) ? 512 : 0) | (((code_r4 >> b) & 0x01) ? 256 : 0); 
					col += 8;  // use analog palette
	                *BITMAP_ADDR16(bitmap, y,  x*8+(7-b)) =  col;
	            }
	        }
	    }
	}
	else
	{
	    for (y = 0; y < 200; y++)
	    {
		    for (x = 0; x < 80; x++)
		    {
		    	if(!(fm7_video.multi_page & 0x40))
	            	code_r = fm7_video_ram[0x8000 + ((y*80 + x + fm7_video.vram_offset) & 0x3fff)];
		    	if(!(fm7_video.multi_page & 0x20))
	    	        code_g = fm7_video_ram[0x4000 + ((y*80 + x + fm7_video.vram_offset) & 0x3fff)];
		    	if(!(fm7_video.multi_page & 0x10))
		            code_b = fm7_video_ram[0x0000 + ((y*80 + x + fm7_video.vram_offset) & 0x3fff)];
	            for (b = 0; b < 8; b++)
	            {
	                col = (((code_r >> b) & 0x01) ? 4 : 0) + (((code_g >> b) & 0x01) ? 2 : 0) + (((code_b >> b) & 0x01) ? 1 : 0);
	                *BITMAP_ADDR16(bitmap, y,  x*8+(7-b)) =  col;
	            }
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

