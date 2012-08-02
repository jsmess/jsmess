/***************************************************************************

        VAX-11

        02/08/2012 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/t11/t11.h"
#include "machine/terminal.h"

class vax11_state : public driver_device
{
public:
	vax11_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
	m_maincpu(*this, "maincpu"),
	m_terminal(*this, TERMINAL_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<generic_terminal_device> m_terminal;
	DECLARE_READ16_MEMBER( term_r );
	DECLARE_READ16_MEMBER( term_tx_status_r );
	DECLARE_READ16_MEMBER( term_rx_status_r );
	DECLARE_WRITE16_MEMBER( term_w );
	
	DECLARE_READ16_MEMBER( rxcs_r ) { return 040; } //m_rxcs; }
	DECLARE_WRITE16_MEMBER( rxcs_w ) {   }
	DECLARE_READ16_MEMBER( rxcs_2_r ) { return 00; } //m_rxcs; }
	DECLARE_WRITE16_MEMBER( rxcs_2_w ) {  }
	
	DECLARE_WRITE8_MEMBER( kbd_put );
	UINT8 m_term_data;
	UINT16 m_term_status;
};

WRITE16_MEMBER(vax11_state::term_w)
{
	m_terminal->write(space, 0, data);
}

READ16_MEMBER(vax11_state::term_r)
{
	m_term_status = 0x0000;
	return m_term_data;
}

READ16_MEMBER(vax11_state::term_tx_status_r)
{   // always ready
	return 0xffff;
}

READ16_MEMBER(vax11_state::term_rx_status_r)
{
	return m_term_status;
}

static ADDRESS_MAP_START(vax11_mem, AS_PROGRAM, 16, vax11_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0xbfff ) AM_RAM  // RAM
	AM_RANGE( 0xc000, 0xd7ff ) AM_ROM
	AM_RANGE( 0xfe78, 0xfe79 ) AM_READWRITE(rxcs_r, rxcs_w)	
	AM_RANGE( 0xfe7a, 0xfe7b ) AM_READWRITE(rxcs_2_r, rxcs_2_w)	

	AM_RANGE( 0xff70, 0xff71 ) AM_READ(term_rx_status_r)
	AM_RANGE( 0xff72, 0xff73 ) AM_READ(term_r)
	AM_RANGE( 0xff74, 0xff75 ) AM_READ(term_tx_status_r)
	AM_RANGE( 0xff76, 0xff77 ) AM_WRITE(term_w)	
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( vax11 )
INPUT_PORTS_END

static const struct t11_setup vax11_data =
{
	0 << 13
};

WRITE8_MEMBER( vax11_state::kbd_put )
{
	m_term_data = data;
	m_term_status = 0xffff;
}

static GENERIC_TERMINAL_INTERFACE( terminal_intf )
{
	DEVCB_DRIVER_MEMBER(vax11_state, kbd_put)
};

static MACHINE_CONFIG_START( vax11, vax11_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",T11, XTAL_4MHz) // Need proper CPU here
	MCFG_CPU_CONFIG(vax11_data)
	MCFG_CPU_PROGRAM_MAP(vax11_mem)
	
	/* video hardware */
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, terminal_intf)
MACHINE_CONFIG_END

ROM_START( vax785 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	// M8236
	ROMX_LOAD( "23-144f1-00.e56", 0xc000, 0x0400, CRC(99c1f117) SHA1(f05b6e97bf258392656058864abc1177379194da), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO | ROM_SKIP(1))
	ROMX_LOAD( "23-145f1-00.e68", 0xc000, 0x0400, CRC(098b63d2) SHA1(c2742aaccdac2921e1704c835ee5cef242cd7308), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI | ROM_SKIP(1))
	ROMX_LOAD( "23-146f1-00.e84", 0xc001, 0x0400, CRC(0f5f5d7b) SHA1(2fe325d2a78a8ce5146317cc39c084c4967c323c), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO | ROM_SKIP(1))
	ROMX_LOAD( "23-147f1-00.e72", 0xc001, 0x0400, CRC(bde386f2) SHA1(fcb5a1fa505912c5f44781619c9508cd142721e3), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI | ROM_SKIP(1))

	ROMX_LOAD( "23-148f1-00.e58", 0xc800, 0x0400, CRC(fe4c61e3) SHA1(4641a236761692a8f45b14ed6a73f535d57c2daa), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO | ROM_SKIP(1))
	ROMX_LOAD( "23-149f1-00.e70", 0xc800, 0x0400, CRC(a13f5f8a) SHA1(6a9d3b5a71a3249f3b9491d541c9854e071a320c), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI | ROM_SKIP(1))
	ROMX_LOAD( "23-150f1-00.e87", 0xc801, 0x0400, CRC(ca8d6419) SHA1(6d9c3e1e2f5a35f92c82240fcede14645aa83340), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO | ROM_SKIP(1))
	ROMX_LOAD( "23-151f1-00.e74", 0xc801, 0x0400, CRC(58ce48d3) SHA1(230dbcab1470752befb6733a89e3612ad7fba10d), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI | ROM_SKIP(1))

	ROMX_LOAD( "23-236f1-00.e57", 0xd000, 0x0400, CRC(6f23470a) SHA1(d90a0bc56f04c2830f8cfb6b870db207b96e75b1), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO | ROM_SKIP(1))
	ROMX_LOAD( "23-237f1-00.e69", 0xd000, 0x0400, CRC(2bf8cf0b) SHA1(6db79c5392b265e38b5b8b386528d7c138d995e9), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI | ROM_SKIP(1))
	ROMX_LOAD( "23-238f1-00.e85", 0xd001, 0x0400, CRC(ff569f71) SHA1(05985396047fb4639959000a1abe50d2f184deaa), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO | ROM_SKIP(1))
	ROMX_LOAD( "23-239f1-00.e73", 0xd001, 0x0400, CRC(cec7abe3) SHA1(8b8b52bd46340c58efa5adef3f306e0cdcb77520), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI | ROM_SKIP(1))
		
ROM_END

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( ????, vax785,  0,       0,	vax11,	  vax11,	 0,   "Digital Equipment Corporation",   "VAX 785",		GAME_NOT_WORKING | GAME_NO_SOUND)

