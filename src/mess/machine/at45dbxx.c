/*

	Atmel Serial DataFlash

	(c) 2001-2007 Tim Schuerewegen

	AT45DB041 -  528 KByte
	AT45DB081 - 1056 KByte
	AT45DB161 - 2112 KByte

*/

#include "at45dbxx.h"

#define LOG_LEVEL  1
#define _logerror(level,...)  if (LOG_LEVEL > level) logerror(__VA_ARGS__)

#define FLASH_CMD_52  0x52
#define FLASH_CMD_57  0x57
#define FLASH_CMD_60  0x60
#define FLASH_CMD_82  0x82

#define FLASH_MODE_XX  0 // unknown
#define FLASH_MODE_SI  1 // input
#define FLASH_MODE_SO  2 // output

typedef struct
{
	int cs;    // chip select
	int sck;   // serial clock
	int si;    // serial input
	int so;    // serial output
	int wp;    // write protect
	int reset; // reset
	int busy;  // busy
} AT45DBXX_PINS;

typedef struct
{
	UINT8 data[8], size;
} AT45DBXX_CMD;

typedef struct
{
	UINT8 *data;
	UINT32 size, pos;
} AT45DBXX_IO;

typedef struct
{
	UINT8 *data;
	UINT32 size;
	UINT32 pages, page_size;
	UINT8 mode, status, devid, *buffer1, *buffer2;
	AT45DBXX_PINS pin;
	UINT8 si_byte, si_bits, so_byte, so_bits;
	AT45DBXX_CMD cmd;
	AT45DBXX_IO io;
} AT45DBXX;

static AT45DBXX flash;

void at45dbxx_state_save( void);

void at45dbxx_init( int type)
{
	_logerror( 0, "at45dbxx_init (%d)\n", type);
	memset( &flash, 0, sizeof( flash));
	switch (type)
	{
		case AT45DB041 : flash.pages = 2048; flash.page_size = 264; flash.devid = 0x18; break;
		case AT45DB081 : flash.pages = 4096; flash.page_size = 264; flash.devid = 0x20; break;
		case AT45DB161 : flash.pages = 4096; flash.page_size = 528; flash.devid = 0x28; break;
	}
	flash.size = flash.pages * flash.page_size;
	flash.data = (UINT8*)malloc( flash.size);
	flash.buffer1 = (UINT8*)malloc( flash.page_size);
	flash.buffer2 = (UINT8*)malloc( flash.page_size);
	at45dbxx_state_save();
}

void at45dbxx_exit( void)
{
	_logerror( 0, "at45dbxx_exit\n");
	free( flash.buffer2);
	free( flash.buffer1);
	free( flash.data);
}

void at45dbxx_reset( void)
{
	_logerror( 1, "at45dbxx_reset\n");
	// mode
	flash.mode = FLASH_MODE_SI;
	// command
	memset( &flash.cmd.data[0], 0, sizeof( flash.cmd.data));
	flash.cmd.size = 0;
	// input/output
	flash.io.data = NULL;
	flash.io.size = 0;
	flash.io.pos  = 0;
	// pins
	flash.pin.cs    = 0;
	flash.pin.sck   = 0;
	flash.pin.si    = 0;
	flash.pin.so    = 0;
	flash.pin.wp    = 0;
	flash.pin.reset = 0;
	flash.pin.busy  = 0;
	// output
	flash.so_byte = 0;
	flash.so_bits = 0;
	// input
	flash.si_byte = 0;
	flash.si_bits = 0;
}

void at45dbxx_state_save( void)
{
	const char *name = "at45dbxx";
	// data
	state_save_register_item_pointer( name, 0, flash.data, flash.size);
	// pins
	state_save_register_item( name, 0, flash.pin.cs);
	state_save_register_item( name, 0, flash.pin.sck);
	state_save_register_item( name, 0, flash.pin.si);
	state_save_register_item( name, 0, flash.pin.so);
	state_save_register_item( name, 0, flash.pin.wp);
	state_save_register_item( name, 0, flash.pin.reset);
	state_save_register_item( name, 0, flash.pin.busy);
}

