/*
    smartmed.c: SmartMedia Flash ROM emulation

    The SmartMedia is a Flash ROM in a fancy card.  It is used in a variety of
    digital devices (still cameras...) and can be interfaced with a computer.

    References:
    Datasheets for various SmartMedia chips were found on Samsung and Toshiba's
    sites (http://www.toshiba.com/taec and
    http://www.samsung.com/Products/Semiconductor/Flash/FlashCard/SmartMedia)

    TODO:
    * support multi-plane mode?
    * use HD-format images instead of our experimental custom format?

    Raphael Nabet 2004
*/

#include "driver.h"

#include "smartmed.h"

#include "harddisk.h"
#include "devices/harddriv.h"

#define MAX_SMARTMEDIA	1

/* machine-independant big-endian 32-bit integer */
typedef struct UINT32BE
{
	UINT8 bytes[4];
} UINT32BE;

INLINE UINT32 get_UINT32BE(UINT32BE word)
{
	return (word.bytes[0] << 24) | (word.bytes[1] << 16) | (word.bytes[2] << 8) | word.bytes[3];
}

#ifdef UNUSED_FUNCTION
INLINE void set_UINT32BE(UINT32BE *word, UINT32 data)
{
	word->bytes[0] = (data >> 24) & 0xff;
	word->bytes[1] = (data >> 16) & 0xff;
	word->bytes[2] = (data >> 8) & 0xff;
	word->bytes[3] = data & 0xff;
}
#endif

/* SmartMedia image header */
typedef struct disk_image_header
{
	UINT8 version;
	UINT32BE page_data_size;
	UINT32BE page_total_size;
	UINT32BE num_pages;
	UINT32BE log2_pages_per_block;
} disk_image_header;

enum
{
	header_len = sizeof(disk_image_header)
};

typedef struct _smartmedia_t smartmedia_t;
struct _smartmedia_t
{
	int page_data_size;	// 256 for a 2MB card, 512 otherwise
	int page_total_size;// 264 for a 2MB card, 528 otherwise
	int num_pages;		// 8192 for a 4MB card, 16184 for 8MB, 32768 for 16MB,
						// 65536 for 32MB, 131072 for 64MB, 262144 for 128MB...
						// 0 means no card loaded
	int log2_pages_per_block;	// log2 of number of pages per erase block (usually 4 or 5)

	UINT8 *data_ptr;	// FEEPROM data area
	UINT8 *data_uid_ptr;

	enum
	{
		SM_M_INIT,		// initial state
		SM_M_READ,		// read page data
		SM_M_PROGRAM,	// program page data
		SM_M_ERASE,		// erase block data
		SM_M_READSTATUS,// read status
		SM_M_READID,		// read ID
		SM_M_30
	} mode;				// current operation mode
	enum
	{
		SM_PM_A,		// accessing first 256-byte half of 512-byte data field
		SM_PM_B,		// accessing second 256-byte half of 512-byte data field
		SM_PM_C			// accessing spare field
	} pointer_mode;		// pointer mode

	int page_addr;		// page address pointer
	int byte_addr;		// byte address pointer
	int addr_load_ptr;	// address load pointer

	int status;			// current status
	int accumulated_status;	// accumulated status

	UINT8 *pagereg;	// page register used by program command
	UINT8 id[3];		// chip ID
	UINT8 mp_opcode;	// multi-plane operation code
	
	int mode_3065;
};


INLINE smartmedia_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == SMARTMEDIA);

	return (smartmedia_t *)device->token;
}


/*
    Init a SmartMedia image
*/
static DEVICE_START( smartmedia )
{
	smartmedia_t *sm = get_safe_token(device);

	sm->page_data_size = 0;
	sm->page_total_size = 0;
	sm->num_pages = 0;
	sm->log2_pages_per_block = 0;
	sm->data_ptr = NULL;
	sm->data_uid_ptr = NULL;
	sm->mode = SM_M_INIT;
	sm->pointer_mode = SM_PM_A;
	sm->page_addr = 0;
	sm->byte_addr = 0;
	sm->status = 0x40;
	sm->accumulated_status = 0;
	sm->pagereg = NULL;
	sm->id[0] = sm->id[1] = sm->id[2] = 0;
	sm->mp_opcode = 0;
	sm->mode_3065 = 0;
}

