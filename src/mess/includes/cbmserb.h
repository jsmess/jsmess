/*****************************************************************************
 *
 * includes/cbmserb.h
 *
 ****************************************************************************/

#ifndef CBMSERB_H_
#define CBMSERB_H_

#include "cbmdrive.h"


/*----------- defined in machine/cbmserb.c -----------*/

/* through this interface, we choose between simulated and emulated drives */
/* this is temporarily needed in order to be able to compile the code for both, without breaking anything */
typedef struct _cbm_serial_interface cbm_serial_interface;
struct _cbm_serial_interface
{
	int serial;			/* This is just a parameter, to log which interface is in use */
	void (*serial_reset_write)(int level);
	int (*serial_request_read)(void);
	void (*serial_request_write)(int level);
	int (*serial_atn_read)(void);
	int (*serial_data_read)(void);
	int (*serial_clock_read)(void);
	void (*serial_atn_write)(int level);
	void (*serial_data_write)(int level);
	void (*serial_clock_write)(int level);
};

void serial_config(running_machine *machine, const cbm_serial_interface *intf);

const cbm_serial_interface sim_drive_interface;		/* serial = 1 */
const cbm_serial_interface emu_drive_interface;		/* serial = 2 */
const cbm_serial_interface fake_drive_interface;	/* serial = 3 */


/* Serial bus for vic20, c64 & c16 with vc1541 and some printer */

/* To be passed directly to the drivers */
void cbm_serial_reset_write (int level);
int cbm_serial_atn_read (void);
void cbm_serial_atn_write (int level);
int cbm_serial_data_read (void);
void cbm_serial_data_write (int level);
int cbm_serial_clock_read (void);
void cbm_serial_clock_write (int level);
int cbm_serial_request_read (void);
void cbm_serial_request_write (int level);


#endif /* CBMSERB_H_ */
