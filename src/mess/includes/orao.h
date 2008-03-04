/*****************************************************************************
 *
 * includes/orao.h
 *
 ****************************************************************************/

#ifndef ORAO_H_
#define ORAO_H_


/*----------- defined in machine/orao.c -----------*/

extern UINT8 *orao_memory;

extern DRIVER_INIT( orao );
extern MACHINE_RESET( orao );

extern DRIVER_INIT( orao103 );

extern READ8_HANDLER( orao_io_r );
extern WRITE8_HANDLER( orao_io_w );


/*----------- defined in video/orao.c -----------*/

extern const unsigned char orao_palette[2*3];
extern PALETTE_INIT( orao );
extern VIDEO_START( orao );
extern VIDEO_UPDATE( orao );


#endif /* ORAO_H_ */