/*
    Load a SmartMedia image
*/
static DEVICE_IMAGE_LOAD( smartmedia )
{
	smartmedia_t *sm = get_safe_token(image);
	disk_image_header custom_header;
	int bytes_read;


	bytes_read = image_fread(image, &custom_header, sizeof(custom_header));
	if (bytes_read != sizeof(custom_header))
	{
		return INIT_FAIL;
	}

	if (custom_header.version > 1)
	{
		return INIT_FAIL;
	}

	sm->page_data_size = get_UINT32BE(custom_header.page_data_size);
	sm->page_total_size = get_UINT32BE(custom_header.page_total_size);
	sm->num_pages = get_UINT32BE(custom_header.num_pages);
	sm->log2_pages_per_block = get_UINT32BE(custom_header.log2_pages_per_block);
	sm->data_ptr = auto_alloc_array(image->machine, UINT8, sm->page_total_size*sm->num_pages);
	sm->data_uid_ptr = auto_alloc_array(image->machine, UINT8, 256 + 16);
	sm->mode = SM_M_INIT;
	sm->pointer_mode = SM_PM_A;
	sm->page_addr = 0;
	sm->byte_addr = 0;
	sm->status = 0x40;
	if (!image_is_writable(image))
		sm->status |= 0x80;
	sm->accumulated_status = 0;
	sm->pagereg = auto_alloc_array(image->machine, UINT8, sm->page_total_size);
	sm->id[0] = sm->id[1] = sm->id[2] = 0;

	if (custom_header.version == 0)
	{
		image_fread(image, sm->id, 2);
		image_fread(image, &sm->mp_opcode, 1);
	}
	else if (custom_header.version == 1)
	{
		image_fread(image, sm->id, 3);
		image_fread(image, &sm->mp_opcode, 1);
		image_fread(image, sm->data_uid_ptr, 256 + 16);
	}
	image_fread(image, sm->data_ptr, sm->page_total_size*sm->num_pages);

	return INIT_PASS;
}

/*
    Unload a SmartMedia image
*/
static DEVICE_IMAGE_UNLOAD( smartmedia )
{
	smartmedia_t *sm = get_safe_token(image);

	sm->page_data_size = 0;
	sm->page_total_size = 0;
	sm->num_pages = 0;
	sm->log2_pages_per_block = 0;
	sm->data_ptr = NULL;
	sm->data_uid_ptr = NULL;
	sm->mode = SM_M_INIT;
	sm->pointer_mode = SM_PM_A;
	sm->page_addr = 0;
	sm->byte_addr = 0;
	sm->status = 0x40;
	sm->accumulated_status = 0;
	sm->pagereg = auto_alloc_array(image->machine, UINT8, sm->page_total_size);
	sm->id[0] = sm->id[1] = sm->id[2] = 0;
	sm->mp_opcode = 0;
	sm->mode_3065 = 0;

	return;
}

int smartmedia_present(const device_config *device)
{
	smartmedia_t *sm = get_safe_token(device);
	return sm->num_pages != 0;
}

int smartmedia_protected(const device_config *device)
{
	smartmedia_t *sm = get_safe_token(device);
	return (sm->status & 0x80) != 0;
}

