/***************************************************************************

	ACT Apricot series

	preliminary driver by Angelo Salese

	TODO:
	- I'm not entirely convinced that Apricot Xi/PC and F1/F10 should stay
	  in the same driver, they looks to have uncompatible devices and
	  probably uncompatible software too.

****************************************************************************/

#include "driver.h"
#include "cpu/i86/i86.h"
#include "video/mc6845.h"
#include "machine/8255ppi.h"
#include "machine/wd17xx.h"
#include "devices/flopdrv.h"
#include "cpu/z80/z80daisy.h"
#include "machine/z80ctc.h"
#include "machine/pit8253.h"
#include "machine/pic8259.h"

#include "devices/flopdrv.h"
#include "formats/basicdsk.h"

static UINT16 *act_vram,*act_scrollram,*act_ram;
static struct
{
	UINT8 disp_en; 		//bit 3
	UINT8 gfx_mode; 	//bit 4
	UINT8 fdrv_num;		//bit 6
	UINT8 p_input_dir;	//bit 7
}xi_sys_ctrl;

static VIDEO_START( act_f1 )
{
}

static VIDEO_UPDATE( act_f1 )
{
	int x,y,i;
	int x_count;

	{
		for(y=0;y<256;y++)
		{
			x_count = 0;
			for(x=0;x<640;x+=8)
			{
				int pen[2],color;

				for (i=0;i<8;i++)
				{
					pen[0] = (act_vram[act_scrollram[y]+x_count])>>(7-i) & 1;
					pen[1] = (act_vram[act_scrollram[y]+x_count])>>(15-i) & 1;

					color = pen[0]|pen[1]<<1;

					if((x+i)<=video_screen_get_visible_area(screen)->max_x && ((y)+0)<video_screen_get_visible_area(screen)->max_y)
						*BITMAP_ADDR16(bitmap, y, x+i) = screen->machine->pens[color];
				}

				x_count++;
			}
		}
	}

	return 0;
}

static VIDEO_START( act_xi )
{
}

static VIDEO_UPDATE( act_xi )
{
	int y,x,xi,yi;

	if(xi_sys_ctrl.disp_en)
		return 0;

	if(xi_sys_ctrl.gfx_mode)
	{
		for (y=0;y<25;y++)
		{
			for (x=0;x<80;x++)
			{
				int tile = act_vram[(x+y*80)] & 0x03ff;

				//0x8000: reverse
				//0x4000: bright
				//0x2000: underline
				//0x1000: strikethrough

				for(yi=0;yi<16;yi++)
				{
					for(xi=0;xi<10;xi++)
					{
						int color;

						color = (act_ram[(tile*16)+(yi)]>>(xi) & 1) ? 7 : 0;

						//FIXME: X should actually be multiplied by 10.
						if((x*8+xi)<=video_screen_get_visible_area(screen)->max_x && ((y*16+yi))<video_screen_get_visible_area(screen)->max_y)
							*BITMAP_ADDR16(bitmap, y*16+yi, x*8+xi) = screen->machine->pens[color];
					}
				}
			}
		}
	}
	else
	{
		//...
	}

	return 0;
}

static READ8_HANDLER( act_sio_r )
{
	return mame_rand(space->machine);
}

static WRITE8_HANDLER( act_sio_w )
{
//	if(data)
//	printf("Write to 66 %c\n",data);
}

//static UINT8 fdc_irq_flag;
//static UINT8 fdc_drq_flag;
//static UINT8 fdc_side;
//static UINT8 fdc_drive;

static READ8_HANDLER( act_fdc_r )
{
	const device_config* dev = devtag_get_device(space->machine,"fdc");

//	printf("%02x\n",offset);

	floppy_drive_set_motor_state(floppy_get_device(space->machine, xi_sys_ctrl.fdrv_num), 1);
	floppy_drive_set_ready_state(floppy_get_device(space->machine, xi_sys_ctrl.fdrv_num), 1,0);

	switch(offset)
	{
		case 0:
			return wd17xx_status_r(dev,offset);
		case 1:
			return wd17xx_track_r(dev,offset);
		case 2:
			return wd17xx_sector_r(dev,offset);
		case 3:
			return wd17xx_data_r(dev,offset);
		default:
			logerror("FDC: read from %04x\n",offset);
			return 0xff;
	}

	return 0x00;
}

