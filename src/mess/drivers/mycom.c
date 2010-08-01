/***************************************************************************

        Japan Electronics College MYCOMZ-80A

        21/07/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "video/mc6845.h"
#include "machine/i8255a.h"

static UINT16 mycom_amask;
static UINT8 ram_bank;
static UINT16 vram_addr;
static UINT16 cursor_addr,cursor_raster;

static VIDEO_START( mycom )
{
}

static VIDEO_UPDATE( mycom )
{
	int x,y,count;
	int xi,yi;
	static UINT8 *vram = memory_region(screen->machine, "vram");
	static UINT8 *gfx_rom = memory_region(screen->machine, "gfx");

	count = 0;

	for(y=0;y<24;y++)
	{
		for(x=0;x<40;x++)
		{
			int tile = vram[count];
			//int color = (display_reg & 0x80) ? 7 : (attr & 0x07);

			for(yi=0;yi<8;yi++)
			{
				for(xi=0;xi<8;xi++)
				{
					int pen;

					pen = (gfx_rom[tile*8+yi] >> (7-xi) & 1) ? 1 : 0;

					//if(pen)
						*BITMAP_ADDR16(bitmap, y*8+yi, x*8+xi) = screen->machine->pens[pen];
				}
			}

			if(cursor_addr == count)
			{
				int xc,yc,cursor_on;

				cursor_on = 0;
				switch(cursor_raster & 0x60)
				{
					case 0x00: cursor_on = 1; break; //always on
					case 0x20: cursor_on = 0; break; //always off
					case 0x40: if(screen->machine->primary_screen->frame_number() & 0x10) { cursor_on = 1; } break; //fast blink
					case 0x60: if(screen->machine->primary_screen->frame_number() & 0x20) { cursor_on = 1; } break; //slow blink
				}

				if(cursor_on)
				{
					for(yc=0;yc<(8-(cursor_raster & 7));yc++)
					{
						for(xc=0;xc<8;xc++)
						{
							*BITMAP_ADDR16(bitmap, y*8+yc, x*8+xc) = screen->machine->pens[0x1];
						}
					}
				}
			}

			count++;
		}
	}

    return 0;
}

static READ8_HANDLER( wram_r )
{
	static UINT8 *ram = memory_region(space->machine, "maincpu");

	if(offset & 0xc000 && ram_bank)
		return ram[(offset & 0x3fff) | 0x10000];

	return ram[offset | mycom_amask];
}

static WRITE8_HANDLER( wram_w )
{
	static UINT8 *ram = memory_region(space->machine, "maincpu");

	ram[offset | mycom_amask] = data;
}

static WRITE8_HANDLER( sys_control_w )
{
	switch(data)
	{
		case 0x00: mycom_amask = 0xc000; break;
		case 0x01: mycom_amask = 0x0000; break;
		case 0x02: ram_bank = 0; break;
		case 0x03: ram_bank = 1; break;
	}
}

static WRITE8_HANDLER( mycom_6845_w )
{
	static int addr_latch;

	if(offset == 0)
	{
		addr_latch = data;
		mc6845_address_w(space->machine->device("crtc"), 0,data);
	}
	else
	{
		if(addr_latch == 0x0a)
			cursor_raster = data;
		if(addr_latch == 0x0e)
			cursor_addr = ((data<<8) & 0x3f00) | (cursor_addr & 0xff);
		else if(addr_latch == 0x0f)
			cursor_addr = (cursor_addr & 0x3f00) | (data & 0xff);

		mc6845_register_w(space->machine->device("crtc"), 0,data);
	}
}

static READ8_HANDLER( vram_data_r )
{
	static UINT8 *vram = memory_region(space->machine,"vram");

	return vram[vram_addr];
}

static WRITE8_HANDLER( vram_data_w )
{
	static UINT8 *vram = memory_region(space->machine,"vram");

	vram[vram_addr] = data;
}

static ADDRESS_MAP_START(mycom_map, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xffff) AM_READWRITE(wram_r,wram_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(mycom_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_WRITE(sys_control_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(vram_data_r,vram_data_w)
	AM_RANGE(0x02, 0x03) AM_WRITE(mycom_6845_w)
	AM_RANGE(0x03, 0x03) AM_DEVREAD("crtc", mc6845_register_r)
	AM_RANGE(0x04, 0x07) AM_DEVREADWRITE("ppi8255_0", i8255a_r, i8255a_w)
	AM_RANGE(0x08, 0x0b) AM_DEVREADWRITE("ppi8255_1", i8255a_r, i8255a_w)
	AM_RANGE(0x0c, 0x0f) AM_DEVREADWRITE("ppi8255_2", i8255a_r, i8255a_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( mycom )
INPUT_PORTS_END


static MACHINE_RESET(mycom)
{
	mycom_amask = 0xc000;
}


/* F4 Character Displayer */
static const gfx_layout mycom_charlayout =
{
	8, 8,					/* 8 x 8 characters */
	256,					/* 256 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8					/* every char takes 8 bytes */
};

