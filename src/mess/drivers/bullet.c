/*

Wave Mate Bullet

PCB Layout
----------

|-------------------------------------------|
|                           CN7     CN6     |
|                                   PIO     |
|                           FDC             |
|       4164 4164                   CN5     |
|       4164 4164       SW1              CN2|
|       4164 4164                           |
|       4164 4164                   PROM    |
|       4164 4164   4.9152MHz               |
|       4164 4164   16MHz   DMA     CPU     |
|       4164 4164                           |
|       4164 4164           CN4 CN3      CN1|
|                                           |
|                           DART    CTC     |
|                                   CN8     |
|-------------------------------------------|

Notes:
    Relevant IC's shown.

    CPU     - SGS Z80ACPUB1 Z80A CPU
    DMA     - Zilog Z8410APS Z80A DMA
    PIO     - SGS Z80APIOB1 Z80A PIO
    DART    - Zilog Z8470APS Z80A DART
    CTC     - Zilog Z8430APS Z80A CTC
    FDC     - Synertek SY1793-02 FDC
    PROM    - AMD AM27S190C 32x8 TTL PROM
    4164    - Fujitsu MB8264-15 64Kx1 RAM
    CN1     - 2x25 PCB header, external DMA bus
    CN2     - 2x17 PCB header, Winchester / 2x25 header, SCSI (in board revision E)
    CN3     - 2x5 PCB header, RS-232 A (system console)
    CN4     - 2x5 PCB header, RS-232 B
    CN5     - 2x17 PCB header, Centronics
    CN6     - 2x25 PCB header, 8" floppy drives
    CN7     - 2x17 PCB header, 5.25" floppy drives
    CN8     - 4-pin Molex

*/

/*

    TODO:

    - wmb_org.imd does not load
    - z80dart wait/ready
    - floppy type dips
    - Winchester hard disk
    - revision E model

*/

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"
#include "imagedev/flopdrv.h"
#include "machine/ram.h"
#include "machine/ctronics.h"
#include "machine/terminal.h"
#include "machine/wd17xx.h"
#include "machine/z80ctc.h"
#include "machine/z80dart.h"
#include "machine/z80dma.h"
#include "machine/z80pio.h"
#include "includes/bullet.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

// DMA ready sources
enum {
	FDRDY = 0,
	DARTARDY,
	DARTBRDY,
	WINRDY,
	EXRDY1,
	EXRDY2
};


//**************************************************************************
//  READ/WRITE HANDLERS
//**************************************************************************

//-------------------------------------------------
//  bankswitch -
//-------------------------------------------------

void bullet_state::bankswitch()
{
	if (m_brom)
	{
		memory_set_bank(m_machine, "bank1", 2);
	}
	else
	{
		memory_set_bank(m_machine, "bank1", m_segst);
	}

	memory_set_bank(m_machine, "bank2", m_segst);
	memory_set_bank(m_machine, "bank3", m_segst);
	memory_set_bank(m_machine, "bank4", 0);
}


//-------------------------------------------------
//  update_dma_rdy -
//-------------------------------------------------

void bullet_state::update_dma_rdy()
{
	int rdy = 1;

	switch (m_exdma & 0x07)
	{
	case FDRDY:
		rdy = m_fdrdy;
		break;

	case DARTARDY:
		rdy = m_dartardy;
		break;

	case DARTBRDY:
		rdy = m_dartbrdy;
		break;

	case WINRDY:
		rdy = m_winrdy;
		break;

	case EXRDY1:
		rdy = m_exrdy1;
		break;

	case EXRDY2:
		rdy = m_exrdy2;
		break;
	}

	z80dma_rdy_w(m_dmac, rdy);
}


//-------------------------------------------------
//  info_r -
//-------------------------------------------------

READ8_MEMBER( bullet_state::info_r )
{
	/*

        bit     signal      description

        0                   DIP switch 1
        1                   DIP switch 2
        2                   DIP switch 3
        3                   DIP switch 4
        4       HLDST       floppy disk head load status
        5       *XDCG       floppy disk exchange (8" only)
        6       FDIRQ       FDC interrupt request line
        7       FDDRQ       FDC data request line

    */

	UINT8 data = 0x10;

	// DIP switches
	data |= input_port_read(m_machine, "SW1") & 0x0f;

	// floppy interrupt
	data |= wd17xx_intrq_r(m_fdc) << 6;

	// floppy data request
	data |= wd17xx_drq_r(m_fdc) << 7;

	return data;
}


