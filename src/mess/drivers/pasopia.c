/***************************************************************************

	Toshiba Pasopia 7 (c) 1983 Toshiba

    preliminary driver by Angelo Salese

	TODO:
	- floppy support (but floppy images are unobtainable at current time)

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/z80ctc.h"
#include "machine/i8255a.h"
#include "machine/z80pio.h"
#include "sound/sn76496.h"
#include "video/mc6845.h"

static UINT8 vram_sel,mio_sel;
static UINT8 *p7_vram;
static UINT8 bank_reg;
static UINT16 cursor_addr;
static UINT8 cursor_blink;

static VIDEO_START( paso7 )
{
}

static VIDEO_UPDATE( paso7 )
{
	int x,y;
	int count;

	count = 0;

	for(y=0;y<25;y++)
	{
		for(x=0;x<40;x++)
		{
			int tile = p7_vram[count];

			drawgfx_opaque(bitmap,cliprect,screen->machine->gfx[0],tile,0,0,0,x*8,y*8);

			if(((cursor_addr*8) == count) && !cursor_blink) // draw cursor
				drawgfx_opaque(bitmap,cliprect,screen->machine->gfx[0],0x5f,0,0,0,x*8,y*8);

			count+=8;
		}
	}

    return 0;
}

static READ8_HANDLER( vram_r )
{
	if(vram_sel)
		return 0xff;

	return p7_vram[offset];
}

static WRITE8_HANDLER( vram_w )
{
	//if(!vram_sel)  //TODO: investigate on this
		p7_vram[offset] = data;
}

static WRITE8_HANDLER( pasopia7_memory_ctrl_w )
{
	UINT8 *cpu = memory_region(space->machine, "maincpu");
	UINT8 *basic = memory_region(space->machine, "basic");

	switch(data & 3)
	{
		case 0:
		case 3: //select Basic ROM
			memory_set_bankptr(space->machine, "bank1", basic + 0x00000);
			memory_set_bankptr(space->machine, "bank2", basic + 0x04000);
			break;
		case 1: //select Basic ROM + BIOS ROM
			memory_set_bankptr(space->machine, "bank1", basic + 0x00000);
			memory_set_bankptr(space->machine, "bank2", cpu   + 0x10000);
			break;
		case 2: //select Work RAM
			memory_set_bankptr(space->machine, "bank1", cpu   + 0x00000);
			memory_set_bankptr(space->machine, "bank2", cpu   + 0x04000);
			break;
	}

	bank_reg = data & 3;
	vram_sel = data & 4;
	mio_sel = data & 8;

	// bank4 is always RAM

//	printf("%02x\n",data);
}

#if 0
static READ8_HANDLER( fdc_r )
{
	return mame_rand(space->machine);
}
#endif

static UINT16 pac2_index[8];
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


	switch(offset)
	{
		case 0: pac2_index[pac2_bank_select] = (pac2_index[pac2_bank_select] & 0x7f00) | (data & 0xff); break;
		case 1: pac2_index[pac2_bank_select] = (pac2_index[pac2_bank_select] & 0xff) | ((data & 0x7f) << 8); break;
		case 2: // RAM write
			printf("Warning: write to RAM packs\n");
			break;
		case 3:
		{
			if(data & 0x80)
			{
				// ...
			}
			else
				pac2_bank_select = data & 7;
		}
		break;
	}
}

static READ8_HANDLER( pac2_r )
{
	UINT8 *pac2_ram = memory_region(space->machine, "rampac1");

	if(offset == 2)
		if(pac2_bank_select == 4)
			return pac2_ram[pac2_index[4]];

	return 0xff;
}

/* writes always occurs to the RAM banks, even if the ROMs are selected. */
static WRITE8_HANDLER( ram_bank_w )
{
	UINT8 *cpu = memory_region(space->machine, "maincpu");

	cpu[offset] = data;
}

