/*****************************************************************************
 *
 * includes/cbmserb.h
 *
 ****************************************************************************/

#ifndef CBMSERB_H_
#define CBMSERB_H_

/*----------- defined in machine/cbmserb.c -----------*/

/* through this interface, we choose between simulated and emulated drives */
/* this is temporarily needed in order to be able to compile the code for both, without breaking anything */
typedef struct _cbm_serial_interface cbm_serial_interface;
struct _cbm_serial_interface
{
	int serial;			/* This is just a parameter, to log which interface is in use */
	void (*serial_reset_write)(running_machine *machine, int level);
	int (*serial_request_read)(running_machine *machine);
	void (*serial_request_write)(running_machine *machine, int level);
	int (*serial_atn_read)(running_machine *machine);
	int (*serial_data_read)(running_machine *machine);
	int (*serial_clock_read)(running_machine *machine);
	void (*serial_atn_write)(running_machine *machine, int level);
	void (*serial_data_write)(running_machine *machine, int level);
	void (*serial_clock_write)(running_machine *machine, int level);
};

void serial_config(running_machine *machine, const cbm_serial_interface *intf);

extern const cbm_serial_interface sim_drive_interface;		/* serial = 1 */
extern const cbm_serial_interface emu_drive_interface;		/* serial = 2 */
extern const cbm_serial_interface fake_drive_interface;	/* serial = 3 */


/* Serial bus for vic20, c64 & c16 with vc1541 and some printer */

/* To be passed directly to the drivers */
void cbm_serial_reset_write (running_machine *machine, int level);
int cbm_serial_atn_read (running_machine *machine);
void cbm_serial_atn_write (running_machine *machine, int level);
int cbm_serial_data_read (running_machine *machine);
void cbm_serial_data_write (running_machine *machine, int level);
int cbm_serial_clock_read (running_machine *machine);
void cbm_serial_clock_write (running_machine *machine, int level);
int cbm_serial_request_read (running_machine *machine);
void cbm_serial_request_write (running_machine *machine, int level);


#endif /* CBMSERB_H_ */
