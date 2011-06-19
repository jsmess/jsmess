/******************************************************************************
 Peter.Trauner@jk.uni-linz.ac.at May 2001

 Paul Robson's Emulator at www.classicgaming.com/studio2 made it possible
******************************************************************************/

/*****************************************************************************
Additional Notes by Manfred Schneider
Memory Map
Memory mapping is done in two steps. The PVI(2636) provides the chip select signals
according to signals provided by the CPU address lines.
The PVI has 12 address line (A0-A11) which give him control over 4K. A11 of the PVI is not
connected to A11 of the CPU, but connected to the cartridge slot. On the cartridge it is
connected to A12 of the CPU which extends the addressable Range to 8K. This is also the
maximum usable space, because A13 and A14 of the CPU are not used.
With the above in mind address range will lock like this:
$0000 - $15FF  ROM, RAM
$1600 - $167F  unused
$1680 - $16FF  used for I/O Control on main PCB
$1700 - $17FF PVI internal Registers and RAM
$1800 - $1DFF ROM, RAM
$1E00 - $1E7F unused
$1E80 - $1EFF mirror of $1680 - $167F
$1F00 - $1FFF mirror of $1700 - $17FF
$2000 - $3FFF mirror of $0000 - $1FFF
$4000 - $5FFF mirror of $0000 - $1FFF
$6000 - $7FFF mirror of $0000 - $1FFF
On all cartridges for the Interton A11 from PVI is connected to A12 of the CPU.
There are four different types of Cartridges with the following memory mapping.
Type 1: 2K Rom or EPROM mapped from $0000 - $07FF
Type 2: 4K Rom or EPROM mapped from $0000 - $0FFF
Type 3: 4K Rom + 1K Ram
    Rom is mapped from $0000 - $0FFF
    Ram is mapped from $1000 - $13FF and mirrored from $1800 - $1BFF
Type 4: 6K Rom + 1K Ram
    Rom is mapped from $0000 - $15FF (only 5,5K ROM visible to the CPU)
    Ram is mapped from $1800 - $1BFF

One other type is known for Radofin (rom compatible to VC4000, but not the Cartridge connector).
It consisted of a 2K ROM and 2K RAM which are most likely mapped as follows (needs to be confirmed):
2K Rom mapped from $0000 - $07FF
2K Ram mapped from $0800 - $0FFF
The Cartridge is called Hobby Module and the Rom is probably the same as used in
elektor TV Game Computer which is a kind of developer machine for the VC4000.

Go to the bottom to see the game list and emulation status of each.
******************************************************************************/

#include "emu.h"
#include "cpu/s2650/s2650.h"

#include "includes/vc4000.h"
#include "imagedev/cartslot.h"
#include "imagedev/snapquik.h"

static QUICKLOAD_LOAD( vc4000 );

static READ8_HANDLER(vc4000_key_r)
{
	UINT8 data=0;
	switch(offset & 0x0f) {
	case 0x08:
		data = input_port_read(space->machine(), "KEYPAD1_1");
		break;
	case 0x09:
		data = input_port_read(space->machine(), "KEYPAD1_2");
		break;
	case 0x0a:
		data = input_port_read(space->machine(), "KEYPAD1_3");
		break;
	case 0x0b:
		data = input_port_read(space->machine(), "PANEL");
		break;
	case 0x0c:
		data = input_port_read(space->machine(), "KEYPAD2_1");
		break;
	case 0x0d:
		data = input_port_read(space->machine(), "KEYPAD2_2");
		break;
	case 0x0e:
		data = input_port_read(space->machine(), "KEYPAD2_3");
		break;
	}
	return data;
}

static WRITE8_HANDLER(vc4000_sound_ctl)
{
	logerror("Write to sound control register offset= %d value= %d\n", offset, data);
}


static ADDRESS_MAP_START( vc4000_mem , AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0x1fff)
	AM_RANGE(0x0000, 0x07ff) AM_ROM
	AM_RANGE(0x1680, 0x16ff) AM_READWRITE( vc4000_key_r, vc4000_sound_ctl ) AM_MIRROR(0x0800)
	AM_RANGE(0x1700, 0x17ff) AM_READWRITE( vc4000_video_r, vc4000_video_w ) AM_MIRROR(0x0800)
