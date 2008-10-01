/*
    abc800.c

    MESS Driver by Curt Coder

    Luxor ABC 800C
    --------------
    (c) 1981 Luxor Datorer AB, Sweden

    CPU:            Z80 @ 3 MHz
    ROM:            32 KB
    RAM:            16 KB, 1 KB frame buffer, 16 KB high-resolution videoram (800C/HR)
    CRTC:           6845
    Resolution:     240x240
    Colors:         8

    Luxor ABC 800M
    --------------
    (c) 1981 Luxor Datorer AB, Sweden

    CPU:            Z80 @ 3 MHz
    ROM:            32 KB
    RAM:            16 KB, 2 KB frame buffer, 16 KB high-resolution videoram (800M/HR)
    CRTC:           6845
    Resolution:     480x240, 240x240 (HR)
    Colors:         2

    Luxor ABC 802
    -------------
    (c) 1983 Luxor Datorer AB, Sweden

    CPU:            Z80 @ 3 MHz
    ROM:            32 KB
    RAM:            16 KB, 2 KB frame buffer, 16 KB ram-floppy
    CRTC:           6845
    Resolution:     480x240
    Colors:         2

    Luxor ABC 806
    -------------
    (c) 1983 Luxor Datorer AB, Sweden

    CPU:            Z80 @ 3 MHz
    ROM:            32 KB
    RAM:            32 KB, 4 KB frame buffer, 128 KB scratch pad ram
    CRTC:           6845
    Resolution:     240x240, 256x240, 512x240
    Colors:         8

    http://www.devili.iki.fi/Computers/Luxor/
    http://hem.passagen.se/mani/abc/
*/

/*

    TODO:

	- rewrite Z80DART for bit level serial I/O
	- connect ABC77 keyboard to DART
    - COM port DIP switch
    - floppy controller board
    - Facit DTC (recased ABC-800?)
    - hard disks (ABC-850 10MB, ABC-852 20MB, ABC-856 60MB)

*/

/* Core includes */
#include "driver.h"
#include "includes/abc80x.h"

/* Components */
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "machine/centroni.h"
#include "includes/serial.h"
#include "machine/z80ctc.h"
#include "machine/z80sio.h"
#include "machine/z80dart.h"
#include "machine/abcbus.h"
#include "machine/e0516.h"
#include "machine/abc77.h"
#include "video/mc6845.h"

/* Devices */
#include "devices/basicdsk.h"
#include "devices/cassette.h"
#include "devices/printer.h"

/* Read/Write Handlers */

// ABC 800

static WRITE8_HANDLER( abc800_ram_ctrl_w )
{
}

// ABC 806

static void abc806_bankswitch(running_machine *machine)
{
	abc806_state *state = machine->driver_data;

	int bank;

	if (state->keydtr)
	{
		/* 32K block mapping */

		UINT16 videoram_offset = ((state->hrs >> 4) & 0x02) << 14;
		int videoram_bank = videoram_offset / 0x1000;

		for (bank = 1; bank <= 8; bank++)
		{
			/* 0x0000-0x7FFF is video RAM */

			UINT16 start_addr = 0x1000 * (bank - 1);
			UINT16 end_addr = start_addr + 0xfff;
			
			logerror("%04x-%04x: Video RAM bank %u (32K)\n", start_addr, end_addr, videoram_bank);

			memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, start_addr, end_addr, 0, 0, SMH_BANK(bank), SMH_BANK(bank));
			memory_set_bank(bank, videoram_bank + 1);

			videoram_bank++;
		}

		for (bank = 9; bank <= 16; bank++)
		{
			/* 0x8000-0xFFFF is main RAM */

			UINT16 start_addr = 0x1000 * (bank - 1);
			UINT16 end_addr = start_addr + 0xfff;
			
			logerror("%04x-%04x: Work RAM (32K)\n", start_addr, end_addr);

			memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, start_addr, end_addr, 0, 0, SMH_BANK(bank), SMH_BANK(bank));
			memory_set_bank(bank, 0);
		}
	}
	else
	{
		/* 4K block mapping */

		for (bank = 1; bank <= 16; bank++)
		{
			UINT16 start_addr = 0x1000 * (bank - 1);
			UINT16 end_addr = start_addr + 0xfff;
			UINT8 map = state->map[bank - 1];

			if (BIT(map, 7) && state->eme)
			{
				/* map to video RAM */
				int videoram_bank = map & 0x1f;

				logerror("%04x-%04x: Video RAM bank %u (4K)\n", start_addr, end_addr, videoram_bank);

				memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, start_addr, end_addr, 0, 0, SMH_BANK(bank), SMH_BANK(bank));
				memory_set_bank(bank, videoram_bank + 1);
			}
			else
			{
				/* map to ROM/RAM */

				switch (bank)
				{
				case 1: case 2: case 3: case 4: case 5: case 6: case 7:
					/* ROM */
					logerror("%04x-%04x: ROM (4K)\n", start_addr, end_addr);

					memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, start_addr, end_addr, 0, 0, SMH_BANK(bank), SMH_UNMAP);
					memory_set_bank(bank, 0);
					break;

				case 8:
					/* ROM/char RAM */
					logerror("%04x-%04x: ROM (4K)\n", start_addr, end_addr);

					memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x7000, 0x77ff, 0, 0, SMH_BANK(bank), SMH_UNMAP);
					memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x7800, 0x7fff, 0, 0, abc806_charram_r, abc806_charram_w);
					memory_set_bank(bank, 0);
					break;

				default:
					/* work RAM */
					logerror("%04x-%04x: Work RAM (4K)\n", start_addr, end_addr);

					memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, start_addr, end_addr, 0, 0, SMH_BANK(bank), SMH_BANK(bank));
					memory_set_bank(bank, 0);
					break;
				}
			}
		}
	}

	if (state->fetch_charram)
	{
		UINT16 videoram_offset = ((state->hrs >> 4) & 0x02) << 14;
		int videoram_bank = videoram_offset / 0x1000;

		for (bank = 1; bank <= 8; bank++)
		{
			/* 0x0000-0x77FF is video RAM */

			UINT16 start_addr = 0x1000 * (bank - 1);
			UINT16 end_addr = start_addr + 0xfff;
			
			logerror("%04x-%04x: Video RAM bank %u (30K)\n", start_addr, end_addr, videoram_bank);

			if (start_addr == 0x7000)
			{
				memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x7000, 0x77ff, 0, 0, SMH_BANK(bank), SMH_BANK(bank));
				memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x7800, 0x7fff, 0, 0, abc806_charram_r, abc806_charram_w);
			}
			else
			{
				memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, start_addr, end_addr, 0, 0, SMH_BANK(bank), SMH_BANK(bank));
			}

			memory_set_bank(bank, bank + videoram_bank);
		}
	}
}

