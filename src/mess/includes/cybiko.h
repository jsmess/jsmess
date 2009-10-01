/*****************************************************************************
 *
 * includes/cybiko.h
 *
 * Cybiko Wireless Inter-tainment System
 *
 * (c) 2001-2007 Tim Schuerewegen
 *
 * Cybiko Classic (V1)
 * Cybiko Classic (V2)
 * Cybiko Xtreme
 *
 ****************************************************************************/

#ifndef CYBIKO_H_
#define CYBIKO_H_


/*----------- defined in machine/cybiko.c -----------*/

// driver init
DRIVER_INIT( cybikov1 );
DRIVER_INIT( cybikov2 );
DRIVER_INIT( cybikoxt );

// non-volatile ram handler
/*
NVRAM_HANDLER( cybikov1 );
NVRAM_HANDLER( cybikov2 );
NVRAM_HANDLER( cybikoxt );
*/

// machine start
MACHINE_START( cybikov1 );
MACHINE_START( cybikov2 );
MACHINE_START( cybikoxt );

// machine reset
MACHINE_RESET( cybikov1 );
MACHINE_RESET( cybikov2 );
MACHINE_RESET( cybikoxt );

// lcd read/write
READ16_HANDLER( cybiko_lcd_r );
WRITE16_HANDLER( cybiko_lcd_w );

// key read
READ16_HANDLER( cybiko_key_r );

// unknown
WRITE16_HANDLER( cybiko_unk1_w );
READ16_HANDLER( cybiko_unk2_r );

// onchip registers read/write
READ8_HANDLER( cybikov1_io_reg_r );
READ8_HANDLER( cybikov2_io_reg_r );
READ8_HANDLER( cybikoxt_io_reg_r );
WRITE8_HANDLER( cybikov1_io_reg_w );
WRITE8_HANDLER( cybikov2_io_reg_w );
WRITE8_HANDLER( cybikoxt_io_reg_w );


#endif /* CYBIKO_H_ */
