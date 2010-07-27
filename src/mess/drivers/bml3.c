/***************************************************************************

        Hitachi Basic Master Level 3 (MB-6890)

        13/07/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/m6809/m6809.h"
#include "video/mc6845.h"

static UINT8 *work_ram;
static UINT16 cursor_addr,cursor_raster;

static VIDEO_START( bml3 )
{
}

static VIDEO_UPDATE( bml3 )
{
	int x,y,count;
	int xi,yi;
	static UINT8 *gfx_rom = memory_region(screen->machine, "char");

	count = 0x0400;

	for(y=0;y<25;y++)
	{
		for(x=0;x<40;x++)
		{
			int pen_b = work_ram[count+0x0000];
			int pen_r = work_ram[count+0x0000];
			int pen_g = work_ram[count+0x0000];

			for(yi=0;yi<8;yi++)
			{
				for(xi=0;xi<8;xi++)
				{
					int pen[3],color;

					pen[0] = (gfx_rom[pen_b*8+yi] >> (7-xi) & 1) ? 1 : 0;
					pen[1] = (gfx_rom[pen_r*8+yi] >> (7-xi) & 1) ? 2 : 0;
					pen[2] = (gfx_rom[pen_g*8+yi] >> (7-xi) & 1) ? 4 : 0;

					color = (pen[0]) | (pen[1]) | (pen[2]);

					*BITMAP_ADDR16(bitmap, y*8+yi, x*8+xi) = screen->machine->pens[color];
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
							*BITMAP_ADDR16(bitmap, y*8+yc+7, x*8+xc) = screen->machine->pens[0x7];
						}
					}
				}
			}

			count++;
		}
	}

    return 0;
}

static WRITE8_HANDLER( bml3_6845_w )
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

/* Note: this custom code is there just for simplicity, it'll be nuked in the end */
static READ8_HANDLER( bml3_io_r )
{
	static UINT8 *rom = memory_region(space->machine, "maincpu");

	if(offset < 0xc0)
	{
		logerror("I/O read [%02x] at PC=%04x\n",offset,cpu_get_pc(space->cpu));
		return 0;
	}

	if(offset == 0xc8) //this seems the keyboard status
		return 0;

	if(offset == 0xc9)
		return 0x11; //put 320 x 200 mode

	/* TODO: pretty sure that there's a bankswitch for this */
	return rom[offset+0xff00];
}

static WRITE8_HANDLER( bml3_io_w )
{
	if(offset == 0xc6 || offset == 0xc7) { bml3_6845_w(space,offset-0xc6,data); }
	else
	{
		logerror("I/O write %02x -> [%02x] at PC=%04x\n",data,offset,cpu_get_pc(space->cpu));
	}
}

static ADDRESS_MAP_START(bml3_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x9fff) AM_RAM AM_BASE(&work_ram)
	AM_RANGE(0xff00, 0xffff) AM_READWRITE(bml3_io_r,bml3_io_w)
	AM_RANGE(0xa000, 0xffff) AM_ROM
ADDRESS_MAP_END


/* Input ports */
static INPUT_PORTS_START( bml3 )
INPUT_PORTS_END


static MACHINE_RESET(bml3)
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


static MACHINE_DRIVER_START( bml3 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",M6809, XTAL_1MHz)
    MDRV_CPU_PROGRAM_MAP(bml3_mem)

    MDRV_MACHINE_RESET(bml3)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(60)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(8)

	MDRV_MC6845_ADD("crtc", H46505, XTAL_3_579545MHz/4, mc6845_intf)	/* unknown clock, hand tuned to get ~60 fps */

    MDRV_VIDEO_START(bml3)
    MDRV_VIDEO_UPDATE(bml3)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( bml3 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "l3bas.rom", 0xa000, 0x6000, CRC(d81baa07) SHA1(a8fd6b29d8c505b756dbf5354341c48f9ac1d24d))

    ROM_REGION( 0x800, "char", 0 )
	ROM_LOAD("char.rom", 0x00000, 0x00800, BAD_DUMP CRC(e3995a57) SHA1(1c1a0d8c9f4c446ccd7470516b215ddca5052fb2) ) //Taken from Sharp X1
	ROM_FILL( 0x0000, 0x0008, 0x00)
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1980, bml3,  	0,       0, 		bml3, 	bml3, 	 0,  	   "Hitachi",   "Basic Master Level 3",		GAME_NOT_WORKING | GAME_NO_SOUND)

