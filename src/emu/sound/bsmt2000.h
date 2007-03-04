/**********************************************************************************************
 *
 *   Data East BSMT2000 driver
 *   by Aaron Giles
 *
 **********************************************************************************************/

#ifndef BSMT2000_H
#define BSMT2000_H

struct BSMT2000interface
{
	int voices;						/* number of voices (11 or 12) */
	int region;						/* memory region where the sample ROM lives */
};

WRITE16_HANDLER( BSMT2000_data_0_w );

#endif
