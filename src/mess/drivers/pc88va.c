/********************************************************************************************

	PC-88VA (c) 1987 NEC

	A follow up of the regular PC-8801. It can also run PC-8801 software in compatible mode

	preliminary driver by Angelo Salese

	TODO:
	- Does this system have one or two CPUs? I'm prone to think that the V30 does all the job
	  and then enters into z80 compatible mode for PC-8801 emulation.

********************************************************************************************/

#include "emu.h"
#include "cpu/nec/nec.h"
//#include "cpu/z80/z80.h"
#include "machine/i8255a.h"

static UINT16 *palram;
static UINT16 bank_reg;
static struct
{
	UINT16 tvram_vreg_offset;
	UINT16 attr_offset;
	UINT16 spr_offset;
	UINT8 disp_on;
	UINT8 spr_on;
	UINT8 pitch;
	UINT8 line_height;
	UINT8 h_line_pos;
	UINT8 blink;
	UINT16 cur_pos_x,cur_pos_y;
}tsp;

static UINT8 backupram_wp;

static VIDEO_START( pc88va )
{

}

static void draw_sprites(running_machine *machine, bitmap_t *bitmap, const rectangle *cliprect)
{
	UINT16 *tvram = (UINT16 *)memory_region(machine, "tvram");
	int offs,i;

	offs = tsp.spr_offset;
	for(i=0;i<(0x100);i+=(8))
	{
		int xp,yp,sw,md,xsize,ysize,spda,fg_col,bc;
		int x_i,y_i;

		ysize = (tvram[(offs + i + 0) / 2] & 0xfc00) >> 10;
		sw = (tvram[(offs + i + 0) / 2] & 0x200) >> 9;
		yp = (tvram[(offs + i + 0) / 2] & 0x1ff);
		xsize = (tvram[(offs + i + 2) / 2] & 0xf800) >> 11;
		md = (tvram[(offs + i + 2) / 2] & 0x400) >> 10;
		xp = (tvram[(offs + i + 2) / 2] & 0x3ff);
		spda = (tvram[(offs + i + 4) / 2] & 0xffff);
		fg_col = (tvram[(offs + i + 6) / 2] & 0xf0) >> 4;
		bc = (tvram[(offs + i + 6) / 2] & 0x08) >> 3;

		if(!sw)
			continue;

		if(yp & 0x100)
		{
			yp &= 0xff;
			yp = 0x100 - yp;
		}

		if(0) // uhm, makes more sense without the sign?
		if(xp & 0x200)
		{
			xp &= 0x1ff;
			xp = 0x200 - xp;
		}

		if(md) // 1bpp mode
		{
			xsize = (xsize + 1) * 32;
			ysize = (ysize + 1) * 4;

			for(y_i=0;y_i<ysize;y_i++)
			{
				for(x_i=0;x_i<xsize;x_i++)
				{
					//UINT8 pen;

					// TODO: hook up drawing routines

					*BITMAP_ADDR32(bitmap, yp+y_i, xp+x_i) = machine->pens[fg_col];
				}
			}
		}
		else // 4bpp mode
		{
			xsize = (xsize + 1) * 8;
			ysize = (ysize + 1) * 4;
			// ...
		}
	}
}

static VIDEO_UPDATE( pc88va )
{
	bitmap_fill(bitmap, cliprect, 0);

	if(tsp.spr_on)
		draw_sprites(screen->machine,bitmap,cliprect);

	return 0;
}

static READ16_HANDLER( sys_mem_r )
{
	switch((bank_reg & 0xf00) >> 8)
	{
		case 0: // select bus slot
			return 0xffff;
		case 1: // TVRAM
		{
			UINT16 *tvram = (UINT16 *)memory_region(space->machine, "tvram");

			if(((offset*2) & 0x30000) == 0)
				return tvram[offset];

			return 0xffff;
		}
		case 4:
		{
			UINT16 *gvram = (UINT16 *)memory_region(space->machine, "gvram");

			return gvram[offset];
		}
		case 8: // kanji ROM
		case 9:
		{
			UINT16 *knj_ram = (UINT16 *)memory_region(space->machine, "kanji");
			UINT32 knj_offset;

			knj_offset = (offset + (((bank_reg & 0x100) >> 8)*0x20000));

			/* 0x00000 - 0x3ffff Kanji ROM 1*/
			/* 0x40000 - 0x4ffff Kanji ROM 2*/
			/* 0x50000 - 0x53fff Backup RAM */
			/* anything else is a NOP (I presume?) */

			return knj_ram[knj_offset];
		}
		break;
		case 0xc: // Dictionary ROM
		case 0xd:
		{
			UINT16 *dic_rom = (UINT16 *)memory_region(space->machine, "dictionary");
			UINT32 dic_offset;

			dic_offset = (offset + (((bank_reg & 0x100) >> 8)*0x20000));

			return dic_rom[dic_offset];
		}
	}

	return 0xffff;
}

