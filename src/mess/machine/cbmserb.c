#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "driver.h"
#include "image.h"
#include "deprecat.h"

#include "includes/cbmserb.h"

/* with these we include the handlers for the interfaces below */
#include "includes/vc1541.h"
#include "includes/cbmdrive.h"


/**************************************

	Handlers for the simulated drive

**************************************/


static void sim_drive_reset_write (int level)
{
	int i;

	for (i = 0; i < cbm_serial.count; i++)
		drive_reset_write (cbm_serial.drives[i], level);
	/* init bus signals */
}

static int sim_drive_request_read (void)
{
	/* in c16 not connected */
	return 1;
}

static void sim_drive_request_write (int level)
{
}

static int sim_drive_atn_read (void)
{
	int i;

	cbm_serial.atn[0] = cbm_serial.atn[1];
	for (i = 0; i < cbm_serial.count; i++)
		cbm_serial.atn[0] &= cbm_serial.atn[i + 2] =
			vc1541_atn_read (Machine, cbm_serial.drives[i]);
	return cbm_serial.atn[0];
}

static int sim_drive_data_read (void)
{
	int i;

	cbm_serial.data[0] = cbm_serial.data[1];
	for (i = 0; i < cbm_serial.count; i++)
		cbm_serial.data[0] &= cbm_serial.data[i + 2] =
			vc1541_data_read (Machine, cbm_serial.drives[i]);
	return cbm_serial.data[0];
}

static int sim_drive_clock_read (void)
{
	int i;

	cbm_serial.clock[0] = cbm_serial.clock[1];
	for (i = 0; i < cbm_serial.count; i++)
		cbm_serial.clock[0] &= cbm_serial.clock[i + 2] =
			vc1541_clock_read (Machine, cbm_serial.drives[i]);
	return cbm_serial.clock[0];
}

static void sim_drive_data_write (int level)
{
	int i;

	cbm_serial.data[0] =
		cbm_serial.data[1] = level;
	/* update line */
	for (i = 0; i < cbm_serial.count; i++)
		cbm_serial.data[0] &= cbm_serial.data[i + 2];
	/* inform drives */
	for (i = 0; i < cbm_serial.count; i++)
		vc1541_data_write (Machine, cbm_serial.drives[i], cbm_serial.data[0]);
}

static void sim_drive_clock_write (int level)
{
	int i;

	cbm_serial.clock[0] =
		cbm_serial.clock[1] = level;
	/* update line */
	for (i = 0; i < cbm_serial.count; i++)
		cbm_serial.clock[0] &= cbm_serial.clock[i + 2];
	/* inform drives */
	for (i = 0; i < cbm_serial.count; i++)
		vc1541_clock_write (Machine, cbm_serial.drives[i], cbm_serial.clock[0]);
}

static void sim_drive_atn_write (int level)
{
	int i;

	cbm_serial.atn[0] =
		cbm_serial.atn[1] = level;
	/* update line */
	for (i = 0; i < cbm_serial.count; i++)
		cbm_serial.atn[0] &= cbm_serial.atn[i + 2];
	/* inform drives */
	for (i = 0; i < cbm_serial.count; i++)
		vc1541_atn_write (Machine, cbm_serial.drives[i], cbm_serial.atn[0]);
}


const cbm_serial_interface sim_drive_interface =
{
	1,
	sim_drive_reset_write,
	sim_drive_request_read,
	sim_drive_request_write,
	sim_drive_atn_read,
	sim_drive_data_read,
	sim_drive_clock_read,
	sim_drive_atn_write,
	sim_drive_data_write,
	sim_drive_clock_write
};


/**************************************

	Handlers for the emulated drive

**************************************/


static void emu_drive_reset_write( int level )
{
	vc1541_serial_reset_write(0, level);
}

static int emu_drive_request_read( void )	
{
	return vc1541_serial_request_read(0);
}
	
static void emu_drive_request_write( int level )
{
	vc1541_serial_request_write(0, level);
}

static int emu_drive_atn_read( void )
{
	return vc1541_serial_atn_read(0);
}

static int emu_drive_data_read( void )
{
	return vc1541_serial_data_read(0);
}

