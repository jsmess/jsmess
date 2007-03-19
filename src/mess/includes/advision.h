/* machine/advision.c */
extern int advision_framestart;
/*extern int advision_videoenable;*/
extern int advision_videobank;

MACHINE_RESET( advision );
READ8_HANDLER ( advision_MAINRAM_r);
WRITE8_HANDLER( advision_MAINRAM_w );
WRITE8_HANDLER( advision_putp1 );
WRITE8_HANDLER( advision_putp2 );
READ8_HANDLER ( advision_getp1 );
READ8_HANDLER ( advision_getp2 );
READ8_HANDLER ( advision_gett0 );
READ8_HANDLER ( advision_gett1 );


READ8_HANDLER ( advision_getL );
WRITE8_HANDLER ( advision_putG );
WRITE8_HANDLER ( advision_putD );



/* vidhrdw/advision.c */
extern int advision_vh_hpos;

VIDEO_START( advision );
VIDEO_UPDATE( advision );
PALETTE_INIT( advision );
void advision_vh_write(int data);
void advision_vh_update(int data);