UINT8 at45dbxx_read_byte( void)
{
	UINT8 data;
	// check mode
	if ((flash.mode != FLASH_MODE_SO) || (!flash.io.data)) return 0;
	// read byte
	data = flash.io.data[flash.io.pos++];
	_logerror( 2, "at45dbxx_read_byte (%02X) (%03d/%03d)\n", data, flash.io.pos, flash.io.size);
	if (flash.io.pos == flash.io.size) flash.io.pos = 0;
	return data;
}

void flash_set_io( UINT8* data, UINT32 size, UINT32 pos)
{
	flash.io.data = data;
	flash.io.size = size;
	flash.io.pos  = pos;
}

UINT32 flash_get_page_addr( void)
{
	switch (flash.devid)
	{
		case 0x18 : return ((flash.cmd.data[1] & 0x0F) << 7) | ((flash.cmd.data[2] & 0xFE) >> 1);
		case 0x20 : return ((flash.cmd.data[1] & 0x1F) << 7) | ((flash.cmd.data[2] & 0xFE) >> 1);
		case 0x28 : return ((flash.cmd.data[1] & 0x3F) << 6) | ((flash.cmd.data[2] & 0xFC) >> 2);
		default   : return 0;
	}
}

UINT32 flash_get_byte_addr( void)
{
	switch (flash.devid)
	{
		case 0x18 : // fall-through
		case 0x20 : return ((flash.cmd.data[2] & 0x01) << 8) | ((flash.cmd.data[3] & 0xFF) >> 0);
		case 0x28 : return ((flash.cmd.data[2] & 0x03) << 8) | ((flash.cmd.data[3] & 0xFF) >> 0);
		default   : return 0;
	}
}

void at45dbxx_write_byte( UINT8 data)
{
	// check mode
	if (flash.mode != FLASH_MODE_SI) return;
	// process byte
	if (flash.cmd.size < 8)
	{
		UINT8 opcode;
		_logerror( 2, "at45dbxx_write_byte (%02X)\n", data);
		// add to command buffer
		flash.cmd.data[flash.cmd.size++] = data;
		// check opcode
		opcode = flash.cmd.data[0];
		switch (opcode)
		{
			// status register read
			case FLASH_CMD_57 :
			{
				// 8 bits command
				if (flash.cmd.size == 1)
				{
					_logerror( 1, "at45dbxx opcode %02X - status register read\n", opcode);
					flash.status = (flash.status & 0xC7) | flash.devid; // 80 = busy / 40 = compare fail
					flash_set_io( &flash.status, 1, 0);
					flash.mode = FLASH_MODE_SO;
					flash.cmd.size = 8;
				}
			}
			break;
			// main memory page to buffer 1 compare
			case FLASH_CMD_60 :
			{
				// 8 bits command + 4 bits reserved + 11 bits page address + 9 bits don't care
				if (flash.cmd.size == 4)
				{
					UINT32 page;
					UINT8 comp;
					page = flash_get_page_addr();
					_logerror( 1, "at45dbxx opcode %02X - main memory page to buffer 1 compare [%04X]\n", opcode, page);
					comp = memcmp( flash.data + page * flash.page_size, flash.buffer1, flash.page_size) == 0 ? 0 : 1;
					if (comp) flash.status |= 0x40; else flash.status &= ~0x40;
					_logerror( 1, "at45dbxx page compare %s\n", comp ? "failure" : "success");
					flash.mode = FLASH_MODE_SI;
					flash.cmd.size = 8;
				}
			}
			break;
			// main memory page read
			case FLASH_CMD_52 :
			{
				// 8 bits command + 4 bits reserved + 11 bits page address + 9 bits buffer address + 32 bits don't care
				if (flash.cmd.size == 8)
				{
					UINT32 page, byte;
					page = flash_get_page_addr();
					byte = flash_get_byte_addr();
					_logerror( 1, "at45dbxx opcode %02X - main memory page read [%04X/%04X]\n", opcode, page, byte);
					flash_set_io( flash.data + page * flash.page_size, flash.page_size, byte);
					flash.mode = FLASH_MODE_SO;
					flash.cmd.size = 8;
				}
			}
			break;
			// main memory page program through buffer 1
			case FLASH_CMD_82 :
			{
				// 8 bits command + 4 bits reserved + 11 bits page address + 9 bits buffer address
				if (flash.cmd.size == 4)
				{
					UINT32 page, byte;
					page = flash_get_page_addr();
					byte = flash_get_byte_addr();
					_logerror( 1, "at45dbxx opcode %02X - main memory page program through buffer 1 [%04X/%04X]\n",opcode, page, byte);
					flash_set_io( flash.buffer1, flash.page_size, byte);
					memset( flash.buffer1, 0xFF, flash.page_size);
					flash.mode = FLASH_MODE_SI;
					flash.cmd.size = 8;
				}
			}
			break;
			// other
			default :
			{
				_logerror( 1, "at45dbxx opcode %02X - unknown\n", opcode);
				flash.cmd.data[0] = 0;
				flash.cmd.size = 0;
			}
			break;
		}
	}
	else
	{
		_logerror( 2, "at45dbxx_write_byte (%02X) (%03d/%03d)\n", data, flash.io.pos + 1, flash.io.size);
		// store byte
		flash.io.data[flash.io.pos] = data;
		flash.io.pos++;
		if (flash.io.pos == flash.io.size) flash.io.pos = 0;
	}
}

