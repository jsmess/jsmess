/***************************************************************************************************

    PC-9801 (c) 1981 NEC

    fork / rewrite from pc98.c, contents of that file will be moved to here once that things gets
    consolidated there.

    preliminary driver by Angelo Salese

    TODO:
    - floppy interface doesn't seem to work at all with either floppy inserted or not, missing DMA irq?
    - proper 8251 uart hook-up on keyboard
    - boot is too slow right now, might be due of the floppy / HDD devices

****************************************************************************************************/

#include "emu.h"
#include "cpu/i86/i86.h"
#include "machine/i8255a.h"
#include "machine/pit8253.h"
#include "machine/8237dma.h"
#include "machine/pic8259.h"
#include "machine/upd765.h"
#include "machine/upd1990a.h"
#include "sound/beep.h"
#include "sound/2203intf.h"
#include "video/upd7220.h"
#include "imagedev/flopdrv.h"

class pc9801_state : public driver_device
{
public:
	pc9801_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		 m_hgdc1(*this, "upd7220_chr"),
		 m_hgdc2(*this, "upd7220_btm")
		{ }

	required_device<device_t> m_hgdc1;
	required_device<device_t> m_hgdc2;

	virtual void video_start();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);
	UINT8 *m_char_rom;

	UINT8 portb_tmp;
	int dma_channel;
	UINT8 dma_offset[2][4];
	UINT8 at_pages[0x10];

	UINT8 vrtc_irq_mask;
	UINT8 video_ff[8];
	UINT8 video_reg[6];
	UINT8 pal_clut[4];

	UINT8 *tvram;

	UINT16 font_addr;
	UINT8 font_line;
	UINT16 font_lr;

	UINT8 keyb_press;

	UINT8 fdc_2dd_ctrl;
	UINT8 nmi_ff;

	UINT8 vram_bank;
	UINT8 vram_disp;
};



#define WIDTH40_REG 2
#define MEMSW_REG   6
#define DISPLAY_REG 7


void pc9801_state::video_start()
{
	//pc9801_state *state = machine->driver_data<pc9801_state>();

	tvram = auto_alloc_array(machine, UINT8, 0x4000);

	// find memory regions
	m_char_rom = machine->region("chargen")->base();

	VIDEO_START_CALL(generic_bitmapped);
}

bool pc9801_state::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
	bitmap_fill(&bitmap, &cliprect, 0);

	/* graphics */
	upd7220_update(m_hgdc2, &bitmap, &cliprect);
	upd7220_update(m_hgdc1, &bitmap, &cliprect);

	return 0;
}

static UPD7220_DISPLAY_PIXELS( hgdc_display_pixels )
{
	pc9801_state *state = device->machine->driver_data<pc9801_state>();
	int xi;
	int res_x,res_y;
	UINT8 pen;
	UINT8 interlace_on;

	if(state->video_ff[DISPLAY_REG] == 0) //screen is off
		return;

	interlace_on = state->video_reg[2] == 0x10;

	for(xi=0;xi<8;xi++)
	{
		res_x = x + xi;
		res_y = y;

		pen = ((vram[address + (0x08000) + (state->vram_disp*0x20000)] >> (7-xi)) & 1) ? 1 : 0;
		pen|= ((vram[address + (0x10000) + (state->vram_disp*0x20000)] >> (7-xi)) & 1) ? 2 : 0;
		pen|= ((vram[address + (0x18000) + (state->vram_disp*0x20000)] >> (7-xi)) & 1) ? 4 : 0;

		if(interlace_on)
		{
			if(res_y*2+0 < 400)
				*BITMAP_ADDR16(bitmap, res_y*2+0, res_x) = pen + 8;
			if(res_y*2+1 < 400)
				*BITMAP_ADDR16(bitmap, res_y*2+1, res_x) = pen + 8;
		}
		else
			*BITMAP_ADDR16(bitmap, res_y, res_x) = pen + 8;
		//if(interlace_on)
		//	*BITMAP_ADDR16(bitmap, res_y+1, res_x) = pen + 8;
	}
}

static UPD7220_DRAW_TEXT_LINE( hgdc_draw_text )
{
	pc9801_state *state = device->machine->driver_data<pc9801_state>();
	int xi,yi;
	int x;
	UINT8 char_size,interlace_on;

	if(state->video_ff[DISPLAY_REG] == 0) //screen is off
		return;

	interlace_on = state->video_reg[2] == 0x10; /* TODO: correct? */
	char_size = (interlace_on) ? 16 : 8;

	for(x=0;x<pitch;x++)
	{
		UINT8 tile_data,secret,reverse,u_line,v_line;
		UINT8 color;
		UINT8 tile,attr,pen;
		UINT32 tile_addr;

		tile_addr = addr+(x*(state->video_ff[WIDTH40_REG]+1));

		tile = vram[(tile_addr*2) & 0x1fff] & 0x00ff; //TODO: kanji
		attr = (vram[(tile_addr*2 & 0x1fff) | 0x2000] & 0x00ff);

		secret = (attr & 1) ^ 1;
		//blink = attr & 2;
		reverse = attr & 4;
		u_line = attr & 8;
		v_line = attr & 0x10;
		color = (attr & 0xe0) >> 5;

		for(yi=0;yi<lr;yi++)
		{
			for(xi=0;xi<8;xi++)
			{
				int res_x,res_y;

				res_x = (x*8+xi) * (state->video_ff[WIDTH40_REG]+1);
				res_y = y*lr+yi;

				if(res_x > 640 || res_y > char_size*25) //TODO
					continue;

				tile_data = secret ? 0 : (state->m_char_rom[tile*char_size+interlace_on*0x800+yi]);

				if(reverse) { tile_data^=0xff; }
				if(u_line && yi == 7) { tile_data = 0xff; }
				if(v_line)	{ tile_data|=8; }

				if(cursor_addr == tile_addr) //TODO: also, cursor_on doesn't work?
					tile_data^=0xff;

				if(yi >= char_size)
					pen = 0;
				else
					pen = (tile_data >> (7-xi) & 1) ? color : 0;

				if(pen)
					*BITMAP_ADDR16(bitmap, res_y, res_x) = pen;

				if(state->video_ff[WIDTH40_REG])
				{
					if(res_x+1 > 640 || res_y > char_size*25) //TODO
						continue;

					*BITMAP_ADDR16(bitmap, res_y, res_x+1) = pen;
				}
			}
		}
	}
}


