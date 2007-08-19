/*

	Cybiko Wireless Inter-tainment System

	(c) 2001-2007 Tim Schuerewegen

	Cybiko Classic (V1)
	Cybiko Classic (V2)
	Cybiko Xtreme

*/

#include "includes/cybiko.h"

#include "cpu/h8s2xxx/h8s2xxx.h"
#include "video/hd66421.h"
#include "sound/speaker.h"
#include "machine/pcf8593.h"
#include "machine/at45dbxx.h"
#include "machine/sst39vfx.h"
#include "debugger.h"

#define LOG_LEVEL  1
#define _logerror(level,...)  if (LOG_LEVEL > level) logerror(__VA_ARGS__)

/////////////////////////
// FUNCTION PROTOTYPES //
/////////////////////////

#define MACHINE_STOP(name) \
	void machine_stop_##name( running_machine *machine)

// machine stop
MACHINE_STOP( cybikov1 );
MACHINE_STOP( cybikov2 );
MACHINE_STOP( cybikoxt );

// rs232
void cybiko_rs232_init( void);
void cybiko_rs232_exit( void);
void cybiko_rs232_reset( void);

////////////////////////
// DRIVER INIT & EXIT //
////////////////////////

static void init_ram_handler( offs_t start, offs_t size, offs_t mirror)
{
	memory_install_read_handler( 0, ADDRESS_SPACE_PROGRAM, start, start + size - 1, 0, mirror - size, STATIC_BANK1);
	memory_install_write_handler( 0, ADDRESS_SPACE_PROGRAM, start, start + size - 1, 0, mirror - size, STATIC_BANK1);
	memory_set_bankptr( 1, mess_ram);
}

DRIVER_INIT( cybikov1 )
{
	_logerror( 0, "init_cybikov1\n");
	init_ram_handler( 0x200000, mess_ram_size, 0x200000);
}

DRIVER_INIT( cybikov2 )
{
	_logerror( 0, "init_cybikov2\n");
	init_ram_handler( 0x200000, mess_ram_size, 0x200000);
}

DRIVER_INIT( cybikoxt )
{
	_logerror( 0, "init_cybikoxt\n");
	init_ram_handler( 0x400000, mess_ram_size, 0x200000);
}

////////////////////
// NVRAM HANDLERS //
////////////////////

/*
NVRAM_HANDLER( cybikov1 )
{
	_logerror( 0, "nvram_handler_cybikov1 (%p/%d)\n", file, read_or_write);
	nvram_handler_at45dbxx( machine, file, read_or_write);
	nvram_handler_pcf8593( machine, file, read_or_write);
}
*/

/*
NVRAM_HANDLER( cybikov2 )
{
	_logerror( 0, "nvram_handler_cybikov2 (%p/%d)\n", file, read_or_write);
	nvram_handler_at45dbxx( machine, file, read_or_write);
	nvram_handler_sst39vfx( machine, file, read_or_write);
	nvram_handler_pcf8593( machine, file, read_or_write);
}
*/

/*
NVRAM_HANDLER( cybikoxt )
{
	_logerror( 0, "nvram_handler_cybikoxt (%p/%d)\n", file, read_or_write);
	nvram_handler_sst39vfx( machine, file, read_or_write);
	nvram_handler_pcf8593( machine, file, read_or_write);
}
*/

///////////////////
// MACHINE START //
///////////////////

static mame_file *nvram_system_fopen( running_machine *machine, UINT32 openflags, const char *name)
{
	astring *fname;
	file_error filerr;
	mame_file *file;
	fname = astring_assemble_4( astring_alloc(), machine->gamedrv->name, PATH_SEPARATOR, name, ".nv");
	filerr = mame_fopen( SEARCHPATH_NVRAM, astring_c( fname), openflags, &file);
	astring_free( fname);
	return (filerr == FILERR_NONE) ? file : NULL;
}

typedef void (nvram_load_func)( mame_file *file);

static int nvram_system_load( running_machine *machine, const char *name, nvram_load_func _nvram_load, int required)
{
	mame_file *file;
	file = nvram_system_fopen( machine, OPEN_FLAG_READ, name);
	if (!file)
	{
		if (required) mame_printf_error( "nvram load failed (%s)\n", name);
		return FALSE;
	}
	_nvram_load( file);
	mame_fclose( file);
	return TRUE;
}

typedef void (nvram_save_func)( mame_file *file);

static int nvram_system_save( running_machine *machine, const char *name, nvram_save_func _nvram_save)
{
	mame_file *file;
	file = nvram_system_fopen( machine, OPEN_FLAG_CREATE | OPEN_FLAG_WRITE | OPEN_FLAG_CREATE_PATHS, name);
	if (!file)
	{
		mame_printf_error( "nvram save failed (%s)\n", name);
		return FALSE;
	}
	_nvram_save( file);
	mame_fclose( file);
	return TRUE;
}

