#ifndef UK101_H
#define UK101_H

/* machine/uk101.c */
DRIVER_INIT( uk101 );
 READ8_HANDLER( uk101_acia0_casin );
 READ8_HANDLER( uk101_acia0_statin );
 READ8_HANDLER( uk101_keyb_r );
WRITE8_HANDLER( uk101_keyb_w );
WRITE8_HANDLER( superbrd_keyb_w );
DEVICE_LOAD( uk101_cassette );
DEVICE_UNLOAD( uk101_cassette );

/* vidhrdw/uk101.c */
VIDEO_UPDATE( uk101 );
VIDEO_UPDATE( superbrd );

#endif /* UK101_H */
