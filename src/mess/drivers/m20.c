/* Olivetti M20 skeleton driver, by incog (19/05/2009) */

/*

Needs a proper Z8001 CPU core, check also

ftp://ftp.groessler.org/pub/chris/olivetti_m20/misc/bios/rom.s

---

APB notes:

0xfc903 checks for the string TEST at 0x3f4-0x3f6, does an int 0xfe if so, unknown purpose

Error codes:
Triangle 	Test CPU registers and instructions
Triangle 	Test system RAM
4 vertical lines 	Test CPU call and trap instructions
Diamond 	Initialize screen and printer drivers
EC0 	8255 parallel interface IC test failed
EC1 	6845 CRT controller IC test failed
EC2 	1797 floppy disk controller chip failed
EC3 	8253 timer IC failed
EC4 	8251 keyboard interface failed
EC5 	8251 keyboard test failed
EC6 	8259 PIC IC test failed
EK0 	Keyboard did not respond
Ek1 	Keyboard responds, but self test failed
ED1 	Disk drive 1 test failed
ED0 	Disk drive 0 test failed
E10 	Non-vectored interrupt error
E11 	Vectored interrupt error

*/

#include "emu.h"
#include "cpu/z8000/z8000.h"
#include "cpu/i86/i86.h"
#include "video/mc6845.h"


class m20_state : public driver_device
{
public:
	m20_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT16 *m_vram;
};


#define MAIN_CLOCK 4000000 /* 4 MHz */
#define PIXEL_CLOCK XTAL_4_433619MHz


static VIDEO_START( m20 )
{
}

static SCREEN_UPDATE( m20 )
{
	m20_state *state = screen->machine().driver_data<m20_state>();
	int x,y,i;
	UINT8 pen;
	UINT32 count;

	bitmap_fill(bitmap,cliprect,get_black_pen(screen->machine()));

	count = (0);

	for(y=0;y<256;y++)
	{
		for(x=0;x<256;x+=16)
		{
			for (i = 0; i < 16; i++)
			{
				pen = (state->m_vram[count]) >> (15 - i) & 1;

				if ((x + i) <= screen->visible_area().max_x && (y + 0) < screen->visible_area().max_y)
					*BITMAP_ADDR32(bitmap, y, x + i) = screen->machine().pens[pen];
			}

			count++;
		}
	}
    return 0;
}

static ADDRESS_MAP_START(m20_mem, AS_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x00000, 0x01fff ) AM_ROM AM_REGION("maincpu",0x10000)
	AM_RANGE( 0x40000, 0x41fff ) AM_ROM AM_REGION("maincpu",0x10000) //mirror

	AM_RANGE( 0x30000, 0x33fff ) AM_RAM AM_BASE_MEMBER(m20_state, m_vram)//base vram
//  AM_RANGE( 0x34000, 0x37fff ) AM_RAM //extra vram for bitmap mode
//  AM_RANGE( 0x20000, 0x2???? ) //work RAM?
//
ADDRESS_MAP_END

static ADDRESS_MAP_START( m20_io , AS_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x61, 0x61) AM_DEVWRITE("crtc", mc6845_address_w)
	AM_RANGE(0x63, 0x63) AM_DEVWRITE("crtc", mc6845_register_w)
	// 0x81 / 0x87 - i8255A
	// 0xa1 - i8251 keyboard data
	// 0xa3 - i8251 keyboard status / control
	// 0xc1 - i8251 TTY / printer data
	// 0xc3 - i8251 TTY / printer status / control
	// 0x121 / 0x127 - pit8253 (TTY/printer, keyboard, RTC/NVI)

	// 0x21?? / 0x21? - fdc ... seems to control the screen colors???
ADDRESS_MAP_END

static ADDRESS_MAP_START(m20_apb_mem, AS_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x00000, 0x007ff ) AM_RAM
	AM_RANGE( 0xf0000, 0xf7fff ) AM_RAM //mirrored?
	AM_RANGE( 0xfc000, 0xfffff ) AM_ROM AM_REGION("apb_bios",0)

ADDRESS_MAP_END

static ADDRESS_MAP_START( m20_apb_io , AS_IO, 16)
	ADDRESS_MAP_UNMAP_HIGH
	//0x4060 crtc address
	//0x4062 crtc data
ADDRESS_MAP_END

static INPUT_PORTS_START( m20 )
INPUT_PORTS_END

static DRIVER_INIT( m20 )
{
}

static MACHINE_RESET( m20 )
{
}

