/***************************************************************************

    Acorn AEH26 FileStore E01

    20/05/2010 Skeleton driver.

    http://acorn.chriswhy.co.uk/Network/Econet.html
    http://acorn.chriswhy.co.uk/Network/Pics/Acorn_FileStoreE01.html
    http://acorn.chriswhy.co.uk/8bit_Upgrades/Acorn_FileStoreE01S.html
    http://www.heyrick.co.uk/econet/fs/emulator.html

****************************************************************************/

/*

    The FileStore E01 is an Econet station in its own right which acts as a fileserver when connected to a network. It is a single unit
    which does not require a monitor or keyboard. Communication with the FileStore is done via another Econet station when the FileStore
    is in one of two "maintenance modes"

    The E01 can be seen as a slimmed-down BBC computer tailored for its function as a fileserver. It has a 6502 processor at its heart
    along with a 6522 VIA just like the BBC. It requires a Master series Econet module to be plugged in and connects to the network via
    an Econet port in the same way as any other station.

    The FileStore E01S was Acorns second generation Filestore replacing the FileStore E01. The FileStore is a dedicated Econet fileserver,
    it does not support a keyboard and monitor, instead you use an Econet attached station to logon and perform administrative tasks.

            Hitachi HD146818P Real Time Clock
            Rockwell R65C102P3 CPU
            2 x TMM27256D-20 white labelled EPROMs, TMSE01 MOS on left and E01 FS on the right
    IC20    WD2793-APL-02 floppy disc controller
            2 x NEC D41464C-12 64k x 4bit NMOS RAM ICs giving 64K memory
    IC21    Rockwell RC6522AP VIA behind to the right

*/

/*

    TODO:

    - RTC interrupts (rewrite mc146818)
    - ADLC interrupts
    - ECONET device
    - printer
    - artwork
    - hard disk

*/

#include "includes/e01.h"
#include "e01.lh"

/***************************************************************************
    INTERRUPT HANDLING
***************************************************************************/

/*-------------------------------------------------
    update_interrupts - update interrupt state
-------------------------------------------------*/

void e01_state::update_interrupts()
{
	m_maincpu->set_input_line(INPUT_LINE_IRQ0, (m_via_irq || (m_hdc_ie & m_hdc_irq) || m_rtc_irq) ? ASSERT_LINE : CLEAR_LINE);
	m_maincpu->set_input_line(INPUT_LINE_NMI, (m_fdc_drq || (m_adlc_ie & m_adlc_irq)) ? ASSERT_LINE : CLEAR_LINE);
}

/*-------------------------------------------------
     network_irq_enable - network interrupt enable
-------------------------------------------------*/

void e01_state::network_irq_enable(int enabled)
{
	m_adlc_ie = enabled;

	update_interrupts();
}

/*-------------------------------------------------
     hdc_irq_enable - hard disk interrupt enable
-------------------------------------------------*/

void e01_state::hdc_irq_enable(int enabled)
{
	m_hdc_ie = enabled;

	update_interrupts();
}

/***************************************************************************
    READ/WRITE HANDLERS
***************************************************************************/

/*-------------------------------------------------
     eprom_r - ROM/RAM select read
-------------------------------------------------*/

READ8_MEMBER( e01_state::ram_select_r )
{
	memory_set_bank(m_machine, "bank1", 0);
	memory_set_bank(m_machine, "bank3", 0);

	return 0;
}

/*-------------------------------------------------
     floppy_w - floppy control write
-------------------------------------------------*/

WRITE8_MEMBER( e01_state::floppy_w )
{
	/*

        bit     description

        0       floppy 1 select
        1       floppy 2 select
        2       floppy side select
        3       NVRAM select
        4       floppy density
        5       floppy master reset
        6       floppy test
        7       mode LED

    */

	/* floppy 1 select */
	if (!BIT(data, 0)) wd17xx_set_drive(m_fdc, 0);

	/* floppy 2 select */
	if (!BIT(data, 1)) wd17xx_set_drive(m_fdc, 1);

	/* floppy side select */
	wd17xx_set_side(m_fdc, BIT(data, 2));

	/* TODO NVRAM select */
	//mc146818_stby_w(m_rtc, BIT(data, 3));

	/* floppy density */
	wd17xx_dden_w(m_fdc, BIT(data, 4));

	/* floppy master reset */
	wd17xx_mr_w(m_fdc, BIT(data, 5));

	/* TODO floppy test */
	//wd17xx_test_w(m_fdc, BIT(data, 6));

	/* mode LED */
	output_set_value("led_0", BIT(data, 7));
}

