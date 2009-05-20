#include "driver.h"
#include "includes/pc8401a.h"
#include "devices/cartslot.h"
#include "machine/8255ppi.h"
#include "cpu/z80/z80.h"

/*

	TODO:

	- everything

	- peripherals
		* PC-8431A Dual Floppy Drive
		* PC-8441A CRT / Disk Interface
		* PC-8461A 1200 Baud Modem
		* PC-8407A 128KB RAM Expansion

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
		memory_install_readwrite8_handler(program, 0x0000, 0x7fff, 0, 0, SMH_BANK(1), SMH_UNMAP);
		memory_set_bank(machine, 1, rombank);
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
		memory_install_readwrite8_handler(program, 0x8000, 0xbfff, 0, 0, SMH_UNMAP, SMH_UNMAP);
//		memory_set_bank(machine, 3, 3); // TODO or 4
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

	pc8401a_bankswitch(space->machine, data);
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
	AM_RANGE(0x31, 0x31) AM_MASK(0xff) AM_WRITE(mmr_w)
	AM_RANGE(0xfc, 0xff) AM_MASK(0xff) AM_DEVREADWRITE(PPI8255_TAG, ppi8255_r, ppi8255_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( pc8401a )
INPUT_PORTS_END

/* Machine Initialization */

static MACHINE_START( pc8401a )
{
	pc8401a_state *state = machine->driver_data;

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

/* Machine Drivers */

static MACHINE_DRIVER_START( pc8401a )
	MDRV_DRIVER_DATA(pc8401a_state)

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80_TAG, Z80, 4000000)
	MDRV_CPU_PROGRAM_MAP(pc8401a_mem)
	MDRV_CPU_IO_MAP(pc8401a_io)
	
	MDRV_MACHINE_START( pc8401a )

	/* video hardware */
	MDRV_SCREEN_ADD("screen", LCD)
	MDRV_SCREEN_REFRESH_RATE(44)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(480, 128)
	MDRV_SCREEN_VISIBLE_AREA(0, 480-1, 0, 128-1)
//	MDRV_DEFAULT_LAYOUT(layout_pc8401a)
	MDRV_PALETTE_LENGTH(2)

	/* devices */
	MDRV_PPI8255_ADD(PPI8255_TAG, pc8401a_ppi8255_interface)

	/* option ROM cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("rom,bin")
	MDRV_CARTSLOT_NOT_MANDATORY
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( pc8500 )
	MDRV_DRIVER_DATA(pc8401a_state)

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80_TAG, Z80, 4000000)
	MDRV_CPU_PROGRAM_MAP(pc8401a_mem)
	MDRV_CPU_IO_MAP(pc8401a_io)
	
	MDRV_MACHINE_START( pc8401a )

	/* video hardware */
	MDRV_SCREEN_ADD("screen", LCD)
	MDRV_SCREEN_REFRESH_RATE(44)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(480, 200)
	MDRV_SCREEN_VISIBLE_AREA(0, 480-1, 0, 200-1)
//	MDRV_DEFAULT_LAYOUT(layout_pc8500)
	MDRV_PALETTE_LENGTH(2)

	/* devices */
	MDRV_PPI8255_ADD(PPI8255_TAG, pc8401a_ppi8255_interface)

	/* option ROM cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("rom,bin")
	MDRV_CARTSLOT_NOT_MANDATORY
MACHINE_DRIVER_END

/* ROMs */

ROM_START( pc8401a )
	ROM_REGION( 0x20000, Z80_TAG, 0 )
	ROM_LOAD( "pc8401a.bin", 0x0000, 0x18000, NO_DUMP )
	ROM_CART_LOAD("cart", 0x18000, 0x8000, ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START( pc8500 )
	ROM_REGION( 0x20000, Z80_TAG, 0 )
	ROM_LOAD( "pc8500.bin", 0x0000, 0x10000, CRC(c2749ef0) SHA1(f766afce9fda9ec84ed5b39ebec334806798afb3) )
	ROM_CART_LOAD("cart", 0x18000, 0x8000, ROM_NOMIRROR | ROM_OPTIONAL)
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
COMP( 1983, pc8500,		0,		0,		pc8500,		pc8401a,	0,		pc8500,		"NEC",	"PC-8500", GAME_NOT_WORKING )
