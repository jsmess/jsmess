/*************************************************************************************

    SMC-777 (c) 1983 Sony

    driver by Angelo Salese

    TODO:
    - no real documentation, the entire driver is just a bunch of educated
      guesses ...
	- ROM/RAM bankswitch, it apparently happens after one instruction prefetching.
	  We currently use an hackish implementation until the MAME/MESS framework can
	  support that ...
	- keyboard input is very sluggish;
	- cursor stuck in Bird Crash;
	- add mc6845 features;
	- many other missing features;

**************************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "sound/sn76496.h"
#include "sound/beep.h"
#include "video/mc6845.h"

#include "machine/wd17xx.h"
#include "formats/basicdsk.h"
#include "imagedev/flopdrv.h"


class smc777_state : public driver_device
{
public:
	smc777_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT16 cursor_addr;
	UINT16 cursor_raster;
	UINT8 keyb_press;
	UINT8 keyb_press_flag;
	UINT8 backdrop_pen;
	UINT8 display_reg;
	int addr_latch;
	UINT8 fdc_irq_flag;
	UINT8 fdc_drq_flag;
	UINT8 system_data;
	struct { UINT8 r,g,b; } pal;
	UINT8 raminh,raminh_pending_change; //bankswitch
	UINT8 raminh_prefetch;
	UINT8 irq_mask;
	UINT8 keyb_direct;
	UINT8 pal_mode;
};



#define CRTC_MIN_X 10
#define CRTC_MIN_Y 10

static VIDEO_START( smc777 )
{
}

static VIDEO_UPDATE( smc777 )
{
	smc777_state *state = screen->machine->driver_data<smc777_state>();
	int x,y,yi;
	UINT16 count;
	UINT8 *vram = screen->machine->region("vram")->base();
	UINT8 *attr = screen->machine->region("attr")->base();
	UINT8 *gram = screen->machine->region("fbuf")->base();
	int x_width;

	bitmap_fill(bitmap, cliprect, screen->machine->pens[state->backdrop_pen]);

	x_width = (state->display_reg & 0x80) ? 2 : 4;

	count = 0x0000;

	for(yi=0;yi<8;yi++)
	{
		for(y=0;y<200;y+=8)
		{
			for(x=0;x<160;x++)
			{
				UINT16 color;

				color = (gram[count] & 0xf0) >> 4;
				/* todo: clean this up! */
				if(x_width == 2)
				{
					*BITMAP_ADDR16(bitmap, y+yi+CRTC_MIN_Y, x*2+0+CRTC_MIN_X) = screen->machine->pens[color];
				}
				else
				{
					*BITMAP_ADDR16(bitmap, y+yi+CRTC_MIN_Y, x*4+0+CRTC_MIN_X) = screen->machine->pens[color];
					*BITMAP_ADDR16(bitmap, y+yi+CRTC_MIN_Y, x*4+1+CRTC_MIN_X) = screen->machine->pens[color];
				}

				color = (gram[count] & 0x0f) >> 0;
				if(x_width == 2)
				{
					*BITMAP_ADDR16(bitmap, y+yi+CRTC_MIN_Y, x*2+1+CRTC_MIN_X) = screen->machine->pens[color];
				}
				else
				{
					*BITMAP_ADDR16(bitmap, y+yi+CRTC_MIN_Y, x*4+2+CRTC_MIN_X) = screen->machine->pens[color];
					*BITMAP_ADDR16(bitmap, y+yi+CRTC_MIN_Y, x*4+3+CRTC_MIN_X) = screen->machine->pens[color];
				}

				count++;

			}
		}
		count+= 0x60;
	}

	count = 0x0000;

	x_width = (state->display_reg & 0x80) ? 40 : 80;

	for(y=0;y<25;y++)
	{
		for(x=0;x<x_width;x++)
		{
			/*
			-x-- ---- blink
			--xx x--- bg color (00 transparent, 01 white, 10 black, 11 complementary to fg color
			---- -xxx fg color
			*/
			int tile = vram[count];
			int color = attr[count] & 7;
			int bk_color = (attr[count] & 0x18) >> 3;
			int blink = attr[count] & 0x40;
			int yi,xi;
			int bk_pen;
			//int bk_struct[4] = { -1, 0x10, 0x11, (color & 7) ^ 8 };

			bk_pen = -1;
			switch(bk_color & 3)
			{
				case 0: bk_pen = -1; break; //transparent
				case 1: bk_pen = 0x17; break; //white
				case 2: bk_pen = 0x10; break; //black
				case 3: bk_pen = (color ^ 0xf); break; //complementary
			}

			if(blink && screen->machine->primary_screen->frame_number() & 0x10) //blinking, used by Dragon's Alphabet
				color = bk_pen;

			for(yi=0;yi<8;yi++)
			{
				for(xi=0;xi<8;xi++)
				{
					UINT8 *gfx_data = screen->machine->region("pcg")->base();
					int pen;

					pen = ((gfx_data[tile*8+yi]>>(7-xi)) & 1) ? (color+state->pal_mode) : bk_pen;

					if(pen != -1)
						*BITMAP_ADDR16(bitmap, y*8+CRTC_MIN_Y+yi, x*8+CRTC_MIN_X+xi) = screen->machine->pens[pen];
				}
			}

			// draw cursor
			if(state->cursor_addr == count)
			{
				int xc,yc,cursor_on;

				cursor_on = 0;
				switch(state->cursor_raster & 0x60)
				{
					case 0x00: cursor_on = 1; break; //always on
					case 0x20: cursor_on = 0; break; //always off
					case 0x40: if(screen->machine->primary_screen->frame_number() & 0x10) { cursor_on = 1; } break; //fast blink
					case 0x60: if(screen->machine->primary_screen->frame_number() & 0x20) { cursor_on = 1; } break; //slow blink
				}

				if(cursor_on)
				{
					for(yc=0;yc<(8-(state->cursor_raster & 7));yc++)
					{
						for(xc=0;xc<8;xc++)
						{
							*BITMAP_ADDR16(bitmap, y*8+CRTC_MIN_Y-yc+7, x*8+CRTC_MIN_X+xc) = screen->machine->pens[0x7];
						}
					}
				}
			}

			(state->display_reg & 0x80) ? count+=2 : count++;
		}
	}

    return 0;
}

