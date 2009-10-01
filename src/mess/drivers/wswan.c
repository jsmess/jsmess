/***************************************************************************

  wswan.c

  Driver file to handle emulation of the Bandai WonderSwan
  By:

  Anthony Kruize
  Wilbert Pol

  Based on the WStech documentation by Judge and Dox.

  Known issues/TODOs:
  - Get the V30MZ core into MAME, still need to remove some nec specific
    instructions and fix the flags handling of the div/mul instructions.
  - Add support for noise sound
  - Add support for voice sound
  - Add support for enveloped sound
  - Perform video DMA at proper timing.
  - Add sound DMA.
  - Setup some reasonable values in the internal EEPROM area?
  - Add (real/proper) RTC support.
  - Look into timing issues like in Puzzle Bobble. VBlank interrupt lasts very long
    which causes sprites to be disabled until about 10%-40% of drawing the screen.
    The real unit seems to display things fine, need a real unit + real cart to
    verify.
  - Is background color setting really ok?
  - Get a dump of the internal BIOSes.
  - Swan Crystal can handle up to 512Mbit ROMs??????

- MameTesters bug 2808:
  Due to MAME breakage, if you need to use the debugger, you will need to
  modify src/emu/cpu/nec/necdasm.c, and comment out the 3 lines containing
  (Iconfig->v25v35_decryptiontable). The "debug" build will assert after
  about 20 instructions. You can comment out this assert check.

***************************************************************************/

#include "driver.h"
#include "cpu/v30mz/nec.h"
#include "includes/wswan.h"
#include "devices/cartslot.h"

static ADDRESS_MAP_START (wswan_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x00000, 0x03fff) AM_RAM		/* 16kb RAM / 4 colour tiles */
	AM_RANGE(0x04000, 0x0ffff) AM_NOP		/* nothing */
	AM_RANGE(0x10000, 0x1ffff) AM_READWRITE( wswan_sram_r, wswan_sram_w )	/* SRAM bank */
	AM_RANGE(0x20000, 0x2ffff) AM_ROMBANK(2)	/* ROM bank 1 */
	AM_RANGE(0x30000, 0x3ffff) AM_ROMBANK(3)	/* ROM bank 2 */
	AM_RANGE(0x40000, 0x4ffff) AM_ROMBANK(4)	/* ROM bank 3 */
	AM_RANGE(0x50000, 0x5ffff) AM_ROMBANK(5)	/* ROM bank 4 */
	AM_RANGE(0x60000, 0x6ffff) AM_ROMBANK(6)	/* ROM bank 5 */
	AM_RANGE(0x70000, 0x7ffff) AM_ROMBANK(7)	/* ROM bank 6 */
	AM_RANGE(0x80000, 0x8ffff) AM_ROMBANK(8)	/* ROM bank 7 */
	AM_RANGE(0x90000, 0x9ffff) AM_ROMBANK(9)	/* ROM bank 8 */
	AM_RANGE(0xA0000, 0xAffff) AM_ROMBANK(10)	/* ROM bank 9 */
	AM_RANGE(0xB0000, 0xBffff) AM_ROMBANK(11)	/* ROM bank 10 */
	AM_RANGE(0xC0000, 0xCffff) AM_ROMBANK(12)	/* ROM bank 11 */
	AM_RANGE(0xD0000, 0xDffff) AM_ROMBANK(13)	/* ROM bank 12 */
	AM_RANGE(0xE0000, 0xEffff) AM_ROMBANK(14)	/* ROM bank 13 */
	AM_RANGE(0xF0000, 0xFffff) AM_ROMBANK(15)	/* ROM bank 14 */
ADDRESS_MAP_END

static ADDRESS_MAP_START (wscolor_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x00000, 0x0ffff) AM_RAM		/* 16kb RAM / 4 colour tiles, 16 colour tiles + palettes */
	AM_RANGE(0x10000, 0x1ffff) AM_READWRITE( wswan_sram_r, wswan_sram_w )	/* SRAM bank */
	AM_RANGE(0x20000, 0x2ffff) AM_ROMBANK(2)	/* ROM bank 1 */
	AM_RANGE(0x30000, 0x3ffff) AM_ROMBANK(3)	/* ROM bank 2 */
	AM_RANGE(0x40000, 0x4ffff) AM_ROMBANK(4)	/* ROM bank 3 */
	AM_RANGE(0x50000, 0x5ffff) AM_ROMBANK(5)	/* ROM bank 4 */
	AM_RANGE(0x60000, 0x6ffff) AM_ROMBANK(6)	/* ROM bank 5 */
	AM_RANGE(0x70000, 0x7ffff) AM_ROMBANK(7)	/* ROM bank 6 */
	AM_RANGE(0x80000, 0x8ffff) AM_ROMBANK(8)	/* ROM bank 7 */
	AM_RANGE(0x90000, 0x9ffff) AM_ROMBANK(9)	/* ROM bank 8 */
	AM_RANGE(0xA0000, 0xAffff) AM_ROMBANK(10)	/* ROM bank 9 */
	AM_RANGE(0xB0000, 0xBffff) AM_ROMBANK(11)	/* ROM bank 10 */
	AM_RANGE(0xC0000, 0xCffff) AM_ROMBANK(12)	/* ROM bank 11 */
	AM_RANGE(0xD0000, 0xDffff) AM_ROMBANK(13)	/* ROM bank 12 */
	AM_RANGE(0xE0000, 0xEffff) AM_ROMBANK(14)	/* ROM bank 13 */
	AM_RANGE(0xF0000, 0xFffff) AM_ROMBANK(15)	/* ROM bank 14 */