static WRITE16_HANDLER( sys_mem_w )
{
	switch((bank_reg & 0xf00) >> 8)
	{
		case 0: // select bus slot
			break;
		case 1: // TVRAM
		{
			UINT16 *tvram = (UINT16 *)memory_region(space->machine, "tvram");

			if(((offset*2) & 0x30000) == 0)
				COMBINE_DATA(&tvram[offset]);
		}
		break;
		case 4: // TVRAM
		{
			UINT16 *gvram = (UINT16 *)memory_region(space->machine, "gvram");

			COMBINE_DATA(&gvram[offset]);
		}
		break;
		case 8: // kanji ROM, backup RAM at 0xb0000 - 0xb3fff
		case 9:
		{
			UINT16 *knj_ram = (UINT16 *)memory_region(space->machine, "kanji");
			UINT32 knj_offset;

			knj_offset = ((offset) + (((bank_reg & 0x100) >> 8)*0x20000));

			if(knj_offset >= 0x50000/2 && knj_offset <= 0x53fff/2) // TODO: there's an area that can be write protected
			{
				COMBINE_DATA(&knj_ram[knj_offset]);
				gfx_element_mark_dirty(space->machine->gfx[0], (knj_offset * 2) / 8);
				gfx_element_mark_dirty(space->machine->gfx[1], (knj_offset * 2) / 32);
			}
		}
		break;
		case 0xc: // Dictionary ROM
		case 0xd:
		{
			// NOP?
		}
		break;
	}
}

static ADDRESS_MAP_START( pc88va_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000, 0x7ffff) AM_RAM
//	AM_RANGE(0x80000, 0x9ffff) AM_RAM // EMM
	AM_RANGE(0xa0000, 0xdffff) AM_READWRITE(sys_mem_r,sys_mem_w)
	AM_RANGE(0xe0000, 0xeffff) AM_ROMBANK("rom00_bank")
	AM_RANGE(0xf0000, 0xfffff) AM_ROMBANK("rom10_bank")
ADDRESS_MAP_END

static READ8_HANDLER( idp_status_r )
{
/*
    x--- ---- LP   Light-pen signal detection (with VA use failure)
    -x-- ---- VB   Vertical elimination period
    --x- ---- SC   Sprite control (sprite over/collision)
    ---x ---- ER   Error occurrence
    ---- x--- In the midst of execution of EMEN emulation development
    ---- -x-- In the midst of BUSY command execution
    ---- --x- OBF output data buffer full
    ---- ---x IBF input data buffer full (command/parameter commonness)
*/
	return 0x00;
}

static UINT8 cmd,buf_size,buf_index;
static UINT8 buf_ram[16];

#define SYNC   0x10
#define DSPON  0x12
#define DSPOFF 0x13
#define DSPDEF 0x14
#define CURDEF 0x15
#define ACTSCR 0x16
#define CURS   0x1e
#define EMUL   0x8c
#define EXIT   0x88
#define SPRON  0x82
#define SPROFF 0x83
#define SPRSW  0x85
#define SPROV  0x81

static WRITE8_HANDLER( idp_command_w )
{
	switch(data)
	{
		/* 0x10 - SYNC: sets CRTC values */
		case SYNC:   cmd = SYNC;  buf_size = 14; buf_index = 0; break;

		/* 0x12 - DSPON: set DiSPlay ON and set up tvram table vreg */
		case DSPON:  cmd = DSPON; buf_size = 3;  buf_index = 0; break;

		/* 0x13 - DSPOFF: set DiSPlay OFF */
		case DSPOFF: tsp.disp_on = 0; break;

		/* 0x14 - DSPDEF: set DiSPlay DEFinitions */
		case DSPDEF: cmd = DSPDEF; buf_size = 6; buf_index = 0; break;

		/* 0x15 - CURDEF: set CURsor DEFinition */
		case CURDEF: cmd = CURDEF; buf_size = 1; buf_index = 0; break;

		/* 0x16 - ACTSCR: ??? */
		case ACTSCR: cmd = ACTSCR; buf_size = 1; buf_index = 0; break;

		/* 0x15 - CURS: set CURSor position */
		case CURS:   cmd = CURS;   buf_size = 4; buf_index = 0; break;

		/* 0x8c - EMUL: set 3301 EMULation */
		case EMUL:   cmd = EMUL;   buf_size = 4; buf_index = 0; break;

		/* 0x88 - EXIT: ??? */
		case EXIT: break;

		/* 0x82 - SPRON: set SPRite ON */
		case SPRON:   cmd = SPRON;  buf_size = 3; buf_index = 0; break;

		/* 0x83 - SPROFF: set SPRite OFF */
		case SPROFF:  tsp.spr_on = 0; break;

		/* 0x85 - SPRSW: ??? */
		case SPRSW:   cmd = SPRSW;  buf_size = 1; buf_index = 0; break;

		/* 0x81 - SPROV: set SPRite OVerflow information */
		case 0x81: /* ... */ break;

		/* TODO: 0x89 shouldn't trigger, should be one of the above commands */
		default:   cmd = 0x00; printf("PC=%05x: Unknown IDP %02x cmd set\n",cpu_get_pc(space->cpu),data); break;
	}
}

