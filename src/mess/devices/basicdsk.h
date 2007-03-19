/* DISK IMAGE FORMAT WHICH USED TO BE PART OF WD179X - NOW SEPERATED */

#ifndef BASICDSK_H
#define BASICDSK_H

#include "driver.h"
#include "devices/flopdrv.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const floppy_interface basicdsk_floppy_interface;

/* init */
DEVICE_INIT(basicdsk_floppy);
DEVICE_LOAD(basicdsk_floppy);
DEVICE_UNLOAD(basicdsk_floppy);

/* set the disk image geometry for the specified drive */
/* this is required to read the disc image correct */

/* geometry details:

  If a disk image is specified with 2 sides, they are stored as follows:

  track 0 side 0, track 0 side 1, track 1 side 0.....

  If a disk image is specified with 1 sides, they are stored as follows:

  track 0 side 0, track 1 side 0, ....

  tracks = the number of tracks stored in the image
  sec_per_track = the number of sectors on each track
  sector_length = size of sector (must be power of 2) e.g. 128,256,512 bytes
  first_sector_id = this is the sector id base. If there are 10 sectors, with the first_sector_id is 1,
  the sector id's will be:

	1,2,3,4,5,6,7,8,9,10

  track_skipping = set to TRUE if a 40-track image has been inserted in an
  80-track drive, FALSE otherwise
*/

void basicdsk_set_geometry(mess_image *img, UINT8 tracks, UINT8 sides, UINT8 sec_per_track, UINT16 sector_length/*, UINT16 dir_sector, UINT16 dir_length*/, UINT8 first_sector_id, UINT16 offset_track_zero, int track_skipping);

void basicdsk_set_calcoffset(mess_image *img, unsigned long (*calcoffset)(UINT8 t, UINT8 h, UINT8 s,
	UINT8 tracks, UINT8 heads, UINT8 sec_per_track, UINT16 sector_length, UINT8 first_sector_id, UINT16 offset_track_zero));

/* set data mark/deleted data mark for the sector specified. If ddam!=0, the sector will
have a deleted data mark, if ddam==0, the sector will have a data mark */
void basicdsk_set_ddam(mess_image *img, UINT8 physical_track, UINT8 physical_side, UINT8 sector_id,UINT8 ddam);

void legacybasicdsk_device_getinfo(const device_class *devclass, UINT32 state, union devinfo *info);

#define CONFIG_DEVICE_FLOPPY_BASICDSK(count, file_extensions, load)		\
	CONFIG_DEVICE_BASE(IO_FLOPPY, (count), (file_extensions), DEVICE_LOAD_RESETS_NONE,	\
		OSD_FOPEN_RW_CREATE_OR_READ, device_init_basicdsk_floppy, NULL, (load), device_unload_basicdsk_floppy, NULL, NULL,\
		NULL, NULL, floppy_status, NULL, NULL, NULL, NULL, NULL, NULL)	\

#define CONFIG_DEVICE_FLOPPY_BASICDSK_RO(count, file_extensions, load)		\
	CONFIG_DEVICE_BASE(IO_FLOPPY, (count), (file_extensions), DEVICE_LOAD_RESETS_NONE,	\
		OSD_FOPEN_READ, device_init_basicdsk_floppy, NULL, (load), device_unload_basicdsk_floppy, NULL, NULL,\
		NULL, NULL, floppy_status, NULL, NULL, NULL, NULL, NULL, NULL)	\

#ifdef __cplusplus
}
#endif

#endif /* BASICDSK_H */
