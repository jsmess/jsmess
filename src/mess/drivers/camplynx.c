/***************************************************************************

      Camputers Lynx

      05/2009 Skeleton driver.

      The Lynx was an 8-bit British home computer that was first released 
      in early 1983 as a 48 kB model. 
      The designer of the Lynx was John Shireff and several models were 
      available with 48 kB, 96 kB (from Sep 1983) or 128 kB RAM (from Dec 
      1983). It was possible reach 192 kB with RAM expansions on-board.

      The machine was based around a Z80A CPU clocked at 4 MHz, and featured 
      a Motorola 6845 as video controller. It was possible to run CP/M with 
      the optional 5.25" floppy disk-drive on the 96 kB and 128 kB models.
      Approximately 30,000 Lynx units were sold world-wide.

      Camputers ceased trading in June 1984. Anston Technology took over in 
      November the same year and a re-launch was planned but never happened. 

      In June 1986, Anston sold everything - hardware, design rights and 
      thousands of cassettes - to the National Lynx User Group. The group 
      planned to produce a Super-Lynx but was too busy supplying spares and 
      technical information to owners of existing models, and the project never 
      came into being.

      Hardware info:
       - CPU: Zilog Z80A 4 MHz
       - CO-PROCESSOR: Motorola 6845 (CRT controller)
       - RAM: 48 kb, 96 kb or 128 kb depending on models (max. 192 kb)
       - ROM: 16 kb (48K version), 24 KB (96K and 128K versions)
       - TEXT MODES: 40 x 24, 80 x 24
       - GRAPHIC MODES: 256 x 248, 512 x 480
       - SOUND: one voice beeper

      Lynx 128 Memory Map

              | 0000    2000    4000    6000    8000    a000    c000     e000
              | 1fff    3fff    5fff    7fff    9fff    bfff    dfff     ffff
     -------------------------------------------------------------------------
              |                      |                        |       |
      Bank 0  |      BASIC ROM       |    Not Available       |  Ext  |  Ext
              |                      |                        |  ROM1 |  ROM2
     -------------------------------------------------------------------------
              |                      |   
      Bank 1  |        STORE         |           Workspace RAM
              |                      |    
     -------------------------------------------------------------------------
              |               |               |               |
      Bank 2  |       RED     |     BLUE      |     GREEN     |     Alt
              |               |               |               |    Green
     -------------------------------------------------------------------------
              |   
      Bank 3  |              Available for Video Expansion
              |   
     -------------------------------------------------------------------------
              |         
      Bank 4  |              Available for User RAM Expansion
              |       



	The work so far is without the benefit of manuals, schematic etc [Robbbert]

	To Do:
	128k, 96k: everything

	48k:
	- find Break key
	- colour
	- sound
	- banking
	- devices (cassette, disk)

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "video/mc6845.h"

static const device_config *mc6845;

static ADDRESS_MAP_START( camplynx_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000,0x5fff) AM_ROM
	AM_RANGE(0x6000,0xffff) AM_RAM
	AM_RANGE(0x8000,0x9fff) AM_BASE(&videoram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( camp96_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000,0x5fff) AM_ROM
	AM_RANGE(0x6000,0xdfff) AM_RAM
	AM_RANGE(0xe000,0xffff) AM_ROM
	AM_RANGE(0x8000,0x9fff) AM_BASE(&videoram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( camplynx_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x007f,0x007f) AM_MIRROR(0xff00) AM_WRITE(SMH_NOP)	/* these 2 seem to be */
	AM_RANGE(0x0080,0x0080) AM_MIRROR(0xff00) AM_WRITE(SMH_NOP)	/* for bankswitching */
	AM_RANGE(0x0080,0x0080) AM_READ_PORT("LINE0")
	AM_RANGE(0x0180,0x0180) AM_READ_PORT("LINE1")
	AM_RANGE(0x0280,0x0280) AM_READ_PORT("LINE2")
	AM_RANGE(0x0380,0x0380) AM_READ_PORT("LINE3")
	AM_RANGE(0x0480,0x0480) AM_READ_PORT("LINE4")
	AM_RANGE(0x0580,0x0580) AM_READ_PORT("LINE5")
	AM_RANGE(0x0680,0x0680) AM_READ_PORT("LINE6")
	AM_RANGE(0x0780,0x0780) AM_READ_PORT("LINE7")
	AM_RANGE(0x0880,0x0880) AM_READ_PORT("LINE8")
	AM_RANGE(0x0980,0x0980) AM_READ_PORT("LINE9")
	AM_RANGE(0x0084,0x0084) AM_MIRROR(0xff00) AM_NOP	/* unknown purpose */
	AM_RANGE(0x0086,0x0086) AM_MIRROR(0xff00) AM_DEVWRITE("crtc", mc6845_address_w)
	AM_RANGE(0x0087,0x0087) AM_MIRROR(0xff00) AM_DEVREADWRITE("crtc", mc6845_register_r,mc6845_register_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( camplynx )
	PORT_START("LINE0")
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Esc") PORT_CODE(KEYCODE_ESC) PORT_CHAR(27)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Up") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT(0x0e, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1 !") PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_START("LINE1")
	PORT_BIT(0xc0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C CONT") PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D DEL") PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X WEND") PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E DATA") PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4 $") PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3 #") PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_START("LINE2")
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Control") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A AUTO") PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S STOP") PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z RESTORE") PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W WHILE") PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q REM") PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2 \"") PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR(34)
	PORT_START("LINE3")
	PORT_BIT(0xc0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F DEFPROC") PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G GOTO") PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V VERIFY") PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T TRACE") PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R REPEAT") PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5 %") PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_START("LINE4")
	PORT_BIT(0xc0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B BEEP") PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N NEXT") PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(32)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H GOSUB") PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y RUN") PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6 &") PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_START("LINE5")
	PORT_BIT(0xd0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J LABEL") PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M RETURN") PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U UNTIL") PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8 (") PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7 \'") PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR(39)
	PORT_START("LINE6")
	PORT_BIT(0xd0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K MON") PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(", <") PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O ENDPROC") PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I INPUT") PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9 )") PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_START("LINE7")
	PORT_BIT(0xd0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("; +") PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(". >") PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L LIST") PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P PROC") PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0 _") PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_START("LINE8")
	PORT_BIT(0xd0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(": *") PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/ ?") PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("[") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("@ \\") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('@') PORT_CHAR('\\')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("- =") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('=')
	PORT_START("LINE9")
	PORT_BIT(0xc0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("End") PORT_CODE(KEYCODE_END) PORT_CHAR(UCHAR_MAMEKEY(END))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Return") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("]") PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Delete") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
INPUT_PORTS_END


static MACHINE_RESET( camplynx )
{
}

static MC6845_UPDATE_ROW( camplynx_update_row )
{
	UINT8 gfx,fg=1,bg=0;

	UINT16 x, *p = BITMAP_ADDR16(bitmap, y, 0);

	for (x = (y << 5); x < x_count + (y << 5); x++)
	{
		gfx = videoram[x];
			if (x == cursor_x) gfx^=0xff;	// not used - cursor is generated in software

			*p = ( gfx & 0x80 ) ? fg : bg; p++;
			*p = ( gfx & 0x40 ) ? fg : bg; p++;
			*p = ( gfx & 0x20 ) ? fg : bg; p++;
			*p = ( gfx & 0x10 ) ? fg : bg; p++;
			*p = ( gfx & 0x08 ) ? fg : bg; p++;
			*p = ( gfx & 0x04 ) ? fg : bg; p++;
			*p = ( gfx & 0x02 ) ? fg : bg; p++;
			*p = ( gfx & 0x01 ) ? fg : bg; p++;
	}
}

static VIDEO_START( camplynx )
{
	mc6845 = devtag_get_device(machine, "crtc");
}

static VIDEO_UPDATE( camplynx )
{
	mc6845_update(mc6845, bitmap, cliprect);
	return 0;
}


static const mc6845_interface camplynx_crtc6845_interface = {
	"screen",
	8,
	NULL,
	camplynx_update_row,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	NULL
};


static MACHINE_DRIVER_START( camplynx )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, XTAL_4MHz)
	MDRV_CPU_PROGRAM_MAP(camplynx_mem)
	MDRV_CPU_IO_MAP(camplynx_io)

	MDRV_MACHINE_RESET(camplynx)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(512, 480)
	MDRV_SCREEN_VISIBLE_AREA(0, 511, 0, 479)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_MC6845_ADD("crtc", MC6845, XTAL_12MHz / 8 /*? dot clock divided by dots per char */, camplynx_crtc6845_interface)

	MDRV_VIDEO_START(camplynx)
	MDRV_VIDEO_UPDATE(camplynx)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( camp96 )
	MDRV_IMPORT_FROM( camplynx )
	MDRV_CPU_MODIFY( "maincpu" )
	MDRV_CPU_PROGRAM_MAP(camp96_mem)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( camplynx )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "lynx48-1.rom", 0x0000, 0x2000, CRC(56feec44) SHA1(7ded5184561168e159a30fa8e9d3fde5e52aa91a) )
	ROM_LOAD( "lynx48-2.rom", 0x2000, 0x2000, CRC(d894562e) SHA1(c08a78ecb4eb05baa4c52488fce3648cd2688744) )
	ROM_FILL(0x8cf,1,0xc9)	// cheap rotten hack