static WRITE8_HANDLER( smc777_6845_w )
{
	smc777_state *state = space->machine->driver_data<smc777_state>();
	if(offset == 0)
	{
		state->addr_latch = data;
		//mc6845_address_w(space->machine->device("crtc"), 0,data);
	}
	else
	{
		if(state->addr_latch == 0x0a)
			state->cursor_raster = data;
		else if(state->addr_latch == 0x0e)
			state->cursor_addr = ((data<<8) & 0x3f00) | (state->cursor_addr & 0xff);
		else if(state->addr_latch == 0x0f)
			state->cursor_addr = (state->cursor_addr & 0x3f00) | (data & 0xff);

		//mc6845_register_w(space->machine->device("crtc"), 0,data);
	}
}

static READ8_HANDLER( smc777_vram_r )
{
	UINT8 *vram = space->machine->region("vram")->base();
	UINT16 vram_index;

	vram_index  = ((offset & 0x0007) << 8);
	vram_index |= ((offset & 0xff00) >> 8);

	return vram[vram_index];
}

static READ8_HANDLER( smc777_attr_r )
{
	UINT8 *attr = space->machine->region("attr")->base();
	UINT16 vram_index;

	vram_index  = ((offset & 0x0007) << 8);
	vram_index |= ((offset & 0xff00) >> 8);

	return attr[vram_index];
}

static READ8_HANDLER( smc777_pcg_r )
{
	UINT8 *pcg = space->machine->region("pcg")->base();
	UINT16 vram_index;

	vram_index  = ((offset & 0x0007) << 8);
	vram_index |= ((offset & 0xff00) >> 8);

	return pcg[vram_index];
}

static WRITE8_HANDLER( smc777_vram_w )
{
	UINT8 *vram = space->machine->region("vram")->base();
	UINT16 vram_index;

	vram_index  = ((offset & 0x0007) << 8);
	vram_index |= ((offset & 0xff00) >> 8);

	vram[vram_index] = data;
}

static WRITE8_HANDLER( smc777_attr_w )
{
	UINT8 *attr = space->machine->region("attr")->base();
	UINT16 vram_index;

	vram_index  = ((offset & 0x0007) << 8);
	vram_index |= ((offset & 0xff00) >> 8);

	attr[vram_index] = data;
}

