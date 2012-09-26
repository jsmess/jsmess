/***************************************************************************

    coco_vhd.c

    Color Computer Virtual Hard Drives

****************************************************************************

    Technical specs on the Virtual Hard Disk interface

    Address       Description
    -------       -----------
    FF80          Logical record number (high byte)
    FF81          Logical record number (middle byte)
    FF82          Logical record number (low byte)
    FF83          Command/status register
    FF84          Buffer address (high byte)
    FF85          Buffer address (low byte)

    Set the other registers, and then issue a command to FF83 as follows:

     0 = read 256-byte sector at LRN
     1 = write 256-byte sector at LRN
     2 = flush write cache (Closes and then opens the image file)

    Error values:

     0 = no error
    -1 = power-on state (before the first command is recieved)
    -2 = invalid command
     2 = VHD image does not exist
     4 = Unable to open VHD image file
     5 = access denied (may not be able to write to VHD image)

    IMPORTANT: The I/O buffer must NOT cross an 8K MMU bank boundary.

 ***************************************************************************/

#include "emu.h"
#include "coco_vhd.h"
#include "includes/coco.h"
#include "machine/ram.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define VERBOSE 0

#define VHDSTATUS_OK					0x00
#define VHDSTATUS_NO_VHD_ATTACHED		0x02
#define VHDSTATUS_ACCESS_DENIED			0x05
#define VHDSTATUS_UNKNOWN_COMMAND		0xFE
#define VHDSTATUS_POWER_ON_STATE		0xFF

#define VHDCMD_READ		0
#define VHDCMD_WRITE	1
#define VHDCMD_FLUSH	2



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _vhd_info vhd_info;
struct _vhd_info
{
	UINT32 logical_record_number;
	UINT32 buffer_address;
	UINT8 status;
};



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE vhd_info *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == COCO_VHD);
	return (vhd_info *) downcast<legacy_device_base *>(device)->token();
}



/***************************************************************************
    CORE IMPLEMENTATION
***************************************************************************/

static DEVICE_START( coco_vhd )
{
	vhd_info *vhd = get_safe_token(device);
	vhd->status = VHDSTATUS_NO_VHD_ATTACHED;
}



static DEVICE_IMAGE_LOAD( coco_vhd )
{
	vhd_info *vhd = get_safe_token(&image.device());
	vhd->status = VHDSTATUS_POWER_ON_STATE;
	vhd->logical_record_number = 0;
	vhd->buffer_address = 0;
	return IMAGE_INIT_PASS;
}



static void coco_vhd_readwrite(device_t *device, UINT8 data)
{
	vhd_info *vhd = get_safe_token(device);
	device_image_interface *image = dynamic_cast<device_image_interface *>(device);
	int result;
	int phyOffset;
	UINT32 nBA = vhd->buffer_address;
	UINT32 bytes_to_read;
	UINT32 bytes_to_write;
	UINT64 seek_position;
	UINT64 total_size;
	char buffer[1024];

	/* access the image */
	if (!image->exists())
	{
		vhd->status = VHDSTATUS_NO_VHD_ATTACHED;
		return;
	}

	/* perform the seek */
	seek_position = ((UINT64) 256) * vhd->logical_record_number;
	total_size = image->length();
	result = image->fseek(MIN(seek_position, total_size), SEEK_SET);
	if (result < 0)
	{
		vhd->status = VHDSTATUS_ACCESS_DENIED;
		return;
	}

	/* expand the disk, if necessary */
	if (data == VHDCMD_WRITE)
	{
		while(total_size < seek_position)
		{
			memset(buffer, 0, sizeof(buffer));

			bytes_to_write = (UINT32) MIN(seek_position - total_size, (UINT64) sizeof(buffer));
			result = image->fwrite(buffer, bytes_to_write);
			if (result != bytes_to_write)
			{
				vhd->status = VHDSTATUS_ACCESS_DENIED;
				return;
			}

			total_size += bytes_to_write;
		}
	}

	phyOffset = coco3_mmu_translate(device->machine(), (nBA >> 12 ) / 2, nBA % 8192 );

	switch(data)
	{
		case VHDCMD_READ: /* Read sector */
			memset(&ram_get_ptr(device->machine().device(RAM_TAG))[phyOffset], 0, 256);
			if (total_size > seek_position)
			{
				bytes_to_read = (UINT32) MIN((UINT64) 256, total_size - seek_position);
				result = image->fread(&ram_get_ptr(device->machine().device(RAM_TAG))[phyOffset], bytes_to_read);
				if (result != bytes_to_read)
				{
					vhd->status = VHDSTATUS_ACCESS_DENIED;
					return;
				}
			}

			vhd->status = VHDSTATUS_OK;
			break;

		case VHDCMD_WRITE: /* Write Sector */
			result = image->fwrite((&ram_get_ptr(device->machine().device(RAM_TAG))[phyOffset]), 256);

			if (result != 256)
			{
				vhd->status = VHDSTATUS_ACCESS_DENIED;
				return;
			}

			vhd->status = VHDSTATUS_OK;
			break;

		case VHDCMD_FLUSH: /* Flush file cache */
			vhd->status = VHDSTATUS_OK;
			break;

		default:
			vhd->status = VHDSTATUS_UNKNOWN_COMMAND;
			break;
	}
}



