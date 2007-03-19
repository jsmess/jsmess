/*********************************************************************

	mc10.h

	TRS-80 Radio Shack MicroColor Computer

*********************************************************************/

#ifndef MC10_H
#define MC10_H

READ8_HANDLER ( mc10_bfff_r );
WRITE8_HANDLER ( mc10_bfff_w );
READ8_HANDLER ( mc10_port1_r );
READ8_HANDLER ( mc10_port2_r );
WRITE8_HANDLER ( mc10_port1_w );
WRITE8_HANDLER ( mc10_port2_w );

VIDEO_START( mc10 );
MACHINE_START( mc10 );

#endif /* MC10_H */
