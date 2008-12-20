/*****************************************************************************
 *
 * includes/amstrad.h
 *
 ****************************************************************************/

#ifndef AMSTRAD_H_
#define AMSTRAD_H_

#include "devices/snapquik.h"


//#define AMSTRAD_VIDEO_USE_EVENT_LIST
#ifdef AMSTRAD_VIDEO_USE_EVENT_LIST
/* codes for eventlist */
enum {
	EVENT_LIST_CODE_GA_COLOUR = 0,       /* change pen colour with gate array */
	EVENT_LIST_CODE_GA_MODE = 1,         /* change mode with gate array */
	EVENT_LIST_CODE_CRTC_WRITE = 2,      /* change CRTC register data */
  EVENT_LIST_CODE_CRTC_INDEX_WRITE = 3 /* change CRTC register selection */
};
#endif


/*----------- defined in drivers/amstrad.c -----------*/

extern int amstrad_plus_asic_enabled;
extern int amstrad_plus_pri;
extern int amstrad_system_type;
extern int amstrad_plus_irq_cause;
extern int amstrad_plus_scroll_x;
extern int amstrad_plus_scroll_y;
extern int amstrad_plus_scroll_border;

extern int amstrad_plus_dma_status;
extern int amstrad_plus_dma_0_addr;   // DMA channel address
extern int amstrad_plus_dma_1_addr;
extern int amstrad_plus_dma_2_addr;
extern int amstrad_plus_dma_prescaler[3];  // DMA channel prescaler

extern unsigned char *amstrad_plus_asic_ram;

extern int aleste_mode;

void amstrad_reset_machine(running_machine *machine);
void amstrad_GateArray_write(running_machine *machine,int);
void amstrad_rethinkMemory(running_machine *machine);
void amstrad_setLowerRom(running_machine *machine);
void amstrad_setUpperRom(running_machine *machine);

void AmstradCPC_SetUpperRom(running_machine *machine, int);
void AmstradCPC_PALWrite(running_machine *machine, int);


/*----------- defined in machine/amstrad.c -----------*/

void amstrad_setup_machine(running_machine *);

SNAPSHOT_LOAD( amstrad );

void amstrad_handle_snapshot(running_machine *, unsigned char *);

DEVICE_IMAGE_LOAD(amstrad_plus_cartridge);

/*----------- defined in video/amstrad.c -----------*/

void amstrad_init_palette(running_machine* machine);
void amstrad_plus_setspritecolour(running_machine *machine, unsigned int off, int r, int g, int b);
void amstrad_plus_setsplitline(unsigned int line, unsigned int address);

/* On the Amstrad, any part of the 64k memory can be access by the video
hardware (GA and CRTC - the CRTC specifies the memory address to access,
and the GA fetches 2 bytes of data for each 1us cycle.

The Z80 must also access the same ram.

To maintain the screen display, the Z80 is halted on each memory access.

The result is that timing for opcodes, appears to fall into a nice pattern,
where the time for each opcode can be measured in NOP cycles. NOP cycles is
the name I give to the time taken for one NOP command to execute.

This happens to be 1us.

From measurement, there are 64 NOPs per line, with 312 lines per screen.
This gives a total of 19968 NOPs per frame. */

#define AMSTRAD_CHARACTERS 8

/* REAL AMSTRAD SCREEN WIDTH and HEIGHT */
#define AMSTRAD_SCREEN_WIDTH	(50*AMSTRAD_CHARACTERS*2)
#define AMSTRAD_SCREEN_HEIGHT	(39*AMSTRAD_CHARACTERS)
#define ALESTE_SCREEN_WIDTH     (82*AMSTRAD_CHARACTERS)

/* number of us cycles per frame (measured) */
#define AMSTRAD_US_PER_SCANLINE   64
#define AMSTRAD_FPS               50.080128205128205

extern VIDEO_START( amstrad );
extern VIDEO_UPDATE( amstrad );
void amstrad_vh_execute_crtc_cycles(running_machine*, int);
void amstrad_vh_update_colour(int, int);
void amstrad_vh_update_mode(int);

/* The VSYNC signal of the CRTC */
extern int amstrad_CRTC_VS;
extern int amstrad_CRTC_CR;

/*** AMSTRAD CPC SPECIFIC ***/

/* initialise palette for CPC464, CPC664 and CPC6128 */
//extern PALETTE_INIT( amstrad_cpc );

/**** KC COMPACT SPECIFIC ***/

/* initialise palette for KC Compact */
extern PALETTE_INIT( kccomp );

/**** AMSTRAD PLUS SPECIFIC ***/

/* initialise palette for 464plus, 6128plus */
extern PALETTE_INIT( amstrad_plus );

/**** ALESTE SPECIFIC ***/
extern PALETTE_INIT( aleste );
void aleste_vh_update_colour(int, int);

extern int prev_reg;

/* The gate array counts CRTC HSYNC pulses. (It has a internal 6-bit counter). */
extern int amstrad_CRTC_HS_Counter;

#endif /* AMSTRAD_H_ */