//-------------------------------------------------
//  win_r -
//-------------------------------------------------

READ8_MEMBER( bullet_state::win_r )
{
	return 0;
}


//-------------------------------------------------
//  wstrobe_w -
//-------------------------------------------------

WRITE8_MEMBER( bullet_state::wstrobe_w )
{
}


//-------------------------------------------------
//  brom_r -
//-------------------------------------------------

READ8_MEMBER( bullet_state::brom_r )
{
	m_brom = 0;
	bankswitch();

	return 0;
}


//-------------------------------------------------
//  brom_w -
//-------------------------------------------------

WRITE8_MEMBER( bullet_state::brom_w )
{
	m_brom = 0;
	bankswitch();
}


//-------------------------------------------------
//  exdsk_w -
//-------------------------------------------------

WRITE8_MEMBER( bullet_state::exdsk_w )
{
	/*

        bit     signal      description

        0                   drive select 0
        1                   drive select 1
        2                   select 8" floppy
        3       MSE         select software control of port
        4       SIDE        select side 2
        5       _MOTOR      disable 5" floppy spindle motors
        6
        7       WPRC        enable write precompensation

    */

	// drive select
	wd17xx_set_drive(m_fdc, data & 0x03);

	// side select
	wd17xx_set_side(m_fdc, BIT(data, 4));

	// floppy motor
	floppy_mon_w(m_floppy0, BIT(data, 5));
	floppy_mon_w(m_floppy1, BIT(data, 5));
	floppy_drive_set_ready_state(m_floppy0, 1, 1);
	floppy_drive_set_ready_state(m_floppy1, 1, 1);
}


//-------------------------------------------------
//  exdma_w -
//-------------------------------------------------

WRITE8_MEMBER( bullet_state::exdma_w )
{
	/*

        bit     description

        0       DMA ready source select 0
        1       DMA ready source select 1
        2       DMA ready source select 2
        3       memory control 0
        4       memory control 1
        5
        6
        7

    */

	m_exdma = data;

	m_buf = BIT(data, 3);

	update_dma_rdy();
}


//-------------------------------------------------
//  hdcon_w -
//-------------------------------------------------

WRITE8_MEMBER( bullet_state::hdcon_w )
{
	/*

        bit     signal  description

        0       PLO     phase lock oscillator
        1       RCD     read clock frequency
        2       EXC     MB8877 clock frequency
        3       DEN     MB8877 density select
        4               enable software control of mode
        5
        6
        7

    */

	// FDC clock
	m_fdc->set_unscaled_clock(BIT(data, 2) ? XTAL_16MHz/16 : XTAL_16MHz/8);

	// density select
	wd17xx_dden_w(m_fdc, BIT(data, 3));
}


//-------------------------------------------------
//  segst_w -
//-------------------------------------------------

WRITE8_MEMBER( bullet_state::segst_w )
{
	m_segst = BIT(data, 0);
	bankswitch();
}



//**************************************************************************
//  ADDRESS MAPS
//**************************************************************************

//-------------------------------------------------
//  ADDRESS_MAP( bullet_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( bullet_mem, AS_PROGRAM, 8, bullet_state )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x001f) AM_READ_BANK("bank1") AM_WRITE_BANK("bank2")
	AM_RANGE(0x0020, 0xbfff) AM_RAMBANK("bank3")
	AM_RANGE(0xc000, 0xffff) AM_RAMBANK("bank4")
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( bullet_io )
//-------------------------------------------------

