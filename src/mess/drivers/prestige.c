/*

VTech PreComputer Prestige Elite

PCB Layout
----------

|-------------------------------------------|
|   |-------CN1-------|     CN2             |
|                                           |
|                                       CN3 |
|                                           |
|CN10                                       |
|CN9            RAM                         --|
|CN8                                          |
|                                             |
|                       Z80         ROM       |
|               04010                     CN4 |
|CN7                                          |
|CN6                                          |
|                       |------CN5------|   --|
|-------------------------------------------|

Notes:
    All IC's shown.

    ROM     - VTech LH5S8R14, labeled "1998 27-6020-02" (dumped as 1Mx8)
    Z80     - Z80 family SOC?
    RAM     - LG Semicon GM76C256CLLFW55 32Kx8 Static RAM
    04010   - ?
    CN1     - Centronics connector
    CN2     - mouse connector
    CN3     - LCD ribbon cable
    CN4     - expansion connector
    CN5     - keyboard ribbon cable
    CN6     - speaker wire
    CN7     - volume wire
    CN8     - ? wire
    CN9     - power wire
    CN10    - NVRAM battery wire
*/

/*
	
	Undumped cartridges:
  
	80-1410   Super Science
	80-1533   Famous Things & Places
	80-0989   Bible Knowledge
	80-1001   Fantasy Trivia
	80-1002   General Knowledge II
	80-1003   Sports History
	80-2314   Familiar Faces
	80-2315   Historical Happenings
	80-2333   Arts, Entertainment & More
	80-2334   Customs & Cultures
	80-1531   32K RAM Memory Expansion Cartridge
	80-12051  Space Scholar
	80-12053  Frenzy of Facts
	80-12052  Spreadsheet Success

*/

/*

	TODO:

	- identify unknown chips
	- boot animation won't play
	- keyboard
	- mouse
	- sound
	- cartridges

*/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "devices/messram.h"
#include "devices/cartslot.h"

static UINT8 bank[6];

static READ8_HANDLER( bankswitch_r )
{
	return bank[offset];
}

static WRITE8_HANDLER( bankswitch_w )
{
	address_space *program = cputag_get_address_space(space->machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	switch (offset)
	{
	case 0:
		memory_set_bank(space->machine, "bank1", data & 0x3f);
		break;

	case 1:
		memory_set_bank(space->machine, "bank2", data & 0x3f);
		break;

	case 2:
		if (bank[5] == 0x0c)
		{
			memory_set_bank(space->machine, "bank3", 0x40 + BIT(data, 7));
		}
		else
		{
			memory_set_bank(space->machine, "bank3", data & 0x3f);
		}
		break;

	case 3:
		memory_set_bank(space->machine, "bank4", data & 0x03);
		break;

	case 4:
		memory_set_bank(space->machine, "bank5", data & 0x03);
		break;

	case 5:
		if (data == 0x0c)
		{
			memory_install_readwrite_bank(program, 0x8000, 0xbfff, 0, 0, "bank3");
		}
		else
		{
			memory_unmap_write(program, 0x8000, 0xbfff, 0, 0);
		}
	}

	bank[offset] = data;
}

static ADDRESS_MAP_START(prestige_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_ROMBANK("bank1")
	AM_RANGE(0x4000, 0x7fff) AM_ROMBANK("bank2")
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK("bank3")
	AM_RANGE(0xc000, 0xdfff) AM_RAMBANK("bank4")
	AM_RANGE(0xe000, 0xffff) AM_RAMBANK("bank5")
ADDRESS_MAP_END

static ADDRESS_MAP_START( prestige_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x50, 0x55) AM_READWRITE(bankswitch_r, bankswitch_w)
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( prestige )
INPUT_PORTS_END


static MACHINE_START(prestige)
{
	UINT8 *ram = messram_get_ptr(machine->device("messram"));

	memory_configure_bank(machine, "bank1", 0, 64, memory_region(machine, "maincpu"), 0x4000);
	memory_configure_bank(machine, "bank2", 0, 64, memory_region(machine, "maincpu"), 0x4000);
	memory_configure_bank(machine, "bank3", 0, 64, memory_region(machine, "maincpu"), 0x4000);
	memory_configure_bank(machine, "bank3", 64, 2, ram + 0x8000, 0x4000);
	memory_configure_bank(machine, "bank4", 0, 4, ram, 0x2000);
	memory_configure_bank(machine, "bank5", 0, 4, ram, 0x2000);

	memory_set_bank(machine, "bank1", 0);
	memory_set_bank(machine, "bank2", 0);
	memory_set_bank(machine, "bank3", 0);
	memory_set_bank(machine, "bank4", 0);
	memory_set_bank(machine, "bank5", 0);
}

static PALETTE_INIT( prestige )
{
	palette_set_color(machine, 0, MAKE_RGB(39, 108, 51));
	palette_set_color(machine, 1, MAKE_RGB(16, 37, 84));
}

static VIDEO_START( prestige )
{
}

static VIDEO_UPDATE( prestige )
{
	address_space *program = cputag_get_address_space(screen->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT16 addr = 0xe000;

	for (int y = 0; y < 100; y++)
	{
		for (int sx = 0; sx < 30; sx++)
		{
			UINT8 data = program->read_byte(addr);

			for (int x = 0; x < 8; x++)
			{
				*BITMAP_ADDR16(bitmap, y, (sx * 8) + x) = BIT(data, 7);

				data <<= 1;
			}

			addr++;
		}
	}

    return 0;
}

static DEVICE_IMAGE_LOAD( prestige_cart )
{
	return IMAGE_INIT_FAIL;
}

static MACHINE_DRIVER_START( prestige )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(prestige_mem)
    MDRV_CPU_IO_MAP(prestige_io)

    MDRV_MACHINE_START(prestige)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", LCD)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE( 240, 100 )
	MDRV_SCREEN_VISIBLE_AREA( 0, 240-1, 0, 100-1 )
	MDRV_DEFAULT_LAYOUT( layout_lcd )

    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(prestige)

    MDRV_VIDEO_START(prestige)
    MDRV_VIDEO_UPDATE(prestige)

	/* cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("bin")
	MDRV_CARTSLOT_INTERFACE("prestige_cart")
	MDRV_CARTSLOT_LOAD(prestige_cart)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("32K")
	MDRV_RAM_EXTRA_OPTIONS("64K")
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( prestige )
    ROM_REGION( 0x100000, "maincpu", 0 )
	ROM_LOAD( "27-6020-02.u2", 0x00000, 0x100000, CRC(6bb6db14) SHA1(5d51fc3fd799e7f01ee99c453f9005fb07747b1e) )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1998, prestige,  0,       0,	prestige,	prestige,	 0,  "VTech",   "PC Prestige Elite",		GAME_NOT_WORKING | GAME_NO_SOUND)
