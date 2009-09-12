/* flopdrv provides simple emulation of a disc drive */
/* the 8271, nec765 and wd179x use this */

#ifndef FLOPDRV_H
#define FLOPDRV_H

#include "device.h"
#include "image.h"
#include "formats/flopimg.h"


typedef enum
{
	DEN_FM_LO = 0,
	DEN_FM_HI,
	DEN_MFM_LO,
	DEN_MFM_HI
} DENSITY;

/* sector has a deleted data address mark */
#define ID_FLAG_DELETED_DATA	0x0001
/* CRC error in id field */
#define ID_FLAG_CRC_ERROR_IN_ID_FIELD 0x0002
/* CRC error in data field */
#define ID_FLAG_CRC_ERROR_IN_DATA_FIELD 0x0004

int	floppy_status(const device_config *img, int new_status);

typedef struct chrn_id
{
	unsigned char C;
	unsigned char H;
	unsigned char R;
	unsigned char N;
	int data_id;			// id for read/write data command
	unsigned long flags;
} chrn_id;

/* set if disc is write protected - also set if drive is present but no disc in drive */
#define FLOPPY_DRIVE_DISK_WRITE_PROTECTED		0x0002
/* set if drive is connected and head is positioned over track 0 */
#define FLOPPY_DRIVE_HEAD_AT_TRACK_0			0x0004
/* set if drive is ready */
#define FLOPPY_DRIVE_READY						0x0010
/* set if index has just occured */
#define FLOPPY_DRIVE_INDEX						0x0020
/* motor state */
#define FLOPPY_DRIVE_MOTOR_ON					0x0040


typedef struct floppy_interface
{
	/* seek to physical track */
	void (*seek_callback)(const device_config *img, int physical_track);

	/* the following are not strictly floppy drive operations, but are used by the
	nec765 to get data from the track - really the whole track should be constructed
	into the raw format the nec765 would normally see and this would be totally accurate */
	/* the disc image would then have to re-interpret this back and update the image
	with the data */

	/* get number of sectors per track on side specified */
	int (*get_sectors_per_track)(const device_config *image, int physical_side);

	/* get id from current track and specified side */
	void (*get_id_callback)(const device_config *image, chrn_id *, int id_index, int physical_side);

	/* read sector data into buffer, length = number of bytes to read */
	void (*read_sector_data_into_buffer)(const device_config *image, int side,int data_id,char *, int length);

	/* write sector data from buffer, length = number of bytes to read  */
	void (*write_sector_data_from_buffer)(const device_config *image, int side,int data_id, const char *, int length, int ddam);

	/* Read track in buffer, length = number of bytes to read */
	void (*read_track_data_info_buffer)(const device_config *image, int side, void *ptr, int *length );

	/* Write track in buffer, length = number of bytes to read */
	void (*write_track_data_info_buffer)(const device_config *image, int side, const void *ptr, int *length );

	/* format */
	void (*format_sector)(const device_config *img, int side, int sector_index,int c, int h, int r, int n, int filler);
} floppy_interface;

struct floppy_drive
{
	/* flags */
	int flags;
	/* maximum track allowed */
	int max_track;
	/* num sides */
	int num_sides;
	/* current track - this may or may not relate to the present cylinder number
	stored by the fdc */
	int current_track;

	/* index pulse timer */
	void	*index_timer;
	/* index pulse callback */
	void	(*index_pulse_callback)(const device_config *controller,const device_config *image, int state);
	/* rotation per minute => gives index pulse frequency */
	float rpm;
	/* current index pulse value */
	int index;

	void	(*ready_state_change_callback)(const device_config *controller,const device_config *img, int state);

	unsigned char id_buffer[4];

	int id_index;
    chrn_id ids[32];

	const device_config *controller;
	
	floppy_interface interface_;
};

/* a callback which will be executed if the ready state of the drive changes e.g. not ready->ready, ready->not ready */
void floppy_drive_set_ready_state_change_callback(const device_config *img, void (*callback)(const device_config *controller,const device_config *img, int state));

/* floppy drive types */
typedef enum
{
	FLOPPY_DRIVE_SS_40,
	FLOPPY_DRIVE_DS_80
} floppy_type;

void floppy_drive_set_index_pulse_callback(const device_config *img, void (*callback)(const device_config *controller,const device_config *image, int state));

/* set flag state */
int floppy_drive_get_flag_state(const device_config *img, int flag);
/* get flag state */
void floppy_drive_set_flag_state(const device_config *img, int flag, int state);
/* get current physical track drive is on */
int floppy_drive_get_current_track(const device_config *img);

void floppy_drive_set_geometry(const device_config *img, floppy_type type);
void floppy_drive_set_geometry_absolute(const device_config *img, int tracks, int sides);

/* called in device init/exit functions */
void floppy_drive_init(const device_config *img, const floppy_interface *iface);

/* get next id from track, 1 if got a id, 0 if no id was got */
int floppy_drive_get_next_id(const device_config *img, int side, chrn_id *);
/* set ready state of drive. If flag == 1, set ready state only if drive present,
disk is in drive, and motor is on. Otherwise set ready state to the state passed */
void floppy_drive_set_ready_state(const device_config *img, int state, int flag);

void floppy_drive_set_motor_state(const device_config *img, int state);

/* deprecated; set interface for disk image functions */
void floppy_drive_set_disk_image_interface(const device_config *img, const floppy_interface *iface);

/* seek up or down */
void floppy_drive_seek(const device_config *img, signed int signed_tracks);

void floppy_drive_read_track_data_info_buffer(const device_config *img, int side, void *ptr, int *length );
void floppy_drive_write_track_data_info_buffer(const device_config *img, int side, const void *ptr, int *length );
void floppy_drive_format_sector(const device_config *img, int side, int sector_index, int c, int h, int r, int n, int filler);
void floppy_drive_read_sector_data(const device_config *img, int side, int index1, void *pBuffer, int length);
void floppy_drive_write_sector_data(const device_config *img, int side, int index1, const void *pBuffer, int length, int ddam);
int	floppy_drive_get_datarate_in_us(DENSITY density);

/* set motor speed to get correct index pulses
   standard RPM are 300 RPM (common) and 360 RPM
   Note: this actually only works for soft sectored disks: one index pulse per
   track.
*/
void floppy_drive_set_rpm(const device_config *image, float rpm);

void floppy_drive_set_controller(const device_config *img, const device_config *controller);

enum
{
	MESS_DEVINFO_PTR_FLOPPY_OPTIONS = MESS_DEVINFO_PTR_DEV_SPECIFIC,
	MESS_DEVINFO_INT_KEEP_DRIVE_GEOMETRY = MESS_DEVINFO_INT_DEV_SPECIFIC
};

floppy_image *flopimg_get_image(const device_config *image);

void floppy_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);

/* hack for apple II; replace this when we think of something better */
void floppy_install_unload_proc(const device_config *image, void (*proc)(const device_config *image));

void floppy_install_load_proc(const device_config *image, void (*proc)(const device_config *image));

/* hack for TI99; replace this when we think of something better */
void floppy_install_tracktranslate_proc(const device_config *image, int (*proc)(const device_config *image, floppy_image *floppy, int physical_track));


#endif /* FLOPDRV_H */
