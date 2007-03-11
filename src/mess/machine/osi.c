#include "driver.h"
#include "image.h"
#include "machine/mc6850.h"
#include "sound/discrete.h"

/* ACIA */

static UINT8 *sb2m600_tape_image;
static int sb2m600_tape_size;
static int sb2m600_tape_index;

READ8_HANDLER( sb2m600_acia0_casin )
{
	if (sb2m600_tape_image && (sb2m600_tape_index < sb2m600_tape_size))
		return sb2m600_tape_image[sb2m600_tape_index++];

	return 0;
}

READ8_HANDLER (sb2m600_acia0_statin )
{
	if (sb2m600_tape_image && (sb2m600_tape_index < sb2m600_tape_size))
		return ACIA_6850_RDRF;
	
	return 0;
}

static struct acia6850_interface sb2m600_acia0 =
{
	sb2m600_acia0_statin,
	sb2m600_acia0_casin,
	0,
	0
};

static struct acia6850_interface sb2m600_acia1 =
{
	0,
	0,
	0,
	0
};

DRIVER_INIT( sb2m600 )
{
	acia6850_config_old(0, &sb2m600_acia0);
	acia6850_config_old(1, &sb2m600_acia1); // DISK CONTROLLER
}

/* Cassette */

DEVICE_LOAD( sb2m600_cassette )
{
	sb2m600_tape_image = (UINT8 *)image_malloc(image, sb2m600_tape_size);
	sb2m600_tape_size = image_length(image);

	if (!sb2m600_tape_image || (image_fread(image, sb2m600_tape_image, sb2m600_tape_size) != sb2m600_tape_size))
	{
		return INIT_FAIL;
	}

	sb2m600_tape_index = 0;

	return INIT_PASS;
}

DEVICE_UNLOAD( sb2m600_cassette )
{
	sb2m600_tape_image = NULL;
	sb2m600_tape_size = 0;
	sb2m600_tape_index = 0;
}

/* Keyboard */

static UINT8 key_row_latch;

READ8_HANDLER( osi_keyboard_r )
{
	UINT8 key_column_data = 0xff;

	if (!(key_row_latch & 0x01)) key_column_data &= readinputport(0);
	if (!(key_row_latch & 0x02)) key_column_data &= readinputport(1);
	if (!(key_row_latch & 0x04)) key_column_data &= readinputport(2);
	if (!(key_row_latch & 0x08)) key_column_data &= readinputport(3);
	if (!(key_row_latch & 0x10)) key_column_data &= readinputport(4);
	if (!(key_row_latch & 0x20)) key_column_data &= readinputport(5);
	if (!(key_row_latch & 0x40)) key_column_data &= readinputport(6);
	if (!(key_row_latch & 0x80)) key_column_data &= readinputport(7);

	return key_column_data;
}

WRITE8_HANDLER ( sb2m600b_keyboard_w )
{
	key_row_latch = data;
	discrete_sound_w(NODE_01, (data >> 2) & 0x0f);
}

WRITE8_HANDLER ( uk101_keyboard_w )
{
	key_row_latch = data;
}

/* Disk Drive */

READ8_HANDLER( osi470_floppy_status_r )
{
	/*
		C000 FLOPIN			FLOPPY DISK STATUS PORT

		BIT	FUNCTION
		0	DRIVE 0 READY (0 IF READY)
		1	TRACK 0 (0 IF AT TRACK 0)
		2	FAULT (0 IF FAULT)
		3
		4	DRIVE 1 READY (0 IF READY)
		5	WRITE PROTECT (0 IF WRITE PROTECT)
		6	DRIVE SELECT (1 = A OR C, 0 = B OR D)
		7	INDEX (0 IF AT INDEX HOLE)
	*/
	return 0x00;
}

WRITE8_HANDLER( osi470_floppy_control_w )
{
	/*
		C002 FLOPOT			FLOPPY DISK CONTROL PORT

		BIT	FUNCTION
		0	WRITE ENABLE (0 ALLOWS WRITING)
		1	ERASE ENABLE (0 ALLOWS ERASING)
			ERASE ENABLE IS ON 200us AFTER WRITE IS ON
			ERASE ENABLE IS OFF 530us AFTER WRITE IS OFF
		2	STEP BIT : INDICATES DIRECTION OF STEP (WAIT 10us FIRST)
			0 INDICATES STEP TOWARD 76
			1 INDICATES STEP TOWARD 0
		3	STEP (TRANSITION FROM 1 TO 0)
			MUST HOLD AT LEAST 10us, MIN 8us BETWEEN
		4	FAULT RESET (0 RESETS)
		5	SIDE SELECT (1 = A OR B, 0 = C OR D)
		6	LOW CURRENT (0 FOR TRKS 43-76, 1 FOR TRKS 0-42)
		7	HEAD LOAD (0 TO LOAD : MUST WAIT 40ms AFTER)

		C010 ACIA			DISK CONTROLLER ACIA STATUS PORT
		C011 ACIAIO			DISK CONTROLLER ACIA I/O PORT
	*/
}
