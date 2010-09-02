/**************************************************************************************************

    Toshiba Pasopia 7 (c) 1983 Toshiba

    preliminary driver by Angelo Salese

    TODO:
    - floppy support (but floppy images are unobtainable at current time)
    - Z80PIO keyboard irq doesn't work at all, kludged keyboard inputs to work for now

***************************************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/z80ctc.h"
#include "machine/i8255a.h"
#include "machine/z80pio.h"
#include "sound/sn76496.h"
#include "video/mc6845.h"

static UINT8 vram_sel,mio_sel;
static UINT8 *p7_pal;
static UINT8 bank_reg;
static UINT16 cursor_addr;
static UINT8 cursor_blink,cursor_raster;
static UINT8 plane_reg,attr_data,attr_wrap,attr_latch,pal_sel,x_width,gfx_mode;

#define VDP_CLOCK XTAL_3_579545MHz/4

static VIDEO_START( paso7 )
{
	p7_pal = auto_alloc_array(machine, UINT8, 0x10);
}

#define keyb_press(_val_,_charset_) \
	if(input_code_pressed(machine, _val_)) \
	{ \
		ram_space->write_byte(0xfda4,0x01); \
		ram_space->write_byte(0xfce1,_charset_); \
	} \

#define keyb_shift_press(_val_,_charset_) \
	if(input_code_pressed(machine, _val_) && input_code_pressed(machine, KEYCODE_LSHIFT)) \
	{ \
		ram_space->write_byte(0xfda4,0x01); \
		ram_space->write_byte(0xfce1,_charset_); \
	} \

/* cheap kludge to use the keyboard without going nuts with the debugger ... */
static void fake_keyboard_data(running_machine *machine)
{
	address_space *ram_space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	ram_space->write_byte(0xfda4,0x00); //clear flag

	keyb_press(KEYCODE_Z, 'z');
	keyb_press(KEYCODE_X, 'x');
	keyb_press(KEYCODE_C, 'c');
	keyb_press(KEYCODE_V, 'v');
	keyb_press(KEYCODE_B, 'b');
	keyb_press(KEYCODE_N, 'n');
	keyb_press(KEYCODE_M, 'm');

	keyb_press(KEYCODE_A, 'a');
	keyb_press(KEYCODE_S, 's');
	keyb_press(KEYCODE_D, 'd');
	keyb_press(KEYCODE_F, 'f');
	keyb_press(KEYCODE_G, 'g');
	keyb_press(KEYCODE_H, 'h');
	keyb_press(KEYCODE_J, 'j');
	keyb_press(KEYCODE_K, 'k');
	keyb_press(KEYCODE_L, 'l');

	keyb_press(KEYCODE_Q, 'q');
	keyb_press(KEYCODE_W, 'w');
	keyb_press(KEYCODE_E, 'e');
	keyb_press(KEYCODE_R, 'r');
	keyb_press(KEYCODE_T, 't');
	keyb_press(KEYCODE_Y, 'y');
	keyb_press(KEYCODE_U, 'u');
	keyb_press(KEYCODE_I, 'i');
	keyb_press(KEYCODE_O, 'o');
	keyb_press(KEYCODE_P, 'p');

	keyb_press(KEYCODE_0, '0');
	keyb_press(KEYCODE_1, '1');
	keyb_press(KEYCODE_2, '2');
	keyb_press(KEYCODE_3, '3');
	keyb_press(KEYCODE_4, '4');
	keyb_press(KEYCODE_5, '5');
	keyb_press(KEYCODE_6, '6');
	keyb_press(KEYCODE_7, '7');
	keyb_press(KEYCODE_8, '8');
	keyb_press(KEYCODE_9, '9');

	keyb_shift_press(KEYCODE_0, '=');
	keyb_shift_press(KEYCODE_1, '!');
	keyb_shift_press(KEYCODE_2, '"');
	keyb_shift_press(KEYCODE_3, '?');
	keyb_shift_press(KEYCODE_4, '$');
	keyb_shift_press(KEYCODE_5, '%');
	keyb_shift_press(KEYCODE_6, '&');
	keyb_shift_press(KEYCODE_7, '/');
	keyb_shift_press(KEYCODE_8, '(');
	keyb_shift_press(KEYCODE_9, ')');

	keyb_press(KEYCODE_ENTER, 0x0d);
	keyb_press(KEYCODE_SPACE, ' ');
	keyb_press(KEYCODE_STOP, '.');
	keyb_shift_press(KEYCODE_STOP, ':');
	keyb_press(KEYCODE_BACKSPACE, 0x08);
	keyb_press(KEYCODE_0_PAD, '@'); //@
	keyb_press(KEYCODE_COMMA, ',');
	keyb_shift_press(KEYCODE_COMMA, ';');
	keyb_press(KEYCODE_MINUS_PAD, '-');
	keyb_press(KEYCODE_PLUS_PAD, '+');
	keyb_press(KEYCODE_ASTERISK, '*');
}

