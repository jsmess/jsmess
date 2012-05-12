/***************************************************************************

        Dream Multimedia Dreambox 7000/5620/500

        20/03/2010 Skeleton driver.


    DM7000 -    CPU STB04500 at 252 MHz
                RAM 64MB
                Flash 8MB
                1 x DVB-S
                1 x IDE interface
                1 x Common Interface (CI)
                1 x Compact flash
                2 x Smart card reader
                1 x USB
                1 x RS232
                1 x LAN 100Mbit/s
                1 x LCD display

    DM56x0 -    CPU STB04500 at 252 MHz
                RAM 64MB
                Flash 8MB
                1 x DVB-S
                2 x Common Interface (CI)
                1 x Smart card reader
                1 x LAN 100Mbit/s (just on 5620)
                1 x LCD display

    DM500 -     CPU STBx25xx at 252 MHz
                RAM 96MB
                Flash 32MB
                1 x DVB-S
                1 x Smart card reader
                1 x LAN 100Mbit/s

****************************************************************************/

#include "emu.h"
#include "cpu/powerpc/ppc.h"
#include "machine/terminal.h"

#define VERBOSE_LEVEL ( 0 )

INLINE void ATTR_PRINTF(3,4) verboselog( running_machine &machine, int n_level, const char *s_fmt, ...)
{
	if (VERBOSE_LEVEL >= n_level)
	{
		va_list v;
		char buf[32768];
		va_start( v, s_fmt);
		vsprintf( buf, s_fmt, v);
		va_end( v);
		logerror( "%s: %s", machine.describe_context( ), buf);
	}
}

class dm7000_state : public driver_device
{
public:
	dm7000_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_terminal(*this, TERMINAL_TAG)		{ }

	required_device<cpu_device> m_maincpu;
	required_device<generic_terminal_device> m_terminal;
	
	DECLARE_WRITE8_MEMBER ( dm7000_iic0_w );
	DECLARE_READ8_MEMBER ( dm7000_iic0_r );
	DECLARE_WRITE8_MEMBER ( dm7000_iic1_w );
	DECLARE_READ8_MEMBER ( dm7000_iic1_r );
	
	DECLARE_WRITE8_MEMBER ( dm7000_scc0_w );
	DECLARE_READ8_MEMBER ( dm7000_scc0_r );
	UINT8 m_scc0_lcr;
	UINT8 m_scc0_lsr;
	
	DECLARE_WRITE8_MEMBER ( dm7000_gpio0_w );
	DECLARE_READ8_MEMBER ( dm7000_gpio0_r );
	
	DECLARE_WRITE8_MEMBER ( dm7000_scp0_w );
	DECLARE_READ8_MEMBER ( dm7000_scp0_r );
	
	UINT32			dcr[512];	
};

READ8_MEMBER( dm7000_state::dm7000_iic0_r )
{
	UINT32 data = 0; // dummy
	verboselog( machine(), 9, "(IIC0) %08X -> %08X (PC %08X)\n", 0x40030000 + offset, data, cpu_get_pc(m_maincpu));
	return data;
}

WRITE8_MEMBER( dm7000_state::dm7000_iic0_w )
{
	verboselog( machine(), 9, "(IIC0) %08X <- %08X (PC %08X)\n", 0x40030000 + offset, data, cpu_get_pc(m_maincpu));
}

READ8_MEMBER( dm7000_state::dm7000_iic1_r )
{
	UINT32 data = 0; // dummy
	verboselog( machine(), 9, "(IIC1) %08X -> %08X (PC %08X)\n", 0x400b0000 + offset, data, cpu_get_pc(m_maincpu));
	return data;
}

WRITE8_MEMBER( dm7000_state::dm7000_iic1_w )
{
	verboselog( machine(), 9, "(IIC1) %08X <- %08X (PC %08X)\n", 0x400b0000 + offset, data, cpu_get_pc(m_maincpu));
}

#define UART_DLL	0
#define UART_RBR	0
#define UART_THR	0
#define UART_DLH	1
#define UART_IER	1
#define UART_IIR	2
#define UART_FCR	2
#define UART_LCR	3
#define 	UART_LCR_DLAB	0x80
#define UART_MCR	4
#define UART_LSR	5
#define		UART_LSR_TEMT	0x20
#define		UART_LSR_THRE	0x40
#define UART_MSR	6
#define UART_SCR	7

