/*

    Cybiko Wireless Inter-tainment System

    (c) 2001-2007 Tim Schuerewegen

    Cybiko Classic (V1)
    Cybiko Classic (V2)
    Cybiko Xtreme

*/

#include "emu.h"
#include "emuopts.h"
#include "includes/cybiko.h"

//#include "cpu/h8s2xxx/h8s2xxx.h"
#include "video/hd66421.h"
#include "sound/speaker.h"
#include "machine/pcf8593.h"
#include "machine/at45dbxx.h"
#include "machine/sst39vfx.h"
#include "machine/ram.h"

#ifndef _H8S2XXX_H_
#define H8S_IO(xxxx) ((xxxx) - 0xFE40)

#define H8S_IO_PFDDR  H8S_IO(0xFEBE)

#define H8S_IO_PORT1  H8S_IO(0xFF50)
#define H8S_IO_PORT3  H8S_IO(0xFF52)
#define H8S_IO_PORT5  H8S_IO(0xFF54)
#define H8S_IO_PORTF  H8S_IO(0xFF5E)

#define H8S_IO_P1DR   H8S_IO(0xFF60)
#define H8S_IO_P3DR   H8S_IO(0xFF62)
#define H8S_IO_P5DR   H8S_IO(0xFF64)
#define H8S_IO_PFDR   H8S_IO(0xFF6E)
#define H8S_IO_SSR2   H8S_IO(0xFF8C)

#define H8S_P1_TIOCB1 0x20

#define H8S_P3_SCK1 0x20
#define H8S_P3_SCK0 0x10
#define H8S_P3_RXD1 0x08
#define H8S_P3_TXD1 0x02

#define H8S_P5_SCK2 0x04
#define H8S_P5_RXD2 0x02
#define H8S_P5_TXD2 0x01

#define H8S_PF_PF2 0x04
#define H8S_PF_PF1 0x02
#define H8S_PF_PF0 0x01

#define H8S_SSR_RDRF 0x40 /* receive data register full */
#endif

#define LOG_LEVEL  1
#define _logerror(level,x)  do { if (LOG_LEVEL > level) logerror x; } while (0)

/////////////////////////
// FUNCTION PROTOTYPES //
/////////////////////////

#define MACHINE_STOP(name) \
	static void machine_stop_##name( running_machine &machine)

// machine stop
MACHINE_STOP( cybikov1 );
MACHINE_STOP( cybikov2 );
MACHINE_STOP( cybikoxt );

// state->m_rs232
static void cybiko_rs232_init(running_machine &machine);
static void cybiko_rs232_exit(void);
static void cybiko_rs232_reset(void);

////////////////////////
// DRIVER INIT & EXIT //
////////////////////////

static void init_ram_handler(running_machine &machine, offs_t start, offs_t size, offs_t mirror)
{
	machine.device("maincpu")->memory().space(AS_PROGRAM)->install_read_bank(start, start + size - 1, 0, mirror - size, "bank1");
	machine.device("maincpu")->memory().space(AS_PROGRAM)->install_write_bank(start, start + size - 1, 0, mirror - size, "bank1");
	memory_set_bankptr( machine, "bank1", ram_get_ptr(machine.device(RAM_TAG)));
}

DRIVER_INIT( cybikov1 )
{
	_logerror( 0, ("init_cybikov1\n"));
	init_ram_handler(machine, 0x200000, ram_get_size(machine.device(RAM_TAG)), 0x200000);
}

DRIVER_INIT( cybikov2 )
{
	_logerror( 0, ("init_cybikov2\n"));
	init_ram_handler(machine, 0x200000, ram_get_size(machine.device(RAM_TAG)), 0x200000);
}

DRIVER_INIT( cybikoxt )
{
	_logerror( 0, ("init_cybikoxt\n"));
	init_ram_handler(machine, 0x400000, ram_get_size(machine.device(RAM_TAG)), 0x200000);
}

////////////////////
// NVRAM HANDLERS //
////////////////////

#ifdef UNUSED_FUNCTION
NVRAM_HANDLER( cybikov1 )
{
	_logerror( 0, ("nvram_handler_cybikov1 (%p/%d)\n", file, read_or_write));
	nvram_handler_at45dbxx( machine, file, read_or_write);
	nvram_handler_pcf8593( machine, file, read_or_write);
}

NVRAM_HANDLER( cybikov2 )
{
	_logerror( 0, ("nvram_handler_cybikov2 (%p/%d)\n", file, read_or_write));
	nvram_handler_at45dbxx( machine, file, read_or_write);
	nvram_handler_sst39vfx( machine, file, read_or_write);
	nvram_handler_pcf8593( machine, file, read_or_write);
}

