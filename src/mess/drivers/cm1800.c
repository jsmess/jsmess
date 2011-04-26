/***************************************************************************
   
        CM-1800
		(note name is in cyrilic letters)
		
		more info at http://ru.wikipedia.org/wiki/%D0%A1%D0%9C_%D0%AD%D0%92%D0%9C
			and http://sapr.lti-gti.ru/index.php?id=66
			
        26/04/2011 Skeleton driver.

****************************************************************************/

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "machine/terminal.h"

class cm1800_state : public driver_device
{
public:
	cm1800_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	DECLARE_READ8_MEMBER( term_ready_r );
	DECLARE_READ8_MEMBER( term_r );
	DECLARE_WRITE8_MEMBER( term_ready_w );
	DECLARE_WRITE8_MEMBER( kbd_put );
	
	UINT8 m_key_ready;
	UINT8 m_term_data;
};

READ8_MEMBER( cm1800_state::term_ready_r )
{
	return 0x04 | m_key_ready; // always ready to write
}

WRITE8_MEMBER( cm1800_state::term_ready_w )
{
	m_key_ready = 0;
	m_term_data = 0;
}

READ8_MEMBER( cm1800_state::term_r )
{
	m_key_ready = 0;
	return m_term_data;
}

WRITE8_MEMBER( cm1800_state::kbd_put )
{
	m_key_ready = 1;
	m_term_data = data;
}

static ADDRESS_MAP_START(cm1800_mem, AS_PROGRAM, 8, cm1800_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x07ff ) AM_ROM
	AM_RANGE( 0x0800, 0xffff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( cm1800_io , AS_IO, 8, cm1800_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00, 0x00) AM_READ(term_r) AM_DEVWRITE_LEGACY(TERMINAL_TAG, terminal_write)
	AM_RANGE(0x01, 0x01) AM_READ(term_ready_r)
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( cm1800 )
INPUT_PORTS_END


static MACHINE_RESET(cm1800) 
{	
}

static GENERIC_TERMINAL_INTERFACE( cm1800_terminal_intf )
{
	DEVCB_DRIVER_MEMBER(cm1800_state, kbd_put)
};

static MACHINE_CONFIG_START( cm1800, cm1800_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",I8080, XTAL_2MHz)
    MCFG_CPU_PROGRAM_MAP(cm1800_mem)
    MCFG_CPU_IO_MAP(cm1800_io)	

    MCFG_MACHINE_RESET(cm1800)
	
    /* video hardware */
    MCFG_FRAGMENT_ADD( generic_terminal )
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG,cm1800_terminal_intf)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( cm1800 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "cm1800.rom", 0x0000, 0x0800, CRC(85d71d25) SHA1(42dc87d2eddc2906fa26d35db88a2e29d50fb481))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1981, cm1800,  0,       0, 	cm1800, 	cm1800, 	 0,  "<unknown>",   "CM-1800",		GAME_NOT_WORKING | GAME_NO_SOUND)

