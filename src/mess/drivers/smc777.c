/***************************************************************************

    SMC-777 (c) 1983 Sony

    preliminary driver by Angelo Salese

    TODO:
    - no documentation, the entire driver is just a bunch of educated
      guesses ...
    - video emulation is just hacked up together to show something simple,
      dunno yet how it really works.
	- BEEP pitch is horrible

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "sound/sn76496.h"
#include "sound/beep.h"
#include "video/mc6845.h"

#include "machine/wd17xx.h"
#include "formats/basicdsk.h"
#include "devices/flopdrv.h"

static UINT16 cursor_addr;
static UINT8 keyb_press,keyb_press_flag;
static UINT8 backdrop_pen;

#define CRTC_MIN_X 10
#define CRTC_MIN_Y 10

static VIDEO_START( smc777 )
{
}

static VIDEO_UPDATE( smc777 )
{
	int x,y,yi;
	UINT16 count;
	static UINT8 *vram = memory_region(screen->machine, "vram");
	static UINT8 *attr = memory_region(screen->machine, "attr");
	static UINT8 *gram = memory_region(screen->machine, "fbuf");

	bitmap_fill(bitmap, cliprect, screen->machine->pens[backdrop_pen+0x10]);

	count = 0x0000;
	for(yi=0;yi<8;yi++)
	{
		for(y=0;y<200;y+=8)
		{
			for(x=0;x<160;x++)
			{
				static UINT16 color;

				color = (gram[count] & 0xf0) >> 4;
				*BITMAP_ADDR16(bitmap, y+yi+CRTC_MIN_Y, x*4+0+CRTC_MIN_X) = screen->machine->pens[color+0x10];
				*BITMAP_ADDR16(bitmap, y+yi+CRTC_MIN_Y, x*4+1+CRTC_MIN_X) = screen->machine->pens[color+0x10];

				color = (gram[count] & 0x0f) >> 0;
				*BITMAP_ADDR16(bitmap, y+yi+CRTC_MIN_Y, x*4+2+CRTC_MIN_X) = screen->machine->pens[color+0x10];
				*BITMAP_ADDR16(bitmap, y+yi+CRTC_MIN_Y, x*4+3+CRTC_MIN_X) = screen->machine->pens[color+0x10];

				count++;

			}
		}
		count+= 0x60;
	}

	count = 0x0000;

	for(y=0;y<25;y++)
	{
		for(x=0;x<80;x++)
		{
			int tile = vram[count];
			int color = attr[count] & 7;

			drawgfx_transpen(bitmap,cliprect,screen->machine->gfx[0],tile,color,0,0,x*8+CRTC_MIN_X,y*8+CRTC_MIN_Y,0);

			if(((cursor_addr) == count) && screen->machine->primary_screen->frame_number() & 0x10) // draw cursor
				drawgfx_transpen(bitmap,cliprect,screen->machine->gfx[0],0x87,7,0,0,x*8+CRTC_MIN_X,y*8+CRTC_MIN_Y,0);

			count++; //TODO: width 40 uses count += 2
		}
	}

    return 0;
}

static WRITE8_HANDLER( smc777_6845_w )
{
	static int addr_latch;

	if(offset == 0)
	{
		addr_latch = data;
		//mc6845_address_w(space->machine->device("crtc"), 0,data);
	}
	else
	{
		/* FIXME: this should be inside the MC6845 core! */
		if(addr_latch == 0x0e)
			cursor_addr = ((data<<8) & 0x3f00) | (cursor_addr & 0xff);
		else if(addr_latch == 0x0f)
			cursor_addr = (cursor_addr & 0x3f00) | (data & 0xff);

		//mc6845_register_w(space->machine->device("crtc"), 0,data);
	}
}

static READ8_HANDLER( smc777_vram_r )
{
	static UINT8 *vram = memory_region(space->machine, "vram");
	static UINT16 vram_index;

	vram_index = cpu_get_reg(space->machine->device("maincpu"), Z80_B);

	return vram[vram_index | offset*0x100];
}

static READ8_HANDLER( smc777_attr_r )
{
	static UINT8 *attr = memory_region(space->machine, "attr");
	static UINT16 vram_index;

	vram_index = cpu_get_reg(space->machine->device("maincpu"), Z80_B);

	return attr[vram_index | offset*0x100];
}

