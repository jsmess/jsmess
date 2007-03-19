#ifndef KIM1_H
#define KIM1_H

/* from mess/machine/kim1.c */
DRIVER_INIT( kim1 );
MACHINE_RESET( kim1 );

void kim1_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info);

INTERRUPT_GEN( kim1_interrupt );

READ8_HANDLER ( m6530_003_r );
READ8_HANDLER ( m6530_002_r );

WRITE8_HANDLER ( m6530_003_w );
WRITE8_HANDLER ( m6530_002_w );

/* from mess/vidhrdw/kim1.c */
PALETTE_INIT( kim1 );
VIDEO_START( kim1 );
VIDEO_UPDATE( kim1 );

#endif /* KIM1_H */
