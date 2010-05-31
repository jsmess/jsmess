/*********************************************************************

	a2cffa.h

	Implementation of Rich Dreher's IDE/CompactFlash board for
	the Apple II

*********************************************************************/

#ifndef __A2CFFA__
#define __A2CFFA__

#include "emu.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define A2CFFA		DEVICE_GET_INFO_NAME(a2cffa)

#define MDRV_A2CFFA_ADD(_tag)	\
	MDRV_DEVICE_ADD((_tag), A2CFFA, 0)


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* device get info function */
DEVICE_GET_INFO(a2cffa);	

/* slot read function */
READ8_DEVICE_HANDLER(a2cffa_r);

/* slot write function */
WRITE8_DEVICE_HANDLER(a2cffa_w);

/* slot ext. ROM (C800) read function */
READ8_DEVICE_HANDLER(a2cffa_c800_r);

/* slot ext. ROM (C800) write function */
WRITE8_DEVICE_HANDLER(a2cffa_c800_w);

/* slot ROM (CN00) read function */
READ8_DEVICE_HANDLER(a2cffa_cnxx_r);

/* slot ROM (CN00) write function */
WRITE8_DEVICE_HANDLER(a2cffa_cnxx_w);

#endif /* __A2SCSI__ */