ADDRESS_MAP_END

static ADDRESS_MAP_START( vc4000_io , AS_IO, 8)
	AM_RANGE( S2650_SENSE_PORT,S2650_SENSE_PORT) AM_READ( vc4000_vsync_r)
ADDRESS_MAP_END

static INPUT_PORTS_START( vc4000 )
	PORT_START("PANEL")
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START ) PORT_NAME("Start")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SELECT ) PORT_NAME("Game Select")

	PORT_START("KEYPAD1_1")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("P1 Keypad 1") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("P1 Keypad 4") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("P1 Keypad 7") PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("P1 Keypad Enter") PORT_CODE(KEYCODE_PLUS_PAD)

	PORT_START("KEYPAD1_2")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("P1 Keypad 2/Button") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("P1 Keypad 5") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("P1 Keypad 8") PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("P1 Keypad 0") PORT_CODE(KEYCODE_0_PAD)

	PORT_START("KEYPAD1_3")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("P1 Keypad 3") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("P1 Keypad 6") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("P1 Keypad 9") PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("P1 Keypad Clear") PORT_CODE(KEYCODE_MINUS_PAD)

	PORT_START("KEYPAD2_1")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("P2 Keypad 1") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("P2 Keypad 4") PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("P2 Keypad 7") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("P2 Keypad Enter") PORT_CODE(KEYCODE_V)

	PORT_START("KEYPAD2_2")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("P2 Keypad 2/Button") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("P2 Keypad 5") PORT_CODE(KEYCODE_S)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("P2 Keypad 8") PORT_CODE(KEYCODE_W)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("P2 Keypad 0") PORT_CODE(KEYCODE_F)

	PORT_START("KEYPAD2_3")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("P2 Keypad 3") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("P2 Keypad 6") PORT_CODE(KEYCODE_D)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("P2 Keypad 9") PORT_CODE(KEYCODE_E)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("P2 Keypad Clear") PORT_CODE(KEYCODE_R)
#ifndef ANALOG_HACK
    // auto centering too slow, so only using 5 bits, and scaling at videoside
    PORT_START("JOY1_X")
PORT_BIT(0xff,0x70,IPT_AD_STICK_X) PORT_SENSITIVITY(70) PORT_KEYDELTA(5) PORT_CENTERDELTA(0) PORT_MINMAX(20,225) PORT_CODE_DEC(KEYCODE_LEFT) PORT_CODE_INC(KEYCODE_RIGHT) PORT_CODE_DEC(JOYCODE_X_LEFT_SWITCH) PORT_CODE_INC(JOYCODE_X_RIGHT_SWITCH) PORT_PLAYER(1)
    PORT_START("JOY1_Y")
PORT_BIT(0xff,0x70,IPT_AD_STICK_Y) PORT_SENSITIVITY(70) PORT_KEYDELTA(5) PORT_CENTERDELTA(0) PORT_MINMAX(20,225) PORT_CODE_DEC(KEYCODE_UP) PORT_CODE_INC(KEYCODE_DOWN) PORT_CODE_DEC(JOYCODE_Y_UP_SWITCH) PORT_CODE_INC(JOYCODE_Y_DOWN_SWITCH) PORT_PLAYER(1)
    PORT_START("JOY2_X")
PORT_BIT(0xff,0x70,IPT_AD_STICK_X) PORT_SENSITIVITY(70) PORT_KEYDELTA(5) PORT_CENTERDELTA(0) PORT_MINMAX(20,225) PORT_CODE_DEC(KEYCODE_DEL) PORT_CODE_INC(KEYCODE_PGDN) PORT_CODE_DEC(JOYCODE_X_LEFT_SWITCH) PORT_CODE_INC(JOYCODE_X_RIGHT_SWITCH) PORT_PLAYER(2)
    PORT_START("JOY2_Y")
