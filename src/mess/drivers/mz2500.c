/***************************************************************************

    Sharp MZ-2500 (c) 1985 Sharp Corporation

	preliminary driver by Angelo Salese

    memory map:

    0x00000-0x3ffff Work RAM
    0x40000-0x5ffff CG RAM (address bitswapped?)
    0x60000-0x67fff "Read modify write" area (related to the CG RAM)
    0x68000-0x6ffff IPL ROM
    0x70000-0x71fff TVRAM
    0x72000-0x73fff PCG / Kanji RAM
    0x74000-0x75fff Dictionary ROM (banked)
    0x76000-0x77fff NOP
    0x78000-0x7ffff Phone ROM

    vblank irq is rst $28?

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/z80pio.h"

/* machine stuff */
static UINT8 bank_val[8],bank_addr;
static UINT8 irq_sel,irq_vector[4];

/* video stuff*/
static UINT8 text_reg[0x100], text_reg_index;
static UINT8 text_col_size, text_font_reg;

static VIDEO_START( mz2500 )
{
}

static VIDEO_UPDATE( mz2500 )
{
	UINT8 *vram = memory_region(screen->machine, "maincpu");
	int x,y,count;
	const gfx_element *gfx = screen->machine->gfx[0];

	count = 0x70000;

	if(text_font_reg)
	{
		for (y=0;y<25;y++)
		{
			for (x=0;x<text_col_size;x++)
			{
				int tile = vram[count+0x0000] & 0xff;
				//int attr = vram[count+0x800];
				int tile_bank = vram[count+0x1000] & 0x3f;

				tile|= tile_bank << 8;

				drawgfx_opaque(bitmap,cliprect,gfx,tile,0,0,0,x*8,(y)*8);

				count++;
			}
		}
	}
	else
	{
		for (y=0;y<25;y+=2)
		{
			for (x=0;x<text_col_size;x++)
			{
				int tile = vram[count+0x0000] & 0xfe;
				//int attr = vram[count+0x800];
				int tile_bank = vram[count+0x1000] & 0x3f;

				tile|= tile_bank << 8;

				drawgfx_opaque(bitmap,cliprect,gfx,tile,0,0,0,x*8,(y)*8);
				drawgfx_opaque(bitmap,cliprect,gfx,tile+1,0,0,0,x*8,(y+1)*8);

				count++;
			}
		}
	}

    return 0;
}

static READ8_HANDLER( mz2500_bank_addr_r )
{
	return bank_addr;
}

static WRITE8_HANDLER( mz2500_bank_addr_w )
{
	bank_addr = data & 7;
}

static READ8_HANDLER( mz2500_bank_data_r )
{
	static UINT8 res;

	res = bank_val[bank_addr];

	bank_addr++;
	bank_addr&=7;

	return res;
}

static WRITE8_HANDLER( mz2500_bank_data_w )
{
	UINT8 *ROM = memory_region(space->machine, "maincpu");
	static const char *const bank_name[] = { "bank0", "bank1", "bank2", "bank3", "bank4", "bank5", "bank6", "bank7" };

	bank_val[bank_addr] = data & 0x3f;

//	printf("%s %02x\n",bank_name[bank_addr],bank_val[bank_addr]*2);

	memory_set_bankptr(space->machine, bank_name[bank_addr], &ROM[bank_val[bank_addr]*0x2000]);

	bank_addr++;
	bank_addr&=7;
}

static READ8_HANDLER( mz2500_crtc_r )
{
	static UINT8 vblank_bit, hblank_bit;

	/* TODO */
	vblank_bit^= 1;//video_screen_get_vblank(space->machine->primary_screen) ? 0 : 1;
	hblank_bit = video_screen_get_hblank(space->machine->primary_screen) ? 0 : 2;

	return vblank_bit | hblank_bit;
}

static WRITE8_HANDLER( mz2500_crtc_w )
{
	switch(offset)
	{
		case 0: text_reg_index = data; break;
		case 1:
			text_reg[text_reg_index] = data;
			//printf("[%02x]<- %02x\n",text_reg[text_reg_index],text_reg_index);
			break;
		case 2: /* CG MASK reg */ break;
		case 3:
			/* Font size reg */
			text_font_reg = data & 1;
			break;
	}
}

static WRITE8_HANDLER( mz2500_irq_sel_w )
{
	irq_sel = data;
	// FIXME: bit 0-3 are irq masks?
//	printf("%02x\n",irq_sel);
}

static WRITE8_HANDLER( mz2500_irq_data_w )
{
	if(irq_sel & 0x80)
		irq_vector[0] = data; //CRTC
	if(irq_sel & 0x40)
		irq_vector[1] = data; //i8253
	if(irq_sel & 0x20)
		irq_vector[2] = data; //printer
	if(irq_sel & 0x10)
		irq_vector[3] = data; //RP5c15
}

