/***************************************************************************
   
        PDP-11

        23/02/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/t11/t11.h"
#include "machine/terminal.h"

class pdp11_state : public driver_device
{
public:
	pdp11_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }
		
	UINT8 m_term_data;
	UINT16 m_term_status;
};

static WRITE16_HANDLER(term_w)
{
	device_t *devconf = space->machine().device("terminal");
	terminal_write(devconf,0,data);
}

static READ16_HANDLER(term_r)
{
	pdp11_state *state = space->machine().driver_data<pdp11_state>();
	state->m_term_status = 0x0000;
	return state->m_term_data;
}

static READ16_HANDLER(term_tx_status_r)
{   // always ready
	return 0xffff;
}

static READ16_HANDLER(term_rx_status_r)
{ 	
	pdp11_state *state = space->machine().driver_data<pdp11_state>();
	return state->m_term_status;
}

static ADDRESS_MAP_START(pdp11_mem, AS_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0xdfff ) AM_RAM  // RAM
	AM_RANGE( 0xea00, 0xfeff ) AM_ROM
	AM_RANGE( 0xff70, 0xff71 ) AM_READ(term_rx_status_r)
	AM_RANGE( 0xff72, 0xff73 ) AM_READ(term_r)
	AM_RANGE( 0xff74, 0xff75 ) AM_READ(term_tx_status_r)
	AM_RANGE( 0xff76, 0xff77 ) AM_WRITE(term_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(pdp11qb_mem, AS_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0xe9ff ) AM_RAM  // RAM
	AM_RANGE( 0xea00, 0xefff ) AM_ROM
	AM_RANGE( 0xf000, 0xffff ) AM_RAM
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( pdp11 )
	PORT_INCLUDE(generic_terminal)
INPUT_PORTS_END


static MACHINE_RESET(pdp11) 
{	
	// Load M9301-YA 
	UINT8* user1 = machine.region("user1")->base();
	UINT8* maincpu = machine.region("maincpu")->base();
	int i;
	
	for(i=0;i<0x100;i++) {
		UINT8 nib1 = user1[i+0x000] ^ 0x00;
		UINT8 nib2 = user1[i+0x200] ^ 0x01;
		UINT8 nib3 = user1[i+0x400] ^ 0x0f;
		UINT8 nib4 = user1[i+0x600] ^ 0x0e;
				
		maincpu[0xea00 + i*2 + 1] = (nib1 << 4) + nib2;
		maincpu[0xea00 + i*2 + 0] = (nib3 << 4) + nib4;
	}
	for(i=0x100;i<0x200;i++) {
		UINT8 nib1 = user1[i+0x000] ^ 0x00;
		UINT8 nib2 = user1[i+0x200] ^ 0x01;
		UINT8 nib3 = user1[i+0x400] ^ 0x0f;
		UINT8 nib4 = user1[i+0x600] ^ 0x0e;
				
		maincpu[0xf600 + (i-0x100)*2 + 1] = (nib1 << 4) + nib2;
		maincpu[0xf600 + (i-0x100)*2 + 0] = (nib3 << 4) + nib4;
	}
}

static MACHINE_RESET(pdp11ub2) 
{	
	// Load M9312
	UINT8* user1 = machine.region("user1")->base();
	UINT8* maincpu = machine.region("maincpu")->base();
	int i;
	
	//   3   2   1   8
	//   7   6   5   4
	// ~11 ~10   9   0
	//  15  14  13 ~12	
	for(i=0;i<0x100;i++) {
		UINT8 nib1 = user1[i*4+0];
		UINT8 nib2 = user1[i*4+1];
		UINT8 nib3 = user1[i*4+2];
		UINT8 nib4 = user1[i*4+3];
				
		maincpu[0xea00 + i*2 + 0] = (nib2 << 4) + ((nib1 & 0x0e) | (nib3 & 1));
		maincpu[0xea00 + i*2 + 1] = ((nib4 ^ 0x01)<<4) + ((nib1 & 0x01) | ((nib3 ^ 0x0c) & 0x0e));
	}
	
	cpu_set_reg(machine.device("maincpu"), T11_PC, 0xea10);	 // diag*/
	//cpu_set_reg(machine.device("maincpu"), T11_PC, 0xea64);	 // no-diag
}

