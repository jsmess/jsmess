/**********************************************************************

    Commodore 1540/1541/1541C/1541-II Single Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

    TODO:

    - C1540 does not work
    - some copy protections fail
    - more accurate timing
    - power/activity LEDs

*/

/*

    1540/1541/1541A/SX-64 Parts

    Location       Part Number    Description
                                  2016 2K x 8 bit Static RAM (short board)
    UA2-UB3                       2114 (4) 1K x 4 bit Static RAM (long board)
                   325572-01      64H105 40 pin Gate Array (short board)
                   325302-01      2364-130 ROM DOS 2.6 C000-DFFF
                   325303-01      2364-131 ROM DOS 2.6 (1540) E000-FFFF
                   901229-01      2364-173 ROM DOS 2.6 rev. 0 E000-FFFF
                   901229-03      2364-197 ROM DOS 2.6 rev. 1 E000-FFFF
                   901229-05      8K ROM DOS 2.6 rev. 2 E000-FFFF
                                  6502 CPU
                                  6522 (2) VIA
    drive                         Alps DFB111M25A
    drive                         Alps FDM2111-B2
    drive                         Newtronics D500

    1541B/1541C Parts

    Location       Part Number    Description
    UA3                           2016 2K x 8 bit Static RAM
    UC2                           6502 CPU
    UC1, UC3                      6522 (2) CIA
    UC4            251828-02      64H156 42 pin Gate Array
    UC5            251829-01      64H157 20 pin Gate Array
    UD1          * 251853-01      R/W Hybrid
    UD1          * 251853-02      R/W Hybrid
    UA2            251968-01      27128 EPROM DOS 2.6 rev. 3 C000-FFFF
    drive                         Newtronics D500
      * Not interchangeable.

    1541-II Parts

    Location       Part Number    Description
    U5                            2016-15 2K x 8 bit Static RAM
    U12                           SONY CX20185 R/W Amp.
    U3                            6502A CPU
    U6, U8                        6522 (2) CIA
    U10            251828-01      64H156 40 pin Gate Array
    U4             251968-03      16K ROM DOS 2.6 rev. 4 C000-FFFF
    drive                         Chinon FZ-501M REV A
    drive                         Digital System DS-50F
    drive                         Newtronics D500
    drive                         Safronic DS-50F

    ...

    PCB Assy # 1540008-01
    Schematic # 1540001
    Original "Long" Board
    Has 4 discreet 2114 RAMs
    ALPS Drive only

    PCB Assy # 1540048
    Schematic # 1540049
    Referred to as the CR board
    Changed to 2048 x 8 bit RAM pkg.
    A 40 pin Gate Array is used
    Alps Drive (-01)
    Newtronics Drive (-03)

    PCB Assy # 250442-01
    Schematic # 251748
    Termed the 1541 A
    Just one jumper change to accommodate both types of drive

    PCB Assy # 250446-01
    Schematic # 251748 (See Notes)
    Termed the 1541 A-2
    Just one jumper change to accommodate both types of drive

    ...

    VIC1541 1540001-01   Very early version, long board.
            1540001-03   As above, only the ROMs are different.
            1540008-01

    1541    1540048-01   Shorter board with a 40 pin gate array, Alps mech.
            1540048-03   As above, but Newtronics mech.
            1540049      Similar to above
            1540050      Similar to above, Alps mechanism.

    SX64    250410-01    Design most similar to 1540048-01, Alps mechanism.

    1541    251777       Function of bridge rects. reversed, Newtronics mech.
            251830       Same as above

    1541A   250442-01    Alps or Newtronics drive selected by a jumper.
    1541A2  250446-01    A 74LS123 replaces the 9602 at UD4.
    1541B   250448-01    Same as the 1541C, but in a case like the 1541.
    1541C   250448-01    Short board, new 40/42 pin gate array, 20 pin gate
                         array and a R/W hybrid chip replace many components.
                         Uses a Newtronics drive with optical trk 0 sensor.
    1541C   251854       As above, single DOS ROM IC, trk 0 sensor, 30 pin
                         IC for R/W ampl & stepper motor control (like 1541).

    1541-II              A complete redesign using the 40 pin gate array
                         from the 1451C and a Sony R/W hybrid, but not the
                         20 pin gate array, single DOS ROM IC.

    NOTE: These system boards are the 60 Hz versions.
          The -02 and -04 boards are probably the 50 Hz versions.

    The ROMs appear to be completely interchangeable. For instance, the
    first version of ROM for the 1541-II contained the same code as the
    last version of the 1541. I copy the last version of the 1541-II ROM
    into two 68764 EPROMs and use them in my original 1541 (long board).
    Not only do they work, but they work better than the originals.


    http://www.amiga-stuff.com/hardware/cbmboards.html

*/