MACHINE_START( cybikov1 )
{
	_logerror( 0, "machine_start_cybikov1\n");
	// real-time clock
	pcf8593_init();
	nvram_system_load( machine, "rtc", pcf8593_load, 0);
	// serial dataflash
	at45dbxx_init( AT45DB041);
	nvram_system_load( machine, "flash1", &at45dbxx_load, 1);
	// serial port
	cybiko_rs232_init();
	// other
	add_exit_callback( machine, machine_stop_cybikov1);
}

MACHINE_START( cybikov2 )
{
	_logerror( 0, "machine_start_cybikov2\n");
	// real-time clock
	pcf8593_init();
	nvram_system_load( machine, "rtc", pcf8593_load, 0);
	// serial dataflash
	at45dbxx_init( AT45DB041);
	nvram_system_load( machine, "flash1", &at45dbxx_load, 1);
	// multi-purpose flash
	sst39vfx_init( SST39VF020, 16, CPU_IS_BE);
	nvram_system_load( machine, "flash2", &sst39vfx_load, 1);
	memory_set_bankptr( 2, sst39vfx_get_base());
	// serial port
	cybiko_rs232_init();
	// other
	add_exit_callback( machine, machine_stop_cybikov2);
}

MACHINE_START( cybikoxt )
{
	_logerror( 0, "machine_start_cybikoxt\n");
	// real-time clock
	pcf8593_init();
	nvram_system_load( machine, "rtc", pcf8593_load, 0);
	// multi-purpose flash
	sst39vfx_init( SST39VF400A, 16, CPU_IS_BE);
	nvram_system_load( machine, "flash1", &sst39vfx_load, 1);
	memory_set_bankptr( 2, sst39vfx_get_base());
	// serial port
	cybiko_rs232_init();
	// other
	add_exit_callback( machine, machine_stop_cybikoxt);
}

///////////////////
// MACHINE RESET //
///////////////////

MACHINE_RESET( cybikov1 )
{
	_logerror( 0, "machine_reset_cybikov1\n");
	pcf8593_reset();
	at45dbxx_reset();
	cybiko_rs232_reset();
}

MACHINE_RESET( cybikov2 )
{
	_logerror( 0, "machine_reset_cybikov2\n");
	pcf8593_reset();
	at45dbxx_reset();
	sst39vfx_reset();
	cybiko_rs232_reset();
}

MACHINE_RESET( cybikoxt )
{
	_logerror( 0, "machine_reset_cybikoxt\n");
	pcf8593_reset();
	sst39vfx_reset();
	cybiko_rs232_reset();
}

//////////////////
// MACHINE STOP //
//////////////////

MACHINE_STOP( cybikov1 )
{
	_logerror( 0, "machine_stop_cybikov1\n");
	// real-time clock
	nvram_system_save( machine, "rtc", pcf8593_save);
	pcf8593_exit();
	// serial dataflash
	nvram_system_save( machine, "flash1", &at45dbxx_save);
	at45dbxx_exit();
	// serial port
	cybiko_rs232_exit();
}

MACHINE_STOP( cybikov2 )
{
	_logerror( 0, "machine_stop_cybikov2\n");
	// real-time clock
	nvram_system_save( machine, "rtc", pcf8593_save);
	pcf8593_exit();
	// serial dataflash
	nvram_system_save( machine, "flash1", &at45dbxx_save);
	at45dbxx_exit();
	// multi-purpose flash
	nvram_system_save( machine, "flash2", &sst39vfx_save);
	sst39vfx_exit();
	// serial port
	cybiko_rs232_exit();
}

MACHINE_STOP( cybikoxt )
{
	_logerror( 0, "machine_stop_cybikoxt\n");
	// real-time clock
	nvram_system_save( machine, "rtc", pcf8593_save);
	pcf8593_exit();
	// multi-purpose flash
	nvram_system_save( machine, "flash1", &sst39vfx_save);
	sst39vfx_exit();
	// serial port
	cybiko_rs232_exit();
}

///////////
// RS232 //
///////////

typedef struct
{
	int sck; // serial clock
	int txd; // transmit data
	int rxd; // receive data
} CYBIKO_RS232_PINS;

typedef struct
{
	CYBIKO_RS232_PINS pin;
	UINT8 rx_bits, rx_byte, tx_byte, tx_bits;
} CYBIKO_RS232;

static CYBIKO_RS232 rs232;

void cybiko_rs232_init( void)
{
	_logerror( 0, "cybiko_rs232_init\n");
	memset( &rs232, 0, sizeof( rs232));
//	timer_pulse( TIME_IN_HZ( 10), 0, rs232_timer_callback);
}