static UPD7220_INTERFACE( hgdc_1_intf )
{
	"screen",
	NULL,
	hgdc_draw_text,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

static UPD7220_INTERFACE( hgdc_2_intf )
{
	"screen",
	hgdc_display_pixels,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


#if 0
static READ8_HANDLER( pc9801_xx_r )
{
	if((offset & 1) == 0)
	{
		printf("Read to undefined port [%02x]\n",offset+0xxx);
		return 0xff;
	}
	else // odd
	{
		printf("Read to undefined port [%02x]\n",offset+0xxx);
		return 0xff;
	}
}

static WRITE8_HANDLER( pc9801_xx_w )
{
	if((offset & 1) == 0)
	{
		printf("Write to undefined port [%02x] <- %02x\n",offset+0xxx,data);
	}
	else // odd
	{
		printf("Write to undefined port [%02x] <- %02x\n",offset+0xxx,data);
	}
}

#endif

static READ8_HANDLER( pc9801_00_r )
{
	if((offset & 1) == 0)
	{
		if(offset & 0x14)
			printf("Read to undefined port [%02x]\n",offset+0x00);
		else
			return pic8259_r(space->machine->device((offset & 8) ? "pic8259_slave" : "pic8259_master"), (offset & 2) >> 1);
	}
	else // odd
	{
		return i8237_r(space->machine->device("dma8237"), (offset & 0x1e) >> 1);
	}

	return 0xff;
}

static WRITE8_HANDLER( pc9801_00_w )
{
	if((offset & 1) == 0)
	{
		if(offset & 0x14)
			printf("Write to undefined port [%02x] <- %02x\n",offset+0x00,data);
		else
			pic8259_w(space->machine->device((offset & 8) ? "pic8259_slave" : "pic8259_master"), (offset & 2) >> 1, data);
	}
	else // odd
	{
		i8237_w(space->machine->device("dma8237"), (offset & 0x1e) >> 1, data);
	}
}

static READ8_HANDLER( pc9801_20_r )
{
	if((offset & 1) == 0)
	{
		if(offset == 0)
			printf("Read to RTC port [%02x]\n",offset+0x20);
		else
			printf("Read to undefined port [%02x]\n",offset+0x20);

		return 0xff;
	}
	else // odd
	{
		printf("Read to undefined port [%02x]\n",offset+0x20);
		return 0xff;
	}
}

static WRITE8_HANDLER( pc9801_20_w )
{
	if((offset & 1) == 0)
	{
		if(offset == 0)
		{
			upd1990a_c0_w(space->machine->device("upd1990a"),      (data & 0x01) >> 0);
			upd1990a_c1_w(space->machine->device("upd1990a"),      (data & 0x02) >> 1);
			upd1990a_c2_w(space->machine->device("upd1990a"),      (data & 0x04) >> 2);
			upd1990a_stb_w(space->machine->device("upd1990a"),     (data & 0x08) >> 3);
			upd1990a_clk_w(space->machine->device("upd1990a"),     (data & 0x10) >> 4);
			upd1990a_data_in_w(space->machine->device("upd1990a"), (data & 0x20) >> 5);
			if(data & 0xc0)
				printf("RTC write to undefined bits %02x\n",data & 0xc0);
		}
		else
			printf("Write to undefined port [%02x] <- %02x\n",offset+0x20,data);
	}
	else // odd
	{
		pc9801_state *state = space->machine->driver_data<pc9801_state>();

		printf("Write to DMA bank register %d %02x\n",((offset >> 1)+1) & 3,data);
		state->dma_offset[0][((offset >> 1)+1) & 3] = data & 0x0f;
	}
}

static READ8_HANDLER( pc9801_30_r )
{
	if((offset & 1) == 0)
	{
		if(offset & 4)
			printf("Read to undefined port [%02x]\n",offset+0x30);
		else
			printf("Read to RS-232c port [%02x]\n",offset+0x30);

		return 0xff;
	}
	else // odd
	{
		return i8255a_r(space->machine->device("ppi8255_sys"), (offset & 6) >> 1);
	}
}

static WRITE8_HANDLER( pc9801_30_w )
{
	if((offset & 1) == 0)
	{
		if(offset & 4)
			printf("Write to undefined port [%02x] %02x\n",offset+0x30,data);
		else
			printf("Write to RS-232c port [%02x] %02x\n",offset+0x30,data);
	}
	else // odd
	{
		i8255a_w(space->machine->device("ppi8255_sys"), (offset & 6) >> 1,data);
	}
}

static READ8_HANDLER( pc9801_40_r )
{
	pc9801_state *state = space->machine->driver_data<pc9801_state>();

	if((offset & 1) == 0)
	{
		return i8255a_r(space->machine->device("ppi8255_prn"), (offset & 6) >> 1);
	}
	else // odd
	{
		if(offset & 4)
			printf("Read to undefined port [%02x]\n",offset+0x40);
		else
		{
			//printf("Read to 8251 kbd port [%02x]\n",offset+0x40);
			if(offset == 1)
			{
				UINT8 res;

				res = state->keyb_press;

				return res;
			}

			return 0;
		}
	}

	return 0xff;
}

static WRITE8_HANDLER( pc9801_40_w )
{
	if((offset & 1) == 0)
	{
		i8255a_w(space->machine->device("ppi8255_prn"), (offset & 6) >> 1,data);
	}
	else // odd
	{
		if(offset & 4)
			printf("Write to undefined port [%02x] <- %02x\n",offset+0x40,data);
		//else
			//printf("Write to 8251 kbd port [%02x] <- %02x\n",offset+0x40,data);
	}
}

static READ8_HANDLER( pc9801_50_r )
{
	//pc9801_state *state = space->machine->driver_data<pc9801_state>();

	if((offset & 1) == 0)
	{
		if(offset & 4)
			printf("Read to undefined port [%02x]\n",offset+0x50);
		else
			printf("Read to NMI FF port [%02x]\n",offset+0x50);

		return 0xff;
	}
	else // odd
	{
		return i8255a_r(space->machine->device("ppi8255_fdd"), (offset & 6) >> 1);
	}
}

static WRITE8_HANDLER( pc9801_50_w )
{
	pc9801_state *state = space->machine->driver_data<pc9801_state>();

	if((offset & 1) == 0)
	{
		if(offset & 4)
			printf("Write to undefined port [%02x] %02x\n",offset+0x50,data);
		else
			state->nmi_ff = (offset & 2) >> 1;

	}
	else // odd
	{
		i8255a_w(space->machine->device("ppi8255_fdd"), (offset & 6) >> 1,data);
	}
}

static READ8_HANDLER( pc9801_60_r )
{
	if((offset & 1) == 0)
	{
		return upd7220_r(space->machine->device("upd7220_chr"),(offset & 2) >> 1); // upd7220 character port
	}
	else // odd
	{
		printf("Read to undefined port [%02x]\n",offset+0x60);
		return 0xff;
	}
}

static WRITE8_HANDLER( pc9801_60_w )
{
	if((offset & 1) == 0)
	{
		upd7220_w(space->machine->device("upd7220_chr"),(offset & 2) >> 1,data); // upd7220 character port
	}
	else // odd
	{
		printf("Write to undefined port [%02x] <- %02x\n",offset+0x60,data);
	}
}

static WRITE8_HANDLER( pc9801_vrtc_mask_w )
{
	pc9801_state *state = space->machine->driver_data<pc9801_state>();

	if((offset & 1) == 0)
	{
		state->vrtc_irq_mask = 1;
	}
	else // odd
	{
		printf("Write to undefined port [%02x] <- %02x\n",offset+0x64,data);
	}
}

static WRITE8_HANDLER( pc9801_video_ff_w )
{
	pc9801_state *state = space->machine->driver_data<pc9801_state>();

	if((offset & 1) == 0)
	{
		state->video_ff[(data & 0x0e) >> 1] = data & 1;

		if(0)
		{
			static const char *const video_ff_regnames[] =
			{
				"Attribute Select",	// 0
				"Graphic",			// 1
				"Column",			// 2
				"Font Select",		// 3
				"200 lines",		// 4
				"KAC?",				// 5
				"Memory Switch",	// 6
				"Display ON"		// 7
			};

			printf("Write to video FF register %s -> %02x\n",video_ff_regnames[(data & 0x0e) >> 1],data & 1);
		}
	}
	else // odd
	{
		printf("Write to undefined port [%02x] <- %02x\n",offset+0x68,data);
	}
}


static READ8_HANDLER( pc9801_70_r )
{
	if((offset & 1) == 0)
	{
		printf("Read to display register [%02x]\n",offset+0x70);
		return 0xff;
	}
	else // odd
	{
		if(offset & 0x08)
			printf("Read to undefined port [%02x]\n",offset+0x70);
		else
			return pit8253_r(space->machine->device("pit8253"), (offset & 6) >> 1);
	}

	return 0xff;
}

static WRITE8_HANDLER( pc9801_70_w )
{
	pc9801_state *state = space->machine->driver_data<pc9801_state>();

	if((offset & 1) == 0)
	{
		printf("Write to display register [%02x] %02x\n",offset+0x70,data);
		state->video_reg[offset >> 1] = data;
	}
	else // odd
	{
		if(offset < 0x08)
			pit8253_w(space->machine->device("pit8253"), (offset & 6) >> 1, data);
		else
			printf("Write to undefined port [%02x] <- %02x\n",offset+0x70,data);
	}
}

static READ8_HANDLER( pc9801_sasi_r )
{
	if((offset & 1) == 0)
	{
		//printf("Read to SASI port [%02x]\n",offset+0x80);
		return 0;
	}
	else // odd
	{
		printf("Read to undefined port [%02x]\n",offset+0x80);
		return 0xff;
	}
}

static WRITE8_HANDLER( pc9801_sasi_w )
{
	if((offset & 1) == 0)
	{
		//printf("Write to SASI port [%02x] <- %02x\n",offset+0x80,data);
	}
	else // odd
	{
		//printf("Write to undefined port [%02x] <- %02x\n",offset+0xxx,data);
	}
}

static READ8_HANDLER( pc9801_a0_r )
{
	pc9801_state *state = space->machine->driver_data<pc9801_state>();

	if((offset & 1) == 0)
	{
		switch(offset & 0xe)
		{
			case 0x00:
				UINT8 v_sync;

				v_sync = upd7220_r(space->machine->device("upd7220_chr"),(offset & 2) >> 1) & 0x20; //TODO: support it into the core

				return (upd7220_r(space->machine->device("upd7220_btm"),(offset & 2) >> 1) & 0xdf) | v_sync;
			case 0x02:
				return upd7220_r(space->machine->device("upd7220_btm"),(offset & 2) >> 1);
			/* bitmap palette clut read */
			case 0x08:
			case 0x0a:
			case 0x0c:
			case 0x0e:
				return state->pal_clut[(offset & 0x6) >> 1];
			default:
				printf("Read to undefined port [%02x]\n",offset+0xa0);
				return 0xff;
		}
	}
	else // odd
	{
		switch((offset & 0xe) + 1)
		{
			case 0x09://cg window font read
			{
				UINT8 *pcg = space->machine->region("pcg")->base();

				return pcg[((state->font_addr & 0x7f7f) << 4) | state->font_lr | (state->font_line & 0x0f)];
			}
		}

		printf("Read to undefined port [%02x]\n",offset+0xa0);
		return 0xff;
	}
}

static WRITE8_HANDLER( pc9801_a0_w )
{
	pc9801_state *state = space->machine->driver_data<pc9801_state>();

	if((offset & 1) == 0)
	{
		switch(offset & 0xe)
		{
			case 0x00:
			case 0x02:
				upd7220_w(space->machine->device("upd7220_btm"),(offset & 2) >> 1,data);
				return;
			case 0x04: state->vram_disp = data & 1; return;
			case 0x06:
				state->vram_bank = data & 1;
				upd7220_bank_w(space->machine->device("upd7220_btm"),0,(data & 1) << 2); //TODO: check me
				return;
			/* bitmap palette clut write */
			case 0x08:
			case 0x0a:
			case 0x0c:
			case 0x0e:
			{
				UINT8 pal_entry;

				state->pal_clut[(offset & 0x6) >> 1] = data;

				/* can't be more twisted I presume ... :-/ */
				pal_entry = (((offset & 4) >> 1) | ((offset & 2) << 1)) >> 1;
				pal_entry ^= 3;

				palette_set_color_rgb(space->machine, (pal_entry)|4|8, pal1bit((data & 0x2) >> 1), pal1bit((data & 4) >> 2), pal1bit((data & 1) >> 0));
				palette_set_color_rgb(space->machine, (pal_entry)|8, pal1bit((data & 0x20) >> 5), pal1bit((data & 0x40) >> 6), pal1bit((data & 0x10) >> 4));
				return;
			}
			default:
				printf("Write to undefined port [%02x] <- %02x\n",offset+0xa0,data);
				return;
		}
	}
	else // odd
	{
		switch((offset & 0xe) + 1)
		{
			case 0x01:
				state->font_addr = (data << 8) | (state->font_addr & 0xff);
				return;
			case 0x03:
				state->font_addr = (data & 0xff) | (state->font_addr & 0xff00);
				return;
			case 0x05:
				state->font_line = data & 0x1f;
				state->font_lr = data & 0x20 ? 0x000 : 0x800;
				return;
			case 0x09: //cg window font write
			{
				UINT8 *pcg = space->machine->region("pcg")->base();

				pcg[((state->font_addr & 0x7f7f) << 4) | state->font_lr | state->font_line] = data;
				return;
			}
		}

		printf("Write to undefined port [%02x] <- %02x\n",offset+0xa0,data);
	}
}

static READ8_HANDLER( pc9801_fdc_2dd_r )
{
	if((offset & 1) == 0)
	{
		switch(offset & 6)
		{
			case 0:	return upd765_status_r(space->machine->device("upd765_2dd"),0);
			case 2: return upd765_data_r(space->machine->device("upd765_2dd"),0);
			case 4: return 0x40; //unknown port meaning, might be 0x70
		}
	}
	else
	{
		printf("Read to undefined port [%02x]\n",offset+0xc8);
		return 0xff;
	}

	return 0xff;
}

static WRITE8_HANDLER( pc9801_fdc_2dd_w )
{
	pc9801_state *state = space->machine->driver_data<pc9801_state>();

	if((offset & 1) == 0)
	{
		switch(offset & 6)
		{
			case 0: printf("Write to undefined port [%02x] <- %02x\n",offset+0xc8,data); return;
			case 2: upd765_data_w(space->machine->device("upd765_2dd"),0,data); return;
			case 4:
				printf("%02x ctrl\n",data);
				if(((state->fdc_2dd_ctrl & 0x80) == 0) && (data & 0x80))
					upd765_reset_w(space->machine->device("upd765_2dd"),1);
				if((state->fdc_2dd_ctrl & 0x80) && (!(data & 0x80)))
					upd765_reset_w(space->machine->device("upd765_2dd"),0);

				state->fdc_2dd_ctrl = data;
				//floppy_mon_w(floppy_get_device(space->machine, 0), (data & 8) ? CLEAR_LINE : ASSERT_LINE);
				//floppy_mon_w(floppy_get_device(space->machine, 1), (data & 8) ? CLEAR_LINE : ASSERT_LINE);
				//floppy_drive_set_ready_state(floppy_get_device(space->machine, 0), (data & 8), 0);
				//floppy_drive_set_ready_state(floppy_get_device(space->machine, 1), (data & 8), 0);
				break;
		}
	}
	else
	{
		printf("Write to undefined port [%02x] <- %02x\n",offset+0xc8,data);
	}
}


/* TODO: banking? */
static READ8_HANDLER( pc9801_tvram_r )
{
	pc9801_state *state = space->machine->driver_data<pc9801_state>();
	UINT8 res;

	if((offset & 0x2000) && offset & 1)
		return 0xff;

	//res = upd7220_vram_r(space->machine->device("upd7220_chr"),offset);
	res = state->tvram[offset];

	return res;
}

static WRITE8_HANDLER( pc9801_tvram_w )
{
	pc9801_state *state = space->machine->driver_data<pc9801_state>();

	if(offset < (0x3fe2) || state->video_ff[MEMSW_REG])
		state->tvram[offset] = data;

	upd7220_vram_w(space->machine->device("upd7220_chr"),offset,data); //TODO: check me
}

static READ8_HANDLER( pc9801_gvram_r )
{
	pc9801_state *state = space->machine->driver_data<pc9801_state>();

	return upd7220_vram_r(space->machine->device("upd7220_btm"),offset+0x8000+state->vram_bank*0x20000);
}

static WRITE8_HANDLER( pc9801_gvram_w )
{
	pc9801_state *state = space->machine->driver_data<pc9801_state>();

	upd7220_vram_w(space->machine->device("upd7220_btm"),offset+0x8000+state->vram_bank*0x20000, data);
}

static READ8_HANDLER( pc9801_opn_r )
{
	if((offset & 1) == 0)
		return ym2203_r(space->machine->device("opn"),offset >> 1);
	else // odd
	{
		printf("Read to undefined port [%02x]\n",offset+0x188);
		return 0xff;
	}
}

static WRITE8_HANDLER( pc9801_opn_w )
{
	if((offset & 1) == 0)
		ym2203_w(space->machine->device("opn"),offset >> 1,data);
	else // odd
	{
		printf("Write to undefined port [%02x] %02x\n",offset+0x188,data);
	}
}


static ADDRESS_MAP_START( pc9801_map, ADDRESS_SPACE_PROGRAM, 16)
	AM_RANGE(0x00000, 0x9ffff) AM_RAM //work RAM
	AM_RANGE(0xa0000, 0xa3fff) AM_READWRITE8(pc9801_tvram_r,pc9801_tvram_w,0xffff) //TVRAM
	AM_RANGE(0xa8000, 0xbffff) AM_READWRITE8(pc9801_gvram_r,pc9801_gvram_w,0xffff) //bitmap VRAM
//  AM_RANGE(0xcc000, 0xcdfff) AM_ROM //sound BIOS
	AM_RANGE(0xd6000, 0xd6fff) AM_ROM AM_REGION("fdc_bios_2dd",0) //floppy BIOS 2dd
//  AM_RANGE(0xd7000, 0xd7fff) AM_ROM AM_REGION("fdc_bios_2hd",0) //floppy BIOS 2hd
	AM_RANGE(0xe8000, 0xfffff) AM_ROM AM_REGION("ipl",0)
ADDRESS_MAP_END

/* first device is even offsets, second one is odd offsets */
static ADDRESS_MAP_START( pc9801_io, ADDRESS_SPACE_IO, 16)
	AM_RANGE(0x0000, 0x001f) AM_READWRITE8(pc9801_00_r,pc9801_00_w,0xffff) // i8259 PIC (bit 3 ON slave / master) / i8237 DMA
	AM_RANGE(0x0020, 0x0027) AM_READWRITE8(pc9801_20_r,pc9801_20_w,0xffff) // RTC / DMA registers (LS244)
	AM_RANGE(0x0030, 0x0037) AM_READWRITE8(pc9801_30_r,pc9801_30_w,0xffff) //i8251 RS232c / i8255 system port
	AM_RANGE(0x0040, 0x0047) AM_READWRITE8(pc9801_40_r,pc9801_40_w,0xffff) //i8255 printer port / i8251 keyboard
	AM_RANGE(0x0050, 0x0057) AM_READWRITE8(pc9801_50_r,pc9801_50_w,0xffff) // NMI FF / i8255 floppy port (2d?)
	AM_RANGE(0x0060, 0x0063) AM_READWRITE8(pc9801_60_r,pc9801_60_w,0xffff) //upd7220 character ports / <undefined>
	AM_RANGE(0x0064, 0x0065) AM_WRITE8(pc9801_vrtc_mask_w,0xffff)
	AM_RANGE(0x0068, 0x0069) AM_WRITE8(pc9801_video_ff_w,0xffff) //mode FF / <undefined>
//  AM_RANGE(0x006c, 0x006f) border color / <undefined>
	AM_RANGE(0x0070, 0x007b) AM_READWRITE8(pc9801_70_r,pc9801_70_w,0xffff) //display registers / i8253 pit
	AM_RANGE(0x0080, 0x0083) AM_READWRITE8(pc9801_sasi_r,pc9801_sasi_w,0xffff) //HDD SASI interface / <undefined>
//  AM_RANGE(0x0090, 0x0097) upd765a 2hd / cmt
	AM_RANGE(0x00a0, 0x00af) AM_READWRITE8(pc9801_a0_r,pc9801_a0_w,0xffff) //upd7220 bitmap ports / display registers
	AM_RANGE(0x00c8, 0x00cd) AM_READWRITE8(pc9801_fdc_2dd_r,pc9801_fdc_2dd_w,0xffff) //upd765a 2dd / <undefined>
	AM_RANGE(0x0188, 0x018b) AM_READWRITE8(pc9801_opn_r,pc9801_opn_w,0xffff) //ym2203 opn / <undefined>
ADDRESS_MAP_END

static ADDRESS_MAP_START( upd7220_1_map, 0, 8 )
	AM_RANGE(0x00000, 0x3ffff) AM_DEVREADWRITE("upd7220_chr",upd7220_vram_r,upd7220_vram_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( upd7220_2_map, 0, 8 )
	AM_RANGE(0x00000, 0x3ffff) AM_DEVREADWRITE("upd7220_btm",upd7220_vram_r,upd7220_vram_w)
ADDRESS_MAP_END

/* keyboard code */
/* TODO: key repeat, remove port impulse! */
static INPUT_CHANGED( key_stroke )
{
	pc9801_state *state = field->port->machine->driver_data<pc9801_state>();

	if(newval && !oldval)
	{
		state->keyb_press = (UINT8)(FPTR)(param) & 0x7f;
		pic8259_ir1_w(field->port->machine->device("pic8259_master"), 1);
	}

	if(oldval && !newval)
		state->keyb_press = 0;
}

/* for key modifiers */
static INPUT_CHANGED( shift_stroke )
{
	pc9801_state *state = field->port->machine->driver_data<pc9801_state>();

	if((newval && !oldval) || (oldval && !newval))
	{
		state->keyb_press = (UINT8)(FPTR)(param) & 0x7f;
		pic8259_ir1_w(field->port->machine->device("pic8259_master"), 1);
	}
	else
		state->keyb_press = 0;
}

static INPUT_PORTS_START( pc9801 )
	PORT_START("KEY0") // 0x00 - 0x07
	PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_UNUSED)
	PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x01)
	PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x02)
	PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x03)
	PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x04)
	PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x05)
	PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x06)
	PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x07)

	PORT_START("KEY1") // 0x08 - 0x0f
	PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x08)
	PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x09)
	PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x0a)
	PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x0b)
	PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("^") PORT_CHAR('^') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x0c)
	PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("\xC2\xA5") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x0d)
	PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("BACKSPACE") PORT_CODE(KEYCODE_BACKSPACE)  PORT_CHAR(8) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x0e)
	PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("TAB") PORT_CODE(KEYCODE_TAB) PORT_CHAR('\t') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x0f)

	PORT_START("KEY2") // 0x10 - 0x17
	PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q) PORT_CHAR('Q') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x10)
	PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W) PORT_CHAR('W') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x11)
	PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E) PORT_CHAR('E') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x12)
	PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R) PORT_CHAR('R') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x13)
	PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T) PORT_CHAR('T') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x14)
	PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y) PORT_CHAR('Y') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x15)
	PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U) PORT_CHAR('U') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x16)
	PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I) PORT_CHAR('I') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x17)

	PORT_START("KEY3") // 0x18 - 0x1f
	PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O) PORT_CHAR('O') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x18)
	PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P) PORT_CHAR('P') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x19)
	PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("@") PORT_CHAR('@') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x1a)
	PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("[") PORT_CHAR('[') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x1b)
	PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(27) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x1c)
	PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A) PORT_CHAR('A') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x1d)
	PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S) PORT_CHAR('S') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x1e)
	PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D) PORT_CHAR('D') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x1f)

	PORT_START("KEY4") // 0x20 - 0x27
	PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F) PORT_CHAR('F') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x20)
	PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G) PORT_CHAR('G') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x21)
	PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H) PORT_CHAR('H') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x22)
	PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J) PORT_CHAR('J') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x23)
	PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K) PORT_CHAR('K') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x24)
	PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L) PORT_CHAR('L') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x25)
	PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(";") PORT_CHAR(';') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x26)
	PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(":") PORT_CHAR(':') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x27)

	PORT_START("KEY5") // 0x28 - 0x2f
	PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("]") PORT_CHAR(']') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x28)
	PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z) PORT_CHAR('Z') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x29)
	PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X) PORT_CHAR('X') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x2a)
	PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C) PORT_CHAR('C') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x2b)
	PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V) PORT_CHAR('V') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x2c)
	PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B) PORT_CHAR('B') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x2d)
	PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N) PORT_CHAR('N') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x2e)
	PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M) PORT_CHAR('M') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x2f)

	PORT_START("KEY6") // 0x30 - 0x37
	PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(",") PORT_CHAR(',') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x30)
	PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(".") PORT_CHAR('.') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x31)
	PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("/") /*PORT_CODE(KEYCODE_SLASH)*/ PORT_CHAR('/') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x32)
	PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("un 0-4") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x33)
	PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_IMPULSE(1) PORT_CHAR(' ') PORT_CHANGED(key_stroke, 0x34)
	PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("un 0-6") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x35)
	PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PG DOWN") PORT_CODE(KEYCODE_PGDN) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x36)
	PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PG UP") PORT_CODE(KEYCODE_PGUP) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x37)

	PORT_START("KEY7") // 0x38 - 0x3f
	PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 1-1") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x38) //PORT_CHAR(',')
	PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("DEL") PORT_CODE(KEYCODE_DEL) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x39) //PORT_CHAR('.')
	PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Up") PORT_CODE(KEYCODE_UP) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x3a) //PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/')
	PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x3b) //PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x3c)
	PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x3d) //PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("CLS") PORT_CODE(KEYCODE_HOME) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x3e) //PORT_CHAR(' ')
	PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 1-8") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x3f) //PORT_CODE(KEYCODE_M) PORT_CHAR('M')

	PORT_START("KEY8") // 0x40 - 0x47
	PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("- (PAD)") PORT_CODE(KEYCODE_MINUS_PAD) PORT_CHAR('-') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x40)
	PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("/ (PAD)") PORT_CODE(KEYCODE_SLASH_PAD) PORT_CHAR('/') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x41)
	PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("7 (PAD)") PORT_CODE(KEYCODE_7_PAD) PORT_CHAR('7') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x42)
	PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("8 (PAD)") PORT_CODE(KEYCODE_8_PAD) PORT_CHAR('8') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x43)
	PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("9 (PAD)") PORT_CODE(KEYCODE_9_PAD) PORT_CHAR('9') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x44)
	PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("* (PAD)") PORT_CODE(KEYCODE_ASTERISK) PORT_CHAR('*') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x45)
	PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("4 (PAD)") PORT_CODE(KEYCODE_4_PAD) PORT_CHAR('4') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x46)
	PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("5 (PAD)") PORT_CODE(KEYCODE_5_PAD) PORT_CHAR('5') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x47)

	PORT_START("KEY9") // 0x48 - 0x4f
	PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("6 (PAD)") PORT_CODE(KEYCODE_6_PAD) PORT_CHAR('6') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x48)
	PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("+ (PAD)") PORT_CODE(KEYCODE_PLUS_PAD) PORT_CHAR('+') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x49)
	PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("1 (PAD)") PORT_CODE(KEYCODE_1_PAD) PORT_CHAR('1') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x4a)
	PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("2 (PAD)") PORT_CODE(KEYCODE_2_PAD) PORT_CHAR('2') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x4b)
	PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("3 (PAD)") PORT_CODE(KEYCODE_3_PAD) PORT_CHAR('3') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x4c)
	PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("EQUAL (PAD)") PORT_CODE(KEYCODE_ENTER_PAD) PORT_CHAR('=') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x4d)
	PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("0 (PAD)") PORT_CODE(KEYCODE_0_PAD) PORT_CHAR('0') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x4e)
	PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(", (PAD)") PORT_CHAR(',') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x4f)

	PORT_START("KEYA") // 0x50 - 0x57
	PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(". (PAD)") PORT_CHAR('.') PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x50)
	PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 2-2") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x51) //PORT_CHAR('.')
	PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 2-3") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x52) //PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/')
	PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 2-4") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x53) //PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 2-5") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x54)//PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 2-6") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x55) //PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 2-7") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x56) //PORT_CODE(KEYCODE_HOME) //PORT_CHAR(' ')
	PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 2-8") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x57) //PORT_CODE(KEYCODE_M) PORT_CHAR('M')

	PORT_START("KEYB") // 0x58 - 0x5f
	PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 3-1") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x58) //PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 3-2") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x59) //PORT_CHAR('.')
	PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 3-3") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x5a) //PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/')
	PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 3-4") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x5b) //PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 3-5") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x5c) //PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 3-6") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x5d) //PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 3-7") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x5e) //PORT_CODE(KEYCODE_HOME) //PORT_CHAR(' ')
	PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 3-8") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x5f) //PORT_CODE(KEYCODE_M) PORT_CHAR('M')

	PORT_START("KEYC") // 0x60 - 0x67
	PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 4-1") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x60) //PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 4-2") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x61) //PORT_CHAR('.')
	PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F1") PORT_CODE(KEYCODE_F1) PORT_CHAR(UCHAR_MAMEKEY(F1)) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x62)
	PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F2") PORT_CODE(KEYCODE_F2) PORT_CHAR(UCHAR_MAMEKEY(F2)) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x63)
	PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F3") PORT_CODE(KEYCODE_F3) PORT_CHAR(UCHAR_MAMEKEY(F3)) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x64)
	PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F4") PORT_CODE(KEYCODE_F4) PORT_CHAR(UCHAR_MAMEKEY(F4)) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x65)
	PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F5") PORT_CODE(KEYCODE_F5) PORT_CHAR(UCHAR_MAMEKEY(F5)) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x66)
	PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F6") PORT_CODE(KEYCODE_F6) PORT_CHAR(UCHAR_MAMEKEY(F6)) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x67)

	PORT_START("KEYD") // 0x68 - 0x6f
	PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F7") PORT_CODE(KEYCODE_F7) PORT_CHAR(UCHAR_MAMEKEY(F7)) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x68)
	PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F8") PORT_CODE(KEYCODE_F8) PORT_CHAR(UCHAR_MAMEKEY(F8)) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x69)
	PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F9") PORT_CODE(KEYCODE_F9) PORT_CHAR(UCHAR_MAMEKEY(F9)) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x6a)
	PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F10") PORT_CODE(KEYCODE_F10) PORT_CHAR(UCHAR_MAMEKEY(F10)) PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x6b)
	PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 5-5") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x6c)//PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 5-6") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x6d)//PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 5-7") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x6e)//PORT_CODE(KEYCODE_HOME) //PORT_CHAR(' ')
	PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 5-8") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x6f) //PORT_CODE(KEYCODE_M) PORT_CHAR('M')

	PORT_START("KEYE") // 0x70 - 0x77
	PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("LSHIFT") PORT_CODE(KEYCODE_LSHIFT) /*PORT_IMPULSE(1)*/ PORT_CHANGED(shift_stroke, 0x70)
	PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("CAPS LOCK") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x71)
	PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("\xe3\x81\x8b\xe3\x81\xaa / KANA LOCK") PORT_TOGGLE PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x72)
	PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 6-4") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x73) //PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 6-5") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x74) //PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 6-6") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x75) //PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 6-7") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x76) //PORT_CODE(KEYCODE_HOME) //PORT_CHAR(' ')
	PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 6-8") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x77) //PORT_CODE(KEYCODE_M) PORT_CHAR('M')

	PORT_START("KEYF") // 0x78 - 0x7f
	PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 7-1") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x78) //PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 7-2") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x79)
	PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 7-3") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x7a)
	PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 7-4") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x7b) //PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 7-5") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x7c) //PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 7-6") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x7d) //PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 7-7") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x7e) //PORT_CODE(KEYCODE_HOME) //PORT_CHAR(' ')
	PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(" un 7-8") PORT_IMPULSE(1) PORT_CHANGED(key_stroke, 0x7f) //PORT_CODE(KEYCODE_M) PORT_CHAR('M')

	PORT_START("DSW1")
	PORT_BIT(0x0001, IP_ACTIVE_HIGH,IPT_SPECIAL) PORT_READ_LINE_DEVICE("upd1990a", upd1990a_data_out_r)
	PORT_DIPNAME( 0x0002, 0x0000, "DSW1" ) // error beep if OFF
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Display Type" ) PORT_DIPLOCATION("SW2:1")
	PORT_DIPSETTING(      0x0000, "Normal Display" )
	PORT_DIPSETTING(      0x0008, "Hi-Res Display" )
	PORT_DIPNAME( 0x0010, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0000, "DSWB" )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x0000, DEF_STR( Unknown ) ) //uhm, this attempts to DMA something if off ...?
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START("DSW2")
	PORT_DIPNAME( 0x01, 0x01, "System Specification" ) PORT_DIPLOCATION("SW1:1") //jumps to daa00 if off, presumably some card booting
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Terminal Mode" ) PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Text width" ) PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x04, "40 chars/line" )
	PORT_DIPSETTING(    0x00, "80 chars/line" )
	PORT_DIPNAME( 0x08, 0x00, "Text height" ) PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x08, "20 lines/screen" )
	PORT_DIPSETTING(    0x00, "25 lines/screen" )
	PORT_DIPNAME( 0x10, 0x00, "Memory Switch Init" ) PORT_DIPLOCATION("SW1:5")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) ) //Fix memory switch condition
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) ) //Initialize Memory Switch with the system default
	PORT_DIPUNUSED_DIPLOC( 0x20, 0x20, "SW1:6" )
	PORT_DIPUNUSED_DIPLOC( 0x40, 0x40, "SW1:7" )
	PORT_DIPUNUSED_DIPLOC( 0x80, 0x80, "SW1:8" )

	PORT_START("ROM_LOAD")
	PORT_CONFNAME( 0x01, 0x01, "Load floppy 2dd BIOS" )
	PORT_CONFSETTING(    0x00, DEF_STR( Yes ) )
	PORT_CONFSETTING(    0x01, DEF_STR( No ) )

	PORT_START("VBLANK")
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_VBLANK )
INPUT_PORTS_END

