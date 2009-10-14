/* flopdrv provides simple emulation of a disc drive */
/* the 8271, nec765 and wd179x use this */

#ifndef __FLOPDRV_H__
#define __FLOPDRV_H__

#include "devcb.h"
#include "device.h"
#include "image.h"
#include "formats/flopimg.h"

#define FLOPPY_TYPE_REGULAR 0
#define FLOPPY_TYPE_APPLE	1
#define FLOPPY_TYPE_SONY	2

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/
/* floppy drive types */
typedef enum
{
	FLOPPY_DRIVE_SS_40,
	FLOPPY_DRIVE_DS_80
} floppy_type;

typedef enum
{
	DO_NOT_KEEP_GEOMETRY=0,
	KEEP_GEOMETRY
} keep_geometry;

typedef struct floppy_config_t	floppy_config;
struct floppy_config_t
{
	devcb_write_line out_idx_func;  /* index */
	devcb_read_line  in_mon_func;   /* motor on */
	devcb_write_line out_tk00_func; /* track 00 */
	devcb_write_line out_wpt_func;  /* write protect */
	devcb_write_line out_rdy_func;  /* ready */

	floppy_type floppy_type;
	const struct FloppyFormat *formats;
	keep_geometry keep_drive_geometry;
};

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

/* a callback which will be executed if the ready state of the drive changes e.g. not ready->ready, ready->not ready */
void floppy_drive_set_ready_state_change_callback(const device_config *img, void (*callback)(const device_config *controller,const device_config *img, int state));

void floppy_drive_set_index_pulse_callback(const device_config *img, void (*callback)(const device_config *controller,const device_config *image, int state));

/* set flag state */
int floppy_drive_get_flag_state(const device_config *img, int flag);
/* get flag state */
void floppy_drive_set_flag_state(const device_config *img, int flag, int state);
/* get current physical track drive is on */
int floppy_drive_get_current_track(const device_config *img);

/* get next id from track, 1 if got a id, 0 if no id was got */
int floppy_drive_get_next_id(const device_config *img, int side, chrn_id *);
/* set ready state of drive. If flag == 1, set ready state only if drive present,
disk is in drive, and motor is on. Otherwise set ready state to the state passed */
void floppy_drive_set_ready_state(const device_config *img, int state, int flag);

void floppy_drive_set_motor_state(const device_config *img, int state);

/* seek up or down */
void floppy_drive_seek(const device_config *img, signed int signed_tracks);

void floppy_drive_read_track_data_info_buffer(const device_config *img, int side, void *ptr, int *length );
void floppy_drive_write_track_data_info_buffer(const device_config *img, int side, const void *ptr, int *length );
void floppy_drive_format_sector(const device_config *img, int side, int sector_index, int c, int h, int r, int n, int filler);
void floppy_drive_read_sector_data(const device_config *img, int side, int index1, void *pBuffer, int length);
void floppy_drive_write_sector_data(const device_config *img, int side, int index1, const void *pBuffer, int length, int ddam);

/* set motor speed to get correct index pulses
   standard RPM are 300 RPM (common) and 360 RPM
   Note: this actually only works for soft sectored disks: one index pulse per
   track.
*/
void floppy_drive_set_rpm(const device_config *image, float rpm);

void floppy_drive_set_controller(const device_config *img, const device_config *controller);

floppy_image *flopimg_get_image(const device_config *image);

/* hack for apple II; replace this when we think of something better */
void floppy_install_unload_proc(const device_config *image, void (*proc)(const device_config *image));

void floppy_install_load_proc(const device_config *image, void (*proc)(const device_config *image));

/* hack for TI99; replace this when we think of something better */
void floppy_install_tracktranslate_proc(const device_config *image, int (*proc)(const device_config *image, floppy_image *floppy, int physical_track));

const device_config *floppy_get_device(running_machine *machine,int drive);
const device_config *floppy_get_device_by_type(running_machine *machine,int ftype,int drive);
int floppy_get_drive_type(const device_config *image);
void floppy_set_type(const device_config *image,int ftype);
int floppy_get_count(running_machine *machine);

int floppy_get_drive(const device_config *image);
int floppy_get_drive_by_type(const device_config *image,int ftype);

void *flopimg_get_custom_data(const device_config *image);
void flopimg_alloc_custom_data(const device_config *image,void *custom);

void floppy_drive_set_geometry(const device_config *img, floppy_type type);

