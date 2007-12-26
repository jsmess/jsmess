#include "devices/snapquik.h"

/* machine/galaxy.c */
extern DRIVER_INIT( galaxy );
extern MACHINE_RESET( galaxy );
extern INTERRUPT_GEN( galaxy_interrupt );
extern SNAPSHOT_LOAD( galaxy );
extern int galaxy_interrupts_enabled;
extern READ8_HANDLER( galaxy_keyboard_r );
extern READ8_HANDLER( galaxy_latch_r );
extern WRITE8_HANDLER( galaxy_latch_w );

/* video/galaxy.c */
extern const gfx_layout galaxy_charlayout;
extern const unsigned char galaxy_palette[2*3];
extern const unsigned short galaxy_colortable[1][2];
extern PALETTE_INIT( galaxy );
extern VIDEO_START( galaxy );
extern VIDEO_UPDATE( galaxy );
extern WRITE8_HANDLER( galaxy_vh_charram_w );
