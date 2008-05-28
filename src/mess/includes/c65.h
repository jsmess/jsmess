/*****************************************************************************
 *
 * includes/c65.h
 *
 ****************************************************************************/

#ifndef C65_H_
#define C65_H_


/*----------- defined in machine/c65.c -----------*/

/*extern UINT8 *c65_memory; */
/*extern UINT8 *c65_basic; */
/*extern UINT8 *c65_kernal; */
extern UINT8 *c65_chargen;
/*extern UINT8 *c65_dos; */
/*extern UINT8 *c65_monitor; */
extern UINT8 *c65_interface;
/*extern UINT8 *c65_graphics; */

void c65_bankswitch (void);
void c65_colorram_write (int offset, int value);

DRIVER_INIT( c65 );
DRIVER_INIT( c65pal );
MACHINE_START( c65 );


#endif /* C65_H_ */