static READ8_HANDLER( abc806_mai_r )
{
	abc806_state *state = machine->driver_data;

	int bank = offset >> 12;

	return state->map[bank];
}

static WRITE8_HANDLER( abc806_mao_w )
{
	/*

		bit		description

		0		physical block address bit 0
		1		physical block address bit 1
		2		physical block address bit 2
		3		physical block address bit 3
		4		physical block address bit 4
		5
		6
		7		allocate block

		- extended 128KB memory is divided into 32 physical blocks of 4KB each
		- logical CPU address space is divided into 16 logical blocks of 4KB each
		- extended blocks can be allocated to the logical blocks
		- logical address is provided by the offset (e.g write to port 0xc034 -> address 0xc000)

	*/

	abc806_state *state = machine->driver_data;

	int bank = offset >> 12;

	state->map[bank] = data;

	abc806_bankswitch(machine);
}

// Z80 SIO/2

static READ8_DEVICE_HANDLER( sio2_r )
{
	switch (offset)
	{
	case 0:
		return z80sio_d_r(device, 0);
	case 1:
		return z80sio_c_r(device, 0);
	case 2:
		return z80sio_d_r(device, 1);
	case 3:
		return z80sio_c_r(device, 1);
	}

	return 0;
}

static WRITE8_DEVICE_HANDLER( sio2_w )
{
	switch (offset)
	{
	case 0:
		z80sio_d_w(device, 0, data);
		break;
	case 1:
		z80sio_c_w(device, 0, data);
		break;
	case 2:
		z80sio_d_w(device, 1, data);
		break;
	case 3:
		z80sio_c_w(device, 1, data);
		break;
	}
}

// Z80 DART

static READ8_DEVICE_HANDLER( dart_r )
{
	switch (offset)
	{
	case 0:
		return z80dart_d_r(device, 0);
	case 1:
		return z80dart_c_r(device, 0);
	case 2:
		return z80dart_d_r(device, 1);
	case 3:
		return z80dart_c_r(device, 1);
	}

	return 0xff;
}

static WRITE8_DEVICE_HANDLER( dart_w )
{
	switch (offset)
	{
	case 0:
		z80dart_d_w(device, 0, data);
		break;
	case 1:
		z80dart_c_w(device, 0, data);
		break;
	case 2:
		z80dart_d_w(device, 1, data);
		break;
	case 3:
		z80dart_c_w(device, 1, data);
		break;
	}
}

/* Memory Maps */

// ABC 800M

