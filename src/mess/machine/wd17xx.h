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

typedef enum
{
	WD17XX_IRQ_CLR,
	WD17XX_IRQ_SET,
	WD17XX_DRQ_CLR,
	WD17XX_DRQ_SET
} wd17xx_state_t;

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
} wd17xx_type_t;



/***************************************************************************

	Prototypes

***************************************************************************/

void wd17xx_init(running_machine *machine, wd17xx_type_t type, void (*callback)(running_machine *, wd17xx_state_t, void *), void *param);
void wd17xx_reset(void);

/* the following are not strictly part of the wd179x hardware/emulation
but will be put here for now until the flopdrv code has been finalised more */
void wd17xx_set_drive(UINT8);		/* set drive wd179x is accessing */
void wd17xx_set_side(UINT8);		/* set side wd179x is accessing */
void wd17xx_set_density(DENSITY);	/* set density */

READ8_HANDLER( wd17xx_status_r );
READ8_HANDLER( wd17xx_track_r );
READ8_HANDLER( wd17xx_sector_r );
READ8_HANDLER( wd17xx_data_r );

WRITE8_HANDLER( wd17xx_command_w );
WRITE8_HANDLER( wd17xx_track_w );
WRITE8_HANDLER( wd17xx_sector_w );
WRITE8_HANDLER( wd17xx_data_w );

READ8_HANDLER( wd17xx_r );
WRITE8_HANDLER( wd17xx_w );

#endif /* WD179X_H */