static ADDRESS_MAP_START( bullet_io, AS_IO, 8, bullet_state )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
//  AM_RANGE(0x00, 0x00) AM_DEVWRITE_LEGACY(TERMINAL_TAG, terminal_write)
	AM_RANGE(0x00, 0x03) AM_DEVREADWRITE_LEGACY(Z80DART_TAG, z80dart_ba_cd_r, z80dart_ba_cd_w)
	AM_RANGE(0x04, 0x07) AM_DEVREADWRITE_LEGACY(Z80PIO_TAG, z80pio_cd_ba_r, z80pio_cd_ba_w)
	AM_RANGE(0x08, 0x0b) AM_DEVREADWRITE_LEGACY(Z80CTC_TAG, z80ctc_r, z80ctc_w)
	AM_RANGE(0x0c, 0x0c) AM_MIRROR(0x03) AM_READWRITE(win_r, wstrobe_w)
	AM_RANGE(0x10, 0x13) AM_DEVREADWRITE_LEGACY(MB8877_TAG, wd17xx_r, wd17xx_w)
	AM_RANGE(0x14, 0x14) AM_DEVREADWRITE_LEGACY(Z80DMA_TAG, z80dma_r, z80dma_w)
	AM_RANGE(0x15, 0x15) AM_READWRITE(brom_r, brom_w)
	AM_RANGE(0x16, 0x16) AM_WRITE(exdsk_w)
	AM_RANGE(0x17, 0x17) AM_WRITE(exdma_w)
	AM_RANGE(0x18, 0x18) AM_WRITE(hdcon_w)
	AM_RANGE(0x19, 0x19) AM_READ(info_r)
	AM_RANGE(0x1a, 0x1a) AM_WRITE(segst_w)
ADDRESS_MAP_END



//**************************************************************************
//  INPUT PORTS
//**************************************************************************

//-------------------------------------------------
//  INPUT_PORTS( bullet )
//-------------------------------------------------

INPUT_PORTS_START( bullet )

	PORT_START("SW1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) ) PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) ) PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) ) PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) ) PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0xf0, 0xf0, "Floppy Type" ) PORT_DIPLOCATION("SW1:5,6,7,8")
	// TODO
INPUT_PORTS_END



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  Z80CTC_INTERFACE( ctc_intf )
//-------------------------------------------------

static TIMER_DEVICE_CALLBACK( ctc_tick )
{
	bullet_state *state = timer.machine().driver_data<bullet_state>();

	z80ctc_trg0_w(state->m_ctc, 1);
	z80ctc_trg0_w(state->m_ctc, 0);

	z80ctc_trg1_w(state->m_ctc, 1);
	z80ctc_trg1_w(state->m_ctc, 0);

	z80ctc_trg2_w(state->m_ctc, 1);
	z80ctc_trg2_w(state->m_ctc, 0);
}

static WRITE_LINE_DEVICE_HANDLER( dart_rxtxca_w )
{
	z80dart_txca_w(device, state);
	z80dart_rxca_w(device, state);
}

static Z80CTC_INTERFACE( ctc_intf )
{
	0,              									// timer disables
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),		// interrupt handler
	DEVCB_DEVICE_LINE(Z80DART_TAG, dart_rxtxca_w),		// ZC/TO0 callback
	DEVCB_DEVICE_LINE(Z80DART_TAG, z80dart_rxtxcb_w),	// ZC/TO1 callback
	DEVCB_LINE(z80ctc_trg3_w)							// ZC/TO2 callback
};


//-------------------------------------------------
//  Z80DART_INTERFACE( dart_intf )
//-------------------------------------------------

WRITE_LINE_MEMBER( bullet_state::dartardy_w )
{
	m_dartardy = state;
	update_dma_rdy();
}

WRITE_LINE_MEMBER( bullet_state::dartbrdy_w )
{
	m_dartbrdy = state;
	update_dma_rdy();
}

static Z80DART_INTERFACE( dart_intf )
{
	0, 0, 0, 0,

	DEVCB_DEVICE_LINE(TERMINAL_TAG, terminal_serial_r),
	DEVCB_DEVICE_LINE(TERMINAL_TAG, terminal_serial_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DRIVER_LINE_MEMBER(bullet_state, dartardy_w),
	DEVCB_NULL,

	DEVCB_LINE_VCC,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DRIVER_LINE_MEMBER(bullet_state, dartbrdy_w),
	DEVCB_NULL,

	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0)
};


