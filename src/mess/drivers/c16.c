/***************************************************************************
    commodore c16 home computer

    PeT mess@utanet.at

    documentation
     www.funet.fi

***************************************************************************/

/*

2008 - Driver Updates
---------------------

(most of the informations are taken from http://www.zimmers.net/cbmpics/ )


[CBM systems which belong to this driver]

* Commodore 116 (1984, Europe only)

  Entry level computer of the Commodore 264 family, it was marketed only
in Europe. It was impressive for the small size of its case, but it didn't
meet commercial success

CPU: MOS Technology 7501 (variable clock rate, with max 1.76 MHz)
RAM: 16 kilobytes (expandable to 64k internally)
ROM: 32 kilobytes
Video: MOS Technology 7360 "TED" (5 Video modes; Max. Resolution 320x200;
    40 columns text; Palette of 16 colors in 8 shades, for 128 colors)
Sound: MOS Technology 7360 "TED" (2 voice tone-generating sound capabilities)
Ports: MOS 7360 (2 Joystick/Mouse ports; CBM Serial port; 'TED' port; "TV"
    Port and switch; CBM Monitor port; Power and reset switches; Power
    connector)
Keyboard: QWERTY 62 key "membrane" (8 programmable function keys; 4 direction
    cursor-pad)


* Commodore 16 (1984)

  Redesigned version of the C116, with a different case and a different
keyboard.

CPU: MOS Technology 7501 (variable clock rate, with max 1.76 MHz)
RAM: 16 kilobytes (expandable to 64k internally)
ROM: 32 kilobytes
Video: MOS Technology 7360 "TED" (5 Video modes; Max. Resolution 320x200;
    40 columns text; Palette of 16 colors in 8 shades, for 128 colors)
Sound: MOS Technology 7360 "TED" (2 voice tone-generating sound capabilities)
Ports: MOS 7360 (2 Joystick/Mouse ports; CBM Serial port; 'TED' port; "TV"
    Port and switch; CBM Monitor port; Power and reset switches; Power
    connector)
Keyboard: QWERTY 66 key typewriter style (8 programmable function keys;
    4 direction cursor-pad)


* Commodore Plus/4 (1984)

  This system became the middle tier of the Commodore 264 family, replacing
the original Commodore 264. The Plus/4 is basically the same as the C264,
but the name refers to the four built-in programs which came with the
machine: Word Processing, Spreadsheet, Database software, Graphing package.

CPU: MOS Technology 7501 (variable clock rate, with max 1.76 MHz)
RAM: 64 kilobytes (expandable to 64k internally)
ROM: 64 kilobytes
Video: MOS Technology 7360 "TED" (5 Video modes; Max. Resolution 320x200;
    40 columns text; Palette of 16 colors in 8 shades, for 128 colors)
Sound: MOS Technology 7360 "TED" (2 voice tone-generating sound capabilities)
Ports: MOS 7360 (2 Joystick/Mouse ports; CBM Serial port; 'TED' port; "TV"
    Port and switch; CBM Monitor port; Power and reset switches; Power
    connector)
Keyboard: Full-sized QWERTY 67 key (8 programmable function keys;
    4 direction cursor-pad)


* Commodore 232 (1984, Prototype)

  This system never reached the production and only few units exist. It is
in between the C16 and the C264, with its 32 kilobytes of RAM.


* Commodore 264 (1984, Prototype)

  Basically the same of a Plus/4 but without the built-in programs.


* Commodore V364 (1984, Prototype)

  This system was supposed to become the high-end system of the family,
featuring 64 kilobytes of RAM, the same technology of the Plus/4, a
keyboard with numeric keypad and built in voice synthesis capabilities.

[TO DO]

* Supported Systems:

- Once we can add / remove devices, we shall only support c16, c116 and plus/4,
removing the separated drivers for different floppy drives

* Floppy drives:

- Drives 8 & 9 supported, but limited compatibility. Real disk emulation is
needed.

* Other Peripherals:

- Lightpen support is unfinished
- Missing support for (it might or might not be added eventually):
printers and other devices; most expansion modules; userport; rs232/v.24 interface.

* System Specific

- V364 lacks speech hardware emulation

*/


#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "sound/sid6581.h"

#include "machine/6525tpi.h"
#include "includes/vc1541.h"
#include "machine/cbmipt.h"
#include "video/ted7360.h"
#include "devices/messram.h"

/* devices config */
#include "includes/cbm.h"
#include "includes/cbmdrive.h"
#include "includes/cbmserb.h"

