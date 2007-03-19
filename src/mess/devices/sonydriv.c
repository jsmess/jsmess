/*********************************************************************

	sonydriv.c

	Apple/Sony 3.5" floppy drive emulation (to be interfaced with iwm.c)

	Nate Woods, Raphael Nabet

	This floppy drive was present in all variants of Lisa 2 (including Mac XL),
	and in all Macintosh in production before 1988, when SIWM and superdrive
	were introduced.

	There were two variants :
	- A single-sided 400k unit which was used on Lisa 2/Mac XL, and Macintosh
	  128k/512k.  This unit needs the computer to send the proper pulses to
	  control the drive motor rotation.  It can be connected to all early
	  Macintosh (but not Mac Classic?) as an external unit.
	- A double-sided 800k unit which was used on Macintosh Plus, 512ke, and
	  early SE and II*.  This unit generates its own drive motor rotation
	  control signals.  It can be connected to earlier (and later) Macintosh as
	  an external or internal unit.  Some Lisa2/10 and Mac XL were upgraded to
	  use it, too, but a fdc ROM upgrade was required.

	* Note: I don't know for sure whether some Mac II were actually produced
	  with a Superdrive unit, though Apple did sell an upgrade kit, made of
	  a Superdrive unit, a SIWM controller and upgraded systems ROMs, which
	  added HD and MFM support to existing Macintosh II.

	TODO :
	* bare images are supposed to be in Macintosh format (look for formatByte
	  in sony_floppy_init(), and you will understand).  We need to support
	  Lisa, Apple II...
	* support for other image formats?
	* should we support more than 2 floppy disk units? (Mac SE *MAY* have
	  supported 3 drives)

*********************************************************************/

#include "sonydriv.h"
#include "mame.h"
#include "timer.h"
#include "cpuintrf.h"
#include "image.h"
#include "formats/ap_dsk35.h"
#include "devices/flopdrv.h"
#include "devices/mflopimg.h"

#ifdef MAME_DEBUG
#define LOG_SONY		1
#define LOG_SONY_EXTRA	0
#else
#define LOG_SONY		0
#define LOG_SONY_EXTRA	0
#endif

static const device_class parent_devclass = { floppy_device_getinfo, NULL };

/*
	These lines are normally connected to the PHI0-PHI3 lines of the IWM
*/
enum
{
	SONY_CA0		= 0x01,
	SONY_CA1		= 0x02,
	SONY_CA2		= 0x04,
	SONY_LSTRB		= 0x08
};
static int sony_lines;				/* four lines SONY_CA0 - SONY_LSTRB */

static int sony_floppy_enable = 0;	/* whether a drive is enabled or not (-> enable line) */
static int sony_floppy_select = 0;	/* which drive is enabled */

static int sony_sel_line;			/* one single line Is 0 or 1 */

static unsigned int rotation_speed;		/* drive rotation speed - ignored if ext_speed_control == 0 */

/*
	Structure that describes the state of a floppy drive, and the associated
	disk image
*/
typedef struct
{
	mess_image *img;
	mame_file *fd;

	unsigned int ext_speed_control : 1;	/* is motor rotation controlled by external device ? */

	unsigned int disk_switched : 1;	/* disk-in-place status bit */
	unsigned int head : 1;			/* active head (-> floppy side) */
	unsigned int step : 1;

	unsigned int loadedtrack_valid : 1;	/* is data in track buffer valid ? */
	unsigned int loadedtrack_dirty : 1;	/* has data in track buffer been modified? */
	size_t loadedtrack_size;			/* size of loaded track */
	size_t loadedtrack_pos;				/* position within loaded track */
	UINT8 *loadedtrack_data;			/* pointer to track buffer */
} floppy;

static floppy sony_floppy[2];			/* data for two floppy disk units */



/* bit of code used in several places - I am unsure why it is here */
static int sony_enable2(void)
{
	return (sony_lines & SONY_CA1) && (sony_lines & SONY_LSTRB);
}

static floppy *get_sony_floppy(mess_image *img)
{
	int id = image_index_in_device(img);
	return &sony_floppy[id];
}