//-------------------------------------------------
//  Z80DMA_INTERFACE( dma_intf )
//-------------------------------------------------

READ8_MEMBER( bullet_state::dma_mreq_r )
{
	if (m_buf)
	{
		offset |= 0x10000;
	}

	UINT8 *ram = ram_get_ptr(m_ram);
	UINT8 data = ram[offset];

	if (BIT(m_exdma, 4))
	{
		m_buf = !m_buf;
	}

	return data;
}

WRITE8_MEMBER( bullet_state::dma_mreq_w )
{
	if (m_buf)
	{
		offset |= 0x10000;
	}

	UINT8 *ram = ram_get_ptr(m_ram);
	ram[offset] = data;

	if (BIT(m_exdma, 4))
	{
		m_buf = !m_buf;
	}
}

static UINT8 memory_read_byte(address_space *space, offs_t address) { return space->read_byte(address); }
static void memory_write_byte(address_space *space, offs_t address, UINT8 data) { space->write_byte(address, data); }

static Z80DMA_INTERFACE( dma_intf )
{
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_HALT),
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),
	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(bullet_state, dma_mreq_r),
	DEVCB_DRIVER_MEMBER(bullet_state, dma_mreq_w),
	DEVCB_MEMORY_HANDLER(Z80_TAG, IO, memory_read_byte),
	DEVCB_MEMORY_HANDLER(Z80_TAG, IO, memory_write_byte)
};


//-------------------------------------------------
//  Z80PIO_INTERFACE( pio_intf )
//-------------------------------------------------

READ8_MEMBER( bullet_state::pio_pb_r )
{
	/*

        bit     signal      description

        0                   centronics busy
        1                   centronics paper end
        2                   centronics selected
        3       *FAULT      centronics fault
        4                   external vector
        5       WBUSDIR     winchester bus direction
        6       WCOMPLETE   winchester command complete
        7       *WINRDY     winchester ready

    */

	UINT8 data = 0;

	// centronics busy
	data |= centronics_busy_r(m_centronics);

	// centronics paper end
	data |= centronics_pe_r(m_centronics) << 1;

	// centronics selected
	data |= centronics_vcc_r(m_centronics) << 2;

	// centronics fault
	data |= centronics_fault_r(m_centronics) << 3;

	return data;
}

static Z80PIO_INTERFACE( pio_intf )
{
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),
	DEVCB_NULL,
	DEVCB_DEVICE_HANDLER(CENTRONICS_TAG, centronics_data_w),
	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(bullet_state, pio_pb_r),
	DEVCB_NULL,
	DEVCB_NULL
};


//-------------------------------------------------
//  wd17xx_interface fdc_intf
//-------------------------------------------------

WRITE_LINE_MEMBER( bullet_state::fdrdy_w )
{
	m_fdrdy = !state;
	update_dma_rdy();
}

static const floppy_config bullet_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSDD,
	FLOPPY_OPTIONS_NAME(default),
	"bullet_flop"
};

static const wd17xx_interface fdc_intf =
{
	DEVCB_NULL,
	DEVCB_DEVICE_LINE(Z80DART_TAG, z80dart_dcda_w),
	DEVCB_DRIVER_LINE_MEMBER(bullet_state, fdrdy_w),
	{ FLOPPY_0, FLOPPY_1, NULL, NULL }
};


//-------------------------------------------------
//  GENERIC_TERMINAL_INTERFACE( terminal_intf )
//-------------------------------------------------

static GENERIC_TERMINAL_INTERFACE( terminal_intf )
{
	DEVCB_NULL
};


//-------------------------------------------------
//  z80_daisy_config daisy_chain
//-------------------------------------------------

static const z80_daisy_config daisy_chain[] =
{
	{ Z80DMA_TAG },
	{ Z80DART_TAG },
	{ Z80PIO_TAG },
	{ Z80CTC_TAG },
	{ NULL }
};



//**************************************************************************
//  MACHINE INITIALIZATION
//**************************************************************************

//-------------------------------------------------
//  MACHINE_START( bullet )
//-------------------------------------------------