static WRITE8_HANDLER( pasopia7_6845_w )
{
	static int addr_latch;

	if(offset == 0)
	{
		addr_latch = data;
		mc6845_address_w(devtag_get_device(space->machine, "crtc"), 0,data);
	}
	else
	{
		/* FIXME: this should be inside the MC6845 core! */
		if(addr_latch == 0x0e)
			cursor_addr = ((data<<8) & 0x3f00) | (cursor_addr & 0xff);
		else if(addr_latch == 0x0f)
			cursor_addr = (cursor_addr & 0x3f00) | (data & 0xff);

		mc6845_register_w(devtag_get_device(space->machine, "crtc"), 0,data);
	}
}

static READ8_HANDLER( pasopia7_io_r )
{
	static UINT16 io_port;

	if(mio_sel)
	{
		const address_space *ram_space = cputag_get_address_space(space->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
		mio_sel = 0;
		//return 0x0d; // hack: this is used for reading the keyboard data, we can fake it a little ... (modify fda4)
		return memory_read_byte(ram_space, offset);
	}

	io_port = offset & 0xff; //trim down to 8-bit bus

	if(io_port >= 0x08 && io_port <= 0x0b) 		{ return i8255a_r(devtag_get_device(space->machine, "ppi8255_0"), (io_port-0x08) & 3); }
	else if(io_port >= 0x0c && io_port <= 0x0f) { return i8255a_r(devtag_get_device(space->machine, "ppi8255_1"), (io_port-0x0c) & 3); }
//	else if(io_port == 0x10 || io_port == 0x11) { M6845 read }
	else if(io_port >= 0x18 && io_port <= 0x1b) { return pac2_r(space, io_port-0x1b);  }
	else if(io_port >= 0x20 && io_port <= 0x23) { return i8255a_r(devtag_get_device(space->machine, "ppi8255_2"), (io_port-0x20) & 3); }
	else if(io_port >= 0x28 && io_port <= 0x2b) { return z80ctc_r(devtag_get_device(space->machine, "ctc"), io_port-0x28);  }
	else if(io_port >= 0x30 && io_port <= 0x33) { return z80pio_cd_ba_r(devtag_get_device(space->machine, "z80pio_0"), (io_port-0x30) & 3); }
//	else if(io_port == 0x3a) 				  	{ SN1 }
//	else if(io_port == 0x3b) 				  	{ SN2 }
//	else if(io_port == 0x3c) 				  	{ bankswitch }
//	else if(io_port >= 0xe0 && io_port <= 0xe6) { fdc }
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
		const address_space *ram_space = cputag_get_address_space(space->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
		mio_sel = 0;
		memory_write_byte(ram_space, offset, data);
		return;
	}

	io_port = offset & 0xff; //trim down to 8-bit bus

	if(io_port >= 0x08 && io_port <= 0x0b) 		{ i8255a_w(devtag_get_device(space->machine, "ppi8255_0"), (io_port-0x08) & 3, data); }
	else if(io_port >= 0x0c && io_port <= 0x0f) { i8255a_w(devtag_get_device(space->machine, "ppi8255_1"), (io_port-0x0c) & 3, data); }
	else if(io_port >= 0x10 && io_port <= 0x11) { pasopia7_6845_w(space, io_port-0x10, data); }
	else if(io_port >= 0x18 && io_port <= 0x1b) { pac2_w(space, io_port-0x1b, data);  }
	else if(io_port >= 0x20 && io_port <= 0x23) { i8255a_w(devtag_get_device(space->machine, "ppi8255_2"), (io_port-0x20) & 3, data); }
	else if(io_port >= 0x28 && io_port <= 0x2b) { z80ctc_w(devtag_get_device(space->machine, "ctc"), io_port-0x28,data);  }
	else if(io_port >= 0x30 && io_port <= 0x33) { z80pio_cd_ba_w(devtag_get_device(space->machine, "z80pio_0"), (io_port-0x30) & 3, data); }
	else if(io_port == 0x3a) 				  	{ sn76496_w(devtag_get_device(space->machine, "sn1"), 0, data); }
	else if(io_port == 0x3b) 				  	{ sn76496_w(devtag_get_device(space->machine, "sn2"), 0, data); }
	else if(io_port == 0x3c) 				  	{ pasopia7_memory_ctrl_w(space,0, data); }
//	else if(io_port >= 0xe0 && io_port <= 0xe6) { fdc }
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
	AM_RANGE( 0x8000, 0xbfff ) AM_READWRITE(vram_r, vram_w ) AM_BASE(&p7_vram)
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
	GFXDECODE_ENTRY( "font",   0x00000, p7_chars_8x8,    0, 1 )
	GFXDECODE_ENTRY( "kanji",  0x00000, p7_chars_16x16,  0, 1 )
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
	DEVCB_NULL,						// ZC/TO1 callback, beep interface
	DEVCB_LINE(z80ctc_trg3_w),		// ZC/TO2 callback
};

static Z80PIO_INTERFACE( z80pio_intf )
{
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0),
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
	// ---- x--- disp bit
	int lcd_bit = input_port_read(device->machine, "DSW") & 1;
	int vdisp = (device->machine->primary_screen->vpos() < (lcd_bit ? 200 : 28)) ? 0x08 : 0x00; //TODO: check LCD vpos trigger

	return 0xe7 | vdisp | (lcd_bit << 4);
}