/* TODO: very preliminary, needs something showable first */
static void execute_sync_cmd(running_machine *machine)
{
	/*
		???? ???? [0] - unknown
		???? ???? [1] - unknown
		--xx xxxx [2] - h blank start
		--xx xxxx [3] - h border start
		xxxx xxxx [4] - h visible area
		--xx xxxx [5] - h border end
		--xx xxxx [6] - h blank end
		--xx xxxx [7] - h sync
		--xx xxxx [8] - v blank start
		--xx xxxx [9] - v border start
		xxxx xxxx [A] - v visible area
		-x-- ---- [B] - v visible area (bit 9)
		--xx xxxx [C] - v border end
		--xx xxxx [D] - v blank end
		--xx xxxx [E] - v sync
	*/
	rectangle visarea;
	attoseconds_t refresh;
	static UINT16 x_vis_area,y_vis_area;

	//printf("V blank start: %d\n",(sync_cmd[0x8]));
	//printf("V border start: %d\n",(sync_cmd[0x9]));
	//printf("V Visible Area: %d\n",(sync_cmd[0xa])|((sync_cmd[0xb] & 0x40)<<2));
	//printf("V border end: %d\n",(sync_cmd[0xc]));
	//printf("V blank end: %d\n",(sync_cmd[0xd]));

	x_vis_area = buf_ram[4] * 4;
	y_vis_area = (buf_ram[0xa])|((buf_ram[0xb] & 0x40)<<2);

	visarea.min_x = 0;
	visarea.min_y = 0;
	visarea.max_x = x_vis_area - 1;
	visarea.max_y = y_vis_area - 1;

	//if(y_vis_area == 400)
	//	refresh = HZ_TO_ATTOSECONDS(24800) * x_vis_area * y_vis_area; //24.8 KHz
	//else
	//	refresh = HZ_TO_ATTOSECONDS(15730) * x_vis_area * y_vis_area; //15.73 KHz

	refresh = HZ_TO_ATTOSECONDS(60);

	machine->primary_screen->configure(640, 480, visarea, refresh);
}

static void execute_dspon_cmd(running_machine *machine)
{
	/*
	[0] text table offset (hi word)
	[1] unknown
	[2] unknown
	*/
	tsp.tvram_vreg_offset = buf_ram[0] << 8;
	tsp.disp_on = 1;
}

static void execute_dspdef_cmd(running_machine *machine)
{
	/*
	[0] attr offset (lo word)
	[1] attr offset (hi word)
	[2] pitch
	[3] line height
	[4] h line position
	[5] blink number
	*/
	tsp.attr_offset = buf_ram[0] | buf_ram[1] << 8;
	tsp.pitch = (buf_ram[2] & 0xf0) >> 4;
	tsp.line_height = buf_ram[3] + 1;
	tsp.h_line_pos = buf_ram[4];
	tsp.blink = (buf_ram[5] & 0xf8) >> 3;
}

static void execute_curdef_cmd(running_machine *machine)
{
	/*
	xxxx x--- [0] Sprite Cursor number (sprite RAM entry)
	---- --x- [0] (bit in sprite def)
	---- ---x [0] Blink Enable
	*/

	/* TODO: needs basic sprite emulation */
}

static void execute_actscr_cmd(running_machine *machine)
{
	/*
	xxxx xxxx [0] param * 32 (???)
	*/

	/* TODO: no idea about this command */
}

static void execute_curs_cmd(running_machine *machine)
{
	/*
	[0] Cursor Position Y (lo word)
	[1] Cursor Position Y (hi word)
	[2] Cursor Position X (lo word)
	[3] Cursor Position X (hi word)
	*/

	tsp.cur_pos_y = buf_ram[0] | buf_ram[1] << 8;
	tsp.cur_pos_x = buf_ram[2] | buf_ram[3] << 8;
}

static void execute_emul_cmd(running_machine *machine)
{
	// TODO: this starts 3301 video emulation
}

static void execute_spron_cmd(running_machine *machine)
{
	/*
	[0] Sprite Table Offset (hi word)
	[1] (unknown / reserved)
	xxxx x--- [2] HSPN: some kind of Sprite Over register
	---- --x- [2] MG: ???
	---- ---x [2] GR: ???
	*/
	tsp.spr_offset = buf_ram[0] << 8;
	tsp.spr_on = 1;
	printf("SPR TABLE %02x %02x %02x\n",buf_ram[0],buf_ram[1],buf_ram[2]);
}

static void execute_sprsw_cmd(running_machine *machine)
{
	/* TODO: no idea about this command, some kind of manual sprite switch? */
}