static WRITE8_HANDLER( smc777_pcg_w )
{
	UINT8 *pcg = space->machine->region("pcg")->base();
	UINT16 vram_index;

	vram_index  = ((offset & 0x0007) << 8);
	vram_index |= ((offset & 0xff00) >> 8);

	pcg[vram_index] = data;

    gfx_element_mark_dirty(space->machine->gfx[0], vram_index >> 3);
}

static READ8_HANDLER( smc777_fbuf_r )
{
	UINT8 *fbuf = space->machine->region("fbuf")->base();
	UINT16 vram_index;

	vram_index  = ((offset & 0x007f) << 8);
	vram_index |= ((offset & 0xff00) >> 8);

	return fbuf[vram_index];
}

static WRITE8_HANDLER( smc777_fbuf_w )
{
	UINT8 *fbuf = space->machine->region("fbuf")->base();
	UINT16 vram_index;

	vram_index  = ((offset & 0x00ff) << 8);
	vram_index |= ((offset & 0xff00) >> 8);

	fbuf[vram_index] = data;
}


static void check_floppy_inserted(running_machine *machine)
{
	int f_num;
	floppy_image *floppy;

	/* check if a floppy is there, automatically disconnect the ready line if so (HW doesn't control the ready line) */
	/* FIXME: floppy drive 1 doesn't work? */
	for(f_num=0;f_num<2;f_num++)
	{
		floppy = flopimg_get_image(floppy_get_device(machine, f_num));
		floppy_mon_w(floppy_get_device(machine, f_num), (floppy != NULL) ? 0 : 1);
		floppy_drive_set_ready_state(floppy_get_device(machine, f_num), (floppy != NULL) ? 1 : 0,0);
	}
}

static READ8_HANDLER( smc777_fdc1_r )
{
	smc777_state *state = space->machine->driver_data<smc777_state>();
	device_t* dev = space->machine->device("fdc");

	check_floppy_inserted(space->machine);

	switch(offset)
	{
		case 0x00:
			return wd17xx_status_r(dev,offset);
		case 0x01:
			return wd17xx_track_r(dev,offset);
		case 0x02:
			return wd17xx_sector_r(dev,offset);
		case 0x03:
			return wd17xx_data_r(dev,offset);
		case 0x04: //irq / drq status
			//popmessage("%02x %02x\n",state->fdc_irq_flag,state->fdc_drq_flag);

			return (state->fdc_irq_flag ? 0x80 : 0x00) | (state->fdc_drq_flag ? 0x00 : 0x40);
	}

	return 0x00;
}

static WRITE8_HANDLER( smc777_fdc1_w )
{
	device_t* dev = space->machine->device("fdc");

	check_floppy_inserted(space->machine);

	switch(offset)
	{
		case 0x00:
			wd17xx_command_w(dev,offset,data);
			break;
		case 0x01:
			wd17xx_track_w(dev,offset,data);
			break;
		case 0x02:
			wd17xx_sector_w(dev,offset,data);
			break;
		case 0x03:
			wd17xx_data_w(dev,offset,data);
			break;
		case 0x04:
			// ---- xxxx select floppy drive (yes, 15 of them, A to P)
			wd17xx_set_drive(dev,data & 0x01);
			//  wd17xx_set_side(dev,(data & 0x10)>>4);
			if(data & 0xf0)
				printf("floppy access %02x\n",data);
			break;
	}
}

static WRITE_LINE_DEVICE_HANDLER( smc777_fdc_intrq_w )
{
	smc777_state *drvstate = device->machine->driver_data<smc777_state>();
	drvstate->fdc_irq_flag = state;
}

static WRITE_LINE_DEVICE_HANDLER( smc777_fdc_drq_w )
{
	smc777_state *drvstate = device->machine->driver_data<smc777_state>();
	drvstate->fdc_drq_flag = state;
}


static READ8_HANDLER( key_r )
{
	smc777_state *state = space->machine->driver_data<smc777_state>();
	UINT8 res;

	if(offset == 1) //keyboard status
		return (0x3c) | state->keyb_press_flag; //bit 6 or 7 is probably key repeat

	state->keyb_press_flag = 0;
	res = state->keyb_press;
//  state->keyb_press = 0xff;

	return res;
}

static WRITE8_HANDLER( border_col_w )
{
	smc777_state *state = space->machine->driver_data<smc777_state>();
	if(data & 0xf0)
		printf("Special border color enabled %02x\n",data);

	state->backdrop_pen = data & 0xf;
}


