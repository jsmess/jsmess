/***************************************************************************

        Sun 2/120 

	Processor(s):   68010 @ 10MHz
	CPU:            501-1007/1051
	Chassis type:   deskside
	Bus:            Multibus, 9 slots
	Memory:         7M physical with mono video, 8M without
	Notes:          First machines in deskside chassis. Serial
			microswitch keyboard (type 2), Mouse Systems
			optical mouse (Sun-2).

        25/08/2009 Skeleton driver.

****************************************************************************/
#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/m68000/m68000.h"

#define MACHINE_RESET_MEMBER(name) void name::machine_reset()

class sun2_state : public driver_device
{
public:
	sun2_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
	m_maincpu(*this, "maincpu")
	{ }

	required_device<cpu_device> m_maincpu;
	virtual void machine_reset();
	
	UINT16* m_p_ram;
};

static ADDRESS_MAP_START(sun2_mem, AS_PROGRAM, 16, sun2_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000000, 0x007fffff) AM_RAM AM_BASE(m_p_ram) // 8MB
	AM_RANGE(0x00ef0000, 0x00ef7fff) AM_ROM AM_REGION("user1",0)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( sun2 )
INPUT_PORTS_END


MACHINE_RESET_MEMBER(sun2_state)
{
	UINT8* user1 = machine().region("user1")->base();

	memcpy((UINT8*)m_p_ram,user1,0x8000);

	machine().device("maincpu")->reset();
}


static MACHINE_CONFIG_START( sun2, sun2_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", M68010, 16670000)
	MCFG_CPU_PROGRAM_MAP(sun2_mem)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( sun2 )
	ROM_REGION( 0x8000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD16_WORD_SWAP( "sun2-multi-rev-r.bin", 0x0000, 0x8000, CRC(4df0df77) SHA1(4d6bcf09ddc9cc8f5823847b8ea88f98fe4a642e))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY         FULLNAME       FLAGS */
COMP( 1982, sun2,  0,       0,       sun2,      sun2,     0,  "Sun Microsystems", "Sun 2/120", GAME_NOT_WORKING | GAME_NO_SOUND)

