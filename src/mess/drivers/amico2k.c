/***************************************************************************

    A.S.EL. Amico 2000

    07/2009 Skeleton driver.

    IC9 - Monitor PROM, handwritten from book listing by Davide
    IC10 - Recorder PROM, yet to be found
    IC6/IC7 - PROMs reconstructed by Luigi Serrantoni

    To Do:
     * Basically everything, in particular implement PROM (described in details
       at the link below) and i8255

    http://www.computerhistory.it/index.php?option=com_content&task=view&id=85&Itemid=117

****************************************************************************/

#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "machine/i8255a.h"
#include "amico2k.lh"


class amico2k_state : public driver_device
{
public:
	amico2k_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};



static READ8_DEVICE_HANDLER( amico2k_i8255a_r )
{
	return 0xff;
}

static WRITE8_DEVICE_HANDLER( amico2k_i8255a_w )
{
}

static ADDRESS_MAP_START(amico2k_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x03ff) AM_RAM
//  AM_RANGE(0x0400, 0x07ff) AM_RAM // optional expansion RAM
	AM_RANGE(0xfb00, 0xfcff) AM_ROM
	AM_RANGE(0xfd00, 0xfd03) AM_DEVREADWRITE("i8255a", amico2k_i8255a_r, amico2k_i8255a_w)
	AM_RANGE(0xfe00, 0xffff) AM_ROM
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( amico2k )
INPUT_PORTS_END


static MACHINE_RESET(amico2k)
{
}

static I8255A_INTERFACE( amico_8255_intf )
{
	DEVCB_NULL,							// Port A read
	DEVCB_NULL,							// Port B read
	DEVCB_NULL,							// Port C read
	DEVCB_NULL,							// Port A write
	DEVCB_NULL,							// Port B write
	DEVCB_NULL							// Port C write
};

static MACHINE_CONFIG_START( amico2k, amico2k_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", M6502, 1000000)	/* 1MHz */
	MCFG_CPU_PROGRAM_MAP(amico2k_mem)

	MCFG_MACHINE_RESET(amico2k)

	/* video hardware */
	MCFG_DEFAULT_LAYOUT( layout_amico2k )

	MCFG_I8255A_ADD( "i8255a", amico_8255_intf )
MACHINE_CONFIG_END


/* ROM definition */
// not sure the ROMs are loaded correctly
ROM_START( amico2k )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "prom.ic10", 0xfb00, 0x200, NO_DUMP )		// cassette recorder ROM, not published anywhere. a board is needed!
	ROM_LOAD( "prom.ic9",  0xfe00, 0x200, CRC(86449f7c) SHA1(fe7deca86e90ab89aae23f11e9dbaf343b4242dc) )

	ROM_REGION( 0x200, "proms", ROMREGION_ERASEFF )
	ROM_LOAD( "prom.ic6",  0x000, 0x100, CRC(4005f760) SHA1(7edcd85feb5a576f6da1bbb723b3cf668cf3df45) )
	ROM_LOAD( "prom.ic7",  0x100, 0x100, CRC(8785d864) SHA1(d169c3b5f5690664083030948db9f33571b08656) )
ROM_END


/* Driver */

/*    YEAR  NAME    PARENT  COMPAT MACHINE INPUT   INIT   COMPANY  FULLNAME                 FLAGS */
COMP( 1978, amico2k,    0,    0,     amico2k,    amico2k,    0,     "A.S.EL.",   "Amico 2000",            GAME_NOT_WORKING | GAME_NO_SOUND)