#include "c1541.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define M6502_TAG		"ucd5"
#define M6522_0_TAG		"uab1"
#define M6522_1_TAG		"ucd4"
#define C64H156_TAG		"64h156"


enum
{
	LED_POWER = 0,
	LED_ACT
};



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type C1540 = &device_creator<c1540_device>;
const device_type C1541 = &device_creator<c1541_device>;
const device_type C1541C = &device_creator<c1541c_device>;
const device_type C1541II = &device_creator<c1541ii_device>;
const device_type SX1541 = &device_creator<sx1541_device>;
const device_type OC118 = &device_creator<oc118_device>;


c1540_device::c1540_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	:c1541_device(mconfig, C1540, "C1540", tag, owner, clock) { m_variant = TYPE_1540; }


c1541c_device::c1541c_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	:c1541_device(mconfig, C1541C, "C1541C", tag, owner, clock) { m_variant = TYPE_1541C; }


c1541ii_device::c1541ii_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	:c1541_device(mconfig, C1541II, "C1541II", tag, owner, clock) { m_variant = TYPE_1541II; }


sx1541_device::sx1541_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	:c1541_device(mconfig, SX1541, "SX1541", tag, owner, clock) { m_variant = TYPE_SX1541; }


oc118_device::oc118_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	:c1541_device(mconfig, OC118, "OC118", tag, owner, clock) { m_variant = TYPE_OC118; }


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void c1541_device::device_config_complete()
{
	switch (m_variant)
	{
	case TYPE_1540:
		m_shortname = "c1540";
		break;

	default:
	case TYPE_1541:
		m_shortname = "c1541";
		break;

	case TYPE_1541C:
		m_shortname = "c1541c";
		break;

	case TYPE_1541II:
		m_shortname = "c1541ii";
		break;

	case TYPE_SX1541:
		m_shortname = "sx1541";
		break;

	case TYPE_OC118:
		m_shortname = "oc118";
		break;
	}
}


//-------------------------------------------------
//  ROM( c1540 )
//-------------------------------------------------

ROM_START( c1540 )
	ROM_REGION( 0x4000, M6502_TAG, 0 )
	ROM_LOAD( "325302-01.uab4", 0x0000, 0x2000, CRC(29ae9752) SHA1(8e0547430135ba462525c224e76356bd3d430f11) )
	ROM_LOAD( "325303-01.uab5", 0x2000, 0x2000, CRC(10b39158) SHA1(56dfe79b26f50af4e83fd9604857756d196516b9) )
ROM_END


//-------------------------------------------------
//  ROM( c1541 )
//-------------------------------------------------

