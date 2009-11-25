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

// there're many chips; we use for PAL: 6569 and for NTSC 6567R8

#ifndef VIC6567_H_
#define VIC6567_H_

#define VIC6567_CLOCK		(1022700 /* = 8181600 / 8) */ )
#define VIC6569_CLOCK		( 985248 /* = 7881984 / 8) */ )
#define VIC2_CLOCK		(vic2.pal ? VIC6569_CLOCK : VIC6567_CLOCK)

#define VIC6567_CYCLESPERLINE	65
#define VIC6569_CYCLESPERLINE	63
#define VIC2_CYCLESPERLINE	(vic2.pal ? VIC6569_CYCLESPERLINE : VIC6567_CYCLESPERLINE)

#define VIC6567_LINES		263
#define VIC6569_LINES		312
#define VIC2_LINES		(vic2.pal ? VIC6569_LINES : VIC6567_LINES)

#define VIC6567_VRETRACERATE	(59.8245100906698 /* = 1022700 / (65 * 263) */ )
#define VIC6569_VRETRACERATE	(50.1245421245421 /* =  985248 / (63 * 312) */ )
#define VIC2_VRETRACERATE	(VIC2_CLOCK / (VIC2_CYCLESPERLINE * VIC2_LINES))

#define VIC6567_HRETRACERATE	(VIC6567_CLOCK / VIC6567_CYCLESPERLINE)
#define VIC6569_HRETRACERATE	(VIC6569_CLOCK / VIC6569_CYCLESPERLINE)
#define VIC2_HRETRACERATE	(VIC2_CLOCK / VIC2_CYCLESPERLINE)

#define VIC2_HSIZE		320
#define VIC2_VSIZE		200

#define VIC6567_VISIBLELINES	235
#define VIC6569_VISIBLELINES	284
#define VIC2_VISIBLELINES	(vic2.pal ? VIC6569_VISIBLELINES : VIC6567_VISIBLELINES)

#define VIC6567_FIRST_DMA_LINE	0x30
#define VIC6569_FIRST_DMA_LINE	0x30
#define VIC2_FIRST_DMA_LINE	(vic2.pal ? VIC6569_FIRST_DMA_LINE : VIC6567_FIRST_DMA_LINE)

#define VIC6567_LAST_DMA_LINE	0xf7
#define VIC6569_LAST_DMA_LINE	0xf7
#define VIC2_LAST_DMA_LINE	(vic2.pal ? VIC6569_LAST_DMA_LINE : VIC6567_LAST_DMA_LINE)

#define VIC6567_FIRST_DISP_LINE	0x29
#define VIC6569_FIRST_DISP_LINE	0x10
#define VIC2_FIRST_DISP_LINE	(vic2.pal ? VIC6569_FIRST_DISP_LINE : VIC6567_FIRST_DISP_LINE)

#define VIC6567_LAST_DISP_LINE	(VIC6567_FIRST_DISP_LINE + VIC6567_VISIBLELINES - 1)
#define VIC6569_LAST_DISP_LINE	(VIC6569_FIRST_DISP_LINE + VIC6569_VISIBLELINES - 1)
#define VIC2_LAST_DISP_LINE	(vic2.pal ? VIC6569_LAST_DISP_LINE : VIC6567_LAST_DISP_LINE)

#define VIC6567_RASTER_2_EMU(a) ((a >= VIC6567_FIRST_DISP_LINE) ? (a - VIC6567_FIRST_DISP_LINE) : (a + 222))
#define VIC6569_RASTER_2_EMU(a) (a - VIC6569_FIRST_DISP_LINE)
#define VIC2_RASTER_2_EMU(a)	(vic2.pal ? VIC6569_RASTER_2_EMU(a) : VIC6567_RASTER_2_EMU(a))

#define VIC6567_FIRSTCOLUMN	50
#define VIC6569_FIRSTCOLUMN	50
#define VIC2_FIRSTCOLUMN	(vic2.pal ? VIC6569_FIRSTCOLUMN : VIC6567_FIRSTCOLUMN)

