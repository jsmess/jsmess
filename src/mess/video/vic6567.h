/*****************************************************************************
 *
 * video/vic6567.h
 *
 * if you need this chip in another mame/mess emulation than let it me know
 * I will split this from the c64 driver
 * peter.trauner@jk.uni-linz.ac.at
 * 1. 1. 2000
 * look at mess/drivers/c64.c and mess/machine/c64.c
 * on how to use it
 *
 ****************************************************************************/

#ifndef VIC6567_H_
#define VIC6567_H_

#define VIC6567_VRETRACERATE 60
#define VIC6569_VRETRACERATE 50
#define VIC2_VRETRACERATE (vic2.pal?VIC6569_VRETRACERATE:VIC6567_VRETRACERATE)
#define VIC2_HRETRACERATE 15625

/* to be inserted in MachineDriver-Structure */
/* the following values depend on the VIC clock,
 * but to achieve TV-frequency the clock must have a fix frequency */
#define VIC2_HSIZE	320
#define VIC2_VSIZE	200

/* of course you clock select an other clock, but for accurate */
/* video timing */
#define VIC6567_CLOCK	1022730
			/* (8180000/8) old value */
#define VIC6569_CLOCK	985250
			/* (7880000/8) old value */ 
/* pixel clock 8 mhz */
/* accesses to memory with 2 megahertz */
/* needs 2 memory accesses for 8 pixel */
/* + sprite + */
/* but system clock 1 megahertz */
/* cpu driven with one (visible screen area) */
#define VIC2_CLOCK ((vic2.pal?VIC6569_CLOCK:VIC6567_CLOCK))

/* pal 50 Hz vertical screen refresh, screen consists of 312 lines
 * ntsc 60 Hz vertical screen refresh, screen consists of 262 lines */
#define VIC6567_LINES 262
#define VIC6569_LINES 312
#define VIC2_LINES (vic2.pal?VIC6569_LINES:VIC6567_LINES)
#define VIC6567_VISIBLELINES 234
#define VIC6569_VISIBLELINES 284
#define VIC2_VISIBLELINES (vic2.pal?VIC6569_VISIBLELINES:VIC6567_VISIBLELINES)
#define VIC6567_STARTVISIBLELINES ((VIC6567_LINES - VIC6567_VISIBLELINES)/2)
#define VIC6569_STARTVISIBLELINES ((VIC6569_LINES - VIC6569_VISIBLELINES)/2)
#define VIC2_STARTVISIBLELINES ((VIC2_LINES - VIC2_VISIBLELINES)/2)

#define VIC6567_COLUMNS 512
#define VIC6569_COLUMNS 504
#define VIC2_COLUMNS (vic2.pal?VIC6569_COLUMNS:VIC6567_COLUMNS)
#define VIC6567_VISIBLECOLUMNS 411
#define VIC6569_VISIBLECOLUMNS 403
#define VIC2_VISIBLECOLUMNS (vic2.pal?VIC6569_VISIBLECOLUMNS:VIC6567_VISIBLECOLUMNS)
#define VIC6567_STARTVISIBLECOLUMNS ((VIC6567_COLUMNS - VIC6567_VISIBLECOLUMNS)/2)
#define VIC6569_STARTVISIBLECOLUMNS ((VIC6569_COLUMNS - VIC6569_VISIBLECOLUMNS)/2)
#define VIC2_STARTVISIBLECOLUMNS ((VIC2_COLUMNS - VIC2_VISIBLECOLUMNS)/2)


/*----------- defined in video/vic6567.c -----------*/

/* call to init videodriver */
/* dma_read: videochip fetched 1 byte data from system bus */
extern void vic6567_init (int vic2e, int pal, int (*dma_read) (int),
						  int (*dma_read_color) (int), void (*irq) (int));

extern void vic2_set_rastering(int onoff);

MACHINE_DRIVER_EXTERN( vh_vic2 );
MACHINE_DRIVER_EXTERN( vh_vic2_pal );
extern VIDEO_START( vic2 );
extern VIDEO_UPDATE( vic2 );
extern INTERRUPT_GEN( vic2_raster_irq );

extern const unsigned char vic2_palette[16 * 3];

/* to be called when writting to port */
extern WRITE8_HANDLER ( vic2_port_w );

/* to be called when reading from port */
extern  READ8_HANDLER ( vic2_port_r );

extern void vic2_lightpen_write (int level);

int vic2e_k0_r (void);
int vic2e_k1_r (void);
int vic2e_k2_r (void);

/* to be called each vertical retrace */
extern INTERRUPT_GEN( vic2_frame_interrupt );

/* private area */

/*extern UINT8 vic2[]; */
/*extern int vic2_pal; */

#endif /* VIC6567_H_ */