/* drive select lines */
WRITE_LINE_DEVICE_HANDLER( floppy_ds0_w );
WRITE_LINE_DEVICE_HANDLER( floppy_ds1_w );
WRITE_LINE_DEVICE_HANDLER( floppy_ds2_w );
WRITE_LINE_DEVICE_HANDLER( floppy_ds3_w );
WRITE8_DEVICE_HANDLER( floppy_ds_w );

READ_LINE_DEVICE_HANDLER( floppy_idx_r );
WRITE_LINE_DEVICE_HANDLER( floppy_mon_w );
WRITE_LINE_DEVICE_HANDLER( floppy_drtn_w );
WRITE_LINE_DEVICE_HANDLER( floppy_stp_w );
WRITE_LINE_DEVICE_HANDLER( floppy_wtd_w );
WRITE_LINE_DEVICE_HANDLER( floppy_wtg_w );
READ_LINE_DEVICE_HANDLER( floppy_tk00_r );
READ_LINE_DEVICE_HANDLER( floppy_wpt_r );
READ_LINE_DEVICE_HANDLER( floppy_rdy_r );

#define FLOPPY	DEVICE_GET_INFO_NAME(floppy)
DEVICE_GET_INFO(floppy);

extern DEVICE_START( floppy );
extern DEVICE_IMAGE_LOAD( floppy );
extern DEVICE_IMAGE_CREATE( floppy );
extern DEVICE_IMAGE_UNLOAD( floppy );

/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/
#define FLOPPY_0 "floppy0"
#define FLOPPY_1 "floppy1"
#define FLOPPY_2 "floppy2"
#define FLOPPY_3 "floppy3"


#define MDRV_FLOPPY_DRIVE_ADD(_tag, _config) 	\
	MDRV_DEVICE_ADD(_tag, FLOPPY, 0)			\
	MDRV_DEVICE_CONFIG(_config)

#define MDRV_FLOPPY_DRIVE_MODIFY(_tag, _config)	\
	MDRV_DEVICE_MODIFY(_tag)		\
	MDRV_DEVICE_CONFIG(_config)

#define MDRV_FLOPPY_4_DRIVES_ADD(_config) 	\
	MDRV_DEVICE_ADD(FLOPPY_0, FLOPPY, 0)		\
	MDRV_DEVICE_CONFIG(_config)	\
	MDRV_DEVICE_ADD(FLOPPY_1, FLOPPY, 0)		\
	MDRV_DEVICE_CONFIG(_config)	\
	MDRV_DEVICE_ADD(FLOPPY_2, FLOPPY, 0)		\
	MDRV_DEVICE_CONFIG(_config)	\
	MDRV_DEVICE_ADD(FLOPPY_3, FLOPPY, 0)		\
	MDRV_DEVICE_CONFIG(_config)

#define MDRV_FLOPPY_4_DRIVES_MODIFY(_config) 	\
	MDRV_DEVICE_MODIFY(FLOPPY_0)		\
	MDRV_DEVICE_CONFIG(_config)	\
	MDRV_DEVICE_MODIFY(FLOPPY_1)		\
	MDRV_DEVICE_CONFIG(_config)	\
	MDRV_DEVICE_MODIFY(FLOPPY_2)		\
	MDRV_DEVICE_CONFIG(_config)	\
	MDRV_DEVICE_MODIFY(FLOPPY_3)		\
	MDRV_DEVICE_CONFIG(_config)

#define MDRV_FLOPPY_4_DRIVES_REMOVE() 	\
	MDRV_DEVICE_REMOVE(FLOPPY_0)		\
	MDRV_DEVICE_REMOVE(FLOPPY_1)		\
	MDRV_DEVICE_REMOVE(FLOPPY_2)		\
	MDRV_DEVICE_REMOVE(FLOPPY_3)

#define MDRV_FLOPPY_2_DRIVES_ADD(_config) 	\
	MDRV_DEVICE_ADD(FLOPPY_0, FLOPPY, 0)		\
	MDRV_DEVICE_CONFIG(_config)	\
	MDRV_DEVICE_ADD(FLOPPY_1, FLOPPY, 0)		\
	MDRV_DEVICE_CONFIG(_config)

#define MDRV_FLOPPY_2_DRIVES_MODIFY(_config) 	\
	MDRV_DEVICE_MODIFY(FLOPPY_0)		\
	MDRV_DEVICE_CONFIG(_config)	\
	MDRV_DEVICE_MODIFY(FLOPPY_1)		\
	MDRV_DEVICE_CONFIG(_config)

#define MDRV_FLOPPY_2_DRIVES_REMOVE() 	\
	MDRV_DEVICE_REMOVE(FLOPPY_0)		\
	MDRV_DEVICE_REMOVE(FLOPPY_1)

#endif /* __FLOPDRV_H__ */
