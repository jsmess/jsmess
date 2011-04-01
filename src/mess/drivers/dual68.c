/***************************************************************************

        Dual Systems 68000

        09/12/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "cpu/i8085/i8085.h"
#include "machine/terminal.h"


class dual68_state : public driver_device
{
public:
	dual68_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT16* m_ram;
};



static WRITE16_HANDLER(dual68_terminal_w)
{
	device_t *devconf = space->machine().device(TERMINAL_TAG);
	terminal_write(devconf,0,data >> 8);
}

static ADDRESS_MAP_START(dual68_mem, AS_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000000, 0x0000ffff) AM_RAM AM_BASE_MEMBER(dual68_state, m_ram)
	AM_RANGE(0x00080000, 0x00081fff) AM_ROM AM_REGION("user1",0)
	AM_RANGE(0x007f0000, 0x007f0001) AM_WRITE(dual68_terminal_w)
	AM_RANGE(0x00800000, 0x00801fff) AM_ROM AM_REGION("user1",0)
ADDRESS_MAP_END

static ADDRESS_MAP_START(sio4_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x07ff) AM_ROM
	AM_RANGE(0x0800, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sio4_io , AS_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( dual68 )
INPUT_PORTS_END


static MACHINE_RESET(dual68)
{
	dual68_state *state = machine.driver_data<dual68_state>();
	UINT8* user1 = machine.region("user1")->base();

	memcpy((UINT8*)state->m_ram,user1,0x2000);

	machine.device("maincpu")->reset();
}

static WRITE8_DEVICE_HANDLER( dual68_kbd_put )
{

}

static GENERIC_TERMINAL_INTERFACE( dual68_terminal_intf )
{
	DEVCB_HANDLER(dual68_kbd_put)
};

static MACHINE_CONFIG_START( dual68, dual68_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu", M68000, XTAL_16MHz / 2)
    MCFG_CPU_PROGRAM_MAP(dual68_mem)

	MCFG_CPU_ADD("siocpu", I8085A, XTAL_16MHz / 8)
    MCFG_CPU_PROGRAM_MAP(sio4_mem)
    MCFG_CPU_IO_MAP(sio4_io)

    MCFG_MACHINE_RESET(dual68)

    /* video hardware */
    MCFG_FRAGMENT_ADD( generic_terminal )
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG,dual68_terminal_intf)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( dual68 )
    ROM_REGION( 0x2000, "user1", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS( 0, "v1", "2 * 4KB" )
	ROMX_LOAD( "dual_cpu68000_1.bin", 0x0001, 0x1000, CRC(d1785c08) SHA1(73c1f68875f1d8eb5e92f4347f509c61103da90f),ROM_SKIP(1) | ROM_BIOS(1))
	ROMX_LOAD( "dual_cpu68000_2.bin", 0x0000, 0x1000, CRC(b9f1ba3c) SHA1(8fd02936ad06d5a22d435d96f06e2442fc7d00ec),ROM_SKIP(1) | ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "v2", "2 * 2KB" )
	ROMX_LOAD( "dual.u2.bin", 0x0001, 0x0800, CRC(e9c44fcd) SHA1(d5cc609d6f5e6745d5f0af1aa6dc66012333ed60),ROM_SKIP(1) | ROM_BIOS(2))
    ROMX_LOAD( "dual.u3.bin", 0x0000, 0x0800, CRC(827b049f) SHA1(8209f8ab3d1068e5bab51e7eb12be46d4ea28354),ROM_SKIP(1) | ROM_BIOS(2))
	ROM_REGION( 0x10000, "siocpu", ROMREGION_ERASEFF )
	ROM_LOAD( "dual_sio4.bin", 0x0000, 0x0800, CRC(6b0a1965) SHA1(5d2dc6c6a315293ded4b9fc95c8ac1599bf31dd3))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY                        FULLNAME       FLAGS */
COMP( 1981, dual68,  0,       0,	dual68, 	dual68, 	 0,  "Dual Systems Corporation",   "Dual Systems 68000",		GAME_NOT_WORKING | GAME_NO_SOUND)

