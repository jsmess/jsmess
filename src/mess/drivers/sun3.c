/***************************************************************************

        Sun 3/150 

	Processor(s):   68020 @ 16.67MHz, 68881, Sun-3 MMU, 8 hardware
			contexts, 2 MIPS
	CPU:            501-1074/1094/1163/1164/1208
	Chassis type:   deskside
	Bus:            VME, 6 slots
	Memory:         16M physical (documented), 256M virtual, 270ns cycle


        25/08/2009 Skeleton driver.

****************************************************************************/
#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/m68000/m68000.h"

#define MACHINE_RESET_MEMBER(name) void name::machine_reset()

class sun3_state : public driver_device
{
public:
	sun3_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
	m_maincpu(*this, "maincpu")
	{ }

	required_device<cpu_device> m_maincpu;
	virtual void machine_reset();
	
	UINT16* m_p_ram;
};

static ADDRESS_MAP_START(sun3_mem, AS_PROGRAM, 32, sun3_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000000, 0x00ffffff) AM_RAM AM_BASE(m_p_ram) // 16MB
	AM_RANGE(0x0fef0000, 0x0fefffff) AM_ROM AM_REGION("user1",0)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( sun3 )
INPUT_PORTS_END


MACHINE_RESET_MEMBER(sun3_state)
{
	UINT8* user1 = machine().region("user1")->base();

	memcpy((UINT8*)m_p_ram,user1,0x10000);

	machine().device("maincpu")->reset();
}


static MACHINE_CONFIG_START( sun3, sun3_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", M68020, 16670000)
	MCFG_CPU_PROGRAM_MAP(sun3_mem)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( sun3 )
	ROM_REGION32_BE( 0x10000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD( "sun3-carrera-rev-3.0.bin", 0x0000, 0x10000, CRC(fee6e4d6) SHA1(440d532e1848298dba0f043de710bb0b001fb675))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY         FULLNAME       FLAGS */
COMP( 1982, sun3,  0,       0,       sun3,      sun3,     0,  "Sun Microsystems", "Sun 3/150", GAME_NOT_WORKING | GAME_NO_SOUND)

