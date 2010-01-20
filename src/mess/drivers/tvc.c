/***************************************************************************

        Videotone TVC 32/64 driver

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "video/mc6845.h"
#include "devices/messram.h"

static UINT8 tvc_video_mode = 0;
static UINT8 tvc_keyline = 0;
static UINT8 tvc_flipflop = 0xff;

static void tvc_set_mem_page(running_machine *machine, UINT8 data)
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	switch(data & 0x18) {
		case 0x00 : // system ROM selected
				memory_install_read_bank(space, 0x0000, 0x3fff, 0, 0, "bank1");
				memory_unmap_write(space, 0x0000, 0x3fff, 0, 0);
				memory_set_bankptr(space->machine, "bank1", memory_region(machine, "sys"));
				break;
		case 0x08 : // Cart ROM selected
				memory_install_read_bank(space, 0x0000, 0x3fff, 0, 0, "bank1");
				memory_unmap_write(space, 0x0000, 0x3fff, 0, 0);
				memory_set_bankptr(space->machine, "bank1", memory_region(machine, "cart"));
				break;
		case 0x10 : // RAM selected
				memory_install_readwrite_bank(space, 0x0000, 0x3fff, 0, 0, "bank1");
				memory_set_bankptr(space->machine, "bank1", messram_get_ptr(devtag_get_device(machine, "messram")));
				break;
	}
	// Bank 2 is always RAM
	memory_set_bankptr(space->machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x4000);
	if ((data & 0x20)==0) {
		// Video RAM
		memory_set_bankptr(space->machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x10000);
	} else {
		// System RAM page 3
		memory_set_bankptr(space->machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x8000);
	}
	switch(data & 0xc0) {
		case 0x00 : // Cart ROM selected
				memory_install_read_bank(space, 0xc000, 0xffff, 0, 0, "bank4");
				memory_unmap_write(space, 0xc000, 0xffff, 0, 0);
				memory_set_bankptr(space->machine, "bank4", memory_region(machine, "cart"));
				break;
		case 0x40 : // System ROM selected
				memory_install_read_bank(space, 0xc000, 0xffff, 0, 0, "bank4");
				memory_unmap_write(space, 0xc000, 0xffff, 0, 0);
				memory_set_bankptr(space->machine, "bank4", memory_region(machine, "sys"));
				break;
		case 0x80 : // RAM selected
				memory_install_readwrite_bank(space, 0xc000, 0xffff, 0, 0, "bank4");
				memory_set_bankptr(space->machine, "bank4", messram_get_ptr(devtag_get_device(machine, "messram"))+0xc000);
				break;
		case 0xc0 : // External ROM selected
				memory_install_read_bank(space, 0xc000, 0xffff, 0, 0, "bank4");
				memory_unmap_write(space, 0xc000, 0xffff, 0, 0);
				memory_set_bankptr(space->machine, "bank4", memory_region(machine, "ext"));
				break;

	}
}

static WRITE8_HANDLER( tvc_bank_w )
{
	tvc_set_mem_page(space->machine, data);
}

static WRITE8_HANDLER( tvc_video_mode_w )
{
	tvc_video_mode = data & 0x03;
}

static UINT8 col[4]= {0,1,2,3};

static WRITE8_HANDLER( tvc_palette_w )
{
	//  0 I 0 R | 0 G 0 B
	//  0 0 0 0 | I R G B
	int i = ((data&0x40)>>3) | ((data&0x10)>>2) | ((data&0x04)>>1) | (data&0x01);

	col[offset] = i;
}

static WRITE8_HANDLER( tvc_keyboard_w )
{
	tvc_keyline = data;
}

static READ8_HANDLER( tvc_keyboard_r )
{
	static const char *const keynames[] = {
		"LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7",
		"LINE8", "LINE9", "LINEA", "LINEB", "LINEC", "LINED", "LINEE", "LINEF"
	};
	return input_port_read(space->machine, keynames[tvc_keyline & 0x0f]);
}

static READ8_HANDLER( tvc_flipflop_r )
{
	return tvc_flipflop;
}

static WRITE8_HANDLER( tvc_flipflop_w )
{
	tvc_flipflop |= 0x10;
}
static READ8_HANDLER( tvc_port59_r )
{
	return 0xff;
}

static WRITE8_HANDLER( tvc_port0_w )
{
}
static ADDRESS_MAP_START(tvc_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_RAMBANK("bank1")
	AM_RANGE(0x4000, 0x7fff) AM_RAMBANK("bank2")
	AM_RANGE(0x8000, 0xbfff) AM_RAMBANK("bank3")
	AM_RANGE(0xc000, 0xffff) AM_RAMBANK("bank4")
ADDRESS_MAP_END

static ADDRESS_MAP_START( tvc_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_WRITE(tvc_port0_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(tvc_bank_w)
	AM_RANGE(0x03, 0x03) AM_WRITE(tvc_keyboard_w)
	AM_RANGE(0x06, 0x06) AM_WRITE(tvc_video_mode_w)
	AM_RANGE(0x07, 0x07) AM_WRITE(tvc_flipflop_w)
	AM_RANGE(0x58, 0x58) AM_READ(tvc_keyboard_r)
	AM_RANGE(0x59, 0x59) AM_READ(tvc_flipflop_r)
	AM_RANGE(0x5a, 0x5a) AM_READ(tvc_port59_r)
	AM_RANGE(0x60, 0x64) AM_WRITE(tvc_palette_w)
	AM_RANGE(0x70, 0x70) AM_DEVWRITE("crtc", mc6845_address_w)
	AM_RANGE(0x71, 0x71) AM_DEVREADWRITE("crtc", mc6845_register_r , mc6845_register_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( tvc )
	PORT_START("LINE0")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I'") PORT_CODE(KEYCODE_TILDE)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)
	PORT_START("LINE1")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("^") PORT_CODE(KEYCODE_TILDE)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("u''") PORT_CODE(KEYCODE_TILDE)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("*") PORT_CODE(KEYCODE_TILDE)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O'") PORT_CODE(KEYCODE_TILDE)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O''") PORT_CODE(KEYCODE_TILDE)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)
	PORT_START("LINE2")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(";") PORT_CODE(KEYCODE_COLON)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("@") PORT_CODE(KEYCODE_TILDE)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)
	PORT_START("LINE3")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("]") PORT_CODE(KEYCODE_CLOSEBRACE)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("?") PORT_CODE(KEYCODE_SLASH)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("[") PORT_CODE(KEYCODE_OPENBRACE)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U'") PORT_CODE(KEYCODE_TILDE)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)
	PORT_START("LINE4")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\\") PORT_CODE(KEYCODE_BACKSLASH)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("<") PORT_CODE(KEYCODE_TILDE)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
	PORT_START("LINE5")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Backspace") PORT_CODE(KEYCODE_BACKSPACE)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A'") PORT_CODE(KEYCODE_TILDE)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Enter") PORT_CODE(KEYCODE_ENTER)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("u''") PORT_CODE(KEYCODE_TILDE)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("e'") PORT_CODE(KEYCODE_TILDE)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)
	PORT_START("LINE6")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_RSHIFT) PORT_CODE(KEYCODE_LSHIFT)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Lock") PORT_CODE(KEYCODE_CAPSLOCK)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)
	PORT_START("LINE7")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Alt") PORT_CODE(KEYCODE_RALT) PORT_CODE(KEYCODE_LALT)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(",") PORT_CODE(KEYCODE_COMMA)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(".") PORT_CODE(KEYCODE_STOP)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Esc") PORT_CODE(KEYCODE_ESC)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Ctrl") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("=") PORT_CODE(KEYCODE_EQUALS)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)
	PORT_START("LINE8")
		PORT_BIT(0xFF, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START("LINE9")
		PORT_BIT(0xFF, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START("LINEA")
		PORT_BIT(0xFF, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START("LINEB")
		PORT_BIT(0xFF, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START("LINEC")
		PORT_BIT(0xFF, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START("LINED")
		PORT_BIT(0xFF, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START("LINEE")
		PORT_BIT(0xFF, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START("LINEF")
		PORT_BIT(0xFF, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END


static MACHINE_RESET(tvc)
{
	memset(messram_get_ptr(devtag_get_device(machine, "messram")),0,(64+14)*1024);
	tvc_set_mem_page(machine, 0);
	tvc_video_mode = 0;
}

static VIDEO_START( tvc )
{
}

static VIDEO_UPDATE( tvc )
{
	const device_config *mc6845 = devtag_get_device(screen->machine, "crtc");
	mc6845_update(mc6845, bitmap, cliprect);
	return 0;
}

static MC6845_UPDATE_ROW( tvc_update_row )
{
	UINT16  *p = BITMAP_ADDR16(bitmap, y, 0);
	int i;

	switch(tvc_video_mode) {
		case 0 :
				for ( i = 0; i < x_count; i++ )
				{
					UINT16 offset = i  + (y * 64);
					UINT8 data = messram_get_ptr(devtag_get_device(device->machine, "messram"))[ offset + 0x10000];
					*p = col[(data >> 7)]; p++;
					*p = col[(data >> 6)]; p++;
					*p = col[(data >> 5)]; p++;
					*p = col[(data >> 4)]; p++;
					*p = col[(data >> 3)]; p++;
					*p = col[(data >> 2)]; p++;
					*p = col[(data >> 1)]; p++;
					*p = col[(data >> 0)]; p++;
				}
				break;
		case 1 :
				for ( i = 0; i < x_count; i++ )
				{
					UINT16 offset = i  + (y * 64);
					UINT8 data = messram_get_ptr(devtag_get_device(device->machine, "messram"))[ offset + 0x10000];
					*p = col[BIT(data,7)*2 + BIT(data,3)]; p++;
					*p = col[BIT(data,7)*2 + BIT(data,3)]; p++;
					*p = col[BIT(data,6)*2 + BIT(data,2)]; p++;
					*p = col[BIT(data,6)*2 + BIT(data,2)]; p++;
					*p = col[BIT(data,5)*2 + BIT(data,1)]; p++;
					*p = col[BIT(data,5)*2 + BIT(data,1)]; p++;
					*p = col[BIT(data,4)*2 + BIT(data,0)]; p++;
					*p = col[BIT(data,4)*2 + BIT(data,0)]; p++;
				}
				break;
		default:
				for ( i = 0; i < x_count; i++ )
				{
					UINT16 offset = i  + (y * 64);
					UINT8 data = messram_get_ptr(devtag_get_device(device->machine, "messram"))[ offset + 0x10000];
					*p = col[(data >> 4) & 0xf]; p++;
					*p = col[(data >> 4) & 0xf]; p++;
					*p = col[(data >> 4) & 0xf]; p++;
					*p = col[(data >> 4) & 0xf]; p++;
					*p = col[(data >> 0) & 0xf]; p++;
					*p = col[(data >> 0) & 0xf]; p++;
					*p = col[(data >> 0) & 0xf]; p++;
					*p = col[(data >> 0) & 0xf]; p++;
				}
				break;

	}
}

static PALETTE_INIT( tvc )
{
	const static unsigned char tvc_palette[16][3] =
	{
		{ 0x00,0x00,0x00 },
		{ 0x00,0x00,0x7f },
		{ 0x00,0x7f,0x00 },
		{ 0x00,0x7f,0x7f },
		{ 0x7f,0x00,0x00 },
		{ 0x7f,0x00,0x7f },
		{ 0x7f,0x7f,0x00 },
		{ 0x7f,0x7f,0x7f },
		{ 0x00,0x00,0x00 },
		{ 0x00,0x00,0xff },
		{ 0x00,0xff,0x00 },
		{ 0x00,0xff,0xff },
		{ 0xff,0x00,0x00 },
		{ 0xff,0x00,0xff },
		{ 0xff,0xff,0x00 },
		{ 0xff,0xff,0xff }
	};
	int i;

	for(i = 0; i < 16; i++)
		palette_set_color_rgb(machine, i, tvc_palette[i][0], tvc_palette[i][1], tvc_palette[i][2]);
}

static const mc6845_interface tvc_crtc6845_interface =
{
	"screen",
	8 /*?*/,
	NULL,
	tvc_update_row,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	NULL
};

