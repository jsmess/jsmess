/***************************************************************************

        Elektor SC/MP

        22/11/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/scmp/scmp.h"
#include "elekscmp.lh"


class elekscmp_state : public driver_device
{
public:
	elekscmp_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};


static WRITE8_HANDLER(hex_display_w)
{
	output_set_digit_value(7-offset, data);
}

static READ8_HANDLER(keyboard_r)
{
	return 0;
}

static ADDRESS_MAP_START(elekscmp_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0x0fff)
	AM_RANGE(0x000, 0x5ff) AM_ROM // ROM
	AM_RANGE(0x700, 0x707) AM_WRITE(hex_display_w)
	AM_RANGE(0x708, 0x70f) AM_READ(keyboard_r)
	AM_RANGE(0x800, 0xfff) AM_RAM // RAM - up to 2K of RAM
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( elekscmp )
INPUT_PORTS_END


static MACHINE_RESET(elekscmp)
{
}

static MACHINE_CONFIG_START( elekscmp, elekscmp_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",INS8060, XTAL_4MHz)
    MCFG_CPU_PROGRAM_MAP(elekscmp_mem)

    MCFG_MACHINE_RESET(elekscmp)

    /* video hardware */
	MCFG_DEFAULT_LAYOUT(layout_elekscmp)

MACHINE_CONFIG_END

/* ROM definition */
ROM_START( elekscmp )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	// Too many possible errors, few found and fixed, but not sure if there are more
	ROM_LOAD( "elbug.001", 0x0000, 0x0200, BAD_DUMP CRC(f733da28) SHA1(b65d98be03eab80478167964beec26bb327bfdf3))
	ROM_LOAD( "elbug.002", 0x0200, 0x0200, BAD_DUMP CRC(529c0b88) SHA1(bd72dd890cd974e1744ca70aa3457657374cbf76))
	ROM_LOAD( "elbug.003", 0x0400, 0x0200, BAD_DUMP CRC(13585ad1) SHA1(93f722b3e84095a1b701b04bf9018c891933b9ff))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1977, elekscmp,  0,       0,	elekscmp,	elekscmp,	 0,  "Elektor Electronics",   "Elektor SC/MP",		GAME_NOT_WORKING | GAME_NO_SOUND)

