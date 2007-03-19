/***************************************************************************

  nes.c

  Driver file to handle emulation of the Nintendo Entertainment System (Famicom).

  MESS driver by Brad Oliver (bradman@pobox.com), NES sound code by Matt Conte.
  Based in part on the old xNes code, by Nicolas Hamel, Chuck Mason, Brad Oliver,
  Richard Bannister and Jeff Mitchell.

***************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "video/ppu2c0x.h"
#include "includes/nes.h"
#include "cpu/m6502/m6502.h"
#include "devices/cartslot.h"
#include "sound/nes_apu.h"
#include "inputx.h"

unsigned char *battery_ram;

static READ8_HANDLER( psg_4015_r )
{
	return NESPSG_0_r(0x15);
}

static WRITE8_HANDLER( psg_4015_w )
{
	NESPSG_0_w(0x15, data);
}

static WRITE8_HANDLER( psg_4017_w )
{
	NESPSG_0_w(0x17, data);
}

static WRITE8_HANDLER(nes_vh_sprite_dma_w)
{
	ppu2c0x_spriteram_dma(0, data);
}

static ADDRESS_MAP_START( nes_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM		AM_MIRROR(0x1800)	/* RAM */
	AM_RANGE(0x2000, 0x3fff) AM_READWRITE(ppu2c0x_0_r,     ppu2c0x_0_w)			/* PPU registers */
	AM_RANGE(0x4000, 0x4013) AM_READWRITE(NESPSG_0_r, NESPSG_0_w)			/* PSG primary registers */
	AM_RANGE(0x4014, 0x4014) AM_WRITE(nes_vh_sprite_dma_w)				/* stupid address space hole */
	AM_RANGE(0x4015, 0x4015) AM_READWRITE(psg_4015_r, psg_4015_w)			/* PSG status / first control register */
	AM_RANGE(0x4016, 0x4016) AM_READWRITE(nes_IN0_r,        nes_IN0_w)			/* IN0 - input port 1 */
	AM_RANGE(0x4017, 0x4017) AM_READWRITE(nes_IN1_r,        psg_4017_w)		/* IN1 - input port 2 / PSG second control register */
	AM_RANGE(0x4100, 0x5fff) AM_READWRITE(nes_low_mapper_r, nes_low_mapper_w)	/* Perform unholy acts on the machine */
ADDRESS_MAP_END


