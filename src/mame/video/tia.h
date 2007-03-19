PALETTE_INIT( tia_NTSC );
PALETTE_INIT( tia_PAL );

VIDEO_START( tia );
VIDEO_UPDATE( tia );

READ8_HANDLER( tia_r );
WRITE8_HANDLER( tia_w );

void tia_init(void);