static READ8_HANDLER( smc777_pcg_r )
{
	static UINT8 *pcg = memory_region(space->machine, "pcg");
	static UINT16 vram_index;

	vram_index = cpu_get_reg(space->machine->device("maincpu"), Z80_B);

	return pcg[vram_index | offset*0x100];
}

static WRITE8_HANDLER( smc777_vram_w )
{
	static UINT8 *vram = memory_region(space->machine, "vram");
	static UINT16 vram_index;

	vram_index = cpu_get_reg(space->machine->device("maincpu"), Z80_B);

	vram[vram_index | offset*0x100] = data;
}

static WRITE8_HANDLER( smc777_attr_w )
{
	static UINT8 *attr = memory_region(space->machine, "attr");
	static UINT16 vram_index;

	vram_index = cpu_get_reg(space->machine->device("maincpu"), Z80_B);

	attr[vram_index | offset*0x100] = data;
}

static WRITE8_HANDLER( smc777_pcg_w )
{
	static UINT8 *pcg = memory_region(space->machine, "pcg");
	static UINT16 vram_index;

	vram_index = cpu_get_reg(space->machine->device("maincpu"), Z80_B);

	pcg[vram_index | offset*0x100] = data;

    gfx_element_mark_dirty(space->machine->gfx[0], vram_index >> 3);
}

static READ8_HANDLER( smc777_fbuf_r )
{
	static UINT8 *fbuf = memory_region(space->machine, "fbuf");
	static UINT16 vram_index;

	vram_index = cpu_get_reg(space->machine->device("maincpu"), Z80_B);

	return fbuf[vram_index | offset*0x100];
}

static WRITE8_HANDLER( smc777_fbuf_w )
{
	static UINT8 *fbuf = memory_region(space->machine, "fbuf");
	static UINT16 vram_index;

	vram_index = cpu_get_reg(space->machine->device("maincpu"), Z80_B);

	fbuf[vram_index | offset*0x100] = data;
}

static UINT8 fdc_irq_flag;
static UINT8 fdc_drq_flag;

static READ8_HANDLER( smc777_fdc_r )
{
	running_device* dev = space->machine->device("fdc");
	//UINT8 ret = 0;

	switch(offset+0x30)
	{
		case 0x30:
			return wd17xx_status_r(dev,offset);
		case 0x31:
			return wd17xx_track_r(dev,offset);
		case 0x32:
			return wd17xx_sector_r(dev,offset);
		case 0x33:
			return wd17xx_data_r(dev,offset);
		case 0x34: //irq / drq status
			return (fdc_irq_flag ? 0x80 : 0x00) | (fdc_drq_flag ? 0x00 : 0x40);
	}

	return 0x00;
}

static WRITE8_HANDLER( smc777_fdc_w )
{
	running_device* dev = space->machine->device("fdc");

	switch(offset+0x30)
	{
		case 0x30:
			wd17xx_command_w(dev,offset,data);
			break;
		case 0x31:
			wd17xx_track_w(dev,offset,data);
			break;
		case 0x32:
			wd17xx_sector_w(dev,offset,data);
			break;
		case 0x33:
			wd17xx_data_w(dev,offset,data);
			break;
		case 0x34:
		//	wd17xx_set_drive(dev,data & 3);
			/* TODO: understand the conditions of this */
			floppy_mon_w(floppy_get_device(space->machine, 0), !BIT(data, 7));
			floppy_drive_set_ready_state(floppy_get_device(space->machine, 0), 1,0);
		//	wd17xx_set_side(dev,(data & 0x10)>>4);
			if(data)
			printf("%02x\n",data);
			break;
	}
}

static WRITE_LINE_DEVICE_HANDLER( smc777_fdc_intrq_w )
{
	fdc_irq_flag = state;
}

static WRITE_LINE_DEVICE_HANDLER( smc777_fdc_drq_w )
{
	fdc_drq_flag = state;
}

static WRITE8_HANDLER( smc777_rom_w )
{
	static UINT8 *rom = memory_region(space->machine, "maincpu");

	rom[offset] = data;
}

#define keyb_press(_val_,_charset_) \
	if(input_code_pressed(space->machine, _val_)) \
		return _charset_ \

