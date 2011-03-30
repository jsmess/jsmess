/***************************************************************************

        MMD-1 & MMD-2 driver by Miodrag Milanovic

        12/05/2009 Initial version

****************************************************************************/

#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "mmd1.lh"
#include "mmd2.lh"


class mmd1_state : public driver_device
{
public:
	mmd1_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};


static WRITE8_HANDLER (mmd1_port0_w)
{
	output_set_value("p0_7", BIT(data,7) ? 0 : 1);
	output_set_value("p0_6", BIT(data,6) ? 0 : 1);
	output_set_value("p0_5", BIT(data,5) ? 0 : 1);
	output_set_value("p0_4", BIT(data,4) ? 0 : 1);
	output_set_value("p0_3", BIT(data,3) ? 0 : 1);
	output_set_value("p0_2", BIT(data,2) ? 0 : 1);
	output_set_value("p0_1", BIT(data,1) ? 0 : 1);
	output_set_value("p0_0", BIT(data,0) ? 0 : 1);
}

static WRITE8_HANDLER (mmd1_port1_w)
{
	output_set_value("p1_7", BIT(data,7) ? 0 : 1);
	output_set_value("p1_6", BIT(data,6) ? 0 : 1);
	output_set_value("p1_5", BIT(data,5) ? 0 : 1);
	output_set_value("p1_4", BIT(data,4) ? 0 : 1);
	output_set_value("p1_3", BIT(data,3) ? 0 : 1);
	output_set_value("p1_2", BIT(data,2) ? 0 : 1);
	output_set_value("p1_1", BIT(data,1) ? 0 : 1);
	output_set_value("p1_0", BIT(data,0) ? 0 : 1);
}

static WRITE8_HANDLER (mmd1_port2_w)
{
	output_set_value("p2_7", BIT(data,7) ? 0 : 1);
	output_set_value("p2_6", BIT(data,6) ? 0 : 1);
	output_set_value("p2_5", BIT(data,5) ? 0 : 1);
	output_set_value("p2_4", BIT(data,4) ? 0 : 1);
	output_set_value("p2_3", BIT(data,3) ? 0 : 1);
	output_set_value("p2_2", BIT(data,2) ? 0 : 1);
	output_set_value("p2_1", BIT(data,1) ? 0 : 1);
	output_set_value("p2_0", BIT(data,0) ? 0 : 1);
}

static READ8_HANDLER (mmd1_keyboard_r)
{
	UINT8 line1 = input_port_read(space->machine(),"LINE1");
	UINT8 line2 = input_port_read(space->machine(),"LINE2");
	if (BIT(line1,0)==0) return 0;
	if (BIT(line1,1)==0) return 1;
	if (BIT(line1,2)==0) return 2;
	if (BIT(line1,3)==0) return 3;
	if (BIT(line1,4)==0) return 4;
	if (BIT(line1,5)==0) return 5;
	if (BIT(line1,6)==0) return 6;
	if (BIT(line1,7)==0) return 7;

	if (BIT(line2,0)==0) return 8;
	if (BIT(line2,2)==0) return 10;
	if (BIT(line2,3)==0) return 11;
	if (BIT(line2,4)==0) return 12;
	if (BIT(line2,5)==0) return 13;
	if (BIT(line2,6)==0) return 14;
	if (BIT(line2,7)==0) return 15;

	return 0xff;
}

