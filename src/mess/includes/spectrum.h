/*****************************************************************************
 *
 * includes/spectrum.h
 *
 ****************************************************************************/

#ifndef __SPECTRUM_H__
#define __SPECTRUM_H__

#include "devices/snapquik.h"
#include "devices/cartslot.h"

/* Spectrum crystals */

#define X1 XTAL_14MHz		// Main clock
#define X2 XTAL_4_433619MHz // PAL color subcarrier

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

class spectrum_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, spectrum_state(machine)); }

	spectrum_state(running_machine &machine) { }

	int port_fe_data;
	int port_7ffd_data;
	int port_1ffd_data;	/* scorpion and plus3 */
	int port_ff_data; /* Display enhancement control */
	int port_f4_data; /* Horizontal Select Register */

	int floppy;

	/* video support */
	int frame_number;    /* Used for handling FLASH 1 */
	int flash_invert;
	UINT8 retrace_cycles;
	UINT8 *video_ram;
	UINT8 *screen_location;

	int ROMSelection;

	/* for elwro800 */
	/* RAM mapped at 0 */
	UINT8 ram_at_0000;

	/* NR signal */
	UINT8 NR;
	UINT8 df_on_databus;
};


/*----------- defined in drivers/spectrum.c -----------*/

INPUT_PORTS_EXTERN( spectrum );
INPUT_PORTS_EXTERN( spec_plus );

MACHINE_DRIVER_EXTERN( spectrum );
extern MACHINE_RESET( spectrum );

extern READ8_HANDLER(spectrum_port_1f_r);
extern READ8_HANDLER(spectrum_port_7f_r);
extern READ8_HANDLER(spectrum_port_df_r);
extern READ8_HANDLER(spectrum_port_fe_r);
extern WRITE8_HANDLER(spectrum_port_fe_w);

/*----------- defined in drivers/spec128.c -----------*/
MACHINE_DRIVER_EXTERN( spectrum_128 );

extern void spectrum_128_update_memory(running_machine *machine);

/*----------- defined in drivers/specpls3.c -----------*/
extern void spectrum_plus3_update_memory(running_machine *machine);

/*----------- defined in drivers/timex.c -----------*/
extern void ts2068_update_memory(running_machine *machine);

/*----------- defined in video/spectrum.c -----------*/

extern PALETTE_INIT( spectrum );

extern VIDEO_START( spectrum );
extern VIDEO_START( spectrum_128 );

extern VIDEO_UPDATE( spectrum );
extern VIDEO_EOF( spectrum );

/*----------- defined in video/timex.c -----------*/
extern VIDEO_EOF( ts2068 );
extern VIDEO_UPDATE( ts2068 );

extern VIDEO_UPDATE( tc2048 );

#endif /* __SPECTRUM_H__ */
