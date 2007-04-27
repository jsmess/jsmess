/***************************************************************************

    Atari Clay Shoot hardware

    driver by Zsolt Vasvari

****************************************************************************/

/*----------- defined in machine/clayshoo.c -----------*/

MACHINE_RESET( clayshoo );
WRITE8_HANDLER( clayshoo_analog_reset_w );
READ8_HANDLER( clayshoo_analog_r );