static READ8_HANDLER( system_input_r )
{
	smc777_state *state = space->machine->driver_data<smc777_state>();

	printf("System FF R %02x\n",state->system_data & 0x0f);

	switch(state->system_data & 0x0f)
	{
		case 0x00:
			return ((state->raminh & 1) << 4); //unknown bit, Dragon's Alphabet and Bird Crush relies on this for correct colors
	}

	return state->system_data;
}



static WRITE8_HANDLER( system_output_w )
{
	smc777_state *state = space->machine->driver_data<smc777_state>();
	/*
	---x 0000 ram inibit signal
    ---x 1001 beep
    all the rest is unknown at current time
    */
	state->system_data = data;
	switch(state->system_data & 0x0f)
	{
		case 0x00:
			state->raminh_pending_change = ((data & 0x10) >> 4) ^ 1;
			state->raminh_prefetch = (UINT8)(cpu_get_reg(space->cpu, Z80_R)) & 0x7f;
			break;
		case 0x02: printf("Interlace %s\n",data & 0x10 ? "on" : "off"); break;
		case 0x05: beep_set_state(space->machine->device("beeper"),data & 0x10); break;
		default: printf("System FF W %02x\n",data); break;
	}
}

/* presumably SMC-777 specific */
static READ8_HANDLER( smc777_joystick_r )
{
	//smc777_state *state = space->machine->driver_data<smc777_state>();

	return input_port_read(space->machine, "JOY_1P");
}

static WRITE8_HANDLER( smc777_color_mode_w )
{
	smc777_state *state = space->machine->driver_data<smc777_state>();

	switch(data & 0x0f)
	{
		case 0x06: state->pal_mode = (data & 0x10) ^ 0x10; break;
		default: printf("Color FF %02x\n",data); break;
	}
}

static WRITE8_HANDLER( smc777_ramdac_w )
{
	smc777_state *state = space->machine->driver_data<smc777_state>();
	UINT8 pal_index;
	pal_index = (offset & 0xf00) >> 8;

	if(data & 0x0f)
		printf("RAMdac used with data bits 0-3 set (%02x)\n",data);

	switch((offset & 0x3000) >> 12)
	{
		case 0: state->pal.r = (data & 0xf0) >> 4; palette_set_color_rgb(space->machine, pal_index, pal4bit(state->pal.r), pal4bit(state->pal.g), pal4bit(state->pal.b)); break;
		case 1: state->pal.g = (data & 0xf0) >> 4; palette_set_color_rgb(space->machine, pal_index, pal4bit(state->pal.r), pal4bit(state->pal.g), pal4bit(state->pal.b)); break;
		case 2: state->pal.b = (data & 0xf0) >> 4; palette_set_color_rgb(space->machine, pal_index, pal4bit(state->pal.r), pal4bit(state->pal.g), pal4bit(state->pal.b)); break;
		case 3: printf("RAMdac used with gradient index = 3! pal_index = %02x data = %02x\n",pal_index,data); break;
	}
}

static READ8_HANDLER( display_reg_r )
{
	smc777_state *state = space->machine->driver_data<smc777_state>();
	return state->display_reg;
}

/* x */
static WRITE8_HANDLER( display_reg_w )
{
	smc777_state *state = space->machine->driver_data<smc777_state>();
	/*
    x--- ---- width 80 / 40 switch (0 = 640 x 200 1 = 320 x 200)
    ---- -x-- mode select?
    */

	{
		if((state->display_reg & 0x80) != (data & 0x80))
		{
			rectangle visarea = space->machine->primary_screen->visible_area();
			int x_width;

			x_width = (data & 0x80) ? 320 : 640;

			visarea.min_x = visarea.min_y = 0;
			visarea.max_y = (200+(CRTC_MIN_Y*2)) - 1;
			visarea.max_x = (x_width+(CRTC_MIN_X*2)) - 1;

			space->machine->primary_screen->configure(660, 220, visarea, space->machine->primary_screen->frame_period().attoseconds);
		}
	}

	state->display_reg = data;
}

