/***************************************************************************

Emulation file for Keytronic AT 4270 keyboard.

This keyboard supports both the PC/XT and the PC/AT keyboard protocols. The
desired protocol can be selected by a switch on the keybaord.

***************************************************************************/

#include "driver.h"


static UINT8	kb_keytronic_p1;
static UINT8	kb_keytronic_p2;
static UINT8	kb_keytronic_p3;
static UINT8	kb_clock_signal;
static UINT8	kb_data_signal;


/* Write handler which is called when the clock signal may have changed */
WRITE8_HANDLER( kb_keytronic_set_clock )
{
	kb_clock_signal = data;
}


/* Write handler which is called when the data signal may have changed */
WRITE8_HANDLER( kb_keytronic_set_data )
{
	kb_data_signal = data;
}


static READ8_HANDLER( kb_keytronic_p1_r )
{
	UINT8 data = kb_keytronic_p1;

	return data;
}


static WRITE8_HANDLER( kb_keytronic_p1_w )
{
	logerror("kb_keytronic_p1_w(): write %02x\n", data );

	kb_keytronic_p1 = data;
}


static READ8_HANDLER( kb_keytronic_p2_r )
{
	UINT8 data = kb_keytronic_p2;

	return data;
}


static WRITE8_HANDLER( kb_keytronic_p2_w )
{
	logerror("kb_keytronic_p2_w(): write %02x\n", data );

	kb_keytronic_p2 = data;
}


static READ8_HANDLER( kb_keytronic_p3_r )
{
	UINT8 data = kb_keytronic_p3;

	return data;
}


static WRITE8_HANDLER( kb_keytronic_p3_w )
{
	logerror("kb_keytronic_p3_w(): write %02x\n", data );

	kb_keytronic_p3 = data;
}


static READ8_HANDLER( kb_keytronic_data_r )
{
	logerror("kb_keytronic_data_r(): read from %04x\n", offset );
	return 0xFF;
}


static WRITE8_HANDLER( kb_keytronic_data_w )
{
	kb_data_signal = ( offset & 0x0100 ) ? 1 : 0;
	kb_clock_signal = ( offset & 0x0200 ) ? 1 : 0;
}


static ADDRESS_MAP_START( kb_keytronic_program, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE( 0x0000, 0x0FFF )	AM_ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START( kb_keytronic_data, ADDRESS_SPACE_DATA, 8 )
	AM_RANGE( 0x0000, 0xFFFF ) AM_READWRITE( kb_keytronic_data_r, kb_keytronic_data_w )
ADDRESS_MAP_END


static ADDRESS_MAP_START( kb_keytronic_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE( 0x01, 0x01 )	AM_READWRITE( kb_keytronic_p1_r, kb_keytronic_p1_w )
	AM_RANGE( 0x02, 0x02 )	AM_READWRITE( kb_keytronic_p2_r, kb_keytronic_p2_w )
	AM_RANGE( 0x03, 0x03 )	AM_READWRITE( kb_keytronic_p3_r, kb_keytronic_p3_w )
ADDRESS_MAP_END


MACHINE_DRIVER_START( kb_keytronic )
	MDRV_CPU_ADD_TAG( "kb_keytronic_", I8051, 11060250 )
	MDRV_CPU_PROGRAM_MAP( kb_keytronic_program, 0 )
	MDRV_CPU_DATA_MAP( kb_keytronic_data, 0 )
	MDRV_CPU_IO_MAP( kb_keytronic_io, 0 )
MACHINE_DRIVER_END


