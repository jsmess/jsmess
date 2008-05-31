#ifndef __NEWBRAIN__
#define __NEWBRAIN__

#define SCREEN_TAG			"main"

typedef struct _newbrain_state newbrain_state;
struct _newbrain_state
{
	/* COP420 state */
	UINT8 cop_bus;				/* data bus */

	/* keyboard state */
	int keylatch;			/* keyboard row */
	int keydata;			/* keyboard column */
};

#endif