static WRITE8_HANDLER( act_fdc_w )
{
	const device_config* dev = devtag_get_device(space->machine,"fdc");

//	printf("%02x %02x\n",offset,data);

	floppy_drive_set_motor_state(floppy_get_device(space->machine, xi_sys_ctrl.fdrv_num), 1);
	floppy_drive_set_ready_state(floppy_get_device(space->machine, xi_sys_ctrl.fdrv_num), 1,0);

	switch(offset)
	{
		case 0:
			wd17xx_command_w(dev,offset,data);
			break;
		case 1:
			wd17xx_track_w(dev,offset,data);
			break;
		case 2:
			wd17xx_sector_w(dev,offset,data);
			break;
		case 3:
			wd17xx_data_w(dev,offset,data);
			break;
		default:
			logerror("FDC: write to %04x = %02x\n",offset,data);
			break;
	}
}

static const wd17xx_interface act_wd2797_interface =
{
	DEVCB_NULL,
	DEVCB_NULL,
	{FLOPPY_0, FLOPPY_1 }
};

static READ16_HANDLER( act_pal_r )
{
	return paletteram16[offset];
}

static WRITE16_HANDLER( act_pal_w )
{
	UINT8 i,r,g,b;
	COMBINE_DATA(&paletteram16[offset]);

	if(ACCESSING_BITS_0_7 && offset) //TODO: offset 0 looks bogus
	{
		i = paletteram16[offset] & 1;
		r = ((paletteram16[offset] & 2)>>0) | i;
		g = ((paletteram16[offset] & 4)>>1) | i;
		b = ((paletteram16[offset] & 8)>>2) | i;

		palette_set_color_rgb(space->machine, offset, pal2bit(r), pal2bit(g), pal2bit(b));
	}
}