static void load_track_data(int floppy_select)
{
	int track_size;
	mess_image *cur_image;
	UINT8 *new_data;
	floppy *f;

	f = &sony_floppy[floppy_select];
	cur_image = image_from_devtag_and_index("sonydriv", floppy_select);

	track_size = floppy_get_track_size(flopimg_get_image(cur_image), f->head, floppy_drive_get_current_track(cur_image));
	new_data = image_realloc(cur_image, f->loadedtrack_data, track_size);
	if (!new_data)
		return;

	floppy_drive_read_track_data_info_buffer(cur_image, f->head, new_data, &track_size);
	f->loadedtrack_valid = 1;
	f->loadedtrack_dirty = 0;
	f->loadedtrack_size = track_size;
	f->loadedtrack_data = new_data;
	f->loadedtrack_pos = 0;
}



static void save_track_data(int floppy_select)
{
	mess_image *cur_image;
	floppy *f;
	int len;

	f = &sony_floppy[floppy_select];
	cur_image = image_from_devtag_and_index("sonydriv", floppy_select);

	if (f->loadedtrack_dirty)
	{
		len = f->loadedtrack_size;
		floppy_drive_write_track_data_info_buffer(cur_image, f->head, f->loadedtrack_data, &len);
		f->loadedtrack_dirty = 0;
	}
}



UINT8 sony_read_data(void)
{
	UINT8 result = 0;
	mess_image *cur_image;
	floppy *f;

	if (sony_enable2() || (! sony_floppy_enable))
		return 0xFF;			/* right ??? */

	f = &sony_floppy[sony_floppy_select];
	cur_image = image_from_devtag_and_index("sonydriv", sony_floppy_select);
	if (!image_exists(cur_image))
		return 0xFF;

	if (!f->loadedtrack_valid)
		load_track_data(sony_floppy_select);

	result = sony_fetchtrack(f->loadedtrack_data, f->loadedtrack_size, &f->loadedtrack_pos);
	return result;
}



void sony_write_data(UINT8 data)
{
	mess_image *cur_image;
	floppy *f;

	f = &sony_floppy[sony_floppy_select];
	cur_image = image_from_devtag_and_index("sonydriv", sony_floppy_select);
	if (!image_exists(cur_image))
		return;

	if (!f->loadedtrack_valid)
		load_track_data(sony_floppy_select);
	sony_filltrack(f->loadedtrack_data, f->loadedtrack_size, &f->loadedtrack_pos, data);
	f->loadedtrack_dirty = 1;
}



static int sony_rpm(floppy *f, mess_image *cur_image)
{
	int result = 0;

	/*
	 * The Mac floppy controller was interesting in that its speed was adjusted
	 * while the thing was running.  On the tracks closer to the rim, it was
	 * sped up so that more data could be placed on it.  Hence, this function
	 * has different results depending on the track number
	 *
	 * The Mac Plus (and probably the other Macs that use the IWM) verify that
	 * the speed of the floppy drive is within a certain range depending on
	 * what track the floppy is at.  These RPM values are just guesses and are
	 * probably not fully accurate, but they are within the range that the Mac
	 * Plus expects and thus are probably in the right ballpark.
	 *
	 * Note - the timing values are the values returned by the Mac Plus routine
	 * that calculates the speed; I'm not sure what units they are in
	 */

	if (f->ext_speed_control)
	{
		/* 400k unit : rotation speed controlled by computer */
		result = rotation_speed;
	}
	else
	{	/* 800k unit : rotation speed controlled by drive */
#if 1	/* Mac Plus */
		static int speeds[] =
		{
			500,	/* 00-15:	timing value 117B (acceptable range {1135-11E9} */
			550,	/* 16-31:	timing value ???? (acceptable range {12C6-138A} */
			600,	/* 32-47:	timing value ???? (acceptable range {14A7-157F} */
			675,	/* 48-63:	timing value ???? (acceptable range {16F2-17E2} */
			750		/* 64-79:	timing value ???? (acceptable range {19D0-1ADE} */
		};
#else	/* Lisa 2 */
		/* 237 + 1.3*(256-reg) */
		static int speeds[] =
		{
			293,	/* 00-15:	timing value ???? (acceptable range {0330-0336} */
			322,	/* 16-31:	timing value ???? (acceptable range {02ED-02F3} */
			351,	/* 32-47:	timing value ???? (acceptable range {02A7-02AD} */
			394,	/* 48-63:	timing value ???? (acceptable range {0262-0266} */
			439		/* 64-79:	timing value ???? (acceptable range {021E-0222} */
		};
#endif
		if (cur_image && image_exists(cur_image))
			result = speeds[floppy_drive_get_current_track(cur_image) / 16];
	}
	return result;
}

