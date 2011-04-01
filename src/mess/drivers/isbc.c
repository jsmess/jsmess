/***************************************************************************

        Intel iSBC series

        09/12/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/i86/i86.h"
#include "cpu/i86/i286.h"
#include "machine/terminal.h"


class isbc_state : public driver_device
{
public:
	isbc_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 m_received_char;
};



static WRITE16_HANDLER(isbc_terminal_w)
{
	device_t *devconf = space->machine().device(TERMINAL_TAG);
	terminal_write(devconf,0,data);
}

static READ16_HANDLER(isbc_terminal_status_r)
{
	isbc_state *state = space->machine().driver_data<isbc_state>();
	if (state->m_received_char!=0) return 3; // char received
	return 1; // ready
}

static READ16_HANDLER(isbc_terminal_r)
{
	isbc_state *state = space->machine().driver_data<isbc_state>();
	UINT8 retVal = state->m_received_char;
	state->m_received_char = 0;
	return retVal;
}

static ADDRESS_MAP_START(rpc86_mem, AS_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000, 0x0ffff) AM_RAM
	AM_RANGE(0xfc000, 0xfffff) AM_ROM AM_REGION("user1",0)
ADDRESS_MAP_END

static ADDRESS_MAP_START( rpc86_io , AS_IO, 16)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

static ADDRESS_MAP_START(isbc86_mem, AS_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000, 0xfbfff) AM_RAM
	AM_RANGE(0xfc000, 0xfffff) AM_ROM AM_REGION("user1",0)
ADDRESS_MAP_END

static ADDRESS_MAP_START( isbc86_io , AS_IO, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00da, 0x00db) AM_READ(isbc_terminal_status_r)
	AM_RANGE(0x00d8, 0x00d9) AM_READWRITE(isbc_terminal_r, isbc_terminal_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(isbc286_mem, AS_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000, 0xdffff) AM_RAM
	AM_RANGE(0xe0000, 0xfffff) AM_ROM AM_REGION("user1",0)
ADDRESS_MAP_END

static ADDRESS_MAP_START( isbc286_io , AS_IO, 16)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

static ADDRESS_MAP_START(isbc2861_mem, AS_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000, 0xeffff) AM_RAM
	AM_RANGE(0xf0000, 0xfffff) AM_ROM AM_REGION("user1",0)
ADDRESS_MAP_END

static ADDRESS_MAP_START( isbc2861_io , AS_IO, 16)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( isbc )
INPUT_PORTS_END


static MACHINE_RESET(isbc)
{
	isbc_state *state = machine.driver_data<isbc_state>();
	state->m_received_char = 0;
}

static WRITE8_DEVICE_HANDLER( isbc_kbd_put )
{
	isbc_state *state = device->machine().driver_data<isbc_state>();
	state->m_received_char = data;
}

static GENERIC_TERMINAL_INTERFACE( isbc_terminal_intf )
{
	DEVCB_HANDLER(isbc_kbd_put)
};

static MACHINE_CONFIG_START( isbc86, isbc_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu", I8086, XTAL_9_8304MHz)
    MCFG_CPU_PROGRAM_MAP(isbc86_mem)
    MCFG_CPU_IO_MAP(isbc86_io)

    MCFG_MACHINE_RESET(isbc)

    /* video hardware */
    MCFG_FRAGMENT_ADD( generic_terminal )
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, isbc_terminal_intf)
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( rpc86, isbc_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu", I8086, XTAL_9_8304MHz)
    MCFG_CPU_PROGRAM_MAP(rpc86_mem)
    MCFG_CPU_IO_MAP(rpc86_io)

    MCFG_MACHINE_RESET(isbc)

    /* video hardware */
    MCFG_FRAGMENT_ADD( generic_terminal )
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, isbc_terminal_intf)
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( isbc286, isbc_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu", I80286, XTAL_9_8304MHz)
    MCFG_CPU_PROGRAM_MAP(isbc286_mem)
    MCFG_CPU_IO_MAP(isbc286_io)

    MCFG_MACHINE_RESET(isbc)

    /* video hardware */
    MCFG_FRAGMENT_ADD( generic_terminal )
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, isbc_terminal_intf)
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( isbc2861, isbc_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu", I80286, XTAL_9_8304MHz)
    MCFG_CPU_PROGRAM_MAP(isbc2861_mem)
    MCFG_CPU_IO_MAP(isbc2861_io)

    MCFG_MACHINE_RESET(isbc)

    /* video hardware */
    MCFG_FRAGMENT_ADD( generic_terminal )
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, isbc_terminal_intf)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( isbc86 )
    ROM_REGION( 0x4000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "8612_2u.bin", 0x0001, 0x1000, CRC(84fa14cf) SHA1(783e1459ab121201fd49368d4bf769c1bab6447a))
	ROM_LOAD16_BYTE( "8612_2l.bin", 0x0000, 0x1000, CRC(922bda5f) SHA1(15743e69f3aba56425fa004d19b82ec20532fd72))
	ROM_LOAD16_BYTE( "8612_3u.bin", 0x2001, 0x1000, CRC(68d47c3e) SHA1(16c17f26b33daffa84d065ff7aefb581544176bd))
	ROM_LOAD16_BYTE( "8612_3l.bin", 0x2000, 0x1000, CRC(17f27ad2) SHA1(c3f379ac7d67dc4a0a7a611a0bc6323b8a3d4840))
