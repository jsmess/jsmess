/*****************************************************************************
 *
 * includes/sym1.h
 *
 * Synertek Systems Corp. SYM-1
 *
 * Early driver by PeT <mess@utanet.at>, May 2000
 * Rewritten by Dirk Best, October 2007
 *
 ****************************************************************************/

#ifndef SYM1_H_
#define SYM1_H_


/* SYM-1 main (and only) oscillator Y1 */
#define SYM1_CLOCK  XTAL_1MHz


/*----------- defined in drivers/sym1.c -----------*/

/* Pointer to the monitor ROM, which includes the reset vectors for the CPU */
extern UINT8 *sym1_monitor;


/*----------- defined in machine/sym1.c -----------*/

DRIVER_INIT( sym1 );
MACHINE_RESET( sym1 );


#endif /* SYM1_H_ */