static READ8_HANDLER( smc777_mem_r )
{
	smc777_state *state = space->machine->driver_data<smc777_state>();
	UINT8 *wram = space->machine->region("wram")->base();
	UINT8 *bios = space->machine->region("bios")->base();
	UINT8 z80_r;

	if(state->raminh_prefetch != 0xff) //do the bankswitch AFTER that the prefetch instruction is executed (FIXME: this is an hackish implementation)
	{
		z80_r = (UINT8)cpu_get_reg(space->cpu, Z80_R);

		if(z80_r == ((state->raminh_prefetch+2) & 0x7f))
		{
			state->raminh = state->raminh_pending_change;
			state->raminh_prefetch = 0xff;
		}
	}

	if(state->raminh == 1 && ((offset & 0xc000) == 0))
		return bios[offset];

	return wram[offset];
}

static WRITE8_HANDLER( smc777_mem_w )
{
	//smc777_state *state = space->machine->driver_data<smc777_state>();
	UINT8 *wram = space->machine->region("wram")->base();

	wram[offset] = data;
}

static READ8_HANDLER( smc777_irq_mask_r )
{
	smc777_state *state = space->machine->driver_data<smc777_state>();

	return state->irq_mask;
}

static WRITE8_HANDLER( smc777_irq_mask_w )
{
	smc777_state *state = space->machine->driver_data<smc777_state>();

	if(data & 0xfe)
		printf("Irq mask = %02x\n",data & 0xfe);

	state->irq_mask = data & 1;
}

static ADDRESS_MAP_START(smc777_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xffff) AM_READWRITE(smc777_mem_r,smc777_mem_w)
ADDRESS_MAP_END

static READ8_HANDLER( smc777_io_r )
{
	UINT8 low_offs;

	low_offs = offset & 0xff;

	if(low_offs <= 0x07) 						  { return smc777_vram_r(space,offset & 0xff07); }
	else if(low_offs >= 0x08 && low_offs <= 0x0f) { return smc777_attr_r(space,offset & 0xff07); }
	else if(low_offs >= 0x10 && low_offs <= 0x17) { return smc777_pcg_r(space,offset & 0xff07); }
	else if(low_offs >= 0x18 && low_offs <= 0x19) { logerror("6845 read %02x",low_offs & 1); }
	else if(low_offs >= 0x1a && low_offs <= 0x1b) { return key_r(space,low_offs & 1); }
	else if(low_offs == 0x1c)					  { return system_input_r(space,0); }
	else if(low_offs == 0x1d)					  { logerror("System and control data R PC=%04x\n",cpu_get_pc(space->cpu)); return 0xff; }
	else if(low_offs == 0x20)					  { return display_reg_r(space,0); }
	else if(low_offs == 0x21)					  { return smc777_irq_mask_r(space,0); }
	else if(low_offs == 0x25)					  { logerror("RTC read PC=%04x\n",cpu_get_pc(space->cpu)); return 0xff; }
	else if(low_offs == 0x26)					  { logerror("RS-232c RX %04x\n",cpu_get_pc(space->cpu)); return 0xff; }
	else if(low_offs >= 0x28 && low_offs <= 0x2c) { logerror("FDC 2 read %02x\n",low_offs & 7); return 0xff; }
	else if(low_offs >= 0x2d && low_offs <= 0x2f) { logerror("RS-232c no. 2 read %02x\n",low_offs & 3); return 0xff; }
	else if(low_offs >= 0x30 && low_offs <= 0x34) { return smc777_fdc1_r(space,low_offs & 7); }
	else if(low_offs >= 0x35 && low_offs <= 0x37) { logerror("RS-232c no. 3 read %02x\n",low_offs & 3); return 0xff; }
	else if(low_offs >= 0x38 && low_offs <= 0x3b) { logerror("Cache disk unit read %02x\n",low_offs & 7); return 0xff; }
	else if(low_offs >= 0x3c && low_offs <= 0x3d) { logerror("RGB superimposer read %02x\n",low_offs & 1); return 0xff; }
	else if(low_offs >= 0x40 && low_offs <= 0x47) { logerror("IEEE-488 interface unit read %02x\n",low_offs & 7); return 0xff; }
	else if(low_offs >= 0x48 && low_offs <= 0x4f) { logerror("HDD (Winchester) read %02x\n",low_offs & 1); return 0xff; } //might be 0x48 - 0x50
	else if(low_offs == 0x51)					  { return smc777_joystick_r(space,0); }
	else if(low_offs >= 0x54 && low_offs <= 0x59) { logerror("VTR Controller read %02x\n",low_offs & 7); return 0xff; }
	else if(low_offs == 0x5a || low_offs == 0x5b) { logerror("RAM Banking %02x\n",low_offs & 1); }
	else if(low_offs == 0x70)					  { logerror("Auto-start ROM read\n"); }
	else if(low_offs == 0x74)					  { logerror("IEEE-488 ROM read\n"); }
	else if(low_offs == 0x75)					  { logerror("VTR Controller ROM read\n"); }
	else if(low_offs == 0x7e || low_offs == 0x7f) { logerror("Kanji ROM read %02x\n",low_offs & 1); }
	else if(low_offs >= 0x80) 					  { return smc777_fbuf_r(space,offset & 0xff7f); }

	logerror("Undefined read at %04x offset = %02x\n",cpu_get_pc(space->cpu),low_offs);
	return 0xff;
}