ROM_START( c1541 )
	ROM_REGION( 0x6000, M6502_TAG, 0 )
	ROM_LOAD( "325302-01.uab4", 0x0000, 0x2000, CRC(29ae9752) SHA1(8e0547430135ba462525c224e76356bd3d430f11) )
	ROM_LOAD_OPTIONAL( "901229-01.uab5", 0x2000, 0x2000, CRC(9a48d3f0) SHA1(7a1054c6156b51c25410caec0f609efb079d3a77) )
	ROM_LOAD_OPTIONAL( "901229-02.uab5", 0x2000, 0x2000, CRC(b29bab75) SHA1(91321142e226168b1139c30c83896933f317d000) )
	ROM_LOAD_OPTIONAL( "901229-03.uab5", 0x2000, 0x2000, CRC(9126e74a) SHA1(03d17bd745066f1ead801c5183ac1d3af7809744) )
	ROM_LOAD_OPTIONAL( "901229-04.uab5", 0x2000, 0x2000, NO_DUMP )
	ROM_LOAD_OPTIONAL( "901229-05 ae.uab5", 0x2000, 0x2000, CRC(361c9f37) SHA1(f5d60777440829e46dc91285e662ba072acd2d8b) )
	ROM_LOAD( "901229-06 aa.uab5", 0x2000, 0x2000, CRC(3a235039) SHA1(c7f94f4f51d6de4cdc21ecbb7e57bb209f0530c0) )
	ROM_LOAD_OPTIONAL( "jiffydos 1541.uab5", 0x4000, 0x2000, CRC(bc7e4aeb) SHA1(db6cfaa6d9b78d58746c811de29f3b1f44d99ddf) )
ROM_END


//-------------------------------------------------
//  ROM( c1541c )
//-------------------------------------------------

ROM_START( c1541c )
	ROM_REGION( 0x4000, M6502_TAG, 0 )
	ROM_LOAD( "251968-01.ua2", 0x0000, 0x4000, CRC(1b3ca08d) SHA1(8e893932de8cce244117fcea4c46b7c39c6a7765) )
	ROM_LOAD( "251968-02.ua2", 0x0000, 0x4000, CRC(2d862d20) SHA1(38a7a489c7bbc8661cf63476bf1eb07b38b1c704) )
ROM_END


//-------------------------------------------------
//  ROM( c1541ii )
//-------------------------------------------------

ROM_START( c1541ii )
	ROM_REGION( 0x8000, M6502_TAG, 0 )
	ROM_LOAD( "251968-03.u4", 0x0000, 0x4000, CRC(899fa3c5) SHA1(d3b78c3dbac55f5199f33f3fe0036439811f7fb3) )
	ROM_LOAD( "355640-01.u4", 0x0000, 0x4000, CRC(57224cde) SHA1(ab16f56989b27d89babe5f89c5a8cb3da71a82f0) )
	ROM_LOAD_OPTIONAL( "jiffydos 1541-ii.u4", 0x4000, 0x4000, CRC(dd409902) SHA1(b1a5b826304d3df2a27d7163c6a81a532e040d32) )
ROM_END


//-------------------------------------------------
//  ROM( sx1541 )
//-------------------------------------------------

ROM_START( sx1541 )
	ROM_REGION( 0xa000, M6502_TAG, 0 )
	ROM_LOAD( "325302-01.uab4",    0x0000, 0x2000, CRC(29ae9752) SHA1(8e0547430135ba462525c224e76356bd3d430f11) )
	ROM_LOAD( "901229-05 ae.uab5", 0x2000, 0x2000, CRC(361c9f37) SHA1(f5d60777440829e46dc91285e662ba072acd2d8b) )
	ROM_LOAD_OPTIONAL( "jiffydos sx1541",   0x4000, 0x4000, CRC(783575f6) SHA1(36ccb9ff60328c4460b68522443ecdb7f002a234) )
	ROM_LOAD_OPTIONAL( "1541 flash.uab5",   0x8000, 0x2000, CRC(22f7757e) SHA1(86a1e43d3d22b35677064cca400a6bd06767a3dc) )
ROM_END


//-------------------------------------------------
//  ROM( oc118 )
//-------------------------------------------------

ROM_START( oc118 )
	ROM_REGION( 0x4000, M6502_TAG, 0 )
	ROM_LOAD_OPTIONAL( "oc118a.u3", 0x0000, 0x4000, CRC(edf18265) SHA1(47a7c4bdcc20ecc5e59d694b151f493229becaea) ) // scrambled address/data lines?
	ROM_LOAD_OPTIONAL( "oc118b.u3", 0x0000, 0x4000, CRC(b39e4600) SHA1(991132fcc6e70e9a428062ae76055a150f2f7ac6) ) // scrambled address/data lines?
	ROM_LOAD( "jiffydos v5.0.u3", 0x0000, 0x4000, CRC(46c3302c) SHA1(e3623658cb7af30c9d3bce2ba3b6ad5ee89ac1b8) )