static ADDRESS_MAP_START( abc800m_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x77ff) AM_ROM
	AM_RANGE(0x7800, 0x7fff) AM_RAM AM_BASE(&videoram)
	AM_RANGE(0x8000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( abc800m_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_MIRROR(0x18) AM_READWRITE(abcbus_data_r, abcbus_data_w)
	AM_RANGE(0x01, 0x01) AM_MIRROR(0x18) AM_READWRITE(abcbus_status_r, abcbus_channel_w)
	AM_RANGE(0x02, 0x05) AM_MIRROR(0x18) AM_WRITE(abcbus_command_w)
	AM_RANGE(0x06, 0x06) AM_MIRROR(0x18) AM_WRITE(abc800m_hrs_w)
	AM_RANGE(0x07, 0x07) AM_MIRROR(0x18) AM_READWRITE(abcbus_reset_r, abc800m_hrc_w)
	AM_RANGE(0x20, 0x23) AM_MIRROR(0x0c) AM_DEVREADWRITE(Z80DART, "z80dart", dart_r, dart_w)
	AM_RANGE(0x30, 0x32) AM_WRITE(abc800_ram_ctrl_w)
	AM_RANGE(0x31, 0x31) AM_MIRROR(0x06) AM_DEVREAD(MC6845, MC6845_TAG, mc6845_register_r)
	AM_RANGE(0x38, 0x38) AM_MIRROR(0x06) AM_DEVWRITE(MC6845, MC6845_TAG, mc6845_address_w)
	AM_RANGE(0x39, 0x39) AM_MIRROR(0x06) AM_DEVWRITE(MC6845, MC6845_TAG, mc6845_register_w)
	AM_RANGE(0x40, 0x43) AM_MIRROR(0x1c) AM_DEVREADWRITE(Z80SIO, "z80sio", sio2_r, sio2_w)
	AM_RANGE(0x50, 0x53) AM_MIRROR(0x1c) AM_DEVREADWRITE(Z80CTC, "z80ctc", z80ctc_r, z80ctc_w)
	AM_RANGE(0x80, 0x80) AM_MIRROR(0x7f) AM_READWRITE(abcbus_strobe_r, abcbus_strobe_w)
ADDRESS_MAP_END

// ABC 800C

static ADDRESS_MAP_START( abc800c_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x0000, 0x77ff) AM_ROM
	AM_RANGE(0x7800, 0x7bff) AM_RAM AM_BASE(&videoram)
	AM_RANGE(0x7800, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( abc800c_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_MIRROR(0x18) AM_READWRITE(abcbus_data_r, abcbus_data_w)
	AM_RANGE(0x01, 0x01) AM_MIRROR(0x18) AM_READWRITE(abcbus_status_r, abcbus_channel_w)
	AM_RANGE(0x02, 0x05) AM_MIRROR(0x18) AM_WRITE(abcbus_command_w)
	AM_RANGE(0x06, 0x06) AM_MIRROR(0x18) AM_WRITE(abc800c_hrs_w)
	AM_RANGE(0x07, 0x07) AM_MIRROR(0x18) AM_READWRITE(abcbus_reset_r, abc800c_hrc_w)
	AM_RANGE(0x20, 0x23) AM_MIRROR(0x0c) AM_DEVREADWRITE(Z80DART, "z80dart", dart_r, dart_w)
	AM_RANGE(0x30, 0x32) AM_WRITE(abc800_ram_ctrl_w)
	AM_RANGE(0x31, 0x31) AM_MIRROR(0x06) AM_DEVREAD(MC6845, MC6845_TAG, mc6845_register_r)
	AM_RANGE(0x38, 0x38) AM_MIRROR(0x06) AM_DEVWRITE(MC6845, MC6845_TAG, mc6845_address_w)
	AM_RANGE(0x39, 0x39) AM_MIRROR(0x06) AM_DEVWRITE(MC6845, MC6845_TAG, mc6845_register_w)
	AM_RANGE(0x40, 0x43) AM_MIRROR(0x1c) AM_DEVREADWRITE(Z80SIO, "z80sio", sio2_r, sio2_w)
	AM_RANGE(0x50, 0x53) AM_MIRROR(0x1c) AM_DEVREADWRITE(Z80CTC, "z80ctc", z80ctc_r, z80ctc_w)
	AM_RANGE(0x80, 0x80) AM_MIRROR(0x7f) AM_READWRITE(abcbus_strobe_r, abcbus_strobe_w)
ADDRESS_MAP_END

// ABC 802

static ADDRESS_MAP_START( abc802_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x77ff) AM_RAMBANK(1)
	AM_RANGE(0x7800, 0x7fff) AM_RAM AM_BASE(&videoram)
	AM_RANGE(0x8000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( abc802_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_READWRITE(abcbus_data_r, abcbus_data_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(abcbus_status_r, abcbus_channel_w)
	AM_RANGE(0x02, 0x05) AM_WRITE(abcbus_command_w)
	AM_RANGE(0x07, 0x07) AM_READ(abcbus_reset_r)
	AM_RANGE(0x20, 0x23) AM_DEVREADWRITE(Z80DART, "z80dart", dart_r, dart_w)
	AM_RANGE(0x31, 0x31) AM_DEVREAD(MC6845, MC6845_TAG, mc6845_register_r)
	AM_RANGE(0x32, 0x35) AM_DEVREADWRITE(Z80SIO, "z80sio", sio2_r, sio2_w)
	AM_RANGE(0x38, 0x38) AM_DEVWRITE(MC6845, MC6845_TAG, mc6845_address_w)
	AM_RANGE(0x39, 0x39) AM_DEVWRITE(MC6845, MC6845_TAG, mc6845_register_w)
	AM_RANGE(0x60, 0x63) AM_DEVREADWRITE(Z80CTC, "z80ctc", z80ctc_r, z80ctc_w)
	AM_RANGE(0x80, 0xff) AM_READWRITE(abcbus_strobe_r, abcbus_strobe_w)
ADDRESS_MAP_END

// ABC 806

static ADDRESS_MAP_START( abc806_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x0fff) AM_RAMBANK(1)
	AM_RANGE(0x1000, 0x1fff) AM_RAMBANK(2)
	AM_RANGE(0x2000, 0x2fff) AM_RAMBANK(3)
	AM_RANGE(0x3000, 0x3fff) AM_RAMBANK(4)
	AM_RANGE(0x4000, 0x4fff) AM_RAMBANK(5)
	AM_RANGE(0x5000, 0x5fff) AM_RAMBANK(6)
	AM_RANGE(0x6000, 0x6fff) AM_RAMBANK(7)
	AM_RANGE(0x7000, 0x7fff) AM_RAMBANK(8)
	AM_RANGE(0x8000, 0x8fff) AM_RAMBANK(9)
	AM_RANGE(0x9000, 0x9fff) AM_RAMBANK(10)
	AM_RANGE(0xa000, 0xafff) AM_RAMBANK(11)
	AM_RANGE(0xb000, 0xbfff) AM_RAMBANK(12)
	AM_RANGE(0xc000, 0xcfff) AM_RAMBANK(13)
	AM_RANGE(0xd000, 0xdfff) AM_RAMBANK(14)
	AM_RANGE(0xe000, 0xefff) AM_RAMBANK(15)
	AM_RANGE(0xf000, 0xffff) AM_RAMBANK(16)
ADDRESS_MAP_END

static ADDRESS_MAP_START( abc806_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
//	AM_RANGE(0x00, 0x00) AM_MIRROR(0xff1f) AM_READWRITE(abcbus_strobe_r, abcbus_strobe_w)
	AM_RANGE(0x00, 0x00) AM_MIRROR(0xff18) AM_READWRITE(abcbus_data_r, abcbus_data_w)
	AM_RANGE(0x01, 0x01) AM_MIRROR(0xff18) AM_READWRITE(abcbus_status_r, abcbus_channel_w)
	AM_RANGE(0x02, 0x05) AM_MIRROR(0xff18) AM_WRITE(abcbus_command_w)
	AM_RANGE(0x06, 0x06) AM_MIRROR(0xff18) AM_WRITE(abc806_hrs_w)
	AM_RANGE(0x07, 0x07) AM_MIRROR(0xff18) AM_READWRITE(abcbus_reset_r, abc806_hrc_w)
	AM_RANGE(0x20, 0x23) AM_MIRROR(0xff0c) AM_DEVREADWRITE(Z80DART, "z80dart", dart_r, dart_w)
	AM_RANGE(0x31, 0x31) AM_MIRROR(0xff06) AM_DEVREAD(MC6845, MC6845_TAG, mc6845_register_r)
	AM_RANGE(0x34, 0x34) AM_MIRROR(0xff00) AM_MASK(0xff00) AM_READWRITE(abc806_mai_r, abc806_mao_w)
	AM_RANGE(0x35, 0x35) AM_MIRROR(0xff00) AM_READWRITE(abc806_ami_r, abc806_amo_w)
	AM_RANGE(0x36, 0x36) AM_MIRROR(0xff00) AM_WRITE(abc806_sso_w)
	AM_RANGE(0x37, 0x37) AM_MIRROR(0xff00) AM_READWRITE(abc806_cli_r, abc806_sto_w)
	AM_RANGE(0x38, 0x38) AM_MIRROR(0xff06) AM_DEVWRITE(MC6845, MC6845_TAG, mc6845_address_w)
	AM_RANGE(0x39, 0x39) AM_MIRROR(0xff06) AM_DEVWRITE(MC6845, MC6845_TAG, mc6845_register_w)
	AM_RANGE(0x40, 0x41) AM_MIRROR(0xff1c) AM_DEVREADWRITE(Z80SIO, "z80sio", sio2_r, sio2_w)
	AM_RANGE(0x60, 0x63) AM_MIRROR(0xff1c) AM_DEVREADWRITE(Z80CTC, "z80ctc", z80ctc_r, z80ctc_w)
	AM_RANGE(0x80, 0x80) AM_MIRROR(0xff7f) AM_READWRITE(abcbus_strobe_r, abcbus_strobe_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( abc800 )
	PORT_INCLUDE(abc77)
INPUT_PORTS_END

static INPUT_PORTS_START( abc802 )
	PORT_INCLUDE(abc77)
INPUT_PORTS_END

static INPUT_PORTS_START( abc806 )
	PORT_INCLUDE(abc77)
INPUT_PORTS_END

/* Machine Initialization */

static ABC77_ON_TXD_CHANGED( abc800_abc77_txd_changed )
{
	abc800_state *state = device->machine->driver_data;

	state->abc77_txd = level;
}

static ABC77_ON_KEYDOWN_CHANGED( abc800_abc77_keydown_changed )
{
	const device_config *z80dart = device_list_find_by_tag(device->machine->config->devicelist, Z80DART, "z80dart");

	z80dart_set_dcd(z80dart, 1, level);
}

static ABC77_INTERFACE( abc800_abc77_intf )
{
	abc800_abc77_txd_changed,
	abc800_abc77_keydown_changed
};

static WRITE8_DEVICE_HANDLER( abc800_ctc_z2_w )
{
	// write to DART channel A RxC/TxC
}

static const z80ctc_interface abc800_ctc_intf =
{
	"main",					/* cpu */
	ABC800_X01/2/2,			/* clock */
	0,              		/* timer disables */
	0,				  		/* interrupt handler */
	0,						/* ZC/TO0 callback */
	0,              		/* ZC/TO1 callback */
	abc800_ctc_z2_w    		/* ZC/TO2 callback */
};

static WRITE8_DEVICE_HANDLER( sio_serial_transmit )
{
}

static int sio_serial_receive( const device_config *device, int channel )
{
	return -1;
}

static z80sio_interface abc800_sio_intf =
{
	"main",					/* cpu */
	ABC800_X01/2/2,			/* clock */
	0,						/* interrupt handler */
	0,						/* DTR changed handler */
	0,						/* RTS changed handler */
	0,						/* BREAK changed handler */
	sio_serial_transmit,	/* transmit handler */
	sio_serial_receive		/* receive handler */
};

static WRITE8_DEVICE_HANDLER( dart_serial_transmit )
{
}

static int dart_serial_receive(const device_config *device, int ch)
{
	return -1;
}

static const z80dart_interface abc800_dart_intf =
{
	"main",					/* cpu */
	ABC800_X01/2/2,			/* clock */
	0,						/* interrupt handler */
	0,						/* DTR changed handler */
	0,						/* RTS changed handler */
	0,						/* BREAK changed handler */
	dart_serial_transmit,	/* transmit handler */
	dart_serial_receive		/* receive handler */
};

static const z80_daisy_chain abc800_daisy_chain[] =
{
	{ Z80CTC, "z80ctc" },
	{ Z80SIO, "z80sio" },
	{ Z80DART, "z80dart" },
	{ NULL }
};

static TIMER_CALLBACK(abc800_ctc_tick)
{
	const device_config *device = ptr;

	z80ctc_trg_w(device, 0, 1);
	z80ctc_trg_w(device, 0, 0);
	z80ctc_trg_w(device, 1, 1);
	z80ctc_trg_w(device, 1, 0);
	z80ctc_trg_w(device, 2, 1);
	z80ctc_trg_w(device, 2, 0);
}

static MACHINE_START( abc800 )
{
	abc800_state *state = machine->driver_data;

	/* allocate timer */

	state->ctc_timer = timer_alloc(abc800_ctc_tick, (void *)device_list_find_by_tag(machine->config->devicelist, Z80CTC, "z80ctc") );
	timer_adjust_periodic(state->ctc_timer, attotime_zero, 0, ATTOTIME_IN_HZ(ABC800_X01/2/2/2));
}

// ABC802

static ABC77_ON_TXD_CHANGED( abc802_abc77_txd_changed )
{
	abc802_state *state = device->machine->driver_data;

	state->abc77_txd = level;
}

static ABC77_ON_KEYDOWN_CHANGED( abc802_abc77_keydown_changed )
{
	const device_config *z80dart = device_list_find_by_tag(device->machine->config->devicelist, Z80DART, "z80dart");

	z80dart_set_dcd(z80dart, 1, level);
}

static ABC77_INTERFACE( abc802_abc77_intf )
{
	abc802_abc77_txd_changed,
	abc802_abc77_keydown_changed
};

static WRITE8_DEVICE_HANDLER( abc802_dart_dtr_w )
{
	if (offset == 1)
	{
		memory_set_bank(1, data);
	}
}

static WRITE8_DEVICE_HANDLER( abc802_dart_rts_w )
{
	abc802_state *state = device->machine->driver_data;

	if (offset == 1)
	{
		state->mux80_40 = !BIT(data, 0);
	}
}

static const z80dart_interface abc802_dart_intf =
{
	"main",					/* cpu */
	ABC800_X01/2/2,			/* clock */
	0,						/* interrupt handler */
	abc802_dart_dtr_w,		/* DTR changed handler */
	abc802_dart_rts_w,		/* RTS changed handler */
	0,						/* BREAK changed handler */
	dart_serial_transmit,	/* transmit handler */
	dart_serial_receive		/* receive handler */
};

static MACHINE_START( abc802 )
{
	abc802_state *state = machine->driver_data;

	/* allocate timer */

	state->ctc_timer = timer_alloc(abc800_ctc_tick, (void *)device_list_find_by_tag(machine->config->devicelist, Z80CTC, "z80ctc") );
	timer_adjust_periodic(state->ctc_timer, attotime_zero, 0, ATTOTIME_IN_HZ(ABC800_X01/2/2/2));

	/* configure memory */

	memory_configure_bank(1, 0, 1, memory_region(machine, "main"), 0);
	memory_configure_bank(1, 1, 1, mess_ram, 0);
}

static MACHINE_RESET( abc802 )
{
	memory_set_bank(1, 0);
}

// ABC806

static ABC77_ON_TXD_CHANGED( abc806_abc77_txd_changed )
{
	abc806_state *state = device->machine->driver_data;

	state->abc77_txd = level;
}

static ABC77_ON_KEYDOWN_CHANGED( abc806_abc77_keydown_changed )
{
	const device_config *z80dart = device_list_find_by_tag(device->machine->config->devicelist, Z80DART, "z80dart");

	z80dart_set_dcd(z80dart, 1, level);
}

static ABC77_INTERFACE( abc806_abc77_intf )
{
	abc806_abc77_txd_changed,
	abc806_abc77_keydown_changed
};

static const e0516_interface abc806_e0516_intf =
{
	ABC806_X02
};

static WRITE8_DEVICE_HANDLER( abc806_dart_dtr_w )
{
	abc806_state *state = device->machine->driver_data;
	
	if (offset == 1)
	{
		state->keydtr = BIT(data, 0);
	}
}

static const z80dart_interface abc806_dart_intf =
{
	"main",					/* cpu */
	ABC800_X01/2/2,			/* clock */
	0,						/* interrupt handler */
	abc806_dart_dtr_w,		/* DTR changed handler */
	0,						/* RTS changed handler */
	0,						/* BREAK changed handler */
	dart_serial_transmit,	/* transmit handler */
	dart_serial_receive		/* receive handler */
};

static MACHINE_START( abc806 )
{
	abc806_state *state = machine->driver_data;

	UINT8 *mem = memory_region(machine, "main");
	int bank;

	/* allocate timer */

	state->ctc_timer = timer_alloc(abc800_ctc_tick, (void *)device_list_find_by_tag(machine->config->devicelist, Z80CTC, "z80ctc") );
	timer_adjust_periodic(state->ctc_timer, attotime_zero, 0, ATTOTIME_IN_HZ(ABC800_X01/2/2/2));

	/* setup memory banking */

	state->videoram = auto_malloc(ABC806_VIDEO_RAM_SIZE);

	for (bank = 1; bank <= 16; bank++)
	{
		memory_configure_bank(bank, 0, 1, mem + (0x1000 * (bank - 1)), 0);
		memory_configure_bank(bank, 1, 32, state->videoram, 0x1000);

		memory_set_bank(bank, 0);
	}

	/* register for state saving */

	state_save_register_global_array(state->map);
	state_save_register_global(state->keydtr);
}

static MACHINE_RESET( abc806 )
{
	/* setup memory banking */

	int bank;

	for (bank = 1; bank <= 16; bank++)
	{
		memory_set_bank(bank, 0);
	}

	abc806_bankswitch(machine);
}

/* Machine Drivers */

static MACHINE_DRIVER_START( abc800m )
	MDRV_DRIVER_DATA(abc800_state)

	/* basic machine hardware */
	MDRV_CPU_ADD("main", Z80, ABC800_X01/2/2)
	MDRV_CPU_CONFIG(abc800_daisy_chain)
	MDRV_CPU_PROGRAM_MAP(abc800m_map, 0)
	MDRV_CPU_IO_MAP(abc800m_io_map, 0)

	MDRV_MACHINE_START(abc800)

	/* ABC-77 keyboard */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_DEVICE_ADD(ABC77_TAG, ABC77)
	MDRV_DEVICE_CONFIG(abc800_abc77_intf)

	/* video hardware */
	MDRV_IMPORT_FROM(abc800m_video)
	
	MDRV_Z80CTC_ADD( "z80ctc", abc800_ctc_intf )
	MDRV_Z80SIO_ADD( "z80sio", abc800_sio_intf )
	MDRV_Z80DART_ADD( "z80dart", abc800_dart_intf )

	/* printer */
	MDRV_DEVICE_ADD("printer", PRINTER)

	/* cassette */
	MDRV_CASSETTE_ADD( "cassette", default_cassette_config )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( abc800c )
	MDRV_DRIVER_DATA(abc800_state)

	/* basic machine hardware */
	MDRV_CPU_ADD("main", Z80, ABC800_X01/2/2)
	MDRV_CPU_CONFIG(abc800_daisy_chain)
	MDRV_CPU_PROGRAM_MAP(abc800c_map, 0)
	MDRV_CPU_IO_MAP(abc800c_io_map, 0)

	MDRV_MACHINE_START(abc800)

	/* ABC-77 keyboard */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_DEVICE_ADD(ABC77_TAG, ABC77)
	MDRV_DEVICE_CONFIG(abc800_abc77_intf)

	/* video hardware */
	MDRV_IMPORT_FROM(abc800c_video)

	MDRV_Z80CTC_ADD( "z80ctc", abc800_ctc_intf )
	MDRV_Z80SIO_ADD( "z80sio", abc800_sio_intf )
	MDRV_Z80DART_ADD( "z80dart", abc800_dart_intf )

	/* printer */
	MDRV_DEVICE_ADD("printer", PRINTER)

	/* cassette */
	MDRV_CASSETTE_ADD( "cassette", default_cassette_config )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( abc802 )
	MDRV_DRIVER_DATA(abc802_state)

	/* basic machine hardware */
	MDRV_CPU_ADD("main", Z80, ABC800_X01/2/2)
	MDRV_CPU_CONFIG(abc800_daisy_chain)
	MDRV_CPU_PROGRAM_MAP(abc802_map, 0)
	MDRV_CPU_IO_MAP(abc802_io_map, 0)

	MDRV_MACHINE_START(abc802)
	MDRV_MACHINE_RESET(abc802)

	/* ABC-77 keyboard */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_DEVICE_ADD(ABC77_TAG, ABC77)
	MDRV_DEVICE_CONFIG(abc802_abc77_intf)

	/* video hardware */
	MDRV_IMPORT_FROM(abc802_video)

	MDRV_Z80CTC_ADD( "z80ctc", abc800_ctc_intf )
	MDRV_Z80SIO_ADD( "z80sio", abc800_sio_intf )
	MDRV_Z80DART_ADD( "z80dart", abc802_dart_intf )

	/* printer */
	MDRV_DEVICE_ADD("printer", PRINTER)

	/* cassette */
	MDRV_CASSETTE_ADD( "cassette", default_cassette_config )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( abc806 )
	MDRV_DRIVER_DATA(abc806_state)

	/* basic machine hardware */
	MDRV_CPU_ADD("main", Z80, ABC800_X01/2/2)
	MDRV_CPU_CONFIG(abc800_daisy_chain)
	MDRV_CPU_PROGRAM_MAP(abc806_map, 0)
	MDRV_CPU_IO_MAP(abc806_io_map, 0)

	MDRV_MACHINE_START(abc806)
	MDRV_MACHINE_RESET(abc806)

	/* ABC-77 keyboard */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_DEVICE_ADD(ABC77_TAG, ABC77)
	MDRV_DEVICE_CONFIG(abc806_abc77_intf)

	/* video hardware */
	MDRV_IMPORT_FROM(abc806_video)

	MDRV_Z80CTC_ADD( "z80ctc", abc800_ctc_intf )
	MDRV_Z80SIO_ADD( "z80sio", abc800_sio_intf )
	MDRV_Z80DART_ADD( "z80dart", abc800_dart_intf )

	/* real time clock */
	MDRV_DEVICE_ADD(E0516_TAG, E0516)
	MDRV_DEVICE_CONFIG(abc806_e0516_intf)

	/* printer */
	MDRV_DEVICE_ADD("printer", PRINTER)

	/* cassette */
	MDRV_CASSETTE_ADD( "cassette", default_cassette_config )
MACHINE_DRIVER_END

/* ROMs */

/*

    ABC800 DOS ROMs

    Label       Drive Type
    ----------------------------
    ABC 6-1X    ABC830,DD82,DD84
    800 8"      DD88
    ABC 6-2X    ABC832
    ABC 6-3X    ABC838
    UFD 6.XX    Winchester

*/

#define ROM_ABC99 \
	ROM_REGION( 0x1800, "abc99", 0 ) \
	ROM_LOAD( "10681909",  0x0000, 0x1000, CRC(ffe32a71) SHA1(fa2ce8e0216a433f9bbad0bdd6e3dc0b540f03b7) ) \
	ROM_LOAD( "10726864",  0x1000, 0x0800, CRC(e33683ae) SHA1(0c1d9e320f82df05f4804992ef6f6f6cd20623f3), BIOS(1) ) \
	ROM_LOAD( "abc99.bin", 0x1000, 0x0800, CRC(d48310fc) SHA1(17a2ffc0ec00d395c2b9caf3d57fed575ba2b137), BIOS(2) )

ROM_START( abc800c )
	ROM_REGION( 0x10000, "main", 0 )
	ROM_LOAD( "abcc.1m",    0x0000, 0x1000, NO_DUMP )
	ROM_LOAD( "abc1-12.1l", 0x1000, 0x1000, CRC(1e99fbdc) SHA1(ec6210686dd9d03a5ed8c4a4e30e25834aeef71d) )
	ROM_LOAD( "abc2-12.1k", 0x2000, 0x1000, CRC(ac196ba2) SHA1(64fcc0f03fbc78e4c8056e1fa22aee12b3084ef5) )
	ROM_LOAD( "abc3-12.1j", 0x3000, 0x1000, CRC(3ea2b5ee) SHA1(5a51ac4a34443e14112a6bae16c92b5eb636603f) )
	ROM_LOAD( "abc4-12.2m", 0x4000, 0x1000, CRC(695cb626) SHA1(9603ce2a7b2d7b1cbeb525f5493de7e5c1e5a803) )
	ROM_LOAD( "abc5-12.2l", 0x5000, 0x1000, CRC(b4b02358) SHA1(95338efa3b64b2a602a03bffc79f9df297e9534a) )
	ROM_LOAD( "abc6-13.2k", 0x6000, 0x1000, CRC(6fa71fb6) SHA1(b037dfb3de7b65d244c6357cd146376d4237dab6) )
	ROM_LOAD( "abc7-21.2j", 0x7000, 0x1000, CRC(fd137866) SHA1(3ac914d90db1503f61397c0ea26914eb38725044) )

	ROM_REGION( 0x1000, "chargen", 0 )
	ROM_LOAD( "vuc-se.7c",  0x0000, 0x1000, NO_DUMP )

	ROM_REGION( 0x2000, "user1", 0 )
	// Fast Controller
	ROM_LOAD( "fast108.bin",  0x0000, 0x2000, CRC(229764cb) SHA1(a2e2f6f49c31b827efc62f894de9a770b65d109d) ) // Luxor v1.08
	ROM_LOAD( "fast207.bin",  0x0000, 0x2000, CRC(86622f52) SHA1(61ad271de53152c1640c0b364fce46d1b0b4c7e2) ) // DIAB v2.07

	// MyAB Turbo-Kontroller
	ROM_LOAD( "unidis5d.bin", 0x0000, 0x1000, CRC(569dd60c) SHA1(47b810bcb5a063ffb3034fd7138dc5e15d243676) ) // 5" 25-pin
	ROM_LOAD( "unidiskh.bin", 0x0000, 0x1000, CRC(5079ad85) SHA1(42bb91318f13929c3a440de3fa1f0491a0b90863) ) // 5" 34-pin
	ROM_LOAD( "unidisk8.bin", 0x0000, 0x1000, CRC(d04e6a43) SHA1(8db504d46ff0355c72bd58fd536abeb17425c532) ) // 8"

	// ABC-832
	ROM_LOAD( "micr1015.bin", 0x0000, 0x0800, CRC(a7bc05fa) SHA1(6ac3e202b7ce802c70d89728695f1cb52ac80307) ) // Micropolis 1015
	ROM_LOAD( "micr1115.bin", 0x0000, 0x0800, CRC(f2fc5ccc) SHA1(86d6baadf6bf1d07d0577dc1e092850b5ff6dd1b) ) // Micropolis 1115
	ROM_LOAD( "basf6118.bin", 0x0000, 0x0800, CRC(9ca1a1eb) SHA1(04973ad69de8da403739caaebe0b0f6757e4a6b1) ) // BASF 6118

	// ABC-850
	ROM_LOAD( "rodi202.bin",  0x0000, 0x0800, CRC(337b4dcf) SHA1(791ebeb4521ddc11fb9742114018e161e1849bdf) ) // Rodime 202
	ROM_LOAD( "basf6185.bin", 0x0000, 0x0800, CRC(06f8fe2e) SHA1(e81f2a47c854e0dbb096bee3428d79e63591059d) ) // BASF 6185

	// ABC-852
	ROM_LOAD( "nec5126.bin",  0x0000, 0x1000, CRC(17c247e7) SHA1(7339738b87751655cb4d6414422593272fe72f5d) ) // NEC 5126

	// ABC-856
	ROM_LOAD( "micr1325.bin", 0x0000, 0x0800, CRC(084af409) SHA1(342b8e214a8c4c2b014604e53c45ef1bd1c69ea3) ) // Micropolis 1325

	// unknown
	ROM_LOAD( "st4038.bin",   0x0000, 0x0800, CRC(4c803b87) SHA1(1141bb51ad9200fc32d92a749460843dc6af8953) ) // Seagate ST4038
	ROM_LOAD( "st225.bin",    0x0000, 0x0800, CRC(c9f68f81) SHA1(7ff8b2a19f71fe0279ab3e5a0a5fffcb6030360c) ) // Seagate ST225
ROM_END

ROM_START( abc800m )
	ROM_REGION( 0x10000, "main", 0 )
	ROM_LOAD( "abcm.1m",    0x0000, 0x1000, CRC(f85b274c) SHA1(7d0f5639a528d8d8130a22fe688d3218c77839dc) )
	ROM_LOAD( "abc1-12.1l", 0x1000, 0x1000, CRC(1e99fbdc) SHA1(ec6210686dd9d03a5ed8c4a4e30e25834aeef71d) )
	ROM_LOAD( "abc2-12.1k", 0x2000, 0x1000, CRC(ac196ba2) SHA1(64fcc0f03fbc78e4c8056e1fa22aee12b3084ef5) )
	ROM_LOAD( "abc3-12.1j", 0x3000, 0x1000, CRC(3ea2b5ee) SHA1(5a51ac4a34443e14112a6bae16c92b5eb636603f) )
	ROM_LOAD( "abc4-12.2m", 0x4000, 0x1000, CRC(695cb626) SHA1(9603ce2a7b2d7b1cbeb525f5493de7e5c1e5a803) )
	ROM_LOAD( "abc5-12.2l", 0x5000, 0x1000, CRC(b4b02358) SHA1(95338efa3b64b2a602a03bffc79f9df297e9534a) )
	ROM_LOAD( "abc6-13.2k", 0x6000, 0x1000, CRC(6fa71fb6) SHA1(b037dfb3de7b65d244c6357cd146376d4237dab6) )
	ROM_LOAD( "abc7-21.2j", 0x7000, 0x1000, CRC(fd137866) SHA1(3ac914d90db1503f61397c0ea26914eb38725044) )

	ROM_REGION( 0x0800, "chargen", 0 )
	ROM_LOAD( "vum-se.7c",  0x0000, 0x0800, CRC(f9152163) SHA1(997313781ddcbbb7121dbf9eb5f2c6b4551fc799) )
ROM_END

ROM_START( abc802 )
	ROM_REGION( 0x10000, "main", 0 )
	ROM_LOAD(  "abc02-11.9f",  0x0000, 0x2000, CRC(b86537b2) SHA1(4b7731ef801f9a03de0b5acd955f1e4a1828355d) )
	ROM_LOAD(  "abc12-11.11f", 0x2000, 0x2000, CRC(3561c671) SHA1(f12a7c0fe5670ffed53c794d96eb8959c4d9f828) )
	ROM_LOAD(  "abc22-11.12f", 0x4000, 0x2000, CRC(8dcb1cc7) SHA1(535cfd66c84c0370fd022d6edf702d3d1ad1b113) )
	ROM_SYSTEM_BIOS( 0, "v19",		"UDF-DOS v6.19" )
	ROMX_LOAD( "abc32-21.14f", 0x6000, 0x2000, CRC(57050b98) SHA1(b977e54d1426346a97c98febd8a193c3e8259574), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "v20",		"UDF-DOS v6.20" )
	ROMX_LOAD( "abc32-31.14f", 0x6000, 0x2000, CRC(fc8be7a8) SHA1(a1d4cb45cf5ae21e636dddfa70c99bfd2050ad60), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "mica",		"MICA DOS v6.20" )
	ROMX_LOAD( "mica820.14f",  0x6000, 0x2000, CRC(edf998af) SHA1(daae7e1ff6ef3e0ddb83e932f324c56f4a98f79b), ROM_BIOS(3) )

	ROM_REGION( 0x2000, "chargen", 0 )
	ROM_LOAD( "abct2-11.3g",  0x0000, 0x2000, CRC(e21601ee) SHA1(2e838ebd7692e5cb9ba4e80fe2aa47ea2584133a) ) // 64 90191-01

	ROM_REGION( 0x400, "plds", 0 )
	ROM_LOAD( "abcp2-11.2g", 0x0000, 0x0400, NO_DUMP ) // PAL16R4
ROM_END

ROM_START( abc806 )
	ROM_REGION( 0x10000, "main", 0 )
	ROM_LOAD( "abc 06-11.1m",  0x0000, 0x1000, CRC(27083191) SHA1(9b45592273a5673e4952c6fe7965fc9398c49827) ) // BASIC PROM ABC 06-11 "64 90231-02"
	ROM_LOAD( "abc 16-11.1l",  0x1000, 0x1000, CRC(eb0a08fd) SHA1(f0b82089c5c8191fbc6a3ee2c78ce730c7dd5145) ) // BASIC PROM ABC 16-11 "64 90232-02"
	ROM_LOAD( "abc 26-11.1k",  0x2000, 0x1000, CRC(97a95c59) SHA1(013bc0a2661f4630c39b340965872bf607c7bd45) ) // BASIC PROM ABC 26-11 "64 90233-02"
	ROM_LOAD( "abc 36-11.1j",  0x3000, 0x1000, CRC(b50e418e) SHA1(991a59ed7796bdcfed310012b2bec50f0b8df01c) ) // BASIC PROM ABC 36-11 "64 90234-02"
	ROM_LOAD( "abc 46-11.2m",  0x4000, 0x1000, CRC(17a87c7d) SHA1(49a7c33623642b49dea3d7397af5a8b9dde8185b) ) // BASIC PROM ABC 46-11 "64 90235-02"
	ROM_LOAD( "abc 56-11.2l",  0x5000, 0x1000, CRC(b4b02358) SHA1(95338efa3b64b2a602a03bffc79f9df297e9534a) ) // BASIC PROM ABC 56-11 "64 90236-02"
	ROM_SYSTEM_BIOS( 0, "v19",		"UDF-DOS v.19" )
	ROMX_LOAD( "abc 66-21.2k", 0x6000, 0x1000, CRC(c311b57a) SHA1(4bd2a541314e53955a0d53ef2f9822a202daa485), ROM_BIOS(1) ) // DOS PROM ABC 66-21 "64 90314-01"
	ROM_SYSTEM_BIOS( 1, "v20",		"UDF-DOS v.20" )
	ROMX_LOAD( "abc 66-31.2k", 0x6000, 0x1000, CRC(a2e38260) SHA1(0dad83088222cb076648e23f50fec2fddc968883), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "mica",		"MICA DOS v.20" )
	ROMX_LOAD( "mica2006.2k", 0x6000, 0x1000, CRC(58bc2aa8) SHA1(0604bd2396f7d15fcf3d65888b4b673f554037c0), ROM_BIOS(3) )
	ROM_SYSTEM_BIOS( 3, "catnet",	"CAT-NET" )
	ROMX_LOAD( "cmd8_5.2k",	  0x6000, 0x1000, CRC(25430ef7) SHA1(03a36874c23c215a19b0be14ad2f6b3b5fb2c839), ROM_BIOS(4) )
	ROM_LOAD( "abc 76-11.2j",  0x7000, 0x1000, CRC(3eb5f6a1) SHA1(02d4e38009c71b84952eb3b8432ad32a98a7fe16) ) // Options-PROM ABC 76-11 "64 90238-02"

	ROM_REGION( 0x1000, "chargen", 0 )
	ROM_LOAD( "abc t6-11.7c",   0x0000, 0x1000, CRC(b17c51c5) SHA1(e466e80ec989fbd522c89a67d274b8f0bed1ff72) ) // 64 90243-01

	ROM_REGION( 0x200, "rad", 0 )
	ROM_LOAD( "64 90241-01.9b",  0x0000, 0x0200, NO_DUMP ) // "RAD" 7621/7643 (82S131/82S137), character line address

	ROM_REGION( 0x20, "hru", 0 )
	ROM_LOAD( "64 90128-01.6e",  0x0000, 0x0020, NO_DUMP ) // "HRU I" 7603 (82S123), HR horizontal timing and video memory access

	ROM_REGION( 0x200, "hru2", 0 )
	ROM_LOAD( "64 90127-01.12g", 0x0000, 0x0200, NO_DUMP ) // "HRU II" 7621 (82S131), ABC800C HR compatibility mode palette

	ROM_REGION( 0x200, "v50", 0 )
	ROM_LOAD( "64 90242-01.7e",  0x0000, 0x0200, NO_DUMP ) // "V50" 7621 (82S131), HR vertical timing 50Hz
//	ROM_LOAD( "64 90319-01.7e",  0x0000, 0x0200, NO_DUMP ) // "V60" 7621 (82S131), HR vertical timing 60Hz

	ROM_REGION( 0x400, "plds", 0 )
	ROM_LOAD( "64 90225-01.11c", 0x0000, 0x0400, NO_DUMP ) // "VIDEO ATTRIBUTE" 40033A (?)
	ROM_LOAD( "64 90239-01.1b", 0x0000, 0x0400, NO_DUMP ) // "ABC P3-11" PAL16R4, color encoder
	ROM_LOAD( "64 90240-01.2d", 0x0000, 0x0400, NO_DUMP ) // "ABC P4-11" PAL16L8, memory mapper
ROM_END

/* System Configuration */

static void abc800_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:						info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:							info->load = DEVICE_IMAGE_LOAD_NAME(abc_floppy); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "dsk"); break;

		default:											legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}

static DEVICE_IMAGE_LOAD( abc800_serial )
{
	/* filename specified */
	if (device_load_serial_device(image)==INIT_PASS)
	{
		/* setup transmit parameters */
		serial_device_setup(image, 9600 >> input_port_read(image->machine, "BAUD"), 8, 1, SERIAL_PARITY_NONE);

		serial_device_set_protocol(image, SERIAL_PROTOCOL_NONE);

		/* and start transmit */
		serial_device_set_transmit_state(image, 1);

		return INIT_PASS;
	}

	return INIT_FAIL;
}

static void abc800_serial_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* serial */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_TYPE:							info->i = IO_SERIAL; break;
		case MESS_DEVINFO_INT_COUNT:						info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_START:						info->start = DEVICE_START_NAME(serial_device); break;
		case MESS_DEVINFO_PTR_LOAD:							info->load = DEVICE_IMAGE_LOAD_NAME(abc800_serial); break;
		case MESS_DEVINFO_PTR_UNLOAD:						info->unload = DEVICE_IMAGE_UNLOAD_NAME(serial_device); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "txt"); break;
	}
}

static SYSTEM_CONFIG_START( abc800 )
	CONFIG_RAM_DEFAULT(16 * 1024)
	CONFIG_RAM		  (32 * 1024)
	CONFIG_DEVICE(abc800_floppy_getinfo)
	CONFIG_DEVICE(abc800_serial_getinfo)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START( abc802 )
	CONFIG_RAM_DEFAULT(64 * 1024)
	CONFIG_DEVICE(abc800_floppy_getinfo)
	CONFIG_DEVICE(abc800_serial_getinfo)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START( abc806 )
	CONFIG_RAM_DEFAULT(160 * 1024)
	CONFIG_DEVICE(abc800_floppy_getinfo)
	CONFIG_DEVICE(abc800_serial_getinfo)
SYSTEM_CONFIG_END

/* Driver Initialization */

static OPBASE_HANDLER( abc800_opbase_handler )
{
	if (address >= 0x7800 && address < 0x8000)
	{
		opbase->rom = opbase->ram = memory_region(machine, "main");
		return ~0;
	}

	return address;
}

static OPBASE_HANDLER( abc806_opbase_handler )
{
	abc806_state *state = machine->driver_data;

	if (address >= 0x7800 && address < 0x8000)
	{
		opbase->rom = opbase->ram = memory_region(machine, "main");

		if (!state->fetch_charram)
		{
			state->fetch_charram = 1;
			abc806_bankswitch(machine);
		}

		return ~0;
	}

	if (state->fetch_charram)
	{
		state->fetch_charram = 0;
		abc806_bankswitch(machine);
	}

	return address;
}

static DRIVER_INIT( abc800 )
{
	memory_set_opbase_handler(0, abc800_opbase_handler);
}

static DRIVER_INIT( abc806 )
{
	memory_set_opbase_handler(0, abc806_opbase_handler);
}

/* System Drivers */

/*    YEAR  NAME        PARENT      COMPAT  MACHINE     INPUT   INIT    CONFIG  COMPANY             FULLNAME    FLAGS */
COMP( 1981, abc800c,    0,          0,      abc800c,    abc800, abc800, abc800, "Luxor Datorer AB", "ABC 800 C", GAME_NOT_WORKING )
COMP( 1981, abc800m,    abc800c,    0,      abc800m,    abc800, abc800, abc800, "Luxor Datorer AB", "ABC 800 M", GAME_NOT_WORKING )
COMP( 1983, abc802,     0,          0,      abc802,     abc802, abc800, abc802, "Luxor Datorer AB", "ABC 802",  GAME_NOT_WORKING )
COMP( 1983, abc806,     0,          0,      abc806,     abc806, abc806, abc806, "Luxor Datorer AB", "ABC 806",  GAME_NOT_WORKING )
