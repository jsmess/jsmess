/*****************************************************************************
 *
 * video/vic6560.h
 *
 * MOS video interface chip 6560, 6561
 *
 * if you need this chip in another mame/mess emulation than let it me know
 * I will split this from the vc20 driver
 * peter.trauner@jk.uni-linz.ac.at
 * 16. november 1999
 * look at mess/drivers/vc20.c and mess/machine/vc20.c
 * on how to use it
 *
 * 2 Versions
 * 6560 NTSC
 * 6561 PAL
 * 14 bit addr bus
 * 12 bit data bus
 * (16 8 bit registers)
 * alternates with MOS 6502 on the address bus
 * fetch 8 bit characternumber and 4 bit color
 * high bit of 4 bit color value determines:
 * 0: 2 color mode
 * 1: 4 color mode
 * than fetch characterbitmap for characternumber
 * 2 color mode:
 * set bit in characterbitmap gives pixel in color of the lower 3 color bits
 * cleared bit gives pixel in backgroundcolor
 * 4 color mode:
 * 2 bits in the characterbitmap are viewed together
 * 00: backgroundcolor
 * 11: colorram
 * 01: helpercolor
 * 10: framecolor
 * advance to next character in videorram until line is full
 * repeat this 8 or 16 lines, before moving to next line in videoram
 * screen ratio ntsc, pal 4/3
 *
 * pal version:
 * can contain greater visible areas
 * expects other sync position (so ntsc modules may be displayed at
 * the upper left corner of the tv screen)
 * pixel ratio seems to be different on pal and ntsc
 *
 * commodore vic20 notes
 * 6560 adress line 13 is connected inverted to address line 15 of the board
 * 1 K 4 bit ram at 0x9400 is additional connected as 4 higher bits
 * of the 6560 (colorram) without decoding the 6560 address line a8..a13
 *
 ****************************************************************************/

#ifndef __VIC6560_H__
#define __VIC6560_H__

/* to be inserted in MachineDriver-Structure */
#define VIC6560_VRETRACERATE 60
#define VIC6561_VRETRACERATE 50
#define VIC656X_VRETRACERATE (vic6560_pal?VIC6561_VRETRACERATE:VIC6560_VRETRACERATE)
#define VIC656X_HRETRACERATE 15625

#define VIC6560_MAME_XPOS  4		   /* xleft not displayed */
#define VIC6560_MAME_YPOS  10		   /* y up not displayed */
#define VIC6561_MAME_XPOS  20
#define VIC6561_MAME_YPOS  10
#define VIC656X_MAME_XPOS   (vic6560_pal?VIC6561_MAME_XPOS:VIC6560_MAME_XPOS)
#define VIC656X_MAME_YPOS   (vic6560_pal?VIC6561_MAME_YPOS:VIC6560_MAME_YPOS)
#define VIC6560_MAME_XSIZE	200
#define VIC6560_MAME_YSIZE	248
#define VIC6561_MAME_XSIZE	224
#define VIC6561_MAME_YSIZE	296
#define VIC656X_MAME_XSIZE   (vic6560_pal?VIC6561_MAME_XSIZE:VIC6560_MAME_XSIZE)
#define VIC656X_MAME_YSIZE   (vic6560_pal?VIC6561_MAME_YSIZE:VIC6560_MAME_YSIZE)
/* real values */
#define VIC6560_LINES 261
#define VIC6561_LINES 312
#define VIC656X_LINES (vic6560_pal?VIC6561_LINES:VIC6560_LINES)
/*#define VREFRESHINLINES 9 */
#define VIC6560_XSIZE	(4+201)		   /* 4 left not visible */
#define VIC6560_YSIZE	(10+251)	   /* 10 not visible */
/* cycles 65 */
#define VIC6561_XSIZE	(20+229)	   /* 20 left not visible */
#define VIC6561_YSIZE	(10+302)	   /* 10 not visible */
/* cycles 71 */
#define VIC656X_XSIZE (vic6560_pal?VIC6561_XSIZE:VIC6560_XSIZE)
#define VIC656X_YSIZE (vic6560_pal?VIC6561_YSIZE:VIC6560_YSIZE)

/* the following values depend on the VIC clock,
 * but to achieve TV-frequency the clock must have a fix frequency */
#define VIC6560_CLOCK	(14318181/14)
#define VIC6561_CLOCK	(4433618/4)
#define VIC656X_CLOCK	(vic6560_pal?VIC6561_CLOCK:VIC6560_CLOCK)


/*----------- defined in video/vic6560.c -----------*/

/* call to init videodriver */
/* pal version */
/* dma_read: videochip fetched 12 bit data from system bus */
void vic6560_init (int (*dma_read) (running_machine *machine, int), int (*dma_read_color) (running_machine *machine, int));
void vic6561_init (int (*dma_read) (running_machine *machine, int), int (*dma_read_color) (running_machine *machine, int));

/* internal */
extern int vic6560_pal;
extern UINT8 vic6560[16];

extern INTERRUPT_GEN( vic656x_raster_interrupt );
/* to be called when writting to port */
WRITE8_HANDLER ( vic6560_port_w );

/* to be called when reading from port */
READ8_HANDLER ( vic6560_port_r );

MACHINE_DRIVER_EXTERN( vic6560_video );
MACHINE_DRIVER_EXTERN( vic6561_video );

/*----------- defined in audio/vic6560.c -----------*/

#define SOUND_VIC6560		DEVICE_GET_INFO_NAME( vic6560_sound )
DEVICE_GET_INFO( vic6560_sound );
void vic6560_soundport_w (running_machine *machine, int mode, int data);


#endif /* __VIC6560_H__ */