static ADDRESS_MAP_START(act_f1_mem, ADDRESS_SPACE_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x01e00,0x01fff) AM_RAM AM_BASE(&act_scrollram)
	AM_RANGE(0xe0000,0xe001f) AM_READWRITE(act_pal_r,act_pal_w) AM_BASE(&paletteram16)
	AM_RANGE(0x00000,0xeffff) AM_RAM AM_BASE(&act_vram)
	AM_RANGE(0xf0000,0xf7fff) AM_RAM
	AM_RANGE(0xf8000,0xfffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START(act_xi_mem, ADDRESS_SPACE_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000,0xeffff) AM_RAM AM_BASE(&act_ram)
	AM_RANGE(0xf0000,0xf0fff) AM_MIRROR(0x7000) AM_RAM AM_BASE(&act_vram)
	AM_RANGE(0xf8000,0xfffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( act_xi_io , ADDRESS_SPACE_IO, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x0003) AM_DEVREADWRITE8("pic8259_master",pic8259_r, pic8259_w,0x00ff)
	AM_RANGE(0x0040, 0x0047) AM_READWRITE8(act_fdc_r, act_fdc_w,0x00ff)
	AM_RANGE(0x0048, 0x004f) AM_DEVREADWRITE8("ppi8255_0", ppi8255_r, ppi8255_w,0x00ff)
//	AM_RANGE(0x0050, 0x0051) sound gen
	AM_RANGE(0x0058, 0x005f) AM_DEVREADWRITE8("pit8253",pit8253_r,pit8253_w,0x00ff)
	AM_RANGE(0x0060, 0x0067) AM_READWRITE8(act_sio_r,act_sio_w,0x00ff)
	AM_RANGE(0x0068, 0x0069) AM_DEVWRITE8("crtc", mc6845_address_w,0x00ff)
	AM_RANGE(0x006a, 0x006b) AM_DEVREADWRITE8("crtc", mc6845_register_r,mc6845_register_w,0x00ff)
//	AM_RANGE(0x0070, 0x0073) 8089
ADDRESS_MAP_END


static WRITE8_HANDLER( actf1_sys_w )
{
//	static UINT8 cur_fdrv;
//	const device_config* dev = devtag_get_device(space->machine,"fdc");

	switch(offset)
	{
		case 0:
//			cur_fdrv = ~data & 1;
//			wd17xx_set_drive(dev,cur_fdrv);
			break;
		case 1:
//			wd17xx_set_side(dev,data ? 1 : 0);
			break;
		case 2:
//			floppy_drive_set_motor_state(floppy_get_device(space->machine, cur_fdrv), data);
//			floppy_drive_set_ready_state(floppy_get_device(space->machine, cur_fdrv), data,0);
			break;
		case 3:
//			data ? 256 : 200 line mode
//			data ? 50 : 60 Hz
			break;
		case 4:
//			data ? 80 : 40 columns mode
//			data ? 14 Mhz : 7 Mhz Pixel clock
			break;
		case 5:
//			caps lock LED
			break;
		case 6:
//			stop LED
			break;
	}
}

static ADDRESS_MAP_START( act_f1_io , ADDRESS_SPACE_IO, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x000f) AM_WRITE8(actf1_sys_w,0x00ff)
	AM_RANGE(0x0010, 0x0017) AM_DEVREADWRITE8("ctc",z80ctc_r,z80ctc_w,0x00ff)
//	AM_RANGE(0x0020, 0x0027) z80 sio (!)
//	AM_RANGE(0x0030, 0x0031) AM_WRITE8(ctc_ack_w,0x00ff)
	AM_RANGE(0x0040, 0x0047) AM_READWRITE8(act_fdc_r, act_fdc_w,0x00ff)
//	AM_RANGE(0x01e0, 0x01ff) winchester
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( act )
INPUT_PORTS_END


static MACHINE_RESET(act)
{
}


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

static READ8_DEVICE_HANDLER( act_portb_r )
{
	return 0xff;
}

/*
	x--- ---- Parallel Input Direction
	-x-- ---- FDC Drive select
	---x ---- Text/GFX Mode
	---- x--- Display Enable
	---- -x-- FDC Side load
	---- ---x CRTC reset
*/
static WRITE8_DEVICE_HANDLER( act_portb_w )
{
	const device_config* dev = devtag_get_device(device->machine,"fdc");

//	printf("PPI Port B write %02x\n",data);
	xi_sys_ctrl.disp_en = (data & 8)>>3;
	xi_sys_ctrl.gfx_mode = (data & 0x10)>>4;
	xi_sys_ctrl.p_input_dir = (data & 0x80)>>7;
	xi_sys_ctrl.fdrv_num = (data & 0x40)>>6;

	wd17xx_set_drive(dev,xi_sys_ctrl.fdrv_num);
	wd17xx_set_side(dev,(data & 0x04)>>2);
}

static const ppi8255_interface ppi8255_intf =
{
	DEVCB_NULL,									/* Port A read */
	DEVCB_HANDLER(act_portb_r),					/* Port B read */
	DEVCB_NULL,									/* Port C read */
	DEVCB_NULL,									/* Port A write */
	DEVCB_HANDLER(act_portb_w),					/* Port B write */
	DEVCB_NULL									/* Port C write */
};

static const gfx_layout charset_8x8 =
{
	8,8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_layout charset_16x16 =
{
	10,16,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 7,6,5,4,3,2,1,0 , 15,14 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,8*16,9*16,10*16,11*16,12*16,13*16,14*16,15*16 },
	16*16
};

static GFXDECODE_START( act_f1 )
	GFXDECODE_ENTRY( "gfx",   0x00000, charset_8x8,    0x000, 1 )
GFXDECODE_END

static GFXDECODE_START( act_xi )
	GFXDECODE_ENTRY( "gfx",   0x00000, charset_16x16,    0x000, 1 )
GFXDECODE_END

static Z80CTC_INTERFACE( ctc_intf )
{
	0,					// timer disables
	DEVCB_NULL,		// interrupt handler
	DEVCB_NULL,		// ZC/TO0 callback
	DEVCB_NULL,		// ZC/TO1 callback
	DEVCB_NULL,		// ZC/TO2 callback
};

#if 0

static const z80sio_interface sio_intf =
{
	0,					/* interrupt handler */
	0,					/* DTR changed handler */
	0,					/* RTS changed handler */
	0,					/* BREAK changed handler */
	0,					/* transmit handler */
	0					/* receive handler */
};
#endif

#if 0
static const z80_daisy_chain x1_daisy[] =
{
	{ "ctc" },
	{ NULL }
};
#endif

static INTERRUPT_GEN( act_f1_irq )
{
	//if(input_code_pressed(device->machine, KEYCODE_C))
	//	cpu_set_input_line_and_vector(device,0,HOLD_LINE,0x60);
}

static PIC8259_SET_INT_LINE( pc98_master_set_int_line ) {
	//printf("%02x\n",interrupt);
	cputag_set_input_line(device->machine, "maincpu", 0, interrupt ? HOLD_LINE : CLEAR_LINE);
}


static const struct pic8259_interface pic8259_master_config = {
	pc98_master_set_int_line
};

static PIT8253_OUTPUT_CHANGED( pc_timer0_w )
{
	pic8259_set_irq_line(devtag_get_device( device->machine, "pic8259_master" ), 0, state);
}

static const struct pit8253_config pit8253_config =
{
	{
		{
			4670000,				/* heartbeat IRQ */
			pc_timer0_w
		}, {
			4670000,				/* dram refresh */
			NULL
		}, {
			4670000,				/* pio port c pin 4, and speaker polling enough */
			NULL
		}
	}
};

static FLOPPY_OPTIONS_START( act )
	FLOPPY_OPTION( img2hd, "dsk", "2HD disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([80])
		SECTORS([16])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

static const floppy_config act_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(act),
	DO_NOT_KEEP_GEOMETRY
};

static MACHINE_DRIVER_START( act_f1 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", I8086, 4670000)
	MDRV_CPU_PROGRAM_MAP(act_f1_mem)
	MDRV_CPU_IO_MAP(act_f1_io)
	MDRV_CPU_VBLANK_INT("screen",act_f1_irq )
//	MDRV_CPU_CONFIG(x1_daisy)

	MDRV_Z80CTC_ADD( "ctc", 4670000 , ctc_intf )

	MDRV_MACHINE_RESET(act)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 256)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 256-1)
	MDRV_PALETTE_LENGTH(16)
//	MDRV_PALETTE_INIT(black_and_white)

//	MDRV_MC6845_ADD("crtc", MC6845, XTAL_3_579545MHz/2, mc6845_intf)	/* hand tuned to get ~50 fps */

//	MDRV_PPI8255_ADD("ppi8255_0", ppi8255_intf )
	MDRV_WD2793_ADD("fdc", act_wd2797_interface )

	MDRV_GFXDECODE(act_f1)

	MDRV_VIDEO_START(act_f1)
	MDRV_VIDEO_UPDATE(act_f1)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( act_xi )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", I8086, 4670000)
	MDRV_CPU_PROGRAM_MAP(act_xi_mem)
	MDRV_CPU_IO_MAP(act_xi_io)

	MDRV_MACHINE_RESET(act)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 375)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 375-1)
	MDRV_PALETTE_LENGTH(16)