static WRITE8_HANDLER( idp_param_w )
{
	if(cmd == DSPOFF || cmd == EXIT || cmd == SPROFF || cmd == SPROV) // no param commands
		return;

	buf_ram[buf_index] = data;
	buf_index++;

	if(buf_index >= buf_size)
	{
		buf_index = 0;
		switch(cmd)
		{
			case SYNC: 		execute_sync_cmd(space->machine); 	break;
			case DSPON: 	execute_dspon_cmd(space->machine); 	break;
			case DSPDEF: 	execute_dspdef_cmd(space->machine); break;
			case CURDEF: 	execute_curdef_cmd(space->machine); break;
			case ACTSCR: 	execute_actscr_cmd(space->machine); break;
			case CURS:		execute_curs_cmd(space->machine);   break;
			case EMUL:		execute_emul_cmd(space->machine);   break;
			case SPRON:		execute_spron_cmd(space->machine);  break;
			case SPRSW:		execute_sprsw_cmd(space->machine);	break;

			default:
				//printf("%02x\n",data);
				break;
		}
	}
}

static WRITE16_HANDLER( palette_ram_w )
{
	int r,g,b;
	COMBINE_DATA(&palram[offset]);

	b = (palram[offset] & 0x001e) >> 1;
	r = (palram[offset] & 0x03c0) >> 6;
	g = (palram[offset] & 0x7800) >> 11;

	palette_set_color_rgb(space->machine,offset,pal4bit(r),pal4bit(g),pal4bit(b));
}

static READ16_HANDLER( sys_port4_r )
{
	UINT8 vrtc,sw1;
	vrtc = (space->machine->primary_screen->vpos() < 200) ? 0x20 : 0x00; // vblank

	sw1 = (input_port_read(space->machine, "DSW") & 1) ? 2 : 0;

	return vrtc | sw1;
}

static READ16_HANDLER( bios_bank_r )
{
	return bank_reg;
}

static WRITE16_HANDLER( bios_bank_w )
{
	/*
	-x-- ---- ---- ---- SMM (compatibility mode)
	---x ---- ---- ---- GMSP (VRAM drawing Mode)
	---- xxxx ---- ---- SMBC (0xa0000 - 0xdffff RAM bank)
	---- ---- xxxx ---- RBC1 (0xf0000 - 0xfffff ROM bank)
	---- ---- ---- xxxx RBC0 (0xe0000 - 0xeffff ROM bank)
	*/
	if ((mem_mask&0xffff) == 0xffff)
		bank_reg = data;
	else if ((mem_mask & 0xffff) == 0xff00)
		bank_reg = (data & 0xff00) | (bank_reg & 0x00ff);
	else if ((mem_mask & 0xffff) == 0x00ff)
		bank_reg = (data & 0x00ff) | (bank_reg & 0xff00);


	/* RBC1 */
	{
		UINT8 *ROM10 = memory_region(space->machine, "rom10");

		if((bank_reg & 0xe0) == 0x00)
			memory_set_bankptr(space->machine, "rom10_bank", &ROM10[(bank_reg & 0x10) ? 0x10000 : 0x00000]);
	}

	/* RBC0 */
	{
		UINT8 *ROM00 = memory_region(space->machine, "rom00");

		memory_set_bankptr(space->machine, "rom00_bank", &ROM00[(bank_reg & 0xf)*0x10000]); // TODO: docs says that only 0 - 5 are used, dunno why ...
	}
}

static READ8_HANDLER( rom_bank_r )
{
	return 0xff; // bit 7 low is va91 rom bank status
}

static READ8_HANDLER( key_r )
{
	// note row C bit 2 does something at POST ... some kind of test mode?
	static const char *const keynames[] = { "KEY0", "KEY1", "KEY2", "KEY3",
	                                        "KEY4", "KEY5", "KEY6", "KEY7",
	                                        "KEY8", "KEY9", "KEYA", "KEYB",
	                                        "KEYC", "KEYD", "KEYE", "KEYF" };

	return input_port_read(space->machine, keynames[offset]);
}

static WRITE16_HANDLER( backupram_wp_1_w )
{
	backupram_wp = 1;
}

static WRITE16_HANDLER( backupram_wp_0_w )
{
	backupram_wp = 0;
}

static READ8_HANDLER( hdd_status_r )
{
	return 0x20;
}

static READ8_HANDLER( fdc_r )
{
	return mame_rand(space->machine);
}

static ADDRESS_MAP_START( pc88va_io_map, ADDRESS_SPACE_IO, 16 )
	AM_RANGE(0x0000, 0x000f) AM_READ8(key_r,0xffff) // Keyboard ROW reading
//	AM_RANGE(0x0010, 0x0010) Printer / Calendar Clock Interface
//	AM_RANGE(0x0020, 0x0021) RS-232C
//	AM_RANGE(0x0030, 0x0030) (R) DSW1 (W) Text Control Port 0
//	AM_RANGE(0x0031, 0x0031) (R) DSW2 (W) System Port 1
//	AM_RANGE(0x0032, 0x0032) (R) ? (W) System Port 2
//	AM_RANGE(0x0034, 0x0034) GVRAM Control Port 1
//	AM_RANGE(0x0035, 0x0035) GVRAM Control Port 2
	AM_RANGE(0x0040, 0x0041) AM_READ(sys_port4_r) // (R) System Port 4 (W) System port 3 (strobe port)
