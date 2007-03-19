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

static long	logicalRecordNumber;
static long	bufferAddress;
static UINT8 vhdStatus;

#define VERBOSE 0

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif



static mess_image *vhd_image(void)
{
	return image_from_devtype_and_index(IO_VHD, 0);
}



static int device_init_coco_vhd(mess_image *image)
{
	vhdStatus = 2;	/* No VHD attached */
	return INIT_PASS;
}



static int device_load_coco_vhd(mess_image *image)
{
	vhdStatus = 0xff; /* -1, Power on state */
	logicalRecordNumber = 0;
	bufferAddress = 0;
	return INIT_PASS;

}



static void coco_vhd_readwrite(UINT8 data)
{
	mess_image *vhdfile;
	int result;
	int phyOffset;
	long nBA = bufferAddress;

	vhdfile = vhd_image();
	if (!vhdfile)
	{
		vhdStatus = 2; /* No VHD attached */
		return;
	}

	result = image_fseek(vhdfile, ((logicalRecordNumber)) * 256, SEEK_SET);

	if (result < 0)
	{
		vhdStatus = 5; /* access denied */
		return;
	}

	phyOffset = coco3_mmu_translate( (nBA >> 12 ) / 2, nBA % 8192 );

	switch(data) {
	case 0: /* Read sector */
		result = image_fread(vhdfile, &(mess_ram[phyOffset]), 256);

		if( result != 256 )
		{
			vhdStatus = 5; /* access denied */
			return;
		}

		vhdStatus = 0; /* Aok */
		break;

	case 1: /* Write Sector */
		result = image_fwrite(vhdfile, &(mess_ram[phyOffset]), 256);

		if (result != 256)
		{
			vhdStatus = 5; /* access denied */
			return;
		}

		vhdStatus = 0; /* Aok */
		break;

	case 2: /* Flush file cache */
		vhdStatus = 0; /* Aok */
		break;

	default:
		vhdStatus = 0xfe; /* -2, Unknown command */
		break;
	}
}



READ8_HANDLER(coco_vhd_io_r)
{
	UINT8 result = 0;

	switch(offset) {
	case 0xff83 - 0xff40:
		LOG(( "vhd: Status read: %d\n", vhdStatus ));
		result = vhdStatus;
		break;
	}
	return result;
}



WRITE8_HANDLER(coco_vhd_io_w)
{
	int pos;
	
	switch(offset) {
	case 0xff80 - 0xff40:
	case 0xff81 - 0xff40:
	case 0xff82 - 0xff40:
		pos = ((0xff82 - 0xff40) - offset) * 8;
		logicalRecordNumber &= ~(0xFF << pos);
		logicalRecordNumber += data << pos;
		LOG(( "vhd: LRN write: %6.6X\n", logicalRecordNumber ));
		break;

	case 0xff83 - 0xff40:
		coco_vhd_readwrite( data );
		LOG(( "vhd: Command: %d\n", data ));
		break;

	case 0xff84 - 0xff40:
		bufferAddress &= 0xFFFF00FF;
		bufferAddress += data << 8;
		LOG(( "vhd: BA write: %X (%2.2X..)\n", bufferAddress, data ));
		break;

	case 0xff85 - 0xff40:
		bufferAddress &= 0xFFFFFF00;
		bufferAddress += data;
		LOG(( "vhd: BA write: %X (..%2.2X)\n", bufferAddress, data ));
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

