/*****************************************************************************
 *
 * includes/pmd85.h
 *
 ****************************************************************************/

#ifndef PMD85_H_
#define PMD85_H_


/*----------- defined in machine/pmd85.c -----------*/

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


/*----------- defined in video/pmd85.c -----------*/

extern VIDEO_START( pmd85 );
extern VIDEO_UPDATE( pmd85 );
extern const unsigned char pmd85_palette[3*3];
extern const unsigned short pmd85_colortable[1][3];
extern PALETTE_INIT( pmd85 );


#endif /* PMD85_H_ */