/*
    write a byte to SmartMedia command port
*/
void smartmedia_command_w(const device_config *device, UINT8 data)
{
	smartmedia_t *sm = get_safe_token(device);

	if (!smartmedia_present(device))
		return;

	switch (data)
	{
	case 0xff:
		sm->mode = SM_M_INIT;
		sm->pointer_mode = SM_PM_A;
		sm->status = (sm->status & 0x80) | 0x40;
		sm->accumulated_status = 0;
		sm->mode_3065 = 0;
		break;
	case 0x00:
		sm->mode = SM_M_READ;
		sm->pointer_mode = SM_PM_A;
		sm->page_addr = 0;
		sm->addr_load_ptr = 0;
		break;
	case 0x01:
		if (sm->page_data_size <= 256)
		{
			logerror("smartmedia: unsupported upper data field select (256-byte pages)\n");
			sm->mode = SM_M_INIT;
		}
		else
		{
			sm->mode = SM_M_READ;
			sm->pointer_mode = SM_PM_B;
			sm->page_addr = 0;
			sm->addr_load_ptr = 0;
		}
		break;
	case 0x50:
		sm->mode = SM_M_READ;
		sm->pointer_mode = SM_PM_C;
		sm->page_addr = 0;
		sm->addr_load_ptr = 0;
		break;
	case 0x80:
		sm->mode = SM_M_PROGRAM;
		sm->page_addr = 0;
		sm->addr_load_ptr = 0;
		memset(sm->pagereg, 0xff, sm->page_total_size);
		break;
	case 0x10:
	case 0x15:
		if (sm->mode != SM_M_PROGRAM)
		{
			logerror("smartmedia: illegal page program confirm command\n");
			sm->mode = SM_M_INIT;
		}
		else
		{
			int i;
			sm->status = (sm->status & 0x80) | sm->accumulated_status;
			for (i=0; i<sm->page_total_size; i++)
				sm->data_ptr[sm->page_addr*sm->page_total_size + i] &= sm->pagereg[i];
			sm->status |= 0x40;
			if (data == 0x15)
				sm->accumulated_status = sm->status & 0x1f;
			else
				sm->accumulated_status = 0;
			sm->mode = SM_M_INIT;
		}
		break;
	/*case 0x11:
        break;*/
	case 0x60:
		sm->mode = SM_M_ERASE;
		sm->page_addr = 0;
		sm->addr_load_ptr = 0;
		break;
	case 0xd0:
		if (sm->mode != SM_M_PROGRAM)
		{
			logerror("smartmedia: illegal block erase confirm command\n");
			sm->mode = SM_M_INIT;
		}
		else
		{
			sm->status &= 0x80;
			memset(sm->data_ptr + (sm->page_addr & (-1 << sm->log2_pages_per_block)), 0, (size_t)(1 << sm->log2_pages_per_block));
			sm->status |= 0x40;
			sm->mode = SM_M_INIT;
			if (sm->pointer_mode == SM_PM_B)
				sm->pointer_mode = SM_PM_A;
		}
		break;
	case 0x70:
		sm->mode = SM_M_READSTATUS;
		break;
	/*case 0x71:
        break;*/
	case 0x90:
		sm->mode = SM_M_READID;
		sm->addr_load_ptr = 0;
		break;
	/*case 0x91:
        break;*/
	case 0x30:
		sm->mode = SM_M_30;
		break;
	case 0x65:
		if (sm->mode != SM_M_30)
		{
			logerror("smartmedia: unexpected address port write\n");
			sm->mode = SM_M_INIT;
		}
		else
		{
			sm->mode_3065 = 1;
		}
		break;
	default:
		logerror("smartmedia: unsupported command 0x%02x\n", data);
		sm->mode = SM_M_INIT;
		break;
	}
}

/*
    write a byte to SmartMedia address port
*/
void smartmedia_address_w(const device_config *device, UINT8 data)
{
	smartmedia_t *sm = get_safe_token(device);

	if (!smartmedia_present(device))
		return;

	switch (sm->mode)
	{
	case SM_M_INIT:
		logerror("smartmedia: unexpected address port write\n");
		break;
	case SM_M_READ:
	case SM_M_PROGRAM:
		if (sm->addr_load_ptr == 0)
		{
			switch (sm->pointer_mode)
			{
			case SM_PM_A:
				sm->byte_addr = data;
				break;
			case SM_PM_B:
				sm->byte_addr = data + 256;
				sm->pointer_mode = SM_PM_A;
				break;
			case SM_PM_C:
				if (!sm->mode_3065)
					sm->byte_addr = (data & 0x0f) + sm->page_data_size;
				else
					sm->byte_addr = (data & 0x0f) + 256;
				break;
			}
		}
		else
			sm->page_addr = (sm->page_addr & ~(0xff << ((sm->addr_load_ptr-1) * 8)))
										| (data << ((sm->addr_load_ptr-1) * 8));
		sm->addr_load_ptr++;
		break;
	case SM_M_ERASE:
		sm->page_addr = (sm->page_addr & ~(0xff << (sm->addr_load_ptr * 8)))
									| (data << (sm->addr_load_ptr * 8));
		sm->addr_load_ptr++;
		break;
	case SM_M_READSTATUS:
	case SM_M_30:
		logerror("smartmedia: unexpected address port write\n");
		break;
	case SM_M_READID:
		if (sm->addr_load_ptr == 0)
			sm->byte_addr = data;
		sm->addr_load_ptr++;
		break;
	}
}

