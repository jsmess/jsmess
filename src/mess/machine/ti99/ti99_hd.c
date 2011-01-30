/*************************************************************************

    Hard disk support

    This device wraps the plain image device as that device does not allow
    for internal states (like head position)
    The plain device is a subdevice ("drive") of this device, so we
    get names like "mfmhd0:drive"

    Michael Zapf, April 2010

**************************************************************************/

#include "emu.h"
#include "imageutl.h"
#include "harddisk.h"
#include "imagedev/harddriv.h"
#include "machine/smc92x4.h"

#include "ti99_hd.h"

typedef struct _mfmhd
{
	int current_cylinder;
	int current_head;
	int seeking;
	int status;
	int id_index; /* position in track for seeking the sector; counts the sector number */
} mfmhd_state, idehd_state;

INLINE mfmhd_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == MFMHD || device->type() == IDEHD);

	return (mfmhd_state *)downcast<legacy_device_base *>(device)->token();
}

#if 0
static void dump_contents(UINT8 *buffer, int length)
{
	for (int i=0; i < length; i+=16)
	{
		for (int j=0; j < 16; j+=2)
		{
			printf("%02x%02x ", buffer[i+j], buffer[i+j+1]);
		}
		printf("\n");
	}
}
#endif

/*
    Returns the linear sector number, given the CHS data.
*/
static int harddisk_chs_to_lba(hard_disk_file *hdfile, int cylinder, int head, int sector, UINT32 *lba)
{
	const hard_disk_info *info;

	if ( hdfile != NULL)
	{
		info = hard_disk_get_info(hdfile);

		if (	(cylinder >= info->cylinders) ||
				(head >= info->heads) ||
				(sector >= info->sectors))
		return FALSE;

		*lba = (cylinder * info->heads + head)
				* info->sectors
				+ sector;
		return TRUE;
	}
	return FALSE;
}

/* Accessor functions */
void ti99_mfm_harddisk_read_sector(device_t *harddisk, int cylinder, int head, int sector, UINT8 **buf, int *sector_length)
{
	UINT32 lba;
	mfmhd_state *hd = get_safe_token(harddisk);
	device_t *drive = harddisk->subdevice("drive");
	hard_disk_file *file = hd_get_hard_disk_file(drive);

	*sector_length = 256;
	*buf = (UINT8 *)malloc(*sector_length);

	if (cylinder != hd->current_cylinder)
	{
		return;
	}

	if (file==NULL)
	{
		hd->status &= ~MFMHD_READY;
		return;
	}

	if (!harddisk_chs_to_lba(file, cylinder, head, sector, &lba))
	{
		hd->status &= ~MFMHD_READY;
		return;
	}

	if (!hard_disk_read(file, lba, *buf))
	{
		hd->status &= ~MFMHD_READY;
		return;
	}
	/* printf("ti99_hd read sector  c=%04d h=%02d s=%02d\n", cylinder, head, sector); */
	hd->status |= MFMHD_READY;
}

void ti99_mfm_harddisk_write_sector(device_t *harddisk, int cylinder, int head, int sector, UINT8 *buf, int sector_length)
{
	UINT32 lba;
	mfmhd_state *hd = get_safe_token(harddisk);
	device_t *drive = harddisk->subdevice("drive");
	hard_disk_file *file = hd_get_hard_disk_file(drive);

	if (file==NULL)
	{
		hd->status &= ~MFMHD_READY;
		return;
	}

	if (!harddisk_chs_to_lba(file, cylinder, head, sector, &lba))
	{
		hd->status &= ~MFMHD_READY;
		hd->status |= MFMHD_WRFAULT;
		return;
	}

	if (!hard_disk_write(file, lba, buf))
	{
		hd->status &= ~MFMHD_READY;
		hd->status |= MFMHD_WRFAULT;
		return;
	}

	hd->status |= MFMHD_READY;
}

#define TI99HD_BLOCKNOTFOUND -1