ROM_END

ROM_START( isbc286 )
    ROM_REGION( 0x20000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "u79.bin", 0x00001, 0x10000, CRC(144182ea) SHA1(4620ca205a6ac98fe2636183eaead7c4bfaf7a72))
	ROM_LOAD16_BYTE( "u36.bin", 0x00000, 0x10000, CRC(22db075f) SHA1(fd29ea77f5fc0697c8f8b66aca549aad5b9db3ea))
	ROM_REGION( 0x4000, "isbc215", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "174581.001.bin", 0x0000, 0x2000, CRC(ccdbc7ab) SHA1(5c2ebdde1b0252124177221ba9cacdb6d925a24d))
	ROM_LOAD16_BYTE( "174581.002.bin", 0x0001, 0x2000, CRC(6190fa67) SHA1(295dd4e75f699aaf93227cc4876cee8accae383a))
ROM_END

ROM_START( isbc2861 )
    ROM_REGION( 0x10000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "174894-001.bin", 0x0000, 0x4000, CRC(79e4f7af) SHA1(911a4595d35e6e82b1149e75bb027927cd1c1658))
	ROM_LOAD16_BYTE( "174894-002.bin", 0x0001, 0x4000, CRC(66747d21) SHA1(4094b1f10a8bc7db8d6dd48d7128e14e875776c7))
	ROM_LOAD16_BYTE( "174894-003.bin", 0x8000, 0x4000, CRC(c98c7f17) SHA1(6e9a14aedd630824dccc5eb6052867e73b1d7db6))
	ROM_LOAD16_BYTE( "174894-004.bin", 0x8001, 0x4000, CRC(61bc1dc9) SHA1(feed5a5f0bb4630c8f6fa0d5cca30654a80b4ee5))
ROM_END

ROM_START( rpc86 )
    ROM_REGION( 0x4000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "145068-001.bin", 0x0001, 0x1000, CRC(0fa9db83) SHA1(4a44f8683c263c9ef6850cbe05aaa73f4d4d4e06))
	ROM_LOAD16_BYTE( "145069-001.bin", 0x2001, 0x1000, CRC(1692a076) SHA1(0ce3a4a867cb92340871bb8f9c3e91ce2984c77c))
	ROM_LOAD16_BYTE( "145070-001.bin", 0x0000, 0x1000, CRC(8c8303ef) SHA1(60f94daa76ab9dea6e309ac580152eb212b847a0))
	ROM_LOAD16_BYTE( "145071-001.bin", 0x2000, 0x1000, CRC(a49681d8) SHA1(e81f8b092cfa2d1737854b1fa270a4ce07d61a9f))
ROM_END
/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT COMPANY   FULLNAME       FLAGS */
COMP( 19??, rpc86,  0,       0, 	rpc86,	isbc,	 0, 	  "Intel",   "RPC 86",			GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 19??, isbc86, 0,       0, 	isbc86,	isbc,	 0, 	  "Intel",   "iSBC 86/12A",		GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 19??, isbc286, 0,      0, 	isbc286,isbc,	 0, 	  "Intel",   "iSBC 286",		GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 19??, isbc2861, 0,      0,	isbc2861,isbc,	 0, 	  "Intel",   "iSBC 286-10",		GAME_NOT_WORKING | GAME_NO_SOUND)

