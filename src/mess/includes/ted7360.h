#ifndef __TED7360_H_
#define __TED7360_H_

/*
 * if you need this chip in another mame/mess emulation than let it me know
 * I will split this from the c16 driver
 * peter.trauner@jk.uni-linz.ac.at
 * 16. november 1999
 * look at mess/systems/c16.c and mess/machine/c16.c
 * on how to use it
 */

/* call to init videodriver */
/* pal version */
/* dma_read: videochip fetched 1 byte data from system bus */
extern void ted7360_init (int pal);
extern void ted7360_set_dma (read8_handler dma_read, 
							 read8_handler dma_read_rom);

#define TED7360NTSC_VRETRACERATE 60
#define TED7360PAL_VRETRACERATE 50
#define TED7360_VRETRACERATE (ted7360_pal?TED7360PAL_VRETRACERATE:TED7360NTSC_VRETRACERATE)
#define TED7360_HRETRACERATE 15625

/* to be inserted in MachineDriver-Structure */
/* the following values depend on the VIC clock,
 * but to achieve TV-frequency the clock must have a fix frequency */
#define TED7360_HSIZE	320
#define TED7360_VSIZE	200
/* of course you clock select an other clock, but for accurate */
/* video timing (these are used in c16/c116/plus4) */
#define TED7360NTSC_CLOCK	(14318180/4)
#define TED7360PAL_CLOCK	(17734470/5)
/* pixel clock 8 mhz */
/* accesses to memory with 4 megahertz */
/* needs 3 memory accesses for 8 pixel */
/* but system clock 1 megahertz */
/* cpu driven with one (visible screen area) or two cycles (when configured) */
#define TED7360_CLOCK ((ted7360_pal?TED7360PAL_CLOCK:TED7360NTSC_CLOCK)/4)

/* pal 50 Hz vertical screen refresh, screen consists of 312 lines
 * ntsc 60 Hz vertical screen refresh, screen consists of 262 lines */
#define TED7360NTSC_LINES 261
#define TED7360PAL_LINES 312
#define TED7360_LINES (ted7360_pal?TED7360PAL_LINES:TED7360NTSC_LINES)

extern VIDEO_START( ted7360 );
extern VIDEO_UPDATE( ted7360 );
extern unsigned char ted7360_palette[16 * 8 * 3];

/* to be inserted in GameDriver-Structure */
extern struct CustomSound_interface ted7360_sound_interface;

/* to be called when writting to port */
extern WRITE8_HANDLER ( ted7360_port_w );

/* to be called when reading from port */
extern  READ8_HANDLER  ( ted7360_port_r );

/* to be called each vertical retrace */
extern void ted7360_frame_interrupt (void);
extern void ted7360_raster_interrupt (void);

/* private area */

/* from sndhrdw/pc.c */
void *ted7360_custom_start (int clock, const struct CustomSound_interface *config);
void ted7360_soundport_w (int mode, int data);

extern UINT8 ted7360[0x20];
extern bool ted7360_pal;
extern bool ted7360_rom;

#endif