#include "includes/c16.h"

/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/



/*
 * commodore c16/c116/plus 4
 * 16 KByte (C16/C116) or 32 KByte or 64 KByte (plus4) RAM
 * 32 KByte Rom (C16/C116) 64 KByte Rom (plus4)
 * availability to append additional 64 KByte Rom
 *
 * ports 0xfd00 till 0xff3f are always read/writeable for the cpu
 * for the video interface chip it seams to read from
 * ram or from rom in this  area
 *
 * writes go always to ram
 * only 16 KByte Ram mapped to 0x4000,0x8000,0xc000
 * only 32 KByte Ram mapped to 0x8000
 *
 * rom bank at 0x8000: 16K Byte(low bank)
 * first: basic
 * second(plus 4 only): plus4 rom low
 * third: expansion slot
 * fourth: expansion slot
 * rom bank at 0xc000: 16K Byte(high bank)
 * first: kernal
 * second(plus 4 only): plus4 rom high
 * third: expansion slot
 * fourth: expansion slot
 * writes to 0xfddx select rom banks:
 * address line 0 and 1: rom bank low
 * address line 2 and 3: rom bank high
 *
 * writes to 0xff3e switches to roms (0x8000 till 0xfd00, 0xff40 till 0xffff)
 * writes to 0xff3f switches to rams
 *
 * at 0xfc00 till 0xfcff is ram or rom kernal readable
 */

