/***************************************************************************

	includes/apple3.h

	Apple ///

***************************************************************************/

#ifndef APPLE3_H
#define APPLE3_H

#include "driver.h"

/*----------- defined in machine/apple3.c -----------*/

extern UINT32 a3;

#define VAR_VM0		0x0001
#define VAR_VM1		0x0002
#define VAR_VM2		0x0004
#define VAR_VM3		0x0008
#define VAR_EXTA0		0x0010
#define VAR_EXTA1		0x0020
#define VAR_EXTPOWER	0x0040
#define VAR_EXTSIDE		0x0080

MACHINE_RESET( apple3 );
DRIVER_INIT( apple3 );
INTERRUPT_GEN( apple3_interrupt );

READ8_HANDLER( apple3_00xx_r );
WRITE8_HANDLER( apple3_00xx_w );


/*----------- defined in video/apple3.c -----------*/

VIDEO_START( apple3 );
VIDEO_UPDATE( apple3 );
void apple3_write_charmem(void);

#endif /* APPLE3_H */