static WRITE8_DEVICE_HANDLER( screen_mode_w )
{
	printf("GFX MODE %02x\n",data);
}

static WRITE8_DEVICE_HANDLER( plane_reg_w )
{
	printf("PLANE %02x\n",data);
}

static WRITE8_DEVICE_HANDLER( video_attr_w )
{
	printf("VIDEO ATTR %02x | TEXT_PAGE %02x\n",data & 0xf,data & 0x70);
}

static WRITE8_DEVICE_HANDLER( video_misc_w )
{
	/*
		--x- ---- blinking
		---x ---- attribute wrap
		---- x--- pal disable
		---- xx-- palette selector (both bits enables this, odd hook-up)
	*/
//	printf("VIDEO MISC REG %02x\n",data);
	cursor_blink = data & 0x20;

}

static WRITE8_DEVICE_HANDLER( nmi_mask_w )
{
	/*
	--x- ---- (related to the data rec)
	---x ---- data rec out
	---- --x- sound off
	---- ---x reset NMI (writes to port B = clears bits 1 & 2) (???)
	*/
	printf("SYSTEM MISC %02x\n",data);

	if(data & 1)
		i8255a_w(devtag_get_device(device->machine, "ppi8255_2"), 1, 0);

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
//	memory_set_bankptr(machine, "bank3", bios + 0x10000);
//	memory_set_bankptr(machine, "bank4", bios + 0x10000);
}

static MACHINE_DRIVER_START( paso7 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(paso7_mem)
    MDRV_CPU_IO_MAP(paso7_io)
//	MDRV_CPU_VBLANK_INT("screen", irq0_line_hold)
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
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MDRV_MC6845_ADD("crtc", H46505, XTAL_3_579545MHz/4, mc6845_intf)	/* unknown clock, hand tuned to get ~60 fps */
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

	MDRV_GFXDECODE( pasopia7 )

    MDRV_VIDEO_START(paso7)
    MDRV_VIDEO_UPDATE(paso7)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD("sn1", SN76489A, 1996800) // unknown clock / divider
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_ADD("sn2", SN76489A, 1996800) // unknown clock / divider
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

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

	ROM_REGION( 0x8000, "rampac1", ROMREGION_ERASE00 )
//	ROM_LOAD( "rampac1.bin", 0x0000, 0x8000, CRC(0e4f09bd) SHA1(4088906d57e4f6085a75b249a6139a0e2eb531a1) )

	ROM_REGION( 0x8000, "rampac2", ROMREGION_ERASE00 )
//	ROM_LOAD( "rampac2.bin", 0x0000, 0x8000, CRC(0e4f09bd) SHA1(4088906d57e4f6085a75b249a6139a0e2eb531a1) )
ROM_END

static DRIVER_INIT( paso7 )
{
}

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 19??, pasopia7,  0,       0, 	 paso7,	paso7,   paso7,   "Toshiba",   "PASOPIA 7", GAME_NOT_WORKING )