int sony_read_status(void)
{
	int result = 1;
	int action;
	floppy *f;
	mess_image *cur_image;

	action = ((sony_lines & (SONY_CA1 | SONY_CA0)) << 2) | (sony_sel_line << 1) | ((sony_lines & SONY_CA2) >> 2);

	if (LOG_SONY_EXTRA)
	{
		logerror("sony_status(): action=%d pc=0x%08x%s\n",
			action, (int) activecpu_get_pc(), sony_floppy_enable ? "" : " (no drive enabled)");
	}

	if ((! sony_enable2()) && sony_floppy_enable)
	{
		f = &sony_floppy[sony_floppy_select];
		cur_image = image_from_devtag_and_index("sonydriv", sony_floppy_select);
		if (!image_exists(cur_image))
			cur_image = NULL;

		switch(action) {
		case 0x00:	/* Step direction */
			result = f->step;
			break;
		case 0x01:	/* Lower head activate */
			if (f->head != 0)
			{
				save_track_data(sony_floppy_select);
				f->head = 0;
				f->loadedtrack_valid = 0;
			}
			result = 0;
			break;
		case 0x02:	/* Disk in place */
			result = cur_image ? 0 : 1;	/* 0=disk 1=nodisk */
			break;
		case 0x03:	/* Upper head activate */
			if (f->head != 1)
			{
				save_track_data(sony_floppy_select);
				f->head = 1;
				f->loadedtrack_valid = 0;
			}
			result = 0;
			break;
		case 0x04:	/* Disk is stepping 0=stepping 1=not stepping*/
			result = 1;
			break;
		case 0x06:	/* Disk is locked 0=locked 1=unlocked */
			if (cur_image)
				result = floppy_drive_get_flag_state(cur_image, FLOPPY_DRIVE_DISK_WRITE_PROTECTED) ? 0 : 1;
			else
				result = 0;
			break;
		case 0x08:	/* Motor on 0=on 1=off */
			if (cur_image)
				result = floppy_drive_get_flag_state(cur_image, FLOPPY_DRIVE_MOTOR_ON) ? 1 : 0;
			else
				result = 0;
			break;
		case 0x09:	/* Number of sides: 0=single sided, 1=double sided */
			if (cur_image)
				result = floppy_get_heads_per_disk(flopimg_get_image(cur_image)) - 1;
			break;
		case 0x0a:	/* At track 0: 0=track zero 1=not track zero */
			logerror("sony_status(): reading Track 0 pc=0x%08x\n", (int) activecpu_get_pc());
			if (cur_image)
				result = floppy_drive_get_flag_state(cur_image, FLOPPY_DRIVE_HEAD_AT_TRACK_0) ? 0 : 1;
			else
				result = 0;
			break;
		case 0x0b:	/* Disk ready: 0=ready, 1=not ready */
			result = 0;
			break;
		case 0x0c:	/* Disk switched */
			result = f->disk_switched;
			break;
		case 0x0d:	/* Unknown */
			/* I'm not sure what this one does, but the Mac Plus executes the
			 * following code that uses this status:
			 *
			 *	417E52: moveq   #$d, D0		; Status 0x0d
			 *	417E54: bsr     4185fe		; Query IWM status
			 *	417E58: bmi     417e82		; If result=1, then skip
			 *
			 * This code is called in the Sony driver's open method, and
			 * _AddDrive does not get called if this status 0x0d returns 1.
			 * Hence, we are returning 0
			 */
			result = 0;
			break;
		case 0x0e:	/* Tachometer */
			/* (time in seconds) / (60 sec/minute) * (rounds/minute) * (60 pulses) * (2 pulse phases) */
			result = ((int) (timer_get_time() / 60.0 * sony_rpm(f, cur_image) * 60.0 * 2.0)) & 1;
			break;
		case 0x0f:	/* Drive installed: 0=drive connected, 1=drive not connected */
			result = 0;
			break;
		default:
			if (LOG_SONY)
				logerror("sony_status(): unknown action\n");
			break;
		}
	}

	return result;
}