static const gfx_layout charset_8x8 =
{
	8,8,
	256,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_layout charset_8x16 =
{
	8,16,
	256,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	8*16
};

static const gfx_layout charset_16x16 =
{
	16,16,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ STEP16(0,1) },
	{ STEP16(0,16) },
	16*16
};

static GFXDECODE_START( pc9801 )
	GFXDECODE_ENTRY( "chargen", 0x00000, charset_8x8,     0x000, 0x01 )
	GFXDECODE_ENTRY( "chargen", 0x00800, charset_8x16,     0x000, 0x01 )
	GFXDECODE_ENTRY( "kanji",   0x00000, charset_16x16,   0x000, 0x01 )
GFXDECODE_END

/****************************************
*
* I8259 PIC interface
*
****************************************/

/*
irq assignment is:

8259 master:
ir0 PIT
ir1 keyboard
ir2 vblank
ir3
ir4 rs-232c
ir5
ir6
ir7

8259 slave:
ir0 <connection with master 8259?>
ir1
ir2 2dd floppy irq
ir3 2hd floppy irq
ir4
ir5
ir6
ir7
*/


static WRITE_LINE_DEVICE_HANDLER( pc9801_master_set_int_line )
{
	//printf("%02x\n",interrupt);
	cputag_set_input_line(device->machine, "maincpu", 0, state ? HOLD_LINE : CLEAR_LINE);
}