INPUT_PORTS_START( nes )
	PORT_START  /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("P1 A") PORT_CODE(KEYCODE_LALT) PORT_CODE(JOYCODE_1_BUTTON1 )	PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("P1 B") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(JOYCODE_1_BUTTON2 )	PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SELECT)															PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START)															PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_NAME("P1 Up") PORT_CODE(KEYCODE_UP) PORT_CODE(JOYCODE_1_UP )		PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_NAME("P1 Down") PORT_CODE(KEYCODE_DOWN) PORT_CODE(JOYCODE_1_DOWN )	PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_NAME("P1 Left") PORT_CODE(KEYCODE_LEFT) PORT_CODE(JOYCODE_1_LEFT )	PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_NAME("P1 Right") PORT_CODE(KEYCODE_RIGHT) PORT_CODE(JOYCODE_1_RIGHT )	PORT_CATEGORY(1) PORT_PLAYER(1)

	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("P2 A") PORT_CODE(KEYCODE_0_PAD) PORT_CODE(JOYCODE_2_BUTTON1 )	PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("P2 B") PORT_CODE(KEYCODE_DEL_PAD) PORT_CODE(JOYCODE_2_BUTTON2 )	PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SELECT)															PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START)															PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_NAME("P2 Up") PORT_CODE(KEYCODE_8_PAD) PORT_CODE(JOYCODE_2_UP )		PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_NAME("P2 Down") PORT_CODE(KEYCODE_2_PAD) PORT_CODE(JOYCODE_2_DOWN )	PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_NAME("P2 Left") PORT_CODE(KEYCODE_4_PAD) PORT_CODE(JOYCODE_2_LEFT )	PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_NAME("P2 Right") PORT_CODE(KEYCODE_6_PAD) PORT_CODE(JOYCODE_2_RIGHT )	PORT_CATEGORY(2) PORT_PLAYER(2)

	PORT_START  /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1)															PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2)															PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SELECT)															PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START)															PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP)															PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN)															PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT)															PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT)															PORT_CATEGORY(3) PORT_PLAYER(3)

	PORT_START  /* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1)															PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2)															PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SELECT)															PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START)															PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP)															PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN)															PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT)															PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT)															PORT_CATEGORY(4) PORT_PLAYER(4)

	PORT_START  /* IN4 - P1 zapper */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X) PORT_SENSITIVITY(70) PORT_KEYDELTA(30) PORT_MINMAX(0,255 )														PORT_CATEGORY(5) PORT_PLAYER(1)
	PORT_START  /* IN5 - P1 zapper */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y) PORT_SENSITIVITY(50) PORT_KEYDELTA(30) PORT_MINMAX(0,255 )														PORT_CATEGORY(5) PORT_PLAYER(1)
	PORT_START  /* IN6 - P1 zapper trigger */
	PORT_BIT( 0x03, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("Lightgun Trigger") 										PORT_CATEGORY(5) PORT_PLAYER(1)

	PORT_START  /* IN7 - P2 zapper */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X) PORT_SENSITIVITY(70) PORT_KEYDELTA(30) PORT_MINMAX(0,255 )														PORT_CATEGORY(6) PORT_PLAYER(2)
	PORT_START  /* IN8 - P2 zapper */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y) PORT_SENSITIVITY(50) PORT_KEYDELTA(30) PORT_MINMAX(0,255 )														PORT_CATEGORY(6) PORT_PLAYER(2)
	PORT_START  /* IN9 - P2 zapper trigger */
	PORT_BIT( 0x03, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("Lightgun 2 Trigger") 										PORT_CATEGORY(6) PORT_PLAYER(2)

	PORT_START  /* IN10 - arkanoid paddle */
	PORT_BIT( 0xff, 0x7f, IPT_PADDLE) PORT_SENSITIVITY(25) PORT_KEYDELTA(3) PORT_MINMAX(0x62,0xf2 )																	PORT_CATEGORY(7)

	PORT_START  /* IN11 - configuration */
	PORT_CATEGORY_CLASS( 0x000f, 0x0001, "P1 Controller")
	PORT_CATEGORY_ITEM(  0x0000, "Unconnected",		0 )
	PORT_CATEGORY_ITEM(  0x0001, "Gamepad",			1 )
	PORT_CATEGORY_ITEM(  0x0002, "Zapper 1",		5 )
	PORT_CATEGORY_ITEM(  0x0003, "Zapper 2",		6 )
	PORT_CATEGORY_CLASS( 0x00f0, 0x0010, "P2 Controller")
	PORT_CATEGORY_ITEM(  0x0000, "Unconnected",		0 )
	PORT_CATEGORY_ITEM(  0x0010, "Gamepad",			2 )
	PORT_CATEGORY_ITEM(  0x0020, "Zapper 1",		5 )
	PORT_CATEGORY_ITEM(  0x0030, "Zapper 2",		6 )
	PORT_CATEGORY_ITEM(  0x0040, "Arkanoid paddle",	7 )
	PORT_CATEGORY_CLASS( 0x0f00, 0x0000, "P3 Controller")
	PORT_CATEGORY_ITEM(  0x0000, "Unconnected",		0 )
	PORT_CATEGORY_ITEM(  0x0100, "Gamepad",			3 )
	PORT_CATEGORY_CLASS( 0xf000, 0x0000, "P4 Controller")
	PORT_CATEGORY_ITEM(  0x0000, "Unconnected",		0 )
	PORT_CATEGORY_ITEM(  0x1000, "Gamepad",			4 )

	PORT_START  /* IN12 - configuration */
	PORT_CONFNAME( 0x01, 0x00, "Draw Top/Bottom 8 Lines")
	PORT_CONFSETTING(    0x01, DEF_STR(No) )
	PORT_CONFSETTING(    0x00, DEF_STR(Yes) )
	PORT_CONFNAME( 0x02, 0x00, "Enforce 8 Sprites/line")
	PORT_CONFSETTING(    0x02, DEF_STR(No) )
	PORT_CONFSETTING(    0x00, DEF_STR(Yes) )
INPUT_PORTS_END

INPUT_PORTS_START( famicom )
	PORT_START  /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("P1 A") PORT_CODE(KEYCODE_LALT) PORT_CODE(JOYCODE_1_BUTTON1 )	PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("P1 B") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(JOYCODE_1_BUTTON2 )	PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_SELECT)															PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_START)															PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_NAME("P1 Up") PORT_CODE(KEYCODE_UP) PORT_CODE(JOYCODE_1_UP )		PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_NAME("P1 Down") PORT_CODE(KEYCODE_DOWN) PORT_CODE(JOYCODE_1_DOWN )	PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_NAME("P1 Left") PORT_CODE(KEYCODE_LEFT) PORT_CODE(JOYCODE_1_LEFT )	PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_NAME("P1 Right") PORT_CODE(KEYCODE_RIGHT) PORT_CODE(JOYCODE_1_RIGHT )	PORT_CATEGORY(1) PORT_PLAYER(1)

	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("P2 A") PORT_CODE(KEYCODE_0_PAD) PORT_CODE(JOYCODE_2_BUTTON1 )	PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("P2 B") PORT_CODE(KEYCODE_DEL_PAD) PORT_CODE(JOYCODE_2_BUTTON2 )	PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_SELECT)															PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_START)															PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_NAME("P2 Up") PORT_CODE(KEYCODE_8_PAD) PORT_CODE(JOYCODE_2_UP )		PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_NAME("P2 Down") PORT_CODE(KEYCODE_2_PAD) PORT_CODE(JOYCODE_2_DOWN )	PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_NAME("P2 Left") PORT_CODE(KEYCODE_4_PAD) PORT_CODE(JOYCODE_2_LEFT )	PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_NAME("P2 Right") PORT_CODE(KEYCODE_6_PAD) PORT_CODE(JOYCODE_2_RIGHT )	PORT_CATEGORY(2) PORT_PLAYER(2)

	PORT_START  /* IN2 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1)															PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2)															PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_SELECT)															PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_START)															PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP)															PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN)															PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT)															PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT)															PORT_CATEGORY(3) PORT_PLAYER(3)

	PORT_START  /* IN3 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1)															PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2)															PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_SELECT)															PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_START)															PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP)															PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN)															PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT)															PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT)															PORT_CATEGORY(4) PORT_PLAYER(4)

	PORT_START  /* IN4 - P1 zapper */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X) PORT_SENSITIVITY(70) PORT_KEYDELTA(30) PORT_MINMAX(0,255 )														PORT_CATEGORY(5) PORT_PLAYER(1)
	PORT_START  /* IN5 - P1 zapper */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y) PORT_SENSITIVITY(50) PORT_KEYDELTA(30) PORT_MINMAX(0,255 )														PORT_CATEGORY(5) PORT_PLAYER(1)
	PORT_START  /* IN6 - P1 zapper trigger */
	PORT_BIT( 0x03, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("Lightgun Trigger") 										PORT_CATEGORY(5) PORT_PLAYER(1)

	PORT_START  /* IN7 - P2 zapper */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X) PORT_SENSITIVITY(70) PORT_KEYDELTA(30) PORT_MINMAX(0,255 )														PORT_CATEGORY(6) PORT_PLAYER(2)
	PORT_START  /* IN8 - P2 zapper */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y) PORT_SENSITIVITY(50) PORT_KEYDELTA(30) PORT_MINMAX(0,255 )														PORT_CATEGORY(6) PORT_PLAYER(2)
	PORT_START  /* IN9 - P2 zapper trigger */
	PORT_BIT( 0x03, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("Lightgun 2 Trigger") 										PORT_CATEGORY(6) PORT_PLAYER(2)

	PORT_START  /* IN10 - arkanoid paddle */
	PORT_BIT( 0xff, 0x7f, IPT_PADDLE) PORT_SENSITIVITY(25) PORT_KEYDELTA(3) PORT_MINMAX(0x62,0xf2 )																	PORT_CATEGORY(7)

	PORT_START  /* IN11 - configuration */
	PORT_CATEGORY_CLASS( 0x000f, 0x0001, "P1 Controller")
	PORT_CATEGORY_ITEM(  0x0000, "Unconnected",		0 )
	PORT_CATEGORY_ITEM(  0x0001, "Gamepad",			1 )
	PORT_CATEGORY_ITEM(  0x0002, "Zapper 1",		5 )
	PORT_CATEGORY_ITEM(  0x0003, "Zapper 2",		6 )
	PORT_CATEGORY_CLASS( 0x00f0, 0x0010, "P2 Controller")
	PORT_CATEGORY_ITEM(  0x0000, "Unconnected",		0 )
	PORT_CATEGORY_ITEM(  0x0010, "Gamepad",			2 )
	PORT_CATEGORY_ITEM(  0x0020, "Zapper 1",		5 )
	PORT_CATEGORY_ITEM(  0x0030, "Zapper 2",		6 )
	PORT_CATEGORY_ITEM(  0x0040, "Arkanoid paddle",	7 )
	PORT_CATEGORY_CLASS( 0x0f00, 0x0000, "P3 Controller")
	PORT_CATEGORY_ITEM(  0x0000, "Unconnected",		0 )
	PORT_CATEGORY_ITEM(  0x0100, "Gamepad",			3 )
	PORT_CATEGORY_CLASS( 0xf000, 0x0000, "P4 Controller")
	PORT_CATEGORY_ITEM(  0x0000, "Unconnected",		0 )
	PORT_CATEGORY_ITEM(  0x1000, "Gamepad",			4 )

	PORT_START /* IN12 - fake keys */
//  PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("Change Disk Side")
INPUT_PORTS_END

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

static struct NESinterface nes_interface =
{
	0
};

static struct NESinterface nespal_interface =
{
	0
};

ROM_START( nes )
    ROM_REGION( 0x10000, REGION_CPU1,0 )  /* Main RAM + program banks */
    ROM_REGION( 0x2000,  REGION_GFX1,0 )  /* VROM */
    ROM_REGION( 0x2000,  REGION_GFX2,0 )  /* VRAM */
    ROM_REGION( 0x10000, REGION_USER1,0 ) /* WRAM */
ROM_END

ROM_START( nespal )
    ROM_REGION( 0x10000, REGION_CPU1,0 )  /* Main RAM + program banks */
    ROM_REGION( 0x2000,  REGION_GFX1,0 )  /* VROM */
    ROM_REGION( 0x2000,  REGION_GFX2,0 )  /* VRAM */
    ROM_REGION( 0x10000, REGION_USER1,0 ) /* WRAM */
ROM_END

ROM_START( famicom )
    ROM_REGION( 0x10000, REGION_CPU1,0 )  /* Main RAM + program banks */
    ROM_LOAD_OPTIONAL ("disksys.rom", 0xe000, 0x2000, CRC(5e607dcf) SHA1(57fe1bdee955bb48d357e463ccbf129496930b62))

    ROM_REGION( 0x2000,  REGION_GFX1,0 )  /* VROM */
    ROM_REGION( 0x2000,  REGION_GFX2,0 )  /* VRAM */
    ROM_REGION( 0x10000, REGION_USER1,0 ) /* WRAM */
ROM_END

ROM_START( famitwin )
    ROM_REGION( 0x10000, REGION_CPU1,0 )  /* Main RAM + program banks */
    ROM_LOAD_OPTIONAL ("disksys.rom", 0xe000, 0x2000, CRC(4df24a6c) SHA1(e4e41472c454f928e53eb10e0509bf7d1146ecc1))

    ROM_REGION( 0x2000,  REGION_GFX1,0 )  /* VROM */
    ROM_REGION( 0x2000,  REGION_GFX2,0 )  /* VRAM */
    ROM_REGION( 0x10000, REGION_USER1,0 ) /* WRAM */
ROM_END



static MACHINE_DRIVER_START( nes )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", N2A03, NTSC_CLOCK)
	MDRV_CPU_PROGRAM_MAP(nes_map, 0)
	MDRV_SCREEN_REFRESH_RATE(60.098)
	// This isn't used so much to calulate the vblank duration (the PPU code tracks that manually) but to determine
	// the number of cycles in each scanline for the PPU scanline timer. Since the PPU has 20 vblank scanlines + 2
	// non-rendering scanlines, we compensate. This ends up being 2500 cycles for the non-rendering portion, 2273
	// cycles for the actual vblank period.
	MDRV_SCREEN_VBLANK_TIME(TIME_IN_USEC((113.66/(NTSC_CLOCK/1000000)) * (PPU_VBLANK_LAST_SCANLINE_NTSC-PPU_VBLANK_FIRST_SCANLINE+1+2)))

	MDRV_MACHINE_START( nes )

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(32*8, 30*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 32*8-1, 0*8, 30*8-1)
	MDRV_PALETTE_INIT(nes)
	MDRV_VIDEO_START(nes_ntsc)
	MDRV_VIDEO_UPDATE(nes)

	MDRV_PALETTE_LENGTH(4*16*8)
	MDRV_COLORTABLE_LENGTH(4*8)

    /* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD_TAG("nessound", NES, NTSC_CLOCK)
	MDRV_SOUND_CONFIG(nes_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( nespal )
	MDRV_IMPORT_FROM( nes )

	/* basic machine hardware */
	MDRV_CPU_REPLACE("main", N2A03, PAL_CLOCK)
	MDRV_SCREEN_REFRESH_RATE(53.355)
	MDRV_SCREEN_VBLANK_TIME(TIME_IN_USEC((106.53/(PAL_CLOCK/1000000)) * (PPU_VBLANK_LAST_SCANLINE_PAL-PPU_VBLANK_FIRST_SCANLINE+1+2)))

	MDRV_VIDEO_START(nes_pal)

    /* sound hardware */
	MDRV_SOUND_REPLACE("nessound", NES, PAL_CLOCK)
	MDRV_SOUND_CONFIG(nespal_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END

static void nes_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;
		case DEVINFO_INT_MUST_BE_LOADED:				info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_nes_cart; break;
		case DEVINFO_PTR_PARTIAL_HASH:					info->partialhash = nes_partialhash; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "nes"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(nes)
	CONFIG_DEVICE(nes_cartslot_getinfo)
SYSTEM_CONFIG_END

static void famicom_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_nes_cart; break;
		case DEVINFO_PTR_PARTIAL_HASH:					info->partialhash = nes_partialhash; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "nes"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

static void famicom_floppy_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:							info->i = IO_FLOPPY; break;
		case DEVINFO_INT_READABLE:						info->i = 1; break;
		case DEVINFO_INT_WRITEABLE:						info->i = 0; break;
		case DEVINFO_INT_CREATABLE:						info->i = 0; break;
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_nes_disk; break;
		case DEVINFO_PTR_UNLOAD:						info->unload = device_unload_nes_disk; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "dsk,fds"); break;
	}
}

SYSTEM_CONFIG_START(famicom)
	CONFIG_DEVICE(famicom_cartslot_getinfo)
	CONFIG_DEVICE(famicom_floppy_getinfo)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*     YEAR  NAME      PARENT    COMPAT	MACHINE   INPUT     INIT      CONFIG	COMPANY   FULLNAME */
CONS( 1983, famicom,   0,        0,		nes,      famicom,  0,	      famicom,	"Nintendo", "Famicom" , GAME_NOT_WORKING)
CONS( 1986, famitwin,  famicom,  0,		nes,      famicom,  0,	      famicom,	"Sharp", "Famicom Twin" , GAME_NOT_WORKING)
CONS( 1985, nes,       0,        0,		nes,      nes,      0,        nes,		"Nintendo", "Nintendo Entertainment System (NTSC)" , 0)
CONS( 1987, nespal,    nes,      0,		nespal,   nes,      0,	      nes,		"Nintendo", "Nintendo Entertainment System (PAL)" , 0)

