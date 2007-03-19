#ifndef INC_BFMADDER2
#define INC_BFMADDER2

extern int adder_acia_triggered;	// flag <>0, ACIA receive IRQ

extern gfx_decode adder2_gfxdecodeinfo[];
extern void adder2_decode_char_roms(void);
void adder2_update(mame_bitmap *bitmap);

MACHINE_RESET( adder2_init_vid );
INTERRUPT_GEN( adder2_vbl );

ADDRESS_MAP_EXTERN( adder2_memmap );

VIDEO_START(  adder2 );
VIDEO_RESET(  adder2 );
VIDEO_UPDATE( adder2 );
PALETTE_INIT( adder2 );

#endif
