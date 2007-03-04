/*
 * DS2401
 *
 * Dallas Semiconductor
 * Silicon Serial Number
 *
 */

#if !defined( DS2401_H )
#define DS2401_H ( 1 )

#define DS2401_MAXCHIP ( 3 )

extern void ds2401_init( int chip, unsigned char *data );
extern void ds2401_write( int chip, int data );
extern int ds2401_read( int chip );

#endif
