/***************************************************************************
Video Technology Genius computers:
	VTech Genius PC (France)
	VTech Genius IQ 512 (Germany)
	The French packaging mentions distributions in Switzerland, the Netherlands,
	USA, Canada, and UK as well. Looking for more information and ROM dumps.

System driver:

	Adrien Destugues <pulkomandy@gmail.com>, May 2012
	  - First attempt

Memory map:
	00000000 System ROM (2MB)
	00200000 RAM (256K)
	00600000 Some memory mapped hardware
	???????? Flash memory (128K)
	???????? Cartridge port

TODO:
    - Mostly everything besides CPU, RAM and ROM
    - Check with different countries ROMs
	
Not very much is known about this computer released in 1997. 

****************************************************************************/

#include "emu.h"
#include "cpu/m68000/m68000.h"

#define MACHINE_RESET_MEMBER(name) void name::machine_reset()

class geniusiq_state : public driver_device
{
public:
	geniusiq_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
	m_maincpu(*this, "maincpu")
	{ }

	required_device<cpu_device> m_maincpu;
	virtual void machine_reset();
};

static ADDRESS_MAP_START(geniusiq_mem, AS_PROGRAM, 16, geniusiq_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x000000, 0x1FFFFF) AM_ROM
	AM_RANGE(0x200000, 0x23FFFF) AM_RAM
	// 0x600000 : some memory mapped hardware
	// Somewhere : internal flash
	// Somewhere : cartridge port
ADDRESS_MAP_END


/* Input ports */
static INPUT_PORTS_START( geniusiq )
INPUT_PORTS_END


MACHINE_RESET_MEMBER( geniusiq_state )
{
}

static MACHINE_CONFIG_START( geniusiq, geniusiq_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", M68000, 16000000) // The main crystal is at 32MHz, not sure whats the CPU freq
	MCFG_CPU_PROGRAM_MAP(geniusiq_mem)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( geniusiq )
	ROM_REGION(0x200000, "maincpu", 0)
	ROM_LOAD( "geniusiq.bin", 0x0000, 0x200000, CRC(9b06cbf1) SHA1(b9438494a9575f78117c0033761f899e3c14e292) )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY                FULLNAME               FLAGS */
COMP( 1997, geniusiq,   0,       0,    geniusiq,   geniusiq,  0,  "Video Technology", "Genius IQ 128", GAME_NO_SOUND_HW)