static ADDRESS_MAP_START(c16_map, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_RAMBANK("bank9")
	AM_RANGE(0x4000, 0x7fff) AM_READ_BANK("bank1") AM_WRITE_BANK("bank5")	   /* only ram memory configuration */
	AM_RANGE(0x8000, 0xbfff) AM_READ_BANK("bank2") AM_WRITE_BANK("bank6")
	AM_RANGE(0xc000, 0xfbff) AM_READ_BANK("bank3")
	AM_RANGE(0xfc00, 0xfcff) AM_READ_BANK("bank4")
	AM_RANGE(0xc000, 0xfcff) AM_WRITE_BANK("bank7")
	AM_RANGE(0xfd10, 0xfd1f) AM_READ(c16_fd1x_r)
	AM_RANGE(0xfd30, 0xfd3f) AM_READWRITE(c16_6529_port_r, c16_6529_port_w)		/* 6529 keyboard matrix */
	AM_RANGE(0xfdd0, 0xfddf) AM_WRITE(c16_select_roms) /* rom chips selection */
	AM_RANGE(0xff00, 0xff1f) AM_READWRITE(ted7360_port_r, ted7360_port_w)
	AM_RANGE(0xff20, 0xffff) AM_READ_BANK("bank8")
	AM_RANGE(0xff3e, 0xff3e) AM_WRITE(c16_switch_to_rom)
	AM_RANGE(0xff3f, 0xff3f) AM_WRITE(c16_switch_to_ram)
#if 0
	AM_RANGE(0x4000, 0x7fff) AM_WRITE( c16_write_4000)  /*configured in c16_common_init */
	AM_RANGE(0x8000, 0xbfff) AM_WRITE( c16_write_8000)  /*configured in c16_common_init */
	AM_RANGE(0xc000, 0xfcff) AM_WRITE( c16_write_c000)  /*configured in c16_common_init */
	AM_RANGE(0xfd40, 0xfd5f) AM_DEVREADWRITE("sid6581", sid6581_r, sid6581_w)	/* sidcard, eoroidpro ... */
	AM_RANGE(0xfec0, 0xfedf) AM_READWRITE(c16_iec9_port_r, c16_iec9_port_w)		/*configured in c16_common_init */
	AM_RANGE(0xfee0, 0xfeff) AM_READWRITE(c16_iec8_port_r, c16_iec8_port_w)		/*configured in c16_common_init */
	AM_RANGE(0xff20, 0xff3d) AM_WRITE( c16_write_ff20)  /*configure in c16_common_init */
	AM_RANGE(0xff40, 0xffff) AM_WRITE( c16_write_ff40)  /*configure in c16_common_init */
#endif
ADDRESS_MAP_END

static ADDRESS_MAP_START(plus4_map, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x7fff) AM_READ_BANK("bank9")
	AM_RANGE(0x8000, 0xbfff) AM_READ_BANK("bank2")
	AM_RANGE(0xc000, 0xfbff) AM_READ_BANK("bank3")
	AM_RANGE(0xfc00, 0xfcff) AM_READ_BANK("bank4")
	AM_RANGE(0x0000, 0xfcff) AM_WRITE_BANK("bank9")
	AM_RANGE(0xfd00, 0xfd0f) AM_READWRITE(c16_6551_port_r, c16_6551_port_w)
	AM_RANGE(0xfd10, 0xfd1f) AM_READWRITE(plus4_6529_port_r, plus4_6529_port_w)
	AM_RANGE(0xfd30, 0xfd3f) AM_READWRITE(c16_6529_port_r, c16_6529_port_w) /* 6529 keyboard matrix */
	AM_RANGE(0xfdd0, 0xfddf) AM_WRITE( c16_select_roms) /* rom chips selection */
#if 0
	AM_RANGE(0xfd40, 0xfd5f) AM_DEVREADWRITE("sid6581", sid6581_r, sid6581_w)	/* sidcard, eoroidpro ... */
	AM_RANGE(0xfec0, 0xfedf) AM_READWRITE(c16_iec9_port_r, c16_iec9_port_w)		/*configured in c16_common_init */
	AM_RANGE(0xfee0, 0xfeff) AM_READWRITE(c16_iec8_port_r, c16_iec8_port_w)		/*configured in c16_common_init */
#endif
	AM_RANGE(0xff00, 0xff1f) AM_READWRITE(ted7360_port_r, ted7360_port_w)
	AM_RANGE(0xff20, 0xffff) AM_READ_BANK("bank8")
	AM_RANGE(0xff20, 0xff3d) AM_WRITEONLY
	AM_RANGE(0xff3e, 0xff3e) AM_WRITE(c16_switch_to_rom)
	AM_RANGE(0xff3f, 0xff3f) AM_WRITE(c16_switch_to_ram)
	AM_RANGE(0xff40, 0xffff) AM_WRITEONLY
ADDRESS_MAP_END

static ADDRESS_MAP_START(c364_map , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x7fff) AM_READ_BANK("bank9")
	AM_RANGE(0x8000, 0xbfff) AM_READ_BANK("bank2")
	AM_RANGE(0xc000, 0xfbff) AM_READ_BANK("bank3")
	AM_RANGE(0xfc00, 0xfcff) AM_READ_BANK("bank4")
	AM_RANGE(0x0000, 0xfcff) AM_WRITE_BANK("bank9")
	AM_RANGE(0xfd00, 0xfd0f) AM_READWRITE(c16_6551_port_r, c16_6551_port_w)
	AM_RANGE(0xfd10, 0xfd1f) AM_READWRITE(plus4_6529_port_r, plus4_6529_port_w)
	AM_RANGE(0xfd20, 0xfd2f) AM_READWRITE(c364_speech_r, c364_speech_w)
	AM_RANGE(0xfd30, 0xfd3f) AM_READWRITE(c16_6529_port_r, c16_6529_port_w) /* 6529 keyboard matrix */
	AM_RANGE(0xfdd0, 0xfddf) AM_WRITE( c16_select_roms) /* rom chips selection */
#if 0
	AM_RANGE(0xfd40, 0xfd5f) AM_DEVREADWRITE("sid6581", sid6581_r, sid6581_w)	/* sidcard, eoroidpro ... */
	AM_RANGE(0xfec0, 0xfedf) AM_READWRITE(c16_iec9_port_r, c16_iec9_port_w)		/*configured in c16_common_init */
	AM_RANGE(0xfee0, 0xfeff) AM_READWRITE(c16_iec8_port_r, c16_iec8_port_w)		/*configured in c16_common_init */
#endif
	AM_RANGE(0xff00, 0xff1f) AM_READWRITE(ted7360_port_r, ted7360_port_w)
	AM_RANGE(0xff20, 0xffff) AM_READ_BANK("bank8")
	AM_RANGE(0xff20, 0xff3d) AM_WRITEONLY
	AM_RANGE(0xff3e, 0xff3e) AM_WRITE(c16_switch_to_rom)
	AM_RANGE(0xff3f, 0xff3f) AM_WRITE(c16_switch_to_ram)
	AM_RANGE(0xff40, 0xffff) AM_WRITEONLY
ADDRESS_MAP_END


/*************************************
 *
 *  Input Ports
 *
 *************************************/

static INPUT_PORTS_START( c16 )
	PORT_INCLUDE( common_cbm_keyboard )		/* ROW0 -> ROW7 */

	PORT_MODIFY("ROW0")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("@") PORT_CODE(KEYCODE_OPENBRACE)				PORT_CHAR('@')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F3)									PORT_CHAR(UCHAR_MAMEKEY(F3)) PORT_CHAR(UCHAR_MAMEKEY(F6))
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F2)									PORT_CHAR(UCHAR_MAMEKEY(F2)) PORT_CHAR(UCHAR_MAMEKEY(F5))
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F1)									PORT_CHAR(UCHAR_MAMEKEY(F1)) PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("HELP f7") PORT_CODE(KEYCODE_F4)				PORT_CHAR(UCHAR_MAMEKEY(F8)) PORT_CHAR(UCHAR_MAMEKEY(F7))
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_INSERT)								PORT_CHAR(0xA3)

	PORT_MODIFY("ROW1")
	/* Both Shift keys were mapped to the same bit */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Shift (Left & Right)") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT)

	PORT_MODIFY("ROW4")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("0  \xE2\x86\x91") PORT_CODE(KEYCODE_0)		PORT_CHAR('0') PORT_CHAR(0x2191)

	PORT_MODIFY("ROW5")
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS)					PORT_CHAR('-')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH2)							PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_PGUP)									PORT_CHAR(UCHAR_MAMEKEY(DOWN))

	PORT_MODIFY("ROW6")
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE)							PORT_CHAR('+')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("=  Pi  \xE2\x86\x90") PORT_CODE(KEYCODE_PGDN)	PORT_CHAR('=') PORT_CHAR(0x03C0) PORT_CHAR(0x2190)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_ESC)									PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS)								PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH)								PORT_CHAR('*')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS)									PORT_CHAR(UCHAR_MAMEKEY(LEFT))

	PORT_MODIFY("ROW7")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Home Clear") PORT_CODE(KEYCODE_DEL)			PORT_CHAR(UCHAR_MAMEKEY(HOME))

	PORT_INCLUDE( c16_special )				/* SPECIAL */

	PORT_INCLUDE( c16_controls )			/* JOY0, JOY1 */

	/* no real floppy */

	PORT_INCLUDE( c16_config )				/* DSW0, CFG1 */
