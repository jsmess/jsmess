/******************************************************************************
 Synertek Systems Corp. SYM-1
 
 Early driver by PeT mess@utanet.at May 2000
 Rewritten by Dirk Best October 2007
 
******************************************************************************/


/* SYM-1 main (and only) oscillator */
#define OSC_Y1 1000000

/*----------- defined in drivers/sym1.c -----------*/

/* Pointer to the monitor ROM, which includes the reset vectors for the CPU */
extern UINT8 *sym1_monitor;


/*----------- defined in machine/sym1.c -----------*/

DRIVER_INIT( sym1 );
MACHINE_RESET( sym1 );