static INTERRUPT_GEN( tvc_interrupt )
{
	tvc_flipflop  &= ~0x10;
	cpu_set_input_line(device, 0, HOLD_LINE);
}

static MACHINE_DRIVER_START( tvc )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, 3125000)
    MDRV_CPU_PROGRAM_MAP(tvc_mem)
    MDRV_CPU_IO_MAP(tvc_io)

    MDRV_MACHINE_RESET(tvc)

	MDRV_CPU_VBLANK_INT("screen", tvc_interrupt)

 /* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(512, 240)
	MDRV_SCREEN_VISIBLE_AREA(0, 512 - 1, 0, 240 - 1)
	MDRV_PALETTE_LENGTH( 16 )
	MDRV_PALETTE_INIT(tvc)

	MDRV_MC6845_ADD("crtc", MC6845, 3125000, tvc_crtc6845_interface) // clk taken from schematics

    MDRV_VIDEO_START(tvc)
    MDRV_VIDEO_UPDATE(tvc)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("80K")
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( tvc64 )
    ROM_REGION( 0x4000, "sys", ROMREGION_ERASEFF )
	ROM_LOAD( "tvc12_d4.64k", 0x0000, 0x2000, CRC(834ca9be) SHA1(c333318c1c6185aae2d3dfb86d55e3a4a3071a73))
	ROM_LOAD( "tvc12_d3.64k", 0x2000, 0x2000, CRC(71753d02) SHA1(d9a1905cf55c532b3380c83158fb5254ee503829))
	ROM_REGION( 0x4000, "cart", ROMREGION_ERASEFF )
	ROM_REGION( 0x4000, "ext", ROMREGION_ERASEFF )
	ROM_LOAD( "tvc12_d7.64k", 0x2000, 0x2000, CRC(1cbbeac6) SHA1(54b29c9ca9942f04620fbf3edab3b8e3cd21c194))
ROM_END

ROM_START( tvc64p )
	ROM_REGION( 0x4000, "sys", ROMREGION_ERASEFF )
	ROM_LOAD( "tvc22_d6.64k", 0x0000, 0x2000, CRC(05ac3a34) SHA1(bdc7eda5fd53f806dca8c4929ee498e8e59eb787))
	ROM_LOAD( "tvc22_d4.64k", 0x2000, 0x2000, CRC(ba6ad589) SHA1(e5c8a6db506836a327d901387a8dc8c681a272db))
	ROM_REGION( 0x4000, "cart", ROMREGION_ERASEFF )
	ROM_REGION( 0x4000, "ext", ROMREGION_ERASEFF )
	ROM_LOAD( "tvc22_d7.64k", 0x2000, 0x2000, CRC(05e1c3a8) SHA1(abf119cf947ea32defd08b29a8a25d75f6bd4987))
/*

    ROM_LOAD( "tvcru_d4.bin", 0x0000, 0x2000, CRC(bac5dd4f) SHA1(665a1b8c80b6ad82090803621f0c73ef9243c7d4))
    ROM_LOAD( "tvcru_d6.bin", 0x2000, 0x2000, CRC(1e0fa0b8) SHA1(9bebb6c8f03f9641bd35c9fd45ffc13a48e5c572))
    ROM_LOAD( "tvcru_d7.bin", 0x2000, 0x2000, CRC(70cde756) SHA1(c49662af9f6653347ead641e85777c3463cc161b))
*/
ROM_END

