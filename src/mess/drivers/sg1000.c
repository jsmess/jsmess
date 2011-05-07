/*

Sega SG-1000

PCB Layout
----------

171-5078 (C) SEGA 1983
171-5046 REV. A (C) SEGA 1983

|---------------------------|                              |----------------------------|
|   SW1     CN2             |   |------|---------------|   |    SW2     CN4             |
|                           |---|         CN3          |---|                            |
|  CN1                                                                              CN5 |
|                                                                                       |
|   10.738635MHz            |------------------------------|                7805        |
|   |---|                   |------------------------------|                            |
|   |   |                                 CN6                                           |
|   | 9 |                                                                               |
|   | 9 |                                                                       LS32    |
|   | 1 |       |---------|                                                             |
|   | 8 |       | TMM2009 |                                                     LS139   |
|   | A |       |---------|             |------------------|                            |
|   |   |                               |       Z80        |                            |
|   |---|                               |------------------|                            |
|                                                                                       |
|                                                                                       |
|       MB8118  MB8118  MB8118  MB8118              SN76489A            SW3             |
|           MB8118  MB8118  MB8118  MB8118                          LS257   LS257       |
|---------------------------------------------------------------------------------------|

Notes:
    All IC's shown.

    Z80     - NEC D780C-1 / Zilog Z8400A (REV.A) Z80A CPU @ 3.579545
    TMS9918A- Texas Instruments TMS9918ANL Video Display Processor @ 10.738635MHz
    MB8118  - Fujitsu MB8118-12 16K x 1 Dynamic RAM
    TMM2009 - Toshiba TMM2009P-A / TMM2009P-B (REV.A)
    SN76489A- Texas Instruments SN76489AN Digital Complex Sound Generator @ 3.579545
    CN1     - player 1 joystick connector
    CN2     - RF video connector
    CN3     - keyboard connector
    CN4     - power connector (+9VDC)
    CN5     - player 2 joystick connector
    CN6     - cartridge connector
    SW1     - TV channel select switch
    SW2     - power switch
    SW3     - hold switch

*/

/*

    TODO:

    - SC-3000 return instruction referenced by R when reading ports 60-7f,e0-ff
    - connect PSG /READY signal to Z80 WAIT
    - accurate video timing
    - SP-400 serial printer
    - SH-400 racing controller
    - SF-7000 serial comms

*/

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"
#include "imagedev/flopdrv.h"
#include "imagedev/cartslot.h"
#include "imagedev/cassette.h"
#include "machine/ram.h"
#include "imagedev/printer.h"
#include "formats/basicdsk.h"
#include "machine/ctronics.h"
#include "machine/i8255.h"
#include "machine/msm8251.h"
#include "machine/upd765.h"
#include "sound/sn76496.h"
#include "crsshair.h"
#include "includes/sg1000.h"

class sf7000_state : public sc3000_state
{
public:
	sf7000_state(const machine_config &mconfig, device_type type, const char *tag)
		: sc3000_state(mconfig, type, tag),
		  m_fdc(*this, UPD765_TAG),
		  m_centronics(*this, CENTRONICS_TAG),
		  m_floppy0(*this, FLOPPY_0)
	{ }

	required_device<device_t> m_fdc;
	required_device<device_t> m_centronics;
	required_device<device_t> m_floppy0;

	virtual void machine_start();
	virtual void machine_reset();

	DECLARE_READ8_MEMBER( ppi_pa_r );
	DECLARE_WRITE8_MEMBER( ppi_pc_w );
	DECLARE_WRITE_LINE_MEMBER( fdc_intrq_w );

	/* floppy state */
	int m_fdc_irq;
	int m_fdc_index;
};

/*-------------------------------------------------
    ADDRESS_MAP( sf7000_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( sf7000_map, AS_PROGRAM, 8, sf7000_state )
	AM_RANGE(0x0000, 0x3fff) AM_READ_BANK("bank1") AM_WRITE_BANK("bank2")
	AM_RANGE(0x4000, 0xffff) AM_RAM
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( sf7000_io_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( sf7000_io_map, AS_IO, 8, sf7000_state )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x7f, 0x7f) AM_DEVWRITE_LEGACY(SN76489A_TAG, sn76496_w)
	AM_RANGE(0xbe, 0xbe) AM_READWRITE_LEGACY(TMS9928A_vram_r, TMS9928A_vram_w)
	AM_RANGE(0xbf, 0xbf) AM_READWRITE_LEGACY(TMS9928A_register_r, TMS9928A_register_w)
	AM_RANGE(0xdc, 0xdf) AM_DEVREADWRITE(UPD9255_0_TAG, i8255_device, read, write)
	AM_RANGE(0xe0, 0xe0) AM_DEVREAD_LEGACY(UPD765_TAG, upd765_status_r)
	AM_RANGE(0xe1, 0xe1) AM_DEVREADWRITE_LEGACY(UPD765_TAG, upd765_data_r, upd765_data_w)
	AM_RANGE(0xe4, 0xe7) AM_DEVREADWRITE(UPD9255_1_TAG, i8255_device, read, write)
	AM_RANGE(0xe8, 0xe8) AM_DEVREADWRITE_LEGACY(UPD8251_TAG, msm8251_data_r, msm8251_data_w)
	AM_RANGE(0xe9, 0xe9) AM_DEVREADWRITE_LEGACY(UPD8251_TAG, msm8251_status_r, msm8251_control_w)
ADDRESS_MAP_END

/***************************************************************************
    INPUT PORTS
***************************************************************************/