PORT_BIT(0xff,0x70,IPT_AD_STICK_Y) PORT_SENSITIVITY(70) PORT_KEYDELTA(5) PORT_CENTERDELTA(0) PORT_MINMAX(20,225) PORT_CODE_DEC(KEYCODE_HOME) PORT_CODE_INC(KEYCODE_END) PORT_CODE_DEC(JOYCODE_Y_UP_SWITCH) PORT_CODE_INC(JOYCODE_Y_DOWN_SWITCH) PORT_PLAYER(2)
#else
	PORT_START("JOYS")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_CODE(KEYCODE_DEL) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_CODE(KEYCODE_PGDN) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_CODE(KEYCODE_END) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_CODE(KEYCODE_HOME) PORT_PLAYER(2)

	PORT_START("CONFIG")
	PORT_CONFNAME( 0x01, 0x00, "Treat Joystick as...")
	PORT_CONFSETTING(    0x00, "Buttons")
	PORT_CONFSETTING(    0x01, "Paddle")
#endif
INPUT_PORTS_END

static const rgb_t vc4000_palette[] =
{
	// background colors
	MAKE_RGB(0, 0, 0), // black
	MAKE_RGB(0, 0, 175), // blue
	MAKE_RGB(0, 175, 0), // green
	MAKE_RGB(0, 255, 255), // cyan
	MAKE_RGB(255, 0, 0), // red
	MAKE_RGB(255, 0, 255), // magenta
	MAKE_RGB(200, 200, 0), // yellow
	MAKE_RGB(200, 200, 200), // white
	/* sprite colors
    The control line simply inverts the RGB lines all at once.
    We can do that in the code with ^7 */
};

static PALETTE_INIT( vc4000 )
{
	palette_set_colors(machine, 0, vc4000_palette, ARRAY_LENGTH(vc4000_palette));
}

static DEVICE_IMAGE_LOAD( vc4000_cart )
{
	running_machine &machine = image.device().machine();
	address_space *memspace = machine.device("maincpu")->memory().space(AS_PROGRAM);
	UINT32 size;

	if (image.software_entry() == NULL)
		size = image.length();
	else
		size = image.get_software_region_length("rom");

	if (size > 0x1600)
		size = 0x1600;

	if (size > 0x1000)	/* 6k rom + 1k ram - Chess2 only */
	{
		memspace->install_read_bank(0x0800, 0x15ff, "bank1");	/* extra rom */
		memory_set_bankptr(machine, "bank1", machine.region("maincpu")->base() + 0x1000);

		memspace->install_readwrite_bank(0x1800, 0x1bff, "bank2");	/* ram */
		memory_set_bankptr(machine, "bank2", machine.region("maincpu")->base() + 0x1800);
	}
	else if (size > 0x0800)	/* some 4k roms have 1k of mirrored ram */
	{
		memspace->install_read_bank(0x0800, 0x0fff, "bank1");	/* extra rom */
		memory_set_bankptr(machine, "bank1", machine.region("maincpu")->base() + 0x0800);

		memspace->install_readwrite_bank(0x1000, 0x15ff, 0, 0x800, "bank2"); /* ram */
		memory_set_bankptr(machine, "bank2", machine.region("maincpu")->base() + 0x1000);
	}
	else if (size == 0x0800)	/* 2k roms + 2k ram - Hobby Module(Radofin) and elektor TVGC*/
	{
		memspace->install_readwrite_bank(0x0800, 0x0fff, "bank1"); /* ram */
		memory_set_bankptr(machine, "bank1", machine.region("maincpu")->base() + 0x0800);
	}

	if (size > 0)
	{
		if (image.software_entry() == NULL)
		{
			if (image.fread( machine.region("maincpu")->base(), size) != size)
				return IMAGE_INIT_FAIL;
		}
		else
			memcpy(machine.region("maincpu")->base(), image.get_software_region("rom"), size);
	}

	return IMAGE_INIT_PASS;
}

static MACHINE_CONFIG_START( vc4000, vc4000_state )
	/* basic machine hardware */