static READ8_HANDLER( key_r )
{
	static UINT8 res;

	if(offset == 1) //keyboard status
		return (0xfc) | keyb_press_flag;

	keyb_press_flag = 0;
	res = keyb_press;
//	keyb_press = 0xff;

	return res;
}

static WRITE8_HANDLER( border_col_w )
{
	backdrop_pen = data & 0xf;
}

static UINT8 system_data;

static READ8_HANDLER( system_input_r )
{
	return system_data;
}

static WRITE8_HANDLER( system_output_w )
{
	/*
	---x --- beep
	all the rest is unknown at current time
	*/
	system_data = data;
	beep_set_state(space->machine->device("beeper"),data & 0x10);
}

static READ8_HANDLER( unk_r )
{
	return 0;
}

static WRITE8_HANDLER( smc777_ramdac_w )
{
	static UINT8 pal_index,gradient_index,r,g,b;
	pal_index = cpu_get_reg(space->machine->device("maincpu"), Z80_B) & 0xf;
	gradient_index = (cpu_get_reg(space->machine->device("maincpu"), Z80_B) & 0x30) >> 4;

	switch(gradient_index)
	{
		case 0: r = data; palette_set_color_rgb(space->machine, pal_index+0x10, r,g,b); break;
		case 1: g = data; palette_set_color_rgb(space->machine, pal_index+0x10, r,g,b); break;
		case 2: b = data; palette_set_color_rgb(space->machine, pal_index+0x10, r,g,b); break;
	}
}

