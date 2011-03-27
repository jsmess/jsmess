/***************************************************************************

        MCS BASIC 52 and MCS BASIC 31 board

        03/12/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/mcs51/mcs51.h"
#include "machine/i8255a.h"
#include "machine/terminal.h"


class basic52_state : public driver_device
{
public:
	basic52_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};


static ADDRESS_MAP_START(basic52_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0x7fff) AM_RAM
	AM_RANGE(0x8000, 0x9fff) AM_ROM // EPROM
	//AM_RANGE(0xc000, 0xdfff) // Expansion block
	//AM_RANGE(0xe000, 0xffff) // Expansion block
ADDRESS_MAP_END

static ADDRESS_MAP_START( basic52_io , AS_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x7fff) AM_RAM
	AM_RANGE(0x8000, 0x9fff) AM_ROM // EPROM
	AM_RANGE(0xa000, 0xa003) AM_DEVREADWRITE("ppi8255", i8255a_r, i8255a_w)  // PPI-8255
	//AM_RANGE(0xc000, 0xdfff) // Expansion block
	//AM_RANGE(0xe000, 0xffff) // Expansion block
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( basic52 )
INPUT_PORTS_END


static MACHINE_RESET(basic52)
{
}

static WRITE8_DEVICE_HANDLER( basic52_kbd_put )
{

}

static GENERIC_TERMINAL_INTERFACE( basic52_terminal_intf )
{
	DEVCB_HANDLER(basic52_kbd_put)
};

static I8255A_INTERFACE( ppi8255_intf )
{
	DEVCB_NULL,					/* Port A read */
	DEVCB_NULL,					/* Port B read */
	DEVCB_NULL,					/* Port C read */
	DEVCB_NULL,					/* Port A write */
	DEVCB_NULL,					/* Port B write */
	DEVCB_NULL					/* Port C write */
};

static MACHINE_CONFIG_START( basic31, basic52_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu", I8031, XTAL_11_0592MHz)
    MCFG_CPU_PROGRAM_MAP(basic52_mem)
    MCFG_CPU_IO_MAP(basic52_io)

    MCFG_MACHINE_RESET(basic52)

    /* video hardware */
    MCFG_FRAGMENT_ADD( generic_terminal )
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG,basic52_terminal_intf)

	MCFG_I8255A_ADD("ppi8255", ppi8255_intf )
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( basic52, basic52_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu", I8052, XTAL_11_0592MHz)
    MCFG_CPU_PROGRAM_MAP(basic52_mem)
    MCFG_CPU_IO_MAP(basic52_io)

    MCFG_MACHINE_RESET(basic52)

    /* video hardware */
    MCFG_FRAGMENT_ADD( generic_terminal )
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG,basic52_terminal_intf)

	MCFG_I8255A_ADD("ppi8255", ppi8255_intf )
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( basic52 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS(0, "v11", "v 1.1")
	ROMX_LOAD( "mcs-51-11.bin",  0x0000, 0x2000, CRC(4157b22b) SHA1(bd9e6869b400cc1c9b163243be7bdcf16ce72789), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "v11b", "v 1.1b")
	ROMX_LOAD( "mcs-51-11b.bin", 0x0000, 0x2000, CRC(a60383cc) SHA1(9515cc435e2ca3d3adb19631c03a62dfbeab0826), ROM_BIOS(2))
	ROM_SYSTEM_BIOS(2, "v131", "v 1.3.1")
	ROMX_LOAD( "mcs-51-131.bin", 0x0000, 0x2000, CRC(6a493162) SHA1(ed1079a6b4d4dbf448e15238c5a9e4dd004e401c), ROM_BIOS(3))
ROM_END

ROM_START( basic31 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS(0, "v12", "v 1.2")
	ROMX_LOAD( "mcs-51-12.bin",  0x0000, 0x2000, CRC(ee667c7c) SHA1(e69b32e69ecda2012c7113649634a3a64e984bed), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "v12a", "v 1.2a")
	ROMX_LOAD( "mcs-51-12a.bin", 0x0000, 0x2000, CRC(225bb2f0) SHA1(46e97643a7a5cb4c278f9e3c73d18cd93209f8bf), ROM_BIOS(2))
ROM_END

/* Driver */
/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1985, basic52,  0,       0,	basic52,	basic52,	 0,  "Intel",   "MCS BASIC 52",		GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 1985, basic31,  basic52, 0,	basic31,	basic52,	 0,  "Intel",   "MCS BASIC 31",		GAME_NOT_WORKING | GAME_NO_SOUND)