static void draw_cg4_screen(running_machine *machine, bitmap_t *bitmap,const rectangle *cliprect,int width)
{
	static UINT8 *vram = memory_region(machine, "vram");
	int x,y,xi,yi;
	int count;

	for(yi=0;yi<8;yi++)
	{
		count = yi;
		for(y=0;y<200;y+=8)
		{
			for(x=0;x<8*width;x+=8)
			{
				for(xi=0;xi<8;xi++)
				{
					int pen_b,pen_r,pen_g,color;

					pen_b = (vram[count+0x0000]>>(7-xi)) & 1;
					pen_r = (vram[count+0x4000]>>(7-xi)) & 1;
					pen_g = 0;//(p7_vram[count+0x8000]>>(7-xi)) & 1;

					color =  pen_g<<2 | pen_r<<1 | pen_b<<0;

					*BITMAP_ADDR16(bitmap, y+yi, x+xi) = machine->pens[color+0x20];
				}
				count+=8;
			}
		}
	}
}

static void draw_tv_screen(running_machine *machine, bitmap_t *bitmap,const rectangle *cliprect,int width)
{
	static UINT8 *vram = memory_region(machine, "vram");
	int x,y/*,xi,yi*/;
	int count;

	count = 0x0000;

	for(y=0;y<25;y++)
	{
		for(x=0;x<width;x++)
		{
			int tile = vram[count+0x8000];
			int attr = vram[count+0xc000];
			int color = attr & 7;

			drawgfx_transpen(bitmap,cliprect,machine->gfx[0],tile,color & 7,0,0,x*8,y*8,0);

			// draw cursor
			if(cursor_addr*8 == count)
			{
				int xc,yc,cursor_on;

				cursor_on = 0;
				switch(cursor_raster & 0x60)
				{
					case 0x00: cursor_on = 1; break; //always on
					case 0x20: cursor_on = 0; break; //always off
					case 0x40: if(machine->primary_screen->frame_number() & 0x10) { cursor_on = 1; } break; //fast blink
					case 0x60: if(machine->primary_screen->frame_number() & 0x20) { cursor_on = 1; } break; //slow blink
				}

				if(cursor_on)
				{
					for(yc=0;yc<(8-(cursor_raster & 7));yc++)
					{
						for(xc=0;xc<8;xc++)
						{
							*BITMAP_ADDR16(bitmap, y*8-yc+7, x*8+xc) = machine->pens[0x27];
						}
					}
				}
			}
			count+=8;
		}
	}
}