static int emu_drive_clock_read( void )
{
	return vc1541_serial_clock_read(0);
}

static void emu_drive_atn_write( int level )
{
	vc1541_serial_atn_write(Machine, 0, level);
}

static void emu_drive_data_write( int level )
{
	vc1541_serial_data_write(0, level);
}

static void emu_drive_clock_write( int level )
{
	vc1541_serial_clock_write(0, level);
}

const cbm_serial_interface emu_drive_interface =
{
	2,
	emu_drive_reset_write,
	emu_drive_request_read,
	emu_drive_request_write,
	emu_drive_atn_read,
	emu_drive_data_read,
	emu_drive_clock_read,
	emu_drive_atn_write,
	emu_drive_data_write,
	emu_drive_clock_write
};


/**************************************

	Empty handlers

**************************************/


/* In sx64, c16v & vic20v we use these while fixing handlers for real emulation */

static void fake_drive_reset_write( int level )
{
}

static int fake_drive_request_read( void )	
{
	return 0;
}
	
static void fake_drive_request_write( int level )
{
}

static int fake_drive_atn_read( void )
{
	return 0;
}

static int fake_drive_data_read( void )
{
	return 0;
}

static int fake_drive_clock_read( void )
{
	return 0;
}

static void fake_drive_atn_write( int level )
{
}

static void fake_drive_data_write( int level )
{
}

static void fake_drive_clock_write( int level )
{
}

const cbm_serial_interface fake_drive_interface =
{
	3,
	fake_drive_reset_write,
	fake_drive_request_read,
	fake_drive_request_write,
	fake_drive_atn_read,
	fake_drive_data_read,
	fake_drive_clock_read,
	fake_drive_atn_write,
	fake_drive_data_write,
	fake_drive_clock_write
};	
	
/**************************************

	Serial communications

**************************************/

/* 2008-09 FP:
	To make possible to test code for both simulated floppy drive and emulated floppy drive, I 
	created this interface for the serial bus emulation. The implementation is maybe not the cleanest 
	possible, but it works.
	This is necessary because using simulation we support two drives (see the handlers above) while
	the emulation only supports one drive (the handlers have a 'which' parameter, but the reset and
	config functions are strictly for 1 drive). 
	Eventually, we will remove the simulation and here we will have only the emu_drive_*** above 
	renamed as cbm_serial_*** and the whole interface will be trashed.
 */
/* To Do: Can we pass more directly the handlers from the interface to the drivers?	*/

static cbm_serial_interface serial_intf_static= { 0 }, *serial_intf = &serial_intf_static;

void serial_config(running_machine *machine, const cbm_serial_interface *intf)
{
	serial_intf->serial = intf->serial;
	serial_intf->serial_reset_write = intf->serial_reset_write;
	serial_intf->serial_request_read = intf->serial_request_read;
	serial_intf->serial_request_write = intf->serial_request_write;
	serial_intf->serial_atn_read = intf->serial_atn_read;
	serial_intf->serial_data_read = intf->serial_data_read;
	serial_intf->serial_clock_read = intf->serial_clock_read;
	serial_intf->serial_atn_write = intf->serial_atn_write;
	serial_intf->serial_data_write = intf->serial_data_write;
	serial_intf->serial_clock_write = intf->serial_clock_write;

	logerror("Serial interface in use: %d\n", serial_intf->serial);
}


/* bus handling */
void cbm_serial_reset_write (int level)
{
	serial_intf->serial_reset_write(level);
}

int cbm_serial_request_read (void)
{
	return serial_intf->serial_request_read();
}

void cbm_serial_request_write (int level)
{
	serial_intf->serial_request_write(level);
}

int cbm_serial_atn_read (void)
{
	return serial_intf->serial_atn_read();
}

int cbm_serial_data_read (void)
{
	return serial_intf->serial_data_read();
}

int cbm_serial_clock_read (void)
{
	return serial_intf->serial_clock_read();
}

void cbm_serial_data_write (int level)
{
	serial_intf->serial_data_write(level);
}

void cbm_serial_clock_write (int level)
{
	serial_intf->serial_clock_write(level);
}

void cbm_serial_atn_write (int level)
{
	serial_intf->serial_atn_write(level);
}
