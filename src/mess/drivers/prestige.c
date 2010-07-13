/***************************************************************************

        VTech PreComputer Prestige

        

        13/07/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "devices/messram.h"

static UINT8 bank[6];

static READ8_HANDLER( bankswitch_r )
{
	return bank[offset];
}

static WRITE8_HANDLER( bankswitch_w )
{
	const address_space *program = cputag_get_address_space(space->machine, "maincpu", ADDRESS_SPACE_PROGRAM);

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
	const address_space *program = cputag_get_address_space(screen->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT16 addr = 0xe000;

	for (int y = 0; y < 100; y++)
	{
		for (int sx = 0; sx < 30; sx++)
		{
			UINT8 data = memory_read_byte(program, addr);

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

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("64K")
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( prestige )
    ROM_REGION( 0x100000, "maincpu", 0 )
	ROM_LOAD( "27-6020-02", 0x00000, 0x100000, CRC(6bb6db14) SHA1(5d51fc3fd799e7f01ee99c453f9005fb07747b1e))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1998, prestige,  0,       0,	prestige, 	prestige, 	 0,  "VTech",   "PC Prestige Elite",		GAME_NOT_WORKING | GAME_NO_SOUND)
