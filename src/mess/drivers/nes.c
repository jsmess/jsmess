/***************************************************************************

  nes.c

  Driver file to handle emulation of the Nintendo Entertainment System (Famicom).

  MESS driver by Brad Oliver (bradman@pobox.com), NES sound code by Matt Conte.
  Based in part on the old xNes code, by Nicolas Hamel, Chuck Mason, Brad Oliver,
  Richard Bannister and Jeff Mitchell.

***************************************************************************/

#include "emu.h"
#include "video/ppu2c0x.h"
#include "machine/nes_mmc.h"
#include "includes/nes.h"
#include "cpu/m6502/m6502.h"
#include "devices/cartslot.h"
#include "sound/nes_apu.h"
#include "devices/flopdrv.h"
#include "formats/nes_dsk.h"


static READ8_DEVICE_HANDLER( psg_4015_r )
{
	return nes_psg_r(device, 0x15);
}

static WRITE8_DEVICE_HANDLER( psg_4015_w )
{
	nes_psg_w(device, 0x15, data);
}

static WRITE8_DEVICE_HANDLER( psg_4017_w )
{
	nes_psg_w(device, 0x17, data);
}

static WRITE8_HANDLER(nes_vh_sprite_dma_w)
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	ppu2c0x_spriteram_dma(space, state->ppu, data);
}

