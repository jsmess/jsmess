#include "driver.h"
#include "cpu/z80/z80daisy.h"
#include "cpu/cop400/cop400.h"
#include "machine/nec765.h"
#include "machine/6850acia.h"
#include "machine/z80ctc.h"
#include "machine/z80dart.h"
#include "machine/z80sio.h"
#include "includes/newbrain.h"
#include "devices/basicdsk.h"
#include "devices/cassette.h"

/*

	NewBrain
	Grundy Business Systems Ltd.	

	32K RAM
	28K ROM

	Z80 @ 2MHz
	COP420 @ 2MHz

	Z80 @ 4MHz (416): INT/NMI=+5V, WAIT=EXTBUSRQ|BUSAKD, RESET=_FDC RESET, 
	NEC 765AC @ 4 MHz (418)
	MC6850 ACIA (459)
	Z80CTC (458)

*/

/*

	TODO:

	- escape key
	- main z80 interrupt from COP420
	- video
	- tape

	- floppy disc controller
	- CP/M 2.2
	- expansion box
	- Micropage ROM/RAM card
	- Z80 PIO board
	- peripheral (PI) box
	- sound card

*/

static READ8_HANDLER( newbrain_cop_l_r )
{
	// connected to the Z80 data bus

	newbrain_state *state = machine->driver_data;

	return state->cop_bus;
}

static WRITE8_HANDLER( newbrain_cop_l_w )
{
	// connected to the Z80 data bus

	newbrain_state *state = machine->driver_data;

	state->cop_bus = data;
}

static WRITE8_HANDLER( newbrain_cop_g_w )
{
	/*

		bit		description
		
		G0		inverted to _COPINT
		G1		
		G2		
		G3		

	*/

	// _Z80INT = ((_CLK | _CLKINT) & _COPINT)
}

static READ8_HANDLER( newbrain_cop_g_r )
{
	/*

		bit		description
		
		G0		
		G1		K9
		G2		K7
		G3		K3

	*/

	newbrain_state *state = machine->driver_data;

	return (BIT(state->keydata, 3) << 3) | (BIT(state->keydata, 0) << 2) | (BIT(state->keydata, 1) << 1);
}

static READ8_HANDLER( newbrain_cop_d_r )
{
	/*

		bit		description
		
		D0		inverted to K4
		D1		TDO
		D2		inverted to K6
		D3		

	*/

	return 0;
}

static WRITE8_HANDLER( newbrain_cop_d_w )
{
	/*

		bit		description
		
		D0		inverted to K4 -> CD4024 pin 2 (reset)
		D1		TDO
		D2		inverted to K6 -> CD4024 pin 1 (clock), CD4076 pin 7 (clock), inverted to DS8881 pin 3 (enable)
		D3		

	*/

	newbrain_state *state = machine->driver_data;

	if (!BIT(data, 0))
	{
		state->keylatch = 0;
	}

	if (!BIT(data, 2))
	{
		char port[4];

		state->keylatch++;
		
		if (state->keylatch == 16)
		{
			state->keylatch = 0;
		}

		sprintf(port, "D%d", state->keylatch);
		
		state->keydata = input_port_read(machine, port);
	}
}

static READ8_HANDLER( newbrain_cop_in_r )
{
	/*

		bit		description
		
		IN0		K8
		IN1		_RD
		IN2		_COP
		IN3		_WR

	*/

	newbrain_state *state = machine->driver_data;

	return BIT(state->keydata, 2);
}

static WRITE8_HANDLER( newbrain_cop_so_w )
{
	// connected to K1
}

static WRITE8_HANDLER( newbrain_cop_sk_w )
{
	// connected to K2
}

static READ8_HANDLER( newbrain_cop_si_r )
{
	// connected to TDI

	return 0;
}

#ifdef UNUSED_CODE
static WRITE8_HANDLER( fdc_io_w )
{
	/*

		bit		description

		0		PAGING
		1		
		2		MA16
		3		MPM
		4		
		5		_FDC RESET
		6		
		7		FDC ATT

	*/
}

