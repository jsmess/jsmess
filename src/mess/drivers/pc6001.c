/***************************************************************************

        NEC PC-6001 series
        NEC PC-6600 series

        12/05/2009 Skeleton driver.

		PC-6001 (1981-09):

		 * CPU: Z80A @ 4 MHz
		 * ROM: 16KB + 4KB (chargen) - no kanji
		 * RAM: 16KB, it can be expanded to 32KB
		 * Text Mode: 32x16 and 2 colors
		 * Graphic Modes: 64x48 (9 colors), 128x192 (4 colors), 256x192 (2 colors)
		 * Sound: BEEP + PSG - Optional Voice Synth Cart
		 * Keyboard: JIS Keyboard with 5 function keys, control key, TAB key,
				HOME/CLR key, INS key, DEL key, GRAPH key, Japanese syllabary
				key, page key, STOP key, and cursor key (4 directions)
		 * 1 cartslot, optional floppy drive, optional serial 232 port, 2
				joystick ports


		PC-6001 mkII (1983-07):

		 * CPU: Z80A @ 4 MHz
		 * ROM: 32KB + 16KB (chargen) + 32KB (kanji) + 16KB (Voice Synth)
		 * RAM: 64KB
		 * Text Mode: same as PC-6001 with N60-BASIC; 40x20 and 15 colors with
				N60M-BASIC
		 * Graphic Modes: same as PC-6001 with N60-BASIC; 80x40 (15 colors),
				160x200 (15 colors), 320x200 (4 colors) with N60M-BASIC
		 * Sound: BEEP + PSG
		 * Keyboard: JIS Keyboard with 5 function keys, control key, TAB key,
				HOME/CLR key, INS key, DEL key, CAPS key, GRAPH key, Japanese
				syllabary key, page key, mode key, STOP key, and cursor key (4
				directions)
		 * 1 cartslot, floppy drive, optional serial 232 port, 2 joystick ports


		PC-6001 mkIISR (1984-12):

		 * CPU: Z80A @ 3.58 MHz
		 * ROM: 64KB + 16KB (chargen) + 32KB (kanji) + 32KB (Voice Synth)
		 * RAM: 64KB
		 * Text Mode: same as PC-6001/PC-6001mkII with N60-BASIC; 40x20, 40x25,
				80x20, 80x25 and 15 colors with N66SR-BASIC
		 * Graphic Modes: same as PC-6001/PC-6001mkII with N60-BASIC; 80x40 (15 colors),
				320x200 (15 colors), 640x200 (15 colors) with N66SR-BASIC
		 * Sound: BEEP + PSG + FM
		 * Keyboard: JIS Keyboard with 5 function keys, control key, TAB key,
				HOME/CLR key, INS key, DEL key, CAPS key, GRAPH key, Japanese
				syllabary key, page key, mode key, STOP key, and cursor key (4
				directions)
		 * 1 cartslot, floppy drive, optional serial 232 port, 2 joystick ports


	info from http://www.geocities.jp/retro_zzz/machines/nec/6001/spc60.html

===

irq vector 00: writes 0x00 to [$fa19]
irq vector 02: (A = 0, B = 0) tests ppi port c, does something with ay ports (plus more?)
irq vector 04: uart irq
irq vector 06: operates with $fa28, $fa2e, $fd1b
irq vector 08: tests ppi port c, puts port A to $fa1d,puts 0x02 to [$fa19]
irq vector 0A: writes 0x00 to [$fa19]
irq vector 0C: writes 0x00 to [$fa19]
irq vector 0E: same as 2, (A = 0x03, B = 0x00)
irq vector 10: same as 2, (A = 0x03, B = 0x00)
irq vector 12: writes 0x10 to [$fa19]
irq vector 14: same as 2, (A = 0x00, B = 0x01)
irq vector 16: tests ppi port c, writes the result to $feca.


****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "machine/i8255a.h"
#include "machine/msm8251.h"
#include "video/m6847.h"
#include "sound/ay8910.h"

static UINT8 *pc6001_ram;
static UINT8 *pc6001_video_ram;

static WRITE8_HANDLER ( pc6001_system_latch_w )
{
	UINT16 startaddr[] = {0xC000, 0xE000, 0x8000, 0xA000 };

	pc6001_video_ram =  pc6001_ram + startaddr[(data >> 1) & 0x03] - 0x8000;
}


static ATTR_CONST UINT8 pc6001_get_attributes(running_machine *machine, UINT8 c,int scanline, int pos)
{
	UINT8 result = 0x00;
	UINT8 val = pc6001_video_ram [(scanline / 12) * 0x20 + pos];
	if (val & 0x01) {
		result |= M6847_INV;
	}
	result |= M6847_INTEXT; // always use external ROM
	return result;
}

static const UINT8 *pc6001_get_video_ram(running_machine *machine, int scanline)
{
	return pc6001_video_ram +0x0200+ (scanline / 12) * 0x20;
}

static UINT8 pc6001_get_char_rom(running_machine *machine, UINT8 ch, int line)
{
	UINT8 *gfx = memory_region(machine, "gfx1");
	return gfx[ch*16+line];
}

static UINT8 port_c_8255;

static READ8_DEVICE_HANDLER(nec_ppi8255_r) {
	if (offset==2) {
		return port_c_8255;
	}
	else if(offset==0)
	{
		if(input_code_pressed(device->machine,KEYCODE_Z)) //quick kludge test
			return 0x30;
		return i8255a_r(device,offset);
	}
	else {
		return i8255a_r(device,offset);
	}
}

static WRITE8_DEVICE_HANDLER(nec_ppi8255_w) {
	if (offset==3) {
		if(data & 1) {
			port_c_8255 |=   1<<((data>>1)&0x07);
		} else {
			port_c_8255 &= ~(1<<((data>>1)&0x07));
		}
		switch(data) {
        	case 0x08: port_c_8255 |= 0x88; break;
         	case 0x09: port_c_8255 &= 0xf7; break;
         	case 0x0c: port_c_8255 |= 0x28; break;
         	case 0x0d: port_c_8255 &= 0xf7; break;
         	default: break;
		}
		port_c_8255 |= 0xa8;
	}
	i8255a_w(device,offset,data);
}

static ADDRESS_MAP_START(pc6001_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xffff) AM_RAM AM_BASE(&pc6001_ram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pc6001_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x80, 0x80) AM_DEVREADWRITE("uart", msm8251_data_r,msm8251_data_w)
	AM_RANGE(0x81, 0x81) AM_DEVREADWRITE("uart", msm8251_status_r,msm8251_control_w)
	AM_RANGE(0x90, 0x93) AM_DEVREADWRITE("ppi8255", nec_ppi8255_r, nec_ppi8255_w)
	AM_RANGE(0xa0, 0xa0) AM_DEVWRITE("ay8910", ay8910_address_w)
	AM_RANGE(0xa1, 0xa1) AM_DEVWRITE("ay8910", ay8910_data_w)
	AM_RANGE(0xa2, 0xa2) AM_DEVREAD("ay8910", ay8910_r)
	AM_RANGE(0xb0, 0xb0) AM_WRITE(pc6001_system_latch_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( pc6001 )
	PORT_START("LINE0")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8)
INPUT_PORTS_END

static UINT8 irq_vector = 0x00;

static INTERRUPT_GEN( pc6001_interrupt )
{
	irq_vector = 0x16;
	cpu_set_input_line(device, 0, ASSERT_LINE);
}

static INTERRUPT_GEN( key_irq )
{
	irq_vector = 0x14;
	cpu_set_input_line(device, 0, ASSERT_LINE);
}

static IRQ_CALLBACK ( pc6001_irq_callback )
{
	cpu_set_input_line(device, 0, CLEAR_LINE);
	return irq_vector;
}

static MACHINE_RESET(pc6001)
{
	port_c_8255=0;
	pc6001_video_ram =  pc6001_ram;

	cpu_set_irq_callback(cputag_get_cpu(machine, "maincpu"),pc6001_irq_callback);
}

static VIDEO_START( pc6001 )
{
	m6847_config cfg;

	memset(&cfg, 0, sizeof(cfg));
	cfg.type = M6847_VERSION_M6847T1_NTSC;
	cfg.get_attributes = pc6001_get_attributes;
	cfg.get_video_ram = pc6001_get_video_ram;
	cfg.get_char_rom = pc6001_get_char_rom;
	m6847_init(machine, &cfg);
}


static READ8_DEVICE_HANDLER (pc6001_8255_porta_r )
{
	return 0;
}
static WRITE8_DEVICE_HANDLER (pc6001_8255_porta_w )
{
//	logerror("pc6001_8255_porta_w %02x\n",data);
}

static READ8_DEVICE_HANDLER (pc6001_8255_portb_r )
{
	return 0;
}
static WRITE8_DEVICE_HANDLER (pc6001_8255_portb_w )
{
	//logerror("pc6001_8255_portb_w %02x\n",data);
}

static WRITE8_DEVICE_HANDLER (pc6001_8255_portc_w )
{
	//logerror("pc6001_8255_portc_w %02x\n",data);
}

static READ8_DEVICE_HANDLER (pc6001_8255_portc_r )
{
	return 0x88;
}



static I8255A_INTERFACE( pc6001_ppi8255_interface )
{
	DEVCB_HANDLER(pc6001_8255_porta_r),
	DEVCB_HANDLER(pc6001_8255_portb_r),
	DEVCB_HANDLER(pc6001_8255_portc_r),
	DEVCB_HANDLER(pc6001_8255_porta_w),
	DEVCB_HANDLER(pc6001_8255_portb_w),
	DEVCB_HANDLER(pc6001_8255_portc_w)
};

static const msm8251_interface pc6001_usart_interface=
{
	NULL,
	NULL,
	NULL
};


static const ay8910_interface pc6001_ay_interface =
{
	AY8910_LEGACY_OUTPUT,
	AY8910_DEFAULT_LOADS,
	DEVCB_NULL
};

static MACHINE_DRIVER_START( pc6001 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu",Z80, 7987200 /2)
	MDRV_CPU_PROGRAM_MAP(pc6001_mem)
	MDRV_CPU_IO_MAP(pc6001_io)
	MDRV_CPU_VBLANK_INT("screen", pc6001_interrupt)
	MDRV_CPU_PERIODIC_INT(key_irq,25)

//	MDRV_CPU_ADD("subcpu", I8049, 7987200)

	MDRV_MACHINE_RESET(pc6001)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(M6847_NTSC_FRAMES_PER_SECOND)
	MDRV_VIDEO_START(pc6001)
	MDRV_VIDEO_UPDATE(m6847)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_SIZE(320, 25+192+26)
	MDRV_SCREEN_VISIBLE_AREA(0, 319, 1, 239)

	MDRV_I8255A_ADD( "ppi8255", pc6001_ppi8255_interface )
	/* uart */
	MDRV_MSM8251_ADD("uart", pc6001_usart_interface)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("ay8910", AY8910, XTAL_4MHz/2)
	MDRV_SOUND_CONFIG(pc6001_ay_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( pc6001sr )
	MDRV_IMPORT_FROM(pc6001)

	/* basic machine hardware */
	MDRV_CPU_REPLACE("maincpu", Z80, XTAL_3_579545MHz)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( pc6001 )	/* screen = 8000-83FF */
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "basicrom.60", 0x0000, 0x4000, CRC(54c03109) SHA1(c622fefda3cdc2b87a270138f24c05828b5c41d2) )

	ROM_REGION( 0x1000, "gfx1", 0 )
	ROM_LOAD( "cgrom60.60", 0x0000, 0x1000, CRC(b0142d32) SHA1(9570495b10af5b1785802681be94b0ea216a1e26) )