static void draw_mixed_screen(running_machine *machine, bitmap_t *bitmap,const rectangle *cliprect,int width)
{
	static UINT8 *vram = memory_region(machine, "vram");
	static UINT8 *gfx_data = memory_region(machine,"font");
	int x,y,xi,yi;
	int count;

	count = 0x0000;

	for(y=0;y<25;y++)
	{
		for(x=0;x<width;x++)
		{
			int tile = vram[count+0x8000];

			for(yi=0;yi<8;yi++)
			{
				int attr = vram[count+0xc000+yi];

				if(attr & 0x80)
				{
					for(xi=0;xi<8;xi++)
					{
						int pen,pen_b,pen_r,pen_g;

						pen_b = (vram[count+yi+0x0000]>>(7-xi)) & 1;
						pen_r = (vram[count+yi+0x4000]>>(7-xi)) & 1;
						pen_g = (vram[count+yi+0x8000]>>(7-xi)) & 1;

						pen =  pen_g<<2 | pen_r<<1 | pen_b<<0;

						*BITMAP_ADDR16(bitmap, y*8+yi, x*8+xi) = machine->pens[pen+0x20];
					}
				}
				else
				{
					int color = attr & 7;

					for(xi=0;xi<8;xi++)
					{
						int pen;
						pen = ((gfx_data[tile*8+yi]>>(7-xi)) & 1) ? color : 0;

						*BITMAP_ADDR16(bitmap, y*8+yi, x*8+xi) = machine->pens[pen+0x20];
					}
					//drawgfx_transpen(bitmap,cliprect,machine->gfx[0],tile,color & 7,0,0,x*8,y*8,0);
				}
			}

			// draw cursor
			if(cursor_addr*8 == count)
			{
				int xc,yc,cursor_on;

				cursor_on = 0;
				switch(cursor_raster & 0x60)
				{
					case 0x00: cursor_on = 1; break; //always on
					case 0x20: cursor_on = 0; break; //always off
					case 0x40: if(machine->primary_screen->frame_number() & 0x10) { cursor_on = 1; } break; //fast blink
					case 0x60: if(machine->primary_screen->frame_number() & 0x20) { cursor_on = 1; } break; //slow blink
				}

				if(cursor_on)
				{
					for(yc=0;yc<(8-(cursor_raster & 7));yc++)
					{
						for(xc=0;xc<8;xc++)
						{
							*BITMAP_ADDR16(bitmap, y*8-yc+7, x*8+xc) = machine->pens[0x27];
						}
					}
				}
			}

			count+=8;
		}
	}
}

static VIDEO_UPDATE( paso7 )
{
	int width;

	bitmap_fill(bitmap, cliprect, screen->machine->pens[0]);

	fake_keyboard_data(screen->machine);

	width = x_width ? 80 : 40;

	if(gfx_mode)
		draw_mixed_screen(screen->machine,bitmap,cliprect,width);
	else
	{
		draw_cg4_screen(screen->machine,bitmap,cliprect,width);
		draw_tv_screen(screen->machine,bitmap,cliprect,width);
	}

    return 0;
}

static READ8_HANDLER( vram_r )
{
	static UINT8 *vram = memory_region(space->machine, "vram");
	static UINT8 res;

	if(vram_sel == 0)
	{
		UINT8 *work_ram = memory_region(space->machine, "maincpu");

		return work_ram[offset+0x8000];
	}

	if(pal_sel && (plane_reg & 0x70) == 0x00)
		return p7_pal[offset & 0xf];

	res = 0xff;

	if((plane_reg & 0x11) == 0x11)
		res &= vram[offset | 0x0000];
	if((plane_reg & 0x22) == 0x22)
		res &= vram[offset | 0x4000];
	if((plane_reg & 0x44) == 0x44)
	{
		res &= vram[offset | 0x8000];
		attr_latch = vram[offset | 0xc000] & 0x87;
	}

	return res;
}

static WRITE8_HANDLER( vram_w )
{
	static UINT8 *vram = memory_region(space->machine, "vram");

	if(vram_sel)
	{
		if(pal_sel && (plane_reg & 0x70) == 0x00)
		{
			p7_pal[offset & 0xf] = data & 0xf;
			return;
		}

		if(plane_reg & 0x10)
			vram[(offset & 0x3fff) | 0x0000] = (plane_reg & 1) ? data : 0xff;
		if(plane_reg & 0x20)
			vram[(offset & 0x3fff) | 0x4000] = (plane_reg & 2) ? data : 0xff;
		if(plane_reg & 0x40)
		{
			vram[(offset & 0x3fff) | 0x8000] = (plane_reg & 4) ? data : 0xff;
			attr_latch = attr_wrap ? attr_latch : attr_data;
			vram[(offset & 0x3fff) | 0xc000] = attr_latch;
		}
	}
	else
	{
		UINT8 *work_ram = memory_region(space->machine, "maincpu");

		work_ram[offset+0x8000] = data;
	}
}

