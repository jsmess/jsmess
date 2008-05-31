/*****************************************************************************
 *
 * includes/radio86.h
 *
 ****************************************************************************/

#ifndef radio86_H_
#define radio86_H_


/*----------- defined in machine/radio86.c -----------*/

extern DRIVER_INIT( radio86 );
extern MACHINE_RESET( radio86 );

extern const ppi8255_interface radio86_ppi8255_interface_1;
extern const i8275_interface radio86_i8275_interface;

/*----------- defined in video/radio86.c -----------*/

extern I8275_DISPLAY_PIXELS(radio86_display_pixels);

extern VIDEO_UPDATE( radio86 );
extern PALETTE_INIT( radio86 );


#endif /* radio86_H_ */
