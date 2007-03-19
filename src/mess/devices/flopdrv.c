/*
	This code handles the floppy drives.
	All FDD actions should be performed using these functions.

	The functions are emulated and a disk image is used.

  Disk image operation:
  - set disk image functions using floppy_drive_set_disk_image_interface

  Real disk operation:
  - set unit id

  TODO:
	- Disk change handling.
	- Override write protect if disk image has been opened in read mode
*/

#include "driver.h"
#include "devices/flopdrv.h"
#include "image.h"

#define VERBOSE		0
#define FLOPDRVTAG	"flopdrv"

static struct floppy_drive *get_drive(mess_image *img)
{
	return image_lookuptag(img, FLOPDRVTAG);
}

static void	floppy_drive_index_callback(int image_ptr);

/* this is called on device init */
int floppy_drive_init(mess_image *img, const floppy_interface *iface)
{
	struct floppy_drive *pDrive;

	assert(image_slotexists(img));

	pDrive = image_alloctag(img, FLOPDRVTAG, sizeof(struct floppy_drive));
	if (!pDrive)
		return INIT_FAIL;

	/* initialise flags */
	pDrive->flags = 0;
	pDrive->index_pulse_callback = NULL;
	pDrive->ready_state_change_callback = NULL;
	pDrive->index_timer = timer_alloc(floppy_drive_index_callback);
	pDrive->index = 0;

	/* all drives are double-sided 80 track - can be overriden in driver! */
	floppy_drive_set_geometry(img, FLOPPY_DRIVE_DS_80);

	/* initialise id index - not so important */
	pDrive->id_index = 0;
	/* initialise track */
	pDrive->current_track = 1;

	/* default RPM */
	pDrive->rpm = 300;

	floppy_drive_set_disk_image_interface(img, iface);
	return INIT_PASS;
}



/* index pulses at rpm/60 Hz, and stays high 1/20th of time */
static void	floppy_drive_index_callback(int image_ptr)
{
	mess_image *img = (mess_image *) image_ptr;
	struct floppy_drive *pDrive = get_drive(img);

	double ms = 1000. / (pDrive->rpm / 60.);

	if (pDrive->index)
	{
		pDrive->index = 0;
		timer_adjust(pDrive->index_timer, TIME_IN_MSEC(ms*19/20), (int) img, 0);
	}
	else
	{
		pDrive->index = 1;
		timer_adjust(pDrive->index_timer, TIME_IN_MSEC(ms/20), (int) img, 0);
	}

	if (pDrive->index_pulse_callback)
		pDrive->index_pulse_callback(img, pDrive->index);
}


/* set the callback for the index pulse */
void floppy_drive_set_index_pulse_callback(mess_image *img, void (*callback)(mess_image *image, int state))
{
	struct floppy_drive *pDrive = get_drive(img);
	pDrive->index_pulse_callback = callback;
}


void floppy_drive_set_ready_state_change_callback(mess_image *img, void (*callback)(mess_image *img, int state))
{
	struct floppy_drive *pDrive = get_drive(img);
	pDrive->ready_state_change_callback = callback;
}

/*************************************************************************/
/* IO_FLOPPY device functions */

/* return and set current status
  use for setting:-
  1) write protect/enable
  2) drive present/missing
*/

int	floppy_status(mess_image *img, int new_status)
{
	/* return current status only? */
	if (new_status!=-1)
	{
		/* we don't set the flags directly.
		The flags are "cooked" when we do a floppy_drive_get_flag_state depending on
		if drive is connected etc. So if we wrote the flags back it would
		corrupt this information. Therefore we update the flags depending on new_status */

		floppy_drive_set_flag_state(img, FLOPPY_DRIVE_DISK_WRITE_PROTECTED, (new_status & FLOPPY_DRIVE_DISK_WRITE_PROTECTED));
	}

	/* return current status */
	return floppy_drive_get_flag_state(img,0x0ff);
}

/* set interface for image interface */
void floppy_drive_set_disk_image_interface(mess_image *img, const floppy_interface *iface)
{
	struct floppy_drive *pDrive = get_drive(img);
	if (iface)
		memcpy(&pDrive->interface_, iface, sizeof(floppy_interface));
	else
		memset(&pDrive->interface_, 0, sizeof(floppy_interface));
}

/* set flag state */
void floppy_drive_set_flag_state(mess_image *img, int flag, int state)
{
	struct floppy_drive *drv = get_drive(img);
	int prev_state;
	int new_state;

	/* get old state */
	prev_state = drv->flags & flag;

	/* set new state */
	drv->flags &= ~flag;
	if (state)
		drv->flags |= flag;

	/* get new state */
	new_state = drv->flags & flag;

	/* changed state? */
	if (prev_state ^ new_state)
	{
		if (flag & FLOPPY_DRIVE_READY)
		{
			/* trigger state change callback */
			if (drv->ready_state_change_callback)
				drv->ready_state_change_callback(img, new_state);
		}
	}
}