//	AM_RANGE(0x0044, 0x0045) YM2203
//	AM_RANGE(0x0046, 0x0047) YM2203 mirror
//	AM_RANGE(0x005c, 0x005c) (R) GVRAM status
//	AM_RANGE(0x005c, 0x005f) (W) GVRAM selection
//	AM_RANGE(0x0070, 0x0070) ? (*)
//	AM_RANGE(0x0071, 0x0071) Expansion ROM select (*)
//	AM_RANGE(0x0078, 0x0078) Memory offset increment (*)
//	AM_RANGE(0x0080, 0x0081) HDD related
	AM_RANGE(0x0082, 0x0083) AM_READ8(hdd_status_r,0xff)// HDD control, byte access 7-0
//	AM_RANGE(0x00bc, 0x00bf) d8255 1
//	AM_RANGE(0x00e2, 0x00e3) Expansion RAM selection (*)
//	AM_RANGE(0x00e4, 0x00e4) 8214 IRQ control (*)
//	AM_RANGE(0x00e6, 0x00e6) 8214 IRQ mask (*)
//	AM_RANGE(0x00e8, 0x00e9) ? (*)
//	AM_RANGE(0x00ec, 0x00ed) ? (*)
	AM_RANGE(0x00fc, 0x00ff) AM_DEVREADWRITE8("d8255_2", i8255a_r,i8255a_w,0xffff) // d8255 2, FDD

//	AM_RANGE(0x0100, 0x0101) Screen Control Register
//	AM_RANGE(0x0102, 0x0103) Graphic Screen Control Register
//	AM_RANGE(0x0106, 0x0107) Palette Control Register
//	AM_RANGE(0x0108, 0x0109) Direct Color Control Register
//	AM_RANGE(0x010a, 0x010b) Picture Mask Mode Register
//	AM_RANGE(0x010c, 0x010d) Color Palette Mode Register
//	AM_RANGE(0x010e, 0x010f) Backdrop Color Register
//	AM_RANGE(0x0110, 0x0111) Color Code/Plain Mask Register
//	AM_RANGE(0x0124, 0x0125) ? (related to Transparent Color of Graphic Screen 0)
//	AM_RANGE(0x0126, 0x0127) ? (related to Transparent Color of Graphic Screen 1)
//	AM_RANGE(0x012e, 0x012f) ? (related to Transparent Color of Text/Sprite)
//	AM_RANGE(0x0130, 0x0137) Picture Mask Parameter
	AM_RANGE(0x0142, 0x0143) AM_READWRITE8(idp_status_r,idp_command_w,0x00ff) //Text Controller (IDP) - (R) Status (W) command
	AM_RANGE(0x0146, 0x0147) AM_WRITE8(idp_param_w,0x00ff) //Text Controller (IDP) - (R/W) Parameter
//	AM_RANGE(0x0148, 0x0149) Text control port 1
//	AM_RANGE(0x014c, 0x014f) ? CG Port
//	AM_RANGE(0x0150, 0x0151) System Operational Mode
	AM_RANGE(0x0152, 0x0153) AM_READWRITE(bios_bank_r,bios_bank_w) // Memory Map Register
//	AM_RANGE(0x0154, 0x0155) Refresh Register (wait states)
	AM_RANGE(0x0156, 0x0157) AM_READ8(rom_bank_r,0x00ff) // ROM bank status
//	AM_RANGE(0x0158, 0x0159) Interruption Mode Modification
//	AM_RANGE(0x015c, 0x015f) NMI mask port (strobe port)
//	AM_RANGE(0x0160, 0x016f) DMA Controller
//	AM_RANGE(0x0184, 0x018b) IRQ Controller
//	AM_RANGE(0x0190, 0x0191) System Port 5
//	AM_RANGE(0x0196, 0x0197) Keyboard sub CPU command port
	AM_RANGE(0x0198, 0x0199) AM_WRITE(backupram_wp_1_w) //Backup RAM write inhibit
	AM_RANGE(0x019a, 0x019b) AM_WRITE(backupram_wp_0_w) //Backup RAM write permission
//	AM_RANGE(0x01a0, 0x01a7) TCU (timer counter unit)
//	AM_RANGE(0x01a8, 0x01a9) General-purpose timer 3 control port
	AM_RANGE(0x01b0, 0x01bb) AM_READ8(fdc_r,0xffff)// FDC related (765)
//	AM_RANGE(0x01c0, 0x01c1) ?
	AM_RANGE(0x01c8, 0x01cf) AM_DEVREADWRITE8("d8255_3", i8255a_r,i8255a_w,0xff00) //i8255 3 (byte access)
//	AM_RANGE(0x01d0, 0x01d1) Expansion RAM bank selection
//	AM_RANGE(0x0200, 0x021f) Frame buffer 0 control parameter
//	AM_RANGE(0x0220, 0x023f) Frame buffer 1 control parameter
//	AM_RANGE(0x0240, 0x025f) Frame buffer 2 control parameter
//	AM_RANGE(0x0260, 0x027f) Frame buffer 3 control parameter
	AM_RANGE(0x0300, 0x033f) AM_RAM_WRITE(palette_ram_w) AM_BASE(&palram) // Palette RAM (xBBBBxRRRRxGGGG format)