static MACHINE_RESET(pdp11qb) 
{
	cpu_set_reg(machine.device("maincpu"), T11_PC, 0xea00);
}

static const struct t11_setup pdp11_data =
{
	6 << 13
};

static const struct t11_setup mxv11_data =
{
	0 << 13
};
static WRITE8_DEVICE_HANDLER( pdp_kbd_put )
{
	pdp11_state *state = device->machine().driver_data<pdp11_state>();
	state->m_term_data = data;
	state->m_term_status = 0xffff;
}

static GENERIC_TERMINAL_INTERFACE( pdp_terminal_intf )
{
	DEVCB_HANDLER(pdp_kbd_put)
};

static MACHINE_CONFIG_START( pdp11, pdp11_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",T11, XTAL_4MHz) // Need proper CPU here
	MCFG_CPU_CONFIG(pdp11_data)
    MCFG_CPU_PROGRAM_MAP(pdp11_mem)

    MCFG_MACHINE_RESET(pdp11)
	
    /* video hardware */
	MCFG_FRAGMENT_ADD( generic_terminal )
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG,pdp_terminal_intf)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( pdp11ub2, pdp11 )
    MCFG_MACHINE_RESET(pdp11ub2)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( pdp11qb, pdp11 )
    MCFG_MACHINE_RESET(pdp11qb)
	
	MCFG_CPU_MODIFY("maincpu")
	MCFG_CPU_CONFIG(mxv11_data)
	MCFG_CPU_PROGRAM_MAP(pdp11qb_mem)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( pdp11ub )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )	
	ROM_REGION( 0x1000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD( "23-034a9.bin", 0x0000, 0x0200, CRC(01c5d78d) SHA1(b447c67bfd5134c142240a919f23a949e1953fb2))
	ROM_LOAD( "23-035a9.bin", 0x0200, 0x0200, CRC(c456df6c) SHA1(188c8ece6a2d67911016f55dd22b698a40aff515))
	ROM_LOAD( "23-036a9.bin", 0x0400, 0x0200, CRC(208ff511) SHA1(27198a1110319b70674a72fd03a798dfa2c2109a))
	ROM_LOAD( "23-037a9.bin", 0x0600, 0x0200, CRC(d248b282) SHA1(ea638de6bde8342654d3e62b7810aa041e111913))
ROM_END

ROM_START( pdp11ub2 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )	
	ROM_REGION( 0x1000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD( "23-248f1.bin", 0x0000, 0x0400, CRC(ecda1a6d) SHA1(b2bf770dda349fdd469235871564280baf06301d))
	//ROM_LOAD( "23-616f1-1666.bin", 0x0000, 0x0400, CRC(a3dfb5aa) SHA1(7f06c624ae3fbb49535258b8722b5a3c548da3ba))
ROM_END

ROM_START( pdp11qb )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )	
	ROM_LOAD16_BYTE( "m7195fa.1", 0xc000, 0x2000, CRC(0fa58752) SHA1(4bcd006790a60f2998ee8377ac5e2c18ef330930))
	ROM_LOAD16_BYTE( "m7195fa.2", 0xc001, 0x2000, CRC(15b6f60c) SHA1(80dd4f8ca3c27babb5e75111b04241596a07c53a))
ROM_END
/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( ????, pdp11ub,  0,       0, 	pdp11, 	  pdp11, 	 0,   "Digital Equipment Corporation",   "PDP-11 [Unibus](M9301-YA)",		GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( ????, pdp11ub2, pdp11ub, 0, 	pdp11ub2, pdp11, 	 0,   "Digital Equipment Corporation",   "PDP-11 [Unibus](M9312)",		GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( ????, pdp11qb,  pdp11ub, 0, 	pdp11qb,  pdp11, 	 0,   "Digital Equipment Corporation",   "PDP-11 [Q-BUS] (M7195 - MXV11)",		GAME_NOT_WORKING | GAME_NO_SOUND)

