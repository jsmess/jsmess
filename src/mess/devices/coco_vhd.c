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

#include "driver.h"
#include "devices/coco_vhd.h"

#define VERBOSE 0

#define VHDSTATUS_OK					0x00
#define VHDSTATUS_NO_VHD_ATTACHED		0x02
#define VHDSTATUS_ACCESS_DENIED			0x05
#define VHDSTATUS_UNKNOWN_COMMAND		0xFE
#define VHDSTATUS_POWER_ON_STATE		0xFF

#define VHDCMD_READ		0
#define VHDCMD_WRITE	1
#define VHDCMD_FLUSH	2

static UINT32 logical_record_number;
static UINT32 buffer_address;
static UINT8 vhd_status;


static mess_image *vhd_image(void)
{
	mess_image *image;
	image = image_from_devtype_and_index(IO_VHD, 0);
	return image_exists(image) ? image : NULL;
}



static int device_init_coco_vhd(mess_image *image)
{
	vhd_status = VHDSTATUS_NO_VHD_ATTACHED;
	return INIT_PASS;
}



static int device_load_coco_vhd(mess_image *image)
{
	vhd_status = VHDSTATUS_POWER_ON_STATE;
	logical_record_number = 0;
	buffer_address = 0;
	return INIT_PASS;
}



static void coco_vhd_readwrite(UINT8 data)
{
	mess_image *vhdfile;
	int result;
	int phyOffset;
	UINT32 nBA = buffer_address;
	UINT32 bytes_to_read;
	UINT32 bytes_to_write;
	UINT64 seek_position;
	UINT64 total_size;
	char buffer[1024];

	/* access the image */
	vhdfile = vhd_image();
	if (!vhdfile)
	{
		vhd_status = VHDSTATUS_NO_VHD_ATTACHED;
		return;
	}

	/* perform the seek */
	seek_position = ((UINT64) 256) * logical_record_number;
	total_size = image_length(vhdfile);
	result = image_fseek(vhdfile, MIN(seek_position, total_size), SEEK_SET);
	if (result < 0)
	{
		vhd_status = VHDSTATUS_ACCESS_DENIED;
		return;
	}

	/* expand the disk, if necessary */
	if (data == VHDCMD_WRITE)
	{
		while(total_size < seek_position)
		{
			memset(buffer, 0, sizeof(buffer));

			bytes_to_write = (UINT32) MIN(seek_position - total_size, (UINT64) sizeof(buffer));
			result = image_fwrite(vhdfile, buffer, bytes_to_write);
			if (result != bytes_to_write)
			{
				vhd_status = VHDSTATUS_ACCESS_DENIED;
				return;
			}

			total_size += bytes_to_write;
		}
	}

	phyOffset = coco3_mmu_translate( (nBA >> 12 ) / 2, nBA % 8192 );

	switch(data)
	{
		case VHDCMD_READ: /* Read sector */
			memset(&mess_ram[phyOffset], 0, 256);
			if (total_size > seek_position)
			{
				bytes_to_read = (UINT32) MIN((UINT64) 256, total_size - seek_position);
				result = image_fread(vhdfile, &mess_ram[phyOffset], bytes_to_read);
				if (result != bytes_to_read)
				{
					vhd_status = VHDSTATUS_ACCESS_DENIED;
					return;
				}
			}

			vhd_status = VHDSTATUS_OK;
			break;

		case VHDCMD_WRITE: /* Write Sector */
			result = image_fwrite(vhdfile, &(mess_ram[phyOffset]), 256);

			if (result != 256)
			{
				vhd_status = VHDSTATUS_ACCESS_DENIED;
				return;
			}

			vhd_status = VHDSTATUS_OK;
			break;

		case VHDCMD_FLUSH: /* Flush file cache */
			vhd_status = VHDSTATUS_OK;
			break;

		default:
			vhd_status = VHDSTATUS_UNKNOWN_COMMAND;
			break;
	}
}



READ8_HANDLER(coco_vhd_io_r)
{
	UINT8 result = 0;

	switch(offset)
	{
		case 0xff83 - 0xff40:
			if (VERBOSE)
				logerror("vhd: Status read: %d\n", vhd_status );
			result = vhd_status;
			break;
	}
	return result;
}



WRITE8_HANDLER(coco_vhd_io_w)
{
	int pos;

	switch(offset)
	{
		case 0xff80 - 0xff40:
		case 0xff81 - 0xff40:
		case 0xff82 - 0xff40:
			pos = ((0xff82 - 0xff40) - offset) * 8;
			logical_record_number &= ~(0xFF << pos);
			logical_record_number += data << pos;
			if (VERBOSE)
				logerror("vhd: LRN write: %6.6X\n", logical_record_number);
			break;

		case 0xff83 - 0xff40:
			coco_vhd_readwrite( data );
			if (VERBOSE)
				logerror("vhd: Command: %d\n", data);
			break;

		case 0xff84 - 0xff40:
			buffer_address &= 0xFFFF00FF;
			buffer_address += data << 8;
			if (VERBOSE)
				logerror("vhd: BA write: %X (%2.2X..)\n", buffer_address, data);
			break;

		case 0xff85 - 0xff40:
			buffer_address &= 0xFFFFFF00;
			buffer_address += data;
			if (VERBOSE)
				logerror("vhd: BA write: %X (..%2.2X)\n", buffer_address, data);
			break;
	}
}



void coco_vhd_device_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:							info->i = IO_VHD; break;
		case DEVINFO_INT_READABLE:						info->i = 1; break;
		case DEVINFO_INT_WRITEABLE:						info->i = 1; break;
		case DEVINFO_INT_CREATABLE:						info->i = 1; break;
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_INIT:							info->init = device_init_coco_vhd; break;
		case DEVINFO_PTR_LOAD:							info->load = device_load_coco_vhd; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_DEV_FILE:						strcpy(info->s = device_temp_str(), __FILE__); break;
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "vhd"); break;
		case DEVINFO_STR_DESCRIPTION+0:					strcpy(info->s = device_temp_str(), "Virtual Hard Disk"); break;
	}
}