ROM_END

ROM_START( camply96 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "lynx96-1.rom", 0x0000, 0x2000, CRC(56feec44) SHA1(7ded5184561168e159a30fa8e9d3fde5e52aa91a) )
	ROM_LOAD( "lynx96-2.rom", 0x2000, 0x2000, CRC(d894562e) SHA1(c08a78ecb4eb05baa4c52488fce3648cd2688744) )
	ROM_LOAD( "lynx96-3.rom", 0x4000, 0x2000, CRC(21f11709) SHA1(f86a729b01de286197c550974f7825c12815a4f4) )
	ROM_LOAD( "dosrom.rom", 0xe000, 0x2000, CRC(011e106a) SHA1(e77f0ca99790551a7122945f3194516b2390fb69) )
	ROM_FILL(0x8cf,1,0xc9)	// cheap rotten hack
ROM_END

ROM_START( camply128 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "lynx128-1.rom", 0x0000, 0x2000, CRC(65d292ce) SHA1(36567c2fbd9cf72f758e8cb80c21cb4d82040752) )
	ROM_LOAD( "lynx128-2.rom", 0x2000, 0x2000, CRC(23288773) SHA1(e12a7ebea3fae5eb375c03e848dbb81070d9d189) )
	ROM_LOAD( "lynx128-3.rom", 0x4000, 0x2000, CRC(9827b9e9) SHA1(1092367b2af51c72ce9be367179240d692aeb131) )
	ROM_LOAD( "dosrom.rom", 0xe000, 0x2000, CRC(011e106a) SHA1(e77f0ca99790551a7122945f3194516b2390fb69) )
ROM_END


static SYSTEM_CONFIG_START(camp48)
	CONFIG_RAM_DEFAULT(48 * 1024)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(camp96)
	CONFIG_RAM_DEFAULT(96 * 1024)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(camp128)
	CONFIG_RAM_DEFAULT(128 * 1024)
SYSTEM_CONFIG_END


/* Driver */
/*    YEAR  NAME       PARENT     COMPAT   MACHINE    INPUT     INIT  CONFIG    COMPANY       FULLNAME     FLAGS */
COMP( 1983, camplynx,  0,         0,       camplynx,  camplynx, 0,    camp48,   "Camputers",  "Lynx 48",   GAME_NOT_WORKING)
COMP( 1983, camply96,  camplynx,  0,       camp96,    camplynx, 0,    camp96,   "Camputers",  "Lynx 96",   GAME_NOT_WORKING)
COMP( 1983, camply128, camplynx,  0,       camp96,    camplynx, 0,    camp128,  "Camputers",  "Lynx 128",  GAME_NOT_WORKING)
