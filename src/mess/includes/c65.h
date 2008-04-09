/*****************************************************************************
 *
 * includes/c65.h
 *
 ****************************************************************************/

#ifndef C65_H_
#define C65_H_


#define C65_KEY_DIN ( input_port_read (machine, "Special") & 0x20 )


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

void c65_driver_init (void);
void c65_driver_alpha1_init (void);
void c65pal_driver_init (void);
MACHINE_START( c65 );


#endif /* C65_H_ */