void cybiko_rs232_exit( void)
{
	_logerror( 0, "cybiko_rs232_exit\n");
}

void cybiko_rs232_reset( void)
{
	_logerror( 0, "cybiko_rs232_reset\n");
}

void cybiko_rs232_write_byte( UINT8 data)
{
	#if 0
	printf( "%c", data);
	#endif
}

void cybiko_rs232_pin_sck( int data)
{
	_logerror( 3, "cybiko_rs232_pin_sck (%d)\n", data);
	// clock high-to-low
	if ((rs232.pin.sck == 1) && (data == 0))
	{
		// transmit
		if (rs232.pin.txd) rs232.tx_byte = rs232.tx_byte | (1 << rs232.tx_bits);
		rs232.tx_bits++;
		if (rs232.tx_bits == 8)
		{
			rs232.tx_bits = 0;
			cybiko_rs232_write_byte( rs232.tx_byte);
			rs232.tx_byte = 0;
		}
		// receive
		rs232.pin.rxd = (rs232.rx_byte >> rs232.rx_bits) & 1;
		rs232.rx_bits++;
		if (rs232.rx_bits == 8)
		{
			rs232.rx_bits = 0;
			rs232.rx_byte = 0;
		}
	}
	// save sck
	rs232.pin.sck = data;
}

void cybiko_rs232_pin_txd( int data)
{
	_logerror( 3, "cybiko_rs232_pin_txd (%d)\n", data);
	rs232.pin.txd = data;
}

int cybiko_rs232_pin_rxd( void)
{
	_logerror( 3, "cybiko_rs232_pin_rxd\n");
	return rs232.pin.rxd;
}

int cybiko_rs232_rx_queue( void)
{
	return 0;
}

/////////////////////////
// READ/WRITE HANDLERS //
/////////////////////////

READ16_HANDLER( cybiko_lcd_r )
{
	UINT16 data = 0;
	if ACCESSING_MSB16 data = data | (hd66421_reg_idx_r() << 8);
	if ACCESSING_LSB16 data = data | (hd66421_reg_dat_r() << 0);
	return data;
}

WRITE16_HANDLER( cybiko_lcd_w )
{
	if ACCESSING_MSB16 hd66421_reg_idx_w( (data >> 8) & 0xFF);
	if ACCESSING_LSB16 hd66421_reg_dat_w( (data >> 0) & 0xFF);
}

READ8_HANDLER( cybiko_key_r_byte )
{
	UINT8 data = 0xFF;
	int i;
	_logerror( 2, "cybiko_key_r_byte (%08X)\n", offset);
	// A11
	if (!(offset & (1 << 11))) data &= 0xFE;
	// A1 .. A9
	for (i=1;i<10;i++) if (!(offset & (1 << i))) data &= readinputport( i - 1);
	// A0
	if (!(offset & (1 <<  0))) data |= 0xFF;
	//
	return data;
}

READ16_HANDLER( cybiko_key_r )
{
	UINT16 data = 0;
	_logerror( 2, "cybiko_key_r (%08X/%04X)\n", offset, mem_mask);
	if ACCESSING_MSB16 data = data | (cybiko_key_r_byte( offset * 2 + 0) << 8);
	if ACCESSING_LSB16 data = data | (cybiko_key_r_byte( offset * 2 + 1) << 0);
	_logerror( 2, "%04X\n", data);
	return data;
}

READ8_HANDLER( cybiko_io_reg_r )
{
	UINT8 data = 0;
	_logerror( 2, "cybiko_io_reg_r (%08X)\n", offset);
	switch (offset)
	{
		// keyboard
		case H8S_IO_PORT1 :
		{
			//if (readinputport(0) & 0x02) data = data | 0x08; else data = data & (~0x08); // "esc" key
			data = data | 0x08;
		}
		break;
		// serial dataflash
		case H8S_IO_PORT3 : if (at45dbxx_pin_so()) data = data | H8S_P3_RXD1; break;
		// rs232
		case H8S_IO_PORT5 : if (cybiko_rs232_pin_rxd()) data = data | H8S_P5_RXD2; break;
		// real-time clock
		case H8S_IO_PORTF :
		{
			data = H8S_PF_PF2;
			if (pcf8593_pin_sda_r()) data |= H8S_PF_PF0;
		}
		break;
		// serial 2
		case H8S_IO_SSR2 :
		{
			if (cybiko_rs232_rx_queue()) data = data | H8S_SSR_RDRF;
		}
		break;
	}
	return data;
}