READ8_MEMBER( e01_state::network_irq_disable_r )
{
	network_irq_enable(0);

	return 0;
}

WRITE8_MEMBER( e01_state::network_irq_disable_w )
{
	network_irq_enable(0);
}

READ8_MEMBER( e01_state::network_irq_enable_r )
{
	network_irq_enable(1);

	return 0;
}

WRITE8_MEMBER( e01_state::network_irq_enable_w )
{
	network_irq_enable(1);
}

WRITE8_MEMBER( e01_state::hdc_irq_enable_w )
{
	hdc_irq_enable(BIT(data, 0));
}

READ8_MEMBER( e01_state::rtc_address_r )
{
	return m_rtc->read(space, 0);
}

WRITE8_MEMBER( e01_state::rtc_address_w )
{
	m_rtc->write(space, 0, data);
}

READ8_MEMBER( e01_state::rtc_data_r )
{
	return m_rtc->read(space, 1);
}

WRITE8_MEMBER( e01_state::rtc_data_w )
{
	m_rtc->write(space, 1, data);
}

/***************************************************************************
    MEMORY MAPS
***************************************************************************/

/*-------------------------------------------------
    ADDRESS_MAP( e01_mem )
-------------------------------------------------*/

static ADDRESS_MAP_START( e01_mem, AS_PROGRAM, 8, e01_state )
	AM_RANGE(0x0000, 0xfbff) AM_READ_BANK("bank1") AM_WRITE_BANK("bank2")
	AM_RANGE(0xfc00, 0xfc00) AM_MIRROR(0x00c3) AM_READWRITE(rtc_address_r, rtc_address_w)
	AM_RANGE(0xfc04, 0xfc04) AM_MIRROR(0x00c3) AM_READWRITE(rtc_data_r, rtc_data_w)
	AM_RANGE(0xfc08, 0xfc08) AM_MIRROR(0x00c0) AM_READ(ram_select_r) AM_WRITE(floppy_w)
	AM_RANGE(0xfc0c, 0xfc0f) AM_MIRROR(0x00c0) AM_DEVREADWRITE_LEGACY(WD2793_TAG, wd17xx_r, wd17xx_w)
	AM_RANGE(0xfc10, 0xfc1f) AM_MIRROR(0x00c0) AM_DEVREADWRITE(R6522_TAG, via6522_device, read, write)
	AM_RANGE(0xfc20, 0xfc23) AM_MIRROR(0x00c0) AM_DEVREADWRITE_LEGACY(MC6854_TAG, mc6854_r, mc6854_w)
	AM_RANGE(0xfc24, 0xfc24) AM_MIRROR(0x00c3) AM_READWRITE(network_irq_disable_r, network_irq_disable_w)
	AM_RANGE(0xfc28, 0xfc28) AM_MIRROR(0x00c3) AM_READWRITE(network_irq_enable_r, network_irq_enable_w)
	AM_RANGE(0xfc2c, 0xfc2c) AM_MIRROR(0x00c3) AM_READ_PORT("FLAP")
//  AM_RANGE(0xfc30, 0xfc30) AM_MIRROR(0x00c0) HD data
//  AM_RANGE(0xfc31, 0xfc31) AM_MIRROR(0x00c0) HD status
//  AM_RANGE(0xfc32, 0xfc32) AM_MIRROR(0x00c0) HD select
	AM_RANGE(0xfc33, 0xfc33) AM_MIRROR(0x00c0) AM_WRITE(hdc_irq_enable_w)
	AM_RANGE(0xfd00, 0xffff) AM_READ_BANK("bank3") AM_WRITE_BANK("bank4")