ROM_END


//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *c1541_device::device_rom_region() const
{
	switch (m_variant)
	{
	case TYPE_1540:
		return ROM_NAME( c1540 );

	default:
	case TYPE_1541:
		return ROM_NAME( c1541 );

	case TYPE_1541C:
		return ROM_NAME( c1541c );

	case TYPE_1541II:
		return ROM_NAME( c1541ii );

	case TYPE_SX1541:
		return ROM_NAME( sx1541 );

	case TYPE_OC118:
		return ROM_NAME( oc118 );
	}
}


//-------------------------------------------------
//  ADDRESS_MAP( c1541_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( c1541_mem, AS_PROGRAM, 8, c1541_device )
	AM_RANGE(0x0000, 0x07ff) AM_MIRROR(0x6000) AM_RAM
	AM_RANGE(0x1800, 0x180f) AM_MIRROR(0x63f0) AM_DEVREADWRITE(M6522_0_TAG, via6522_device, read, write)
	AM_RANGE(0x1c00, 0x1c0f) AM_MIRROR(0x63f0) AM_DEVREADWRITE(M6522_1_TAG, via6522_device, read, write)
	AM_RANGE(0x8000, 0xbfff) AM_MIRROR(0x4000) AM_ROM AM_REGION(M6502_TAG, 0)
ADDRESS_MAP_END


//-------------------------------------------------
//  via6522_interface via0_intf
//-------------------------------------------------

WRITE_LINE_MEMBER( c1541_device::via0_irq_w )
{
	m_via0_irq = state;

	m_maincpu->set_input_line(INPUT_LINE_IRQ0, (m_via0_irq | m_via1_irq) ? ASSERT_LINE : CLEAR_LINE);
}


READ8_MEMBER( c1541_device::via0_pa_r )
{
	// dummy read to acknowledge ATN IN interrupt
	return 0;
}


READ8_MEMBER( c1541_device::via0_pb_r )
{
	/*

        bit     description

        PB0     DATA IN
        PB1
        PB2     CLK IN
        PB3
        PB4
        PB5     J1
        PB6     J2
        PB7     ATN IN

    */

	UINT8 data = 0;

	// data in
	data = !m_bus->data_r();

	// clock in
	data |= !m_bus->clk_r() << 2;

	// serial bus address
	data |= (m_address - 8) << 5;

	// attention in
	data |= !m_bus->atn_r() << 7;

	return data;
}


WRITE8_MEMBER( c1541_device::via0_pb_w )
{
	/*

        bit     description

        PB0
        PB1     DATA OUT
        PB2
        PB3     CLK OUT
        PB4     ATNA
        PB5
        PB6
        PB7

    */

	// data out
	m_data_out = BIT(data, 1);

	// attention acknowledge
	m_ga->atna_w(BIT(data, 4));

	// clock out
	m_bus->clk_w(this, !BIT(data, 3));
}


READ_LINE_MEMBER( c1541_device::atn_in_r )
{
	return !m_bus->atn_r();
}


static const via6522_interface c1541_via0_intf =
{
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, c1541_device, via0_pa_r),
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, c1541_device, via0_pb_r),
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, c1541_device, atn_in_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_NULL,
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, c1541_device, via0_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, c1541_device, via0_irq_w)
};


//-------------------------------------------------
//  via6522_interface c1541c_via0_intf
//-------------------------------------------------

READ8_MEMBER( c1541c_device::via0_pa_r )
{
	/*

        bit     description

        PA0     TR00 SENCE
        PA1
        PA2
        PA3
        PA4
        PA5
        PA6
        PA7

    */

	return !floppy_tk00_r(m_image);
}


static const via6522_interface c1541c_via0_intf =
{
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, c1541c_device, via0_pa_r),
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, c1541_device, via0_pb_r),
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, c1541_device, atn_in_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_NULL,
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, c1541_device, via0_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, c1541_device, via0_irq_w)
};


