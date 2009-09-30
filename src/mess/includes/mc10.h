/*****************************************************************************
 *
 * includes/mc10.h
 *
 * TRS-80 Radio Shack MicroColor Computer
 *
 ****************************************************************************/

#ifndef MC10_H_
#define MC10_H_


/*----------- defined in machine/mc10.c -----------*/

READ8_HANDLER ( mc10_bfff_r );
WRITE8_HANDLER ( mc10_bfff_w );
READ8_HANDLER ( mc10_port1_r );
READ8_HANDLER ( mc10_port2_r );
WRITE8_HANDLER ( mc10_port1_w );
WRITE8_HANDLER ( mc10_port2_w );

READ8_DEVICE_HANDLER(mc10_mc6847_videoram_r);
VIDEO_UPDATE(mc10);
MACHINE_START( mc10 );


#endif /* MC10_H_ */