/*-------------------------------------------------
    INPUT_PORTS( sf7000 )
-------------------------------------------------*/

static INPUT_PORTS_START( sf7000 )
	PORT_INCLUDE( sk1100 )

	PORT_START("BAUD")
	PORT_CONFNAME( 0x05, 0x05, "Baud rate")
	PORT_CONFSETTING( 0x00, "9600 baud" )
	PORT_CONFSETTING( 0x01, "4800 baud" )
	PORT_CONFSETTING( 0x02, "2400 baud" )
	PORT_CONFSETTING( 0x03, "1200 baud" )
	PORT_CONFSETTING( 0x04, "600 baud" )
	PORT_CONFSETTING( 0x05, "300 baud" )
INPUT_PORTS_END

/***************************************************************************
    DEVICE CONFIGURATION
***************************************************************************/

/*-------------------------------------------------
    I8255_INTERFACE( sf7000_ppi_intf )
-------------------------------------------------*/

READ8_MEMBER( sf7000_state::ppi_pa_r )
{
	/*
        Signal  Description

        PA0     INT from FDC
        PA1     BUSY from Centronics printer
        PA2     INDEX from FDD
        PA3
        PA4
        PA5
        PA6
        PA7
    */

	UINT8 data = 0;

	data |= m_fdc_irq;
	data |= centronics_busy_r(m_centronics) << 1;
	data |= m_fdc_index << 2;

	return data;
}

WRITE8_MEMBER( sf7000_state::ppi_pc_w )
{
	/*
        Signal  Description

        PC0     /INUSE signal to FDD
        PC1     /MOTOR ON signal to FDD
        PC2     TC signal to FDC
        PC3     RESET signal to FDC
        PC4     not connected
        PC5     not connected
        PC6     /ROM SEL (switch between IPL ROM and RAM)
        PC7     /STROBE to Centronics printer
    */

	/* floppy motor */
	floppy_mon_w(m_floppy0, BIT(data, 1));
	floppy_drive_set_ready_state(m_floppy0, 1, 1);

	/* FDC terminal count */
	upd765_tc_w(m_fdc, BIT(data, 2));

	/* FDC reset */
	if (BIT(data, 3))
	{
		upd765_reset(m_fdc, 0);
	}

	/* ROM selection */
	memory_set_bank(machine(), "bank1", BIT(data, 6));

	/* printer strobe */
	centronics_strobe_w(m_centronics, BIT(data, 7));
}

static I8255_INTERFACE( sf7000_ppi_intf )
{
	DEVCB_DRIVER_MEMBER(sf7000_state, ppi_pa_r),				// Port A read
	DEVCB_NULL,													// Port A write
	DEVCB_NULL,													// Port B read
	DEVCB_DEVICE_HANDLER(CENTRONICS_TAG, centronics_data_w),	// Port B write
	DEVCB_NULL,													// Port C read
	DEVCB_DRIVER_MEMBER(sf7000_state, ppi_pc_w)					// Port C write
};

/*-------------------------------------------------
    upd765_interface sf7000_upd765_interface
-------------------------------------------------*/

WRITE_LINE_MEMBER( sf7000_state::fdc_intrq_w )
{
	m_fdc_irq = state;
}

static const struct upd765_interface sf7000_upd765_interface =
{
	DEVCB_DRIVER_LINE_MEMBER(sf7000_state, fdc_intrq_w),
	DEVCB_NULL,
	NULL,
	UPD765_RDY_PIN_CONNECTED,
	{ FLOPPY_0, NULL, NULL, NULL }
};

/*-------------------------------------------------
    FLOPPY_OPTIONS( sf7000 )
-------------------------------------------------*/

