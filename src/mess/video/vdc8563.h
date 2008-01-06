/*****************************************************************************
 *
 * video/vdc8563.h
 *
 * CBM Video Device Chip 8563
 *
 * peter.trauner@jk.uni-linz.ac.at, 2000
 *
 ****************************************************************************/

#ifndef VDC8563_H_
#define VDC8563_H_

#ifdef __cplusplus
extern "C" {
#endif


/*----------- defined in video/vdc8563.c -----------*/

/* call to init videodriver */
extern void vdc8563_init (int ram16konly);

extern void vdc8563_set_rastering(int on);

extern VIDEO_START( vdc8563 );
extern VIDEO_UPDATE( vdc8563 );

extern const unsigned char vdc8563_palette[16 * 3];

/* to be called when writting to port */
extern WRITE8_HANDLER ( vdc8563_port_w );

/* to be called when reading from port */
extern  READ8_HANDLER ( vdc8563_port_r );


#ifdef __cplusplus
}
#endif

#endif /* VDC8563_H_ */