static ADDRESS_MAP_START(mmd1_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x00ff ) AM_ROM // Main ROM
	AM_RANGE( 0x0100, 0x01ff ) AM_ROM // Expansion slot
	AM_RANGE( 0x0200, 0x02ff ) AM_RAM
	AM_RANGE( 0x0300, 0x03ff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( mmd1_io , AS_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0x07)
	AM_RANGE( 0x00, 0x00 ) AM_READWRITE(mmd1_keyboard_r, mmd1_port0_w)
	AM_RANGE( 0x01, 0x01 ) AM_WRITE(mmd1_port1_w)
	AM_RANGE( 0x02, 0x02 ) AM_WRITE(mmd1_port2_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(mmd2_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x03ff ) AM_RAM // WE0
	AM_RANGE( 0x0400, 0x07ff ) AM_RAM // WE1
	AM_RANGE( 0x0800, 0x0bff ) AM_RAM // WE2
	AM_RANGE( 0x0c00, 0x0fff ) AM_RAM // WE3

	AM_RANGE( 0xd800, 0xdfff ) AM_ROM // ROM label 330
	AM_RANGE( 0xe000, 0xe7ff ) AM_ROM // ROM label 340
	AM_RANGE( 0xe800, 0xefff ) AM_RAM // ROM label 350
	AM_RANGE( 0xf000, 0xf7ff ) AM_RAM // ROM label 360
	AM_RANGE( 0xfc00, 0xfcff ) AM_RAM // System RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( mmd2_io , AS_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END


/* Input ports */
static INPUT_PORTS_START( mmd1 )
	PORT_START("LINE1")
			PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)
			PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)
			PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)
			PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)
			PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)
			PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)
			PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)
			PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)
	PORT_START("LINE2")
			PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)
			PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
			PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
			PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)
			PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)
			PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)
			PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
			PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)
	PORT_START("LINE3")
			PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)
INPUT_PORTS_END


static INPUT_PORTS_START( mmd2 )
/*
DIP switches:
WE 0      - Write enable for addresses $0000..$03FF
WE 1      - Write enable for addresses $0400..$07FF
WE 2      - Write enable for addresses $0800..$0BFF
WE 3      - Write enable for addresses $0C00..$0FFF
SPARE     - ???
HEX OCT   - choose display and entry to be in Hexadecimal or Octal
PUP RESET - ???
EXEC USER - update binary LED's with data entry? Or not?
(in either setting, outputs to ports 0,1,2 still show)

Keyboard
0  1  2  3      PREV  STORE  NEXT  STEP
4  5  6  7      HIGH  LOW  GO  OPTION
8  9  A  B      LOAD  DUMP  PROM  COPY
C  D  E  F      MEM  REGS  AUX  CANCEL

*/
INPUT_PORTS_END

static MACHINE_RESET(mmd1)
{
}

static MACHINE_RESET(mmd2)
{
}

static MACHINE_CONFIG_START( mmd1, mmd1_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",I8080, 6750000 / 9)
    MCFG_CPU_PROGRAM_MAP(mmd1_mem)
    MCFG_CPU_IO_MAP(mmd1_io)

    MCFG_MACHINE_RESET(mmd1)

	/* video hardware */
	MCFG_DEFAULT_LAYOUT(layout_mmd1)
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( mmd2, mmd1_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",I8080, 6750000 / 9)
    MCFG_CPU_PROGRAM_MAP(mmd2_mem)
    MCFG_CPU_IO_MAP(mmd2_io)

    MCFG_MACHINE_RESET(mmd2)

	/* video hardware */
	MCFG_DEFAULT_LAYOUT(layout_mmd2)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( mmd1 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
    ROM_LOAD( "kex.ic15", 0x0000, 0x0100, CRC(434f6923) SHA1(a2af60deda54c8d3f175b894b47ff554eb37e9cb))
ROM_END

ROM_START( mmd2 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "mmd2330.bin", 0xd800, 0x0800, CRC(69a77199) SHA1(6c83093b2c32a558c969f4fe8474b234023cc348))
	ROM_LOAD( "mmd2340.bin", 0xe000, 0x0800, CRC(70681bd6) SHA1(c37e3cf34a75e8538471030bb49b8aed45d00ec3))
	ROM_LOAD( "mmd2350.bin", 0xe800, 0x0800, CRC(359f577c) SHA1(9405ca0c1977721e4540a4017907c06dab08d398))
	ROM_LOAD( "mmd2360.bin", 0xf000, 0x0800, CRC(967e69b8) SHA1(c21ec8bef955806a2c6e1b1c8e9068662fb88038))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1976, mmd1,	0,       0, 		mmd1,	mmd1,	 0, 	 "E&L Instruments Inc",   "MMD-1",		GAME_NO_SOUND)
COMP( 1976, mmd2,	mmd1,    0, 		mmd2,	mmd2,	 0, 	 "E&L Instruments Inc",   "MMD-2",		GAME_NOT_WORKING | GAME_NO_SOUND)