static GFXDECODE_START( mycom )
	GFXDECODE_ENTRY( "gfx", 0x0000, mycom_charlayout, 0, 1 )
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

static WRITE8_DEVICE_HANDLER( vram_laddr_w )
{
	vram_addr = (vram_addr & 0x700) | (data & 0xff);
}

static WRITE8_DEVICE_HANDLER( vram_haddr_w )
{
	vram_addr = (vram_addr & 0xff) | ((data & 0x07) << 8);
}

static READ8_DEVICE_HANDLER( input_1a )
{
	/*
	---- --x- keyboard shift
	---- ---x keyboard strobe
	*/

	if(input_code_pressed(device->machine,KEYCODE_Z))
		return 0x1;

	return 0x00;
}

static READ8_DEVICE_HANDLER( input_0c )
{
	/*
	xxxx ---- keyboard related, it expects to return 1101 here
	*/
	if(input_code_pressed(device->machine,KEYCODE_Z))
		return 0x20;

	return 0x00;
}

static I8255A_INTERFACE( ppi8255_intf_0 )
{
	DEVCB_NULL,			/* Port A read */
	DEVCB_NULL,			/* Port B read */
	DEVCB_HANDLER(input_0c),			/* Port C read */
	DEVCB_HANDLER(vram_laddr_w),			/* Port A write */
	DEVCB_NULL,						/* Port B write */
	DEVCB_HANDLER(vram_haddr_w)		/* Port C write */
};

static I8255A_INTERFACE( ppi8255_intf_1 )
{
	DEVCB_HANDLER(input_1a),			/* Port A read */
	DEVCB_NULL,			/* Port B read */
	DEVCB_NULL,			/* Port C read */
	DEVCB_NULL,			/* Port A write */
	DEVCB_NULL,			/* Port B write */
	DEVCB_NULL			/* Port C write */
};

static I8255A_INTERFACE( ppi8255_intf_2 )
{
	DEVCB_NULL,			/* Port A read */
	DEVCB_NULL,			/* Port B read */
	DEVCB_NULL,			/* Port C read */
	DEVCB_NULL,			/* Port A write */
	DEVCB_NULL,			/* Port B write */
	DEVCB_NULL			/* Port C write */
};

static MACHINE_DRIVER_START( mycom )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(mycom_map)
    MDRV_CPU_IO_MAP(mycom_io)

    MDRV_MACHINE_RESET(mycom)

	MDRV_I8255A_ADD( "ppi8255_0", ppi8255_intf_0 )
	MDRV_I8255A_ADD( "ppi8255_1", ppi8255_intf_1 )
	MDRV_I8255A_ADD( "ppi8255_2", ppi8255_intf_2 )

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(320, 192)
    MDRV_SCREEN_VISIBLE_AREA(0, 320-1, 0, 192-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)
	MDRV_GFXDECODE(mycom)

	MDRV_MC6845_ADD("crtc", H46505, XTAL_3_579545MHz/4, mc6845_intf)	/* unknown clock, hand tuned to get ~60 fps */

    MDRV_VIDEO_START(mycom)
    MDRV_VIDEO_UPDATE(mycom)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( mycom )
    ROM_REGION( 0x14000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "bios.rom", 0xc000, 0x3000, CRC(e6f50355) SHA1(5d3acea360c0a8ab547db03a43e1bae5125f9c2a))
	ROM_LOAD( "basic.rom",0xf000, 0x1000, CRC(3b077465) SHA1(777427182627f371542c5e0521ed3ca1466a90e1))

	ROM_REGION( 0x0800, "vram", ROMREGION_ERASE00 )

	ROM_REGION( 0x0800, "gfx", ROMREGION_ERASEFF )
	ROM_LOAD( "font.rom", 0x0000, 0x0800, CRC(4039bb6f) SHA1(086ad303bf4bcf983fd6472577acbf744875fea8))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1981, mycom,  	0,       0, 		mycom, 	mycom, 	 0,  	   "Japan Electronics College",   "MYCOMZ-80A",		GAME_NOT_WORKING | GAME_NO_SOUND)