static const struct pic8259_interface pic8259_master_config =
{
	DEVCB_LINE(pc9801_master_set_int_line)
};

static const struct pic8259_interface pic8259_slave_config =
{
	DEVCB_DEVICE_LINE("pic8259_master", pic8259_ir7_w) //TODO: check me
};

/****************************************
*
* I8253 PIT interface
*
****************************************/

static const struct pit8253_config pit8253_config =
{
	{
		{
			1996800,				/* heartbeat IRQ */
			DEVCB_NULL,
			DEVCB_DEVICE_LINE("pic8259_master", pic8259_ir0_w)
		}, {
			1996800,				/* dram refresh */
			DEVCB_NULL,
			DEVCB_NULL
		}, {
			1996800,				/* pio port c pin 4, and speaker polling enough */
			DEVCB_NULL,
			DEVCB_NULL
		}
	}
};

/****************************************
*
* I8237 DMA interface
*
****************************************/

static WRITE_LINE_DEVICE_HANDLER( pc_dma_hrq_changed )
{
	cputag_set_input_line(device->machine, "maincpu", INPUT_LINE_HALT, state ? ASSERT_LINE : CLEAR_LINE);

	/* Assert HLDA */
	i8237_hlda_w( device, state );
}


static READ8_HANDLER( pc_dma_read_byte )
{
	pc9801_state *state = space->machine->driver_data<pc9801_state>();
	offs_t page_offset = (((offs_t) state->dma_offset[0][state->dma_channel]) << 16)
		& 0xFF0000;

	return space->read_byte(page_offset + offset);
}