//	MDRV_PALETTE_INIT(black_and_white)

	MDRV_MC6845_ADD("crtc", MC6845, XTAL_3_579545MHz/2, mc6845_intf)	/* hand tuned to get ~50 fps */

	MDRV_PPI8255_ADD("ppi8255_0", ppi8255_intf )
	MDRV_WD2793_ADD("fdc", act_wd2797_interface )
	MDRV_PIC8259_ADD( "pic8259_master", pic8259_master_config )
	MDRV_PIT8253_ADD( "pit8253", pit8253_config )

	MDRV_GFXDECODE(act_xi)

	MDRV_VIDEO_START(act_xi)
	MDRV_VIDEO_UPDATE(act_xi)

	MDRV_FLOPPY_2_DRIVES_ADD(act_floppy_config)
MACHINE_DRIVER_END

/* ROM definition */
// not sure the ROMs are loaded correctly
ROM_START( aprixi )
	ROM_REGION( 0x100000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "lo_ve007.u11", 0xfc000, 0x2000, CRC(e74e14d1) SHA1(569133b0266ce3563b21ae36fa5727308797deee) )	// Labelled LO Ve007 03.04.84
	ROM_RELOAD(						 0xf8000, 0x2000 )
	ROM_LOAD16_BYTE( "hi_ve007.u9",  0xfc001, 0x2000, CRC(b04fb83e) SHA1(cc2b2392f1b4c04bb6ec8ee26f8122242c02e572) )	// Labelled HI Ve007 03.04.84
	ROM_RELOAD(						 0xf8001, 0x2000 )

	ROM_REGION( 0x08000, "gfx", ROMREGION_ERASEFF )