int at45dbxx_pin_so( void)
{
	if (flash.pin.cs == 0) return 0;
	return flash.pin.so;
}

void at45dbxx_pin_si( int data)
{
	if (flash.pin.cs == 0) return;
	flash.pin.si = data;
}

void at45dbxx_pin_cs( int data)
{
	// check if changed
	if (flash.pin.cs == data) return;
	// cs low-to-high
	if (data != 0)
	{
		// complete program command
		if ((flash.cmd.size >= 4) && (flash.cmd.data[0] == FLASH_CMD_82))
		{
			UINT32 page, byte;
			page = flash_get_page_addr();
			byte = flash_get_byte_addr();
			_logerror( 1, "at45dbxx - program data stored in buffer 1 into selected page in main memory [%04X/%04X]\n", page, byte);
			memcpy( flash.data + page * flash.page_size, flash.buffer1, flash.page_size);
		}
		// reset
		at45dbxx_reset();
	}
	// save cs
	flash.pin.cs = data;
}

void at45dbxx_pin_sck( int data)
{
	// check if changed
	if (flash.pin.sck == data) return;
	// sck high-to-low
	if (data == 0)
	{
		// output (part 1)
		if (flash.so_bits == 8)
		{
			flash.so_bits = 0;
			flash.so_byte = at45dbxx_read_byte();
		}
		// input
		if (flash.pin.si) flash.si_byte = flash.si_byte | (1 << flash.si_bits);
		flash.si_bits++;
		if (flash.si_bits == 8)
		{
			flash.si_bits = 0;
			at45dbxx_write_byte( flash.si_byte);
			flash.si_byte = 0;
		}
		// output (part 2)
		flash.pin.so = (flash.so_byte >> flash.so_bits) & 1;
		flash.so_bits++;
	}
	// save sck
	flash.pin.sck = data;
}

void at45dbxx_load( mame_file *file)
{
	_logerror( 0, "at45dbxx_load (%p)\n", file);
	mame_fread( file, flash.data, flash.size);
}

void at45dbxx_save( mame_file *file)
{
	_logerror( 0, "at45dbxx_save (%p)\n", file);
	mame_fwrite( file, flash.data, flash.size);
}

/*
NVRAM_HANDLER( at45dbxx )
{
	_logerror( 0, "nvram_handler_at45dbxx (%p/%d)\n", file, read_or_write);
	if (read_or_write)
	{
		at45dbxx_save( file);
	}
	else
	{
		if (file)
		{
			at45dbxx_load( file);
		}
		else
		{
			memset( flash.data, 0xFF, flash.size);
		}
	}
}
*/