static WRITE8_HANDLER( pasopia7_memory_ctrl_w )
{
	UINT8 *work_ram = memory_region(space->machine, "maincpu");
	UINT8 *basic = memory_region(space->machine, "basic");

	switch(data & 3)
	{
		case 0:
		case 3: //select Basic ROM
			memory_set_bankptr(space->machine, "bank1", basic    + 0x00000);
			memory_set_bankptr(space->machine, "bank2", basic    + 0x04000);
			break;
		case 1: //select Basic ROM + BIOS ROM
			memory_set_bankptr(space->machine, "bank1", basic    + 0x00000);
			memory_set_bankptr(space->machine, "bank2", work_ram + 0x10000);
			break;
		case 2: //select Work RAM
			memory_set_bankptr(space->machine, "bank1", work_ram + 0x00000);
			memory_set_bankptr(space->machine, "bank2", work_ram + 0x04000);
			break;
	}

	bank_reg = data & 3;
	vram_sel = data & 4;
	mio_sel = data & 8;

	// bank4 is always RAM

//	printf("%02x\n",vram_sel);
}

#if 0
static READ8_HANDLER( fdc_r )
{
	return mame_rand(space->machine);
}
#endif

static UINT16 pac2_index[2];
static UINT32 kanji_index;
static UINT8 pac2_bank_select;

static WRITE8_HANDLER( pac2_w )
{
	/*
    select register:
    4 = ram1;
    3 = ram2;
    2 = kanji ROM;
    1 = joy;
    anything else is nop
    */

	if(pac2_bank_select == 3 || pac2_bank_select == 4)
	{
		switch(offset)
		{
			case 0:	pac2_index[(pac2_bank_select-3) & 1] = (pac2_index[(pac2_bank_select-3) & 1] & 0x7f00) | (data & 0xff); break;
			case 1: pac2_index[(pac2_bank_select-3) & 1] = (pac2_index[(pac2_bank_select-3) & 1] & 0xff) | ((data & 0x7f) << 8); break;
			case 2: // PAC2 RAM write
			{
				UINT8 *pac2_ram;

				pac2_ram = memory_region(space->machine, ((pac2_bank_select-3) & 1) ? "rampac2" : "rampac1");

				pac2_ram[pac2_index[(pac2_bank_select-3) & 1]] = data;
			}
		}
	}
	else if(pac2_bank_select == 2) // kanji ROM
	{
		switch(offset)
		{
			case 0: kanji_index = (kanji_index & 0x1ff00) | ((data & 0xff) << 0); break;
			case 1: kanji_index = (kanji_index & 0x100ff) | ((data & 0xff) << 8); break;
			case 2: kanji_index = (kanji_index & 0x0ffff) | ((data & 0x01) << 16); break;
		}
	}

	if(offset == 3)
	{
		if(data & 0x80)
		{
			// ...
		}
		else
			pac2_bank_select = data & 7;
	}
}

static READ8_HANDLER( pac2_r )
{
	if(offset == 2)
	{
		if(pac2_bank_select == 3 || pac2_bank_select == 4)
		{
			UINT8 *pac2_ram;

			pac2_ram = memory_region(space->machine, ((pac2_bank_select-3) & 1) ? "rampac2" : "rampac1");

			return pac2_ram[pac2_index[(pac2_bank_select-3) & 1]];
		}
		else if(pac2_bank_select == 2)
		{
			UINT8 *kanji_rom = memory_region(space->machine, "kanji");

			return kanji_rom[kanji_index];
		}
		else
		{
			printf("%02x\n",pac2_bank_select);
		}
	}

	return 0xff;
}

/* writes always occurs to the RAM banks, even if the ROMs are selected. */
static WRITE8_HANDLER( ram_bank_w )
{
	UINT8 *work_ram = memory_region(space->machine, "maincpu");

	work_ram[offset] = data;
}