INPUT_PORTS_END


static INPUT_PORTS_START( plus4 )
	PORT_INCLUDE( c16 )

	/* no real floppy */

	PORT_MODIFY( "ROW0" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE)					PORT_CHAR(0xA3)
	PORT_MODIFY( "ROW5" )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS)						PORT_CHAR('-')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_UP)							PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_DOWN)							PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_MODIFY( "ROW6" )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS)							PORT_CHAR('+')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("=  Pi  \xE2\x86\x90") PORT_CODE(KEYCODE_BACKSLASH2)	PORT_CHAR('=') PORT_CHAR(0x03C0) PORT_CHAR(0x2190)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_RIGHT)							PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_INSERT)						PORT_CHAR('*')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_LEFT)							PORT_CHAR(UCHAR_MAMEKEY(LEFT))

	PORT_MODIFY("CFG1")
	PORT_BIT( 0x10, 0x10, IPT_UNUSED )			/* ntsc */
	PORT_BIT( 0x0c, 0x04, IPT_UNUSED )			/* plus4 */
INPUT_PORTS_END



#if 0
static INPUT_PORTS_START (c364)
	PORT_INCLUDE( plus4 )

	/* no real floppy */

	PORT_MODIFY("CFG1")
	PORT_BIT( 0x10, 0x10, IPT_UNUSED )			/* ntsc */
	PORT_BIT( 0x0c, 0x08, IPT_UNUSED )			/* 364 */
	/* numeric block - hardware wired to other keys? */
	/* check cbmipt.c for layout */
INPUT_PORTS_END
#endif


/*************************************
 *
 *  Graphics definitions
 *
 *************************************/


static PALETTE_INIT( c16 )
{
	int i;

	for ( i = 0; i < sizeof(ted7360_palette) / 3; i++ ) {
		palette_set_color_rgb(machine, i, ted7360_palette[i*3], ted7360_palette[i*3+1], ted7360_palette[i*3+2]);
	}
}




/*************************************
 *
 *  Machine driver
 *
 *************************************/

static const tpi6525_interface c16_tpi6525_tpi_2_intf =
{
	c1551_0_read_data,
	c1551_0_read_status,
	c1551_0_read_handshake,
	c1551_0_write_data,
	NULL,
	c1551_0_write_handshake,
	NULL,
	NULL,
	NULL
};

static const tpi6525_interface c16_tpi6525_tpi_2_c1551_intf =
{
	c1551x_read_data,
	c1551x_read_status,
	c1551x_read_handshake,
	c1551x_write_data,
	NULL,
	c1551x_write_handshake,
	NULL,
	NULL,
	NULL
};

