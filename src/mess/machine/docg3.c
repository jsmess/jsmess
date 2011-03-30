/*

    M-Systems DiskOnChip G3 - Flash Disk with MLC NAND and M-Systems? x2 Technology

    (c) 2009 Tim Schuerewegen

*/

#include "emu.h"
#include "machine/docg3.h"

#define VERBOSE_LEVEL ( 0 )

INLINE void ATTR_PRINTF(3,4) verboselog( running_machine &machine, int n_level, const char *s_fmt, ... )
{
	if( VERBOSE_LEVEL >= n_level )
	{
		va_list v;
		char buf[ 32768 ];
		va_start( v, s_fmt );
		vsprintf( buf, s_fmt, v );
		va_end( v );
		logerror( "%s: %s", machine.describe_context(), buf );
	}
}

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _diskonchip_g3_t diskonchip_g3_t;
struct _diskonchip_g3_t
{
	UINT32 planes;
	UINT32 blocks;
	UINT32 pages;
	UINT32 user_data_size;
	UINT32 extra_area_size;
	UINT8 *data[3];
	UINT32 data_size[3];
	UINT8 sec_2[0x800];
	UINT32 data_1036;
	UINT32 data_1036_count;
	UINT32 transfer_offset;
	UINT8 device;
	UINT32 block;
	UINT32 page;
	UINT32 plane;
	UINT32 transfersize;
	UINT8 test;
	UINT32 address_count;
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

static DEVICE_NVRAM( diskonchip_g3 );

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE diskonchip_g3_t *get_token( device_t *device)
{
	assert(device != NULL);
	assert(device->type() == DISKONCHIP_G3);

	return (diskonchip_g3_t *)downcast<legacy_device_base *>(device)->token();
}

INLINE const diskonchip_g3_config *get_config( device_t *device)
{
	assert(device != NULL);
	assert(device->type() == DISKONCHIP_G3);
	assert(downcast<const legacy_device_config_base &>(device->baseconfig()).inline_config() != NULL);
	return (const diskonchip_g3_config *)downcast<const legacy_device_config_base &>(device->baseconfig()).inline_config();
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

static UINT32 diskonchip_g3_offset_data_1( device_t *device)
{
	diskonchip_g3_t *doc = get_token( device);
//  printf( "block %d pages %d page %d planes %d plane %d user_data_size %d extra_area_size %d\n", doc->block, doc->pages, doc->page, doc->planes, doc->plane, doc->user_data_size, doc->extra_area_size);
	return ((((doc->block * doc->pages) + doc->page) * doc->planes) + doc->plane) * (doc->user_data_size + doc->extra_area_size);
}

static UINT32 diskonchip_g3_offset_data_2( device_t *device)
{
	diskonchip_g3_t *doc = get_token( device);
	return ((((doc->block * doc->pages) + doc->page) * doc->planes) + doc->plane) * 16;
}

static UINT32 diskonchip_g3_offset_data_3( device_t *device)
{
	diskonchip_g3_t *doc = get_token( device);
	return doc->block * 8;
}

static UINT8 diskonchip_g3_read_data( device_t *device)
{
	diskonchip_g3_t *doc = get_token( device);
	UINT8 data = 0;
	UINT32 offset;
	if (doc->test == 0)
	{
		// read page data (512 + 16)
		if (doc->transfer_offset >= (doc->user_data_size + doc->extra_area_size))
		{
			doc->transfer_offset = doc->transfer_offset - (doc->user_data_size + doc->extra_area_size);
			doc->plane++;
		}
		offset = diskonchip_g3_offset_data_1( device) + doc->transfer_offset;
		data = doc->data[0][offset];
	}
	else if (doc->test == 1)
	{
		// read ??? (0x28 bytes)
		if (doc->transfer_offset < 0x20)
		{
			offset = diskonchip_g3_offset_data_1( device) + (doc->user_data_size + doc->extra_area_size) + (doc->user_data_size - 16) + doc->transfer_offset;
			data = doc->data[0][offset];
		}
		else if (doc->transfer_offset < 0x28)
		{
			offset = diskonchip_g3_offset_data_3( device) + doc->transfer_offset - 0x20;
			data = doc->data[2][offset];
		}
		else
		{
			data = 0xFF;
		}
	}
	else if (doc->test == 2)
	{
		// read erase block status
		data = 0x00;
	}
	doc->transfer_offset++;
	return data;
}

READ16_DEVICE_HANDLER( diskonchip_g3_sec_1_r )
{
	diskonchip_g3_t *doc = get_token( device);
	UINT16 data;
	if (doc->sec_2[0x1B] & 0x40)
	{
		data = diskonchip_g3_read_data( device);
		doc->sec_2[0x1B] &= ~0x40;
	}
	else
	{
		data = (diskonchip_g3_read_data( device) << 0) | (diskonchip_g3_read_data( device) << 8);
	}
	verboselog( device->machine(), 9, "(DOC) %08X -> %04X\n", 0x0800 + (offset << 1), data);
	return data;
}

static void diskonchip_g3_write_data( device_t *device, UINT8 data)
{
	diskonchip_g3_t *doc = get_token( device);
	UINT32 offset;
	if (doc->test == 3)
	{
		// read page data (512 + 16)
		if (doc->transfer_offset >= (doc->user_data_size + doc->extra_area_size))
		{
			doc->transfer_offset = doc->transfer_offset - (doc->user_data_size + doc->extra_area_size);
			doc->plane++;
		}
		if (doc->transfer_offset == 0)
		{
			const UINT8 xxx[] = { 0x00, 0x00, 0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
			offset = diskonchip_g3_offset_data_2( device);
			memcpy( doc->data[1] + offset, xxx, 16);
		}
		offset = diskonchip_g3_offset_data_1( device) + doc->transfer_offset;
		doc->data[0][offset] = data;
	}
	doc->transfer_offset++;
}

WRITE16_DEVICE_HANDLER( diskonchip_g3_sec_1_w )
{
	diskonchip_g3_t *doc = get_token( device);
	verboselog( device->machine(), 9, "(DOC) %08X <- %04X\n", 0x0800 + (offset << 1), data);
	if (doc->sec_2[0x1B] & 0x40)
	{
		diskonchip_g3_write_data( device, data);
		doc->sec_2[0x1B] &= ~0x40;
	}
	else
	{
		diskonchip_g3_write_data( device, (data >> 0) & 0xFF);
		diskonchip_g3_write_data( device, (data >> 8) & 0xFF);
	}
}

// #define DoC_G3_IO 0x0800
// #define DoC_G3_ChipID 0x1000
// #define DoC_G3_DeviceIdSelect 0x100a
// #define DoC_G3_Ctrl 0x100c
// #define DoC_G3_CtrlConfirm 0x1072
// #define DoC_G3_ReadAddress 0x101a
// #define DoC_G3_FlashSelect 0x1032
// #define DoC_G3_FlashCmd 0x1034
// #define DoC_G3_FlashAddr 0x1036
// #define DoC_G3_FlashCtrl 0x1038
// #define DoC_G3_Nop 0x103e

static UINT16 diskonchip_sec_2_read_1000( device_t *device)
{
	diskonchip_g3_t *doc = get_token( device);
	if (doc->device == 0)
	{
		return 0x0200;
	}
	else
	{
		return 0x008C;
	}
}

static UINT16 diskonchip_sec_2_read_1074( device_t *device)
{
	diskonchip_g3_t *doc = get_token( device);
	if (doc->device == 0)
	{
		return 0xFDFF;
	}
	else
	{
		return 0x02B3;
	}
}

static UINT8 diskonchip_sec_2_read_1042( device_t *device)
{
	diskonchip_g3_t *doc = get_token( device);
	UINT8 data;
	if ((doc->block == 0) && (doc->page == 0) && (doc->transfersize == 0x27))
	{
		data = 0x07;
	}
	else
	{
		UINT32 offset = diskonchip_g3_offset_data_2( device) + 0x02;
		data = doc->data[1][offset]; //0x07; //data & ~(1 << 7);
		if ((doc->sec_2[0x41] & 0x10) == 0)
		{
			data = data & ~0x20;
		}
		if ((doc->sec_2[0x41] & 0x08) == 0)
		{
			data = data & ~0x80;
		}
		if (doc->test != 0)
		{
			data = 0x00;
		}
	}
	return data;
}

static UINT8 diskonchip_sec_2_read_1046( device_t *device)
{
	diskonchip_g3_t *doc = get_token( device);
	if (doc->test == 0)
	{
		UINT32 offset = diskonchip_g3_offset_data_2( device) + 0x06;
		return doc->data[1][offset];
	}
	else
	{
		return 0x00;
	}
}

static UINT8 diskonchip_sec_2_read_1048( device_t *device)
{
	diskonchip_g3_t *doc = get_token( device);
	if (doc->test == 0)
	{
		UINT32 offset = diskonchip_g3_offset_data_2( device) + 0x08;
		return doc->data[1][offset];
	}
	else
	{
		return 0x00;
	}
}

static UINT8 diskonchip_sec_2_read_1049( device_t *device)
{
	diskonchip_g3_t *doc = get_token( device);
	if (doc->test == 0)
	{
		UINT32 offset = diskonchip_g3_offset_data_2( device) + 0x09;
		return doc->data[1][offset];
	}
	else
	{
		return 0x00;
	}
}

static UINT8 diskonchip_sec_2_read_104A( device_t *device)
{
	diskonchip_g3_t *doc = get_token( device);
	if (doc->test == 0)
	{
		UINT32 offset = diskonchip_g3_offset_data_2( device) + 0x0A;
		return doc->data[1][offset];
	}
	else
	{
		return 0x00;
	}
}

static UINT8 diskonchip_sec_2_read_104B( device_t *device)
{
	diskonchip_g3_t *doc = get_token( device);
	if (doc->test == 0)
	{
		UINT32 offset = diskonchip_g3_offset_data_2( device) + 0x0B;
		return doc->data[1][offset];
	}
	else
	{
		return 0x00;
	}
}

static UINT8 diskonchip_sec_2_read_104C( device_t *device)
{
	diskonchip_g3_t *doc = get_token( device);
	if (doc->test == 0)
	{
		UINT32 offset = diskonchip_g3_offset_data_2( device) + 0x0C;
		return doc->data[1][offset];
	}
	else
	{
		return 0x00;
	}
}

static UINT8 diskonchip_sec_2_read_104D( device_t *device)
{
	diskonchip_g3_t *doc = get_token( device);
	if (doc->test == 0)
	{
		UINT32 offset = diskonchip_g3_offset_data_2( device) + 0x0D;
		return doc->data[1][offset];
	}
	else
	{
		return 0x00;
	}
}

static UINT8 diskonchip_sec_2_read_104E( device_t *device)
{
	diskonchip_g3_t *doc = get_token( device);
	if (doc->test == 0)
	{
		UINT32 offset = diskonchip_g3_offset_data_2( device) + 0x0E;
		return doc->data[1][offset];
	}
	else
	{
		return 0x00;
	}
}

static UINT8 diskonchip_sec_2_read_104F( device_t *device)
{
	diskonchip_g3_t *doc = get_token( device);
	if (doc->test == 0)
	{
		UINT32 offset = diskonchip_g3_offset_data_2( device) + 0x0F;
		return doc->data[1][offset];
	}
	else
	{
		return 0x00;
	}
}

static UINT8 diskonchip_sec_2_read_100E( device_t *device)
{
	return 0x81;
}

static UINT8 diskonchip_sec_2_read_1014( device_t *device)
{
	return 0x01;
}

static UINT8 diskonchip_sec_2_read_1022( device_t *device)
{
	return 0x85;
}

static UINT8 diskonchip_sec_2_read_1038( device_t *device)
{
	// bit 0 = ?
	// bit 1 = ? error 0x7C
	// bit 2 = ? error 0x73
	// bit 3 = ?
	// bit 4 = ?
	// bit 5 = ?
	// bit 6 = ?
	// bit 7 = ?
	diskonchip_g3_t *doc = get_token( device);
	if (doc->device == 0)
	{
		return 0x19;
	}
	else
	{
		return 0x38;
	}
}

static UINT16 diskonchip_sec_2_read16( device_t *device, UINT32 offset)
{
	diskonchip_g3_t *doc = get_token( device);
	UINT16 data = (doc->sec_2[offset+0] << 0) + (doc->sec_2[offset+1] << 8);
	switch (0x1000 + offset)
	{
		// ?
		case 0x1000 : data = diskonchip_sec_2_read_1000( device); break;
		// ?
		case 0x100E : data = diskonchip_sec_2_read_100E( device); break;
		// ?
		case 0x1014 : data = diskonchip_sec_2_read_1014( device); break;
		// ?
		case 0x1022 : data = diskonchip_sec_2_read_1022( device); break;
		// flash control
		case 0x1038 : data = diskonchip_sec_2_read_1038( device); break;
		// flash ?
		case 0x1042 : data = diskonchip_sec_2_read_1042( device); break;
		// ?
		case 0x1046 : data = diskonchip_sec_2_read_1046( device); break;
		// ?
		case 0x1048 : data = (diskonchip_sec_2_read_1048( device) << 0) + (diskonchip_sec_2_read_1049( device) << 8); break;
		case 0x104A : data = (diskonchip_sec_2_read_104A( device) << 0) + (diskonchip_sec_2_read_104B( device) << 8); break;
		case 0x104C : data = (diskonchip_sec_2_read_104C( device) << 0) + (diskonchip_sec_2_read_104D( device) << 8); break;
		case 0x104E : data = (diskonchip_sec_2_read_104E( device) << 0) + (diskonchip_sec_2_read_104F( device) << 8); break;
		// ?
		case 0x1074 : data = diskonchip_sec_2_read_1074( device); break;
	}
	verboselog( device->machine(), 9, "(DOC) %08X -> %04X\n", 0x1000 + offset, data);
	return data;
}

static UINT8 diskonchip_sec_2_read8( device_t *device, UINT32 offset)
{
	diskonchip_g3_t *doc = get_token( device);
	UINT8 data = doc->sec_2[offset];
	switch (0x1000 + offset)
	{
		// ?
		case 0x100E : data = diskonchip_sec_2_read_100E( device); break;
		// ?
		case 0x1014 : data = diskonchip_sec_2_read_1014( device); break;
		// ?
		case 0x1022 : data = diskonchip_sec_2_read_1022( device); break;
		// flash control
		case 0x1038 : data = diskonchip_sec_2_read_1038( device); break;
		// flash ?
		case 0x1042 : data = diskonchip_sec_2_read_1042( device); break;
		// ?
		case 0x1046 : data = diskonchip_sec_2_read_1046( device); break;
		// ?
		case 0x1048 : data = diskonchip_sec_2_read_1048( device); break;
		case 0x1049 : data = diskonchip_sec_2_read_1049( device); break;
		case 0x104A : data = diskonchip_sec_2_read_104A( device); break;
		case 0x104B : data = diskonchip_sec_2_read_104B( device); break;
		case 0x104C : data = diskonchip_sec_2_read_104C( device); break;
		case 0x104D : data = diskonchip_sec_2_read_104D( device); break;
		case 0x104E : data = diskonchip_sec_2_read_104E( device); break;
		case 0x104F : data = diskonchip_sec_2_read_104F( device); break;
	}
	verboselog( device->machine(), 9, "(DOC) %08X -> %02X\n", 0x1000 + offset, data);
	return data;
}

static void diskonchip_sec_2_write_100C( device_t *device, UINT8 data)
{
	diskonchip_g3_t *doc = get_token( device);
	const char *mode_name[] = { "reset", "normal", "deep power down" };
	UINT32 mode = data & 3;
	verboselog( device->machine(), 5, "mode %d (%s)\n", mode, mode_name[mode]);
	if (mode == 0)
	{
		doc->sec_2[0x04] = 00;
	}
}

static void diskonchip_sec_2_write_1032( device_t *device, UINT8 data)
{
	diskonchip_g3_t *doc = get_token( device);
	verboselog( device->machine(), 5, "flash select %02X\n", data);
	if ((data == 0x12) || (data == 0x27))
	{
		doc->transfer_offset = 0;
		doc->plane = 0;
		doc->block = 0;
		doc->page = 0;
	}
}

static void diskonchip_g3_erase_block( device_t *device)
{
	diskonchip_g3_t *doc = get_token( device);
	UINT32 offset;
	int i, j;
	const UINT8 xxx[] = { 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0xCE, 0x00, 0xCF, 0x72, 0xFC, 0x1B, 0xA9, 0xC7, 0xB9, 0x00 };
	verboselog( device->machine(), 5, "erase block %04X\n", doc->block);
	for (i=0;i<doc->pages;i++)
	{
		doc->page = i;
		for (j=0;j<2;j++)
		{
			doc->plane = j;
			offset = diskonchip_g3_offset_data_1( device);
			memset( doc->data[0] + offset, 0xFF, (doc->user_data_size + doc->extra_area_size));
			offset = diskonchip_g3_offset_data_2( device);
			memcpy( doc->data[1] + offset, xxx, 16);
		}
	}
}

// 'maincpu' (028848E4): diskonchip_sec_2_write_1034: unknown value 60/27
// 'maincpu' (028848E4): diskonchip_sec_2_write_1034: unknown value D0/27
// 'maincpu' (028848E4): diskonchip_sec_2_write_1034: unknown value 71/31
// 'maincpu' (028848E4): diskonchip_sec_2_write_1034: unknown value 80/1D
// 'maincpu' (028848E4): diskonchip_sec_2_write_1034: unknown value 11/1D
// 'maincpu' (028848E4): diskonchip_sec_2_write_1034: unknown value 10/1D

static void diskonchip_sec_2_write_1034( device_t *device, UINT8 data)
{
	diskonchip_g3_t *doc = get_token( device);
	verboselog( device->machine(), 5, "flash command %02X\n", data);
	if ((doc->sec_2[0x32] == 0x0E) && (data == 0x00))
	{
		doc->test = 0;
		doc->address_count = 0;
	}
	else if ((doc->sec_2[0x32] == 0x10) && (data == 0x50))
	{
		doc->test = 1;
		doc->address_count = 0;
	}
	else if ((doc->sec_2[0x32] == 0x03) && (data == 0x3C))
	{
		// do nothing
	}
	else if ((doc->sec_2[0x32] == 0x00) && (data == 0xFF))
	{
		doc->address_count = 0;
	}
	else if (doc->sec_2[0x32] == 0x09)
	{
		if (data == 0x22)
		{
			// do nothing
		}
		else if (data == 0xA2)
		{
			// do nothing
		}
		else
		{
			verboselog( device->machine(), 0, "diskonchip_sec_2_write_1034: unknown value %02X/%02X\n", data, doc->sec_2[0x32]);
		}
	}
	else if (doc->sec_2[0x32] == 0x12)
	{
		if (data == 0x60)
		{
			doc->data_1036 = 0;
			doc->data_1036_count = 0;
			doc->address_count++;
		}
		else if (data == 0x30)
		{
			// do nothing
		}
		else if (data == 0x05)
		{
			// do nothing
		}
		else if (data == 0xE0)
		{
			// do nothing
		}
		else
		{
			verboselog( device->machine(), 0, "diskonchip_sec_2_write_1034: unknown value %02X/%02X\n", data, doc->sec_2[0x32]);
		}
	}
	else if (doc->sec_2[0x32] == 0x27)
	{
		if (data == 0x60)
		{
			doc->data_1036 = 0;
			doc->data_1036_count = 0;
			doc->address_count++;
		}
		else if (data == 0xD0)
		{
			diskonchip_g3_erase_block( device);
		}
		else
		{
			verboselog( device->machine(), 0, "diskonchip_sec_2_write_1034: unknown value %02X/%02X\n", data, doc->sec_2[0x32]);
		}
	}
	else if ((doc->sec_2[0x32] == 0x31) && (data == 0x71))
	{
		// erase block status? (after: read one byte from 08xx, bit 0/1/2 is checked)
		doc->test = 2;
	}
	else if (doc->sec_2[0x32] == 0x1D)
	{
		if (data == 0x80)
		{
			doc->data_1036 = 0;
			doc->data_1036_count = 0;
			doc->address_count++;
		}
		else if (data == 0x11)
		{
			doc->test = 3;
		}
		else
		{
			verboselog( device->machine(), 0, "diskonchip_sec_2_write_1034: unknown value %02X/%02X\n", data, doc->sec_2[0x32]);
		}
	}
	else
	{
		verboselog( device->machine(), 0, "diskonchip_sec_2_write_1034: unknown value %02X/%02X\n", data, doc->sec_2[0x32]);
	}
}

static void diskonchip_sec_2_write_1036( device_t *device, UINT8 data)
{
	diskonchip_g3_t *doc = get_token( device);
	if (doc->sec_2[0x34] == 0x60)
	{
		doc->data_1036 |= data << (8 * doc->data_1036_count++);
		if (doc->data_1036_count == 3)
		{
			UINT32 block, page, plane;
			block = (doc->data_1036 >> 7);
			if (block >= doc->blocks) fatalerror( "DOCG3: invalid block (%d)", block);
			plane = (doc->data_1036 >> 6) & 1;
			page = (doc->data_1036 >> 0) & 0x3F;
			verboselog( device->machine(), 5, "flash address %d - %06X (plane %d block %04X page %04X)\n", doc->address_count, doc->data_1036, plane, block, page);
			if (doc->address_count == 1)
			{
				doc->plane = 0;
				doc->block = block;
				doc->page = page;
			}
		}
	}
	else if (doc->sec_2[0x34] == 0x80)
	{
		doc->data_1036 |= data << (8 * doc->data_1036_count++);
		if (doc->data_1036_count == 4)
		{
			UINT32 block, page, plane, unk;
			block = (doc->data_1036 >> 15);
			plane = (doc->data_1036 >> 14) & 1;
			page = (doc->data_1036 >> 8) & 0x3F;
			unk = (doc->data_1036 >> 0) & 0xFF;
			verboselog( device->machine(), 5, "flash address %d - %08X (plane %d block %04X page %04X unk %02X)\n", doc->address_count, doc->data_1036, plane, block, page, unk);
			if (doc->address_count == 1)
			{
				doc->plane = 0;
				doc->block = block;
				doc->page = page;
				doc->transfer_offset = 0;
			}
		}
	}
	else if (doc->sec_2[0x34] == 0x05)
	{
		doc->transfer_offset = data << 2;
		verboselog( device->machine(), 5, "flash transfer offset %04X\n", doc->transfer_offset);
	}
}

static void diskonchip_sec_2_write_1040( device_t *device, UINT16 data)
{
	diskonchip_g3_t *doc = get_token( device);
	doc->transfersize = (data & 0x3FF);
	verboselog( device->machine(), 5, "flash transfer size %04X\n", doc->transfersize);
}

static void diskonchip_sec_2_write_100A( device_t *device, UINT8 data)
{
	diskonchip_g3_t *doc = get_token( device);
	doc->device = data & 3;
	verboselog( device->machine(), 5, "select device %d\n", doc->device);
}

static void diskonchip_sec_2_write16( device_t *device, UINT32 offset, UINT16 data)
{
	diskonchip_g3_t *doc = get_token( device);
	doc->sec_2[offset+0] = (data >> 0) & 0xFF;
	doc->sec_2[offset+1] = (data >> 8) & 0xFF;
	verboselog( device->machine(), 9, "(DOC) %08X <- %04X\n", 0x1000 + offset, data);
	switch (0x1000 + offset)
	{
		// ?
		case 0x100C : diskonchip_sec_2_write_100C( device, data); break;
		// Device ID Select Register
		case 0x100A : diskonchip_sec_2_write_100A( device, data); break;
		// flash select
		case 0x1032 : diskonchip_sec_2_write_1032( device, data); break;
		// flash command
		case 0x1034 : diskonchip_sec_2_write_1034( device, data); break;
		// flash address
		case 0x1036 : diskonchip_sec_2_write_1036( device, data); break;
		// ?
		case 0x1040 : diskonchip_sec_2_write_1040( device, data); break;
	}
}

static void diskonchip_sec_2_write8( device_t *device, UINT32 offset, UINT8 data)
{
	diskonchip_g3_t *doc = get_token( device);
	doc->sec_2[offset] = data;
	verboselog( device->machine(), 9, "(DOC) %08X <- %02X\n", 0x1000 + offset, data);
	switch (0x1000 + offset)
	{
		// ?
		case 0x100C : diskonchip_sec_2_write_100C( device, data); break;
		// Device ID Select Register
		case 0x100A : diskonchip_sec_2_write_100A( device, data); break;
		// flash select
		case 0x1032 : diskonchip_sec_2_write_1032( device, data); break;
		// flash command
		case 0x1034 : diskonchip_sec_2_write_1034( device, data); break;
		// flash address
		case 0x1036 : diskonchip_sec_2_write_1036( device, data); break;
	}
}

READ16_DEVICE_HANDLER( diskonchip_g3_sec_2_r )
{
	if (mem_mask == 0xffff)
	{
		return diskonchip_sec_2_read16( device, offset * 2);
	}
	else if (mem_mask == 0x00ff)
	{
		return diskonchip_sec_2_read8( device, offset * 2 + 0) << 0;
	}
	else if (mem_mask == 0xff00)
	{
		return diskonchip_sec_2_read8( device, offset * 2 + 1) << 8;
	}
	else
	{
		verboselog( device->machine(), 0, "diskonchip_g3_sec_2_r: unknown mem_mask %08X\n", mem_mask);
		return 0;
	}
}

WRITE16_DEVICE_HANDLER( diskonchip_g3_sec_2_w )
{
	if (mem_mask == 0xffff)
	{
		diskonchip_sec_2_write16( device, offset * 2, data);
	}
	else if (mem_mask == 0x00ff)
	{
		diskonchip_sec_2_write8( device, offset * 2 + 0, (data >> 0) & 0xFF);
	}
	else if (mem_mask == 0xff00)
	{
		diskonchip_sec_2_write8( device, offset * 2 + 1, (data >> 8) & 0xFF);
	}
	else
	{
		verboselog( device->machine(), 0, "diskonchip_g3_sec_2_w: unknown mem_mask %08X\n", mem_mask);
	}
}

READ16_DEVICE_HANDLER( diskonchip_g3_sec_3_r )
{
	UINT16 data = 0;
	verboselog( device->machine(), 9, "(DOC) %08X -> %04X\n", 0x1800 + (offset << 1), data);
	return data;
}

WRITE16_DEVICE_HANDLER( diskonchip_g3_sec_3_w )
{
	verboselog( device->machine(), 9, "(DOC) %08X <- %02X\n", 0x1800 + (offset << 1), data);
}

#if 0

static emu_file *nvram_system_fopen( running_machine &machine, UINT32 openflags, const char *name)
{
	astring *fname;
	file_error filerr;
	emu_file *file;
	fname = astring_assemble_4( astring_alloc(), machine.system().name, PATH_SEPARATOR, name, ".nv");
	filerr = mame_fopen( SEARCHPATH_NVRAM, astring_c( fname), openflags, &file);
	astring_free( fname);
	return (filerr == FILERR_NONE) ? file : NULL;
}

static void diskonchip_load( device_t *device, const char *name)
{
	emu_file *file;
	file = nvram_system_fopen( device->machine(), OPEN_FLAG_READ, name);
	device_nvram_diskonchip_g3( device, file, 0);
	if (file) mame_fclose( file);
}

static void diskonchip_save( device_t *device, const char *name)
{
	emu_file *file;
	file = nvram_system_fopen( device->machine(), OPEN_FLAG_CREATE | OPEN_FLAG_WRITE | OPEN_FLAG_CREATE_PATHS, name);
	device_nvram_diskonchip_g3( device, file, 1);
	if (file) mame_fclose( file);
}

#endif

static DEVICE_START( diskonchip_g3 )
{
	diskonchip_g3_t *doc = get_token( device);
	const diskonchip_g3_config *config = get_config( device);

	verboselog( device->machine(), 9, "(DOC) device start\n");

	switch (config->size)
	{
		case 64 :
		{
			doc->planes = 2;
			doc->blocks = 1024;
			doc->pages = 64;
			doc->user_data_size = 512;
			doc->extra_area_size = 16;
		}
		break;
	}

	doc->data_size[0] = doc->planes * doc->blocks * doc->pages * (doc->user_data_size + doc->extra_area_size);
	doc->data_size[1] = doc->planes * doc->blocks * doc->pages * 16;
	doc->data_size[2] = doc->blocks * 8;

	doc->data[0] = auto_alloc_array( device->machine(), UINT8, doc->data_size[0]);
	doc->data[1] = auto_alloc_array( device->machine(), UINT8, doc->data_size[1]);
	doc->data[2] = auto_alloc_array( device->machine(), UINT8, doc->data_size[2]);

//  diskonchip_load( device, "diskonchip");

	device->save_item( NAME(doc->planes));
	device->save_item( NAME(doc->blocks));
	device->save_item( NAME(doc->pages));
	device->save_item( NAME(doc->user_data_size));
	device->save_item( NAME(doc->extra_area_size));
	device->save_pointer( NAME(doc->data[0]), doc->data_size[0]);
	device->save_pointer( NAME(doc->data[1]), doc->data_size[1]);
	device->save_pointer( NAME(doc->data[2]), doc->data_size[2]);
}

static DEVICE_STOP( diskonchip_g3 )
{
	verboselog( device->machine(), 9, "(DOC) device stop\n");
//  diskonchip_save( device, "diskonchip");
}

static DEVICE_RESET( diskonchip_g3 )
{
	verboselog( device->machine(), 9, "(DOC) device reset\n");
}

static DEVICE_NVRAM( diskonchip_g3 )
{
	diskonchip_g3_t *doc = get_token( device);
	verboselog( device->machine(), 9, "(DOC) nvram %s\n", read_or_write ? "write" : "read");
	if (read_or_write)
	{
		if (file)
		{
			file->write(doc->data[0], doc->data_size[0]);
			file->write(doc->data[1], doc->data_size[1]);
			file->write(doc->data[2], doc->data_size[2]);
		}
	}
	else
	{
		if (file)
		{
			file->read(doc->data[0], doc->data_size[0]);
			file->read(doc->data[1], doc->data_size[1]);
			file->read(doc->data[2], doc->data_size[2]);
		}
		else
		{
			memset( doc->data[0], 0xFF, doc->data_size[0]);
			memset( doc->data[1], 0x00, doc->data_size[1]);
			memset( doc->data[2], 0xFF, doc->data_size[2]);
		}
	}
}

DEVICE_GET_INFO( diskonchip_g3 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES: info->i = sizeof(diskonchip_g3_t); break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES: info->i = sizeof(diskonchip_g3_config); break;
//      case DEVINFO_INT_CLASS: info->i = DEVICE_CLASS_PERIPHERAL; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START: info->start = DEVICE_START_NAME(diskonchip_g3); break;
		case DEVINFO_FCT_STOP: info->stop = DEVICE_STOP_NAME(diskonchip_g3); break;
		case DEVINFO_FCT_RESET: info->reset = DEVICE_RESET_NAME(diskonchip_g3); break;
		case DEVINFO_FCT_NVRAM: info->nvram = DEVICE_NVRAM_NAME(diskonchip_g3); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME: strcpy(info->s, "DiskOnChip G3"); break;
		case DEVINFO_STR_FAMILY: strcpy(info->s, "DiskOnChip"); break;
		case DEVINFO_STR_VERSION: strcpy(info->s, "1.0"); break;
		case DEVINFO_STR_SOURCE_FILE: strcpy(info->s, __FILE__); break;
		case DEVINFO_STR_CREDITS: strcpy( info->s, "Copyright Tim Schuerewegen and the MESS Team" ); break;
	}
}

DEFINE_LEGACY_NVRAM_DEVICE(DISKONCHIP_G3, diskonchip_g3);