ADDRESS_MAP_END

/***************************************************************************
    INPUT PORTS
***************************************************************************/

/*-------------------------------------------------
    INPUT_PORTS( e01 )
-------------------------------------------------*/

static INPUT_PORTS_START( e01 )
	PORT_START("FLAP")
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_CONFNAME( 0x40, 0x00, "Front Flap")
	PORT_CONFSETTING( 0x00, "Closed" )
	PORT_CONFSETTING( 0x40, "Open" )
	PORT_DIPNAME( 0x80, 0x00, "SW3")
	PORT_DIPSETTING( 0x00, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x80, DEF_STR( On ) )
INPUT_PORTS_END

/***************************************************************************
    DEVICE CONFIGURATION
***************************************************************************/

/*-------------------------------------------------
    MC146818_INTERFACE( rtc_intf )
-------------------------------------------------*/

WRITE_LINE_MEMBER( e01_state::rtc_irq_w )
{
    m_rtc_irq = state;

    update_interrupts();
}

static TIMER_DEVICE_CALLBACK( rtc_irq_hack )
{
	e01_state *state = timer.machine().driver_data<e01_state>();

	state->m_rtc_irq = !state->m_rtc_irq;

	state->update_interrupts();
}

/*-------------------------------------------------
    mc6854_interface adlc_intf
-------------------------------------------------*/

WRITE_LINE_MEMBER( e01_state::adlc_irq_w )
{
    m_adlc_irq = state;

    update_interrupts();
}

static const mc6854_interface adlc_intf =
{
	NULL,
	NULL,
	NULL,
	NULL
};

/*-------------------------------------------------
    via6522_interface via_intf
-------------------------------------------------*/

WRITE_LINE_MEMBER( e01_state::via_irq_w )
{
	m_via_irq = state;

	update_interrupts();
}

static const via6522_interface via_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DRIVER_LINE_MEMBER(e01_state, via_irq_w)
};

/*-------------------------------------------------
    floppy_config e01_floppy_config
-------------------------------------------------*/

static const floppy_config e01_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_3_5_DSDD, // NEC FD1036 A
	FLOPPY_OPTIONS_NAME(default),
	NULL
};

/*-------------------------------------------------
    wd17xx_interface fdc_intf
-------------------------------------------------*/

WRITE_LINE_MEMBER( e01_state::fdc_drq_w )
{
	m_fdc_drq = state;

	update_interrupts();
}

static const wd17xx_interface fdc_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DRIVER_LINE_MEMBER(e01_state, fdc_drq_w),
	{ FLOPPY_0, FLOPPY_1, NULL, NULL }
};

/***************************************************************************
    MACHINE INITIALIZATION
***************************************************************************/

/*-------------------------------------------------
    MACHINE_START( e01 )
-------------------------------------------------*/

void e01_state::machine_start()
{
	UINT8 *ram = ram_get_ptr(m_ram);
	UINT8 *rom = m_machine.region(R65C102_TAG)->base();

	/* setup memory banking */
	memory_configure_bank(m_machine, "bank1", 0, 1, ram, 0);
	memory_configure_bank(m_machine, "bank1", 1, 1, rom, 0);
	memory_set_bank(m_machine, "bank1", 1);

	memory_configure_bank(m_machine, "bank2", 0, 1, ram, 0);
	memory_set_bank(m_machine, "bank2", 0);

	memory_configure_bank(m_machine, "bank3", 0, 1, ram + 0xfd00, 0);
	memory_configure_bank(m_machine, "bank3", 1, 1, rom + 0xfd00, 0);
	memory_set_bank(m_machine, "bank3", 1);

	memory_configure_bank(m_machine, "bank4", 0, 1, ram + 0xfd00, 0);
	memory_set_bank(m_machine, "bank4", 0);

	/* register for state saving */
	save_item(NAME(m_adlc_ie));
	save_item(NAME(m_hdc_ie));
	save_item(NAME(m_rtc_irq));
	save_item(NAME(m_via_irq));
	save_item(NAME(m_hdc_irq));
	save_item(NAME(m_fdc_drq));
	save_item(NAME(m_adlc_irq));
}

