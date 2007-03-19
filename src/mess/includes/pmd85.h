/* machine/pmd85.c */
 READ8_HANDLER ( pmd85_io_r );
WRITE8_HANDLER ( pmd85_io_w );
 READ8_HANDLER ( mato_io_r );
WRITE8_HANDLER ( mato_io_w );
DRIVER_INIT ( pmd851 );
DRIVER_INIT ( pmd852a );
DRIVER_INIT ( pmd853 );
DRIVER_INIT ( alfa );
DRIVER_INIT ( mato );
extern MACHINE_RESET( pmd85 );
extern int pmd85_tape_init(int);
extern void pmd85_tape_exit(int);

/* vidhrdw/pmd85.c */
extern VIDEO_START( pmd85 );
extern VIDEO_UPDATE( pmd85 );
extern unsigned char pmd85_palette[3*3];
extern unsigned short pmd85_colortable[1][3];
extern PALETTE_INIT( pmd85 );