ROM_END

ROM_START( pc6001a )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "basicrom.60a", 0x0000, 0x4000, CRC(fa8e88d9) SHA1(c82e30050a837e5c8ffec3e0c8e3702447ffd69c) )

	ROM_REGION( 0x1000, "gfx1", 0 )
	ROM_LOAD( "cgrom60.60a", 0x0000, 0x1000, CRC(49c21d08) SHA1(9454d6e2066abcbd051bad9a29a5ca27b12ec897) )
ROM_END

ROM_START( pc6001m2 )
	ROM_REGION( 0x14000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "basicrom.62", 0x0000, 0x8000, CRC(950ac401) SHA1(fbf195ba74a3b0f80b5a756befc96c61c2094182) )
	/* This rom loads at 4000 with 0-3FFF being RAM (a bankswitch, obviously) */
	ROM_LOAD( "voicerom.62", 0x10000, 0x4000, CRC(49b4f917) SHA1(1a2d18f52ef19dc93da3d65f19d3abbd585628af) )

	ROM_REGION( 0xc000, "gfx1", 0 )
	ROM_LOAD( "cgrom60.62",  0x0000, 0x2000, CRC(81eb5d95) SHA1(53d8ae9599306ff23bf95208d2f6cc8fed3fc39f) )
	ROM_LOAD( "cgrom60m.62", 0x2000, 0x2000, CRC(3ce48c33) SHA1(f3b6c63e83a17d80dde63c6e4d86adbc26f84f79) )
	ROM_LOAD( "kanjirom.62", 0x4000, 0x8000, CRC(20c8f3eb) SHA1(4c9f30f0a2ebbe70aa8e697f94eac74d8241cadd) )