static FLOPPY_OPTIONS_START( sf7000 )
	FLOPPY_OPTION(sf7000, "sf7", "SF7 disk image", basicdsk_identify_default, basicdsk_construct_default, NULL,
		HEADS([1])
		TRACKS([40])
		SECTORS([16])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

/*-------------------------------------------------
    floppy_config sf7000_floppy_config
-------------------------------------------------*/

static const floppy_config sf7000_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSHD,
	FLOPPY_OPTIONS_NAME(sf7000),
	NULL
};

/***************************************************************************
    MACHINE INITIALIZATION
***************************************************************************/

/*-------------------------------------------------
    sf7000_fdc_index_callback -
-------------------------------------------------*/

static void sf7000_fdc_index_callback(device_t *controller, device_t *img, int state)
{
	sf7000_state *driver_state = img->machine().driver_data<sf7000_state>();

	driver_state->m_fdc_index = state;
}

/*-------------------------------------------------
    MACHINE_START( sf7000 )
-------------------------------------------------*/

void sf7000_state::machine_start()
{
	/* configure VDP */
	TMS9928A_configure(&tms9928a_interface);

	/* configure FDC */
	floppy_drive_set_index_pulse_callback(m_floppy0, sf7000_fdc_index_callback);

	/* configure memory banking */
	memory_configure_bank(machine(), "bank1", 0, 1, machine().region(Z80_TAG)->base(), 0);
	memory_configure_bank(machine(), "bank1", 1, 1, ram_get_ptr(m_ram), 0);
	memory_configure_bank(machine(), "bank2", 0, 1, ram_get_ptr(m_ram), 0);

	/* register for state saving */
	state_save_register_global(machine(), m_keylatch);
	state_save_register_global(machine(), m_fdc_irq);
	state_save_register_global(machine(), m_fdc_index);
}

/*-------------------------------------------------
    MACHINE_RESET( sf7000 )
-------------------------------------------------*/

void sf7000_state::machine_reset()
{
	memory_set_bank(machine(), "bank1", 0);
	memory_set_bank(machine(), "bank2", 0);
}

/***************************************************************************
    MACHINE DRIVERS
***************************************************************************/


/*-------------------------------------------------
    MACHINE_CONFIG_START( sf7000, sf7000_state )
-------------------------------------------------*/

static MACHINE_CONFIG_START( sf7000, sf7000_state )
	/* basic machine hardware */
	MCFG_CPU_ADD(Z80_TAG, Z80, XTAL_10_738635MHz/3)
	MCFG_CPU_PROGRAM_MAP(sf7000_map)
	MCFG_CPU_IO_MAP(sf7000_io_map)
	MCFG_CPU_VBLANK_INT(SCREEN_TAG, sg1000_int)

    /* video hardware */
	MCFG_FRAGMENT_ADD(tms9928a)
	MCFG_SCREEN_MODIFY(SCREEN_TAG)
	MCFG_SCREEN_REFRESH_RATE((float)XTAL_10_738635MHz/2/342/262)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(SN76489A_TAG, SN76489A, XTAL_10_738635MHz/3)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* devices */
	MCFG_I8255_ADD(UPD9255_0_TAG, sc3000_ppi_intf)
	MCFG_I8255_ADD(UPD9255_1_TAG, sf7000_ppi_intf)
	MCFG_MSM8251_ADD(UPD8251_TAG, default_msm8251_interface)
	MCFG_UPD765A_ADD(UPD765_TAG, sf7000_upd765_interface)
	MCFG_FLOPPY_DRIVE_ADD(FLOPPY_0, sf7000_floppy_config)
//  MCFG_PRINTER_ADD("sp400") /* serial printer */
	MCFG_CENTRONICS_ADD(CENTRONICS_TAG, standard_centronics)
	MCFG_CASSETTE_ADD(CASSETTE_TAG, sc3000_cassette_config)

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("64K")
MACHINE_CONFIG_END

/***************************************************************************
    ROMS
***************************************************************************/

ROM_START( sf7000 )
	ROM_REGION( 0x10000, Z80_TAG, 0 )
	ROM_LOAD( "ipl.rom", 0x0000, 0x2000, CRC(d76810b8) SHA1(77339a6db2593aadc638bed77b8e9bed5d9d87e3) )
ROM_END

/***************************************************************************
    SYSTEM DRIVERS
***************************************************************************/

/*    YEAR  NAME        PARENT      COMPAT      MACHINE     INPUT       INIT    COMPANY             FULLNAME                                    FLAGS */
COMP( 1983,	sf7000,     sc3000,     0,          sf7000,     sf7000,     0,      "Sega",             "SC-3000/Super Control Station SF-7000",    GAME_SUPPORTS_SAVE )
