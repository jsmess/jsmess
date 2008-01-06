/*****************************************************************************
 *
 * includes/astrocde.h
 *
 * IBM AT Compatibles
 *
 ****************************************************************************/

#ifndef AT_H_
#define AT_H_


/*----------- defined in machine/at.c -----------*/

DRIVER_INIT( atcga );
DRIVER_INIT( at386 );
DRIVER_INIT( at586 );

DRIVER_INIT( at_vga );
DRIVER_INIT( ps2m30286 );

MACHINE_START( at );
MACHINE_RESET( at );


#endif /* AT_H_ */
