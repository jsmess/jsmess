/***************************************************************************
   
        MMD-1 driver by Miodrag Milanovic
		
        12/05/2009 Initial version

****************************************************************************/

#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "mmd1.lh"

WRITE8_HANDLER (mmd1_port0_w)
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

WRITE8_HANDLER (mmd1_port1_w)
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

WRITE8_HANDLER (mmd1_port2_w)
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

READ8_HANDLER (mmd1_keyboard_r)
{
	UINT8 line1 = input_port_read(space->machine,"LINE1");
	UINT8 line2 = input_port_read(space->machine,"LINE2");
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

static ADDRESS_MAP_START(mmd1_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x00ff ) AM_ROM // Main ROM
	AM_RANGE( 0x0100, 0x01ff ) AM_ROM // Expansion slot
	AM_RANGE( 0x0200, 0x02ff ) AM_RAM
	AM_RANGE( 0x0300, 0x03ff ) AM_RAM	
ADDRESS_MAP_END

static ADDRESS_MAP_START( mmd1_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0x07)
	AM_RANGE( 0x00, 0x00 ) AM_READWRITE(mmd1_keyboard_r, mmd1_port0_w)
	AM_RANGE( 0x01, 0x01 ) AM_WRITE(mmd1_port1_w)
	AM_RANGE( 0x02, 0x02 ) AM_WRITE(mmd1_port2_w)
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( mmd1 )
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


static MACHINE_RESET(mmd1) 
{	
}

static MACHINE_DRIVER_START( mmd1 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",8080, 6750000 / 9)
    MDRV_CPU_PROGRAM_MAP(mmd1_mem)
    MDRV_CPU_IO_MAP(mmd1_io)	

    MDRV_MACHINE_RESET(mmd1)
	
	/* video hardware */
	MDRV_DEFAULT_LAYOUT(layout_mmd1)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(mmd1)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( mmd1 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
    ROM_LOAD( "kex.ic15", 0x0000, 0x0100, CRC(434f6923) SHA1(a2af60deda54c8d3f175b894b47ff554eb37e9cb))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( 1976, mmd1,  0,       0, 	mmd1, 	mmd1, 	 0,  	  mmd1,  	 "E&L Instruments, Inc.",   "MMD-1",		0)