READ8_MEMBER( dm7000_state::dm7000_scc0_r )
{
	UINT32 data = 0; // dummy
	switch(offset) {
		case UART_LSR:
			data = UART_LSR_THRE | UART_LSR_TEMT;
			break;
	}
	verboselog( machine(), 9, "(SCC0) %08X -> %08X (PC %08X)\n", 0x40040000 + offset, data, cpu_get_pc(m_maincpu));
	return data;
}

WRITE8_MEMBER( dm7000_state::dm7000_scc0_w )
{
	switch(offset) {
		case UART_THR:
			if(!(m_scc0_lcr & UART_LCR_DLAB)) {
				m_terminal->write(space, 0, data);
			}
			break;
		case UART_LCR:
			m_scc0_lcr = data;
			break;
	}
	verboselog( machine(), 9, "(SCC0) %08X <- %08X (PC %08X)\n", 0x40040000 + offset, data, cpu_get_pc(m_maincpu));
}

READ8_MEMBER( dm7000_state::dm7000_gpio0_r )
{
	UINT32 data = 0; // dummy
	verboselog( machine(), 9, "(GPIO0) %08X -> %08X (PC %08X)\n", 0x40060000 + offset, data, cpu_get_pc(m_maincpu));
	return data;
}

WRITE8_MEMBER( dm7000_state::dm7000_gpio0_w )
{
	verboselog( machine(), 9, "(GPIO0) %08X <- %08X (PC %08X)\n", 0x40060000 + offset, data, cpu_get_pc(m_maincpu));
}

#define SCP_SPMODE 0
#define SCP_RXDATA 1
#define SCP_TXDATA 2
#define SCP_SPCOM 3
#define SCP_STATUS 4
#define 	SCP_STATUS_RXRDY 1
#define SCP_CDM 6

READ8_MEMBER( dm7000_state::dm7000_scp0_r )
{
	UINT32 data = 0; // dummy
	switch(offset) {
		case SCP_STATUS:
			data = SCP_STATUS_RXRDY;
			break;
	}
	verboselog( machine(), 9, "(SCP0) %08X -> %08X (PC %08X)\n", 0x400c0000 + offset, data, cpu_get_pc(m_maincpu));
	return data;
}

WRITE8_MEMBER( dm7000_state::dm7000_scp0_w )
{
	verboselog( machine(), 9, "(SCP0) %08X <- %08X (PC %08X)\n", 0x400c0000 + offset, data, cpu_get_pc(m_maincpu));
	switch(offset) {
		case SCP_TXDATA:
			//printf("%02X ", data);
			break;
	}
}

/*
 Memory map for the IBM "Redwood-4" STB03xxx evaluation board.

 The  STB03xxx internal i/o addresses don't work for us 1:1,
 so we need to map them at a well know virtual address.

 4000 000x   uart1
 4001 00xx   ppu
 4002 00xx   smart card
 4003 000x   iic
 4004 000x   uart0
 4005 0xxx   timer
 4006 00xx   gpio
 4007 00xx   smart card
 400b 000x   iic
 400c 000x   scp
 400d 000x   modem
 
 STB04xxx
 
 4000 00xx   Serial1/Infrared Controller
 4001 00xx   Universal Serial Bus
 4002 00xx   Smart Card Interface 0
 4003 000x   IIC Interface 0
 4004 000x   Serial0/Uart750 Controller
 4005 0xxx   General Purpose Timers
 4006 00xx   General Purpose Input / Output
 4007 00xx   Smart Card Interface 1
 400b 000x   IIC Interface 1
 400c 000x   Serial Controller Port
 400d 00xx   Synchronous Serial Port
 400e 000x   Serial2/UART750 Controller
 400f 0xxx   IDE Controller
  
*/
static ADDRESS_MAP_START( dm7000_mem, AS_PROGRAM, 32, dm7000_state )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000000, 0x01ffffff) AM_RAM										// RAM page 0 - 32MB
	AM_RANGE(0x20000000, 0x21ffffff) AM_RAM										// RAM page 1 - 32MB
	
	AM_RANGE(0x40030000, 0x4003000f) AM_READWRITE8(dm7000_iic0_r, dm7000_iic0_w, 0xffffffff)
	AM_RANGE(0x40040000, 0x40040007) AM_READWRITE8(dm7000_scc0_r, dm7000_scc0_w, 0xffffffff)
	AM_RANGE(0x40060000, 0x40060047) AM_READWRITE8(dm7000_gpio0_r, dm7000_gpio0_w, 0xffffffff)
	AM_RANGE(0x400b0000, 0x400b000f) AM_READWRITE8(dm7000_iic1_r, dm7000_iic1_w, 0xffffffff)
	AM_RANGE(0x400c0000, 0x400c0007) AM_READWRITE8(dm7000_scp0_r, dm7000_scp0_w, 0xffffffff)
	
	AM_RANGE(0xfffe0000, 0xffffffff) AM_ROM AM_REGION("user1",0)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( dm7000 )
