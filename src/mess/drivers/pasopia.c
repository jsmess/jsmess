/***************************************************************************

        Toshiba PASOPIA / PASOPIA7 emulation

        Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/z80ctc.h"
#include "machine/i8255a.h"
#include "sound/sn76496.h"
#include "video/mc6845.h"

static UINT8 vram_sel;
static UINT8 *p7_vram;

static VIDEO_START( paso7 )
{
}

static VIDEO_UPDATE( paso7 )
{
	int x,y;
	int count;

	count = 0x10;

	for(y=0;y<25;y++)
	{
		for(x=0;x<40;x++)
		{
			int tile = p7_vram[count];

			drawgfx_opaque(bitmap,cliprect,screen->machine->gfx[0],tile,0,0,0,x*8,y*8);

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
	if(!vram_sel)
		p7_vram[offset] = data;
}

static WRITE8_HANDLER( paso7_bankswitch )
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

	vram_sel = data & 4;

	// bit 3? PIO2 port C

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
static WRITE8_HANDLER( ram_bank1_w )
{
	UINT8 *cpu = memory_region(space->machine, "maincpu");

	cpu[offset] = data;
}

static WRITE8_HANDLER( ram_bank2_w )
{
	UINT8 *cpu = memory_region(space->machine, "maincpu");

	cpu[offset+0x4000] = data;
}

static ADDRESS_MAP_START(paso7_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x3fff ) AM_ROMBANK("bank1") AM_WRITE( ram_bank1_w )
	AM_RANGE( 0x4000, 0x7fff ) AM_ROMBANK("bank2") AM_WRITE( ram_bank2_w )
	AM_RANGE( 0x8000, 0xbfff ) AM_READWRITE(vram_r, vram_w ) AM_BASE(&p7_vram)
	AM_RANGE( 0xc000, 0xffff ) AM_RAMBANK("bank4")
ADDRESS_MAP_END

static ADDRESS_MAP_START( paso7_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE( 0x08, 0x0b ) AM_DEVREADWRITE("ppi8255_0", i8255a_r, i8255a_w)
	AM_RANGE( 0x0c, 0x0f ) AM_DEVREADWRITE("ppi8255_1", i8255a_r, i8255a_w)
	AM_RANGE( 0x10, 0x10 ) AM_DEVWRITE("crtc", mc6845_address_w)
	AM_RANGE( 0x11, 0x11 ) AM_DEVWRITE("crtc", mc6845_register_w)
	AM_RANGE( 0x18, 0x1b ) AM_READWRITE( pac2_r, pac2_w )
	AM_RANGE( 0x20, 0x23 ) AM_DEVREADWRITE("ppi8255_2", i8255a_r, i8255a_w)
	AM_RANGE( 0x28, 0x2b ) AM_DEVREADWRITE("ctc", z80ctc_r, z80ctc_w)
//	AM_RANGE( 0x30, 0x33 ) //I8255 related (port mirrors?)
	AM_RANGE( 0x3a, 0x3a ) AM_DEVWRITE("sn1", sn76496_w)
	AM_RANGE( 0x3b, 0x3b ) AM_DEVWRITE("sn2", sn76496_w)
	AM_RANGE( 0x3c, 0x3c ) AM_WRITE(paso7_bankswitch)
//	AM_RANGE( 0xe0, 0xe6 ) AM_READWRITE( fdc_r, fdc_w )
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( paso7 )
INPUT_PORTS_END

static MACHINE_RESET( paso7 )
{
}

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


static const z80_daisy_config p7_daisy[] =
{
	{ "ctc" },
	{ NULL }
};

static READ8_DEVICE_HANDLER( crtc_portb_r )
{
	// --x- ---- vsync bit
	// ---- x--- disp bit
	int vdisp = (device->machine->primary_screen->vpos() < 200) ? 0x08 : 0x00;

	return 0xf7 | vdisp;
}

static WRITE8_DEVICE_HANDLER( ppi8255_0a_w )
{

}

static WRITE8_DEVICE_HANDLER( ppi8255_1a_w )
{

}

static WRITE8_DEVICE_HANDLER( ppi8255_1b_w )
{

}

static WRITE8_DEVICE_HANDLER( ppi8255_1c_w )
{

}

static WRITE8_DEVICE_HANDLER( ppi8255_2a_w )
{
	/* --x- ---- (related to the data rec) */
	/* ---x ---- data rec out */
	/* ---- --x- sound off */
}


static I8255A_INTERFACE( ppi8255_intf_0 )
{
	DEVCB_NULL,						/* Port A read */
	DEVCB_HANDLER(crtc_portb_r),	/* Port B read */
	DEVCB_NULL,						/* Port C read */
	DEVCB_HANDLER(ppi8255_0a_w),		/* Port A write */
	DEVCB_NULL,						/* Port B write */
	DEVCB_NULL						/* Port C write */
};

static I8255A_INTERFACE( ppi8255_intf_1 )
{
	DEVCB_NULL,						/* Port A read */
	DEVCB_NULL,						/* Port B read */
	DEVCB_NULL,						/* Port C read */
	DEVCB_HANDLER(ppi8255_1a_w),	/* Port A write */
	DEVCB_HANDLER(ppi8255_1b_w),	/* Port B write */
	DEVCB_HANDLER(ppi8255_1c_w)		/* Port C write */
};

static I8255A_INTERFACE( ppi8255_intf_2 )
{
	DEVCB_NULL,						/* Port A read */
	DEVCB_NULL,						/* Port B read */
	DEVCB_NULL,						/* Port C read */
	DEVCB_HANDLER(ppi8255_2a_w),	/* Port A write */
	DEVCB_NULL,						/* Port B write */
	DEVCB_NULL						/* Port C write */
};

static MACHINE_DRIVER_START( paso7 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(paso7_mem)
    MDRV_CPU_IO_MAP(paso7_io)
//	MDRV_CPU_VBLANK_INT("screen", irq0_line_hold)
	MDRV_CPU_CONFIG(p7_daisy)

	MDRV_Z80CTC_ADD( "ctc", XTAL_4MHz/4 , ctc_intf )

    MDRV_MACHINE_RESET(paso7)

	MDRV_I8255A_ADD( "ppi8255_0", ppi8255_intf_0 )
	MDRV_I8255A_ADD( "ppi8255_1", ppi8255_intf_1 )
	MDRV_I8255A_ADD( "ppi8255_2", ppi8255_intf_2 )

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

	MDRV_SOUND_ADD("sn1", SN76489A, 18432000/4) // unknown clock / divider
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_ADD("sn2", SN76489A, 18432000/4) // unknown clock / divider
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
	UINT8 *bios = memory_region(machine, "maincpu");
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	memory_unmap_write(space, 0x0000, 0x7fff, 0, 0);
	memory_set_bankptr(machine, "bank1", bios + 0x10000);
	memory_set_bankptr(machine, "bank2", bios + 0x10000);
//	memory_set_bankptr(machine, "bank3", bios + 0x10000);
//	memory_set_bankptr(machine, "bank4", bios + 0x10000);
}

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 19??, pasopia7,  0,       0, 	 paso7,	paso7,   paso7,   "Toshiba",   "PASOPIA 7", GAME_NOT_WORKING )
