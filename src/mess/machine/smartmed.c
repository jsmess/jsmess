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

/*INLINE void set_UINT32BE(UINT32BE *word, UINT32 data)
{
	word->bytes[0] = (data >> 24) & 0xff;
	word->bytes[1] = (data >> 16) & 0xff;
	word->bytes[2] = (data >> 8) & 0xff;
	word->bytes[3] = data & 0xff;
}*/

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

static struct
{
	int page_data_size;	// 256 for a 2MB card, 512 otherwise
	int page_total_size;// 264 for a 2MB card, 528 otherwise
	int num_pages;		// 8192 for a 4MB card, 16184 for 8MB, 32768 for 16MB,
						// 65536 for 32MB, 131072 for 64MB, 262144 for 128MB...
						// 0 means no card loaded
	int log2_pages_per_block;	// log2 of number of pages per erase block (usually 4 or 5)

	UINT8 *data_ptr;	// FEEPROM data area

	enum
	{
		SM_M_INIT,		// initial state
		SM_M_READ,		// read page data
		SM_M_PROGRAM,	// program page data
		SM_M_ERASE,		// erase block data
		SM_M_READSTATUS,// read status
		SM_M_READID		// read ID
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
	UINT8 id[2];		// chip ID
	UINT8 mp_opcode;	// multi-plane operation code
} smartmedia[MAX_SMARTMEDIA];

/*
	Init a SmartMedia image
*/
DEVICE_INIT( smartmedia )
{
	int id = image_index_in_device(image);

	if (id >= MAX_SMARTMEDIA)
	{
		logerror("smartmedia: invalid chip %d\n", id);
		return INIT_FAIL;
	}

	smartmedia[id].page_data_size = 0;
	smartmedia[id].page_total_size = 0;
	smartmedia[id].num_pages = 0;
	smartmedia[id].log2_pages_per_block = 0;
	smartmedia[id].data_ptr = NULL;
	smartmedia[id].mode = SM_M_INIT;
	smartmedia[id].pointer_mode = SM_PM_A;
	smartmedia[id].page_addr = 0;
	smartmedia[id].byte_addr = 0;
	smartmedia[id].status = 0x40;
	smartmedia[id].accumulated_status = 0;
	smartmedia[id].pagereg = NULL;
	smartmedia[id].id[0] = smartmedia[id].id[1] = 0;
	smartmedia[id].mp_opcode = 0;

	return INIT_PASS;
}

/*
	Load a SmartMedia image
*/
DEVICE_LOAD( smartmedia )
{
	int id = image_index_in_device(image);
	disk_image_header custom_header;
	int bytes_read;

	if (id >= MAX_SMARTMEDIA)
	{
		logerror("smartmedia: invalid chip %d\n", id);
		return INIT_FAIL;
	}

	/*if (device_load_mess_hd(image, file))
		return INIT_FAIL;*/

	//mess_hd_get_hard_disk_file(image)

	bytes_read = image_fread(image, &custom_header, sizeof(custom_header));
	if (bytes_read != sizeof(custom_header))
	{
		return INIT_FAIL;
	}

	if (custom_header.version != 0)
	{
		return INIT_FAIL;
	}

	smartmedia[id].page_data_size = get_UINT32BE(custom_header.page_data_size);
	smartmedia[id].page_total_size = get_UINT32BE(custom_header.page_total_size);
	smartmedia[id].num_pages = get_UINT32BE(custom_header.num_pages);
	smartmedia[id].log2_pages_per_block = get_UINT32BE(custom_header.log2_pages_per_block);
	smartmedia[id].data_ptr = auto_malloc(smartmedia[id].page_total_size*smartmedia[id].num_pages);
	smartmedia[id].mode = SM_M_INIT;
	smartmedia[id].pointer_mode = SM_PM_A;
	smartmedia[id].page_addr = 0;
	smartmedia[id].byte_addr = 0;
	smartmedia[id].status = 0x40;
	if (!image_is_writable(image))
		smartmedia[id].status |= 0x80;
	smartmedia[id].accumulated_status = 0;
	smartmedia[id].pagereg = auto_malloc(smartmedia[id].page_total_size);
	smartmedia[id].id[0] = smartmedia[id].id[1] = 0;

	image_fread(image, smartmedia[id].id, 2);
	image_fread(image, &smartmedia[id].mp_opcode, 1);
	image_fread(image, smartmedia[id].data_ptr, smartmedia[id].page_total_size*smartmedia[id].num_pages);

	return INIT_PASS;
}

/*
	Unload a SmartMedia image
*/
DEVICE_UNLOAD( smartmedia )
{
	int id = image_index_in_device(image);

	if (id >= MAX_SMARTMEDIA)
	{
		logerror("smartmedia: invalid chip %d\n", id);
		return;
	}

	//device_unload_mess_hd(image);

	smartmedia[id].page_data_size = 0;
	smartmedia[id].page_total_size = 0;
	smartmedia[id].num_pages = 0;
	smartmedia[id].log2_pages_per_block = 0;
	smartmedia[id].data_ptr = NULL;
	smartmedia[id].mode = SM_M_INIT;
	smartmedia[id].pointer_mode = SM_PM_A;
	smartmedia[id].page_addr = 0;
	smartmedia[id].byte_addr = 0;
	smartmedia[id].status = 0x40;
	smartmedia[id].accumulated_status = 0;
	smartmedia[id].pagereg = auto_malloc(smartmedia[id].page_total_size);
	smartmedia[id].id[0] = smartmedia[id].id[1] = 0;
	smartmedia[id].mp_opcode = 0;

	return;
}

/*
	Initialize one SmartMedia chip: may be called at driver init or image load
	time (or machine init time if you don't use MESS image core)
*/
int smartmedia_machine_init(int id)
{
	if (id >= MAX_SMARTMEDIA)
	{
		logerror("smartmedia: invalid chip %d\n", id);
		return 1;
	}

	smartmedia[id].mode = SM_M_INIT;
	smartmedia[id].pointer_mode = SM_PM_A;
	smartmedia[id].status = (smartmedia[id].status & 0x80) | 0x40;
	smartmedia[id].accumulated_status = 0;

	return 0;
}

int smartmedia_present(int id)
{
	return smartmedia[id].num_pages != 0;
}

int smartmedia_protected(int id)
{
	return (smartmedia[id].status & 0x80) != 0;
}

/*
	write a byte to SmartMedia command port
*/
void smartmedia_command_w(int id, UINT8 data)
{
	if (id >= MAX_SMARTMEDIA)
	{
		logerror("smartmedia: invalid chip %d\n", id);
		return;
	}

	if (!smartmedia_present(id))
		return;

	switch (data)
	{
	case 0xff:
		smartmedia[id].mode = SM_M_INIT;
		smartmedia[id].pointer_mode = SM_PM_A;
		smartmedia[id].status = (smartmedia[id].status & 0x80) | 0x40;
		smartmedia[id].accumulated_status = 0;
		break;
	case 0x00:
		smartmedia[id].mode = SM_M_READ;
		smartmedia[id].pointer_mode = SM_PM_A;
		smartmedia[id].page_addr = 0;
		smartmedia[id].addr_load_ptr = 0;
		break;
	case 0x01:
		if (smartmedia[id].page_data_size <= 256)
		{
			logerror("smartmedia: unsupported upper data field select (256-byte pages)\n");
			smartmedia[id].mode = SM_M_INIT;
		}
		else
		{
			smartmedia[id].mode = SM_M_READ;
			smartmedia[id].pointer_mode = SM_PM_B;
			smartmedia[id].page_addr = 0;
			smartmedia[id].addr_load_ptr = 0;
		}
		break;
	case 0x50:
		smartmedia[id].mode = SM_M_READ;
		smartmedia[id].pointer_mode = SM_PM_C;
		smartmedia[id].page_addr = 0;
		smartmedia[id].addr_load_ptr = 0;
		break;
	case 0x80:
		smartmedia[id].mode = SM_M_PROGRAM;
		smartmedia[id].page_addr = 0;
		smartmedia[id].addr_load_ptr = 0;
		memset(smartmedia[id].pagereg, 0xff, smartmedia[id].page_total_size);
		break;
	case 0x10:
	case 0x15:
		if (smartmedia[id].mode != SM_M_PROGRAM)
		{
			logerror("smartmedia: illegal page program confirm command\n");
			smartmedia[id].mode = SM_M_INIT;
		}
		else
		{
			int i;
			smartmedia[id].status = (smartmedia[id].status & 0x80) | smartmedia[id].accumulated_status;
			for (i=0; i<smartmedia[id].page_total_size; i++)
				smartmedia[id].data_ptr[smartmedia[id].page_addr*smartmedia[id].page_total_size + i] &= smartmedia[id].pagereg[i];
			smartmedia[id].status |= 0x40;
			if (data == 0x15)
				smartmedia[id].accumulated_status = smartmedia[id].status & 0x1f;
			else
				smartmedia[id].accumulated_status = 0;
			smartmedia[id].mode = SM_M_INIT;
		}
		break;
	/*case 0x11:
		break;*/
	case 0x60:
		smartmedia[id].mode = SM_M_ERASE;
		smartmedia[id].page_addr = 0;
		smartmedia[id].addr_load_ptr = 0;
		break;
	case 0xd0:
		if (smartmedia[id].mode != SM_M_PROGRAM)
		{
			logerror("smartmedia: illegal block erase confirm command\n");
			smartmedia[id].mode = SM_M_INIT;
		}
		else
		{
			smartmedia[id].status &= 0x80;
			memset(smartmedia[id].data_ptr + (smartmedia[id].page_addr & (-1 << smartmedia[id].log2_pages_per_block)), 0, 1 << smartmedia[id].log2_pages_per_block);
			smartmedia[id].status |= 0x40;
			smartmedia[id].mode = SM_M_INIT;
			if (smartmedia[id].pointer_mode == SM_PM_B)
				smartmedia[id].pointer_mode = SM_PM_A;
		}
		break;
	case 0x70:
		smartmedia[id].mode = SM_M_READSTATUS;
		break;
	/*case 0x71:
		break;*/
	case 0x90:
		smartmedia[id].mode = SM_M_READID;
		break;
	/*case 0x91:
		break;*/
	default:
		logerror("smartmedia: unsupported command 0x%02x\n", data);
		smartmedia[id].mode = SM_M_INIT;
		break;
	}
}

/*
	write a byte to SmartMedia address port
*/
void smartmedia_address_w(int id, UINT8 data)
{
	if (id >= MAX_SMARTMEDIA)
	{
		logerror("smartmedia: invalid chip %d\n", id);
		return;
	}

	if (!smartmedia_present(id))
		return;

	switch (smartmedia[id].mode)
	{
	case SM_M_INIT:
		logerror("smartmedia: unexpected address port write\n");
		break;
	case SM_M_READ:
	case SM_M_PROGRAM:
		if (smartmedia[id].addr_load_ptr == 0)
		{
			switch (smartmedia[id].pointer_mode)
			{
			case SM_PM_A:
				smartmedia[id].byte_addr = data;
				break;
			case SM_PM_B:
				smartmedia[id].byte_addr = data + 256;
				smartmedia[id].pointer_mode = SM_PM_A;
				break;
			case SM_PM_C:
				smartmedia[id].byte_addr = (data & 0x0f) + smartmedia[id].page_data_size;
				break;
			}
		}
		else
			smartmedia[id].page_addr = (smartmedia[id].page_addr & ~(0xff << ((smartmedia[id].addr_load_ptr-1) * 8)))
										| (data << ((smartmedia[id].addr_load_ptr-1) * 8));
		smartmedia[id].addr_load_ptr++;
		break;
	case SM_M_ERASE:
		smartmedia[id].page_addr = (smartmedia[id].page_addr & ~(0xff << (smartmedia[id].addr_load_ptr * 8)))
									| (data << (smartmedia[id].addr_load_ptr * 8));
		smartmedia[id].addr_load_ptr++;
		break;
	case SM_M_READSTATUS:
		logerror("smartmedia: unexpected address port write\n");
		break;
	case SM_M_READID:
		if (smartmedia[id].addr_load_ptr == 0)
			smartmedia[id].byte_addr = data;
		smartmedia[id].addr_load_ptr++;
		break;
	}
}

/*
	read a byte from SmartMedia data port
*/
UINT8 smartmedia_data_r(int id)
{
	UINT8 reply = 0;

	if (id >= MAX_SMARTMEDIA)
	{
		logerror("smartmedia: invalid chip %d\n", id);
		return 0;
	}

	if (!smartmedia_present(id))
		return 0;

	switch (smartmedia[id].mode)
	{
	case SM_M_INIT:
		logerror("smartmedia: unexpected data port read\n");
		break;
	case SM_M_READ:
		reply = smartmedia[id].data_ptr[smartmedia[id].page_addr*smartmedia[id].page_total_size + smartmedia[id].byte_addr];
		smartmedia[id].byte_addr++;
		if (smartmedia[id].byte_addr == smartmedia[id].page_total_size)
		{
			smartmedia[id].byte_addr = (smartmedia[id].pointer_mode != SM_PM_C) ? 0 : smartmedia[id].page_data_size;
			smartmedia[id].page_addr++;
			if (smartmedia[id].page_addr == smartmedia[id].num_pages)
				smartmedia[id].page_addr = 0;
		}
		break;
	case SM_M_PROGRAM:
		logerror("smartmedia: unexpected data port read\n");
		break;
	case SM_M_ERASE:
		logerror("smartmedia: unexpected data port read\n");
		break;
	case SM_M_READSTATUS:
		reply = smartmedia[id].status & 0xc1;
		break;
	case SM_M_READID:
		if (smartmedia[id].byte_addr < 2)
			reply = smartmedia[id].id[smartmedia[id].byte_addr];
		break;
	}

	return reply;
}

/*
	write a byte to SmartMedia data port
*/
void smartmedia_data_w(int id, UINT8 data)
{
	if (id >= MAX_SMARTMEDIA)
	{
		logerror("smartmedia: invalid chip %d\n", id);
		return;
	}

	if (!smartmedia_present(id))
		return;

	switch (smartmedia[id].mode)
	{
	case SM_M_INIT:
	case SM_M_READ:
		logerror("smartmedia: unexpected data port write\n");
		break;
	case SM_M_PROGRAM:
		smartmedia[id].pagereg[smartmedia[id].byte_addr] = data;
		smartmedia[id].byte_addr++;
		if (smartmedia[id].byte_addr == smartmedia[id].page_total_size)
			smartmedia[id].byte_addr = (smartmedia[id].pointer_mode != SM_PM_C) ? 0 : smartmedia[id].page_data_size;
		break;
	case SM_M_ERASE:
	case SM_M_READSTATUS:
	case SM_M_READID:
		logerror("smartmedia: unexpected data port write\n");
		break;
	}
}