static const tpi6525_interface c16_tpi6525_tpi_3_intf =
{
	c1551_1_read_data,
	c1551_1_read_status,
	c1551_1_read_handshake,
	c1551_1_write_data,
	NULL,
	c1551_1_write_handshake,
	NULL,
	NULL,
	NULL
};

static const m6502_interface c16_m7501_interface =
{
	NULL,					/* read_indexed_func */
	NULL,					/* write_indexed_func */
	c16_m7501_port_read,	/* port_read_func */
	c16_m7501_port_write	/* port_write_func */
};

static MACHINE_DRIVER_START( c16 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", M7501, 1400000)        /* 7.8336 MHz */
	MDRV_CPU_PROGRAM_MAP(c16_map)
	MDRV_CPU_CONFIG( c16_m7501_interface )
	MDRV_CPU_VBLANK_INT("screen", c16_frame_interrupt)
	MDRV_CPU_PERIODIC_INT(ted7360_raster_interrupt, TED7360_HRETRACERATE)
	MDRV_QUANTUM_TIME(HZ(60))

	MDRV_MACHINE_RESET( c16 )

    /* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(TED7360PAL_VRETRACERATE)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(336, 216)
	MDRV_SCREEN_VISIBLE_AREA(0, 336 - 1, 0, 216 - 1)
	MDRV_PALETTE_LENGTH(ARRAY_LENGTH(ted7360_palette) / 3)
	MDRV_PALETTE_INIT(c16)

	MDRV_VIDEO_START( ted7360 )
	MDRV_VIDEO_UPDATE( ted7360 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("ted7360", TED7360, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD("sid", SID8580, TED7360PAL_CLOCK/4)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* devices */
	MDRV_QUICKLOAD_ADD("quickload", cbm_c16, "p00,prg", CBM_QUICKLOAD_DELAY_SECONDS)

	/* cassette */
	MDRV_CASSETTE_ADD( "cassette", cbm_cassette_config )

	/* floppy from serial bus */
	MDRV_IMPORT_FROM(simulated_drive)

	/* tpi */
	MDRV_TPI6525_ADD("tpi6535_tpi_2", c16_tpi6525_tpi_2_intf)
	MDRV_TPI6525_ADD("tpi6535_tpi_3", c16_tpi6525_tpi_3_intf)

	MDRV_IMPORT_FROM(c16_cartslot)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("64K")
	MDRV_RAM_EXTRA_OPTIONS("16K,32K")
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( c16c )
	MDRV_IMPORT_FROM( c16 )

	/* c16c uses 'real' floppy drive emulation from machine/vc1541.c...
    still in progress, atm */
	MDRV_DEVICE_REMOVE("tpi6535_tpi_2")
	MDRV_TPI6525_ADD("tpi6535_tpi_2", c16_tpi6525_tpi_2_c1551_intf)

	/* emulation code currently supports only one drive */
	MDRV_DEVICE_REMOVE("tpi6535_tpi_3")

	MDRV_DEVICE_REMOVE("serial_bus")	// in the current code, serial bus device is tied to the floppy drive
	MDRV_IMPORT_FROM( cpu_c1551 )
#ifdef CPU_SYNC
	MDRV_QUANTUM_TIME(HZ(60))
#else
	MDRV_QUANTUM_TIME(HZ(6000))
#endif
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( c16v )
	MDRV_IMPORT_FROM( c16 )

	MDRV_DEVICE_REMOVE("serial_bus")	// in the current code, serial bus device is tied to the floppy drive
	MDRV_IMPORT_FROM( cpu_vc1541 )
#ifdef CPU_SYNC
	MDRV_QUANTUM_TIME(HZ(60))
#else
	MDRV_QUANTUM_TIME(HZ(300000))
#endif

	/* no c1551 in this driver */
	MDRV_DEVICE_REMOVE("tpi6535_tpi_2")
	MDRV_DEVICE_REMOVE("tpi6535_tpi_3")
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( plus4 )
	MDRV_IMPORT_FROM( c16 )
	MDRV_CPU_REPLACE( "maincpu", M7501, 1200000)
	MDRV_CPU_PROGRAM_MAP(plus4_map)
	MDRV_CPU_CONFIG( c16_m7501_interface )
	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_REFRESH_RATE(TED7360NTSC_VRETRACERATE)

	MDRV_SOUND_REPLACE("sid", SID8580, TED7360NTSC_CLOCK/4)

	/* internal ram */
	MDRV_RAM_MODIFY("messram")
	MDRV_RAM_DEFAULT_SIZE("64K")
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( plus4c )
	MDRV_IMPORT_FROM( plus4 )

	/* plus4c uses 'real' floppy drive emulation from machine/vc1541.c...
    still in progress, atm */
	MDRV_DEVICE_REMOVE("tpi6535_tpi_2")
	MDRV_TPI6525_ADD("tpi6535_tpi_2", c16_tpi6525_tpi_2_c1551_intf)

	/* emulation code currently supports only one drive */
	MDRV_DEVICE_REMOVE("serial_bus")	// in the current code, serial bus device is tied to the floppy drive
	MDRV_DEVICE_REMOVE("tpi6535_tpi_3")

	MDRV_IMPORT_FROM( cpu_c1551 )

	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
#ifdef CPU_SYNC
	MDRV_QUANTUM_TIME(HZ(60))
#else
	MDRV_QUANTUM_TIME(HZ(60000))
#endif
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( plus4v )
	MDRV_IMPORT_FROM( plus4 )

	MDRV_DEVICE_REMOVE("serial_bus")	// in the current code, serial bus device is tied to the floppy drive
	MDRV_IMPORT_FROM( cpu_vc1541 )

	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
#ifdef CPU_SYNC
	MDRV_QUANTUM_TIME(HZ(60))
#else
	MDRV_QUANTUM_TIME(HZ(300000))
#endif

	/* no c1551 in this driver */
	MDRV_DEVICE_REMOVE("tpi6535_tpi_2")
	MDRV_DEVICE_REMOVE("tpi6535_tpi_3")
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( c364 )
	MDRV_IMPORT_FROM( plus4 )
	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_CPU_MODIFY( "maincpu" )
	MDRV_CPU_PROGRAM_MAP(c364_map)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( c264 )
	MDRV_IMPORT_FROM( c16 )
	/* internal ram */
	MDRV_RAM_MODIFY("messram")
	MDRV_RAM_DEFAULT_SIZE("64K")
MACHINE_DRIVER_END


/*************************************
 *
 *  ROM definition(s)
 *
 *************************************/

ROM_START( c232 )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "318006-01.bin", 0x10000, 0x4000, CRC(74eaae87) SHA1(161c96b4ad20f3a4f2321808e37a5ded26a135dd) )
	ROM_LOAD( "318004-01.bin", 0x14000, 0x4000, CRC(dbdc3319) SHA1(3c77caf72914c1c0a0875b3a7f6935cd30c54201) )