static WRITE8_HANDLER( pc_dma_write_byte )
{
	pc9801_state *state = space->machine->driver_data<pc9801_state>();
	offs_t page_offset = (((offs_t) state->dma_offset[0][state->dma_channel]) << 16)
		& 0xFF0000;

	space->write_byte(page_offset + offset, data);
}

static void set_dma_channel(device_t *device, int channel, int state)
{
	pc9801_state *drvstate = device->machine->driver_data<pc9801_state>();
	if (!state) drvstate->dma_channel = channel;
}

static WRITE_LINE_DEVICE_HANDLER( pc_dack0_w ) { /*printf("%02x 0\n",state);*/ set_dma_channel(device, 0, state); }
static WRITE_LINE_DEVICE_HANDLER( pc_dack1_w ) { /*printf("%02x 1\n",state);*/ set_dma_channel(device, 1, state); }
static WRITE_LINE_DEVICE_HANDLER( pc_dack2_w ) { /*printf("%02x 2\n",state);*/ set_dma_channel(device, 2, state); }
static WRITE_LINE_DEVICE_HANDLER( pc_dack3_w ) { /*printf("%02x 3\n",state);*/ set_dma_channel(device, 3, state); }

static READ8_DEVICE_HANDLER( test_r )
{
	printf("2dd DACK R\n");

	return 0xff;
}

