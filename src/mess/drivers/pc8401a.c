#include "driver.h"
#include "includes/pc8401a.h"
#include "devices/cartslot.h"
#include "machine/8255ppi.h"
#include "machine/upd1990a.h"
#include "sound/ay8910.h"
#include "video/sed1330.h"
#include "cpu/z80/z80.h"

/*

	NEC PC-8401A-LS "Starlet"
	NEC PC-8500 "Studley"

	TODO:

	- keyboard
	- 8251 USART
	- 8255 ports
	- AY-3-8910 ports

	- peripherals
		* PC-8431A Dual Floppy Drive
		* PC-8441A CRT / Disk Interface
		* PC-8461A 1200 Baud Modem
		* PC-8407A 128KB RAM Expansion
		* PC-8508A ROM/RAM Cartridge

*/

/* Read/Write Handlers */

static void pc8401a_bankswitch(running_machine *machine, UINT8 data)
{
	const address_space *program = cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM);

	int rombank = data & 0x03;
	int ram0000 = (data >> 2) & 0x03;
	int ram8000 = (data >> 4) & 0x03;

	switch (ram0000)
	{
	case 0: /* ROM 0000H to 7FFFH */
		if (rombank < 3)
		{
			/* internal ROM */
			memory_install_readwrite8_handler(program, 0x0000, 0x7fff, 0, 0, SMH_BANK(1), SMH_UNMAP);
			memory_set_bank(machine, 1, rombank);
		}
		else
		{
			/* ROM cartridge */
			memory_install_readwrite8_handler(program, 0x0000, 0x7fff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		}
		logerror("0x0000-0x7fff = ROM %u\n", rombank);
		break;

	case 1: /* RAM 0000H to 7FFFH */
		memory_install_readwrite8_handler(program, 0x0000, 0x7fff, 0, 0, SMH_BANK(1), SMH_BANK(1));
		memory_set_bank(machine, 1, 4);
		logerror("0x0000-0x7fff = RAM 0-7fff\n");
		break;

	case 2:	/* RAM 8000H to FFFFH */
		memory_install_readwrite8_handler(program, 0x0000, 0x7fff, 0, 0, SMH_BANK(1), SMH_BANK(1));
		memory_set_bank(machine, 1, 5);
		logerror("0x0000-0x7fff = RAM 8000-ffff\n");
		break;

	case 3: /* invalid */
		logerror("0x0000-0x7fff = invalid\n");
		break;
	}

	switch (ram8000)
	{
	case 0: /* cell addresses 0000H to 03FFFH */
		memory_install_readwrite8_handler(program, 0x8000, 0xbfff, 0, 0, SMH_BANK(3), SMH_BANK(3));
		memory_set_bank(machine, 3, 0);
		logerror("0x8000-0xbfff = RAM 0-3fff\n");
		break;

	case 1: /* cell addresses 4000H to 7FFFH */
		memory_install_readwrite8_handler(program, 0x8000, 0xbfff, 0, 0, SMH_BANK(3), SMH_BANK(3));
		memory_set_bank(machine, 3, 1);
		logerror("0x8000-0xbfff = RAM 4000-7fff\n");
		break;

	case 2: /* cell addresses 8000H to BFFFH */
		memory_install_readwrite8_handler(program, 0x8000, 0xbfff, 0, 0, SMH_BANK(3), SMH_BANK(3));
		memory_set_bank(machine, 3, 2);
		logerror("0x8000-0xbfff = RAM 8000-bfff\n");
		break;

	case 3: /* RAM cartridge */
		if (mess_ram_size > 64)
		{
			memory_install_readwrite8_handler(program, 0x8000, 0xbfff, 0, 0, SMH_BANK(3), SMH_BANK(3));
			memory_set_bank(machine, 3, 3); // TODO or 4
		}
		else
		{
			memory_install_readwrite8_handler(program, 0x8000, 0xbfff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		}
		logerror("0x8000-0xbfff = RAM cartridge\n");
		break;
	}

	if (BIT(data, 6))
	{
		/* CRT video RAM */
		memory_install_readwrite8_handler(program, 0xc000, 0xdfff, 0, 0, SMH_BANK(4), SMH_BANK(4));
		memory_install_readwrite8_handler(program, 0xe000, 0xe7ff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		memory_set_bank(machine, 4, 1);
		logerror("0xc000-0xdfff = video RAM\n");
	}
	else
	{
		/* RAM */
		memory_install_readwrite8_handler(program, 0xc000, 0xe7ff, 0, 0, SMH_BANK(4), SMH_BANK(4));
		memory_set_bank(machine, 4, 0);
		logerror("0xc000-0e7fff = RAM c000-e7fff\n");
	}
}

static WRITE8_HANDLER( mmr_w )
{
	/*

		bit		description

		0		ROM section bit 0
		1		ROM section bit 1
		2		mapping for CPU addresses 0000H to 7FFFH bit 0
		3		mapping for CPU addresses 0000H to 7FFFH bit 1
		4		mapping for CPU addresses 8000H to BFFFH bit 0
		5		mapping for CPU addresses 8000H to BFFFH bit 1
		6		mapping for CPU addresses C000H to E7FFH
		7

	*/

	pc8401a_state *state = space->machine->driver_data;

	if (data != state->mmr)
	{
		pc8401a_bankswitch(space->machine, data);
	}

	state->mmr = data;
}

static void pc8500_bankswitch(running_machine *machine, UINT8 data)
{
	const address_space *program = cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM);

	int rombank = data & 0x03;
	int ram0000 = (data >> 2) & 0x03;
	int ram8000 = (data >> 4) & 0x03;

	switch (ram0000)
	{
	case 0: /* ROM 0000H to 7FFFH */
		if (rombank < 2)
		{
			/* internal ROM */
			memory_install_readwrite8_handler(program, 0x0000, 0x7fff, 0, 0, SMH_BANK(1), SMH_UNMAP);
			memory_set_bank(machine, 1, rombank);
		}
		else
		{
			/* ROM cartridge */
			memory_install_readwrite8_handler(program, 0x0000, 0x7fff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		}
		logerror("0x0000-0x7fff = ROM %u\n", rombank);
		break;

	case 1: /* RAM 0000H to 7FFFH */
		memory_install_readwrite8_handler(program, 0x0000, 0x7fff, 0, 0, SMH_BANK(1), SMH_BANK(1));
		memory_set_bank(machine, 1, 4);
		logerror("0x0000-0x7fff = RAM 0-7fff\n");
		break;

	case 2:	/* RAM 8000H to FFFFH */
		memory_install_readwrite8_handler(program, 0x0000, 0x7fff, 0, 0, SMH_BANK(1), SMH_BANK(1));
		memory_set_bank(machine, 1, 5);
		logerror("0x0000-0x7fff = RAM 8000-ffff\n");
		break;

	case 3: /* invalid */
		logerror("0x0000-0x7fff = invalid\n");
		break;
	}

	switch (ram8000)
	{
	case 0: /* cell addresses 0000H to 03FFFH */
		memory_install_readwrite8_handler(program, 0x8000, 0xbfff, 0, 0, SMH_BANK(3), SMH_BANK(3));
		memory_set_bank(machine, 3, 0);
		logerror("0x8000-0xbfff = RAM 0-3fff\n");
		break;

	case 1: /* cell addresses 4000H to 7FFFH */
		memory_install_readwrite8_handler(program, 0x8000, 0xbfff, 0, 0, SMH_BANK(3), SMH_BANK(3));
		memory_set_bank(machine, 3, 1);
		logerror("0x8000-0xbfff = RAM 4000-7fff\n");
		break;

	case 2: /* cell addresses 8000H to BFFFH */
		memory_install_readwrite8_handler(program, 0x8000, 0xbfff, 0, 0, SMH_BANK(3), SMH_BANK(3));
		memory_set_bank(machine, 3, 2);
		logerror("0x8000-0xbfff = RAM 8000-bfff\n");
		break;

	case 3: /* RAM cartridge */
		if (mess_ram_size > 64)
		{
			memory_install_readwrite8_handler(program, 0x8000, 0xbfff, 0, 0, SMH_BANK(3), SMH_BANK(3));
			memory_set_bank(machine, 3, 3); // TODO or 4
		}
		else
		{
			memory_install_readwrite8_handler(program, 0x8000, 0xbfff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		}
		logerror("0x8000-0xbfff = RAM cartridge\n");
		break;
	}

	if (BIT(data, 6))
	{
		/* CRT video RAM */
		memory_install_readwrite8_handler(program, 0xc000, 0xdfff, 0, 0, SMH_BANK(4), SMH_BANK(4));
		memory_install_readwrite8_handler(program, 0xe000, 0xe7ff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		memory_set_bank(machine, 4, 1);
		logerror("0xc000-0xdfff = video RAM\n");
	}
	else
	{
		/* RAM */
		memory_install_readwrite8_handler(program, 0xc000, 0xe7ff, 0, 0, SMH_BANK(4), SMH_BANK(4));
		memory_set_bank(machine, 4, 0);
		logerror("0xc000-0e7fff = RAM c000-e7fff\n");
	}
}

static WRITE8_HANDLER( pc8500_mmr_w )
{
	/*

		bit		description

		0		ROM section bit 0
		1		ROM section bit 1
		2		mapping for CPU addresses 0000H to 7FFFH bit 0
		3		mapping for CPU addresses 0000H to 7FFFH bit 1
		4		mapping for CPU addresses 8000H to BFFFH bit 0
		5		mapping for CPU addresses 8000H to BFFFH bit 1
		6		mapping for CPU addresses C000H to E7FFH
		7

	*/

	pc8401a_state *state = space->machine->driver_data;

	if (data != state->mmr)
	{
		pc8500_bankswitch(space->machine, data);
	}

	state->mmr = data;
}

static READ8_HANDLER( mmr_r )
{
	pc8401a_state *state = space->machine->driver_data;

	return state->mmr;
}

static READ8_HANDLER( rtc_r )
{
	pc8401a_state *state = space->machine->driver_data;

	return state->upd1990a_data;
}

static WRITE8_HANDLER( rtc_cmd_w )
{
	/*

		bit		description

		0		RTC C0
		1		RTC C1
		2		RTC C2
		3		RTC DATA IN?
		4		
		5		
		6		
		7

	*/

	pc8401a_state *state = space->machine->driver_data;

	upd1990a_c0_w(state->upd1990a, BIT(data, 0));
	upd1990a_c1_w(state->upd1990a, BIT(data, 1));
	upd1990a_c2_w(state->upd1990a, BIT(data, 2));
	upd1990a_data_w(state->upd1990a, BIT(data, 3));
//	logerror("10 %02x\n", data);
}

static WRITE8_HANDLER( rtc_ctrl_w )
{
	/*

		bit		description

		0		RTC OE or CS?
		1		RTC STB
		2		RTC CLK
		3		
		4		
		5		
		6		
		7

	*/

	pc8401a_state *state = space->machine->driver_data;

	upd1990a_oe_w(state->upd1990a, BIT(data, 0));
	upd1990a_stb_w(state->upd1990a, BIT(data, 1));
	upd1990a_clk_w(state->upd1990a, BIT(data, 2));
}

static READ8_HANDLER( io_rom_data_r )
{
	pc8401a_state *state = space->machine->driver_data;

	UINT8 *iorom = memory_region(space->machine, "iorom");

	logerror("I/O ROM read from %05x\n", state->io_addr);

	return iorom[state->io_addr];
}

static WRITE8_HANDLER( io_rom_addr_w )
{
	pc8401a_state *state = space->machine->driver_data;

	switch (offset)
	{
	case 0: /* A17..A16 */
		state->io_addr = ((data & 0x03) << 16) | (state->io_addr & 0xffff);
		break;

	case 1: /* A15..A8 */
		state->io_addr = (state->io_addr & 0x300ff) | (data << 8);
		break;

	case 2: /* A7..A0 */
		state->io_addr = (state->io_addr & 0x3ff00) | data;
		break;

	case 3:
		/* the same data is written here as to 0xb2, maybe this latches the address value? */
		break;
	}
}

/* Memory Maps */

static ADDRESS_MAP_START( pc8401a_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x7fff) AM_RAMBANK(1)
	AM_RANGE(0x8000, 0xbfff) AM_RAMBANK(3)
	AM_RANGE(0xc000, 0xe7ff) AM_RAMBANK(4)
	AM_RANGE(0xe800, 0xffff) AM_RAMBANK(5)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pc8401a_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x10, 0x10) AM_WRITE(rtc_cmd_w)
	AM_RANGE(0x30, 0x30) AM_READWRITE(mmr_r, mmr_w)
	AM_RANGE(0x40, 0x40) AM_READWRITE(rtc_r, rtc_ctrl_w)
	AM_RANGE(0xb0, 0xb3) AM_WRITE(io_rom_addr_w)
	AM_RANGE(0xb3, 0xb3) AM_READ(io_rom_data_r)
	AM_RANGE(0xfc, 0xff) AM_DEVREADWRITE(PPI8255_TAG, ppi8255_r, ppi8255_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pc8500_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
//	AM_RANGE(0x08, 0x08) receive comm char
	AM_RANGE(0x10, 0x10) AM_WRITE(rtc_cmd_w)
//	AM_RANGE(0x20, 0x21) USART?
	AM_RANGE(0x30, 0x30) AM_READWRITE(mmr_r, pc8500_mmr_w)
//	AM_RANGE(0x31, 0x31)
	AM_RANGE(0x40, 0x40) AM_READWRITE(rtc_r, rtc_ctrl_w)
//	AM_RANGE(0x41, 0x41)
//	AM_RANGE(0x50, 0x51) USART?
	AM_RANGE(0x60, 0x60) AM_DEVREADWRITE(SED1330_TAG, sed1330_data_r, sed1330_data_w)
	AM_RANGE(0x61, 0x61) AM_DEVREADWRITE(SED1330_TAG, sed1330_status_r, sed1330_command_w)
//	AM_RANGE(0x70, 0x71)
//	AM_RANGE(0x80, 0x80) modem status, set to 0xff to boot
//	AM_RANGE(0x90, 0x93)
	AM_RANGE(0x98, 0x99) AM_DEVREADWRITE(AY8910_TAG, ay8910_r, ay8910_address_data_w)
//	AM_RANGE(0xa0, 0xa1) USART?
	AM_RANGE(0xb0, 0xb3) AM_WRITE(io_rom_addr_w)
	AM_RANGE(0xb3, 0xb3) AM_READ(io_rom_data_r)
	AM_RANGE(0xfc, 0xff) AM_DEVREADWRITE(PPI8255_TAG, ppi8255_r, ppi8255_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( pc8401a )
INPUT_PORTS_END

/* Machine Initialization */

static MACHINE_START( pc8401a )
{
	pc8401a_state *state = machine->driver_data;

	/* find devices */
	state->upd1990a = devtag_get_device(machine, UPD1990A_TAG);

	/* initialize RTC */
	upd1990a_cs_w(state->upd1990a, 1);

	/* allocate video memory */
	state->video_ram = auto_alloc_array(machine, UINT8, PC8401A_VIDEORAM_SIZE);

	/* set up A0/A1 memory banking */
	memory_configure_bank(machine, 1, 0, 4, memory_region(machine, Z80_TAG), 0x8000);
	memory_configure_bank(machine, 1, 4, 2, mess_ram, 0x8000);
	memory_set_bank(machine, 1, 0);

	/* set up A2 memory banking */
	memory_configure_bank(machine, 3, 0, 5, mess_ram, 0x4000);
	memory_set_bank(machine, 3, 0);

	/* set up A3 memory banking */
	memory_configure_bank(machine, 4, 0, 1, mess_ram + 0xc000, 0);
	memory_configure_bank(machine, 4, 1, 1, state->video_ram, 0);
	memory_set_bank(machine, 4, 0);

	/* set up A4 memory banking */
	memory_configure_bank(machine, 5, 0, 1, mess_ram + 0xe800, 0);
	memory_set_bank(machine, 5, 0);

	pc8401a_bankswitch(machine, 0);
}

static MACHINE_START( pc8500 )
{
	pc8401a_state *state = machine->driver_data;

	/* find devices */
	state->upd1990a = devtag_get_device(machine, UPD1990A_TAG);
	state->sed1330 = devtag_get_device(machine, SED1330_TAG);

	/* initialize RTC */
	upd1990a_cs_w(state->upd1990a, 1);

	/* allocate video memory */
	state->video_ram = auto_alloc_array(machine, UINT8, PC8500_VIDEORAM_SIZE);

	/* set up A0/A1 memory banking */
	memory_configure_bank(machine, 1, 0, 4, memory_region(machine, Z80_TAG), 0x8000);
	memory_configure_bank(machine, 1, 4, 2, mess_ram, 0x8000);
	memory_set_bank(machine, 1, 0);

	/* set up A2 memory banking */
	memory_configure_bank(machine, 3, 0, 5, mess_ram, 0x4000);
	memory_set_bank(machine, 3, 0);

	/* set up A3 memory banking */
	memory_configure_bank(machine, 4, 0, 1, mess_ram + 0xc000, 0);
	memory_configure_bank(machine, 4, 1, 1, state->video_ram, 0);
	memory_set_bank(machine, 4, 0);

	/* set up A4 memory banking */
	memory_configure_bank(machine, 5, 0, 1, mess_ram + 0xe800, 0);
	memory_set_bank(machine, 5, 0);

	pc8500_bankswitch(machine, 0);
}

static READ8_DEVICE_HANDLER( pc8401a_ppi8255_c_r )
{
	/*

		bit		signal			description

		PC0
		PC1
		PC2
		PC3
		PC4     PC-8431A DAV	data valid
		PC5     PC-8431A RFD	ready for data
		PC6     PC-8431A DAC	data accepted
		PC7     PC-8431A ATN	attention

	*/

	return 0;
}

static WRITE8_DEVICE_HANDLER( pc8401a_ppi8255_c_w )
{
	/*

		bit		signal			description

		PC0
		PC1
		PC2
		PC3
		PC4     PC-8431A DAV	data valid
		PC5     PC-8431A RFD	ready for data
		PC6     PC-8431A DAC	data accepted
		PC7     PC-8431A ATN	attention

	*/
}

static const ppi8255_interface pc8401a_ppi8255_interface =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(pc8401a_ppi8255_c_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(pc8401a_ppi8255_c_w),
};

/* Video */

static PALETTE_INIT( pc8401a )
{
	palette_set_color(machine, 0, MAKE_RGB(138, 146, 148));
	palette_set_color(machine, 1, MAKE_RGB(92, 83, 88));
}

static VIDEO_UPDATE( pc8401a )
{
	pc8401a_state *state = screen->machine->driver_data;

	int y, sx, x;
	UINT16 addr = 0;

	for (y = 0; y < 128; y++)
	{
		for (sx = 0; sx < 80; sx++)
		{
			UINT8 data = state->video_ram[addr & PC8401A_VIDEORAM_MASK];

			for (x = 0; x < 6; x++)
			{
				*BITMAP_ADDR16(bitmap, y, (sx * 6) + x) = BIT(data, 7);
				data <<= 1;
			}

			addr++;
		}
	}

	return 0;
}

static VIDEO_UPDATE( pc8500 )
{
	pc8401a_state *state = screen->machine->driver_data;

	sed1330_update(state->sed1330, bitmap, cliprect);

	return 0;
}

static READ8_HANDLER( pc8500_sed1330_vd_r )
{
	pc8401a_state *state = space->machine->driver_data;

	return state->video_ram[offset & PC8500_VIDEORAM_MASK];
}

static WRITE8_HANDLER( pc8500_sed1330_vd_w )
{
	pc8401a_state *state = space->machine->driver_data;

	state->video_ram[offset & PC8500_VIDEORAM_MASK] = data;
}

static SED1330_INTERFACE( pc8500_sed1330_config )
{
	SCREEN_TAG,
	DEVCB_MEMORY_HANDLER(Z80_TAG, PROGRAM, pc8500_sed1330_vd_r),
	DEVCB_MEMORY_HANDLER(Z80_TAG, PROGRAM, pc8500_sed1330_vd_w)
};

/* uPD1990A Interface */

static WRITE_LINE_DEVICE_HANDLER( pc8401a_upd1990a_data_w )
{
	pc8401a_state *driver_state = device->machine->driver_data;

	driver_state->upd1990a_data = state;
}

static UPD1990A_INTERFACE( pc8401a_upd1990a_intf )
{
	DEVCB_LINE(pc8401a_upd1990a_data_w),
	DEVCB_NULL // ???
};

/* AY-3-8910 Interface */

static const ay8910_interface ay8910_config =
{
	AY8910_LEGACY_OUTPUT,
	AY8910_DEFAULT_LOADS,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

/* Machine Drivers */

static MACHINE_DRIVER_START( pc8401a )
	MDRV_DRIVER_DATA(pc8401a_state)

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80_TAG, Z80, 4000000) // NEC uPD70008C
	MDRV_CPU_PROGRAM_MAP(pc8401a_mem)
	MDRV_CPU_IO_MAP(pc8401a_io)
	
	MDRV_MACHINE_START( pc8401a )

	/* video hardware */
	MDRV_SCREEN_ADD(SCREEN_TAG, LCD)
	MDRV_SCREEN_REFRESH_RATE(44)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(480, 128)
	MDRV_SCREEN_VISIBLE_AREA(0, 480-1, 0, 128-1)

//	MDRV_DEFAULT_LAYOUT(layout_pc8401a)
	
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(pc8401a)
	MDRV_VIDEO_UPDATE(pc8401a)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910_TAG, AY8910, 4000000)
	MDRV_SOUND_CONFIG(ay8910_config)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* devices */
	MDRV_UPD1990A_ADD(UPD1990A_TAG, XTAL_32_768kHz, pc8401a_upd1990a_intf)
	MDRV_PPI8255_ADD(PPI8255_TAG, pc8401a_ppi8255_interface)

	/* option ROM cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("rom,bin")
	MDRV_CARTSLOT_NOT_MANDATORY

	/* I/O ROM cartridge */
	MDRV_CARTSLOT_ADD("iocart")
	MDRV_CARTSLOT_EXTENSION_LIST("rom,bin")
	MDRV_CARTSLOT_NOT_MANDATORY
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( pc8500 )
	MDRV_DRIVER_DATA(pc8401a_state)

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80_TAG, Z80, 4000000) // NEC uPD70008C
	MDRV_CPU_PROGRAM_MAP(pc8401a_mem)
	MDRV_CPU_IO_MAP(pc8500_io)
	
	MDRV_MACHINE_START( pc8500 )

	/* video hardware */
	MDRV_SCREEN_ADD(SCREEN_TAG, LCD)
	MDRV_SCREEN_REFRESH_RATE(44)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(480, 208)
	MDRV_SCREEN_VISIBLE_AREA(0, 480-1, 0, 200-1)

//	MDRV_DEFAULT_LAYOUT(layout_pc8500)
	
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(pc8401a)
	MDRV_VIDEO_UPDATE(pc8500)

	MDRV_SED1330_ADD(SED1330_TAG, 0, pc8500_sed1330_config)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910_TAG, AY8910, 4000000)
	MDRV_SOUND_CONFIG(ay8910_config)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* devices */
	MDRV_UPD1990A_ADD(UPD1990A_TAG, XTAL_32_768kHz, pc8401a_upd1990a_intf)
	MDRV_PPI8255_ADD(PPI8255_TAG, pc8401a_ppi8255_interface)

	/* option ROM cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("rom,bin")
	MDRV_CARTSLOT_NOT_MANDATORY

	/* I/O ROM cartridge */
	MDRV_CARTSLOT_ADD("iocart")
	MDRV_CARTSLOT_EXTENSION_LIST("rom,bin")
	MDRV_CARTSLOT_NOT_MANDATORY
MACHINE_DRIVER_END

/* ROMs */

ROM_START( pc8401a )
	ROM_REGION( 0x20000, Z80_TAG, ROMREGION_ERASEFF )
	ROM_LOAD( "pc8401a.bin", 0x0000, 0x18000, NO_DUMP )
	ROM_CART_LOAD("cart", 0x18000, 0x8000, ROM_NOMIRROR | ROM_OPTIONAL)

	ROM_REGION( 0x40000, "iorom", ROMREGION_ERASEFF )
	ROM_CART_LOAD("iocart", 0x00000, 0x40000, ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START( pc8500 )
	ROM_REGION( 0x20000, Z80_TAG, ROMREGION_ERASEFF )
	ROM_LOAD( "pc8500.bin", 0x0000, 0x10000, CRC(c2749ef0) SHA1(f766afce9fda9ec84ed5b39ebec334806798afb3) )
	ROM_CART_LOAD("cart", 0x18000, 0x8000, ROM_NOMIRROR | ROM_OPTIONAL)

	ROM_REGION( 0x40000, "iorom", ROMREGION_ERASEFF )
	ROM_CART_LOAD("iocart", 0x00000, 0x40000, ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

/* System Configurations */

static SYSTEM_CONFIG_START( pc8401a )
	CONFIG_RAM_DEFAULT	(64 * 1024)
	CONFIG_RAM			(96 * 1024)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START( pc8500 )
	CONFIG_RAM_DEFAULT	(64 * 1024)
SYSTEM_CONFIG_END

/* System Drivers */

/*    YEAR  NAME		PARENT	COMPAT	MACHINE		INPUT		INIT	CONFIG		COMPANY	FULLNAME */
COMP( 1984,	pc8401a,	0,		0,		pc8401a,	pc8401a,	0,		pc8401a,	"NEC",	"PC-8401A-LS", GAME_NOT_WORKING )
/*
COMP( 1984,	pc8401bd,	pc8401a,0,		pc8401a,	pc8401a,	0,		pc8401a,	"NEC",	"PC-8401BD", GAME_NOT_WORKING )
*/
COMP( 1985, pc8500,		0,		0,		pc8500,		pc8401a,	0,		pc8500,		"NEC",	"PC-8500", GAME_NOT_WORKING )