WRITE8_HANDLER( cybiko_io_reg_w )
{
	_logerror( 2, "cybiko_io_reg_w (%08X/%02X)\n", offset, data);
	switch (offset)
	{
		// speaker
		case H8S_IO_P1DR : speaker_level_w( 0, (data & H8S_P1_TIOCB1) ? 1 : 0); break;
		// serial dataflash
		case H8S_IO_P3DR :
		{
			at45dbxx_pin_cs ( (data & H8S_P3_SCK0) ? 0 : 1);
			at45dbxx_pin_si ( (data & H8S_P3_TXD1) ? 1 : 0);
			at45dbxx_pin_sck( (data & H8S_P3_SCK1) ? 1 : 0);
		}
		break;
		// rs232
		case H8S_IO_P5DR :
		{
			cybiko_rs232_pin_txd( (data & H8S_P5_TXD2) ? 1 : 0);
			cybiko_rs232_pin_sck( (data & H8S_P5_SCK2) ? 1 : 0);
		}
		break;
		// real-time clock
		case H8S_IO_PFDR  : pcf8593_pin_scl( (data & H8S_PF_PF1) ? 1 : 0); break;
		case H8S_IO_PFDDR : pcf8593_pin_sda_w( (data & H8S_PF_PF0) ? 0 : 1); break;
	}
}

READ8_HANDLER( cybikov1_io_reg_r )
{
	_logerror( 2, "cybikov1_io_reg_r (%08X)\n", offset);
	return cybiko_io_reg_r( offset);
}

READ8_HANDLER( cybikov2_io_reg_r )
{
	_logerror( 2, "cybikov2_io_reg_r (%08X)\n", offset);
	return cybiko_io_reg_r( offset);
}

READ8_HANDLER( cybikoxt_io_reg_r )
{
	UINT8 data = 0;
	_logerror( 2, "cybikoxt_io_reg_r (%08X)\n", offset);
	switch (offset)
	{
		// rs232
		case H8S_IO_PORT3 : if (cybiko_rs232_pin_rxd()) data = data | H8S_P3_RXD1; break;
		// default
		default : data = cybiko_io_reg_r( offset);
	}
	return data;
}

WRITE8_HANDLER( cybikov1_io_reg_w )
{
	_logerror( 2, "cybikov1_io_reg_w (%08X/%02X)\n", offset, data);
	cybiko_io_reg_w( offset, data);
}

WRITE8_HANDLER( cybikov2_io_reg_w )
{
	_logerror( 2, "cybikov2_io_reg_w (%08X/%02X)\n", offset, data);
	cybiko_io_reg_w( offset, data);
}

WRITE8_HANDLER( cybikoxt_io_reg_w )
{
	_logerror( 2, "cybikoxt_io_reg_w (%08X/%02X)\n", offset, data);
	switch (offset)
	{
		// rs232
		case H8S_IO_P3DR :
		{
			cybiko_rs232_pin_txd( (data & H8S_P3_TXD1) ? 1 : 0);
			cybiko_rs232_pin_sck( (data & H8S_P3_SCK1) ? 1 : 0);
		}
		break;
		// default
		default : cybiko_io_reg_w( offset, data);
	}
}

// Cybiko Xtreme writes following byte pairs to 0x200003/0x200000
// 00/01, 00/C0, 0F/32, 0D/03, 0B/03, 09/50, 07/D6, 05/00, 04/00, 20/00, 23/08, 27/01, 2F/08, 2C/02, 2B/08, 28/01
// 04/80, 05/02, 00/C8, 00/C8, 00/C0, 1B/2C, 00/01, 00/C0, 1B/6C, 0F/10, 0D/07, 0B/07, 09/D2, 07/D6, 05/00, 04/00,
// 20/00, 23/08, 27/01, 2F/08, 2C/02, 2B/08, 28/01, 37/08, 34/04, 33/08, 30/03, 04/80, 05/02, 1B/6C, 00/C8
WRITE16_HANDLER( cybiko_unk1_w )
{
	if ACCESSING_MSB16 logerror( "[%08X] <- %02X\n", 0x200000 + offset * 2 + 0, (data >> 8) & 0xFF);
	if ACCESSING_LSB16 logerror( "[%08X] <- %02X\n", 0x200000 + offset * 2 + 1, (data >> 0) & 0xFF);
}

READ16_HANDLER( cybiko_unk2_r )
{
	switch (offset << 1)
	{
		case 0x000 : return 0xBA0B; break; // magic (part 1)
		case 0x002 : return 0xAB15; break; // magic (part 2)
		case 0x004 : return (0x07 << 8) | (0x07 ^0xFF); break; // brightness (or contrast) & value xor FF
//		case 0x006 : return 0x2B57; break; // do not show "Free Games and Applications at www.cybiko.com" screen
		case 0x008 : return 0xDEBA; break; // enable debug output
		case 0x016 : return 0x5000 | 0x03FF; break; // ?
		case 0x7FC : return 0x22A1; break; // crc32 (part 1)
		case 0x7FE : return 0x8728; break; // crc32 (part 2)
		default    : return 0xFFFF; break;
	}
}