void floppy_drive_set_motor_state(mess_image *img, int state)
{
	int new_motor_state = 0;
	int previous_state = 0;

	/* previous state */
	if (floppy_drive_get_flag_state(img, FLOPPY_DRIVE_MOTOR_ON))
		previous_state = 1;

	/* calc new state */

	/* drive present? */
	if (image_slotexists(img))
	{
		/* disk inserted? */
		if (image_exists(img))
		{
			/* drive present and disc inserted */

			/* state of motor is same as the programmed state */
			if (state)
			{
				new_motor_state = 1;
			}
		}
	}

	if ((new_motor_state^previous_state)!=0)
	{
		/* if timer already setup remove it */
		if (image_slotexists(img))
		{
			struct floppy_drive *pDrive = get_drive(img);

			pDrive->index = 0;
			if (new_motor_state)
			{
				/* off->on */
				/* check it's in range */

				/* setup timer to trigger at rpm */
				floppy_drive_index_callback((int)img);
			}
			else
			{
				/* on->off */
				timer_adjust(pDrive->index_timer, 0, (int) img, 0);
			}
		}
	}

	floppy_drive_set_flag_state(img, FLOPPY_DRIVE_MOTOR_ON, new_motor_state);

}

/* for pc, drive is always ready, for amstrad,pcw,spectrum it is only ready under
a fixed set of circumstances */
/* use this to set ready state of drive */
void floppy_drive_set_ready_state(mess_image *img, int state, int flag)
{
	if (flag)
	{
		/* set ready only if drive is present, disk is in the drive,
		and disk motor is on - for Amstrad, Spectrum and PCW*/

		/* drive present? */
		if (image_slotexists(img))
		{
			/* disk inserted? */
			if (image_exists(img))
			{
				if (floppy_drive_get_flag_state(img, FLOPPY_DRIVE_MOTOR_ON))
				{

					/* set state */
					floppy_drive_set_flag_state(img, FLOPPY_DRIVE_READY, state);
                    return;
				}
			}
		}

		floppy_drive_set_flag_state(img, FLOPPY_DRIVE_READY, 0);
	}
	else
	{
		/* force ready state - for PC driver */
		floppy_drive_set_flag_state(img, FLOPPY_DRIVE_READY, state);
	}

}


/* get flag state */
int	floppy_drive_get_flag_state(mess_image *img, int flag)
{
	struct floppy_drive *drv = get_drive(img);
	int drive_flags;
	int flags;

	flags = 0;

	drive_flags = drv->flags;

	/* these flags are independant of a real drive/disk image */
    flags |= drive_flags & (FLOPPY_DRIVE_READY | FLOPPY_DRIVE_MOTOR_ON | FLOPPY_DRIVE_INDEX);

	flags |= drive_flags & FLOPPY_DRIVE_HEAD_AT_TRACK_0;

	/* if disk image is read-only return write protected all the time */
	if (!image_is_writable(img))
	{
		flags |= FLOPPY_DRIVE_DISK_WRITE_PROTECTED;
	}
	else
	{
		/* return real state of write protected flag */
		flags |= drive_flags & FLOPPY_DRIVE_DISK_WRITE_PROTECTED;
	}

	/* drive present not */
	if (!image_slotexists(img))
	{
		/* adjust some flags if drive is not present */
		flags &= ~FLOPPY_DRIVE_HEAD_AT_TRACK_0;
		flags |= FLOPPY_DRIVE_DISK_WRITE_PROTECTED;
	}

    flags &= flag;

	return flags;
}


void floppy_drive_set_geometry(mess_image *img, floppy_type type)
{
	int max_track, num_sides;

	switch (type) {
	case FLOPPY_DRIVE_SS_40:	/* single sided, 40 track drive e.g. Amstrad CPC internal 3" drive */
		max_track = 42;
		num_sides = 1;
		break;

	case FLOPPY_DRIVE_DS_80:
		max_track = 83;
		num_sides = 2;
		break;

	default:
		assert(0);
		return;
	}
	floppy_drive_set_geometry_absolute(img, max_track, num_sides);
}

void floppy_drive_set_geometry_absolute(mess_image *img, int tracks, int sides)
{
	struct floppy_drive *pDrive = get_drive(img);
	pDrive->max_track = tracks;
	pDrive->num_sides = sides;
}

