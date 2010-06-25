#ifndef __TI99_HD__
#define __TI99_HD__

#include "emu.h"
#include "machine/smc92x4.h"

#define MFMHD_0 "mfmhd0"
#define MFMHD_1 "mfmhd1"
#define MFMHD_2 "mfmhd2"

#define IDEHD_0 "idehd0"

/* typedef struct _mfmhd_config mfmhd_config;
struct _mfmhd_config
{
    device_start_func           device_start;
    device_image_load_func          device_load;
    device_image_unload_func        device_unload;
}; */

/* Accessor functions */
void ti99_mfm_harddisk_read_sector(running_device *harddisk, int cylinder, int head, int sector, UINT8 **buf, int *sector_length);
void ti99_mfm_harddisk_write_sector(running_device *harddisk, int cylinder, int head, int sector, UINT8 *buf, int sector_length);
void ti99_mfm_harddisk_read_track(running_device *harddisk, int head, UINT8 **buffer, int *data_count);
void ti99_mfm_harddisk_write_track(running_device *harddisk, int head, UINT8 *buffer, int data_count);
UINT8 ti99_mfm_harddisk_status(running_device *harddisk);
void ti99_mfm_harddisk_seek(running_device *harddisk, int direction);
void ti99_mfm_harddisk_get_next_id(running_device *harddisk, int head, chrn_id_hd *id);

DECLARE_LEGACY_DEVICE(MFMHD, mfmhd);

DECLARE_LEGACY_DEVICE(IDEHD, idehd);

#define MDRV_MFMHD_3_DRIVES_ADD()			\
	MDRV_DEVICE_ADD(MFMHD_0, MFMHD, 0)		\
	MDRV_DEVICE_ADD(MFMHD_1, MFMHD, 0)		\
	MDRV_DEVICE_ADD(MFMHD_2, MFMHD, 0)		\
	MDRV_DEVICE_ADD(IDEHD_0, IDEHD, 0)
#endif


/*
    MDRV_MFMHD_START(mfmhd)             \
    MDRV_MFMHD_LOAD(mfmhd)              \
    MDRV_MFMHD_UNLOAD(mfmhd)
*/