static void sony_doaction(void)
{
	int action;
	floppy *f;
	mess_image *cur_image;

	action = ((sony_lines & (SONY_CA1 | SONY_CA0)) << 2) | ((sony_lines & SONY_CA2) >> 2) | (sony_sel_line << 1);

	if (LOG_SONY)
	{
		logerror("sony_doaction(): action=%d pc=0x%08x%s\n",
			action, (int) activecpu_get_pc(), (sony_floppy_enable) ? "" : " (MOTOR OFF)");
	}

	if (sony_floppy_enable)
	{
		f = &sony_floppy[sony_floppy_select];
		cur_image = image_from_devtag_and_index("sonydriv", sony_floppy_select);
		if (!image_exists(cur_image))
			cur_image = NULL;

		switch(action)
		{
		case 0x00:	/* Set step inward (higher tracks) */
			f->step = 0;
			break;
		case 0x01:	/* Set step outward (lower tracks) */
			f->step = 1;
			break;
		case 0x03:	/* Reset diskswitched */
			f->disk_switched = 0;
			break;
		case 0x04:	/* Step disk */
			if (cur_image)
			{
				save_track_data(sony_floppy_select);
				if (f->step)
					floppy_drive_seek(cur_image, -1);
				else
					floppy_drive_seek(cur_image, +1);
				f->loadedtrack_valid = 0;
			}
			break;
		case 0x08:	/* Turn motor on */
			if (cur_image)
				floppy_drive_set_flag_state(cur_image, FLOPPY_DRIVE_MOTOR_ON, 1);
			break;
		case 0x09:	/* Turn motor off */
			if (cur_image)
				floppy_drive_set_flag_state(cur_image, FLOPPY_DRIVE_MOTOR_ON, 0);
			break;
		case 0x0d:	/* Eject disk */
			if (cur_image)
				image_unload(cur_image);
			break;
		default:
			if (LOG_SONY)
				logerror("sony_doaction(): unknown action %d\n", action);
			break;
		}
	}
}



void sony_set_lines(UINT8 lines)
{
	int old_sony_lines = sony_lines;

	sony_lines = lines & 0x0F;

	/* have we just set LSTRB ? */
	if ((sony_lines & ~old_sony_lines) & SONY_LSTRB)
	{
		/* if so, write drive reg */
		sony_doaction();
	}

	if (LOG_SONY_EXTRA)
		logerror("sony_set_lines(): %d\n", lines);
}



void sony_set_enable_lines(int enable_mask)
{
	switch (enable_mask)
	{
	case 0:
	default:	/* well, we have to do something, right ? */
		sony_floppy_enable = 0;
		break;
	case 1:
		sony_floppy_enable = 1;
		sony_floppy_select = 0;
		break;
	case 2:
		sony_floppy_enable = 1;
		sony_floppy_select = 1;
		break;
	}

	if (LOG_SONY_EXTRA)
		logerror("sony_set_enable_lines(): %d\n", enable_mask);
}



void sony_set_sel_line(int sel)
{
	sony_sel_line = sel ? 1 : 0;

	if (LOG_SONY_EXTRA)
		logerror("sony_set_sel_line(): %s line IWM_SEL\n", sony_sel_line ? "setting" : "clearing");
}



void sony_set_speed(int speed)
{
	rotation_speed = speed;
}



static void device_unload_sonydriv_floppy(mess_image *image)
{
	int id;
	device_unload_handler parent_unload;
	
	id = image_index_in_device(image);
	save_track_data(id);
	memset(&sony_floppy[id], 0, sizeof(sony_floppy[id]));

	parent_unload = (device_unload_handler) device_get_info_fct(&parent_devclass, DEVINFO_PTR_UNLOAD);
	parent_unload(image);
}



void sonydriv_device_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:				info->i = 2; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_DEV_TAG:			strcpy(info->s = device_temp_str(), "sonydriv"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_UNLOAD:			info->unload = device_unload_sonydriv_floppy; break;
		case DEVINFO_PTR_FLOPPY_OPTIONS:
			if (device_get_info_int(devclass, DEVINFO_INT_SONYDRIV_ALLOWABLE_SIZES) & SONY_FLOPPY_SUPPORT2IMG)
				info->p = (void *) floppyoptions_apple35_iigs;
			else
				info->p = (void *) floppyoptions_apple35_mac;
			break;

		default: floppy_device_getinfo(devclass, state, info); break;
	}
}

