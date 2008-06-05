/*****************************************************************************
 *
 * includes/apogee.h
 *
 ****************************************************************************/

#ifndef APOGEE_H_
#define APOGEE_H_


/*----------- defined in machine/apogee.c -----------*/

extern DRIVER_INIT( apogee );
extern MACHINE_RESET( apogee );

extern const ppi8255_interface apogee_ppi8255_interface_1;
extern const i8275_interface apogee_i8275_interface;

/*----------- defined in video/apogee.c -----------*/

extern I8275_DISPLAY_PIXELS(apogee_display_pixels);

extern VIDEO_UPDATE( apogee );
extern PALETTE_INIT( apogee );


#endif /* APOGEE_H_ */
