#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "driver.h"
#include "image.h"

#define VERBOSE_DBG 0				   /* general debug messages */
#include "includes/cbm.h"
#include "includes/cbmdrive.h"

#include "includes/cbmserb.h"

static void vc1541_reset_write (CBM_Drive * vc1541, int level);

CBM_Drive cbm_drive[2];

CBM_Serial cbm_serial;

/* must be called before other functions */
static void cbm_drive_open (void)
{
	int i;

	memset(cbm_drive, 0, sizeof(cbm_drive));
	memset(&cbm_serial, 0, sizeof(cbm_serial));

	cbm_drive_open_helper ();

	cbm_serial.count = 0;
	for (i = 0; i < sizeof (cbm_serial.atn) / sizeof (int); i++)

	{
		cbm_serial.atn[i] =
			cbm_serial.data[i] =
			cbm_serial.clock[i] = 1;
	}
}

static void cbm_drive_close (void)
{
	int i;

	cbm_serial.count = 0;
	for (i = 0; i < sizeof (cbm_drive) / sizeof (CBM_Drive); i++)
	{
		cbm_drive[i].interface = 0;

		if (cbm_drive[i].drive == D64_IMAGE)
			cbm_drive[i].image = NULL;
		cbm_drive[i].drive = 0;
	}
}



static void cbm_drive_config (CBM_Drive * drive, int interface, int serialnr)
{
	int i;

	if (interface==SERIAL)
		drive->i.serial.device=serialnr;

	if (interface==IEEE)
		drive->i.ieee.device=serialnr;

	if (drive->interface == interface)
		return;

	if (drive->interface == SERIAL)
	{
		for (i = 0; (i < cbm_serial.count) && (cbm_serial.drives[i] != drive); i++) ;
		for (; i + 1 < cbm_serial.count; i++)
			cbm_serial.drives[i] = cbm_serial.drives[i + 1];
		cbm_serial.count--;
	}

	drive->interface = interface;

	if (drive->interface == IEC)
	{
		drive->i.iec.handshakein =
			drive->i.iec.handshakeout = 0;
		drive->i.iec.status = 0;
		drive->i.iec.dataout = drive->i.iec.datain = 0xff;
		drive->i.iec.state = 0;
	}
	else if (drive->interface == SERIAL)
	{
		cbm_serial.drives[cbm_serial.count++] = drive;
		vc1541_reset_write(drive, 0);
	}
}

void cbm_drive_0_config (int interface, int serialnr)
{
	cbm_drive_config (cbm_drive, interface, serialnr);
}
void cbm_drive_1_config (int interface, int serialnr)
{
	cbm_drive_config (cbm_drive + 1, interface, serialnr);
}



static int device_init_cbm_drive(mess_image *image)
{
	int id = image_index_in_device(image);
	if (id == 0)
		cbm_drive_open();	/* boy these C64 drivers are butt ugly */
	return INIT_PASS;
}

static void device_exit_cbm_drive(mess_image *image)
{
	int id = image_index_in_device(image);
	if (id == 0)
		cbm_drive_close();	/* boy these C64 drivers are butt ugly */
}



/* open an d64 image */
static int device_load_cbm_drive(mess_image *image)
{
	int id = image_index_in_device(image);

	cbm_drive[id].drive = 0;
	cbm_drive[id].image = NULL;

	cbm_drive[id].image = image_ptr(image);
	if (!cbm_drive[id].image)
		return INIT_FAIL;

	cbm_drive[id].drive = D64_IMAGE;
	return 0;
}



static void c1551_write_data (CBM_Drive * c1551, int data)
{
	c1551->i.iec.datain = data;
	c1551_state (c1551);
}

static int c1551_read_data (CBM_Drive * c1551)
{
	c1551_state (c1551);
	return c1551->i.iec.dataout;
}

static void c1551_write_handshake (CBM_Drive * c1551, int data)
{
	c1551->i.iec.handshakein = data&0x40?1:0;
	c1551_state (c1551);
}

static int c1551_read_handshake (CBM_Drive * c1551)
{
	c1551_state (c1551);
	return c1551->i.iec.handshakeout?0x80:0;
}

static int c1551_read_status (CBM_Drive * c1551)
{
	c1551_state (c1551);
	return c1551->i.iec.status;
}

void c1551_0_write_data (int data)
{
	c1551_write_data (cbm_drive, data);
}
int c1551_0_read_data (void)
{
	return c1551_read_data (cbm_drive);
}
void c1551_0_write_handshake (int data)
{
	c1551_write_handshake (cbm_drive, data);
}
int c1551_0_read_handshake (void)
{
	return c1551_read_handshake (cbm_drive);
}
int c1551_0_read_status (void)
{
	return c1551_read_status (cbm_drive);
}

void c1551_1_write_data (int data)
{
	c1551_write_data (cbm_drive + 1, data);
}
int c1551_1_read_data (void)
{
	return c1551_read_data (cbm_drive + 1);
}
void c1551_1_write_handshake (int data)
{
	c1551_write_handshake (cbm_drive + 1, data);
}
int c1551_1_read_handshake (void)
{
	return c1551_read_handshake (cbm_drive + 1);
}
int c1551_1_read_status (void)
{
	return c1551_read_status (cbm_drive + 1);
}

static void vc1541_reset_write (CBM_Drive * vc1541, int level)
{
	if (level == 0)
	{
		vc1541->i.serial.data =
			vc1541->i.serial.clock =
			vc1541->i.serial.atn = 1;
		vc1541->i.serial.state = 0;
	}
}

static int vc1541_atn_read (CBM_Drive * vc1541)
{
	vc1541_state (vc1541);
	return vc1541->i.serial.atn;
}