ROM_END

ROM_START( c264 )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "basic-264.bin", 0x10000, 0x4000, CRC(6a2fc8e3) SHA1(473fce23afa07000cdca899fbcffd6961b36a8a0) )
	ROM_LOAD( "kernal-264.bin", 0x14000, 0x4000, CRC(8f32abe7) SHA1(d481faf5fcbb331878dc7851c642d04f26a32873) )
ROM_END

ROM_START( c364 )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "318006.01", 0x10000, 0x4000, CRC(74eaae87) SHA1(161c96b4ad20f3a4f2321808e37a5ded26a135dd) )
	ROM_LOAD( "kern364p.bin", 0x14000, 0x4000, CRC(84fd4f7a) SHA1(b9a5b5dacd57ca117ef0b3af29e91998bf4d7e5f) )
	ROM_LOAD( "317053-01.bin", 0x18000, 0x4000, CRC(4fd1d8cb) SHA1(3b69f6e7cb4c18bb08e203fb18b7dabfa853390f) )
	ROM_LOAD( "317054-01.bin", 0x1c000, 0x4000, CRC(109de2fc) SHA1(0ad7ac2db7da692d972e586ca0dfd747d82c7693) )
	/* at address 0x20000 not so good */
	ROM_LOAD( "spk3cc4.bin", 0x28000, 0x4000, CRC(5227c2ee) SHA1(59af401cbb2194f689898271c6e8aafa28a7af11) )
ROM_END


ROM_START( c16 )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "318006-01.bin", 0x10000, 0x4000, CRC(74eaae87) SHA1(161c96b4ad20f3a4f2321808e37a5ded26a135dd) )
	ROM_SYSTEM_BIOS( 0, "default", "rev. 5" )
	ROMX_LOAD( "318004-05.bin", 0x14000, 0x4000, CRC(71c07bd4) SHA1(7c7e07f016391174a557e790c4ef1cbe33512cdb), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "rev3", "rev. 3" )
	ROMX_LOAD( "318004-03.bin", 0x14000, 0x4000, CRC(77bab934) SHA1(97814dab9d757fe5a3a61d357a9a81da588a9783), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "rev4", "rev. 4" )
	ROMX_LOAD( "318004-04.bin", 0x14000, 0x4000, CRC(be54ed79) SHA1(514ad3c29d01a2c0a3b143d9c1d4143b1912b793), ROM_BIOS(3) )