NVRAM_HANDLER( cybikoxt )
{
	_logerror( 0, ("nvram_handler_cybikoxt (%p/%d)\n", file, read_or_write));
	nvram_handler_sst39vfx( machine, file, read_or_write);
	nvram_handler_pcf8593( machine, file, read_or_write);
}
#endif

///////////////////
// MACHINE START //
///////////////////

static emu_file *nvram_system_fopen( running_machine &machine, UINT32 openflags, const char *name)
{
	file_error filerr;
	astring *fname = astring_assemble_4( astring_alloc(), machine.system().name, PATH_SEPARATOR, name, ".nv");	
	emu_file *file = global_alloc(emu_file(machine.options().nvram_directory(), openflags));
	filerr = file->open(astring_c( fname));	
	astring_free( fname);
	return (filerr == FILERR_NONE) ? file : NULL;
}

typedef void (nvram_load_func)(running_machine &machine, emu_file *file);

static int nvram_system_load( running_machine &machine, const char *name, nvram_load_func _nvram_load, int required)
{
	emu_file *file;
	file = nvram_system_fopen( machine, OPEN_FLAG_READ, name);
	if (!file)
	{
		if (required) mame_printf_error( "nvram load failed (%s)\n", name);
		return FALSE;
	}
	(*_nvram_load)(machine, file);
	global_free(file);
	return TRUE;
}

typedef void (nvram_save_func)(running_machine &machine, emu_file *file);

static int nvram_system_save( running_machine &machine, const char *name, nvram_save_func _nvram_save)
{
	emu_file *file;
	file = nvram_system_fopen( machine, OPEN_FLAG_CREATE | OPEN_FLAG_WRITE | OPEN_FLAG_CREATE_PATHS, name);
	if (!file)
	{
		mame_printf_error( "nvram save failed (%s)\n", name);
		return FALSE;
	}
	(*_nvram_save)(machine, file);
	global_free(file);
	return TRUE;
}

static void cybiko_pcf8593_load(running_machine &machine, emu_file *file)
{
	device_t *device = machine.device("rtc");
	pcf8593_load(device, file);
}

static void cybiko_pcf8593_save(running_machine &machine, emu_file *file)
{
	device_t *device = machine.device("rtc");
	pcf8593_save(device, file);
}

static void cybiko_at45dbxx_load(running_machine &machine, emu_file *file)
{
	device_t *device = machine.device("flash1");
	at45dbxx_load(device, file);
}

static void cybiko_at45dbxx_save(running_machine &machine, emu_file *file)
{
	device_t *device = machine.device("flash1");
	at45dbxx_save(device, file);
}

static void cybiko_sst39vfx_load(running_machine &machine, emu_file *file)
{
	device_t *device = machine.device("flash2");
	sst39vfx_load(device, file);
}

static void cybiko_sst39vfx_save(running_machine &machine, emu_file *file)
{
	device_t *device = machine.device("flash2");
	sst39vfx_save(device, file);
}

MACHINE_START( cybikov1 )
{
	_logerror( 0, ("machine_start_cybikov1\n"));
	// real-time clock
	nvram_system_load( machine, "rtc", cybiko_pcf8593_load, 0);
	// serial dataflash
	nvram_system_load( machine, "flash1", cybiko_at45dbxx_load, 1);
	// serial port
	cybiko_rs232_init(machine);
	// other
	machine.add_notifier(MACHINE_NOTIFY_EXIT, machine_stop_cybikov1);
}

MACHINE_START( cybikov2 )
{
	device_t *flash2 = machine.device("flash2");

	_logerror( 0, ("machine_start_cybikov2\n"));
	// real-time clock
	nvram_system_load( machine, "rtc", cybiko_pcf8593_load, 0);
	// serial dataflash
	nvram_system_load( machine, "flash1", cybiko_at45dbxx_load, 1);
	// multi-purpose flash
	nvram_system_load( machine, "flash2", cybiko_sst39vfx_load, 1);
	memory_set_bankptr( machine, "bank2", sst39vfx_get_base(flash2));
	// serial port
	cybiko_rs232_init(machine);
	// other
	machine.add_notifier(MACHINE_NOTIFY_EXIT, machine_stop_cybikov2);
}