static ADDRESS_MAP_START(smc777_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x3fff) AM_ROM AM_WRITE(smc777_rom_w)
	AM_RANGE(0x4000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( smc777_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x07) AM_READWRITE(smc777_vram_r,smc777_vram_w)
	AM_RANGE(0x08, 0x0f) AM_READWRITE(smc777_attr_r,smc777_attr_w)
	AM_RANGE(0x10, 0x17) AM_READWRITE(smc777_pcg_r, smc777_pcg_w)
	AM_RANGE(0x18, 0x19) AM_WRITE(smc777_6845_w)
	AM_RANGE(0x1a, 0x1b) AM_READ(key_r) AM_WRITENOP//keyboard data
	AM_RANGE(0x1c, 0x1c) AM_READWRITE(system_input_r,system_output_w) //status and control data / Printer strobe
//	AM_RANGE(0x1d, 0x1d) AM_WRITENOP //status and control data / Printer status / strobe
//	AM_RANGE(0x1e, 0x1f) AM_WRITENOP //RS232C irq control
//	AM_RANGE(0x20, 0x20) AM_WRITENOP //display mode switching
//	AM_RANGE(0x21, 0x21) AM_WRITENOP //60 Hz irq control
//	AM_RANGE(0x22, 0x22) AM_WRITENOP //printer output data
	AM_RANGE(0x23, 0x23) AM_WRITE(border_col_w) //border area control
//	AM_RANGE(0x24, 0x24) AM_WRITENOP //Timer write / specify address (RTC?)
//	AM_RANGE(0x25, 0x25) AM_READNOP  //Timer read (RTC?)
//	AM_RANGE(0x26, 0x26) AM_WRITENOP //RS232C RX / TX
//	AM_RANGE(0x27, 0x27) AM_WRITENOP //RS232C Mode / Command / Status

//	AM_RANGE(0x28, 0x2c) AM_NOP //fdc 2, MB8876 -> FD1791
//	AM_RANGE(0x2d, 0x2f) AM_NOP //rs-232c no. 2
	AM_RANGE(0x30, 0x34) AM_READWRITE(smc777_fdc_r,smc777_fdc_w) //fdc 1, MB8876 -> FD1791
//	AM_RANGE(0x35, 0x37) AM_NOP //rs-232c no. 3
//	AM_RANGE(0x38, 0x3b) AM_NOP //cache disk unit
//	AM_RANGE(0x3c, 0x3d) AM_NOP //RGB Superimposer
//	AM_RANGE(0x40, 0x47) AM_NOP //IEEE-488 interface unit
//	AM_RANGE(0x48, 0x50) AM_NOP //HDD (Winchester)
	AM_RANGE(0x51, 0x51) AM_READ(unk_r)
	AM_RANGE(0x52, 0x52) AM_WRITE(smc777_ramdac_w)
//	AM_RANGE(0x54, 0x59) AM_NOP //VTR Controller
	AM_RANGE(0x53, 0x53) AM_DEVWRITE("sn1", sn76496_w) //not in the datasheet ... almost certainly SMC-777 specific
//	AM_RANGE(0x5a, 0x5b) AM_WRITENOP //RAM banking
//	AM_RANGE(0x70, 0x70) AM_NOP //Auto Start ROM
//	AM_RANGE(0x74, 0x74) AM_NOP //IEEE-488 ROM
//	AM_RANGE(0x75, 0x75) AM_NOP //VTR Controller ROM
	AM_RANGE(0x80, 0xff) AM_READWRITE(smc777_fbuf_r, smc777_fbuf_w) //GRAM
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( smc777 )
	PORT_START("key1") //0x00-0x1f
	PORT_BIT(0x00000001,IP_ACTIVE_HIGH,IPT_UNUSED) //0x00 null
	PORT_BIT(0x00000002,IP_ACTIVE_HIGH,IPT_UNUSED) //0x01 soh
	PORT_BIT(0x00000004,IP_ACTIVE_HIGH,IPT_UNUSED) //0x02 stx
	PORT_BIT(0x00000008,IP_ACTIVE_HIGH,IPT_UNUSED) //0x03 etx
	PORT_BIT(0x00000010,IP_ACTIVE_HIGH,IPT_UNUSED) //0x04 etx
	PORT_BIT(0x00000020,IP_ACTIVE_HIGH,IPT_UNUSED) //0x05 eot
	PORT_BIT(0x00000040,IP_ACTIVE_HIGH,IPT_UNUSED) //0x06 enq
	PORT_BIT(0x00000080,IP_ACTIVE_HIGH,IPT_UNUSED) //0x07 ack
	PORT_BIT(0x00000100,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Backspace") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT(0x00000200,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tab") PORT_CODE(KEYCODE_TAB) PORT_CHAR(9)
	PORT_BIT(0x00000400,IP_ACTIVE_HIGH,IPT_UNUSED) //0x0a
	PORT_BIT(0x00000800,IP_ACTIVE_HIGH,IPT_UNUSED) //0x0b lf
	PORT_BIT(0x00001000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x0c vt
	PORT_BIT(0x00002000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(27)
	PORT_BIT(0x00004000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x0e cr
	PORT_BIT(0x00008000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x0f so

	PORT_BIT(0x00010000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x10 si
	PORT_BIT(0x00020000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x11 dle
	PORT_BIT(0x00040000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x12 dc1
	PORT_BIT(0x00080000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x13 dc2
	PORT_BIT(0x00100000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x14 dc3
	PORT_BIT(0x00200000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x15 dc4
	PORT_BIT(0x00400000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x16 nak
	PORT_BIT(0x00800000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x17 syn
	PORT_BIT(0x01000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x18 etb
	PORT_BIT(0x02000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x19 cancel
	PORT_BIT(0x04000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x1a em
	PORT_BIT(0x08000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("ESC") PORT_CHAR(27)
	PORT_BIT(0x10000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x1c ???
	PORT_BIT(0x20000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x1d fs
	PORT_BIT(0x40000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x1e gs
	PORT_BIT(0x80000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x1f us

	PORT_START("key2") //0x20-0x3f
	PORT_BIT(0x00000001,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT(0x00000002,IP_ACTIVE_HIGH,IPT_UNUSED) //0x21 !
	PORT_BIT(0x00000004,IP_ACTIVE_HIGH,IPT_UNUSED) //0x22 "
	PORT_BIT(0x00000008,IP_ACTIVE_HIGH,IPT_UNUSED) //0x23 #
	PORT_BIT(0x00000010,IP_ACTIVE_HIGH,IPT_UNUSED) //0x24 $
	PORT_BIT(0x00000020,IP_ACTIVE_HIGH,IPT_UNUSED) //0x25 %
	PORT_BIT(0x00000040,IP_ACTIVE_HIGH,IPT_UNUSED) //0x26 &
	PORT_BIT(0x00000080,IP_ACTIVE_HIGH,IPT_UNUSED) //0x27 '
	PORT_BIT(0x00000100,IP_ACTIVE_HIGH,IPT_UNUSED) //0x28 (
	PORT_BIT(0x00000200,IP_ACTIVE_HIGH,IPT_UNUSED) //0x29 )
	PORT_BIT(0x00000400,IP_ACTIVE_HIGH,IPT_UNUSED) //0x2a *
	PORT_BIT(0x00000800,IP_ACTIVE_HIGH,IPT_UNUSED) //0x2b +
	PORT_BIT(0x00001000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x2c ,
	PORT_BIT(0x00002000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-')
	PORT_BIT(0x00004000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x2e .
	PORT_BIT(0x00008000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x2f /

	PORT_BIT(0x00010000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT(0x00020000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1) PORT_CHAR('1')
	PORT_BIT(0x00040000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2) PORT_CHAR('2')
	PORT_BIT(0x00080000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3) PORT_CHAR('3')
	PORT_BIT(0x00100000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4) PORT_CHAR('4')
	PORT_BIT(0x00200000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5) PORT_CHAR('5')
	PORT_BIT(0x00400000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6) PORT_CHAR('6')
	PORT_BIT(0x00800000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7) PORT_CHAR('7')
	PORT_BIT(0x01000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT(0x02000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9) PORT_CHAR('9')
	PORT_BIT(0x04000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(":") PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':')
	PORT_BIT(0x08000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(";") PORT_CODE(KEYCODE_COLON) PORT_CHAR(';')
	PORT_BIT(0x10000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x3c <
	PORT_BIT(0x20000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x3d =
	PORT_BIT(0x40000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x3e >
	PORT_BIT(0x80000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x3f ?

	PORT_START("key3") //0x40-0x5f
	PORT_BIT(0x00000001,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("@") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('@')
	PORT_BIT(0x00000002,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT(0x00000004,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT(0x00000008,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT(0x00000010,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT(0x00000020,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT(0x00000040,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT(0x00000080,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT(0x00000100,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT(0x00000200,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT(0x00000400,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT(0x00000800,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT(0x00001000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT(0x00002000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT(0x00004000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT(0x00008000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT(0x00010000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT(0x00020000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT(0x00040000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT(0x00080000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT(0x00100000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT(0x00200000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT(0x00400000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT(0x00800000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT(0x01000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT(0x02000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT(0x04000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT(0x08000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("[") PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR('[')
	PORT_BIT(0x10000000,IP_ACTIVE_HIGH,IPT_UNUSED)
	PORT_BIT(0x20000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("]") PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR(']')
	PORT_BIT(0x40000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("^") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('^')
	PORT_BIT(0x80000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("_")
INPUT_PORTS_END

static TIMER_CALLBACK( keyboard_callback )
{
	const char* portnames[3] = { "key1","key2","key3" };
	int i,port_i,scancode;
	//UINT8 keymod = input_port_read(machine,"key_modifiers") & 0x1f;
	scancode = 0;

	for(port_i=0;port_i<3;port_i++)
	{
		for(i=0;i<32;i++)
		{
			if((input_port_read(machine,portnames[port_i])>>i) & 1)
			{
				//key_flag = 1;
//				if(keymod & 0x02)  // shift not pressed
//				{
//					if(scancode >= 0x41 && scancode < 0x5a)
//						scancode += 0x20;  // lowercase
//				}
//				else
//				{
//					if(scancode >= 0x31 && scancode < 0x3a)
//						scancode -= 0x10;
//					if(scancode == 0x30)
//					{
//						scancode = 0x3d;
//					}
//				}
				keyb_press = scancode;
				keyb_press_flag = 1;
				return;
			}
			scancode++;
		}
	}
}

static MACHINE_START(smc777)
{
	timer_pulse(machine, ATTOTIME_IN_HZ(240/32), NULL, 0, keyboard_callback);
	beep_set_frequency(machine->device("beeper"),300); //guesswork
	beep_set_state(machine->device("beeper"),0);
}

static MACHINE_RESET(smc777)
{
}

static const gfx_layout smc777_charlayout =
{
	8, 8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static GFXDECODE_START( smc777 )
	GFXDECODE_ENTRY( "pcg", 0x0000, smc777_charlayout, 0, 8 )
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

#if 0
static const rgb_t defcolors[]=
{
	MAKE_RGB(0x00,0x00,0x00),
	MAKE_RGB(0x00,0x00,0xff),
	MAKE_RGB(0x00,0xff,0x00),
	MAKE_RGB(0x00,0xff,0xff),
	MAKE_RGB(0xff,0x00,0x00),
	MAKE_RGB(0xff,0x00,0xff),
	MAKE_RGB(0xff,0xff,0x00),
	MAKE_RGB(0xff,0xff,0xff),
	/* TODO: values extracted from WinSMC emulator, almost certainly not 100% accurate */
	MAKE_RGB(0x10,0x40,0x10), //dark green
	MAKE_RGB(0x10,0x70,0x20), //moss green
	MAKE_RGB(0xd0,0x50,0x20), //salmon
	MAKE_RGB(0xe0,0x90,0x20), //tan
	MAKE_RGB(0x10,0x50,0x80), //ash blue
	MAKE_RGB(0x10,0x90,0xe0), //light blue
	MAKE_RGB(0xf0,0x70,0x90), //pink
	MAKE_RGB(0x80,0x80,0x80) //gray
};
#endif

static PALETTE_INIT( smc777 )
{
	int i;

	for (i = 0x000; i < 0x010; i++)
	{
		UINT8 r,g,b;

		if((i & 0x01) == 0x01)
		{
			r = ((i >> 2) & 1) ? 0xff : 0x00;
			g = ((i >> 3) & 1) ? 0xff : 0x00;
			b = ((i >> 1) & 1) ? 0xff : 0x00;
		}
		else { r = g = b = 0x00; }

		palette_set_color_rgb(machine, i, r,g,b);
	}

//	for(i=0;i<0x10;i++)
//		palette_set_color(machine, 0x10+i,defcolors[i]);
}

static const wd17xx_interface smc777_mb8876_interface =
{
	DEVCB_NULL,
	DEVCB_LINE(smc777_fdc_intrq_w),
	DEVCB_LINE(smc777_fdc_drq_w),
	{FLOPPY_0, FLOPPY_1, NULL, NULL}
};

static FLOPPY_OPTIONS_START( smc777 )
	FLOPPY_OPTION( img, "img", "SMC70 disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([70])
		SECTORS([16])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

static const floppy_config smc777_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_SS_80,
	FLOPPY_OPTIONS_NAME(smc777),
	DO_NOT_KEEP_GEOMETRY,
	NULL
};

static MACHINE_DRIVER_START( smc777 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz) //4,028 Mhz!
    MDRV_CPU_PROGRAM_MAP(smc777_mem)
    MDRV_CPU_IO_MAP(smc777_io)

    MDRV_MACHINE_START(smc777)
    MDRV_MACHINE_RESET(smc777)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(60)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(0x400, 400)
    MDRV_SCREEN_VISIBLE_AREA(0, 660-1, 0, 220-1) //normal 640 x 200 + 20 pixels for border color
    MDRV_PALETTE_LENGTH(0x20)
    MDRV_PALETTE_INIT(smc777)
	MDRV_GFXDECODE(smc777)

	MDRV_MC6845_ADD("crtc", H46505, XTAL_3_579545MHz/2, mc6845_intf)	/* unknown clock, hand tuned to get ~60 fps */

    MDRV_VIDEO_START(smc777)
    MDRV_VIDEO_UPDATE(smc777)

	MDRV_MB8877_ADD("fdc",smc777_mb8876_interface)
	MDRV_FLOPPY_2_DRIVES_ADD(smc777_floppy_config)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD("sn1", SN76489A, 1996800) // unknown clock / divider
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_ADD("beeper", BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS,"mono",0.50)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( smc777 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "smcrom.dat", 0x0000, 0x4000, CRC(b2520d31) SHA1(3c24b742c38bbaac85c0409652ba36e20f4687a1))

    ROM_REGION( 0x800, "vram", ROMREGION_ERASE00 )

    ROM_REGION( 0x800, "attr", ROMREGION_ERASE00 )

    ROM_REGION( 0x800, "pcg", ROMREGION_ERASE00 )

    ROM_REGION( 0x8000,"fbuf", ROMREGION_ERASE00 )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1983, smc777,  0,       0, 	smc777, 	smc777, 	 0,  "Sony",   "SMC-777",		GAME_NOT_WORKING | GAME_IMPERFECT_SOUND)

