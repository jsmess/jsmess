/*

	TODO:

	- fix keyboard
	- power on reset timeout?
	- reset on SPACE+RESET
	- DMA memory refresh
	- load .comx snapshots
	
	- tape
	- floppy
	- expansion box
	- expansion box w/F&M
	- 32K RAM card
	- F&M joy card
	- printer controller
	- printer controller w/F&M
	- F&M basic
	- PL-80 plotter
	- thermal printer
	- 80 column card with MC6845

*/

#include "driver.h"
#include "includes/comx35.h"
#include "cpu/cdp1802/cdp1802.h"
#include "devices/cassette.h"
#include "machine/cdp1871.h"
#include "video/cdp1869.h"

static const device_config *cassette_device_image(void)
{
	return image_from_devtype_and_index(IO_CASSETTE, 0);
}

/* Read/Write Handlers */

static WRITE8_HANDLER( bank_select_w )
{
	memory_set_bank(1, data & 0x07);
}

static WRITE8_HANDLER( system_io_w )
{
	logerror("System IO %02x\n", data);
}

/* Memory Maps */

static ADDRESS_MAP_START( comx35_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0xbfff) AM_RAM
	AM_RANGE(0xc000, 0xdfff) AM_RAMBANK(1)
	AM_RANGE(0xe000, 0xefff) AM_RAM
	AM_RANGE(0xf400, 0xf7ff) AM_DEVREADWRITE(CDP1869_VIDEO, CDP1869_TAG, cdp1869_charram_r, cdp1869_charram_w)
	AM_RANGE(0xf800, 0xffff) AM_DEVREADWRITE(CDP1869_VIDEO, CDP1869_TAG, cdp1869_pageram_r, cdp1869_pageram_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( comx35_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_WRITE(bank_select_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(system_io_w)
	AM_RANGE(0x03, 0x03) AM_DEVREAD(CDP1871, CDP1871_TAG, cdp1871_data_r) AM_DEVWRITE(CDP1869_VIDEO, CDP1869_TAG, cdp1869_out3_w)
	AM_RANGE(0x04, 0x04) AM_DEVWRITE(CDP1869_VIDEO, CDP1869_TAG, cdp1869_out4_w)
	AM_RANGE(0x05, 0x05) AM_DEVWRITE(CDP1869_VIDEO, CDP1869_TAG, cdp1869_out5_w)
	AM_RANGE(0x06, 0x06) AM_DEVWRITE(CDP1869_VIDEO, CDP1869_TAG, cdp1869_out6_w)
	AM_RANGE(0x07, 0x07) AM_DEVWRITE(CDP1869_VIDEO, CDP1869_TAG, cdp1869_out7_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( comx35 )
	PORT_START_TAG("D1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR(' ')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('´')

	PORT_START_TAG("D2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('[')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(']')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR(':')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR(';')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('<') PORT_CHAR('(')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=') PORT_CHAR('^')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR('>') PORT_CHAR(')')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\') PORT_CHAR('_')

	PORT_START_TAG("D3")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('?') PORT_CHAR('@')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G')

	PORT_START_TAG("D4")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('O')

	PORT_START_TAG("D5")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W')

	PORT_START_TAG("D6")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR('+') PORT_CHAR('{')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('-') PORT_CHAR('|')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH2) PORT_CHAR('*') PORT_CHAR('}')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_TILDE) PORT_CHAR('/') PORT_CHAR('~')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ') PORT_CHAR(8)

	PORT_START_TAG("D7")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC) PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x91") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x92") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x93") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("DEL") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("D8")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("D9")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("D10")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("D11")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("MODIFIERS")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL)) PORT_CHAR(UCHAR_MAMEKEY(RCONTROL))
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("RESET")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED ) PORT_NAME("RT") PORT_CODE(KEYCODE_F10) PORT_CHAR(UCHAR_MAMEKEY(F10))
INPUT_PORTS_END

/* CDP1802 Interface */

static TIMER_CALLBACK( dma_tick )
{
	comx35_state *state = machine->driver_data;

	if (!state->iden)
	{
	//	cpunum_set_input_line(device->machine, 0, CDP1802_INPUT_LINE_DMAOUT, ASSERT_LINE);
	//	cpunum_set_input_line(device->machine, 0, CDP1802_INPUT_LINE_DMAOUT, CLEAR_LINE);
	}
}

static CDP1802_MODE_READ( comx35_mode_r )
{
	comx35_state *state = machine->driver_data;

	return state->cdp1802_mode;
}

static CDP1802_EF_READ( comx35_ef_r )
{
	comx35_state *state = machine->driver_data;

	int flags = 0x0f;

	/*
        EF1     predis
        EF2     on power up: 1=PAL/0=NTSC, after toggle Q: _EFXB
        EF3     _EFXA
        EF4     cassette in (ear)
    */

	if (!state->cdp1869_prd) flags -= EF1;

	if (state->iden)
	{
		if (!state->pal_ntsc) flags -= EF2;
	}
	else
	{
		if (!state->cdp1871_efxb) flags -= EF2;
	}

	if (!state->cdp1871_efxa) flags -= EF3;

	if (cassette_input(cassette_device_image()) > +1.0) flags -= EF4;

	return flags;
}

static CDP1802_SC_WRITE( comx35_sc_w )
{
	if (state == CDP1802_STATE_CODE_S3_INTERRUPT)
	{
		cpunum_set_input_line(machine, 0, CDP1802_INPUT_LINE_INT, CLEAR_LINE);
	}
}

static CDP1802_Q_WRITE( comx35_q_w )
{
	comx35_state *state = machine->driver_data;

	if (state->iden && level)
	{
		state->iden = 0;
	}

	cassette_output(cassette_device_image(), level ? -1.0 : +1.0);
}

static const cdp1802_interface comx35_cdp1802_config =
{
	comx35_mode_r,
	comx35_ef_r,
	comx35_sc_w,
	comx35_q_w,
	NULL,
	NULL
};

/* CDP1871 Interface */

static CDP1871_ON_DA_CHANGED( comx35_da_w )
{
	comx35_state *state = device->machine->driver_data;

	state->cdp1871_efxa = level;
}

static CDP1871_ON_DA_CHANGED( comx35_rpt_w )
{
	comx35_state *state = device->machine->driver_data;

	state->cdp1871_efxb = level;
}

static CDP1871_INTERFACE( comx35n_cdp1871_intf )
{
	CDP1869_CPU_CLK_PAL / 8,
	comx35_da_w,
	comx35_rpt_w
};

static CDP1871_INTERFACE( comx35p_cdp1871_intf )
{
	CDP1869_CPU_CLK_NTSC / 8,
	comx35_da_w,
	comx35_rpt_w
};

/* Machine Initialization */

static TIMER_CALLBACK( reset_tick )
{
	comx35_state *state = machine->driver_data;

	state->cdp1802_mode = CDP1802_MODE_RUN;
}

static MACHINE_START( comx35p )
{
	comx35_state *state = machine->driver_data;

	// RAM banking

	memory_configure_bank(1, 0, 8, memory_region(REGION_USER1), 0x2000);
	memory_set_bank(1, 0);

	// allocate DMA timer

	state->dma_timer = timer_alloc(dma_tick, NULL);
	timer_adjust_periodic(state->dma_timer, attotime_zero, 0, ATTOTIME_IN_HZ(CDP1869_CPU_CLK_PAL / 8));

	// allocate reset timer
	
	state->reset_timer = timer_alloc(reset_tick, NULL);

	// screen format

	state->pal_ntsc = CDP1869_PAL;
}

static MACHINE_START( comx35n )
{
	comx35_state *state = machine->driver_data;

	MACHINE_START_CALL(comx35p);

	timer_adjust_periodic(state->dma_timer, attotime_zero, 0, ATTOTIME_IN_HZ(CDP1869_CPU_CLK_NTSC / 8));

	// screen format

	state->pal_ntsc = CDP1869_NTSC;
}

static MACHINE_RESET( comx35 )
{
	comx35_state *state = machine->driver_data;

	state->cdp1802_mode = CDP1802_MODE_RESET;
	state->iden = 1;

	timer_adjust_oneshot(state->reset_timer, ATTOTIME_IN_MSEC(200), 0);
}

/* Machine Drivers */

static MACHINE_DRIVER_START( comx35p )
	MDRV_DRIVER_DATA(comx35_state)

	// basic system hardware

	MDRV_CPU_ADD_TAG(CDP1802_TAG, CDP1802, CDP1869_CPU_CLK_PAL)
	MDRV_CPU_PROGRAM_MAP(comx35_map, 0)
	MDRV_CPU_IO_MAP(comx35_io_map, 0)
	MDRV_CPU_CONFIG(comx35_cdp1802_config)

	MDRV_MACHINE_START(comx35p)
	MDRV_MACHINE_RESET(comx35)

	// keyboard encoder

	MDRV_DEVICE_ADD(CDP1871_TAG, CDP1871)
	MDRV_DEVICE_CONFIG(comx35p_cdp1871_intf)
	
	// video hardware

	MDRV_IMPORT_FROM(comx35p_video)

	// sound hardware

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(CDP1869, CDP1869_DOT_CLK_PAL)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( comx35n )
	MDRV_DRIVER_DATA(comx35_state)

	// basic system hardware

	MDRV_CPU_ADD_TAG(CDP1802_TAG, CDP1802, CDP1869_CPU_CLK_NTSC)
	MDRV_CPU_PROGRAM_MAP(comx35_map, 0)
	MDRV_CPU_IO_MAP(comx35_io_map, 0)
	MDRV_CPU_CONFIG(comx35_cdp1802_config)

	MDRV_MACHINE_START(comx35n)
	MDRV_MACHINE_RESET(comx35)

	// keyboard encoder

	MDRV_DEVICE_ADD(CDP1871_TAG, CDP1871)
	MDRV_DEVICE_CONFIG(comx35n_cdp1871_intf)

	// video hardware

	MDRV_IMPORT_FROM(comx35n_video)

	// sound hardware

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(CDP1869, CDP1869_DOT_CLK_NTSC)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( comx35p )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_SYSTEM_BIOS( 0, "basic100", "COMX BASIC V1.00" )
	ROMX_LOAD( "comx_10.u21",			0x0000, 0x4000, CRC(68d0db2d) SHA1(062328361629019ceed9375afac18e2b7849ce47), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "default",	"COMX BASIC V1.01" )
	ROMX_LOAD( "comx_11.u21",			0x0000, 0x4000, CRC(609d89cd) SHA1(799646810510d8236fbfafaff7a73d5170990f16), ROM_BIOS(2) )

	ROM_REGION( 0x2000, REGION_USER1, 0 )
	ROM_LOAD( "expansion.bin",			0x0000, 0x1000, CRC(52cb44e2) SHA1(3f9a3d9940b36d4fee5eca9f1359c99d7ed545b9) )
	ROM_LOAD( "f&m.expansion.3.1.bin",	0x0000, 0x1000, CRC(818ca2ef) SHA1(ea000097622e7fd472d53e7899e3c83773433045) )
	ROM_LOAD( "printer.bin",			0x0000, 0x0800, CRC(3bbc2b2e) SHA1(08bf7ea4174713ab24969c553affd5c1401876b8) )
	ROM_LOAD( "thermal.bin",			0x0000, 0x1000, CRC(41a72ba8) SHA1(3a8760c78bd8c7bec2dbf26657b930c9a6814803) )
	ROM_LOAD( "f&m.printer.1.2.bin",	0x0000, 0x1000, CRC(2feb997d) SHA1(ee9cb91042696c88ff5f2f44d2f702dc93369ba0) )
	ROM_LOAD( "fdc.bin",				0x0000, 0x2000, CRC(cf4ecd2e) SHA1(290e19bdc89e3c8059e63d5ae3cca4daa194e1fe) )
	ROM_LOAD( "pl80.tiny.bin",			0x0000, 0x0400, CRC(940ec1ed) SHA1(ad83a3b57e2f0fbaa1e40644cd999b3f239635e8) )
	ROM_LOAD( "rs232.bin",				0x0000, 0x0800, CRC(926ff2d1) SHA1(be02bd388bba0211ea72d4868264a63308e4318d) )
	ROM_LOAD( "f&m.joycard.bin",		0x0000, 0x0002, CRC(a681ea11) SHA1(ff4baa8b79471f31667337c30345d642810776e1) )
ROM_END

ROM_START( comx35n )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_SYSTEM_BIOS( 0, "basic100", "COMX BASIC V1.00" )
	ROMX_LOAD( "comx_10.u21",			0x0000, 0x4000, CRC(68d0db2d) SHA1(062328361629019ceed9375afac18e2b7849ce47), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "default",	"COMX BASIC V1.01" )
	ROMX_LOAD( "comx_11.u21",			0x0000, 0x4000, CRC(609d89cd) SHA1(799646810510d8236fbfafaff7a73d5170990f16), ROM_BIOS(2) )

	ROM_REGION( 0x2000, REGION_USER1, 0 )
	ROM_LOAD( "expansion.bin",			0x0000, 0x1000, CRC(52cb44e2) SHA1(3f9a3d9940b36d4fee5eca9f1359c99d7ed545b9) )
	ROM_LOAD( "f&m.expansion.3.1.bin",	0x0000, 0x1000, CRC(818ca2ef) SHA1(ea000097622e7fd472d53e7899e3c83773433045) )
	ROM_LOAD( "printer.bin",			0x0000, 0x0800, CRC(3bbc2b2e) SHA1(08bf7ea4174713ab24969c553affd5c1401876b8) )
	ROM_LOAD( "thermal.bin",			0x0000, 0x1000, CRC(41a72ba8) SHA1(3a8760c78bd8c7bec2dbf26657b930c9a6814803) )
	ROM_LOAD( "f&m.printer.1.2.bin",	0x0000, 0x1000, CRC(2feb997d) SHA1(ee9cb91042696c88ff5f2f44d2f702dc93369ba0) )
	ROM_LOAD( "fdc.bin",				0x0000, 0x2000, CRC(cf4ecd2e) SHA1(290e19bdc89e3c8059e63d5ae3cca4daa194e1fe) )
	ROM_LOAD( "pl80.tiny.bin",			0x0000, 0x0400, CRC(940ec1ed) SHA1(ad83a3b57e2f0fbaa1e40644cd999b3f239635e8) )
	ROM_LOAD( "rs232.bin",				0x0000, 0x0800, CRC(926ff2d1) SHA1(be02bd388bba0211ea72d4868264a63308e4318d) )
	ROM_LOAD( "f&m.joycard.bin",		0x0000, 0x0002, CRC(a681ea11) SHA1(ff4baa8b79471f31667337c30345d642810776e1) )
ROM_END

/* System Configuration */

static void comx35_cassette_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:					info->i = 1; break;
		case MESS_DEVINFO_INT_CASSETTE_DEFAULT_STATE:	info->i = CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_MUTED; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START( comx35 )
	CONFIG_RAM_DEFAULT	(32 * 1024)
	CONFIG_RAM			(64 * 1024)
	CONFIG_DEVICE(comx35_cassette_getinfo)
SYSTEM_CONFIG_END

/* System Drivers */

//    YEAR  NAME		PARENT  COMPAT	MACHINE		INPUT     INIT	CONFIG    COMPANY						FULLNAME			FLAGS
COMP( 1983, comx35p,	0,		0,		comx35p,	comx35,   0, 	comx35,   "Comx World Operations Ltd",	"COMX 35 (PAL)",	GAME_NOT_WORKING )
COMP( 1983, comx35n,	comx35p,0,		comx35n,	comx35,   0, 	comx35,   "Comx World Operations Ltd",	"COMX 35 (NTSC)",	GAME_NOT_WORKING )