static WRITE8_HANDLER( smc777_io_w )
{
	UINT8 low_offs;

	low_offs = offset & 0xff;

	if(low_offs <= 0x07) 	 					  { smc777_vram_w(space,offset & 0xff07,data); }
	else if(low_offs >= 0x08 && low_offs <= 0x0f) { smc777_attr_w(space,offset & 0xff07,data); }
	else if(low_offs >= 0x10 && low_offs <= 0x17) { smc777_pcg_w(space,offset & 0xff07,data); }
	else if(low_offs >= 0x18 && low_offs <= 0x19) { smc777_6845_w(space,low_offs & 1,data); }
	else if(low_offs == 0x1a || low_offs == 0x1b) { logerror("Keyboard write %02x [%02x]\n",data,low_offs & 1); }
	else if(low_offs == 0x1c)					  { system_output_w(space,0,data); }
	else if(low_offs == 0x1d)					  { logerror("Printer status / strobe write %02x\n",data); }
	else if(low_offs == 0x1e || low_offs == 0x1f) { logerror("RS-232C irq control [%02x] %02x\n",low_offs & 1,data); }
	else if(low_offs == 0x20)					  { display_reg_w(space,0,data); }
	else if(low_offs == 0x21)					  { smc777_irq_mask_w(space,0,data); }
	else if(low_offs == 0x22)					  { logerror("Printer output data %02x\n",data); }
	else if(low_offs == 0x23)					  { border_col_w(space,0,data); }
	else if(low_offs == 0x24)					  { logerror("RTC write / specify address %02x\n",data); }
	else if(low_offs == 0x26)					  { logerror("RS-232c TX %02x\n",data); }
	else if(low_offs >= 0x28 && low_offs <= 0x2c) { logerror("FDC 2 write %02x %02x\n",low_offs & 7,data); }
	else if(low_offs >= 0x2d && low_offs <= 0x2f) { logerror("RS-232c no. 2 write %02x %02x\n",low_offs & 3,data); }
	else if(low_offs >= 0x30 && low_offs <= 0x34) { smc777_fdc1_w(space,low_offs & 7,data); }
	else if(low_offs >= 0x35 && low_offs <= 0x37) { logerror("RS-232c no. 3 write %02x %02x\n",low_offs & 3,data); }
	else if(low_offs >= 0x38 && low_offs <= 0x3b) { logerror("Cache disk unit write %02x %02x\n",low_offs & 7,data); }
	else if(low_offs >= 0x3c && low_offs <= 0x3d) { logerror("RGB superimposer write %02x %02x\n",low_offs & 1,data); }
	else if(low_offs >= 0x40 && low_offs <= 0x47) { logerror("IEEE-488 interface unit write %02x %02x\n",low_offs & 7,data); }
	else if(low_offs >= 0x48 && low_offs <= 0x4f) { logerror("HDD (Winchester) write %02x %02x\n",low_offs & 1,data); } //might be 0x48 - 0x50
	else if(low_offs == 0x51)					  { smc777_color_mode_w(space,0,data); }
	else if(low_offs == 0x52)					  { smc777_ramdac_w(space,offset & 0xff00,data); }
	else if(low_offs == 0x53)					  { sn76496_w(space->machine->device("sn1"),0,data); }
	else if(low_offs >= 0x54 && low_offs <= 0x59) { logerror("VTR Controller write [%02x] %02x\n",low_offs & 7,data); }
	else if(low_offs == 0x5a || low_offs == 0x5b) { logerror("RAM Banking write [%02x] %02x\n",low_offs & 1,data); }
	else if(low_offs == 0x70)					  { logerror("Auto-start ROM write %02x\n",data); }
	else if(low_offs == 0x74)					  { logerror("IEEE-488 ROM write %02x\n",data); }
	else if(low_offs == 0x75)					  { logerror("VTR Controller ROM write %02x\n",data); }
	else if(low_offs == 0x7e || low_offs == 0x7f) { logerror("Kanji ROM write [%02x] %02x\n",low_offs & 1,data); }
	else if(low_offs >= 0x80) 					  { smc777_fbuf_w(space,offset & 0xff7f,data); }
	else 										  { logerror("Undefined write at %04x offset = %02x data = %02x\n",cpu_get_pc(space->cpu),low_offs,data); }
}