/*-------------------------------------------------
    MACHINE_RESET( e01 )
-------------------------------------------------*/

void e01_state::machine_reset()
{
	memory_set_bank(m_machine, "bank1", 1);
	memory_set_bank(m_machine, "bank3", 1);
}

/***************************************************************************
    MACHINE DRIVERS
***************************************************************************/

/*-------------------------------------------------
    MACHINE_DRIVER( e01 )
-------------------------------------------------*/

static MACHINE_CONFIG_START( e01, e01_state )
    /* basic machine hardware */
	MCFG_CPU_ADD(R65C102_TAG, M65C02, 1000000) // Rockwell R65C102P3
    MCFG_CPU_PROGRAM_MAP(e01_mem)

	MCFG_MC146818_ADD(HD146818_TAG, MC146818_STANDARD)

	MCFG_TIMER_ADD_PERIODIC("rtc_hack", rtc_irq_hack, attotime::from_hz(2)) // HACK!

	/* video hardware */
	MCFG_DEFAULT_LAYOUT( layout_e01 )

	/* devices */
	MCFG_VIA6522_ADD(R6522_TAG, 100000, via_intf)
	MCFG_MC6854_ADD(MC6854_TAG, adlc_intf)
	MCFG_WD2793_ADD(WD2793_TAG, fdc_intf)
	MCFG_FLOPPY_2_DRIVES_ADD(e01_floppy_config)
//  MCFG_CENTRONICS_ADD(CENTRONICS_TAG, e01_centronics_config)

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("64K")
MACHINE_CONFIG_END

/***************************************************************************
    ROMS
***************************************************************************/

/*-------------------------------------------------
    ROM( e01 )
-------------------------------------------------*/

ROM_START( e01 )
    ROM_REGION( 0x10000, R65C102_TAG, 0 )
	ROM_DEFAULT_BIOS("v131")
	ROM_SYSTEM_BIOS( 0, "v131", "V 1.31" )
	ROMX_LOAD( "0254,205-04 e01 fs",  0x0000, 0x8000, CRC(ae666c76) SHA1(0954119eb5cd09cdbadf76d60d812aa845838d5a), ROM_BIOS(1) )
	ROMX_LOAD( "0254,205-03 e01 mos", 0x8000, 0x8000, CRC(a13e8014) SHA1(6f44a1a48108c60a64a1774cb30c1a59c4a6a199), ROM_BIOS(1) )
ROM_END

/*-------------------------------------------------
    ROM( e01s )
-------------------------------------------------*/

ROM_START( e01s )
    ROM_REGION( 0x10000, R65C102_TAG, 0 )
	ROM_DEFAULT_BIOS("v140")
	ROM_SYSTEM_BIOS( 0, "v133", "V 1.33" ) // 0282,008-02 e01s rom
	ROMX_LOAD( "e01sv133.rom",  0x0000, 0x10000, CRC(2a4a0032) SHA1(54ad68ceae44992293ccdd64ec88ad8520deec22), ROM_BIOS(1) ) // which label?
	ROM_SYSTEM_BIOS( 1, "v140", "V 1.40" )
	ROMX_LOAD( "e01sv140.rom",  0x0000, 0x10000, CRC(5068fe86) SHA1(9b8740face15b5541e2375b3054988af00757931), ROM_BIOS(2) ) // which label?
ROM_END

/***************************************************************************
    SYSTEM DRIVERS
***************************************************************************/

/*    YEAR  NAME      PARENT    COMPAT  MACHINE   INPUT     INIT      COMPANY   FULLNAME */
COMP( 1988, e01,  0,       0,	e01,	e01,	 0,  "Acorn",   "FileStore E01",		GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 1988, e01s, e01,       0,	e01,	e01,	 0,  "Acorn",   "FileStore E01S",		GAME_NOT_WORKING | GAME_NO_SOUND)
