#ifndef __NEWBRAIN__
#define __NEWBRAIN__

#define SCREEN_TAG			"main"

typedef struct _newbrain_state newbrain_state;
struct _newbrain_state
{
	/* processor state */
	int pwrup;				/* power up */

	/* COP420 state */
	UINT8 cop_bus;			/* data bus */
	int cop_so;				/* serial out */
	int cop_tdo;			/* tape data output */
	int cop_tdi;			/* tape data input */
	int cop_rd;				/* memory read */
	int cop_wr;				/* memory write */
	int cop_access;			/* COP access */
	int cop_int;			/* COP interrupt */

	/* keyboard state */
	int keylatch;			/* keyboard row */
	int keydata;			/* keyboard column */

	/* floppy state */
	int fdc_int;			/* interrupt */

	/* video state */
	int segment_data[16];	/* VF segment data */
	int clk_int;			/* clock interrupt */
	int clk;				/* clock interrupt enable */
	int tvp;				/*  */

	/* serial state */
	int v24_rts;			/* V.24 ready to send */
	int v24_txd;			/* V.24 transmit */
	int prt_txd;			/* printer transmit */

	/* timers */
	emu_timer *reset_timer;	/* power on reset timer */
	emu_timer *pwrup_timer;	/* power up timer */
};

/* ---------- defined in video/newbrain.c ---------- */

MACHINE_DRIVER_EXTERN( newbrain_video );

#endif