MACHINE_START( cybikoxt )
{
	device_t *flash2 = machine.device("flash2");
	_logerror( 0, ("machine_start_cybikoxt\n"));
	// real-time clock
	nvram_system_load( machine, "rtc", cybiko_pcf8593_load, 0);
	// multi-purpose flash
	nvram_system_load( machine, "flash2", cybiko_sst39vfx_load, 1);
	memory_set_bankptr( machine, "bank2", sst39vfx_get_base(flash2));
	// serial port
	cybiko_rs232_init(machine);
	// other
	machine.add_notifier(MACHINE_NOTIFY_EXIT, machine_stop_cybikoxt);
}

///////////////////
// MACHINE RESET //
///////////////////

MACHINE_RESET( cybikov1 )
{
	_logerror( 0, ("machine_reset_cybikov1\n"));
	cybiko_rs232_reset();
}

MACHINE_RESET( cybikov2 )
{
	_logerror( 0, ("machine_reset_cybikov2\n"));
	cybiko_rs232_reset();
}

MACHINE_RESET( cybikoxt )
{
	_logerror( 0, ("machine_reset_cybikoxt\n"));
	cybiko_rs232_reset();
}

//////////////////
// MACHINE STOP //
//////////////////

MACHINE_STOP( cybikov1 )
{
	_logerror( 0, ("machine_stop_cybikov1\n"));
	// real-time clock
	nvram_system_save( machine, "rtc", cybiko_pcf8593_save);
	// serial dataflash
	nvram_system_save( machine, "flash1", cybiko_at45dbxx_save);
	// serial port
	cybiko_rs232_exit();
}

MACHINE_STOP( cybikov2 )
{
	_logerror( 0, ("machine_stop_cybikov2\n"));
	// real-time clock
	nvram_system_save( machine, "rtc", cybiko_pcf8593_save);
	// serial dataflash
	nvram_system_save( machine, "flash1", cybiko_at45dbxx_save);
	// multi-purpose flash
	nvram_system_save( machine, "flash2", cybiko_sst39vfx_save);
	// serial port
	cybiko_rs232_exit();
}

MACHINE_STOP( cybikoxt )
{
	_logerror( 0, ("machine_stop_cybikoxt\n"));
	// real-time clock
	nvram_system_save( machine, "rtc", cybiko_pcf8593_save);
	// multi-purpose flash
	nvram_system_save( machine, "flash1", cybiko_sst39vfx_save);
	// serial port
	cybiko_rs232_exit();
}

///////////
// RS232 //
///////////


static void cybiko_rs232_init(running_machine &machine)
{
	cybiko_state *state = machine.driver_data<cybiko_state>();
	_logerror( 0, ("cybiko_rs232_init\n"));
	memset( &state->m_rs232, 0, sizeof( state->m_rs232));
//  machine.scheduler().timer_pulse(TIME_IN_HZ( 10), FUNC(rs232_timer_callback));
}

static void cybiko_rs232_exit( void)
{
	_logerror( 0, ("cybiko_rs232_exit\n"));
}

static void cybiko_rs232_reset( void)
{
	_logerror( 0, ("cybiko_rs232_reset\n"));
}

static void cybiko_rs232_write_byte( UINT8 data)
{
	#if 0
	printf( "%c", data);
	#endif
}

static void cybiko_rs232_pin_sck(cybiko_state *state, int data)
{
	_logerror( 3, ("cybiko_rs232_pin_sck (%d)\n", data));
	// clock high-to-low
	if ((state->m_rs232.pin.sck == 1) && (data == 0))
	{
		// transmit
		if (state->m_rs232.pin.txd) state->m_rs232.tx_byte = state->m_rs232.tx_byte | (1 << state->m_rs232.tx_bits);
		state->m_rs232.tx_bits++;
		if (state->m_rs232.tx_bits == 8)
		{
			state->m_rs232.tx_bits = 0;
			cybiko_rs232_write_byte( state->m_rs232.tx_byte);
			state->m_rs232.tx_byte = 0;
		}
		// receive
		state->m_rs232.pin.rxd = (state->m_rs232.rx_byte >> state->m_rs232.rx_bits) & 1;
		state->m_rs232.rx_bits++;
		if (state->m_rs232.rx_bits == 8)
		{
			state->m_rs232.rx_bits = 0;
			state->m_rs232.rx_byte = 0;
		}
	}
	// save sck
	state->m_rs232.pin.sck = data;
}

static void cybiko_rs232_pin_txd(cybiko_state *state, int data)
{
	_logerror( 3, ("cybiko_rs232_pin_txd (%d)\n", data));
	state->m_rs232.pin.txd = data;
}

