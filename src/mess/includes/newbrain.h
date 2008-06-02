#ifndef __NEWBRAIN__
#define __NEWBRAIN__

#define SCREEN_TAG			"main"

typedef struct _newbrain_state newbrain_state;
struct _newbrain_state
{
	/* COP420 state */
	UINT8 cop_bus;			/* data bus */
	int cop_so;				/* serial out */
	int cop_tdo;			/* ? */
	int cop_tdi;			/* ? */
	int cop_rd;				/* memory read */
	int cop_wr;				/* memory write */
	int cop_access;			/* COP access */

	/* fluorescent display state */
	int segment_data[16];	/* segment data */

	/* keyboard state */
	int keylatch;			/* keyboard row */
	int keydata;			/* keyboard column */

	/* floppy state */
	int fdc_int;			/* interrupt */
};

#endif