#define VIC6567_VISIBLECOLUMNS	418
#define VIC6569_VISIBLECOLUMNS	403
#define VIC2_VISIBLECOLUMNS	(vic2.pal ? VIC6569_VISIBLECOLUMNS : VIC6567_VISIBLECOLUMNS)

#define VIC6567_X_2_EMU(a)	(a)
#define VIC6569_X_2_EMU(a)	(a)
#define VIC2_X_2_EMU(a)		(vic2.pal ? VIC6569_X_2_EMU(a) : VIC6567_X_2_EMU(a))

#define VIC6567_STARTVISIBLELINES ((VIC6567_LINES - VIC6567_VISIBLELINES)/2)
#define VIC6569_STARTVISIBLELINES 16 /* ((VIC6569_LINES - VIC6569_VISIBLELINES)/2) */
#define VIC2_STARTVISIBLELINES ((VIC2_LINES - VIC2_VISIBLELINES)/2)

#define VIC6567_FIRSTRASTERLINE 34
#define VIC6569_FIRSTRASTERLINE 0
#define VIC2_FIRSTRASTERLINE (vic2.pal?VIC6569_FIRSTRASTERLINE:VIC6567_FIRSTRASTERLINE)

#define VIC6567_COLUMNS 512
#define VIC6569_COLUMNS 504
#define VIC2_COLUMNS (vic2.pal?VIC6569_COLUMNS:VIC6567_COLUMNS)



#define VIC6567_STARTVISIBLECOLUMNS ((VIC6567_COLUMNS - VIC6567_VISIBLECOLUMNS)/2)
#define VIC6569_STARTVISIBLECOLUMNS ((VIC6569_COLUMNS - VIC6569_VISIBLECOLUMNS)/2)
#define VIC2_STARTVISIBLECOLUMNS ((VIC2_COLUMNS - VIC2_VISIBLECOLUMNS)/2)

#define VIC6567_FIRSTRASTERCOLUMNS 412
#define VIC6569_FIRSTRASTERCOLUMNS 404
#define VIC2_FIRSTRASTERCOLUMNS (vic2.pal?VIC6569_FIRSTRASTERCOLUMNS:VIC6567_FIRSTRASTERCOLUMNS)

#define VIC6569_FIRST_X 0x194
#define VIC6567_FIRST_X 0x19c
#define VIC2_FIRST_X (vic2.pal?VIC6569_FIRST_X:VIC6567_FIRST_X)

#define VIC6569_FIRST_VISIBLE_X 0x1e0
#define VIC6567_FIRST_VISIBLE_X 0x1e8
#define VIC2_FIRST_VISIBLE_X (vic2.pal?VIC6569_FIRST_VISIBLE_X:VIC6567_FIRST_VISIBLE_X)

#define VIC6569_MAX_X 0x1f7
#define VIC6567_MAX_X 0x1ff
#define VIC2_MAX_X (vic2.pal?VIC6569_MAX_X:VIC6567_MAX_X)

#define VIC6569_LAST_VISIBLE_X 0x17c
#define VIC6567_LAST_VISIBLE_X 0x184
#define VIC2_LAST_VISIBLE_X (vic2.pal?VIC6569_LAST_VISIBLE_X:VIC6567_LAST_VISIBLE_X)

#define VIC6569_LAST_X 0x193
#define VIC6567_LAST_X 0x19b
#define VIC2_LAST_X (vic2.pal?VIC6569_LAST_X:VIC6567_LAST_X)


/*----------- defined in video/vic6567.c -----------*/

/* call to init videodriver */
/* dma_read: videochip fetched 1 byte data from system bus */
extern void vic6567_init(int vic2e, int pal, int (*dma_read)(running_machine *, int), int (*dma_read_color)(running_machine *, int), void (*irq)(running_machine *,int));

extern void vic2_set_rastering(int onoff);

MACHINE_DRIVER_EXTERN( vh_vic2 );
MACHINE_DRIVER_EXTERN( vh_vic2_pal );
extern VIDEO_START( vic2 );
extern VIDEO_UPDATE( vic2 );
// extern INTERRUPT_GEN( vic2_raster_irq );
// extern emu_timer *vicii_scanline_timer;
// extern TIMER_CALLBACK( vicii_scanline_interrupt );

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