static WRITE8_HANDLER( fdc_io2_w )
{
	/*

		bit		description

		0		MOTON
		1		765 RESET
		2		TC
		3		
		4		
		5		PA15
		6		
		7		

	*/

	floppy_drive_set_motor_state(image_from_devtype_and_index(IO_FLOPPY, 0), BIT(data, 0));
	floppy_drive_set_ready_state(image_from_devtype_and_index(IO_FLOPPY, 0), 1, 0);

	nec765_set_reset_state(BIT(data, 1));

	nec765_set_tc_state(BIT(data, 2));
}

static WRITE8_HANDLER( io_w )
{
	/*

		bit		description

		0		_USERP
		1		ANP
		2		MLTMD
		3		MSPD
		4		ENOR
		5		ANSW
		6		ENOT
		7		

	*/
}
#endif

/* Memory Maps */

static ADDRESS_MAP_START( newbrain_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x7fff) AM_RAM
	AM_RANGE(0xa000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( newbrain_io_map, ADDRESS_SPACE_IO, 8 )
ADDRESS_MAP_END

static ADDRESS_MAP_START( newbrain_cop_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x000, 0x3ff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( newbrain_cop_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(COP400_PORT_L, COP400_PORT_L) AM_READWRITE(newbrain_cop_l_r, newbrain_cop_l_w)
	AM_RANGE(COP400_PORT_G, COP400_PORT_G) AM_READWRITE(newbrain_cop_g_r, newbrain_cop_g_w)
	AM_RANGE(COP400_PORT_D, COP400_PORT_D) AM_READWRITE(newbrain_cop_d_r, newbrain_cop_d_w)
	AM_RANGE(COP400_PORT_IN, COP400_PORT_IN) AM_READ(newbrain_cop_in_r)
	AM_RANGE(COP400_PORT_SK, COP400_PORT_SK) AM_WRITE(newbrain_cop_sk_w)
	AM_RANGE(COP400_PORT_SIO, COP400_PORT_SIO) AM_READWRITE(newbrain_cop_si_r, newbrain_cop_so_w)
	AM_RANGE(COP400_PORT_CKO, COP400_PORT_CKO) AM_READNOP
ADDRESS_MAP_END

static ADDRESS_MAP_START( newbrain_fdc_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0xe000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( newbrain_fdc_io_map, ADDRESS_SPACE_IO, 8 )
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( newbrain )
	PORT_START_TAG("D0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("STOP") PORT_CODE(KEYCODE_END) PORT_CHAR(UCHAR_MAMEKEY(END))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("D1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x93") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x92") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x91") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	
	PORT_START_TAG("D2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	
	PORT_START_TAG("D3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('N')

	PORT_START_TAG("D4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M')

	PORT_START_TAG("D5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('(') PORT_CHAR('[')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	
	PORT_START_TAG("D6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(')') PORT_CHAR(']')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	
	PORT_START_TAG("D7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CHAR(';') PORT_CHAR(':')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("* \xC2\xA3") PORT_CHAR('*') PORT_CHAR(0x00A3)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	
	PORT_START_TAG("D8")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("VIDEO TEXT") PORT_CODE(KEYCODE_RALT) PORT_CHAR(UCHAR_MAMEKEY(RALT)) // Vd
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	
	PORT_START_TAG("D9")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	
	PORT_START_TAG("D10")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=') PORT_CHAR('@')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('X')

	PORT_START_TAG("D11")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('\\')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	
	PORT_START_TAG("D12")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CHAR('+') PORT_CHAR('^')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("INSERT") PORT_CODE(KEYCODE_INSERT) PORT_CHAR(UCHAR_MAMEKEY(INSERT))
	
	PORT_START_TAG("D13")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("NEW LINE") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13) // NL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	
	PORT_START_TAG("D14")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("HOME") PORT_CODE(KEYCODE_HOME) PORT_CHAR(UCHAR_MAMEKEY(HOME)) // CH
	
	PORT_START_TAG("D15")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1) // SH
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("GRAPHICS") PORT_CODE(KEYCODE_LALT) PORT_CHAR(UCHAR_MAMEKEY(LALT)) // GR
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("REPEAT") // RPT
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CONTROL") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL)) // GL
INPUT_PORTS_END

/* Video */

static VIDEO_START( newbrain )
{
}

static VIDEO_UPDATE( newbrain )
{
	return 0;
}

/* */

static int acia_irq;
static UINT8 rxd = 1, txd = 1;

static void acia_interrupt(int state)
{
	acia_irq = state;
}

static const struct acia6850_interface newbrain_acia_intf =
{
	500000, // ???
	500000, // ???
	&rxd,
	&txd,
	NULL,
	NULL,
	NULL,
	acia_interrupt
};

static void newbrain_fdc_interrupt(int state)
{
}

static const struct nec765_interface newbrain_nec765_interface =
{
	newbrain_fdc_interrupt,
	NULL
};

MACHINE_START( newbrain )
{
	nec765_init(&newbrain_nec765_interface, NEC765A, NEC765_RDY_PIN_NOT_CONNECTED);
	acia6850_config(0, &newbrain_acia_intf);
}

MACHINE_RESET( newbrain )
{
}

static const struct z80_irq_daisy_chain newbrain_daisy_chain[] =
{
	{ z80ctc_reset, z80ctc_irq_state, z80ctc_irq_ack, z80ctc_irq_reti, 0 },
	{ 0, 0, 0, 0, -1 }
};

/* Machine Drivers */

static MACHINE_DRIVER_START( newbrain )
	MDRV_DRIVER_DATA(newbrain_state)

	// basic system hardware

	MDRV_CPU_ADD_TAG("main", Z80, XTAL_16MHz/8)
	MDRV_CPU_PROGRAM_MAP(newbrain_map, 0)
	MDRV_CPU_IO_MAP(newbrain_io_map, 0)

	MDRV_CPU_ADD_TAG("cop", COP420, XTAL_16MHz/8)
	MDRV_CPU_PROGRAM_MAP(newbrain_cop_map, 0)
	MDRV_CPU_IO_MAP(newbrain_cop_io_map, 0)

	MDRV_CPU_ADD_TAG("fdc", Z80, XTAL_4MHz)
	MDRV_CPU_PROGRAM_MAP(newbrain_fdc_map, 0)
	MDRV_CPU_IO_MAP(newbrain_fdc_io_map, 0)

	MDRV_MACHINE_START(newbrain)
	MDRV_MACHINE_RESET(newbrain)

	// video hardware

	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_SIZE(640, 400)
	MDRV_SCREEN_VISIBLE_AREA(0, 639, 0, 399)

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(newbrain)
	MDRV_VIDEO_UPDATE(newbrain)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( newbraid )
	MDRV_IMPORT_FROM(newbrain)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( newbramd )
	MDRV_IMPORT_FROM(newbrain)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( newbrain )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_SYSTEM_BIOS( 0, "default", "ROM 2.0" )
	ROMX_LOAD( "aben20.rom", 0xa000, 0x2000, CRC(3d76d0c8) SHA1(753b4530a518ad832e4b81c4e5430355ba3f62e0), ROM_BIOS(1) )
	ROMX_LOAD( "cd20tci.rom", 0xc000, 0x4000, CRC(f65b2350) SHA1(1ada7fbf207809537ec1ffb69808524300622ada), ROM_BIOS(1) )

	ROM_SYSTEM_BIOS( 1, "rom191", "ROM 1.91" )
	ROMX_LOAD( "aben191.rom", 0xa000, 0x2000, CRC(b7be8d89) SHA1(cce8d0ae7aa40245907ea38b7956c62d039d45b7), ROM_BIOS(2) )
	ROMX_LOAD( "cd.rom", 0xc000, 0x2000, CRC(6b4d9429) SHA1(ef688be4e75aced61f487c928258c8932a0ae00a), ROM_BIOS(2) )
	ROMX_LOAD( "ef1x.rom", 0xe000, 0x2000, CRC(20dd0b49) SHA1(74b517ca223cefb588e9f49e72ff2d4f1627efc6), ROM_BIOS(2) )

	ROM_SYSTEM_BIOS( 2, "rom19", "ROM 1.9" )
	ROMX_LOAD( "aben19.rom", 0xa000, 0x2000, CRC(d0283eb1) SHA1(351d248e69a77fa552c2584049006911fb381ff0), ROM_BIOS(3) )
	ROMX_LOAD( "cd.rom", 0xc000, 0x2000, CRC(6b4d9429) SHA1(ef688be4e75aced61f487c928258c8932a0ae00a), ROM_BIOS(3) )
	ROMX_LOAD( "ef1x.rom", 0xe000, 0x2000, CRC(20dd0b49) SHA1(74b517ca223cefb588e9f49e72ff2d4f1627efc6), ROM_BIOS(3) )

	ROM_SYSTEM_BIOS( 3, "rom14", "ROM 1.4" )
	ROMX_LOAD( "aben14.rom", 0xa000, 0x2000, CRC(d0283eb1) SHA1(351d248e69a77fa552c2584049006911fb381ff0), ROM_BIOS(4) )
	ROMX_LOAD( "cd.rom", 0xc000, 0x2000, CRC(6b4d9429) SHA1(ef688be4e75aced61f487c928258c8932a0ae00a), ROM_BIOS(4) )
	ROMX_LOAD( "ef1x.rom", 0xe000, 0x2000, CRC(20dd0b49) SHA1(74b517ca223cefb588e9f49e72ff2d4f1627efc6), ROM_BIOS(4) )

	ROM_REGION( 0x400, REGION_CPU2, 0 )
	ROM_LOAD( "cop420.419", 0x000, 0x400, NO_DUMP )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )
	ROM_LOAD( "d417-2.417", 0xe000, 0x2000, CRC(e8bda8b9) SHA1(c85a76a5ff7054f4ef4a472ce99ebaed1abd269c) )
ROM_END

#define rom_newbraid rom_newbrain

ROM_START( newbramd )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "cdmd.rom", 0xc000, 0x2000, CRC(6b4d9429) SHA1(ef688be4e75aced61f487c928258c8932a0ae00a) )
	ROM_LOAD( "efmd.rom", 0xe000, 0x2000, CRC(20dd0b49) SHA1(74b517ca223cefb588e9f49e72ff2d4f1627efc6) )

	ROM_REGION( 0x400, REGION_CPU2, 0 )
	ROM_LOAD( "cop420.419", 0x000, 0x400, NO_DUMP )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) // TODO: remove this
	ROM_LOAD( "d417-2.417", 0xe000, 0x2000, CRC(e8bda8b9) SHA1(c85a76a5ff7054f4ef4a472ce99ebaed1abd269c) )
ROM_END

/* System Configuration */

static void newbrain_cassette_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:					info->i = 2; break;
		case MESS_DEVINFO_INT_CASSETTE_DEFAULT_STATE:	info->i = CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_MUTED; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}

static DEVICE_IMAGE_LOAD( newbrain_floppy )
{
	if (image_has_been_created(image))
		return INIT_FAIL;

	if (device_load_basicdsk_floppy(image) == INIT_PASS)
	{
		// 180K
	}

	return INIT_PASS;
}

static void newbrain_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:					info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:						info->load = DEVICE_IMAGE_LOAD_NAME(newbrain_floppy); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:			strcpy(info->s = device_temp_str(), "img"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START( newbrain )
	CONFIG_RAM_DEFAULT	(32 * 1024)
	CONFIG_DEVICE(newbrain_cassette_getinfo)
	CONFIG_DEVICE(newbrain_floppy_getinfo)
SYSTEM_CONFIG_END

/* System Drivers */

//    YEAR  NAME		PARENT		COMPAT	MACHINE		INPUT		INIT	CONFIG		COMPANY							FULLNAME		FLAGS
COMP( 1981, newbrain,	0,			0,		newbrain,	newbrain,   0, 		newbrain,	"Grundy Business Systems Ltd.",	"NewBrain A",	GAME_NOT_WORKING | GAME_NO_SOUND )
COMP( 1981, newbraid,	newbrain,	0,		newbraid,	newbrain,   0, 		newbrain,	"Grundy Business Systems Ltd.",	"NewBrain AD",	GAME_NOT_WORKING | GAME_NO_SOUND )
COMP( 1981, newbramd,	newbrain,	0,		newbramd,	newbrain,   0, 		newbrain,	"Grundy Business Systems Ltd.",	"NewBrain MD",	GAME_NOT_WORKING | GAME_NO_SOUND )
