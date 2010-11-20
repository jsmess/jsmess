/*****************************************************************************
 *
 * includes/ti89.h
 *
 ****************************************************************************/

#ifndef TI89_H_
#define TI89_H_

 enum
 {
	HW1 = 1,
	HW2,
	HW3,
	HW4
 };

 enum
 {
	FLASH,
	EPROM
 };

/*----------- defined in drivers/ti89.c -----------*/

WRITE16_HANDLER ( ti68k_io_w );
READ16_HANDLER ( ti68k_io_r );
WRITE16_HANDLER ( ti68k_io2_w );
READ16_HANDLER ( ti68k_io2_r );
static WRITE16_HANDLER ( flash_w );
static READ16_HANDLER ( rom_r );

static TIMER_CALLBACK( ti68k_timer_callback );
static INPUT_CHANGED( ti68k_on_key );
static MACHINE_RESET(ti68k);
static VIDEO_START( ti68k );
static VIDEO_UPDATE( ti68k );

#endif /* TI89_H_ */