//-------------------------------------------------
//  via6522_interface via1_intf
//-------------------------------------------------

WRITE_LINE_MEMBER( c1541_device::via1_irq_w )
{
	m_via1_irq = state;

	m_maincpu->set_input_line(INPUT_LINE_IRQ0, (m_via0_irq | m_via1_irq) ? ASSERT_LINE : CLEAR_LINE);
}


READ8_MEMBER( c1541_device::via1_pb_r )
{
	/*

        bit     signal      description

        PB0
        PB1
        PB2
        PB3
        PB4     WPS         write protect sense
        PB5
        PB6
        PB7     SYNC        SYNC detect line

    */

	UINT8 data = 0;

	// write protect sense
	data |= !floppy_wpt_r(m_image) << 4;

	// SYNC detect line
	data |= m_ga->sync_r() << 7;

	return data;
}


WRITE8_MEMBER( c1541_device::via1_pb_w )
{
	/*

        bit     signal      description

        PB0     STP0        stepping motor bit 0
        PB1     STP1        stepping motor bit 1
        PB2     MTR         motor ON/OFF
        PB3     ACT         drive 0 LED
        PB4
        PB5     DS0         density select 0
        PB6     DS1         density select 1
        PB7     SYNC        SYNC detect line

    */

	// spindle motor
	m_ga->mtr_w(BIT(data, 2));

	// stepper motor
	m_ga->stp_w(data & 0x03);

	// activity LED
	output_set_led_value(LED_ACT, BIT(data, 3));

	// density select
	m_ga->ds_w((data >> 5) & 0x03);
}


static const via6522_interface c1541_via1_intf =
{
	DEVCB_DEVICE_MEMBER(C64H156_TAG, c64h156_device, yb_r),
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, c1541_device, via1_pb_r),
	DEVCB_DEVICE_LINE_MEMBER(C64H156_TAG, c64h156_device, byte_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_DEVICE_MEMBER(C64H156_TAG, c64h156_device, yb_w),
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, c1541_device, via1_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DEVICE_LINE_MEMBER(C64H156_TAG, c64h156_device, soe_w),
	DEVCB_DEVICE_LINE_MEMBER(C64H156_TAG, c64h156_device, oe_w),

	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, c1541_device, via1_irq_w)
};


//-------------------------------------------------
//  C64H156_INTERFACE( ga_intf )
//-------------------------------------------------

WRITE_LINE_MEMBER( c1541_device::atn_w )
{
	set_iec_data();
}


WRITE_LINE_MEMBER( c1541_device::byte_w )
{
	m_maincpu->set_input_line(M6502_SET_OVERFLOW, state);
	m_via1->write_ca1(state);
}


static C64H156_INTERFACE( ga_intf )
{
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, c1541_device, atn_w),
	DEVCB_NULL,
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, c1541_device, byte_w)
};



//-------------------------------------------------
//  LEGACY_FLOPPY_OPTIONS( c1541 )
//-------------------------------------------------

static LEGACY_FLOPPY_OPTIONS_START( c1541 )
	LEGACY_FLOPPY_OPTION( c1541, "g64", "Commodore 1541 GCR Disk Image", g64_dsk_identify, g64_dsk_construct, NULL, NULL )
	LEGACY_FLOPPY_OPTION( c1541, "d64", "Commodore 1541 Disk Image", d64_dsk_identify, d64_dsk_construct, NULL, NULL )
LEGACY_FLOPPY_OPTIONS_END


//-------------------------------------------------
//  floppy_interface c1541_floppy_interface
//-------------------------------------------------

static const floppy_interface c1541_floppy_interface =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSDD,
	LEGACY_FLOPPY_OPTIONS_NAME(c1541),
	"floppy_5_25",
	NULL
};


