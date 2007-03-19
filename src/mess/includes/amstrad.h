#include "driver.h"
#include "inputx.h"
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

void amstrad_setup_machine(void);
void amstrad_reset_machine(void);
void amstrad_shutdown_machine(void);

SNAPSHOT_LOAD( amstrad );

void amstrad_GateArray_write(int);
void amstrad_rethinkMemory(void);
void amstrad_setLowerRom(void);
void amstrad_setUpperRom(void);

void Amstrad_Init(void);
void amstrad_handle_snapshot(unsigned char *);

void AmstradCPC_SetUpperRom(int);
void AmstradCPC_PALWrite(int);

extern int amstrad_cassette_init(mess_image *img, mame_file *fp, int open_mode);
void amstrad_plus_setspritecolour(unsigned int off, int r, int g, int b);
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

/* number of us cycles per frame (measured) */
#define AMSTRAD_US_PER_SCANLINE   64
#define AMSTRAD_FPS               50.080128205128205

extern VIDEO_START( amstrad );
extern VIDEO_UPDATE( amstrad );
void amstrad_update_scanline(void);
void amstrad_vh_execute_crtc_cycles(int);
void amstrad_vh_update_colour(int, int);
void amstrad_vh_update_mode(int);

/* The VSYNC signal of the CRTC */
extern int amstrad_CRTC_VS;
extern int amstrad_CRTC_CR;

/* update interrupt timer */
void amstrad_interrupt_timer_update(void);
/* if start of vsync sound, wait to reset interrupt counter 2 hsyncs later */
void amstrad_interrupt_timer_trigger_reset_by_vsync(void);

/*** AMSTRAD CPC SPECIFIC ***/

/* initialise palette for CPC464, CPC664 and CPC6128 */
extern PALETTE_INIT( amstrad_cpc );

/**** KC COMPACT SPECIFIC ***/

/* initialise palette for KC Compact */
extern PALETTE_INIT( kccomp );

/**** AMSTRAD PLUS SPECIFIC ***/

/* initialise palette for 464plus, 6128plus */
extern PALETTE_INIT( amstrad_plus );

DEVICE_LOAD(amstrad_plus_cartridge);