//	ROM_COPY( "maincpu", 0xf8800, 0x00000, 0x01000 )
ROM_END

ROM_START( aprif1 )
	ROM_REGION( 0x100000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "lo_f1_1.6.8f",  0xf8000, 0x4000, CRC(be018be2) SHA1(80b97f5b2111daf112c69b3f58d1541a4ba69da0) )	// Labelled F1 - LO Vr. 1.6
	ROM_LOAD16_BYTE( "hi_f1_1.6.10f", 0xf8001, 0x4000, CRC(bbba77e2) SHA1(e62bed409eb3198f4848f85fccd171cd0745c7c0) )	// Labelled F1 - HI Vr. 1.6

	ROM_REGION( 0x00800, "gfx", ROMREGION_ERASEFF )
	ROM_COPY( "maincpu", 0xf8800, 0x00000, 0x00800 )
ROM_END

ROM_START( aprif10 )
	ROM_REGION( 0x100000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "lo_f10_3.1.1.8f",  0xf8000, 0x4000, CRC(bfd46ada) SHA1(0a36ef379fa9af7af9744b40c167ce6e12093485) )	// Labelled LO-FRange Vr3.1.1
	ROM_LOAD16_BYTE( "hi_f10_3.1.1.10f", 0xf8001, 0x4000, CRC(67ad5b3a) SHA1(a5ececb87476a30167cf2a4eb35c03aeb6766601) )	// Labelled HI-FRange Vr3.1.1

	ROM_REGION( 0x00800, "gfx", ROMREGION_ERASEFF )
	ROM_COPY( "maincpu", 0xf8800, 0x00000, 0x00800 )
ROM_END

ROM_START( aprifp )
	ROM_REGION( 0x100000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "lo_fp_3.1.ic20", 0xf8000, 0x4000, CRC(0572add2) SHA1(c7ab0e5ced477802e37f9232b5673f276b8f5623) )	// Labelled 11212721 F97E PORT LO VR 3.1
	ROM_LOAD16_BYTE( "hi_fp_3.1.ic9",  0xf8001, 0x4000, CRC(3903674b) SHA1(8418682dcc0c52416d7d851760fea44a3cf2f914) )	// Labelled 11212721 BD2D PORT HI VR 3.1

	ROM_REGION( 0x00800, "gfx", ROMREGION_ERASEFF )
	ROM_COPY( "maincpu", 0xf8800, 0x00000, 0x00800 )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT MACHINE INPUT   INIT  CONFIG COMPANY  FULLNAME                 FLAGS */
COMP( 1984, aprixi,    0,    0,     act_xi,    act,    0,    0,     "ACT",   "Apricot Xi",            GAME_NOT_WORKING)
COMP( 1984, aprif1,    0,    0,     act_f1,    act,    0,    0,     "ACT",   "Apricot F1",            GAME_NOT_WORKING)
COMP( 1985, aprif10,   0,    0,     act_f1,    act,    0,    0,     "ACT",   "Apricot F10",           GAME_NOT_WORKING)
COMP( 1984, aprifp,    0,    0,     act_f1,    act,    0,    0,     "ACT",   "Apricot Portable / FP", GAME_NOT_WORKING)