/* unknown decoding */
static const gfx_layout m20_chars_8x8 =
{
	8,8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4,5,6,7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static GFXDECODE_START( m20 )
	GFXDECODE_ENTRY( "maincpu",   0x11614, m20_chars_8x8,    0x000, 1 )
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

static MACHINE_CONFIG_START( m20, m20_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu", Z8001, MAIN_CLOCK)
    MCFG_CPU_PROGRAM_MAP(m20_mem)
    MCFG_CPU_IO_MAP(m20_io)

    MCFG_CPU_ADD("apb", I8086, MAIN_CLOCK)
    MCFG_CPU_PROGRAM_MAP(m20_apb_mem)
    MCFG_CPU_IO_MAP(m20_apb_io)
    MCFG_DEVICE_DISABLE()

	MCFG_MACHINE_RESET(m20)

	MCFG_MC6845_ADD("crtc", MC6845, PIXEL_CLOCK/8, mc6845_intf)	/* hand tuned to get ~50 fps */

    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(60)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
    MCFG_SCREEN_SIZE(512, 256)
    MCFG_SCREEN_VISIBLE_AREA(0, 512-1, 0, 256-1)
    MCFG_SCREEN_UPDATE(m20)

    MCFG_PALETTE_LENGTH(4)
//  MCFG_PALETTE_INIT(black_and_white)

    MCFG_VIDEO_START(m20)

	MCFG_GFXDECODE(m20)
MACHINE_CONFIG_END

ROM_START(m20)
	ROM_REGION(0x12000,"maincpu",0)
	ROM_SYSTEM_BIOS( 0, "m20", "M20 1.0" )
	ROMX_LOAD("m20.bin", 0x10000, 0x2000, CRC(5c93d931) SHA1(d51025e087a94c55529d7ee8fd18ff4c46d93230), ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "m20-20d", "M20 2.0d" )
	ROMX_LOAD("m20-20d.bin", 0x10000, 0x2000, CRC(cbe265a6) SHA1(c7cb9d9900b7b5014fcf1ceb2e45a66a91c564d0), ROM_BIOS(2))
	ROM_SYSTEM_BIOS( 2, "m20-20f", "M20 2.0f" )
	ROMX_LOAD("m20-20f.bin", 0x10000, 0x2000, CRC(db7198d8) SHA1(149d8513867081d31c73c2965dabb36d5f308041), ROM_BIOS(3))
	ROM_REGION(0x4000,"apb_bios",0) // Processor board with 8086
	ROM_LOAD( "apb-1086-2.0.bin", 0x0000, 0x4000, CRC(8c05be93) SHA1(2bb424afd874cc6562e9642780eaac2391308053))
ROM_END

ROM_START(m40)
	ROM_REGION(0x14000,"maincpu",0)
	ROM_SYSTEM_BIOS( 0, "m40-81", "M40 15.dec.81" )
	ROMX_LOAD( "m40rom-15-dec-81", 0x0000, 0x2000, CRC(e8e7df84) SHA1(e86018043bf5a23ff63434f9beef7ce2972d8153), ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "m40-82", "M40 17.dec.82" )
	ROMX_LOAD( "m40rom-17-dec-82", 0x0000, 0x2000, CRC(cf55681c) SHA1(fe4ae14a6751fef5d7bde49439286f1da3689437), ROM_BIOS(2))
	ROM_SYSTEM_BIOS( 2, "m40-41", "M40 4.1" )
	ROMX_LOAD( "m40rom-4.1", 0x0000, 0x2000, CRC(cf55681c) SHA1(fe4ae14a6751fef5d7bde49439286f1da3689437), ROM_BIOS(3))
	ROM_SYSTEM_BIOS( 3, "m40-60", "M40 6.0" )
	ROMX_LOAD( "m40rom-6.0", 0x0000, 0x4000, CRC(8114ebec) SHA1(4e2c65b95718c77a87dbee0288f323bd1c8837a3), ROM_BIOS(4))
	ROM_REGION(0x4000,"apb_bios",ROMREGION_ERASEFF) // Processor board with 8086
ROM_END

/*    YEAR  NAME   PARENT  COMPAT  MACHINE INPUT   INIT COMPANY     FULLNAME        FLAGS */
COMP( 1981, m20,   0,      0,      m20,    m20,    m20,	"Olivetti", "Olivetti L1 M20", GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 1981, m40,   m20,    0,      m20,    m20,    m20, "Olivetti", "Olivetti L1 M40", GAME_NOT_WORKING | GAME_NO_SOUND)
