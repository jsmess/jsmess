/*********************************************************************

	appldriv.c
	
	Apple 5.25" floppy drive emulation (to be interfaced with applefdc.c)

*********************************************************************/

#include "appldriv.h"
#include "devices/flopdrv.h"
#include "devices/mflopimg.h"
#include "formats/ap2_dsk.h"

#define APPLE525TAG	"apple525driv"

struct apple525_disk
{
	unsigned int state : 4;		/* bits 0-3 are the phase */
	unsigned int tween_tracks : 1;
	unsigned int track_loaded : 1;
	unsigned int track_dirty : 1;
	int position;
	int spin_count; 		/* simulate drive spin to fool RWTS test at $BD34 */
	UINT8 track_data[APPLE2_NIBBLE_SIZE * APPLE2_SECTOR_COUNT];
};

static const device_class parent_devclass = { floppy_device_getinfo, NULL };
static int apple525_enable_mask = 1;



void apple525_set_enable_lines(int enable_mask)
{
	apple525_enable_mask = enable_mask;
}



/* ----------------------------------------------------------------------- */

static void apple525_load_current_track(mess_image *image)
{
	int len;
	struct apple525_disk *disk;

	disk = (struct apple525_disk *) image_lookuptag(image, APPLE525TAG);
	len = sizeof(disk->track_data);

	floppy_drive_read_track_data_info_buffer(image, 0, disk->track_data, &len);
	disk->track_loaded = 1;
	disk->track_dirty = 0;
}



static void apple525_save_current_track(mess_image *image, int unload)
{
	int len;
	struct apple525_disk *disk;

	disk = (struct apple525_disk *) image_lookuptag(image, APPLE525TAG);

	if (disk->track_dirty)
	{
		len = sizeof(disk->track_data);
		floppy_drive_write_track_data_info_buffer(image, 0, disk->track_data, &len);
		disk->track_dirty = 0;
	}
	if (unload)
		disk->track_loaded = 0;
}



static void apple525_seek_disk(mess_image *img, struct apple525_disk *disk, signed int step)
{
	int track;
	int pseudo_track;

	apple525_save_current_track(img, FALSE);
	
	track = floppy_drive_get_current_track(img);
	pseudo_track = (track * 2) + disk->tween_tracks;
	
	pseudo_track += step;
	if (pseudo_track < 0)
		pseudo_track = 0;
	else if (pseudo_track/2 >= APPLE2_TRACK_COUNT)
		pseudo_track = APPLE2_TRACK_COUNT*2-1;

	if (pseudo_track/2 != track)
	{
		floppy_drive_seek(img, pseudo_track/2 - floppy_drive_get_current_track(img));
		disk->track_loaded = 0;
	}

	if (pseudo_track & 1)
		disk->tween_tracks = 1;
	else
		disk->tween_tracks = 0;
}



static void apple525_disk_set_lines(mess_image *image, UINT8 new_state)
{
	struct apple525_disk *cur_disk;
	UINT8 old_state;
	unsigned int phase;

	cur_disk = (struct apple525_disk *) image_lookuptag(image, APPLE525TAG);

	old_state = cur_disk->state;
	cur_disk->state = new_state;

	if ((new_state & 0x0F) > (old_state & 0x0F))
	{
		phase = 0;
		switch((old_state ^ new_state) & 0x0F)
		{
			case 1:	phase = 0; break;
			case 2:	phase = 1; break;
			case 4:	phase = 2; break;
			case 8:	phase = 3; break;
		}

		phase -= floppy_drive_get_current_track(image) * 2;
		if (cur_disk->tween_tracks)
			phase--;
		phase %= 4;

		switch(phase)
		{
			case 1:
				apple525_seek_disk(image, cur_disk, +1);
				break;
			case 3:
				apple525_seek_disk(image, cur_disk, -1);
				break;
		}
	}
}



void apple525_set_lines(UINT8 lines)
{
	int i, count;
	mess_image *image;

	count = device_count_tag(Machine->devices, APPLE525TAG);

	for (i = 0; i < count; i++)
	{
		if (apple525_enable_mask & (1 << i))
		{
			image = image_from_devtag_and_index(APPLE525TAG, i);
			if (image)
				apple525_disk_set_lines(image, lines);
		}
	}
}