/*
    Searches a block containing number * byte, starting at the given
    position. Returns the position of the first byte of the block.
*/
static int find_block(const UINT8 *buffer, int start, int stop, UINT8 byte, size_t number)
{
	int i = start;
	size_t current = number;
	while (i < stop && current > 0)
	{
		if (buffer[i++] != byte)
		{
			current = number;
		}
		else
		{
			current--;
		}
	}
	if (current==0)
	{
		return i - number;
	}
	else
		return TI99HD_BLOCKNOTFOUND;
}

/*
    Reads a complete track. We have to rebuild the gaps. This is basically
    done in the same way as in the SDF format in ti99_dsk.

    WARNING: This function is untested! We need to create a suitable
    application program for the TI which makes use of it.
*/
void ti99_mfm_harddisk_read_track(device_t *harddisk, int head, UINT8 **pbuffer, int *data_count)
{
	mfmhd_state *hd = get_safe_token(harddisk);
	device_t *drive = harddisk->subdevice("drive");

	/* We assume an interleave of 3 for 32 sectors. */
	int step = 3;
	UINT32 lba;
	int i;

	int gap1 = 16;
	int gap2 = 8;
	int gap3 = 15;
	int gap4 = 340;
	int sync = 13;
	int count;
	int size;
	int position = 0;
	int sector;
	int crc;

	const hard_disk_info *info;
	hard_disk_file *file = hd_get_hard_disk_file(drive);

	if (file==NULL)
	{
		hd->status &= ~MFMHD_READY;
		return;
	}

	info = hard_disk_get_info(file);

	count = info->sectors;
	size = info->sectorbytes/128;

	*data_count = gap1 + count*(sync+12+gap2+sync+size*128+gap3)+gap4;

	UINT8 *trackdata = (UINT8*)malloc(*data_count);

	/* Write lead-in. */
	memset(trackdata, 0x4e, gap1);

	position += gap1;
	for (i=0; i < info->sectors; i++)
	{
		sector = (i*step) % info->sectors;
		memset(&trackdata[position], 0x00, sync);
		position += 10;

		/* Write sync */
		trackdata[position++] = 0xa1;

		/* Write IDAM */
		trackdata[position++] = cylinder_to_ident(hd->current_cylinder);
		trackdata[position++] = hd->current_cylinder;
		trackdata[position++] = head;
		trackdata[position++] = sector;
		trackdata[position++] = (size==1)? 0x00 : (0x01 << (size-1));

		/* Set CRC16 */
		crc = ccitt_crc16(0xffff, &trackdata[position-5], 5);
		trackdata[position++] = (crc>>8)&0xff;
		trackdata[position++] = crc & 0xff;

		/* Write Gap2 */
		memset(&trackdata[position], 0x4e, gap2);
		position += gap2;

		/* Write sync */
		memset(&trackdata[position], 0x00, sync);
		position += sync;
		trackdata[position++] = 0xa1;

		/* Write DAM */
		trackdata[position++] = 0xfb;

		/* Locate the sector content in the image and load it. */

		if (!harddisk_chs_to_lba(file, hd->current_cylinder, head, sector, &lba))
		{
			hd->status &= ~MFMHD_READY;
			free(trackdata);
			return;
		}

		if (!hard_disk_read(file, lba, &trackdata[position]))
		{
			hd->status &= ~MFMHD_READY;
			free(trackdata);
			return;
		}

		position += size;

		/* Set CRC16. Includes the address mark. */
		crc = ccitt_crc16(0xffff, &trackdata[position-size-1], size+1);
		trackdata[position++] = (crc>>8)&0xff;
		trackdata[position++] = crc & 0xff;

		/* Write remaining 3 bytes which would have been used for ECC. */
		memset(&trackdata[position], 0x00, 3);
		position += 3;

		/* Write Gap3 */
		memset(&trackdata[position], 0x4e, gap3);
		position += gap3;
	}
	/* Write Gap 4 */
	memset(&trackdata[position], 0x4e, gap4);
	position += gap4;

	hd->status |= MFMHD_READY;

	*pbuffer = trackdata;

	// Remember to free the buffer! Except when an error has occured,
	// the function returns a newly allocated buffer.
	// Errors are indicated by READY=FALSE.
}

