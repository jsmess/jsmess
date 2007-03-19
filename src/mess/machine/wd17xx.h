/*********************************************************************

	wd17xx.h

	Implementations of the Western Digital 17xx and 19xx families of
	floppy disk controllers

*********************************************************************/

#ifndef WD179X_H
#define WD179X_H

#include "devices/flopdrv.h"



/***************************************************************************

	Constants and enumerations

***************************************************************************/

#define WD179X_IRQ_CLR	0
#define WD179X_IRQ_SET	1
#define WD179X_DRQ_CLR	2
#define WD179X_DRQ_SET	3

/* enumeration to specify the type of FDC; there are subtle differences */
typedef enum
{
	WD_TYPE_1770,
	WD_TYPE_1772,
	WD_TYPE_1773,
	WD_TYPE_179X,
	WD_TYPE_1793,
	WD_TYPE_2793,

	/* duplicate constants */
	WD_TYPE_177X = WD_TYPE_1770,
	WD_TYPE_MB8877 = WD_TYPE_179X
} wd179x_type_t;



/***************************************************************************

	Prototypes

***************************************************************************/

void wd179x_init(wd179x_type_t type, void (*callback)(int));
void wd179x_reset(void);

/* the following are not strictly part of the wd179x hardware/emulation
but will be put here for now until the flopdrv code has been finalised more */
void wd179x_set_drive(UINT8);		/* set drive wd179x is accessing */
void wd179x_set_side(UINT8);		/* set side wd179x is accessing */
void wd179x_set_density(DENSITY);	/* set density */

READ8_HANDLER( wd179x_status_r );
READ8_HANDLER( wd179x_track_r );
READ8_HANDLER( wd179x_sector_r );
READ8_HANDLER( wd179x_data_r );

WRITE8_HANDLER( wd179x_command_w );
WRITE8_HANDLER( wd179x_track_w );
WRITE8_HANDLER( wd179x_sector_w );
WRITE8_HANDLER( wd179x_data_w );

READ8_HANDLER( wd179x_r );
WRITE8_HANDLER( wd179x_w );

#endif /* WD179X_H */