void bullet_state::machine_start()
{
	// setup memory banking
	UINT8 *ram = ram_get_ptr(m_ram);

	memory_configure_bank(m_machine, "bank1", 0, 2, ram, 0x10000);
	memory_configure_bank(m_machine, "bank1", 2, 1, m_machine.region(Z80_TAG)->base(), 0);
	memory_configure_bank(m_machine, "bank2", 0, 2, ram, 0x10000);
	memory_configure_bank(m_machine, "bank3", 0, 2, ram + 0x0020, 0x10000);
	memory_configure_bank(m_machine, "bank4", 0, 1, ram + 0xc000, 0);

	// register for state saving
	state_save_register_global(m_machine, m_segst);
	state_save_register_global(m_machine, m_brom);
	state_save_register_global(m_machine, m_exdma);
	state_save_register_global(m_machine, m_buf);
	state_save_register_global(m_machine, m_fdrdy);
	state_save_register_global(m_machine, m_dartardy);
	state_save_register_global(m_machine, m_dartbrdy);
	state_save_register_global(m_machine, m_winrdy);
	state_save_register_global(m_machine, m_exrdy1);
	state_save_register_global(m_machine, m_exrdy2);
}


//-------------------------------------------------
//  MACHINE_RESET( bullet )
//-------------------------------------------------

void bullet_state::machine_reset()
{
	// memory banking
	m_segst = 0;
	m_brom = 1;
	bankswitch();

	// DMA ready
	m_exdma = 0;
	update_dma_rdy();
}



//**************************************************************************
//  MACHINE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  MACHINE_CONFIG( bullet )
//-------------------------------------------------

static MACHINE_CONFIG_START( bullet, bullet_state )
    // basic machine hardware
    MCFG_CPU_ADD(Z80_TAG, Z80, XTAL_16MHz/4)
    MCFG_CPU_PROGRAM_MAP(bullet_mem)
    MCFG_CPU_IO_MAP(bullet_io)
	MCFG_CPU_CONFIG(daisy_chain)

    // video hardware
	MCFG_FRAGMENT_ADD( generic_terminal )

	// devices
	MCFG_Z80CTC_ADD(Z80CTC_TAG, XTAL_16MHz/4, ctc_intf)
	MCFG_TIMER_ADD_PERIODIC("ctc", ctc_tick, attotime::from_hz(XTAL_4_9152MHz/4))
	MCFG_Z80DART_ADD(Z80DART_TAG, XTAL_16MHz/4, dart_intf)
	MCFG_Z80DMA_ADD(Z80DMA_TAG, XTAL_16MHz/4, dma_intf)
	MCFG_Z80PIO_ADD(Z80PIO_TAG, XTAL_16MHz/4, pio_intf)
	MCFG_WD179X_ADD(MB8877_TAG, fdc_intf)
	MCFG_FLOPPY_2_DRIVES_ADD(bullet_floppy_config)
	MCFG_CENTRONICS_ADD(CENTRONICS_TAG, standard_centronics)
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, terminal_intf)

	// internal ram
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("128K")
MACHINE_CONFIG_END



//**************************************************************************
//  ROMS
//**************************************************************************

//-------------------------------------------------
//  ROM( bullet )
//-------------------------------------------------

ROM_START( bullet )
    ROM_REGION( 0x10000, Z80_TAG, ROMREGION_ERASEFF )
	ROM_LOAD( "sr70x.u8", 0x0000, 0x0020, CRC(d54b8a30) SHA1(65ff8753dd63c9dd1899bc9364a016225585d050) )
ROM_END



//**************************************************************************
//  SYSTEM DRIVERS
//**************************************************************************

//    YEAR  NAME        PARENT      COMPAT  MACHINE     INPUT       INIT    COMPANY         FULLNAME                FLAGS
COMP( 1982, bullet,		0,			0,		bullet,		bullet,		0,		"Wave Mate",	"Bullet",				GAME_NOT_WORKING | GAME_SUPPORTS_SAVE | GAME_NO_SOUND_HW )
//COMP( 1982, bullete,  bullet,     0,      bullet,     bullet,     0,      "Wave Mate",    "Bullet (Revision E)",  GAME_NOT_WORKING | GAME_SUPPORTS_SAVE | GAME_NO_SOUND_HW )