/*
    Writes a track to the image. We need to isolate the sector contents.
    This is basically done in the same way as in the SDF format in ti99_dsk.
*/
void ti99_mfm_harddisk_write_track(device_t *harddisk, int head, UINT8 *track_image, int data_count)
{
	int current_pos = 0;
	int found;
	UINT8 wident, whead = 0, wsector = 0, wsize;
	UINT16 wcyl = 0;
	int state;

	/* Only search in the first 100 bytes for the start. */
	int gap1 = 16;
	int sync = 13;

	UINT32 lba;
	mfmhd_state *hd = get_safe_token(harddisk);
	device_t *drive = harddisk->subdevice("drive");
	hard_disk_file *file = hd_get_hard_disk_file(drive);

	/* printf("ti99_hd write track c=%d h=%d\n", hd->current_cylinder, head); */

	if (file==NULL)
	{
		hd->status &= ~MFMHD_READY;
		return;
	}

	current_pos = find_block(track_image, 0, 100, 0x4e, gap1);

	// In case of defect formats, we continue as far as possible. This
	// may lead to sectors not being written. */
	if (current_pos==TI99HD_BLOCKNOTFOUND)
	{
		logerror("ti99_hd error: write track: Cannot find gap1 for cylinder %d, head %d.\n", hd->current_cylinder, head);
		/* What now? The track data are illegal, so we refuse to continue. */
		hd->status |= MFMHD_WRFAULT;
		return;
	}

	/* Get behind GAP1 */
	current_pos += gap1;
	found = FALSE;

	while (current_pos < data_count)
	{
		/* We must find the address block to determine the sector. */
		int new_pos = find_block(track_image, current_pos, data_count, 0x00, sync);
		if (new_pos==TI99HD_BLOCKNOTFOUND)
		{
			/* Forget about the rest; we're done. */
			if (found) break;  /* we were already successful, so all ok */
			logerror("ti99_hd error: write track: Cannot find sync for track %d, head %d.\n", hd->current_cylinder, head);
			hd->status |= MFMHD_WRFAULT;
			return;
		}
		found = TRUE;

		new_pos = new_pos + sync; /* skip sync bytes */

		if (track_image[new_pos]==0xa1)
		{
			/* IDAM found. */
			current_pos = new_pos + 1;
			wident = track_image[current_pos];
			wcyl = track_image[current_pos+1] + ((track_image[current_pos+2]&0x70)<<4);
			whead = track_image[current_pos+2]&0x0f;
			wsector = track_image[current_pos+3];
			wsize = track_image[current_pos+4];

			if (wcyl == hd->current_cylinder && whead == head)
			{
				if (!harddisk_chs_to_lba(file, wcyl, whead, wsector, &lba))
				{
					hd->status |= MFMHD_WRFAULT;
					return;
				}

				/* Skip to the sector content. */
				new_pos = find_block(track_image, current_pos, data_count, 0x00, sync);
				current_pos = new_pos + sync;
				if (track_image[current_pos]==0xa1)
				{
					current_pos += 2;
					state = hard_disk_write(file, lba, track_image+current_pos);
					if (state==0)
					{
						logerror("ti99_hd error: write track: Write error during formatting cylinder %d, head %d\n", wcyl, whead);
						hd->status |= MFMHD_WRFAULT;
						return;
					}
					current_pos = current_pos+256+2; /* Skip contents and CRC */
				}
				else
				{
					logerror("ti99_hd error: write track: Cannot find DAM for cylinder %d, head %d, sector %d.\n", wcyl, whead, wsector);
					hd->status |= MFMHD_WRFAULT;
					return;
				}
			}
			else
			{
				logerror("ti99_hd error: write track: Cylinder/head mismatch. Drive is on cyl=%d, head=%d, track data say cyl=%d, head=%d\n", hd->current_cylinder, head, wcyl, whead);
				hd->status |= MFMHD_WRFAULT;
			}
		}
		else
		{
			logerror("ti99_hd error: write track: Invalid track image for cyl=%d, head=%d. Cannot find any IDAM in track data.\n",  hd->current_cylinder, head);
			hd->status |= MFMHD_WRFAULT;
			return;
		}
	}
}