//  MCFG_CPU_ADD("maincpu", S2650, 865000)        /* 3550000/4, 3580000/3, 4430000/3 */
	MCFG_CPU_ADD("maincpu", S2650, 3546875/4)
	MCFG_CPU_PROGRAM_MAP(vc4000_mem)
	MCFG_CPU_IO_MAP(vc4000_io)
	MCFG_CPU_PERIODIC_INT(vc4000_video_line, 312*53)	// GOLF needs this exact value

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(226, 312)
	MCFG_SCREEN_VISIBLE_AREA(8, 184, 0, 269)
	MCFG_SCREEN_UPDATE( vc4000 )

	MCFG_PALETTE_LENGTH(8)
	MCFG_PALETTE_INIT( vc4000 )

	MCFG_VIDEO_START( vc4000 )

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("custom", VC4000, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	/* quickload */
	MCFG_QUICKLOAD_ADD("quickload", vc4000, "tvc", 0)

	/* cartridge */
	MCFG_CARTSLOT_ADD("cart")
	MCFG_CARTSLOT_EXTENSION_LIST("rom,bin")
	MCFG_CARTSLOT_NOT_MANDATORY
	MCFG_CARTSLOT_INTERFACE("vc4000_cart")
	MCFG_CARTSLOT_LOAD(vc4000_cart)

	/* software lists */
	MCFG_SOFTWARE_LIST_ADD("cart_list","vc4000")
MACHINE_CONFIG_END


ROM_START( vc4000 )
	ROM_REGION( 0x2000,"maincpu", ROMREGION_ERASEFF )
ROM_END

ROM_START( spc4000 )
	ROM_REGION( 0x2000,"maincpu", ROMREGION_ERASEFF )
ROM_END

ROM_START( cx3000tc )
	ROM_REGION( 0x2000,"maincpu", ROMREGION_ERASEFF )
ROM_END

ROM_START( tvc4000 )
	ROM_REGION( 0x2000,"maincpu", ROMREGION_ERASEFF )
ROM_END

ROM_START( 1292apvs )
	ROM_REGION( 0x2000,"maincpu", ROMREGION_ERASEFF )
ROM_END

ROM_START( 1392apvs )
	ROM_REGION( 0x2000,"maincpu", ROMREGION_ERASEFF )
ROM_END

ROM_START( mpu1000 )
	ROM_REGION( 0x2000,"maincpu", ROMREGION_ERASEFF )
ROM_END

ROM_START( mpu2000 )
	ROM_REGION( 0x2000,"maincpu", ROMREGION_ERASEFF )
ROM_END

ROM_START( pp1292 )
	ROM_REGION( 0x2000,"maincpu", ROMREGION_ERASEFF )
ROM_END

ROM_START( pp1392 )
	ROM_REGION( 0x2000,"maincpu", ROMREGION_ERASEFF )
ROM_END

ROM_START( f1392 )
	ROM_REGION( 0x2000,"maincpu", ROMREGION_ERASEFF )
ROM_END

ROM_START( fforce2 )
	ROM_REGION( 0x2000,"maincpu", ROMREGION_ERASEFF )
ROM_END

ROM_START( hmg1292 )
	ROM_REGION( 0x2000,"maincpu", ROMREGION_ERASEFF )
ROM_END

ROM_START( hmg1392 )
	ROM_REGION( 0x2000,"maincpu", ROMREGION_ERASEFF )
ROM_END

ROM_START( lnsy1392 )
	ROM_REGION( 0x2000,"maincpu", ROMREGION_ERASEFF )
ROM_END

ROM_START( vc6000 )
	ROM_REGION( 0x2000,"maincpu", ROMREGION_ERASEFF )
ROM_END

ROM_START( database )
	ROM_REGION( 0x2000,"maincpu", ROMREGION_ERASEFF )
ROM_END

ROM_START( vmdtbase )
	ROM_REGION( 0x2000,"maincpu", ROMREGION_ERASEFF )
ROM_END

ROM_START( rwtrntcs )
	ROM_REGION( 0x2000,"maincpu", ROMREGION_ERASEFF )
ROM_END

ROM_START( telngtcs )
	ROM_REGION( 0x2000,"maincpu", ROMREGION_ERASEFF )
ROM_END

ROM_START( krvnjvtv )
	ROM_REGION( 0x2000,"maincpu", ROMREGION_ERASEFF )
ROM_END

ROM_START( oc2000 )
	ROM_REGION( 0x2000,"maincpu", ROMREGION_ERASEFF )
ROM_END

ROM_START( mpt05 )
	ROM_REGION( 0x2000,"maincpu", ROMREGION_ERASEFF )
ROM_END

QUICKLOAD_LOAD(vc4000)
{
	address_space *space = image.device().machine().device("maincpu")->memory().space(AS_PROGRAM);
	int i;
	int quick_addr = 0x08c0;
	int quick_length;
	UINT8 *quick_data;
	int read_;

	quick_length = image.length();
	quick_data = (UINT8*)malloc(quick_length);
	if (!quick_data)
		return IMAGE_INIT_FAIL;

	read_ = image.fread( quick_data, quick_length);
	if (read_ != quick_length)
		return IMAGE_INIT_FAIL;

	if (quick_data[0] != 2)
		return IMAGE_INIT_FAIL;

	quick_addr = quick_data[1] * 256 + quick_data[2];

	//if ((quick_addr + quick_length - 5) > 0x1000)
	//  return IMAGE_INIT_FAIL;

	space->write_byte(0x08be, quick_data[3]);
	space->write_byte(0x08bf, quick_data[4]);

	for (i = 0; i < quick_length - 5; i++)
	{	if ((quick_addr + i) < 0x1000)
			space->write_byte(i + quick_addr, quick_data[i+5]);
	}

	logerror("quick loading at %.4x size:%.4x\n", quick_addr, (quick_length-5));
	return IMAGE_INIT_PASS;
}


/*   YEAR  NAME      PARENT     COMPAT    MACHINE    INPUT        INIT      COMPANY         FULLNAME */
CONS(1978, vc4000,   0,         0,        vc4000,    vc4000,      0,        "Interton",         "VC 4000",          GAME_IMPERFECT_GRAPHICS )					/* Germany, Austria, UK, Australia */
CONS(1979, spc4000,  vc4000,    0,        vc4000,    vc4000,      0,        "Grundig",          "Super Play Computer 4000", GAME_IMPERFECT_GRAPHICS )			/* Germany, Austria */
CONS(1979, cx3000tc, vc4000,    0,        vc4000,    vc4000,      0,        "Palson",           "CX 3000 Tele Computer", GAME_IMPERFECT_GRAPHICS )			/* Spain */
CONS(1979, tvc4000,  vc4000,    0,        vc4000,    vc4000,      0,        "Koerting",         "TVC-4000",         GAME_IMPERFECT_GRAPHICS )					/* Argentina */
CONS(1976, 1292apvs, 0,         vc4000,   vc4000,    vc4000,      0,        "Radofin",          "1292 Advanced Programmable Video System", GAME_IMPERFECT_GRAPHICS )	/* Europe */
CONS(1976, 1392apvs, 1292apvs,  0,        vc4000,    vc4000,      0,        "Radofin",          "1392 Advanced Programmable Video System", GAME_IMPERFECT_GRAPHICS )	/* Europe */
CONS(1979, mpu1000,  1292apvs,  0,        vc4000,    vc4000,      0,        "Acetronic",        "MPU-1000",         GAME_IMPERFECT_GRAPHICS )					/* Europe */
CONS(1979, mpu2000,  1292apvs,  0,        vc4000,    vc4000,      0,        "Acetronic",        "MPU-2000",         GAME_IMPERFECT_GRAPHICS )					/* Europe */
CONS(1978, pp1292,   1292apvs,  0,        vc4000,    vc4000,      0,        "Audio Sonic",      "PP-1292 Advanced Programmable Video System", GAME_IMPERFECT_GRAPHICS )	/* Europe */
CONS(1978, pp1392,   1292apvs,  0,        vc4000,    vc4000,      0,        "Audio Sonic",      "PP-1392 Advanced Programmable Video System", GAME_IMPERFECT_GRAPHICS )	/* Europe */
CONS(1979, f1392,    1292apvs,  0,        vc4000,    vc4000,      0,        "Fountain",         "Fountain 1392",    GAME_IMPERFECT_GRAPHICS )				/* New Zealand */
CONS(1979, fforce2,  1292apvs,  0,        vc4000,    vc4000,      0,        "Fountain",         "Fountain Force 2", GAME_IMPERFECT_GRAPHICS )				/* New Zealand, Australia */
CONS(1979, hmg1292,  1292apvs,  0,        vc4000,    vc4000,      0,        "Hanimex",          "HMG 1292",         GAME_IMPERFECT_GRAPHICS )					/* Europe */
CONS(1979, hmg1392,  1292apvs,  0,        vc4000,    vc4000,      0,        "Hanimex",          "HMG 1392",         GAME_IMPERFECT_GRAPHICS )					/* Europe */
CONS(1979, lnsy1392, 1292apvs,  0,        vc4000,    vc4000,      0,        "Lansay",           "Lansay 1392",      GAME_IMPERFECT_GRAPHICS )				/* Europe */
CONS(1979, vc6000,   1292apvs,  0,        vc4000,    vc4000,      0,        "Prinztronic",      "VC 6000",          GAME_IMPERFECT_GRAPHICS )					/* UK */
CONS(1979, database, 0,         vc4000,   vc4000,    vc4000,      0,        "Voltmace",         "Voltmace Database", GAME_IMPERFECT_GRAPHICS )				/* UK */
CONS(1979, vmdtbase, database,  0,        vc4000,    vc4000,      0,        "Videomaster",      "Videomaster Database Games-Computer", GAME_IMPERFECT_GRAPHICS )		/* UK */
CONS(1979, rwtrntcs, 0,         vc4000,   vc4000,    vc4000,      0,        "Rowtron",          "Rowtron Television Computer System", GAME_IMPERFECT_GRAPHICS )		/* UK */
CONS(1979, telngtcs, rwtrntcs,  0,        vc4000,    vc4000,      0,        "Teleng",           "Teleng Television Computer System", GAME_IMPERFECT_GRAPHICS )		/* UK */
CONS(1979, krvnjvtv, 0,         vc4000,   vc4000,    vc4000,      0,        "SOE",              "OC Jeu Video TV Karvan", GAME_IMPERFECT_GRAPHICS )				/* France */
CONS(1979, oc2000,   krvnjvtv,  0,        vc4000,    vc4000,      0,        "SOE",              "OC-2000",          GAME_IMPERFECT_GRAPHICS )					/* France */
CONS(1980, mpt05,    0,         vc4000,   vc4000,    vc4000,      0,        "ITMC",             "MPT-05",           GAME_IMPERFECT_GRAPHICS )					/* France */

/*  Game List and Emulation Status

When you load a game it will normally appear to be unresponsive. Most carts contain a number of variants
of each game (e.g. Difficulty, Player1 vs Player2 or Player1 vs Computer, etc).

Press F2 (if needed) to select which game variant you would like to play. The variant number will increment
on-screen. When you've made your choice, press F1 to start. The main keys are unlabelled, because an overlay
is provided with each cart. See below for a guide. You need to read the instructions that come with each game.

In some games, the joystick is used like 4 buttons, and other games like a paddle. The two modes are
incompatible when using a keyboard. Therefore (in the emulation) a config dipswitch is used. The preferred
setting is listed below.

(AC = Auto-centre, NAC = no auto-centre, 90 = turn controller 90 degrees).

The list is rather incomplete, information will be added as it becomes available.

The game names and numbers were obtained from the Amigan Software site.

Cart Num    Name
----------------------------------------------
1.      Grand Prix / Car Races / Autosport / Motor Racing / Road Race
Config: Paddle, NAC
Status: Working
Controls: Left-Right: Steer; Up: Accelerate

2.      Black Jack
Status: Not working (some digits missing; indicator missing; dealer's cards missing)
Controls: set bet with S and D; A to deal; 1 to hit, 2 to stay; Q accept insurance, E to decline; double-up (unknown key)
Indicator: E make a bet then deal; I choose insurance; - you lost; + you won; X hit or stay

3.      Olympics / Paddle Games / Bat & Ball / Pro Sport 60 / Sportsworld
Config: Paddle, NAC
Status: Working

4.      Tank Battle / Combat
Config: Button, 90
Status: Working
Controls: Left-Right: Steer; Up: Accelerate; Fire: Shoot

5.      Maths 1
Status: Working
Controls: Z difficulty; X = addition or subtraction; C ask question; A=1;S=2;D=3;Q=4;W=5;E=6;1=7;2=8;3=9;0=0; C enter

6.      Maths 2
Status: Not working
Controls: Same as above.

7.      Air Sea Attack / Air Sea Battle
Config: Button, 90
Status: Working
Controls: Left-Right: Move; Fire: Shoot

8.      Treasure Hunt / Capture the Flag / Concentration / Memory Match
Config: Buttons
Status: Working

9.      Labyrinth / Maze / Intelligence 1
Config: Buttons
Status: Working

10.     Winter Sports
Notes: Background colours should be Cyan and White instead of Red and Black

11.     Hippodrome / Horse Race

12.     Hunting / Shooting Gallery

13.     Chess 1
Status: Can't see what you're typing, wrong colours

14.     Moto-cros

15.     Four in a row / Intelligence 2
Config: Buttons
Status: Working
Notes: Seems the unused squares should be black. The screen jumps about while the computer is "thinking".

16.     Code Breaker / Master Mind / Intelligence 3 / Challenge

17.     Circus
STatus: severe gfx issues

18.     Boxing / Prize Fight

19.     Outer Space / Spacewar / Space Attack / Outer Space Combat

20.     Melody Simon / Musical Memory / Follow the Leader / Musical Games / Electronic Music / Face the Music

21.     Capture / Othello / Reversi / Attack / Intelligence 4
Config: Buttons
Status: Working
Notes: Seems the unused squares should be black

22.     Chess 2
Status: Can't see what you're typing, wrong colours

23.     Pinball / Flipper / Arcade
Status: gfx issues

24.     Soccer

25.     Bowling / NinePins
Config: Paddle, rotated 90 degrees, up/down autocentre, left-right does not
Status: Working

26.     Draughts

27.     Golf
Status: gfx issues

28.     Cockpit
Status: gfx issues

29.     Metropolis / Hangman
Status: gfx issues

30.     Solitaire

31.     Casino
Status: gfx issues, items missing and unplayable
Controls: 1 or 3=START; q=GO; E=STOP; D=$; Z=^; X=tens; C=units

32.     Invaders / Alien Invasion / Earth Invasion
Status: Works
Config: Buttons

33.     Super Invaders
Status: Stars are missing, colours are wrong
Config: Buttons (90)

36.     BackGammon
Status: Not all counters are visible, Dice & game number not visible.
Controls: Fire=Exec; 1=D+; 3=D-; Q,W,E=4,5,6; A,S,D=1,2,3; Z=CL; X=STOP; C=SET

37.     Monster Man / Spider's Web
Status: Works
Config: Buttons

38.     Hyperspace
Status: Works
Config: Buttons (90)
Controls: 3 - status button; Q,W,E,A,S,D,Z,X,C selects which galaxy to visit


40.     Super Space
Status: Works, some small gfx issues near the bottom
Config: Buttons



Acetronic: (dumps are compatible)
------------

* Shooting Gallery
Status: works but screen flickers
Config: Buttons

* Planet Defender
Status: Works
Config: Paddle (NAC)

* Laser Attack
Status: Works
Config: Buttons



Public Domain: (written for emulators, may not work on real hardware)
---------------
* Picture (no controls) - works
* Wincadia Stub (no controls) - works, small graphic error */
