/***************************************************************************

        Sage II

        For memory map look at :
            http://www.thebattles.net/sage/img/SDT.pdf  (pages 14-)


        06/12/2009 Skeleton driver.

****************************************************************************/
#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "machine/i8251.h"
#include "machine/terminal.h"


class sage2_state : public driver_device
{
public:
	sage2_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

	DECLARE_WRITE8_MEMBER(kbd_put);
	UINT16* m_p_ram;
};



static ADDRESS_MAP_START(sage2_mem, AS_PROGRAM, 16, sage2_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x000000, 0x07ffff ) AM_RAM AM_BASE(m_p_ram) // 512 KB RAM / ROM at boot
//  Board #1
//	AM_RANGE(0xffc000, 0xffc007 ) // 8253-S, RTC
//	AM_RANGE(0xffc010, 0xffc01f ) // TMS9914, IEEE-488 Interface
//	AM_RANGE(0xffc020, 0xffc027 ) // i8255, DIPs + Floppy ctrl port
//	AM_RANGE(0xffc030, 0xffc033 ) // i8251, Serial Port 2
//	AM_RANGE(0xffc040, 0xffc045 ) // i8259
//	AM_RANGE(0xffc050, 0xffc051 ) // FDC 765, status
//	AM_RANGE(0xffc052, 0xffc053 ) // FDC 765, control
//	AM_RANGE(0xffc060, 0xffc067 ) // i8255, Printer
	AM_RANGE(0xffc070, 0xffc071 ) AM_DEVREADWRITE8("uart", i8251_device, data_r, data_w, 0x00ff) // terminal data
	AM_RANGE(0xffc072, 0xffc073 ) AM_DEVREADWRITE8("uart", i8251_device, status_r, control_w, 0x00ff) // terminal control/status
//	AM_RANGE(0xffc080, 0xffc087 ) // RTC? i8253-S
//  Board #2
//	AM_RANGE(0xffc400, 0xffc4ff ) // AUX Serial Channels (2651)
//	AM_RANGE(0xffc500, 0xffc7ff ) // Winchester drive ports

	AM_RANGE(0xfe0000, 0xfeffff ) AM_ROM AM_REGION("user1",0)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( sage2 )
INPUT_PORTS_END


static MACHINE_RESET(sage2)
{
	sage2_state *state = machine.driver_data<sage2_state>();
	UINT8* user1 = machine.region("user1")->base();

	memcpy((UINT8*)state->m_p_ram, user1, 0x2000);

	machine.device("maincpu")->reset();
}

WRITE8_MEMBER( sage2_state::kbd_put )
{
}

static GENERIC_TERMINAL_INTERFACE( terminal_intf )
{
	DEVCB_DRIVER_MEMBER(sage2_state, kbd_put)
};

static MACHINE_CONFIG_START( sage2, sage2_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", M68000, XTAL_8MHz)
	MCFG_CPU_PROGRAM_MAP(sage2_mem)

	MCFG_MACHINE_RESET(sage2)

	/* video hardware */
	MCFG_FRAGMENT_ADD( generic_terminal )
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, terminal_intf)

	/* uart */
	MCFG_I8251_ADD("uart", default_i8251_interface)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( sage2 )
	ROM_REGION( 0x10000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "sage2.u18", 0x0000, 0x1000, CRC(ca9b312d) SHA1(99436a6d166aa5280c3b2d28355c4d20528fe48c))
	ROM_LOAD16_BYTE( "sage2.u17", 0x0001, 0x1000, CRC(27e25045) SHA1(041cd9d4617473d089f31f18cbb375046c3b61bb))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT   COMPANY         FULLNAME       FLAGS */
COMP( 1982, sage2,  0,       0,      sage2,     sage2,    0, "Sage Technology", "Sage II", GAME_NOT_WORKING | GAME_NO_SOUND)