static int vc1541_data_read (CBM_Drive * vc1541)
{
	vc1541_state (vc1541);
	return vc1541->i.serial.data;
}

static int vc1541_clock_read (CBM_Drive * vc1541)
{
	vc1541_state (vc1541);
	return vc1541->i.serial.clock;
}

static void vc1541_data_write (CBM_Drive * vc1541, int level)
{
	vc1541_state (vc1541);
}

static void vc1541_clock_write (CBM_Drive * vc1541, int level)
{
	vc1541_state (vc1541);
}

static void vc1541_atn_write (CBM_Drive * vc1541, int level)
{
	vc1541_state (vc1541);
}


/* bus handling */
void cbm_serial_reset_write (int level)
{
	int i;

	for (i = 0; i < cbm_serial.count; i++)
		vc1541_reset_write (cbm_serial.drives[i], level);
	/* init bus signals */
}

int cbm_serial_request_read (void)
{
	/* in c16 not connected */
	return 1;
}

void cbm_serial_request_write (int level)
{
}

int cbm_serial_atn_read (void)
{
	int i;

	cbm_serial.atn[0] = cbm_serial.atn[1];
	for (i = 0; i < cbm_serial.count; i++)
		cbm_serial.atn[0] &= cbm_serial.atn[i + 2] =
			vc1541_atn_read (cbm_serial.drives[i]);
	return cbm_serial.atn[0];
}

int cbm_serial_data_read (void)
{
	int i;

	cbm_serial.data[0] = cbm_serial.data[1];
	for (i = 0; i < cbm_serial.count; i++)
		cbm_serial.data[0] &= cbm_serial.data[i + 2] =
			vc1541_data_read (cbm_serial.drives[i]);
	return cbm_serial.data[0];
}

int cbm_serial_clock_read (void)
{
	int i;

	cbm_serial.clock[0] = cbm_serial.clock[1];
	for (i = 0; i < cbm_serial.count; i++)
		cbm_serial.clock[0] &= cbm_serial.clock[i + 2] =
			vc1541_clock_read (cbm_serial.drives[i]);
	return cbm_serial.clock[0];
}

void cbm_serial_data_write (int level)
{
	int i;

	cbm_serial.data[0] =
		cbm_serial.data[1] = level;
	/* update line */
	for (i = 0; i < cbm_serial.count; i++)
		cbm_serial.data[0] &= cbm_serial.data[i + 2];
	/* inform drives */
	for (i = 0; i < cbm_serial.count; i++)
		vc1541_data_write (cbm_serial.drives[i], cbm_serial.data[0]);
}

void cbm_serial_clock_write (int level)
{
	int i;

	cbm_serial.clock[0] =
		cbm_serial.clock[1] = level;
	/* update line */
	for (i = 0; i < cbm_serial.count; i++)
		cbm_serial.clock[0] &= cbm_serial.clock[i + 2];
	/* inform drives */
	for (i = 0; i < cbm_serial.count; i++)
		vc1541_clock_write (cbm_serial.drives[i], cbm_serial.clock[0]);
}

void cbm_serial_atn_write (int level)
{
	int i;

	cbm_serial.atn[0] =
		cbm_serial.atn[1] = level;
	/* update line */
	for (i = 0; i < cbm_serial.count; i++)
		cbm_serial.atn[0] &= cbm_serial.atn[i + 2];
	/* inform drives */
	for (i = 0; i < cbm_serial.count; i++)
		vc1541_atn_write (cbm_serial.drives[i], cbm_serial.atn[0]);
}

/* delivers status for displaying */
static void cbm_drive_status (CBM_Drive * c1551, char *text, int size)
{
	text[0] = 0;
#if VERBOSE_DBG
	if ((c1551->interface == SERIAL) /*&&(c1551->i.serial.device==8) */ )
	{
		snprintf (text, size, "%d state:%d %d %d %s %s %s",
				  c1551->state, c1551->i.serial.state, c1551->pos, c1551->size,
				  cbm_serial.atn[0] ? "ATN" : "atn",
				  cbm_serial.clock[0] ? "CLOCK" : "clock",
				  cbm_serial.data[0] ? "DATA" : "data");
		return;
	}
	if ((c1551->interface == IEC) /*&&(c1551->i.serial.device==8) */ )
	{
		snprintf (text, size, "%d state:%d %d %d",
				  c1551->state, c1551->i.iec.state, c1551->pos, c1551->size);
		return;
	}
#endif
	if (c1551->drive == D64_IMAGE)
	{
		switch (c1551->state)
		{
		case OPEN:
			snprintf (text, size, "Image File %s open",
					  c1551->filename);
			break;
		case READING:
			snprintf (text, size, "Image File %s loading %d",
					  c1551->filename,
					  c1551->size - c1551->pos - 1);
			break;
		case WRITING:
			snprintf (text, size, "Image File %s saving %d",
					  c1551->filename, c1551->pos);
			break;
		}
	}
}

void cbm_drive_0_status (char *text, int size)
{
	cbm_drive_status (cbm_drive, text, size);
}

void cbm_drive_1_status (char *text, int size)
{
	cbm_drive_status (cbm_drive + 1, text, size);
}

void cbmfloppy_device_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:							info->i = IO_FLOPPY; break;
		case DEVINFO_INT_READABLE:						info->i = 1; break;
		case DEVINFO_INT_WRITEABLE:						info->i = 0; break;
		case DEVINFO_INT_CREATABLE:						info->i = 0; break;
		case DEVINFO_INT_COUNT:							info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_INIT:							info->init = device_init_cbm_drive; break;
		case DEVINFO_PTR_EXIT:							info->exit = device_exit_cbm_drive; break;
		case DEVINFO_PTR_LOAD:							info->load = device_load_cbm_drive; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "d64"); break;
	}
}