/* reads/writes a byte; write_value is -1 for read only */
static UINT8 apple525_process_byte(mess_image *img, int write_value)
{
	UINT8 read_value;
	struct apple525_disk *disk;
	int spinfract_divisor;
	int spinfract_dividend;
	const struct IODevice *dev;

	disk = (struct apple525_disk *) image_lookuptag(img, APPLE525TAG);
	dev = image_device(img);
	spinfract_dividend = (int) device_get_info_int(&dev->devclass, DEVINFO_INT_APPLE525_SPINFRACT_DIVIDEND);
	spinfract_divisor = (int) device_get_info_int(&dev->devclass, DEVINFO_INT_APPLE525_SPINFRACT_DIVISOR);

	/* no image initialized for that drive ? */
	if (!image_exists(img))
		return 0xFF;

	/* check the spin count if reading*/
	if (write_value < 0)
	{
		disk->spin_count++;
		disk->spin_count %= spinfract_divisor;
		if (disk->spin_count >= spinfract_dividend)
			return 0x00;
	}

	/* load track if need be */
	if (disk->track_loaded == 0)
		apple525_load_current_track(img);

	/* perform the read */
	read_value = disk->track_data[disk->position];

	/* perform the write, if applicable */
	if (write_value >= 0)
	{
		disk->track_data[disk->position] = write_value;
		disk->track_dirty = 1;
	}

	disk->position++;
	disk->position %= (sizeof(disk->track_data) / sizeof(disk->track_data[0]));

	/* when writing; save the current track after every full sector write */
	if ((write_value >= 0) && ((disk->position % APPLE2_NIBBLE_SIZE) == 0))
		apple525_save_current_track(img, FALSE);

	return read_value;
}



static mess_image *apple525_selected_image(void)
{
	int i, count;

	count = device_count_tag(Machine->devices, APPLE525TAG);

	for (i = 0; i < count; i++)
	{
		if (apple525_enable_mask & (1 << i))
			return image_from_devtag_and_index(APPLE525TAG, i);
	}
	return NULL;
}



UINT8 apple525_read_data(void)
{
	mess_image *image;
	image = apple525_selected_image();
	return image ? apple525_process_byte(image, -1) : 0xFF;
}



void apple525_write_data(UINT8 data)
{
	mess_image *image;
	image = apple525_selected_image();
	if (image)
		apple525_process_byte(image, data);
}



int apple525_read_status(void)
{
	int i, count, result = 0;
	mess_image *image;

	count = device_count_tag(Machine->devices, APPLE525TAG);

	for (i = 0; i < count; i++)
	{
		if (apple525_enable_mask & (1 << i))
		{
			image = image_from_devtag_and_index(APPLE525TAG, i);
			if (image && !image_is_writable(image))
				result = 1;
		}
	}
	return result;
}



/* ----------------------------------------------------------------------- */

static int device_init_apple525_floppy(mess_image *image)
{
	device_init_handler parent_init;

	if (!image_alloctag(image, APPLE525TAG, sizeof(struct apple525_disk)))
		return INIT_FAIL;

	parent_init = (device_init_handler) device_get_info_fct(&parent_devclass, DEVINFO_PTR_INIT);
	return parent_init(image);
}



static int device_load_apple525_floppy(mess_image *image)
{
	int result;
	device_load_handler parent_load;

	parent_load = (device_load_handler) device_get_info_fct(&parent_devclass, DEVINFO_PTR_LOAD);
	result = parent_load(image);

	floppy_drive_seek(image, -999);
	floppy_drive_seek(image, +35/2);
	return result;
}



static void device_unload_apple525_floppy(mess_image *image)
{
	device_unload_handler parent_unload;

	apple525_save_current_track(image, TRUE);

	parent_unload = (device_unload_handler) device_get_info_fct(&parent_devclass, DEVINFO_PTR_UNLOAD);
	parent_unload(image);
}



void apple525_device_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:				info->i = 2; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_DEV_TAG:			strcpy(info->s = device_temp_str(), APPLE525TAG); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_INIT:				info->init = device_init_apple525_floppy; break;
		case DEVINFO_PTR_LOAD:				info->load = device_load_apple525_floppy; break;
		case DEVINFO_PTR_UNLOAD:			info->unload = device_unload_apple525_floppy; break;
		case DEVINFO_PTR_FLOPPY_OPTIONS:	info->p = (void *) floppyoptions_apple2; break;

		default: floppy_device_getinfo(devclass, state, info); break;
	}
}