ROM_END

ROM_START( c16c )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "318006-01.bin", 0x10000, 0x4000, CRC(74eaae87) SHA1(161c96b4ad20f3a4f2321808e37a5ded26a135dd) )
	ROM_SYSTEM_BIOS( 0, "default", "rev. 5" )
	ROMX_LOAD( "318004-05.bin", 0x14000, 0x4000, CRC(71c07bd4) SHA1(7c7e07f016391174a557e790c4ef1cbe33512cdb), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "rev3", "rev. 3" )
	ROMX_LOAD( "318004-03.bin", 0x14000, 0x4000, CRC(77bab934) SHA1(97814dab9d757fe5a3a61d357a9a81da588a9783), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "rev4", "rev. 4" )
	ROMX_LOAD( "318004-04.bin", 0x14000, 0x4000, CRC(be54ed79) SHA1(514ad3c29d01a2c0a3b143d9c1d4143b1912b793), ROM_BIOS(3) )

	C1551_ROM("cpu_c1551")
ROM_END

ROM_START( c16v )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "318006-01.bin", 0x10000, 0x4000, CRC(74eaae87) SHA1(161c96b4ad20f3a4f2321808e37a5ded26a135dd) )
	ROM_LOAD( "318004-05.bin", 0x14000, 0x4000, CRC(71c07bd4) SHA1(7c7e07f016391174a557e790c4ef1cbe33512cdb) )

	/* we temporarily use -bios to select among vc1541 firmwares, for this system */
	VC1541_ROM("cpu_vc1540")
ROM_END

#define rom_c116		rom_c16
#define rom_c116c		rom_c16c
#define rom_c116v		rom_c16v

ROM_START( c16hun )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "318006-01.bin", 0x10000, 0x4000, CRC(74eaae87) SHA1(161c96b4ad20f3a4f2321808e37a5ded26a135dd) )
	ROM_LOAD( "hungary.bin", 0x14000, 0x4000, CRC(775f60c5) SHA1(20cf3c4bf6c54ef09799af41887218933f2e27ee) )
ROM_END

ROM_START( plus4 )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "318006-01.bin", 0x10000, 0x4000, CRC(74eaae87) SHA1(161c96b4ad20f3a4f2321808e37a5ded26a135dd) )
	ROM_SYSTEM_BIOS( 0, "default", "rev. 5" )
	ROMX_LOAD( "318005-05.bin", 0x14000, 0x4000, CRC(70295038) SHA1(a3d9e5be091b98de39a046ab167fb7632d053682), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "rev4", "rev. 4" )
	ROMX_LOAD( "318005-04.bin", 0x14000, 0x4000, CRC(799a633d) SHA1(5df52c693387c0e2b5d682613a3b5a65477311cf), ROM_BIOS(2) )

	ROM_LOAD( "317053-01.bin", 0x18000, 0x4000, CRC(4fd1d8cb) SHA1(3b69f6e7cb4c18bb08e203fb18b7dabfa853390f) )
	ROM_LOAD( "317054-01.bin", 0x1c000, 0x4000, CRC(109de2fc) SHA1(0ad7ac2db7da692d972e586ca0dfd747d82c7693) )
ROM_END

ROM_START( plus4c )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "318006-01.bin", 0x10000, 0x4000, CRC(74eaae87) SHA1(161c96b4ad20f3a4f2321808e37a5ded26a135dd) )
	ROM_SYSTEM_BIOS( 0, "default", "rev. 5" )
	ROMX_LOAD( "318005-05.bin", 0x14000, 0x4000, CRC(70295038) SHA1(a3d9e5be091b98de39a046ab167fb7632d053682), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "rev4", "rev. 4" )
	ROMX_LOAD( "318005-04.bin", 0x14000, 0x4000, CRC(799a633d) SHA1(5df52c693387c0e2b5d682613a3b5a65477311cf), ROM_BIOS(2) )

	ROM_LOAD( "317053-01.bin", 0x18000, 0x4000, CRC(4fd1d8cb) SHA1(3b69f6e7cb4c18bb08e203fb18b7dabfa853390f) )
	ROM_LOAD( "317054-01.bin", 0x1c000, 0x4000, CRC(109de2fc) SHA1(0ad7ac2db7da692d972e586ca0dfd747d82c7693) )

	C1551_ROM("cpu_c1551")