static WRITE8_DEVICE_HANDLER( test_w )
{
	printf("2dd DACK W\n");
}

static I8237_INTERFACE( dma8237_config )
{
	DEVCB_LINE(pc_dma_hrq_changed),
	DEVCB_NULL,
	DEVCB_MEMORY_HANDLER("maincpu", PROGRAM, pc_dma_read_byte),
	DEVCB_MEMORY_HANDLER("maincpu", PROGRAM, pc_dma_write_byte),
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_HANDLER(test_r) },
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_HANDLER(test_w) },
	{ DEVCB_LINE(pc_dack0_w), DEVCB_LINE(pc_dack1_w), DEVCB_LINE(pc_dack2_w), DEVCB_LINE(pc_dack3_w) }
};

/****************************************
*
* PPI interfaces
*
****************************************/

static READ8_DEVICE_HANDLER( ppi_sys_porta_r ) { return input_port_read(device->machine,"DSW2"); }
static READ8_DEVICE_HANDLER( ppi_sys_portb_r ) { return input_port_read(device->machine,"DSW1") & 0xff; }
static READ8_DEVICE_HANDLER( ppi_prn_portb_r ) { return input_port_read(device->machine,"DSW1") >> 8; }

static WRITE8_DEVICE_HANDLER( ppi_sys_portc_w )
{
	beep_set_state(device->machine->device("beeper"),!(data & 0x08));
}

