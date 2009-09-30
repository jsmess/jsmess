/*********************************************************************

    appldriv.c

    Apple 5.25" floppy drive emulation (to be interfaced with applefdc.c)

*********************************************************************/
#include "driver.h"
#include "appldriv.h"
#include "flopdrv.h"
#include "formats/ap2_dsk.h"

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

INLINE const appledriv_config *get_config(const device_config *device)
{
	assert(device != NULL);
	return (const appledriv_config *) device->inline_config;
}

static int apple525_enable_mask = 1;

void apple525_set_enable_lines(const device_config *device,int enable_mask)
{
	apple525_enable_mask = enable_mask;
}

/* ----------------------------------------------------------------------- */

static void apple525_load_current_track(const device_config *image)
{
	int len;
	struct apple525_disk *disk;

	disk = (struct apple525_disk *) flopimg_get_custom_data(image);
	len = sizeof(disk->track_data);

	floppy_drive_read_track_data_info_buffer(image, 0, disk->track_data, &len);
	disk->track_loaded = 1;
	disk->track_dirty = 0;
}

static void apple525_save_current_track(const device_config *image, int unload)
{
	int len;
	struct apple525_disk *disk;

	disk = (struct apple525_disk *)  flopimg_get_custom_data(image);

	if (disk->track_dirty)
	{
		len = sizeof(disk->track_data);
		floppy_drive_write_track_data_info_buffer(image, 0, disk->track_data, &len);
		disk->track_dirty = 0;
	}
	if (unload)
		disk->track_loaded = 0;
}

static void apple525_seek_disk(const device_config *img, struct apple525_disk *disk, signed int step)
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

static void apple525_disk_set_lines(const device_config *device,const device_config *image, UINT8 new_state)
{
	struct apple525_disk *cur_disk;
	UINT8 old_state;
	unsigned int phase;

	cur_disk = (struct apple525_disk *)  flopimg_get_custom_data(image);

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

int apple525_get_count(running_machine *machine) {
	int cnt = 0;
	if (devtag_get_device(machine,FLOPPY_0)!=NULL && flopimg_get_custom_data(devtag_get_device(machine,FLOPPY_0))!=NULL) cnt++;
    if (devtag_get_device(machine,FLOPPY_1)!=NULL && flopimg_get_custom_data(devtag_get_device(machine,FLOPPY_1))!=NULL) cnt++;
    if (devtag_get_device(machine,FLOPPY_2)!=NULL && flopimg_get_custom_data(devtag_get_device(machine,FLOPPY_2))!=NULL) cnt++;
    if (devtag_get_device(machine,FLOPPY_3)!=NULL && flopimg_get_custom_data(devtag_get_device(machine,FLOPPY_3))!=NULL) cnt++;
	return cnt;
}

void apple525_set_lines(const device_config *device,UINT8 lines)
{
	int i, count;
	const device_config *image;

	count = apple525_get_count(device->machine);
	for (i = 0; i < count; i++)
	{
		if (apple525_enable_mask & (1 << i))
		{
			image = floppy_get_device_by_type(device->machine, FLOPPY_TYPE_APPLE, i);
			if (image)
				apple525_disk_set_lines(device,image, lines);
		}
	}
}

/* reads/writes a byte; write_value is -1 for read only */
static UINT8 apple525_process_byte(const device_config *img, int write_value)
{
	UINT8 read_value;
	struct apple525_disk *disk;
	int spinfract_divisor;
	int spinfract_dividend;
	const appledriv_config *config = get_config(img);

	disk = (struct apple525_disk *)  flopimg_get_custom_data(img);
	spinfract_dividend = config->dividend;
	spinfract_divisor = config->divisor;

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

static const device_config *apple525_selected_image(running_machine *machine)
{
	int i,count;

	count = apple525_get_count(machine);

	for (i = 0; i < count; i++)
	{
		if (apple525_enable_mask & (1 << i))
			return floppy_get_device_by_type(machine, FLOPPY_TYPE_APPLE, i);
	}
	return NULL;
}

UINT8 apple525_read_data(const device_config *device)
{
	const device_config *image;
	image = apple525_selected_image(device->machine);
	return image ? apple525_process_byte(image, -1) : 0xFF;
}

void apple525_write_data(const device_config *device,UINT8 data)
{
	const device_config *image;
	image = apple525_selected_image(device->machine);
	if (image)
		apple525_process_byte(image, data);
}

int apple525_read_status(const device_config *device)
{
	int i, count, result = 0;
	const device_config *image;

	count = apple525_get_count(device->machine);

	for (i = 0; i < count; i++)
	{
		if (apple525_enable_mask & (1 << i))
		{
			image = floppy_get_device_by_type(device->machine, FLOPPY_TYPE_APPLE, i);
			if (image && !image_is_writable(image))
				result = 1;
		}
	}
	return result;
}

/* ----------------------------------------------------------------------- */

static DEVICE_START( apple525_floppy )
{

	DEVICE_START_CALL(floppy);
	flopimg_alloc_custom_data(device,auto_alloc_clear(device->machine,struct apple525_disk));
	floppy_set_type(device,FLOPPY_TYPE_APPLE);
}

static DEVICE_IMAGE_LOAD( apple525_floppy )
{
	int result = DEVICE_IMAGE_LOAD_NAME(floppy)(image);
	floppy_drive_seek(image, -999);
	floppy_drive_seek(image, +35/2);
	return result;
}

static DEVICE_IMAGE_UNLOAD( apple525_floppy )
{
	apple525_save_current_track(image, TRUE);

	DEVICE_IMAGE_UNLOAD_NAME(floppy)(image);
}

DEVICE_GET_INFO( apple525 )
{
	switch (state)
	{
		case DEVINFO_STR_NAME:						strcpy(info->s, "Floppy Disk [Apple]"); break;
		case DEVINFO_FCT_START:						info->start = DEVICE_START_NAME(apple525_floppy); break;
		case DEVINFO_FCT_IMAGE_LOAD:				info->f = (genf *) DEVICE_IMAGE_LOAD_NAME(apple525_floppy); break;
		case DEVINFO_FCT_IMAGE_UNLOAD:				info->f = (genf *) DEVICE_IMAGE_UNLOAD_NAME(apple525_floppy); break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:		info->i = sizeof(appledriv_config); break;

		default: 									DEVICE_GET_INFO_CALL(floppy);				break;
	}
}
