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
void ti99_mfm_harddisk_read_sector(device_t *harddisk, int cylinder, int head, int sector, UINT8 **buf, int *sector_length);
void ti99_mfm_harddisk_write_sector(device_t *harddisk, int cylinder, int head, int sector, UINT8 *buf, int sector_length);
void ti99_mfm_harddisk_read_track(device_t *harddisk, int head, UINT8 **buffer, int *data_count);
void ti99_mfm_harddisk_write_track(device_t *harddisk, int head, UINT8 *buffer, int data_count);
UINT8 ti99_mfm_harddisk_status(device_t *harddisk);
void ti99_mfm_harddisk_seek(device_t *harddisk, int direction);
void ti99_mfm_harddisk_get_next_id(device_t *harddisk, int head, chrn_id_hd *id);

DECLARE_LEGACY_DEVICE(MFMHD, mfmhd);

DECLARE_LEGACY_DEVICE(IDEHD, idehd);

#define MCFG_MFMHD_3_DRIVES_ADD()			\
	MCFG_DEVICE_ADD(MFMHD_0, MFMHD, 0)		\
	MCFG_DEVICE_ADD(MFMHD_1, MFMHD, 0)		\
	MCFG_DEVICE_ADD(MFMHD_2, MFMHD, 0)		\
	MCFG_DEVICE_ADD(IDEHD_0, IDEHD, 0)
#endif


/*
    MCFG_MFMHD_START(mfmhd)             \
    MCFG_MFMHD_LOAD(mfmhd)              \
    MCFG_MFMHD_UNLOAD(mfmhd)
*/
