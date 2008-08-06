#ifndef __NEWBRAIN__
#define __NEWBRAIN__

#define SCREEN_TAG			"main"

#define NEWBRAIN_VIDEO_RV			0x01
#define NEWBRAIN_VIDEO_FS			0x02
#define NEWBRAIN_VIDEO_32_40		0x04
#define NEWBRAIN_VIDEO_UCR			0x08
#define NEWBRAIN_VIDEO_80L			0x40

typedef struct _newbrain_state newbrain_state;
struct _newbrain_state
{
	/* processor state */
	int pwrup;				/* power up */
	int userint;			/* user interrupt */
	int clkint;				/* clock interrupt */
	int aciaint;			/* ACIA interrupt */
	int copint;				/* COP interrupt */
	int bee;				/* identity */
	UINT8 enrg1;			/* enable register 1 */
	UINT8 enrg2;			/* enable register 2 */

	/* COP420 state */
	UINT8 cop_bus;			/* data bus */
	int cop_so;				/* serial out */
	int cop_tdo;			/* tape data output */
	int cop_tdi;			/* tape data input */
	int cop_rd;				/* memory read */
	int cop_wr;				/* memory write */
	int cop_access;			/* COP access */

	/* keyboard state */
	int keylatch;			/* keyboard row */
	int keydata;			/* keyboard column */

	/* paging state */
	UINT8 paging;			/* paging control register */
	UINT8 pr[16];			/* expansion interface paging register */

	/* floppy state */
	int fdc_int;			/* interrupt */
	int fdc_att;			/* attention */

	/* video state */
	int segment_data[16];	/* VF segment data */
	int tvcnsl;				/* TV console required */
	int tvctl;				/* TV control register */
	UINT16 tvram;			/* TV start address */

	/* user bus state */
	UINT8 user;				

	/* timers */
	emu_timer *reset_timer;	/* power on reset timer */
	emu_timer *pwrup_timer;	/* power up timer */
};

/* ---------- defined in video/newbrain.c ---------- */

MACHINE_DRIVER_EXTERN( newbrain_video );

#endif
