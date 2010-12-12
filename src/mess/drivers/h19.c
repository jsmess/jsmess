/***************************************************************************

        Heathkit H19

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "video/mc6845.h"


class h19_state : public driver_device
{
public:
	h19_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *video_ram;
	UINT8 *charrom;
};



static ADDRESS_MAP_START(h19_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0xf7ff) AM_RAM
	AM_RANGE(0xf800, 0xffff) AM_RAM AM_BASE_MEMBER(h19_state, video_ram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( h19_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x60, 0x60) AM_DEVWRITE("crtc", mc6845_address_w)
	AM_RANGE(0x61, 0x61) AM_DEVREADWRITE("crtc", mc6845_register_r , mc6845_register_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( h19 )
INPUT_PORTS_END


static MACHINE_RESET(h19)
{
}

static VIDEO_START( h19 )
{
	h19_state *state = machine->driver_data<h19_state>();
	state->charrom = memory_region(machine, "chargen");
}

static VIDEO_UPDATE( h19 )
{
	running_device *mc6845 = screen->machine->device("crtc");
	mc6845_update(mc6845, bitmap, cliprect);
	return 0;
}

static MC6845_UPDATE_ROW( h19_update_row )
{
	h19_state *state = device->machine->driver_data<h19_state>();
	UINT8 chr,gfx;
	UINT16 mem,x;
	UINT16 *p = BITMAP_ADDR16(bitmap, y, 0);

	for (x = 0; x < x_count; x++)
	{
		UINT8 inv=0;
		if (x == cursor_x) inv=0xff;
		mem = (ma + x) & 0xfff;
		chr = state->video_ram[mem];

		if (chr & 0x80)
		{
			inv ^= 0xff;
			chr &= 0x7f;
		}

		/* get pattern of pixels for that character scanline */
		gfx = state->charrom[(chr<<4) | ra] ^ inv;

		/* Display a scanline of a character (8 pixels) */
		*p++ = ( gfx & 0x80 ) ? 1 : 0;
		*p++ = ( gfx & 0x40 ) ? 1 : 0;
		*p++ = ( gfx & 0x20 ) ? 1 : 0;
		*p++ = ( gfx & 0x10 ) ? 1 : 0;
		*p++ = ( gfx & 0x08 ) ? 1 : 0;
		*p++ = ( gfx & 0x04 ) ? 1 : 0;
		*p++ = ( gfx & 0x02 ) ? 1 : 0;
		*p++ = ( gfx & 0x01 ) ? 1 : 0;
	}
}

static const mc6845_interface h19_crtc6845_interface =
{
	"screen",
	8 /*?*/,
	NULL,
	h19_update_row,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	NULL
};

/* F4 Character Displayer */
static const gfx_layout h19_charlayout =
{
	8, 10,					/* 8 x 10 characters */
	128,					/* 128 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8 },
	8*16					/* every char takes 16 bytes */
};

static GFXDECODE_START( h19 )
	GFXDECODE_ENTRY( "chargen", 0x0000, h19_charlayout, 0, 1 )
GFXDECODE_END


static MACHINE_CONFIG_START( h19, h19_state )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu",Z80, XTAL_12_288MHz / 6) // From schematics
	MDRV_CPU_PROGRAM_MAP(h19_mem)
	MDRV_CPU_IO_MAP(h19_io)

	MDRV_MACHINE_RESET(h19)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 200)
	MDRV_SCREEN_VISIBLE_AREA(0, 640 - 1, 0, 200 - 1)
	MDRV_GFXDECODE(h19)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_MC6845_ADD("crtc", MC6845, XTAL_12_288MHz / 8, h19_crtc6845_interface) // clk taken from schematics

	MDRV_VIDEO_START( h19 )
	MDRV_VIDEO_UPDATE( h19 )
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( h19 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	// Original
	ROM_SYSTEM_BIOS(0, "orig", "Original")
	ROMX_LOAD( "2732_444-46_h19code.bin", 0x0000, 0x1000, CRC(F4447DA0) SHA1(fb4093d5b763be21a9580a0defebed664b1f7a7b), ROM_BIOS(1))
	// Super H19 ROM (
	ROM_SYSTEM_BIOS(1, "super", "Super 19")
	ROMX_LOAD( "2732_super19_h447.bin", 0x0000, 0x1000, CRC(68FBFF54) SHA1(c0aa7199900709d717b07e43305dfdf36824da9b), ROM_BIOS(2))
	// Watzman ROM
	ROM_SYSTEM_BIOS(2, "watzman", "Watzman")
	ROMX_LOAD( "watzman.bin", 0x0000, 0x1000, CRC(8168b6dc) SHA1(bfaebb9d766edbe545d24bc2b6630be4f3aa0ce9), ROM_BIOS(3))

	ROM_REGION( 0x0800, "chargen", 0 )
	// Original font dump
	ROM_LOAD( "2716_444-29_h19font.bin", 0x0000, 0x0800, CRC(d595ac1d) SHA1(130fb4ea8754106340c318592eec2d8a0deaf3d0))

	ROM_REGION( 0x0800, "keyboard", 0 )
	// Original dump
	ROM_LOAD( "2716_444-37_h19keyb.bin", 0x0000, 0x0800, CRC(5c3e6972) SHA1(df49ce64ae48652346a91648c58178a34fb37d3c))
	// Watzman keyboard
	ROM_LOAD( "keybd.bin", 0x0000, 0x0800, CRC(58dc8217) SHA1(1b23705290bdf9fc6342065c6a528c04bff67b13))

ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 19??, h19,     0,       0,	h19,	h19,	 0, 	"Heath Inc", "Heathkit H-19", GAME_NOT_WORKING | GAME_NO_SOUND)