ADDRESS_MAP_END

static ADDRESS_MAP_START (wswan_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x00, 0xff) AM_READWRITE( wswan_port_r, wswan_port_w )	/* I/O ports */
ADDRESS_MAP_END

static INPUT_PORTS_START( wswan )
	PORT_START("CURSX")		/* Cursors (X1-X4) */
	PORT_BIT( 0x1, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_NAME("X1 - Up")
	PORT_BIT( 0x4, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_NAME("X3 - Down")
	PORT_BIT( 0x8, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_NAME("X4 - Left")
	PORT_BIT( 0x2, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_NAME("X2 - Right")

	PORT_START("BUTTONS")	/* Buttons */
	PORT_BIT( 0x2, IP_ACTIVE_HIGH, IPT_START1) PORT_NAME("Start")
	PORT_BIT( 0x4, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Button A")
	PORT_BIT( 0x8, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("Button B")

	PORT_START("CURSY")		/* Cursors (Y1-Y4) */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Y1 - Left") PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Y3 - Right") PORT_CODE(KEYCODE_D)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Y4 - Down") PORT_CODE(KEYCODE_S)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Y2 - Up") PORT_CODE(KEYCODE_W)
INPUT_PORTS_END

static GFXDECODE_START( wswan )
GFXDECODE_END

/* WonderSwan can display 16 shades of grey */
static PALETTE_INIT( wswan )
{
	int ii;
	for( ii = 0; ii < 16; ii++ )
	{
		UINT8 shade = ii * (256 / 16);
		palette_set_color_rgb(machine,  15 - ii, shade, shade, shade );
	}
}

static PALETTE_INIT( wscolor ) {
	int i;
	for( i = 0; i < 4096; i++ ) {
		int r = ( i & 0x0F00 ) >> 8;
		int g = ( i & 0x00F0 ) >> 4;
		int b = i & 0x000F;
		palette_set_color_rgb(machine,  i, r << 4, g << 4, b << 4 );
	}
}

static MACHINE_DRIVER_START( wswan )
	/* Basic machine hardware */
	MDRV_CPU_ADD("maincpu", V30MZ, 3072000)
	MDRV_CPU_PROGRAM_MAP(wswan_mem)
	MDRV_CPU_IO_MAP(wswan_io)

	MDRV_SCREEN_ADD("screen", LCD)
	MDRV_SCREEN_REFRESH_RATE(75)
	MDRV_SCREEN_VBLANK_TIME(0)
	MDRV_QUANTUM_TIME(HZ(60))

	MDRV_NVRAM_HANDLER( wswan )

	MDRV_MACHINE_START( wswan )
	MDRV_MACHINE_RESET( wswan )

	MDRV_VIDEO_START( generic_bitmapped )
	MDRV_VIDEO_UPDATE( generic_bitmapped )

	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE( WSWAN_X_PIXELS, WSWAN_X_PIXELS )
	MDRV_SCREEN_VISIBLE_AREA(0*8, WSWAN_X_PIXELS - 1, 0, WSWAN_X_PIXELS - 1)
	MDRV_GFXDECODE(wswan)
	MDRV_PALETTE_LENGTH(16)
	MDRV_PALETTE_INIT(wswan)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")
	MDRV_SOUND_ADD("custom", WSWAN, 0)
	MDRV_SOUND_ROUTE(0, "lspeaker", 0.50)
	MDRV_SOUND_ROUTE(1, "rspeaker", 0.50)

	/* cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("ws,wsc,bin")
	MDRV_CARTSLOT_MANDATORY
	MDRV_CARTSLOT_START(wswan_cart)
	MDRV_CARTSLOT_LOAD(wswan_cart)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( wscolor )
	MDRV_IMPORT_FROM(wswan)
	MDRV_CPU_MODIFY("maincpu")
	MDRV_CPU_PROGRAM_MAP(wscolor_mem)
	MDRV_MACHINE_START( wscolor )
	MDRV_PALETTE_LENGTH(4096)
	MDRV_PALETTE_INIT( wscolor )
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( wswan )
	ROM_REGION( 0x100000, "maincpu", ROMREGION_ERASEFF )
//  ROM_LOAD_OPTIONAL( "ws_bios.bin", 0x0000, 0x0001, NO_DUMP )
ROM_END

ROM_START( wscolor )
	ROM_REGION( 0x100000, "maincpu", ROMREGION_ERASEFF )
//  ROM_LOAD_OPTIONAL( "wsc_bios.bin", 0x0000, 0x0001, NO_DUMP )
ROM_END

/*     YEAR  NAME     PARENT  COMPAT  MACHINE  INPUT  INIT  CONFIG  COMPANY   FULLNAME*/
CONS( 1999, wswan,   0,      0,      wswan,   wswan, 0,    0,  "Bandai", "WonderSwan",       GAME_IMPERFECT_SOUND )
CONS( 2000, wscolor, wswan,  0,      wscolor, wswan, 0,    0,  "Bandai", "WonderSwan Color", GAME_IMPERFECT_SOUND )