INPUT_PORTS_END


static MACHINE_RESET(dm7000)
{
	dm7000_state *state = machine.driver_data<dm7000_state>();
	
	state->m_scc0_lsr = UART_LSR_THRE | UART_LSR_TEMT;
}

static VIDEO_START( dm7000 )
{
}

static SCREEN_UPDATE_IND16( dm7000 )
{
	return 0;
}
#define DCRSTB045_CMD_STAT	 0x14a		/* Command status */

static READ32_DEVICE_HANDLER( dcr_r )
{
	dm7000_state *state = device->machine().driver_data<dm7000_state>();
	switch(offset) {
		case DCRSTB045_CMD_STAT:
			return 0; // assume that video dev is always ready
		default:
			return state->dcr[offset];
	}

}

static WRITE32_DEVICE_HANDLER( dcr_w )
{
	dm7000_state *state = device->machine().driver_data<dm7000_state>();
	state->dcr[offset] = data;
}

static GENERIC_TERMINAL_INTERFACE( terminal_intf )
{
	DEVCB_NULL
};

static const powerpc_config ppc405_config =
{
	252000000,
	dcr_r,
	dcr_w
};

static MACHINE_CONFIG_START( dm7000, dm7000_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",PPC405GP, 252000000) // Should be PPC405
	MCFG_CPU_PROGRAM_MAP(dm7000_mem)

	MCFG_MACHINE_RESET(dm7000)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_SIZE(640, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MCFG_SCREEN_UPDATE_STATIC(dm7000)

	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(black_and_white)
	
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, terminal_intf)

	MCFG_VIDEO_START(dm7000)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( dm7000 )
	ROM_REGION( 0x20000, "user1", ROMREGION_32BIT | ROMREGION_BE  )
	ROMX_LOAD( "dm7000.bin", 0x0000, 0x20000, CRC(8a410f67) SHA1(9d6c9e4f5b05b28453d3558e69a207f05c766f54), ROM_GROUPWORD )
ROM_END

ROM_START( dm5620 )
	ROM_REGION( 0x20000, "user1", ROMREGION_32BIT | ROMREGION_BE  )
	ROMX_LOAD( "dm5620.bin", 0x0000, 0x20000, CRC(ccddb822) SHA1(3ecf553ced0671599438368f59d8d30df4d13ade), ROM_GROUPWORD )
ROM_END

ROM_START( dm500 )
	ROM_REGION( 0x20000, "user1", ROMREGION_32BIT | ROMREGION_BE )
	ROM_SYSTEM_BIOS( 0, "alps", "Alps" )
	ROMX_LOAD( "dm500-alps-boot.bin",   0x0000, 0x20000, CRC(daf2da34) SHA1(68f3734b4589fcb3e73372e258040bc8b83fd739), ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "phil", "Philips" )
	ROMX_LOAD( "dm500-philps-boot.bin", 0x0000, 0x20000, CRC(af3477c7) SHA1(9ac918f6984e6927f55bea68d6daaf008787136e), ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(2))
ROM_END
/* Driver */

/*    YEAR  NAME     PARENT   COMPAT   MACHINE    INPUT    INIT     COMPANY                FULLNAME       FLAGS */
SYST( 2003, dm7000,  0,       0,       dm7000,    dm7000,  0,   "Dream Multimedia",   "Dreambox 7000", GAME_NOT_WORKING | GAME_NO_SOUND)
SYST( 2004, dm5620,  dm7000,  0,       dm7000,    dm7000,  0,   "Dream Multimedia",   "Dreambox 5620", GAME_NOT_WORKING | GAME_NO_SOUND)
SYST( 2006, dm500,   dm7000,  0,       dm7000,    dm7000,  0,   "Dream Multimedia",   "Dreambox 500", GAME_NOT_WORKING | GAME_NO_SOUND)
