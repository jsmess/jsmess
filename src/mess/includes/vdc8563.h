#ifndef __VDC8563_H_
#define __VDC8563_H_
/***************************************************************************

  CBM Video Device Chip 8563

  copyright 2000 peter.trauner@jk.uni-linz.ac.at

***************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/* call to init videodriver */
extern void vdc8563_init (int ram16konly);

extern void vdc8563_set_rastering(int on);

extern VIDEO_START( vdc8563 );
extern VIDEO_UPDATE( vdc8563 );

extern unsigned char vdc8563_palette[16 * 3];

/* to be called when writting to port */
extern WRITE8_HANDLER ( vdc8563_port_w );

/* to be called when reading from port */
extern  READ8_HANDLER ( vdc8563_port_r );

extern void vdc8563_state (void);

#ifdef __cplusplus
}
#endif

#endif