ROM_END

ROM_START( pc6001sr )
	ROM_REGION( 0x20000, "maincpu", ROMREGION_ERASEFF )
	/* If you split this into 4x 8000, the first 3 need to load to 0-7FFF (via bankswitch). The 4th part looks like gfx data */
	ROM_LOAD( "systemrom1.64", 0x0000, 0x10000, CRC(b6fc2db2) SHA1(dd48b1eee60aa34780f153359f5da7f590f8dff4) )
	ROM_LOAD( "systemrom2.64", 0x10000, 0x10000, CRC(55a62a1d) SHA1(3a19855d290fd4ac04e6066fe4a80ecd81dc8dd7) )

	ROM_REGION( 0x4000, "gfx1", 0 )
	ROM_LOAD( "cgrom68.64", 0x0000, 0x4000, CRC(73bc3256) SHA1(5f80d62a95331dc39b2fb448a380fd10083947eb) )
ROM_END

ROM_START( pc6600 )	/* Variant of pc6001m2 */
	ROM_REGION( 0x14000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "basicrom.66", 0x0000, 0x8000, CRC(c0b01772) SHA1(9240bb6b97fe06f5f07b5d65541c4d2f8758cc2a) )
	ROM_LOAD( "voicerom.66", 0x10000, 0x4000, CRC(91d078c1) SHA1(6a93bd7723ef67f461394530a9feee57c8caf7b7) )

	ROM_REGION( 0xc000, "gfx1", 0 )
	ROM_LOAD( "cgrom60.66",  0x0000, 0x2000, CRC(d2434f29) SHA1(a56d76f5cbdbcdb8759abe601eab68f01b0a8fe8) )
	ROM_LOAD( "cgrom66.66",  0x2000, 0x2000, CRC(3ce48c33) SHA1(f3b6c63e83a17d80dde63c6e4d86adbc26f84f79) )
	ROM_LOAD( "kanjirom.66", 0x4000, 0x8000, CRC(20c8f3eb) SHA1(4c9f30f0a2ebbe70aa8e697f94eac74d8241cadd) )