//-------------------------------------------------
//  MACHINE_DRIVER( c1541 )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( c1541 )
	MCFG_CPU_ADD(M6502_TAG, M6502, XTAL_16MHz/16)
	MCFG_CPU_PROGRAM_MAP(c1541_mem)

	MCFG_VIA6522_ADD(M6522_0_TAG, XTAL_16MHz/16, c1541_via0_intf)
	MCFG_VIA6522_ADD(M6522_1_TAG, XTAL_16MHz/16, c1541_via1_intf)

	MCFG_LEGACY_FLOPPY_DRIVE_ADD(FLOPPY_0, c1541_floppy_interface)
	MCFG_64H156_ADD(C64H156_TAG, XTAL_16MHz, ga_intf)
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_DRIVER( c1541c )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( c1541c )
	MCFG_CPU_ADD(M6502_TAG, M6502, XTAL_16MHz/16)
	MCFG_CPU_PROGRAM_MAP(c1541_mem)

	MCFG_VIA6522_ADD(M6522_0_TAG, XTAL_16MHz/16, c1541c_via0_intf)
	MCFG_VIA6522_ADD(M6522_1_TAG, XTAL_16MHz/16, c1541_via1_intf)

	MCFG_LEGACY_FLOPPY_DRIVE_ADD(FLOPPY_0, c1541_floppy_interface)
	MCFG_64H156_ADD(C64H156_TAG, XTAL_16MHz, ga_intf)
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor c1541_device::device_mconfig_additions() const
{
	switch (m_variant)
	{
	case TYPE_1541C:
		return MACHINE_CONFIG_NAME( c1541c );

	default:
		return MACHINE_CONFIG_NAME( c1541 );
	}
}



//**************************************************************************
//  INLINE HELPERS
//**************************************************************************

//-------------------------------------------------
//  set_iec_data -
//-------------------------------------------------

inline void c1541_device::set_iec_data()
{
	int data = !m_data_out & !m_ga->atn_r();

	m_bus->data_w(this, data);
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  c1541_device - constructor
//-------------------------------------------------

c1541_device::c1541_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, C1541, "C1541", tag, owner, clock),
	  device_cbm_iec_interface(mconfig, *this),
	  m_maincpu(*this, M6502_TAG),
	  m_via0(*this, M6522_0_TAG),
	  m_via1(*this, M6522_1_TAG),
	  m_ga(*this, C64H156_TAG),
	  m_image(*this, FLOPPY_0),
	  m_data_out(1),
	  m_via0_irq(0),
	  m_via1_irq(0)
{
	m_variant = TYPE_1541;
}


c1541_device::c1541_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, type, name, tag, owner, clock),
	  device_cbm_iec_interface(mconfig, *this),
	  m_maincpu(*this, M6502_TAG),
	  m_via0(*this, M6522_0_TAG),
	  m_via1(*this, M6522_1_TAG),
	  m_ga(*this, C64H156_TAG),
	  m_image(*this, FLOPPY_0),
	  m_data_out(1),
	  m_via0_irq(0),
	  m_via1_irq(0)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void c1541_device::device_start()
{
	// install image callbacks
	floppy_install_unload_proc(m_image, c1541_device::on_disk_change);
	floppy_install_load_proc(m_image, c1541_device::on_disk_change);

	// register for state saving
	save_item(NAME(m_data_out));
	save_item(NAME(m_via0_irq));
	save_item(NAME(m_via1_irq));
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void c1541_device::device_reset()
{
}


//-------------------------------------------------
//  iec_atn_w -
//-------------------------------------------------

void c1541_device::cbm_iec_atn(int state)
{
	m_via0->write_ca1(!state);
	m_ga->atni_w(!state);

	set_iec_data();
}


//-------------------------------------------------
//  iec_reset_w -
//-------------------------------------------------

void c1541_device::cbm_iec_reset(int state)
{
	if (!state)
	{
		device_reset();
	}
}


//-------------------------------------------------
//  on_disk_change -
//-------------------------------------------------

void c1541_device::on_disk_change(device_image_interface &image)
{
    c1541_device *c1541 = static_cast<c1541_device *>(image.device().owner());

	c1541->m_ga->on_disk_changed();
}