/*
    read a byte from SmartMedia data port
*/
UINT8 smartmedia_data_r(const device_config *device)
{
	UINT8 reply = 0;
	smartmedia_t *sm = get_safe_token(device);

	if (!smartmedia_present(device))
		return 0;

	switch (sm->mode)
	{
	case SM_M_INIT:
	case SM_M_30:
		logerror("smartmedia: unexpected data port read\n");
		break;
	case SM_M_READ:
		if (!sm->mode_3065)
			reply = sm->data_ptr[sm->page_addr*sm->page_total_size + sm->byte_addr];
		else
			reply = sm->data_uid_ptr[sm->page_addr*sm->page_total_size + sm->byte_addr];
		sm->byte_addr++;
		if (sm->byte_addr == sm->page_total_size)
		{
			sm->byte_addr = (sm->pointer_mode != SM_PM_C) ? 0 : sm->page_data_size;
			sm->page_addr++;
			if (sm->page_addr == sm->num_pages)
				sm->page_addr = 0;
		}
		break;
	case SM_M_PROGRAM:
		logerror("smartmedia: unexpected data port read\n");
		break;
	case SM_M_ERASE:
		logerror("smartmedia: unexpected data port read\n");
		break;
	case SM_M_READSTATUS:
		reply = sm->status & 0xc1;
		break;
	case SM_M_READID:
		if (sm->byte_addr < 3)
			reply = sm->id[sm->byte_addr];
		sm->byte_addr++;
		break;
	}

	return reply;
}

/*
    write a byte to SmartMedia data port
*/
void smartmedia_data_w(const device_config *device, UINT8 data)
{
	smartmedia_t *sm = get_safe_token(device);

	if (!smartmedia_present(device))
		return;

	switch (sm->mode)
	{
	case SM_M_INIT:
	case SM_M_READ:
	case SM_M_30:
		logerror("smartmedia: unexpected data port write\n");
		break;
	case SM_M_PROGRAM:
		sm->pagereg[sm->byte_addr] = data;
		sm->byte_addr++;
		if (sm->byte_addr == sm->page_total_size)
			sm->byte_addr = (sm->pointer_mode != SM_PM_C) ? 0 : sm->page_data_size;
		break;
	case SM_M_ERASE:
	case SM_M_READSTATUS:
	case SM_M_READID:
		logerror("smartmedia: unexpected data port write\n");
		break;
	}
}


/*
    Initialize one SmartMedia chip: may be called at driver init or image load
    time (or machine init time if you don't use MESS image core)
*/
static DEVICE_RESET(smartmedia)
{
	smartmedia_t *sm = get_safe_token(device);

	sm->mode = SM_M_INIT;
	sm->pointer_mode = SM_PM_A;
	sm->status = (sm->status & 0x80) | 0x40;
	sm->accumulated_status = 0;
}

DEVICE_GET_INFO( smartmedia )
{
	switch ( state )
	{
		case DEVINFO_INT_CLASS:	                    info->i = DEVICE_CLASS_PERIPHERAL;                         break;
		case DEVINFO_INT_TOKEN_BYTES:				info->i = sizeof(smartmedia_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:		info->i = 0;
		case DEVINFO_INT_IMAGE_TYPE:	            info->i = IO_MEMCARD;                                      break;
		case DEVINFO_INT_IMAGE_READABLE:            info->i = 1;                                               break;
		case DEVINFO_INT_IMAGE_WRITEABLE:			info->i = 1;                                               break;
		case DEVINFO_INT_IMAGE_CREATABLE:	     	info->i = 0;                                               break;
		case DEVINFO_FCT_START:		                info->start = DEVICE_START_NAME( smartmedia );              break;
		case DEVINFO_FCT_RESET:						info->reset = DEVICE_RESET_NAME( smartmedia );			break;
		case DEVINFO_FCT_IMAGE_LOAD:		        info->f = (genf *) DEVICE_IMAGE_LOAD_NAME( smartmedia );    break;
		case DEVINFO_FCT_IMAGE_UNLOAD:		        info->f = (genf *) DEVICE_IMAGE_UNLOAD_NAME(smartmedia );  break;
		case DEVINFO_STR_NAME:		                strcpy( info->s, "SmartMedia Flash ROM");	                         break;
		case DEVINFO_STR_FAMILY:                    strcpy(info->s, "SmartMedia Flash ROM");	                         break;
		case DEVINFO_STR_SOURCE_FILE:		        strcpy(info->s, __FILE__);                                        break;
		case DEVINFO_STR_IMAGE_FILE_EXTENSIONS:	    strcpy(info->s, "");                                           break;
	}
}
