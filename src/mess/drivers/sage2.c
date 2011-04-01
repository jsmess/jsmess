/***************************************************************************

        Sage II

        For memory map look at :
            htpp://www.thebattles.net/sage/img/SDT.pdf  (pages 14-)


        06/12/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "machine/msm8251.h"
#include "machine/terminal.h"


class sage2_state : public driver_device
{
public:
	sage2_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT16* m_ram;
};



static ADDRESS_MAP_START(sage2_mem, AS_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000000, 0x0007ffff) AM_RAM AM_BASE_MEMBER(sage2_state, m_ram) // 512 KB RAM / ROM at boot
	AM_RANGE(0x00fe0000, 0x00feffff) AM_ROM AM_REGION("user1",0)
//  AM_RANGE(0x00ffc070, 0x00ffc071 ) AM_DEVREADWRITE8("uart", msm8251_data_r,msm8251_data_w, 0xffff)
//  AM_RANGE(0x00ffc072, 0x00ffc073 ) AM_DEVREADWRITE8("uart", msm8251_status_r,msm8251_control_w, 0xffff)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( sage2 )
INPUT_PORTS_END


static MACHINE_RESET(sage2)
{
	sage2_state *state = machine.driver_data<sage2_state>();
	UINT8* user1 = machine.region("user1")->base();

	memcpy((UINT8*)state->m_ram,user1,0x2000);

	machine.device("maincpu")->reset();
}

static WRITE8_DEVICE_HANDLER( sage2_kbd_put )
{
}

static GENERIC_TERMINAL_INTERFACE( sage2_terminal_intf )
{
	DEVCB_HANDLER(sage2_kbd_put)
};

static MACHINE_CONFIG_START( sage2, sage2_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",M68000, XTAL_8MHz)
    MCFG_CPU_PROGRAM_MAP(sage2_mem)

    MCFG_MACHINE_RESET(sage2)

    /* video hardware */
    MCFG_FRAGMENT_ADD( generic_terminal )
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG,sage2_terminal_intf)

	/* uart */
	MCFG_MSM8251_ADD("uart", default_msm8251_interface)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( sage2 )
    ROM_REGION( 0x10000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "sage2.u18", 0x0000, 0x1000, CRC(ca9b312d) SHA1(99436a6d166aa5280c3b2d28355c4d20528fe48c))
	ROM_LOAD16_BYTE( "sage2.u17", 0x0001, 0x1000, CRC(27e25045) SHA1(041cd9d4617473d089f31f18cbb375046c3b61bb))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT   COMPANY                 FULLNAME       FLAGS */
COMP( 1982, sage2,  0,       0, 	sage2,		sage2,	 0, 	"Sage Technology",   "Sage II",		GAME_NOT_WORKING | GAME_NO_SOUND)