static int cybiko_rs232_pin_rxd(cybiko_state *state)
{
	_logerror( 3, ("cybiko_rs232_pin_rxd\n"));
	return state->m_rs232.pin.rxd;
}

static int cybiko_rs232_rx_queue( void)
{
	return 0;
}

/////////////////////////
// READ/WRITE HANDLERS //
/////////////////////////

READ16_HANDLER( cybiko_lcd_r )
{
	UINT16 data = 0;
	if (ACCESSING_BITS_8_15) data = data | (hd66421_reg_idx_r() << 8);
	if (ACCESSING_BITS_0_7) data = data | (hd66421_reg_dat_r() << 0);
	return data;
}

WRITE16_HANDLER( cybiko_lcd_w )
{
	if (ACCESSING_BITS_8_15) hd66421_reg_idx_w( (data >> 8) & 0xFF);
	if (ACCESSING_BITS_0_7) hd66421_reg_dat_w( (data >> 0) & 0xFF);
}

static READ8_HANDLER( cybiko_key_r_byte )
{
	UINT8 data = 0xFF;
	int i;
	static const char *const keynames[] = { "A1", "A2", "A3", "A4", "A5", "A6", "A7", "A8", "A9" };

	_logerror( 2, ("cybiko_key_r_byte (%08X)\n", offset));
	// A11
	if (!(offset & (1 << 11)))
		data &= 0xFE;
	// A1 .. A9
	for (i=1; i<10; i++)
	{
		if (!(offset & (1 << i)))
			data &= input_port_read(space->machine(), keynames[i-1]);
	}
	// A0
	if (!(offset & (1 <<  0)))
		data |= 0xFF;
	//
	return data;
}

READ16_HANDLER( cybiko_key_r )
{
	UINT16 data = 0;
	_logerror( 2, ("cybiko_key_r (%08X/%04X)\n", offset, mem_mask));
	if (ACCESSING_BITS_8_15) data = data | (cybiko_key_r_byte(space, offset * 2 + 0) << 8);
	if (ACCESSING_BITS_0_7) data = data | (cybiko_key_r_byte(space, offset * 2 + 1) << 0);
	_logerror( 2, ("%04X\n", data));
	return data;
}