//	AM_RANGE(0x0500, 0x05ff) GVRAM
//	AM_RANGE(0x1000, 0xfeff) user area (???)
//	AM_RANGE(0xff00, 0xffff) CPU internal use
ADDRESS_MAP_END
// (*) are specific N88 V1 / V2 ports

#if 0
static ADDRESS_MAP_START( pc88va_z80_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( pc88va_z80_io_map, ADDRESS_SPACE_IO, 8 )
ADDRESS_MAP_END
#endif

/* TODO: active low or active high? */
static INPUT_PORTS_START( pc88va )
	PORT_START("KEY0")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("0 (PAD)") PORT_CODE(KEYCODE_0_PAD) PORT_CHAR(UCHAR_MAMEKEY(0_PAD))
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("1 (PAD)") PORT_CODE(KEYCODE_1_PAD) PORT_CHAR(UCHAR_MAMEKEY(1_PAD))
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("2 (PAD)") PORT_CODE(KEYCODE_2_PAD) PORT_CHAR(UCHAR_MAMEKEY(2_PAD))
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("3 (PAD)") PORT_CODE(KEYCODE_3_PAD) PORT_CHAR(UCHAR_MAMEKEY(3_PAD))
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("4 (PAD)") PORT_CODE(KEYCODE_4_PAD) PORT_CHAR(UCHAR_MAMEKEY(4_PAD))
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("5 (PAD)") PORT_CODE(KEYCODE_5_PAD) PORT_CHAR(UCHAR_MAMEKEY(5_PAD))
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("6 (PAD)") PORT_CODE(KEYCODE_6_PAD) PORT_CHAR(UCHAR_MAMEKEY(6_PAD))
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("7 (PAD)") PORT_CODE(KEYCODE_7_PAD) PORT_CHAR(UCHAR_MAMEKEY(7_PAD))

	PORT_START("KEY1")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("8 (PAD)") PORT_CODE(KEYCODE_8_PAD) PORT_CHAR(UCHAR_MAMEKEY(8_PAD))
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("9 (PAD)") PORT_CODE(KEYCODE_9_PAD) PORT_CHAR(UCHAR_MAMEKEY(9_PAD))
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("* (PAD)") PORT_CODE(KEYCODE_ASTERISK) PORT_CHAR(UCHAR_MAMEKEY(SLASH_PAD))
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("+ (PAD)") PORT_CODE(KEYCODE_PLUS_PAD) PORT_CHAR(UCHAR_MAMEKEY(PLUS_PAD))
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("= (PAD)") // PORT_CODE(KEYCODE_EQUAL_PAD) PORT_CHAR(UCHAR_MAMEKEY(EQUAL_PAD))
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME(", (PAD)") // PORT_CODE(KEYCODE_EQUAL_PAD) PORT_CHAR(UCHAR_MAMEKEY(EQUAL_PAD))
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME(". (PAD)") // PORT_CODE(KEYCODE_EQUAL_PAD) PORT_CHAR(UCHAR_MAMEKEY(EQUAL_PAD))
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("RETURN (PAD)") //PORT_CODE(KEYCODE_RETURN_PAD) PORT_CHAR(UCHAR_MAMEKEY(RETURN_PAD))

	PORT_START("KEY2")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("@") // PORT_CODE(KEYCODE_8_PAD) PORT_CHAR(UCHAR_MAMEKEY(8_PAD))
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')

	PORT_START("KEY3")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')

	PORT_START("KEY4")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')

	PORT_START("KEY5")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("[")
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("\xC2\xA5") /* Yen */
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("]")
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("^")
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("-")

	PORT_START("KEY6")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("0")  PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("1 !")  PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("2 \"") PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("3 #")  PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("4 $")  PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("5 %")  PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("6 &")  PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("7 '")  PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')

	PORT_START("KEY7")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("8 (")  PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("9 )")  PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME(": *")  //PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("; +")  //PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME(", <")  //PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME(". >")  //PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("/ ?")  //PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("/ ?")  //PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')

	PORT_START("KEY8")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("CLR") // PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("Up")  PORT_CODE(KEYCODE_UP)  /* Up */
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT)  /* Right */
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("INSDEL")
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("GRPH")
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("KANA") PORT_CODE(KEYCODE_LALT)
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT)
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL)

	PORT_START("KEY9")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("STOP") // PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F1 (mirror)") PORT_CODE(KEYCODE_F1)
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F2 (mirror)") PORT_CODE(KEYCODE_F2)
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F3 (mirror)") PORT_CODE(KEYCODE_F3)
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F4 (mirror)") PORT_CODE(KEYCODE_F4)
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F5 (mirror)") PORT_CODE(KEYCODE_F5)
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("SPACES") // PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC)

	PORT_START("KEYA")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("TAB") PORT_CODE(KEYCODE_TAB)
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN)  /* Down */
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT)  /* Left */
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("HELP") // PORT_CODE(KEYCODE_TAB)
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("COPY") // PORT_CODE(KEYCODE_TAB)
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("- (mirror)") // PORT_CODE(KEYCODE_TAB)
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("/") // PORT_CODE(KEYCODE_TAB)
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("Caps Lock") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))

	PORT_START("KEYB")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD)  PORT_NAME("Roll Up")  PORT_CODE(KEYCODE_PGUP)  /* Roll Up */
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD)  PORT_NAME("Roll Down")  PORT_CODE(KEYCODE_PGDN)  /* Roll Down */
	PORT_BIT(0xfc,IP_ACTIVE_LOW,IPT_UNUSED)

	PORT_START("KEYC")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F1") PORT_CODE(KEYCODE_F1)
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F2") PORT_CODE(KEYCODE_F2)
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F3") PORT_CODE(KEYCODE_F3)
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F4") PORT_CODE(KEYCODE_F4)
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F5") PORT_CODE(KEYCODE_F5)
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("Backspace") PORT_CODE(KEYCODE_BACKSPACE)
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("INS") PORT_CODE(KEYCODE_INSERT)
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("DEL") PORT_CODE(KEYCODE_DEL)

	PORT_START("KEYD")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F6") PORT_CODE(KEYCODE_F6)
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F7") PORT_CODE(KEYCODE_F7)
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F8") PORT_CODE(KEYCODE_F8)
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F9") PORT_CODE(KEYCODE_F9)
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F10") PORT_CODE(KEYCODE_F10)
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) // Conversion?
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) // Decision?
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE)

	/* TODO: I don't understand the meaning of several of these */
	PORT_START("KEYE")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD)
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD)
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("Left Shift") PORT_CODE(KEYCODE_LSHIFT)
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("Right Shift") PORT_CODE(KEYCODE_RSHIFT)
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD)
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD)
	PORT_BIT(0xc0,IP_ACTIVE_LOW,IPT_UNUSED)

	PORT_START("KEYF")
	PORT_BIT(0xff,IP_ACTIVE_LOW,IPT_UNUSED)

	PORT_START("DSW")
	PORT_DIPNAME( 0x01, 0x01, "CRT Mode" )
	PORT_DIPSETTING(    0x01, "15.7 KHz" )
	PORT_DIPSETTING(    0x00, "24.8 KHz" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

static MACHINE_RESET( pc88va )
{
	UINT8 *ROM00 = memory_region(machine, "rom00");
	UINT8 *ROM10 = memory_region(machine, "rom10");

	memory_set_bankptr(machine, "rom10_bank", &ROM10[0x00000]);
	memory_set_bankptr(machine, "rom00_bank", &ROM00[0x00000]);

	bank_reg = 0x4100;
	backupram_wp = 1;
}

static const gfx_layout pc88va_chars_8x8 =
{
	8,8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_layout pc88va_chars_16x16 =
{
	16,16,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	16*16
};

/* decoded for debugging purpose, this will be nuked in the end... */
static GFXDECODE_START( pc88va )
	GFXDECODE_ENTRY( "kanji",   0x00000, pc88va_chars_8x8,    0, 1 )
	GFXDECODE_ENTRY( "kanji",   0x00000, pc88va_chars_16x16,  0, 1 )
GFXDECODE_END

static READ8_DEVICE_HANDLER( fdd_porta_r )
{
	return 0xff;
}

static READ8_DEVICE_HANDLER( fdd_portb_r )
{
	return 0xff;
}

static READ8_DEVICE_HANDLER( fdd_portc_r )
{
	static UINT8 test1,test2;

	test1^=4;
	test2^=1;

	return 0xff ^ test1 ^ test2;
}

static WRITE8_DEVICE_HANDLER( fdd_porta_w )
{
	// ...
}

static WRITE8_DEVICE_HANDLER( fdd_portb_w )
{
	// ...
}

static WRITE8_DEVICE_HANDLER( fdd_portc_w )
{
	// ...
}

static I8255A_INTERFACE( fdd_intf )
{
	DEVCB_HANDLER(fdd_porta_r),						/* Port A read */
	DEVCB_HANDLER(fdd_portb_r),						/* Port B read */
	DEVCB_HANDLER(fdd_portc_r),						/* Port C read */
	DEVCB_HANDLER(fdd_porta_w),						/* Port A write */
	DEVCB_HANDLER(fdd_portb_w),						/* Port B write */
	DEVCB_HANDLER(fdd_portc_w)						/* Port C write */
};

static READ8_DEVICE_HANDLER( r232_ctrl_porta_r )
{
	static UINT8 sw5, sw4, sw3, sw2;

	sw5 = (input_port_read(device->machine, "DSW") & 0x10);
	sw4 = (input_port_read(device->machine, "DSW") & 0x08);
	sw3 = (input_port_read(device->machine, "DSW") & 0x04);
	sw2 = (input_port_read(device->machine, "DSW") & 0x02);

	return 0xe1 | sw5 | sw4 | sw3 | sw2;
}

static READ8_DEVICE_HANDLER( r232_ctrl_portb_r )
{
	static UINT8 xsw1;

	xsw1 = (input_port_read(device->machine, "DSW") & 1) ? 0 : 8;

	return 0xf7 | xsw1;
}

static READ8_DEVICE_HANDLER( r232_ctrl_portc_r )
{
	return 0xff;
}

static WRITE8_DEVICE_HANDLER( r232_ctrl_porta_w )
{
	// ...
}

static WRITE8_DEVICE_HANDLER( r232_ctrl_portb_w )
{
	// ...
}

static WRITE8_DEVICE_HANDLER( r232_ctrl_portc_w )
{
	// ...
}

static I8255A_INTERFACE( r232c_ctrl_intf )
{
	DEVCB_HANDLER(r232_ctrl_porta_r),						/* Port A read */
	DEVCB_HANDLER(r232_ctrl_portb_r),						/* Port B read */
	DEVCB_HANDLER(r232_ctrl_portc_r),						/* Port C read */
	DEVCB_HANDLER(r232_ctrl_porta_w),						/* Port A write */
	DEVCB_HANDLER(r232_ctrl_portb_w),						/* Port B write */
	DEVCB_HANDLER(r232_ctrl_portc_w)						/* Port C write */
};

static MACHINE_CONFIG_START( pc88va, driver_device )

	MDRV_CPU_ADD("maincpu", V30, 8000000)        /* 8 MHz */
	MDRV_CPU_PROGRAM_MAP(pc88va_map)
	MDRV_CPU_IO_MAP(pc88va_io_map)

	#if 0
	MDRV_CPU_ADD("subcpu", Z80, 8000000)        /* 8 MHz */
	MDRV_CPU_PROGRAM_MAP(pc88va_z80_map)
	MDRV_CPU_IO_MAP(pc88va_z80_io_map)
	#endif

	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 200-1)
	MDRV_PALETTE_LENGTH(32)
//	MDRV_PALETTE_INIT( pc8801 )
	MDRV_GFXDECODE( pc88va )

	MDRV_VIDEO_START( pc88va )
	MDRV_VIDEO_UPDATE( pc88va )

	MDRV_MACHINE_RESET( pc88va )

	MDRV_I8255A_ADD( "d8255_2", fdd_intf )
	MDRV_I8255A_ADD( "d8255_3", r232c_ctrl_intf )


MACHINE_CONFIG_END


ROM_START( pc88va )
	ROM_REGION( 0x100000, "maincpu", ROMREGION_ERASEFF )

	ROM_REGION( 0x100000, "subcpu", ROMREGION_ERASEFF )

	ROM_REGION( 0x100000, "rom00", ROMREGION_ERASEFF ) // 0xe0000 - 0xeffff
	ROM_LOAD( "varom00.rom",   0x00000, 0x80000, CRC(98c9959a) SHA1(bcaea28c58816602ca1e8290f534360f1ca03fe8) )
	ROM_LOAD( "varom08.rom",   0x80000, 0x20000, CRC(eef6d4a0) SHA1(47e5f89f8b0ce18ff8d5d7b7aef8ca0a2a8e3345) )

	ROM_REGION( 0x20000, "rom10", 0 ) // 0xf0000 - 0xfffff
	ROM_LOAD( "varom1.rom",    0x00000, 0x20000, CRC(7e767f00) SHA1(dd4f4521bfbb068f15ab3bcdb8d47c7d82b9d1d4) )

	ROM_REGION( 0x2000, "sub", 0 )		// not sure what this should do...
	ROM_LOAD( "vasubsys.rom", 0x0000, 0x2000, CRC(08962850) SHA1(a9375aa480f85e1422a0e1385acb0ea170c5c2e0) )

	/* No idea of the proper size: it has never been dumped */
	ROM_REGION( 0x2000, "audiocpu", 0)
	ROM_LOAD( "soundbios.rom", 0x0000, 0x2000, NO_DUMP )

	ROM_REGION( 0x80000, "kanji", ROMREGION_ERASEFF )
	ROM_LOAD( "vafont.rom", 0x00000, 0x50000, BAD_DUMP CRC(b40d34e4) SHA1(a0227d1fbc2da5db4b46d8d2c7e7a9ac2d91379f) ) // should be splitted

	/* 32 banks, to be loaded at 0xc000 - 0xffff */
	ROM_REGION( 0x80000, "dictionary", 0 )
	ROM_LOAD( "vadic.rom", 0x00000, 0x80000, CRC(a6108f4d) SHA1(3665db538598abb45d9dfe636423e6728a812b12) )

	ROM_REGION( 0x10000, "tvram", ROMREGION_ERASE00 )

	ROM_REGION( 0x40000, "gvram", ROMREGION_ERASE00 )
ROM_END

COMP( 1987, pc88va,         0,		0,     pc88va,   pc88va,  0,    "Nippon Electronic Company",  "PC-88VA", GAME_NOT_WORKING | GAME_NO_SOUND)
//COMP( 1988, pc88va2,      pc88va, 0,     pc88va,   pc88va,  0,    "Nippon Electronic Company",  "PC-88VA2", GAME_NOT_WORKING )
//COMP( 1988, pc88va3,      pc88va, 0,     pc88va,   pc88va,  0,    "Nippon Electronic Company",  "PC-88VA3", GAME_NOT_WORKING )