static WRITE8_HANDLER( pasopia7_6845_w )
{
	static int addr_latch;

	if(offset == 0)
	{
		addr_latch = data;
		mc6845_address_w(space->machine->device("crtc"), 0,data);
	}
	else
	{
		/* FIXME: this should be inside the MC6845 core! */
		if(addr_latch == 0x0a)
			cursor_raster = data;
		if(addr_latch == 0x0e)
			cursor_addr = ((data<<8) & 0x3f00) | (cursor_addr & 0xff);
		else if(addr_latch == 0x0f)
			cursor_addr = (cursor_addr & 0x3f00) | (data & 0xff);

		mc6845_register_w(space->machine->device("crtc"), 0,data);

		/* double pump the pixel clock if we are in 640 x 200 mode */
		mc6845_set_clock(space->machine->device("crtc"), (x_width) ? VDP_CLOCK*2 : VDP_CLOCK);
	}
}

static READ8_HANDLER( pasopia7_io_r )
{
	static UINT16 io_port;

	if(mio_sel)
	{
		address_space *ram_space = cputag_get_address_space(space->machine, "maincpu", ADDRESS_SPACE_PROGRAM);

		mio_sel = 0;
		//printf("%08x\n",offset);
		//return 0x0d; // hack: this is used for reading the keyboard data, we can fake it a little ... (modify fda4)
		return ram_space->read_byte(offset);
	}

	io_port = offset & 0xff; //trim down to 8-bit bus

	if(io_port >= 0x08 && io_port <= 0x0b)		{ return i8255a_r(space->machine->device("ppi8255_0"), (io_port-0x08) & 3); }
	else if(io_port >= 0x0c && io_port <= 0x0f) { return i8255a_r(space->machine->device("ppi8255_1"), (io_port-0x0c) & 3); }
//  else if(io_port == 0x10 || io_port == 0x11) { M6845 read }
	else if(io_port >= 0x18 && io_port <= 0x1b) { return pac2_r(space, (io_port-0x18) & 3);  }
	else if(io_port >= 0x20 && io_port <= 0x23) { return i8255a_r(space->machine->device("ppi8255_2"), (io_port-0x20) & 3); }
	else if(io_port >= 0x28 && io_port <= 0x2b) { return z80ctc_r(space->machine->device("ctc"), (io_port-0x28) & 3);  }
	else if(io_port >= 0x30 && io_port <= 0x33) { return z80pio_ba_cd_r(space->machine->device("z80pio_0"), (io_port-0x30) & 3); }
//  else if(io_port == 0x3a)                    { SN1 }
//  else if(io_port == 0x3b)                    { SN2 }
//  else if(io_port == 0x3c)                    { bankswitch }
//  else if(io_port >= 0xe0 && io_port <= 0xe6) { fdc }
	else
	{
		logerror("(PC=%06x) Read i/o address %02x\n",cpu_get_pc(space->cpu),io_port);
	}
	return 0xff;
}