static READ8_HANDLER( cybiko_io_reg_r )
{
	cybiko_state *state = space->machine().driver_data<cybiko_state>();
	UINT8 data = 0;
	_logerror( 2, ("cybiko_io_reg_r (%08X)\n", offset));
	switch (offset)
	{
		// keyboard
		case H8S_IO_PORT1 :
		{
			//if (input_port_read(space->machine(), "A1") & 0x02) data = data | 0x08; else data = data & (~0x08); // "esc" key
			data = data | 0x08;
		}
		break;
		// serial dataflash
		case H8S_IO_PORT3 :
		{
				device_t *device = space->machine().device("flash1");
				if (at45dbxx_pin_so(device)) data = data | H8S_P3_RXD1;
		}
		break;

		// state->m_rs232
		case H8S_IO_PORT5 : if (cybiko_rs232_pin_rxd(state)) data = data | H8S_P5_RXD2; break;
		// real-time clock
		case H8S_IO_PORTF :
		{
			device_t *device = space->machine().device("rtc");
			data = H8S_PF_PF2;
			if (pcf8593_pin_sda_r(device)) data |= H8S_PF_PF0;
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

static WRITE8_HANDLER( cybiko_io_reg_w )
{
	cybiko_state *state = space->machine().driver_data<cybiko_state>();
	device_t *rtc = space->machine().device("rtc");
	device_t *speaker = space->machine().device("speaker");

	_logerror( 2, ("cybiko_io_reg_w (%08X/%02X)\n", offset, data));
	switch (offset)
	{
		// speaker
		case H8S_IO_P1DR : speaker_level_w( speaker, (data & H8S_P1_TIOCB1) ? 1 : 0); break;
		// serial dataflash
		case H8S_IO_P3DR :
		{
			device_t *device = space->machine().device("flash1");
			at45dbxx_pin_cs ( device, (data & H8S_P3_SCK0) ? 0 : 1);
			at45dbxx_pin_si ( device, (data & H8S_P3_TXD1) ? 1 : 0);
			at45dbxx_pin_sck( device, (data & H8S_P3_SCK1) ? 1 : 0);
		}
		break;
		// state->m_rs232
		case H8S_IO_P5DR :
		{
			cybiko_rs232_pin_txd(state, (data & H8S_P5_TXD2) ? 1 : 0);
			cybiko_rs232_pin_sck(state, (data & H8S_P5_SCK2) ? 1 : 0);
		}
		break;
		// real-time clock
		case H8S_IO_PFDR  : pcf8593_pin_scl(rtc, (data & H8S_PF_PF1) ? 1 : 0); break;
		case H8S_IO_PFDDR : pcf8593_pin_sda_w(rtc, (data & H8S_PF_PF0) ? 0 : 1); break;
	}
}

READ8_HANDLER( cybikov1_io_reg_r )
{
	_logerror( 2, ("cybikov1_io_reg_r (%08X)\n", offset));
	return cybiko_io_reg_r(space, offset);
}

READ8_HANDLER( cybikov2_io_reg_r )
{
	_logerror( 2, ("cybikov2_io_reg_r (%08X)\n", offset));
	return cybiko_io_reg_r(space, offset);
}

READ8_HANDLER( cybikoxt_io_reg_r )
{
	cybiko_state *state = space->machine().driver_data<cybiko_state>();
	UINT8 data = 0;
	_logerror( 2, ("cybikoxt_io_reg_r (%08X)\n", offset));
	switch (offset)
	{
		// state->m_rs232
		case H8S_IO_PORT3 : if (cybiko_rs232_pin_rxd(state)) data = data | H8S_P3_RXD1; break;
		// default
		default : data = cybiko_io_reg_r(space, offset);
	}
	return data;
}

WRITE8_HANDLER( cybikov1_io_reg_w )
{
	_logerror( 2, ("cybikov1_io_reg_w (%08X/%02X)\n", offset, data));
	cybiko_io_reg_w(space, offset, data);
}

WRITE8_HANDLER( cybikov2_io_reg_w )
{
	_logerror( 2, ("cybikov2_io_reg_w (%08X/%02X)\n", offset, data));
	cybiko_io_reg_w(space, offset, data);
}

WRITE8_HANDLER( cybikoxt_io_reg_w )
{
	cybiko_state *state = space->machine().driver_data<cybiko_state>();
	_logerror( 2, ("cybikoxt_io_reg_w (%08X/%02X)\n", offset, data));
	switch (offset)
	{
		// state->m_rs232
		case H8S_IO_P3DR :
		{
			cybiko_rs232_pin_txd(state, (data & H8S_P3_TXD1) ? 1 : 0);
			cybiko_rs232_pin_sck(state, (data & H8S_P3_SCK1) ? 1 : 0);
		}
		break;
		// default
		default : cybiko_io_reg_w(space, offset, data);
	}
}

// Cybiko Xtreme writes following byte pairs to 0x200003/0x200000
// 00/01, 00/C0, 0F/32, 0D/03, 0B/03, 09/50, 07/D6, 05/00, 04/00, 20/00, 23/08, 27/01, 2F/08, 2C/02, 2B/08, 28/01
// 04/80, 05/02, 00/C8, 00/C8, 00/C0, 1B/2C, 00/01, 00/C0, 1B/6C, 0F/10, 0D/07, 0B/07, 09/D2, 07/D6, 05/00, 04/00,
// 20/00, 23/08, 27/01, 2F/08, 2C/02, 2B/08, 28/01, 37/08, 34/04, 33/08, 30/03, 04/80, 05/02, 1B/6C, 00/C8
WRITE16_HANDLER( cybiko_unk1_w )
{
	if (ACCESSING_BITS_8_15) logerror( "[%08X] <- %02X\n", 0x200000 + offset * 2 + 0, (data >> 8) & 0xFF);
	if (ACCESSING_BITS_0_7) logerror( "[%08X] <- %02X\n", 0x200000 + offset * 2 + 1, (data >> 0) & 0xFF);
}

READ16_HANDLER( cybiko_unk2_r )
{
	switch (offset << 1)
	{
		case 0x000 : return 0xBA0B; // magic (part 1)
		case 0x002 : return 0xAB15; // magic (part 2)
		case 0x004 : return (0x07 << 8) | (0x07 ^0xFF); // brightness (or contrast) & value xor FF
//      case 0x006 : return 0x2B57; // do not show "Free Games and Applications at www.cybiko.com" screen
		case 0x008 : return 0xDEBA; // enable debug output
		case 0x016 : return 0x5000 | 0x03FF; // ?
		case 0x7FC : return 0x22A1; // crc32 (part 1)
		case 0x7FE : return 0x8728; // crc32 (part 2)
		default    : return 0xFFFF;
	}
}