UINT8 ti99_mfm_harddisk_status(device_t *harddisk)
{
	UINT8 status = 0;
	device_t *drive;
	mfmhd_state *hd = get_safe_token(harddisk);
	drive = harddisk->subdevice("drive");
	hard_disk_file *file = hd_get_hard_disk_file(drive);

	if (file!=NULL)
		status |= MFMHD_READY;

	if (hd->current_cylinder==0)
		status |= MFMHD_TRACK00;

	if (!hd->seeking)
		status |= MFMHD_SEEKCOMP;

	if (hd->id_index == 0)
		status |= MFMHD_INDEX;

	hd->status = status;
	return status;
}

void ti99_mfm_harddisk_seek(device_t *harddisk, int direction)
{
	device_t *drive = harddisk->subdevice("drive");
	mfmhd_state *hd = get_safe_token(harddisk);
	const hard_disk_info *info;
	hard_disk_file *file = hd_get_hard_disk_file(drive);

	if (file==NULL)	return;

	info = hard_disk_get_info(file);

	hd->seeking = TRUE;

	if (direction<0)
	{
		if (hd->current_cylinder>0)
			hd->current_cylinder--;
	}
	else
	{
		if (hd->current_cylinder < info->cylinders)
			hd->current_cylinder++;
	}

	hd->seeking = FALSE;
}

void ti99_mfm_harddisk_get_next_id(device_t *harddisk, int head, chrn_id_hd *id)
{
	device_t *drive = harddisk->subdevice("drive");
	mfmhd_state *hd = get_safe_token(harddisk);
	const hard_disk_info *info;
	hard_disk_file *file;
	int interleave = 3;

	file = hd_get_hard_disk_file(drive);
	if (file==NULL)
	{
		hd->status &= ~MFMHD_READY;
		return;
	}

	info = hard_disk_get_info(file);

	hd->current_head = head;

	/* TODO: implement an interleave suitable for the number of sectors in the track. */
	hd->id_index = (hd->id_index + interleave) % info->sectors;

	/* build a new info block. */
	id->C = hd->current_cylinder;
	id->H = hd->current_head;
	id->R = hd->id_index;
	id->N = 1;
	id->data_id = hd->id_index;
	id->flags = 0;
}

static DEVICE_START( mfmhd )
{
}

static DEVICE_START( idehd )
{
}


MACHINE_CONFIG_FRAGMENT( mfmhd )
	MCFG_DEVICE_ADD( "drive", HARDDISK, 0 )
MACHINE_CONFIG_END

MACHINE_CONFIG_FRAGMENT( idehd )
	MCFG_DEVICE_ADD( "drive", IDE_HARDDISK, 0 )
MACHINE_CONFIG_END

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)             p##mfmhd##s
#define DEVTEMPLATE_FEATURES            DT_HAS_START | DT_HAS_MACHINE_CONFIG
#define DEVTEMPLATE_NAME                "TI99 MFM Hard disk"
#define DEVTEMPLATE_FAMILY              "Hard disks"
#define DEVTEMPLATE_VERSION		"1.0"
#define DEVTEMPLATE_CREDITS		"Copyright MESS Team"
#include "devtempl.h"

#undef DEVTEMPLATE_ID
#undef DEVTEMPLATE_FEATURES
#undef DEVTEMPLATE_NAME

#define DEVTEMPLATE_ID(p,s)             p##idehd##s
#define DEVTEMPLATE_FEATURES            DT_HAS_START | DT_HAS_MACHINE_CONFIG
#define DEVTEMPLATE_NAME                "TI99 IDE Hard disk"
#include "devtempl.h"

DEFINE_LEGACY_DEVICE(MFMHD, mfmhd);
DEFINE_LEGACY_DEVICE(IDEHD, idehd);