static I8255A_INTERFACE( ppi_system_intf )
{
	DEVCB_HANDLER(ppi_sys_porta_r),					/* Port A read */
	DEVCB_HANDLER(ppi_sys_portb_r),					/* Port B read */
	DEVCB_NULL,					/* Port C read */
	DEVCB_NULL,					/* Port A write */
	DEVCB_NULL,					/* Port B write */
	DEVCB_HANDLER(ppi_sys_portc_w)					/* Port C write */
};

static I8255A_INTERFACE( ppi_printer_intf )
{
	DEVCB_NULL,					/* Port A read */
	DEVCB_HANDLER(ppi_prn_portb_r),					/* Port B read */
	DEVCB_NULL,					/* Port C read */
	DEVCB_NULL,					/* Port A write */
	DEVCB_NULL,					/* Port B write */
	DEVCB_NULL					/* Port C write */
};

static READ8_DEVICE_HANDLER( ppi_fdd_porta_r )
{
	return 0xff;
}

static READ8_DEVICE_HANDLER( ppi_fdd_portb_r )
{
	return 0xff; //upd765_status_r(device->machine->device("upd765_2dd"),0);
}

static READ8_DEVICE_HANDLER( ppi_fdd_portc_r )
{
	return 0xff; //upd765_data_r(device->machine->device("upd765_2dd"),0);
}

static WRITE8_DEVICE_HANDLER( ppi_fdd_portc_w )
{
	//upd765_data_w(device->machine->device("upd765_2dd"),0,data);
}

static I8255A_INTERFACE( ppi_fdd_intf )
{
	DEVCB_HANDLER(ppi_fdd_porta_r),					/* Port A read */
	DEVCB_HANDLER(ppi_fdd_portb_r),					/* Port B read */
	DEVCB_HANDLER(ppi_fdd_portc_r),					/* Port C read */
	DEVCB_NULL,					/* Port A write */
	DEVCB_NULL,					/* Port B write */
	DEVCB_HANDLER(ppi_fdd_portc_w)					/* Port C write */
};

/****************************************
*
* UPD765 interface
*
****************************************/

static WRITE_LINE_DEVICE_HANDLER( fdc_2dd_irq )
{
	pc9801_state *drvstate = device->machine->driver_data<pc9801_state>();

	printf("IRQ %d\n",state);

	if(drvstate->fdc_2dd_ctrl & 8)
	{
		pic8259_ir2_w(device->machine->device("pic8259_slave"), state);
	}
}

static WRITE_LINE_DEVICE_HANDLER( fdc_2dd_drq )
{
	printf("%02x DRQ\n",state);
}

static const struct upd765_interface upd765_2dd_intf =
{
	DEVCB_LINE(fdc_2dd_irq),
	DEVCB_LINE(fdc_2dd_drq), //DRQ, TODO
	NULL,
	UPD765_RDY_PIN_CONNECTED,
	{FLOPPY_0, FLOPPY_1, NULL, NULL}
};

static const floppy_config pc9801_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSHD,
	FLOPPY_OPTIONS_NAME(default),
	NULL
};

static UPD1990A_INTERFACE( pc9801_upd1990a_intf )
{
	DEVCB_NULL,
	DEVCB_NULL
};

/****************************************
*
* Init emulation status
*
****************************************/

static PALETTE_INIT( pc9801 )
{
	int i;

	for(i=0;i<8;i++)
		palette_set_color_rgb(machine, i, pal1bit(i >> 1), pal1bit(i >> 2), pal1bit(i >> 0));
}

static IRQ_CALLBACK(irq_callback)
{
	int r = 0;
	r = pic8259_acknowledge( device->machine->device( "pic8259_slave" ));
	if (r==0)
	{
		r = pic8259_acknowledge( device->machine->device( "pic8259_master" ));
		//printf("%02x ACK\n",r);
	}
	return r;
}

static MACHINE_START(pc9801)
{
	cpu_set_irq_callback(machine->device("maincpu"), irq_callback);

	upd1990a_cs_w(machine->device("upd1990a"), 1);
	upd1990a_oe_w(machine->device("upd1990a"), 1);
}

static MACHINE_RESET(pc9801)
{
	pc9801_state *state = machine->driver_data<pc9801_state>();

	/* this looks like to be some kind of backup ram, system will boot with green colors otherwise */
	{
		int i;
		static const UINT8 default_memsw_data[0x10] =
		{
			0xe1, 0x48, 0xe1, 0x05, 0xe1, 0x04, 0xe1, 0x00, 0xe1, 0x01, 0xe1, 0x00, 0xe1, 0x00, 0xe1, 0x00
		};

		for(i=0;i<0x10;i++)
			state->tvram[(0x3fe0)+i*2] = default_memsw_data[i];
	}

	beep_set_frequency(machine->device("beeper"),2400);
	beep_set_state(machine->device("beeper"),0);

	/* 2dd interface ready line is ON by default */
	floppy_mon_w(floppy_get_device(machine, 0), CLEAR_LINE);
	floppy_mon_w(floppy_get_device(machine, 1), CLEAR_LINE);
	floppy_drive_set_ready_state(floppy_get_device(machine, 0), (1), 0);
	floppy_drive_set_ready_state(floppy_get_device(machine, 1), (1), 0);

	state->nmi_ff = 0;

	{
		UINT8 op_mode;
		UINT8 *ROM = machine->region("fdc_bios_2dd")->base();
		UINT8 *PRG = machine->region("fdc_data")->base();
		int i;

		op_mode = input_port_read(machine, "ROM_LOAD") & 1;

		for(i=0;i<0x1000;i++)
			ROM[i] = PRG[i+op_mode*0x8000];

	}
}