READ8_DEVICE_HANDLER(coco_vhd_io_r)
{
	vhd_info *vhd = get_safe_token(device);
	UINT8 result = 0;

	switch(offset)
	{
		case 0xff83 - 0xff80:
			if (VERBOSE)
				logerror("vhd: Status read: %d\n", vhd->status);
			result = vhd->status;
			break;
	}
	return result;
}



WRITE8_DEVICE_HANDLER(coco_vhd_io_w)
{
	vhd_info *vhd = get_safe_token(device);
	int pos;

	switch(offset)
	{
		case 0xff80 - 0xff80:
		case 0xff81 - 0xff80:
		case 0xff82 - 0xff80:
			pos = ((0xff82 - 0xff80) - offset) * 8;
			vhd->logical_record_number &= ~(0xFF << pos);
			vhd->logical_record_number += data << pos;
			if (VERBOSE)
				logerror("vhd: LRN write: %6.6X\n", vhd->logical_record_number);
			break;

		case 0xff83 - 0xff80:
			coco_vhd_readwrite(device, data);
			if (VERBOSE)
				logerror("vhd: Command: %d\n", data);
			break;

		case 0xff84 - 0xff80:
			vhd->buffer_address &= 0xFFFF00FF;
			vhd->buffer_address += data << 8;
			if (VERBOSE)
				logerror("vhd: BA write: %X (%2.2X..)\n", vhd->buffer_address, data);
			break;

		case 0xff85 - 0xff80:
			vhd->buffer_address &= 0xFFFFFF00;
			vhd->buffer_address += data;
			if (VERBOSE)
				logerror("vhd: BA write: %X (..%2.2X)\n", vhd->buffer_address, data);
			break;
	}
}



DEVICE_GET_INFO(coco_vhd)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(vhd_info); break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0; break;
		case DEVINFO_INT_IMAGE_TYPE:					info->i = IO_HARDDISK; break;
		case DEVINFO_INT_IMAGE_READABLE:				info->i = 1; break;
		case DEVINFO_INT_IMAGE_WRITEABLE:				info->i = 1; break;
		case DEVINFO_INT_IMAGE_CREATABLE:				info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(coco_vhd); break;
		case DEVINFO_FCT_IMAGE_LOAD:					info->f = (genf *) DEVICE_IMAGE_LOAD_NAME(coco_vhd); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Virtual Hard Disk"); break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Virtual Hard Disk"); break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__); break;
		case DEVINFO_STR_IMAGE_FILE_EXTENSIONS:			strcpy(info->s, "vhd"); break;
	}
}

DEFINE_LEGACY_IMAGE_DEVICE(COCO_VHD, coco_vhd);