static ADDRESS_MAP_START(mz2500_map, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x1fff) AM_RAMBANK("bank0")
	AM_RANGE(0x2000, 0x3fff) AM_RAMBANK("bank1")
	AM_RANGE(0x4000, 0x5fff) AM_RAMBANK("bank2")
	AM_RANGE(0x6000, 0x7fff) AM_RAMBANK("bank3")
	AM_RANGE(0x8000, 0x9fff) AM_RAMBANK("bank4")
	AM_RANGE(0xa000, 0xbfff) AM_RAMBANK("bank5")
	AM_RANGE(0xc000, 0xdfff) AM_RAMBANK("bank6")
	AM_RANGE(0xe000, 0xffff) AM_RAMBANK("bank7")
ADDRESS_MAP_END

#if 0
static READ8_HANDLER( kludge_r )
{
	return mame_rand(space->machine);
}
#endif

static ADDRESS_MAP_START(mz2500_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
//	AM_RANGE(0x60, 0x63) AM_WRITE(w3100a_w)
//	AM_RANGE(0x63, 0x63) AM_READ(w3100a_r)
//	AM_RANGE(0xa0, 0xa3) AM_READWRITE(sio_r,sio_w)
//	AM_RANGE(0xa4, 0xa5) AM_READWRITE(sasi_r, sasi_w)
//	AM_RANGE(0xa8, 0xa8) AM_WRITE(rom_w)
//	AM_RANGE(0xa9, 0xa9) AM_READ(rom_r)
//	AM_RANGE(0xac, 0xad) AM_WRITE(emm_w)
//	AM_RANGE(0xad, 0xad) AM_READ(emm_r)
//	AM_RANGE(0xae, 0xae) AM_WRITE(crtc_w)
//	AM_RANGE(0xb0, 0xb3) AM_READWRITE(sio_r,sio_w)
	AM_RANGE(0xb4, 0xb4) AM_READWRITE(mz2500_bank_addr_r,mz2500_bank_addr_w)
	AM_RANGE(0xb5, 0xb5) AM_READWRITE(mz2500_bank_data_r,mz2500_bank_data_w)
// 	AM_RANGE(0xb8, 0xb9) AM_READWRITE(kanji_r,kanji_w)
//	AM_RANGE(0xbc, 0xbd) AM_READWRITE(crtc_r,crtc_w)
	AM_RANGE(0xc6, 0xc6) AM_WRITE(mz2500_irq_sel_w)
	AM_RANGE(0xc7, 0xc7) AM_WRITE(mz2500_irq_data_w)
//	AM_RANGE(0xc8, 0xc9) AM_READWRITE(opn_r,opn_w)
//	AM_RANGE(0xc8, 0xc9) AM_READ(kludge_r)
//	AM_RANGE(0xca, 0xca) AM_READWRITE(voice_r,voice_w)
//	AM_RANGE(0xcc, 0xcc) AM_READWRITE(calendar_r,calendar_w)
//	AM_RANGE(0xce, 0xcf) AM_WRITE(memory_w)
//	AM_RANGE(0xd8, 0xdb) AM_READWRITE(fdc_r,fdc_w)
//	AM_RANGE(0xdc, 0xdd) AM_WRITE(floppy_w)
	AM_RANGE(0xe0, 0xe3) AM_DEVREADWRITE("z80pio_0",z80pio_cd_ba_r,z80pio_cd_ba_w)
//	AM_RANGE(0xe4, 0xe7) AM_READWRITE(pit_r,pit_w)
	AM_RANGE(0xe8, 0xeb) AM_DEVREADWRITE("z80pio_1",z80pio_cd_ba_r,z80pio_cd_ba_w)
//	AM_RANGE(0xef, 0xef) AM_READWRITE(joystick_r,joystick_w)
//	AM_RANGE(0xf0, 0xf3) AM_WRITE(timer_w)
	AM_RANGE(0xf4, 0xf7) AM_READ(mz2500_crtc_r) AM_WRITE(mz2500_crtc_w)
//	AM_RANGE(0xf8, 0xf9) AM_READWRITE(extrom_r,extrom_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( mz2500 )
INPUT_PORTS_END


static MACHINE_RESET(mz2500)
{
	UINT8 *ROM = memory_region(machine, "maincpu");

	bank_val[0] = 0x00;
	bank_val[1] = 0x01;
	bank_val[2] = 0x02;
	bank_val[3] = 0x03;
	bank_val[4] = 0x04;
	bank_val[5] = 0x05;
	bank_val[6] = 0x06;
	bank_val[7] = 0x07;

	memory_set_bankptr(machine, "bank0", &ROM[bank_val[0]*0x2000]);
	memory_set_bankptr(machine, "bank1", &ROM[bank_val[1]*0x2000]);
	memory_set_bankptr(machine, "bank2", &ROM[bank_val[2]*0x2000]);
	memory_set_bankptr(machine, "bank3", &ROM[bank_val[3]*0x2000]);
	memory_set_bankptr(machine, "bank4", &ROM[bank_val[4]*0x2000]);
	memory_set_bankptr(machine, "bank5", &ROM[bank_val[5]*0x2000]);
	memory_set_bankptr(machine, "bank6", &ROM[bank_val[6]*0x2000]);
	memory_set_bankptr(machine, "bank7", &ROM[bank_val[7]*0x2000]);

	irq_vector[0] = 0xef; /* RST 28h - vblank */

	text_col_size = 40;
	text_font_reg = 0;
}

static const gfx_layout mz2500_cg_layout =
{
	8, 8,		/* 8 x 8 graphics */
	RGN_FRAC(1,1),		/* 512 codes */
	1,		/* 1 bit per pixel */
	{ 0 },		/* no bitplanes */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8 * 8		/* code takes 8 times 8 bits */
};

/* gfx1 is mostly 16x16, but there are some 8x8 characters */
static const gfx_layout mz2500_8_layout =
{
	8, 8,		/* 8 x 8 graphics */
	1920,		/* 1920 codes */
	1,		/* 1 bit per pixel */
	{ 0 },		/* no bitplanes */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8 * 8		/* code takes 8 times 8 bits */
};

static const gfx_layout mz2500_16_layout =
{
	16, 16,		/* 16 x 16 graphics */
	RGN_FRAC(1,1),		/* 8192 codes */
	1,		/* 1 bit per pixel */
	{ 0 },		/* no bitplanes */
	{ 0, 1, 2, 3, 4, 5, 6, 7, 128, 129, 130, 131, 132, 133, 134, 135 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16 * 16		/* code takes 16 times 16 bits */
};

static GFXDECODE_START( mz2500 )
	GFXDECODE_ENTRY("gfx1", 0, mz2500_cg_layout, 0, 256)
	GFXDECODE_ENTRY("gfx1", 0x4400, mz2500_8_layout, 0, 256)	// for viewer only
	GFXDECODE_ENTRY("gfx1", 0, mz2500_16_layout, 0, 256)		// for viewer only
GFXDECODE_END

static INTERRUPT_GEN( mz2500_vbl )
{
	cpu_set_input_line_and_vector(device, 0, HOLD_LINE, irq_vector[0]);
}

static WRITE8_DEVICE_HANDLER( mz2500_pio1_porta_w )
{
//	printf("%02x\n",data);
	text_col_size = (data & 0x20) ? 80 : 40;
}

static Z80PIO_INTERFACE( mz2500_pio0_intf )
{
	DEVCB_NULL, //irq handler
	DEVCB_NULL, //port a r
	DEVCB_NULL, //port a w
	DEVCB_NULL, //port a ardy
	DEVCB_NULL, //port b r
	DEVCB_NULL, //port b w
	DEVCB_NULL  //port b ardy
};

static Z80PIO_INTERFACE( mz2500_pio1_intf )
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER( mz2500_pio1_porta_w ),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


static MACHINE_DRIVER_START( mz2500 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu", Z80, 6000000)
    MDRV_CPU_PROGRAM_MAP(mz2500_map)
    MDRV_CPU_IO_MAP(mz2500_io)
	MDRV_CPU_VBLANK_INT("screen", mz2500_vbl)

    MDRV_MACHINE_RESET(mz2500)

	MDRV_Z80PIO_ADD( "z80pio_0", 6000000/4, mz2500_pio0_intf ) // unknown clock / divider
	MDRV_Z80PIO_ADD( "z80pio_1", 6000000/4, mz2500_pio1_intf )

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_RAW_PARAMS(XTAL_17_73447MHz/2, 568, 0, 40*8, 312, 0, 25*8)
	MDRV_PALETTE_LENGTH(256*2)
//	MDRV_PALETTE_INIT(black_and_white)

	MDRV_GFXDECODE(mz2500)

    MDRV_VIDEO_START(mz2500)
    MDRV_VIDEO_UPDATE(mz2500)
MACHINE_DRIVER_END



/* ROM definition */
ROM_START( mz2500 )
	ROM_REGION( 0x80000, "maincpu", ROMREGION_ERASE00 )
	ROM_LOAD( "ipl.rom", 0x00000, 0x8000, CRC(7a659f20) SHA1(ccb3cfdf461feea9db8d8d3a8815f7e345d274f7) )
	ROM_RELOAD( 0x68000, 0x8000 )

	/* this is probably an hand made ROM, will be removed in the end ...*/
	ROM_REGION( 0x1000, "cgrom", 0 )
	ROM_LOAD( "cg.rom", 0x0000, 0x0800, CRC(a082326f) SHA1(dfa1a797b2159838d078650801c7291fa746ad81) )

	ROM_REGION( 0x40000, "gfx1", 0 )
	ROM_LOAD( "kanji.rom", 0x0000, 0x40000, CRC(dd426767) SHA1(cc8fae0cd1736bc11c110e1c84d3f620c5e35b80) )

	ROM_REGION( 0x40000, "dictionary", 0 )
	ROM_LOAD( "dict.rom", 0x00000, 0x40000, CRC(aa957c2b) SHA1(19a5ba85055f048a84ed4e8d471aaff70fcf0374) )
ROM_END

/* Driver */

COMP( 1985, mz2500,   0,        0,      mz2500,   mz2500,        0,      "Sharp",     "MZ-2500", GAME_NOT_WORKING | GAME_NO_SOUND)
