/*****************************************************************************
 *
 * includes/spectrum.h
 *
 ****************************************************************************/

#ifndef SPECTRUM_H
#define SPECTRUM_H

#include "devices/snapquik.h"


/* Spectrum screen size in pixels */
#define SPEC_UNSEEN_LINES  16   /* Non-visible scanlines before first border
                                   line. Some of these may be vertical retrace. */
#define SPEC_TOP_BORDER    48   /* Number of border lines before actual screen */
#define SPEC_DISPLAY_YSIZE 192  /* Vertical screen resolution */
#define SPEC_BOTTOM_BORDER 56   /* Number of border lines at bottom of screen */
#define SPEC_SCREEN_HEIGHT (SPEC_TOP_BORDER + SPEC_DISPLAY_YSIZE + SPEC_BOTTOM_BORDER)

#define SPEC_LEFT_BORDER   48   /* Number of left hand border pixels */
#define SPEC_DISPLAY_XSIZE 256  /* Horizontal screen resolution */
#define SPEC_RIGHT_BORDER  48   /* Number of right hand border pixels */
#define SPEC_SCREEN_WIDTH (SPEC_LEFT_BORDER + SPEC_DISPLAY_XSIZE + SPEC_RIGHT_BORDER)

#define SPEC_LEFT_BORDER_CYCLES   24   /* Cycles to display left hand border */
#define SPEC_DISPLAY_XSIZE_CYCLES 128  /* Horizontal screen resolution */
#define SPEC_RIGHT_BORDER_CYCLES  24   /* Cycles to display right hand border */
#define SPEC_RETRACE_CYCLES       48   /* Cycles taken for horizonal retrace */
#define SPEC_CYCLES_PER_LINE      224  /* Number of cycles to display a single line */

/* 128K machines take an extra 4 cycles per scan line - add this to retrace */
#define SPEC128_UNSEEN_LINES    15
#define SPEC128_RETRACE_CYCLES  52
#define SPEC128_CYCLES_PER_LINE 228

/* Border sizes for TS2068. These are guesses based on the number of cycles
   available per frame. */
#define TS2068_TOP_BORDER    32
#define TS2068_BOTTOM_BORDER 32
#define TS2068_SCREEN_HEIGHT (TS2068_TOP_BORDER + SPEC_DISPLAY_YSIZE + TS2068_BOTTOM_BORDER)

/* Double the border sizes to maintain ratio of screen to border */
#define TS2068_LEFT_BORDER   96   /* Number of left hand border pixels */
#define TS2068_DISPLAY_XSIZE 512  /* Horizontal screen resolution */
#define TS2068_RIGHT_BORDER  96   /* Number of right hand border pixels */
#define TS2068_SCREEN_WIDTH (TS2068_LEFT_BORDER + TS2068_DISPLAY_XSIZE + TS2068_RIGHT_BORDER)

typedef enum
{
	TIMEX_CART_NONE,
	TIMEX_CART_DOCK,
	TIMEX_CART_EXROM,
	TIMEX_CART_HOME
} TIMEX_CART_TYPE;


/*----------- defined in machine/spectrum.c -----------*/

extern TIMEX_CART_TYPE timex_cart_type;
extern UINT8 timex_cart_chunks;
extern UINT8 * timex_cart_data;

DEVICE_IMAGE_LOAD( timex_cart );
DEVICE_IMAGE_UNLOAD( timex_cart );

extern MACHINE_RESET( spectrum );

extern SNAPSHOT_LOAD( spectrum );
extern QUICKLOAD_LOAD( spectrum );


/*----------- defined in drivers/spectrum.c -----------*/
INPUT_PORTS_EXTERN( spectrum );
MACHINE_DRIVER_EXTERN( spectrum );
SYSTEM_CONFIG_EXTERN(spectrum)
SYSTEM_CONFIG_EXTERN(spectrum_common)

extern READ8_HANDLER(spectrum_port_1f_r);
extern READ8_HANDLER(spectrum_port_7f_r);
extern READ8_HANDLER(spectrum_port_df_r);
extern READ8_HANDLER(spectrum_port_fe_r);
extern WRITE8_HANDLER(spectrum_port_fe_w);
extern int PreviousFE;

/*----------- defined in drivers/spec128.c -----------*/
MACHINE_DRIVER_EXTERN( spectrum_128 );

extern READ8_HANDLER(spectrum_128_port_fffd_r);
extern WRITE8_HANDLER(spectrum_128_port_bffd_w);
extern WRITE8_HANDLER(spectrum_128_port_fffd_w);
extern unsigned char *spectrum_128_screen_location;
extern void spectrum_128_update_memory(running_machine *machine);
extern int spectrum_128_port_7ffd_data;

/*----------- defined in drivers/specpls3.c -----------*/
extern int spectrum_plus3_port_1ffd_data;
extern void spectrum_plus3_update_memory(running_machine *machine);

/*----------- defined in drivers/spec128.c -----------*/
extern void ts2068_update_memory(running_machine *machine);
extern int ts2068_port_ff_data;
extern int ts2068_port_f4_data;

/*----------- defined in video/spectrum.c -----------*/
extern int frame_number;    /* Used for handling FLASH 1 */
extern int flash_invert;

extern PALETTE_INIT( spectrum );

extern VIDEO_START( spectrum );
extern VIDEO_UPDATE( spectrum );
extern VIDEO_EOF( spectrum );

extern unsigned char *spectrum_characterram;
extern unsigned char *spectrum_colorram;

extern const gfx_layout spectrum_charlayout;

/*----------- defined in video/spec128.c -----------*/
extern PALETTE_INIT( spectrum_128 );

extern VIDEO_START( spectrum_128 );
extern VIDEO_UPDATE( spectrum_128 );


/*----------- defined in video/timex.c -----------*/
extern VIDEO_EOF( ts2068 );
extern VIDEO_UPDATE( ts2068 );

extern VIDEO_UPDATE( tc2048 );

#endif /* SPECTRUM_H */