static WRITE8_HANDLER( pasopia7_io_w )
{
	static UINT16 io_port;

	if(mio_sel)
	{
		address_space *ram_space = cputag_get_address_space(space->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
		mio_sel = 0;
		ram_space->write_byte(offset, data);
		return;
	}

	io_port = offset & 0xff; //trim down to 8-bit bus

	if(io_port >= 0x08 && io_port <= 0x0b)		{ i8255a_w(space->machine->device("ppi8255_0"), (io_port-0x08) & 3, data); }
	else if(io_port >= 0x0c && io_port <= 0x0f) { i8255a_w(space->machine->device("ppi8255_1"), (io_port-0x0c) & 3, data); }
	else if(io_port >= 0x10 && io_port <= 0x11) { pasopia7_6845_w(space, io_port-0x10, data); }
	else if(io_port >= 0x18 && io_port <= 0x1b) { pac2_w(space, (io_port-0x18) & 3, data);  }
	else if(io_port >= 0x20 && io_port <= 0x23) { i8255a_w(space->machine->device("ppi8255_2"), (io_port-0x20) & 3, data); }
	else if(io_port >= 0x28 && io_port <= 0x2b) { z80ctc_w(space->machine->device("ctc"), (io_port-0x28) & 3,data);  }
	else if(io_port >= 0x30 && io_port <= 0x33) { z80pio_ba_cd_w(space->machine->device("z80pio_0"), (io_port-0x30) & 3, data); }
	else if(io_port == 0x3a)					{ sn76496_w(space->machine->device("sn1"), 0, data); }
	else if(io_port == 0x3b)					{ sn76496_w(space->machine->device("sn2"), 0, data); }
	else if(io_port == 0x3c)					{ pasopia7_memory_ctrl_w(space,0, data); }
//  else if(io_port >= 0xe0 && io_port <= 0xe6) { fdc }
	else
	{
		logerror("(PC=%06x) Write i/o address %02x = %02x\n",cpu_get_pc(space->cpu),offset,data);
	}
}

static ADDRESS_MAP_START(paso7_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x7fff ) AM_WRITE( ram_bank_w )
	AM_RANGE( 0x0000, 0x3fff ) AM_ROMBANK("bank1")
	AM_RANGE( 0x4000, 0x7fff ) AM_ROMBANK("bank2")
	AM_RANGE( 0x8000, 0xbfff ) AM_READWRITE(vram_r, vram_w )
	AM_RANGE( 0xc000, 0xffff ) AM_RAMBANK("bank4")
ADDRESS_MAP_END

static ADDRESS_MAP_START( paso7_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0xffff) AM_READWRITE( pasopia7_io_r, pasopia7_io_w )
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( paso7 )
	PORT_START("DSW")
	PORT_DIPNAME( 0x01, 0x01, "System type" )
	PORT_DIPSETTING(    0x01, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x00, "LCD" )
INPUT_PORTS_END

static const gfx_layout p7_chars_8x8 =
{
	8,8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_layout p7_chars_16x16 =
{
	16,16,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	16*16
};

static GFXDECODE_START( pasopia7 )
	GFXDECODE_ENTRY( "font",   0x00000, p7_chars_8x8,    0, 0x10 )
	GFXDECODE_ENTRY( "kanji",  0x00000, p7_chars_16x16,  0, 0x10 )
GFXDECODE_END

static const mc6845_interface mc6845_intf =
{
	"screen",	/* screen we are acting on */
	8,			/* number of pixels per video memory address */
	NULL,		/* before pixel update callback */
	NULL,		/* row update callback */
	NULL,		/* after pixel update callback */
	DEVCB_NULL,	/* callback for display state changes */
	DEVCB_NULL,	/* callback for cursor state changes */
	DEVCB_NULL,	/* HSYNC callback */
	DEVCB_NULL,	/* VSYNC callback */
	NULL		/* update address callback */
};

static Z80CTC_INTERFACE( ctc_intf )
{
	0,					// timer disables
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0),		// interrupt handler
	DEVCB_LINE(z80ctc_trg1_w),		// ZC/TO0 callback
	DEVCB_LINE(z80ctc_trg2_w),						// ZC/TO1 callback, beep interface
	DEVCB_LINE(z80ctc_trg3_w)		// ZC/TO2 callback
};

static Z80PIO_INTERFACE( z80pio_intf )
{
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0), //doesn't work?
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

static const z80_daisy_config p7_daisy[] =
{
	{ "ctc" },
	{ "z80pio_0" },
	{ NULL }
};

static READ8_DEVICE_HANDLER( crtc_portb_r )
{
	// --x- ---- vsync bit
	// ---x ---- lcd dip-sw
	// ---- x--- disp bit
	UINT8 lcd_bit = input_port_read(device->machine, "DSW") & 1;
	UINT8 vdisp = (device->machine->primary_screen->vpos() < (lcd_bit ? 200 : 28)) ? 0x08 : 0x00; //TODO: check LCD vpos trigger
	UINT8 vsync = vdisp ? 0x00 : 0x20;

	return 0x40 | (attr_latch & 0x87) | vsync | vdisp | (lcd_bit << 4);
}

static WRITE8_DEVICE_HANDLER( screen_mode_w )
{
	if(data & 0x5f)
		printf("GFX MODE %02x\n",data);

	x_width = data & 0x20;
	gfx_mode = data & 0x80;

//	printf("%02x\n",gfx_mode);
}

static WRITE8_DEVICE_HANDLER( plane_reg_w )
{
	//if(data & 0x11)
	//printf("PLANE %02x\n",data);
	plane_reg = data;
}

static WRITE8_DEVICE_HANDLER( video_attr_w )
{
	//printf("VIDEO ATTR %02x | TEXT_PAGE %02x\n",data & 0xf,data & 0x70);
	attr_data = (data & 0x7) | ((data & 0x8)<<4);
}

//#include "debugger.h"

static WRITE8_DEVICE_HANDLER( video_misc_w )
{
	/*
        --x- ---- blinking
        ---x ---- attribute wrap
        ---- x--- pal disable
        ---- xx-- palette selector (both bits enables this, odd hook-up)
    */
//  if(data & 2)
//  {
//      printf("VIDEO MISC %02x\n",data);
//      debugger_break(device->machine);
//  }
	cursor_blink = data & 0x20;
	attr_wrap = data & 0x10;
//  pal_sel = data & 0x02;
}

static WRITE8_DEVICE_HANDLER( nmi_mask_w )
{
	/*
    --x- ---- (related to the data rec)
    ---x ---- data rec out
    ---- --x- sound off
    ---- ---x reset NMI (writes to port B = clears bits 1 & 2) (???)
    */
//  printf("SYSTEM MISC %02x\n",data);

//  if(data & 1)
//      i8255a_w(devtag_get_device(device->machine, "ppi8255_2"), 1, 0);

}

/* TODO: investigate on these. */
static READ8_DEVICE_HANDLER( unk_r )
{
	return 0xff;//mame_rand(device->machine);
}

static READ8_DEVICE_HANDLER( nmi_reg_r )
{
	return 0xfc | bank_reg;//mame_rand(device->machine);
}

static UINT8 nmi_mask,nmi_enable_reg;

static WRITE8_DEVICE_HANDLER( nmi_reg_w )
{
	/*
        x--- ---- NMI mask
        -x-- ---- enable NMI regs (?)
    */
	nmi_mask = data & 0x80;
	nmi_enable_reg = data & 0x40;

	if(!nmi_mask && nmi_enable_reg)
		cputag_set_input_line(device->machine, "maincpu", INPUT_LINE_NMI, PULSE_LINE);
}



static I8255A_INTERFACE( ppi8255_intf_0 )
{
	DEVCB_HANDLER(unk_r),			/* Port A read */
	DEVCB_HANDLER(crtc_portb_r),	/* Port B read */
	DEVCB_NULL,						/* Port C read */
	DEVCB_HANDLER(screen_mode_w),	/* Port A write */
	DEVCB_NULL,						/* Port B write */
	DEVCB_NULL						/* Port C write */
};

static I8255A_INTERFACE( ppi8255_intf_1 )
{
	DEVCB_NULL,						/* Port A read */
	DEVCB_NULL,						/* Port B read */
	DEVCB_NULL,						/* Port C read */
	DEVCB_HANDLER(plane_reg_w),		/* Port A write */
	DEVCB_HANDLER(video_attr_w),	/* Port B write */
	DEVCB_HANDLER(video_misc_w)		/* Port C write */
};

static I8255A_INTERFACE( ppi8255_intf_2 )
{
	DEVCB_NULL,						/* Port A read */
	DEVCB_NULL,						/* Port B read */
	DEVCB_HANDLER(nmi_reg_r),		/* Port C read */
	DEVCB_HANDLER(nmi_mask_w),		/* Port A write */
	DEVCB_NULL,						/* Port B write */
	DEVCB_HANDLER(nmi_reg_w)		/* Port C write */
};

static MACHINE_RESET( paso7 )
{
	UINT8 *bios = memory_region(machine, "maincpu");

	memory_set_bankptr(machine, "bank1", bios + 0x10000);
	memory_set_bankptr(machine, "bank2", bios + 0x10000);
//  memory_set_bankptr(machine, "bank3", bios + 0x10000);
//  memory_set_bankptr(machine, "bank4", bios + 0x10000);
}

static PALETTE_INIT( pasopia7 )
{
	int i;

	for (i = 0x000; i < 0x020; i++)
	{
		UINT8 r,g,b;

		if(((i & 0x11) == 0x01) || ((i & 0x11) == 0x10))
		{
			r = ((i >> 2) & 1) ? 0xff : 0x00;
			g = ((i >> 3) & 1) ? 0xff : 0x00;
			b = ((i >> 1) & 1) ? 0xff : 0x00;
		}
		else { r = g = b = 0x00; }

		palette_set_color_rgb(machine, i, r,g,b);
	}
	for( i = 0x000; i < 8; i++)
		palette_set_color_rgb(machine, i+0x020, pal1bit(i >> 1), pal1bit(i >> 2), pal1bit(i >> 0));
}

static MACHINE_CONFIG_START( paso7, driver_device )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(paso7_mem)
    MDRV_CPU_IO_MAP(paso7_io)
//  MDRV_CPU_VBLANK_INT("screen", irq0_line_hold)
	MDRV_CPU_CONFIG(p7_daisy)

	MDRV_Z80CTC_ADD( "ctc", XTAL_4MHz, ctc_intf )

    MDRV_MACHINE_RESET(paso7)

	MDRV_I8255A_ADD( "ppi8255_0", ppi8255_intf_0 )
	MDRV_I8255A_ADD( "ppi8255_1", ppi8255_intf_1 )
	MDRV_I8255A_ADD( "ppi8255_2", ppi8255_intf_2 )
	MDRV_Z80PIO_ADD( "z80pio_0", XTAL_4MHz, z80pio_intf )

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(60)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 200-1)
	MDRV_MC6845_ADD("crtc", H46505, VDP_CLOCK, mc6845_intf)	/* unknown clock, hand tuned to get ~60 fps */
    MDRV_PALETTE_LENGTH(0x28)
    MDRV_PALETTE_INIT(pasopia7)

	MDRV_GFXDECODE( pasopia7 )

    MDRV_VIDEO_START(paso7)
    MDRV_VIDEO_UPDATE(paso7)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD("sn1", SN76489A, 1996800) // unknown clock / divider
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_ADD("sn2", SN76489A, 1996800) // unknown clock / divider
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( pasopia7 )
	ROM_REGION( 0x14000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "bios.rom", 0x10000, 0x4000, CRC(b8111407) SHA1(ac93ae62db4c67de815f45de98c79cfa1313857d))

	ROM_REGION( 0x8000, "basic", ROMREGION_ERASEFF )
	ROM_LOAD( "basic.rom", 0x0000, 0x8000, CRC(8a58fab6) SHA1(5e1a91dfb293bca5cf145b0a0c63217f04003ed1))

	ROM_REGION( 0x800, "font", ROMREGION_ERASEFF )
	ROM_LOAD( "font.rom", 0x0000, 0x0800, CRC(a91c45a9) SHA1(a472adf791b9bac3dfa6437662e1a9e94a88b412))

	ROM_REGION( 0x20000, "kanji", ROMREGION_ERASEFF )
	ROM_LOAD( "kanji.rom", 0x0000, 0x20000, CRC(6109e308) SHA1(5c21cf1f241ef1fa0b41009ea41e81771729785f))

	ROM_REGION( 0x8000, "rampac1", ROMREGION_ERASEFF )
//  ROM_LOAD( "rampac1.bin", 0x0000, 0x8000, CRC(0e4f09bd) SHA1(4088906d57e4f6085a75b249a6139a0e2eb531a1) )

	ROM_REGION( 0x8000, "rampac2", ROMREGION_ERASEFF )
//  ROM_LOAD( "rampac2.bin", 0x0000, 0x8000, CRC(0e4f09bd) SHA1(4088906d57e4f6085a75b249a6139a0e2eb531a1) )

	ROM_REGION( 0x10000, "vram", ROMREGION_ERASE00 )
ROM_END

static DRIVER_INIT( paso7 )
{
}

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1983, pasopia7,  0,       0,	 paso7,	paso7,   paso7,   "Toshiba",   "PASOPIA 7", GAME_NOT_WORKING )