static ADDRESS_MAP_START( nes_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM AM_MIRROR(0x1800)					/* RAM */
	AM_RANGE(0x2000, 0x3fff) AM_DEVREADWRITE("ppu", ppu2c0x_r, ppu2c0x_w)		/* PPU registers */
	AM_RANGE(0x4000, 0x4013) AM_DEVREADWRITE("nessound", nes_psg_r, nes_psg_w)		/* PSG primary registers */
	AM_RANGE(0x4014, 0x4014) AM_WRITE(nes_vh_sprite_dma_w)				/* stupid address space hole */
	AM_RANGE(0x4015, 0x4015) AM_DEVREADWRITE("nessound", psg_4015_r, psg_4015_w)		/* PSG status / first control register */
	AM_RANGE(0x4016, 0x4016) AM_READWRITE(nes_IN0_r, nes_IN0_w)			/* IN0 - input port 1 */
	AM_RANGE(0x4017, 0x4017) AM_READ(nes_IN1_r)							/* IN1 - input port 2 */
	AM_RANGE(0x4017, 0x4017) AM_DEVWRITE("nessound", psg_4017_w)		/* PSG second control register */
	AM_RANGE(0x4100, 0x5fff) AM_READWRITE(nes_low_mapper_r, nes_low_mapper_w)	/* Perform unholy acts on the machine */
ADDRESS_MAP_END


static INPUT_PORTS_START( nes_controllers )
	PORT_START("CTRLSEL")  /* Select Controller Type */
	PORT_CATEGORY_CLASS( 0x000f, 0x0001, "P1 Controller")
	PORT_CATEGORY_ITEM(  0x0000, "Unconnected",		0 )
	PORT_CATEGORY_ITEM(  0x0001, "Gamepad",			1 )
	PORT_CATEGORY_ITEM(  0x0002, "Zapper",			5 )
	PORT_CATEGORY_CLASS( 0x00f0, 0x0010, "P2 Controller")
	PORT_CATEGORY_ITEM(  0x0000, "Unconnected",		0 )
	PORT_CATEGORY_ITEM(  0x0010, "Gamepad",			2 )
	PORT_CATEGORY_ITEM(  0x0030, "Zapper",			6 )
	PORT_CATEGORY_ITEM(  0x0040, "Arkanoid paddle",	7 )
	PORT_CATEGORY_CLASS( 0x0f00, 0x0000, "P3 Controller")
	PORT_CATEGORY_ITEM(  0x0000, "Unconnected",		0 )
	PORT_CATEGORY_ITEM(  0x0100, "Gamepad",			3 )
	PORT_CATEGORY_CLASS( 0xf000, 0x0000, "P4 Controller")
	PORT_CATEGORY_ITEM(  0x0000, "Unconnected",		0 )
	PORT_CATEGORY_ITEM(  0x1000, "Gamepad",			4 )

	PORT_START("PAD1")	/* Joypad 1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("P1 A") PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("P1 B") PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SELECT ) PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START ) PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_CATEGORY(1) PORT_PLAYER(1)

	PORT_START("PAD2")	/* Joypad 2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("P2 A") PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("P2 B") PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SELECT ) PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START ) PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_CATEGORY(2) PORT_PLAYER(2)

	PORT_START("PAD3")	/* Joypad 3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("P3 A") PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("P3 B") PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SELECT ) PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START ) PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_CATEGORY(3) PORT_PLAYER(3)

	PORT_START("PAD4")	/* Joypad 4 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("P4 A") PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("P4 B") PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SELECT ) PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START ) PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_CATEGORY(4) PORT_PLAYER(4)

	PORT_START("ZAPPER1_X")  /* P1 zapper */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X) PORT_CROSSHAIR(X, 1.0, 0.0, 0) PORT_SENSITIVITY(70) PORT_KEYDELTA(30) PORT_MINMAX(0,255)														PORT_CATEGORY(5) PORT_PLAYER(1)
	PORT_START("ZAPPER1_Y")  /* P1 zapper */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y) PORT_CROSSHAIR(Y, 1.0, 0.0, 0) PORT_SENSITIVITY(50) PORT_KEYDELTA(30) PORT_MINMAX(0,255)														PORT_CATEGORY(5) PORT_PLAYER(1)
	PORT_START("ZAPPER1_T")  /* P1 zapper trigger */
	PORT_BIT( 0x03, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("P1 Lightgun Trigger") PORT_CATEGORY(5) PORT_PLAYER(1)

	PORT_START("ZAPPER2_X")  /* P2 zapper */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X) PORT_CROSSHAIR(X, 1.0, 0.0, 0) PORT_SENSITIVITY(70) PORT_KEYDELTA(30) PORT_MINMAX(0,255 )														PORT_CATEGORY(6) PORT_PLAYER(2)
	PORT_START("ZAPPER2_Y")  /* P2 zapper */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y) PORT_CROSSHAIR(Y, 1.0, 0.0, 0) PORT_SENSITIVITY(50) PORT_KEYDELTA(30) PORT_MINMAX(0,255 )														PORT_CATEGORY(6) PORT_PLAYER(2)
	PORT_START("ZAPPER2_T")  /* P2 zapper trigger */
	PORT_BIT( 0x03, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("P2 Lightgun Trigger") PORT_CATEGORY(6) PORT_PLAYER(2)

	PORT_START("PADDLE")  /* Arkanoid paddle */
	PORT_BIT( 0xff, 0x7f, IPT_PADDLE) PORT_SENSITIVITY(25) PORT_KEYDELTA(3) PORT_MINMAX(0x62,0xf2)																	PORT_CATEGORY(7)
INPUT_PORTS_END

static INPUT_PORTS_START( nes )
	PORT_INCLUDE( nes_controllers )

	PORT_START("CONFIG")  /* configuration */
	PORT_CONFNAME( 0x01, 0x00, "Draw Top/Bottom 8 Lines")
	PORT_CONFSETTING(    0x01, DEF_STR(No) )
	PORT_CONFSETTING(    0x00, DEF_STR(Yes) )
	PORT_CONFNAME( 0x02, 0x00, "Enforce 8 Sprites/line")
	PORT_CONFSETTING(    0x02, DEF_STR(No) )
	PORT_CONFSETTING(    0x00, DEF_STR(Yes) )
INPUT_PORTS_END

static INPUT_PORTS_START( famicom )
	PORT_INCLUDE( nes_controllers )

	PORT_START("FLIPDISK") /* fake keys */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("Change Disk Side")
INPUT_PORTS_END

#ifdef UNUSED_FUNCTION
/* This layout is not changed at runtime */
gfx_layout nes_vram_charlayout =
{
    8,8,    /* 8*8 characters */
    512,    /* 512 characters */
    2,  /* 2 bits per pixel */
    { 8*8, 0 }, /* the two bitplanes are separated */
    { 0, 1, 2, 3, 4, 5, 6, 7 },
    { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
    16*8    /* every char takes 16 consecutive bytes */
};
#endif

static const nes_interface nes_apu_interface =
{
	"maincpu"
};


static void ppu_nmi(running_device *device, int *ppu_regs)
{
	cputag_set_input_line(device->machine, "maincpu", INPUT_LINE_NMI, PULSE_LINE);
}


static const ppu2c0x_interface nes_ppu_interface =
{
	0,
	0,
	PPU_MIRROR_NONE,
	ppu_nmi
};

static const floppy_config nes_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(nes_only),
	DO_NOT_KEEP_GEOMETRY
};


static MACHINE_DRIVER_START( nes )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", N2A03, NTSC_CLOCK)
	MDRV_CPU_PROGRAM_MAP(nes_map)

	MDRV_DRIVER_DATA( nes_state )

	MDRV_MACHINE_START( nes )
	MDRV_MACHINE_RESET( nes )

	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60.098)
	// This isn't used so much to calulate the vblank duration (the PPU code tracks that manually) but to determine
	// the number of cycles in each scanline for the PPU scanline timer. Since the PPU has 20 vblank scanlines + 2
	// non-rendering scanlines, we compensate. This ends up being 2500 cycles for the non-rendering portion, 2273
	// cycles for the actual vblank period.
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC((113.66/(NTSC_CLOCK/1000000)) * (PPU_VBLANK_LAST_SCANLINE_NTSC-PPU_VBLANK_FIRST_SCANLINE+1+2)))
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(32*8, 262)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 32*8-1, 0*8, 30*8-1)
	MDRV_PALETTE_INIT(nes)
	MDRV_VIDEO_START(nes_ntsc)
	MDRV_VIDEO_UPDATE(nes)

	MDRV_PALETTE_LENGTH(4*16*8)

	MDRV_PPU2C02_ADD( "ppu", nes_ppu_interface )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("nessound", NES, NTSC_CLOCK)
	MDRV_SOUND_CONFIG(nes_apu_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.90)

	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("nes,unf")
	MDRV_CARTSLOT_MANDATORY
	MDRV_CARTSLOT_LOAD(nes_cart)
	MDRV_CARTSLOT_PARTIALHASH(nes_partialhash)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( nespal )
	MDRV_IMPORT_FROM( nes )

	/* basic machine hardware */
	MDRV_CPU_REPLACE("maincpu", N2A03, PAL_CLOCK)

	MDRV_DEVICE_REMOVE( "ppu" )
	MDRV_PPU2C07_ADD( "ppu", nes_ppu_interface )

	/* video hardware */
	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_REFRESH_RATE(53.355)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC((106.53/(PAL_CLOCK/1000000)) * (PPU_VBLANK_LAST_SCANLINE_PAL-PPU_VBLANK_FIRST_SCANLINE+1+2)))
	MDRV_VIDEO_START(nes_pal)

	/* sound hardware */
	MDRV_SOUND_REPLACE("nessound", NES, PAL_CLOCK)
	MDRV_SOUND_CONFIG(nes_apu_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.90)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( dendy )
	MDRV_IMPORT_FROM( nes )

	/* basic machine hardware */
	MDRV_CPU_REPLACE("maincpu", N2A03, 26601712/15) /* 26.601712MHz / 15 == 1.77344746666... MHz */

	MDRV_DEVICE_REMOVE( "ppu" )
	MDRV_PPU2C07_ADD( "ppu", nes_ppu_interface )

	/* video hardware */
	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_REFRESH_RATE(50.00697796827)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC((106.53/(PAL_CLOCK/1000000)) * (PPU_VBLANK_LAST_SCANLINE_PAL-PPU_VBLANK_FIRST_SCANLINE+1+2)))
	MDRV_VIDEO_START(nes_pal)

	/* sound hardware */
	MDRV_SOUND_REPLACE("nessound", NES, 26601712/15) /* 26.601712MHz / 15 == 1.77344746666... MHz */
	MDRV_SOUND_CONFIG(nes_apu_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.90)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( famicom )
	MDRV_IMPORT_FROM( nes )

	MDRV_CARTSLOT_MODIFY("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("nes,unf")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_LOAD(nes_cart)
	MDRV_CARTSLOT_PARTIALHASH(nes_partialhash)

	MDRV_FLOPPY_DRIVE_ADD(FLOPPY_0, nes_floppy_config)
MACHINE_DRIVER_END


ROM_START( nes )
	ROM_REGION( 0x10000, "maincpu", 0 )  /* Main RAM + program banks */
	ROM_FILL( 0x0000, 0x10000, 0x00 )
	ROM_REGION( 0x2000,  "gfx1", 0 )  /* VROM */
	ROM_FILL( 0x0000, 0x2000, 0x00 )
	ROM_REGION( 0x4000,  "gfx2", 0 )  /* VRAM */
	ROM_FILL( 0x0000, 0x4000, 0x00 )
	ROM_REGION( 0x800,  "gfx3", 0 )  /* CI RAM */
	ROM_FILL( 0x0000, 0x800, 0x00 )
	ROM_REGION( 0x10000, "user1", 0 ) /* WRAM */
	ROM_FILL( 0x0000, 0x10000, 0x00 )
ROM_END

ROM_START( nespal )
	ROM_REGION( 0x10000, "maincpu", 0 )  /* Main RAM + program banks */
	ROM_FILL( 0x0000, 0x10000, 0x00 )
	ROM_REGION( 0x2000,  "gfx1", 0 )  /* VROM */
	ROM_FILL( 0x0000, 0x2000, 0x00 )
	ROM_REGION( 0x4000,  "gfx2", 0 )  /* VRAM */
	ROM_FILL( 0x0000, 0x4000, 0x00 )
	ROM_REGION( 0x800,  "gfx3", 0 )  /* CI RAM */
	ROM_FILL( 0x0000, 0x800, 0x00 )
	ROM_REGION( 0x10000, "user1", 0 ) /* WRAM */
	ROM_FILL( 0x0000, 0x10000, 0x00 )
ROM_END

ROM_START( famicom )
	ROM_REGION( 0x10000, "maincpu", 0 )  /* Main RAM + program banks */
	ROM_LOAD_OPTIONAL( "disksys.rom", 0xe000, 0x2000, CRC(5e607dcf) SHA1(57fe1bdee955bb48d357e463ccbf129496930b62) )

	ROM_REGION( 0x2000,  "gfx1", 0 )  /* VROM */
	ROM_FILL( 0x0000, 0x2000, 0x00 )
	ROM_REGION( 0x4000,  "gfx2", 0 )  /* VRAM */
	ROM_FILL( 0x0000, 0x4000, 0x00 )
	ROM_REGION( 0x800,  "gfx3", 0 )  /* CI RAM */
	ROM_FILL( 0x0000, 0x800, 0x00 )
	ROM_REGION( 0x10000, "user1", 0 ) /* WRAM */
	ROM_FILL( 0x0000, 0x10000, 0x00 )
ROM_END

ROM_START( famitwin )
	ROM_REGION( 0x10000, "maincpu", 0 )  /* Main RAM + program banks */
	ROM_LOAD_OPTIONAL( "disksyst.rom", 0xe000, 0x2000, CRC(4df24a6c) SHA1(e4e41472c454f928e53eb10e0509bf7d1146ecc1) )

	ROM_REGION( 0x2000,  "gfx1", 0 )  /* VROM */
	ROM_FILL( 0x0000, 0x2000, 0x00 )
	ROM_REGION( 0x4000,  "gfx2", 0 )  /* VRAM */
	ROM_FILL( 0x0000, 0x4000, 0x00 )
	ROM_REGION( 0x800,  "gfx3", 0 )  /* CI RAM */
	ROM_FILL( 0x0000, 0x800, 0x00 )
	ROM_REGION( 0x10000, "user1", 0 ) /* WRAM */
	ROM_FILL( 0x0000, 0x10000, 0x00 )
ROM_END

ROM_START( m82 )
	ROM_REGION( 0x14000, "maincpu", 0 )  /* Main RAM + program banks */
	/* Banks to be mapped at 0xe000? More investigations needed... */
	ROM_LOAD( "m82_v1_0.bin", 0x10000, 0x4000, CRC(7d56840a) SHA1(cbd2d14fa073273ba58367758f40d67fd8a9106d) )

	ROM_REGION( 0x2000,  "gfx1", 0 )  /* VROM */
	ROM_FILL( 0x0000, 0x2000, 0x00 )
	ROM_REGION( 0x4000,  "gfx2", 0 )  /* VRAM */
	ROM_FILL( 0x0000, 0x4000, 0x00 )
	ROM_REGION( 0x800,  "gfx3", 0 )  /* CI RAM */
	ROM_FILL( 0x0000, 0x800, 0x00 )
	ROM_REGION( 0x10000, "user1", 0 ) /* WRAM */
	ROM_FILL( 0x0000, 0x10000, 0x00 )
ROM_END

// see http://www.disgruntleddesigner.com/chrisc/drpcjr/index.html
// and http://www.disgruntleddesigner.com/chrisc/drpcjr/DrPCJrMemMap.txt
ROM_START( drpcjr )
	ROM_REGION( 0x18000, "maincpu", 0 )  /* Main RAM + program banks */
	/* 4 banks to be mapped in 0xe000-0xffff (or 8 banks to be mapped in 0xe000-0xefff & 0xf000-0xffff).
    Banks selected by writing at 0x4180 */
	ROM_LOAD("drpcjr_bios.bin", 0x10000, 0x8000, CRC(c8fbef89) SHA1(2cb0a817b31400cdf27817d09bae7e69f41b062b) )	// bios vers. 1.0a
	// Not sure if we should support this: hacked version 1.5a by Chris Covell with bugfixes and GameGenie support
//  ROM_LOAD("drpcjr_v1_5_gg.bin", 0x10000, 0x8000, CRC(98f2033b) SHA1(93c114da787a19279d1a46667c2f69b49e25d4f1) )

	ROM_REGION( 0x2000,  "gfx1", 0 )  /* VROM */
	ROM_FILL( 0x0000, 0x2000, 0x00 )
	ROM_REGION( 0x4000,  "gfx2", 0 )  /* VRAM */
	ROM_FILL( 0x0000, 0x4000, 0x00 )
	ROM_REGION( 0x800,  "gfx3", 0 )  /* CI RAM */
	ROM_FILL( 0x0000, 0x800, 0x00 )
	ROM_REGION( 0x10000, "user1", 0 ) /* WRAM */
	ROM_FILL( 0x0000, 0x10000, 0x00 )
ROM_END

ROM_START( dendy )
	ROM_REGION( 0x10000, "maincpu", 0 )  /* Main RAM + program banks */
	ROM_FILL( 0x0000, 0x10000, 0x00 )
	ROM_REGION( 0x2000,  "gfx1", 0 )  /* VROM */
	ROM_FILL( 0x0000, 0x2000, 0x00 )
	ROM_REGION( 0x4000,  "gfx2", 0 )  /* VRAM */
	ROM_FILL( 0x0000, 0x4000, 0x00 )
	ROM_REGION( 0x800,  "gfx3", 0 )  /* CI RAM */
	ROM_FILL( 0x0000, 0x800, 0x00 )
	ROM_REGION( 0x10000, "user1", 0 ) /* WRAM */
	ROM_FILL( 0x0000, 0x10000, 0x00 )
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*     YEAR  NAME      PARENT  COMPAT MACHINE   INPUT    INIT    COMPANY       FULLNAME */
CONS( 1985, nes,       0,      0,     nes,      nes,     0,      "Nintendo",  "Nintendo Entertainment System / Famicom (NTSC)", GAME_IMPERFECT_GRAPHICS )
CONS( 1987, nespal,    nes,    0,     nespal,   nes,     0,      "Nintendo",  "Nintendo Entertainment System (PAL)", GAME_IMPERFECT_GRAPHICS )
CONS( 1983, famicom,   nes,    0,     famicom,  famicom, famicom,"Nintendo",  "Famicom Disk System", GAME_IMPERFECT_GRAPHICS )
CONS( 1986, famitwin,  nes,    0,     famicom,  famicom, famicom,"Sharp",     "Famicom Twin", GAME_IMPERFECT_GRAPHICS )
CONS( 198?, m82,       nes,    0,     nes,      nes,     0,      "Nintendo",  "M82 Display Unit", GAME_IMPERFECT_GRAPHICS )
CONS( 1996, drpcjr,    nes,    0,     famicom,  famicom, famicom,"Bung",      "Doctor PC Jr", GAME_IMPERFECT_GRAPHICS )
CONS( 1992, dendy,     nes,    0,     dendy,    nes,     0,      "Steepler",  "Dendy Classic", GAME_IMPERFECT_GRAPHICS )