ROM_END

ROM_START( plus4v )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "318006-01.bin", 0x10000, 0x4000, CRC(74eaae87) SHA1(161c96b4ad20f3a4f2321808e37a5ded26a135dd) )
	ROM_LOAD( "318005-05.bin", 0x14000, 0x4000, CRC(70295038) SHA1(a3d9e5be091b98de39a046ab167fb7632d053682) )

	ROM_LOAD( "317053-01.bin", 0x18000, 0x4000, CRC(4fd1d8cb) SHA1(3b69f6e7cb4c18bb08e203fb18b7dabfa853390f) )
	ROM_LOAD( "317054-01.bin", 0x1c000, 0x4000, CRC(109de2fc) SHA1(0ad7ac2db7da692d972e586ca0dfd747d82c7693) )

	/* we temporarily use -bios to select among vc1541 firmwares, for this system */
	VC1541_ROM("cpu_vc1540")
ROM_END


/*************************************
 *
 *  System configuration(s)
 *
 *************************************/


static SYSTEM_CONFIG_START(c16)
	CONFIG_DEVICE(cbmfloppy_device_getinfo)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(c16c)
	CONFIG_DEVICE(c1551_device_getinfo)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(c16v)
	CONFIG_DEVICE(vc1541_device_getinfo)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(plus)
	CONFIG_DEVICE(cbmfloppy_device_getinfo)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(plusc)
	CONFIG_DEVICE(c1551_device_getinfo)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(plusv)
	CONFIG_DEVICE(vc1541_device_getinfo)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME  PARENT COMPAT MACHINE INPUT   INIT   CONFIG    COMPANY                             FULLNAME            FLAGS */

COMP( 1984, c16,     0,     0,  c16,    c16,    c16,    c16,     "Commodore Business Machines Co.",  "Commodore 16 (PAL)", 0)
COMP( 1984, c16c,    c16,   0,  c16c,   c16,    c16c,   c16c,    "Commodore Business Machines Co.",  "Commodore 16 (PAL, 1551)", 0 )
COMP( 1984, c16v,    c16,   0,  c16v,   c16,    c16v,   c16v,    "Commodore Business Machines Co.",  "Commodore 16 (PAL, VC1541)", GAME_NOT_WORKING)
COMP( 1984, c16hun,  c16,   0,  c16,    c16,    c16,    c16,     "Commodore Business Machines Co.",  "Commodore 16 Novotrade (PAL, Hungary)", 0)

COMP( 1984, c116,    c16,   0,  c16,    c16,    c16,    c16,     "Commodore Business Machines Co.",  "Commodore 116 (PAL)", 0)
COMP( 1984, c116c,	 c16,   0,  c16c,   c16,    c16c,   c16c,    "Commodore Business Machines Co.",  "Commodore 116 (PAL, 1551)", 0 )
COMP( 1984, c116v,   c16,   0,  c16v,   c16,    c16v,   c16v,    "Commodore Business Machines Co.",  "Commodore 116 (PAL, VC1541)", GAME_NOT_WORKING)

COMP( 1984, plus4,   c16,   0,  plus4,  plus4,  c16,    plus,    "Commodore Business Machines Co.",  "Commodore Plus/4 (NTSC)", 0)
COMP( 1984, plus4c,  c16,   0,  plus4c, plus4,  c16c,   plusc,   "Commodore Business Machines Co.",  "Commodore Plus/4 (NTSC, 1551)", 0 )
COMP( 1984, plus4v,  c16,   0,  plus4v, plus4,  c16v,   plusv,   "Commodore Business Machines Co.",  "Commodore Plus/4 (NTSC, VC1541)", GAME_NOT_WORKING)

COMP( 1984, c232,    c16,   0,  c16,    c16,    c16,    c16,     "Commodore Business Machines Co.",  "Commodore 232 (Prototype)", 0)
COMP( 1984, c264,    c16,   0,  c264,   plus4,  c16,    plus,    "Commodore Business Machines Co.",  "Commodore 264 (Prototype)", 0)
COMP( 1984, c364,    c16,   0,  c364,   plus4,  c16,    plus,    "Commodore Business Machines Co.",  "Commodore V364 (Prototype)", GAME_IMPERFECT_SOUND)