ROM_END

/* There exists an alternative (incomplete?) dump, consisting of more .68 pieces, but it's been probably created for emulators:
systemrom1.68 = 0x0-0x8000 BASICROM.68 + ??
systemrom2.68 = 0x0-0x2000 ?? + 0x2000-0x4000 SYSROM2.68 + 0x4000-0x8000 VOICEROM.68 + 0x8000-0x10000 KANJIROM.68
cgrom68.68 = CGROM60.68 + CGROM66.68
 */
ROM_START( pc6600sr )	/* Variant of pc6001sr */
	ROM_REGION( 0x20000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "systemrom1.68", 0x0000, 0x10000, CRC(b6fc2db2) SHA1(dd48b1eee60aa34780f153359f5da7f590f8dff4) )
	ROM_LOAD( "systemrom2.68", 0x10000, 0x10000, CRC(55a62a1d) SHA1(3a19855d290fd4ac04e6066fe4a80ecd81dc8dd7) )

	ROM_REGION( 0x4000, "gfx1", 0 )
	ROM_LOAD( "cgrom68.68", 0x0000, 0x4000, CRC(73bc3256) SHA1(5f80d62a95331dc39b2fb448a380fd10083947eb) )
ROM_END

/*    YEAR  NAME      PARENT   COMPAT MACHINE   INPUT     INIT    CONFIG     COMPANY  FULLNAME          FLAGS */
COMP( 1981, pc6001,   0,       0,     pc6001,   pc6001,   0,      0,    "Nippon Electronic Company",   "PC-6001",       GAME_NOT_WORKING )
COMP( 1981, pc6001a,  pc6001,  0,     pc6001,   pc6001,   0,      0,    "Nippon Electronic Company",   "PC-6001A",      GAME_NOT_WORKING )	// US version of PC-6001
COMP( 1983, pc6001m2, pc6001,  0,     pc6001,   pc6001,   0,      0,    "Nippon Electronic Company",   "PC-6001mkII",   GAME_NOT_WORKING )
COMP( 1983, pc6600,   pc6001,  0,     pc6001,   pc6001,   0,      0,    "Nippon Electronic Company",   "PC-6600",       GAME_NOT_WORKING )	// high-end version of PC-6001mkII
COMP( 1984, pc6001sr, pc6001,  0,     pc6001sr, pc6001,   0,      0,    "Nippon Electronic Company",   "PC-6001mkIISR", GAME_NOT_WORKING )
COMP( 1984, pc6600sr, pc6001,  0,     pc6001sr, pc6001,   0,      0,    "Nippon Electronic Company",   "PC-6600SR",     GAME_NOT_WORKING )	// high-end version of PC-6001mkIISR