ROM_START( tvc64pru )
	ROM_REGION( 0x4000, "sys", ROMREGION_ERASEFF )
	ROM_LOAD( "tvcru_d6.bin", 0x0000, 0x2000, CRC(1e0fa0b8) SHA1(9bebb6c8f03f9641bd35c9fd45ffc13a48e5c572))
	ROM_LOAD( "tvcru_d4.bin", 0x2000, 0x2000, CRC(bac5dd4f) SHA1(665a1b8c80b6ad82090803621f0c73ef9243c7d4))
	ROM_REGION( 0x4000, "cart", ROMREGION_ERASEFF )
	ROM_REGION( 0x4000, "ext", ROMREGION_ERASEFF )
	ROM_LOAD( "tvcru_d7.bin", 0x2000, 0x2000, CRC(70cde756) SHA1(c49662af9f6653347ead641e85777c3463cc161b))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT COMPANY   FULLNAME       FLAGS */
COMP( 1985, tvc64,  0,   	 0, 	tvc, 	tvc, 	 0,  	  "Videoton",   "TVC 64",		GAME_NOT_WORKING)
COMP( 1985, tvc64p, tvc64,   0, 	tvc, 	tvc, 	 0,  	  "Videoton",   "TVC 64+",		GAME_NOT_WORKING)
COMP( 1985, tvc64pru,tvc64,   0, 	tvc, 	tvc, 	 0,  	  "Videoton",   "TVC 64+ (Russian)",		GAME_NOT_WORKING)