void floppy_drive_seek(mess_image *img, signed int signed_tracks)
{
	struct floppy_drive *pDrive;

	pDrive = get_drive(img);

#if VERBOSE
	logerror("seek from: %d delta: %d\n",pDrive->current_track, signed_tracks);
#endif

	/* update position */
	pDrive->current_track+=signed_tracks;

	if (pDrive->current_track<0)
	{
		pDrive->current_track = 0;
	}
	else
	if (pDrive->current_track>=pDrive->max_track)
	{
		pDrive->current_track = pDrive->max_track-1;
	}

	/* set track 0 flag */
	pDrive->flags &= ~FLOPPY_DRIVE_HEAD_AT_TRACK_0;

	if (pDrive->current_track==0)
	{
		pDrive->flags |= FLOPPY_DRIVE_HEAD_AT_TRACK_0;
	}

	/* inform disk image of step operation so it can cache information */
	if (image_exists(img) && pDrive->interface_.seek_callback)
		pDrive->interface_.seek_callback(img, pDrive->current_track);
        
        pDrive->id_index = 0;
}


/* this is not accurate. But it will do for now */
int	floppy_drive_get_next_id(mess_image *img, int side, chrn_id *id)
{
	struct floppy_drive *pDrive;
	int spt;

	pDrive = get_drive(img);

	/* get sectors per track */
	spt = 0;
	if (pDrive->interface_.get_sectors_per_track)
		spt = pDrive->interface_.get_sectors_per_track(img, side);

	/* set index */
	if ((pDrive->id_index==(spt-1)) || (spt==0))
	{
		floppy_drive_set_flag_state(img, FLOPPY_DRIVE_INDEX, 1);
	}
	else
	{
		floppy_drive_set_flag_state(img, FLOPPY_DRIVE_INDEX, 0);
	}

	/* get id */
	if (spt!=0)
	{
		if (pDrive->interface_.get_id_callback)
			pDrive->interface_.get_id_callback(img, id, pDrive->id_index, side);
	}

	pDrive->id_index++;
	if (spt!=0)
		pDrive->id_index %= spt;
	else
		pDrive->id_index = 0;

	return (spt == 0) ? 0 : 1;
}

int	floppy_drive_get_current_track(mess_image *img)
{
	struct floppy_drive *drv = get_drive(img);
	return drv->current_track;
}

void floppy_drive_read_track_data_info_buffer(mess_image *img, int side, void *ptr, int *length )
{
	if (image_exists(img))
	{
		struct floppy_drive *drv = get_drive(img);
		if (drv->interface_.read_track_data_info_buffer)
			drv->interface_.read_track_data_info_buffer(img, side, ptr, length);
	}
}

void floppy_drive_write_track_data_info_buffer(mess_image *img, int side, const void *ptr, int *length )
{
	if (image_exists(img))
	{
		struct floppy_drive *drv = get_drive(img);
		if (drv->interface_.write_track_data_info_buffer)
			drv->interface_.write_track_data_info_buffer(img, side, ptr, length);
	}
}

void floppy_drive_format_sector(mess_image *img, int side, int sector_index,int c,int h, int r, int n, int filler)
{
	if (image_exists(img))
	{
		struct floppy_drive *drv = get_drive(img);
		if (drv->interface_.format_sector)
			drv->interface_.format_sector(img, side, sector_index,c, h, r, n, filler);
	}
}

void floppy_drive_read_sector_data(mess_image *img, int side, int index1, void *pBuffer, int length)
{
	if (image_exists(img))
	{
		struct floppy_drive *drv = get_drive(img);
		if (drv->interface_.read_sector_data_into_buffer)
			drv->interface_.read_sector_data_into_buffer(img, side, index1, pBuffer,length);
	}
}

void floppy_drive_write_sector_data(mess_image *img, int side, int index1, const void *pBuffer,int length, int ddam)
{
	if (image_exists(img))
	{
		struct floppy_drive *drv = get_drive(img);
		if (drv->interface_.write_sector_data_from_buffer)
			drv->interface_.write_sector_data_from_buffer(img, side, index1, pBuffer,length,ddam);
	}
}

int floppy_drive_get_datarate_in_us(DENSITY density)
{
	int usecs;
	/* 64 for single density */
	switch (density)
	{
		case DEN_FM_LO:
		{
			usecs = 128;
		}
		break;

		case DEN_FM_HI:
		{
			usecs = 64;
		}
		break;

		default:
		case DEN_MFM_LO:
		{
			usecs = 32;
		}
		break;

		case DEN_MFM_HI:
		{
			usecs = 16;
		}
		break;
	}

	return usecs;
}



void floppy_drive_set_rpm(mess_image *img, float rpm)
{
	struct floppy_drive *drv = get_drive(img);
	drv->rpm = rpm;
}
