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

#include "devcb.h"


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef UINT8 (*vic656x_lightpen_x_callback)(running_machine *machine);
typedef UINT8 (*vic656x_lightpen_y_callback)(running_machine *machine);
typedef UINT8 (*vic656x_lightpen_button_callback)(running_machine *machine);
typedef UINT8 (*vic656x_paddle_callback)(running_machine *machine);

typedef int (*vic656x_dma_read)(running_machine *machine, int);
typedef int (*vic656x_dma_read_color)(running_machine *machine, int);


typedef enum
{
	VIC6560,
	VIC6561
} vic656x_type;

typedef struct _vic656x_interface vic656x_interface;
struct _vic656x_interface
{
	const char         *screen;

	vic656x_type type;

	vic656x_lightpen_x_callback        x_cb;
	vic656x_lightpen_y_callback        y_cb;
	vic656x_lightpen_button_callback   button_cb;

	vic656x_paddle_callback        paddle0_cb, paddle1_cb;

	vic656x_dma_read          dma_read;
	vic656x_dma_read_color    dma_read_color;
};

/***************************************************************************
    CONSTANTS
***************************************************************************/

#define VIC6560_VRETRACERATE 60
#define VIC6561_VRETRACERATE 50
#define VIC656X_HRETRACERATE 15625

#define VIC6560_MAME_XPOS  4		   /* xleft not displayed */
#define VIC6560_MAME_YPOS  10		   /* y up not displayed */
#define VIC6561_MAME_XPOS  20
#define VIC6561_MAME_YPOS  10
#define VIC6560_MAME_XSIZE	200
#define VIC6560_MAME_YSIZE	248
#define VIC6561_MAME_XSIZE	224
#define VIC6561_MAME_YSIZE	296
/* real values */
#define VIC6560_LINES 261
#define VIC6561_LINES 312
/*#define VREFRESHINLINES 9 */
#define VIC6560_XSIZE	(4+201)		   /* 4 left not visible */
#define VIC6560_YSIZE	(10+251)	   /* 10 not visible */
/* cycles 65 */
#define VIC6561_XSIZE	(20+229)	   /* 20 left not visible */
#define VIC6561_YSIZE	(10+302)	   /* 10 not visible */
/* cycles 71 */


/* the following values depend on the VIC clock,
 * but to achieve TV-frequency the clock must have a fix frequency */
#define VIC6560_CLOCK	(14318181/14)
#define VIC6561_CLOCK	(4433618/4)


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

DEVICE_GET_INFO( vic656x );

/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define VIC656X DEVICE_GET_INFO_NAME( vic656x )
#define SOUND_VIC656X VIC656X

#define MDRV_VIC656X_ADD(_tag, _interface) \
	MDRV_DEVICE_ADD(_tag, SOUND, 0) \
	MDRV_DEVICE_CONFIG_DATAPTR(sound_config, type, SOUND_VIC656X) \
	MDRV_DEVICE_CONFIG(_interface)


/*----------- defined in video/vic6560.c -----------*/

WRITE8_DEVICE_HANDLER( vic6560_port_w );
READ8_DEVICE_HANDLER( vic6560_port_r );

void vic656x_raster_interrupt_gen( running_device *device );
UINT32 vic656x_video_update( running_device *device, bitmap_t *bitmap, const rectangle *cliprect );


#endif /* __VIC6560_H__ */