static ADDRESS_MAP_START( smc777_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xffff) AM_READWRITE(smc777_io_r,smc777_io_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( smc777 )
	PORT_START("key1") //0x00-0x1f
	PORT_BIT(0x00000001,IP_ACTIVE_HIGH,IPT_UNUSED) //0x00 null
	PORT_BIT(0x00000002,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F1") PORT_CODE(KEYCODE_F1)
	PORT_BIT(0x00000004,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F2") PORT_CODE(KEYCODE_F2)
	PORT_BIT(0x00000008,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F4") PORT_CODE(KEYCODE_F4)
	PORT_BIT(0x00000010,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F3") PORT_CODE(KEYCODE_F3)
	PORT_BIT(0x00000020,IP_ACTIVE_HIGH,IPT_UNUSED) //0x05 eot
	PORT_BIT(0x00000040,IP_ACTIVE_HIGH,IPT_UNUSED) //0x06 enq
	PORT_BIT(0x00000080,IP_ACTIVE_HIGH,IPT_UNUSED) //0x07 ack
	PORT_BIT(0x00000100,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Backspace") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT(0x00000200,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tab") PORT_CODE(KEYCODE_TAB) PORT_CHAR(9)
	PORT_BIT(0x00000400,IP_ACTIVE_HIGH,IPT_UNUSED) //0x0a
	PORT_BIT(0x00000800,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F5") PORT_CODE(KEYCODE_F5)
	PORT_BIT(0x00001000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x0c vt
	PORT_BIT(0x00002000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(27)
	PORT_BIT(0x00004000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x0e cr
	PORT_BIT(0x00008000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x0f so

	PORT_BIT(0x00010000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x10 si
	PORT_BIT(0x00020000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x11 dle
	PORT_BIT(0x00040000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x12 dc1
	PORT_BIT(0x00080000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x13 dc2
	PORT_BIT(0x00100000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("HOME") PORT_CODE(KEYCODE_HOME)
	PORT_BIT(0x00200000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x15 dc4
	PORT_BIT(0x00400000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT)
	PORT_BIT(0x00800000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Up") PORT_CODE(KEYCODE_UP)
	PORT_BIT(0x01000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x18 etb
	PORT_BIT(0x02000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT)
	PORT_BIT(0x04000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x1a em
	PORT_BIT(0x08000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("ESC") PORT_CHAR(27)
	PORT_BIT(0x10000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN)
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

	PORT_START("JOY_1P")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH,IPT_UNKNOWN ) //status?
INPUT_PORTS_END

static TIMER_DEVICE_CALLBACK( keyboard_callback )
{
	smc777_state *state = timer.machine->driver_data<smc777_state>();
	static const char *const portnames[3] = { "key1","key2","key3" };
	int i,port_i,scancode;
	//UINT8 keymod = input_port_read(timer.machine,"key_modifiers") & 0x1f;
	scancode = 0;

	for(port_i=0;port_i<3;port_i++)
	{
		for(i=0;i<32;i++)
		{
			if((input_port_read(timer.machine,portnames[port_i])>>i) & 1)
			{
				//key_flag = 1;
//              if(keymod & 0x02)  // shift not pressed
//              {
//                  if(scancode >= 0x41 && scancode < 0x5a)
//                      scancode += 0x20;  // lowercase
//              }
//              else
//              {
//                  if(scancode >= 0x31 && scancode < 0x3a)
//                      scancode -= 0x10;
//                  if(scancode == 0x30)
//                  {
//                      scancode = 0x3d;
//                  }
//              }
				state->keyb_press = scancode;
				state->keyb_press_flag = 1;
				return;
			}
			scancode++;
		}
	}
}

static MACHINE_START(smc777)
{
	//smc777_state *state = machine->driver_data<smc777_state>();


	beep_set_frequency(machine->device("beeper"),300); //guesswork
	beep_set_state(machine->device("beeper"),0);
}

static MACHINE_RESET(smc777)
{
	smc777_state *state = machine->driver_data<smc777_state>();

	state->raminh = 1;
	state->raminh_pending_change = 1;
	state->raminh_prefetch = 0xff;
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

static PALETTE_INIT( smc777 )
{
	int i;

	for(i=0x10;i<0x18;i++)
	{
		UINT8 r,g,b;

		r = (i & 4) >> 2;
		g = (i & 2) >> 1;
		b = (i & 1) >> 0;

		palette_set_color_rgb(machine, i, pal1bit(r),pal1bit(g),pal1bit(b));
	}
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
	FLOPPY_STANDARD_5_25_SSDD,
	FLOPPY_OPTIONS_NAME(smc777),
	NULL
};

static INTERRUPT_GEN( smc777_vblank_irq )
{
	smc777_state *state = device->machine->driver_data<smc777_state>();

	if(state->irq_mask)
		cpu_set_input_line(device,0,HOLD_LINE);
}

#define MASTER_CLOCK XTAL_4_028MHz

static MACHINE_CONFIG_START( smc777, smc777_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",Z80, MASTER_CLOCK)
    MCFG_CPU_PROGRAM_MAP(smc777_mem)
    MCFG_CPU_IO_MAP(smc777_io)
	MCFG_CPU_VBLANK_INT("screen",smc777_vblank_irq)

    MCFG_MACHINE_START(smc777)
    MCFG_MACHINE_RESET(smc777)

    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(60)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(0x400, 400)
    MCFG_SCREEN_VISIBLE_AREA(0, 660-1, 0, 220-1) //normal 640 x 200 + 20 pixels for border color
    MCFG_PALETTE_LENGTH(0x10+8) //16 palette entries + 8 special colors
    MCFG_PALETTE_INIT(smc777)
	MCFG_GFXDECODE(smc777)

	MCFG_MC6845_ADD("crtc", H46505, MASTER_CLOCK/2, mc6845_intf)	/* unknown clock, hand tuned to get ~60 fps */

    MCFG_VIDEO_START(smc777)
    MCFG_VIDEO_UPDATE(smc777)

	MCFG_WD179X_ADD("fdc",smc777_mb8876_interface)
	MCFG_FLOPPY_2_DRIVES_ADD(smc777_floppy_config)

	MCFG_SPEAKER_STANDARD_MONO("mono")

	MCFG_SOUND_ADD("sn1", SN76489A, MASTER_CLOCK) // unknown clock / divider
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MCFG_SOUND_ADD("beeper", BEEP, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS,"mono",0.50)

	MCFG_TIMER_ADD_PERIODIC("keyboard_timer", keyboard_callback, attotime::from_hz(240/32))
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( smc777 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )

	/* shadow ROM */
    ROM_REGION( 0x10000, "bios", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS(0, "1st", "1st rev.")
	ROMX_LOAD( "smcrom.dat", 0x0000, 0x4000, CRC(b2520d31) SHA1(3c24b742c38bbaac85c0409652ba36e20f4687a1), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "2nd", "2nd rev.")
	ROMX_LOAD( "smcrom.v2",  0x0000, 0x4000, CRC(c1494b8f) SHA1(a7396f5c292f11639ffbf0b909e8473c5aa63518), ROM_BIOS(2))

	ROM_REGION( 0x800, "mcu", ROMREGION_ERASEFF )
	ROM_LOAD( "i80xx", 0x000, 0x800, NO_DUMP ) // keyboard mcu, needs decapping

    ROM_REGION( 0x10000, "wram", ROMREGION_ERASE00 )

    ROM_REGION( 0x800, "vram", ROMREGION_ERASE00 )

    ROM_REGION( 0x800, "attr", ROMREGION_ERASE00 )

    ROM_REGION( 0x800, "pcg", ROMREGION_ERASE00 )

    ROM_REGION( 0x8000,"fbuf", ROMREGION_ERASE00 )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1983, smc777,  0,       0,	smc777, 	smc777, 	 0,  "Sony",   "SMC-777",		GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND)