static INTERRUPT_GEN(pc9801_vrtc_irq)
{
	pc9801_state *state = device->machine->driver_data<pc9801_state>();
	#if 0
	address_space *space = cpu_get_address_space(device->machine->device("maincpu"), ADDRESS_SPACE_PROGRAM);
	static UINT8 test;

	if(input_code_pressed_once(device->machine,JOYCODE_BUTTON1))
		test^=1;

	if(test)
	{
		popmessage("Go hack go");
		space->write_word(0x55e,space->machine->rand());
	}
	#endif

	if(state->vrtc_irq_mask)
	{
		pic8259_ir2_w(device->machine->device("pic8259_master"), 1);
		state->vrtc_irq_mask = 0; // TODO: this irq auto-masks?
	}
}

static MACHINE_CONFIG_START( pc9801, pc9801_state )
	MCFG_CPU_ADD("maincpu", I8086, 5000000) //unknown clock
	MCFG_CPU_PROGRAM_MAP(pc9801_map)
	MCFG_CPU_IO_MAP(pc9801_io)
	MCFG_CPU_VBLANK_INT("screen",pc9801_vrtc_irq)

	MCFG_MACHINE_START(pc9801)
	MCFG_MACHINE_RESET(pc9801)

	MCFG_PIT8253_ADD( "pit8253", pit8253_config )
	MCFG_I8237_ADD( "dma8237", 5000000, dma8237_config ) //unknown clock
	MCFG_PIC8259_ADD( "pic8259_master", pic8259_master_config )
	MCFG_PIC8259_ADD( "pic8259_slave", pic8259_slave_config )
	MCFG_I8255A_ADD( "ppi8255_sys", ppi_system_intf )
	MCFG_I8255A_ADD( "ppi8255_prn", ppi_printer_intf )
	MCFG_I8255A_ADD( "ppi8255_fdd", ppi_fdd_intf )
	MCFG_UPD1990A_ADD("upd1990a", XTAL_32_768kHz, pc9801_upd1990a_intf)

	MCFG_UPD765A_ADD("upd765_2dd", upd765_2dd_intf)
	MCFG_FLOPPY_4_DRIVES_ADD(pc9801_floppy_config)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(640, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 200-1)

	MCFG_UPD7220_ADD("upd7220_chr", 5000000/2, hgdc_1_intf, upd7220_1_map)
	MCFG_UPD7220_ADD("upd7220_btm", 5000000/2, hgdc_2_intf, upd7220_2_map)

	MCFG_PALETTE_LENGTH(16)
	MCFG_PALETTE_INIT(pc9801)
	MCFG_GFXDECODE(pc9801)

	MCFG_SPEAKER_STANDARD_MONO("mono")

	MCFG_SOUND_ADD("opn", YM2203, 4000000) // unknown clock / divider
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MCFG_SOUND_ADD("beeper", BEEP, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS,"mono",0.50)
MACHINE_CONFIG_END

ROM_START( pc9801f )
	ROM_REGION( 0x18000, "ipl", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "urm01-02.bin", 0x00000, 0x4000, CRC(cde04615) SHA1(8f6fb587c0522af7a8131b45d13f8ae8fc60e8cd) )
	ROM_LOAD16_BYTE( "urm02-02.bin", 0x00001, 0x4000, CRC(9e39b8d1) SHA1(df1f3467050a41537cb9d071e4034f0506f07eda) )
	ROM_LOAD16_BYTE( "urm03-02.bin", 0x08000, 0x4000, CRC(95e79064) SHA1(c27d96949fad82aeb26e316200c15a4891e1063f) )
	ROM_LOAD16_BYTE( "urm04-02.bin", 0x08001, 0x4000, CRC(e4855a53) SHA1(223f66482c77409706cfc64c214cec7237c364e9) )
	ROM_LOAD16_BYTE( "urm05-02.bin", 0x10000, 0x4000, CRC(ffefec65) SHA1(106e0d920e857e59da12225a489ca2756ca405c1) )
	ROM_LOAD16_BYTE( "urm06-02.bin", 0x10001, 0x4000, CRC(1147760b) SHA1(4e0299091dfd53ac7988d40c5a6775a10389faac) )

	ROM_REGION( 0x1000, "fdc_bios_2dd", ROMREGION_ERASEFF )

	ROM_REGION( 0x10000, "fdc_data", ROMREGION_ERASEFF ) // 2dd fdc bios, presumably bad size (should be 0x800 for each rom)
	ROM_LOAD16_BYTE( "urf01-01.bin", 0x00000, 0x4000, BAD_DUMP CRC(2f5ae147) SHA1(69eb264d520a8fc826310b4fce3c8323867520ee) )
	ROM_LOAD16_BYTE( "urf02-01.bin", 0x00001, 0x4000, BAD_DUMP CRC(62a86928) SHA1(4160a6db096dbeff18e50cbee98f5d5c1a29e2d1) )

	ROM_REGION( 0x800, "kbd_mcu", ROMREGION_ERASEFF)
	ROM_LOAD( "mcu.bin", 0x0000, 0x0800, NO_DUMP ) //connected thru a i8251 UART, needs decapping

	/* note: ROM names of following two might be swapped */
	ROM_REGION( 0x1800, "chargen", 0 )
	ROM_LOAD( "d23128c-17.bin", 0x00000, 0x00800, BAD_DUMP CRC(eea57180) SHA1(4aa037c684b72ad4521212928137d3369174eb1e) ) //original is a bad dump, this is taken from i386 model
	ROM_LOAD("hn613128pac8.bin",0x00800, 0x01000, BAD_DUMP CRC(b5a15b5c) SHA1(e5f071edb72a5e9a8b8b1c23cf94a74d24cb648e) ) //bad dump, 8x16 charset? (it's on the kanji board)

	ROM_REGION( 0x20000, "kanji", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "24256c-x01.bin", 0x00000, 0x8000, CRC(28ec1375) SHA1(9d8e98e703ce0f483df17c79f7e841c5c5cd1692) )
	ROM_LOAD16_BYTE( "24256c-x02.bin", 0x00001, 0x8000, CRC(90985158) SHA1(78fb106131a3f4eb054e87e00fe4f41193416d65) )
	ROM_LOAD16_BYTE( "24256c-x03.bin", 0x10000, 0x8000, CRC(d4893543) SHA1(eb8c1bee0f694e1e0c145a24152222d4e444e86f) )
	ROM_LOAD16_BYTE( "24256c-x04.bin", 0x10001, 0x8000, CRC(5dec0fc2) SHA1(41000da14d0805ed0801b31eb60623552e50e41c) )

	ROM_REGION( 0x80000, "pcg", ROMREGION_ERASEFF )

ROM_END

COMP( 1983, pc9801f,   0,       0,     pc9801,   pc9801,   0, "Nippon Electronic Company",   "PC-9801F",  GAME_NOT_WORKING | GAME_IMPERFECT_SOUND)
