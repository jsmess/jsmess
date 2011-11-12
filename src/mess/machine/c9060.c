/**********************************************************************

    Commodore 9060/9090 Hard Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "c9060.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define M6502_TAG		"7e"
#define M6532_0_TAG		"7f"
#define M6532_1_TAG		"7g"

#define M6504_TAG		"4a"
#define M6522_TAG		"4b"

#define SASIBUS_TAG		"sasi"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type C9060 = &device_creator<c9060_device>;
const device_type C9090 = &device_creator<c9090_device>;


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void base_c9060_device::device_config_complete()
{
	switch (m_variant)
	{
	default:
	case TYPE_9060:
		m_shortname = "c9060";
		break;

	case TYPE_9090:
		m_shortname = "c9090";
		break;
	}
}


//-------------------------------------------------
//  ROM( c9060 )
//-------------------------------------------------

ROM_START( c9060 )
	ROM_REGION( 0x4000, M6502_TAG, 0 )
	ROM_LOAD_OPTIONAL( "300516-revb.7c", 0x0000, 0x2000, CRC(2d758a14) SHA1(c959cc9dde84fc3d64e95e58a0a096a26d8107fd) )
	ROM_LOAD( "300516-revc.7c", 0x0000, 0x2000, CRC(d6a3e88f) SHA1(bb1ddb5da94a86266012eca54818aa21dc4cef6a) )
	ROM_LOAD_OPTIONAL( "300517-reva.7d", 0x2000, 0x2000, CRC(566df630) SHA1(b1602dfff408b165ee52a6a4ca3e2ec27e689ba9) )
	ROM_LOAD_OPTIONAL( "300517-revb.7d", 0x2000, 0x2000, CRC(f0382bc3) SHA1(0b0a8dc520f5b41ffa832e4a636b3d226ccbb7f1) )
	ROM_LOAD( "300517-revc.7d", 0x2000, 0x2000, CRC(2a9ad4ad) SHA1(4c17d014de48c906871b9b6c7d037d8736b1fd52) )

	ROM_REGION( 0x800, M6504_TAG, 0 )
	ROM_LOAD_OPTIONAL( "300515-reva.4c", 0x000, 0x800, CRC(99e096f7) SHA1(a3d1deb27bf5918b62b89c27fa3e488eb8f717a4) )
	ROM_LOAD( "300515-revb.4c", 0x000, 0x800, CRC(49adf4fb) SHA1(59dafbd4855083074ba8dc96a04d4daa5b76e0d6) )
ROM_END


//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *base_c9060_device::device_rom_region() const
{
	return ROM_NAME( c9060 );
}


//-------------------------------------------------
//  ADDRESS_MAP( c9060_main_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( c9060_main_mem, AS_PROGRAM, 8, base_c9060_device )
	AM_RANGE(0x0000, 0x007f) AM_MIRROR(0x0100) AM_RAM // 6532 #1
	AM_RANGE(0x0080, 0x00ff) AM_MIRROR(0x0100) AM_RAM // 6532 #2
	AM_RANGE(0x0200, 0x021f) AM_MIRROR(0x0d60) AM_DEVREADWRITE_LEGACY(M6532_0_TAG, riot6532_r, riot6532_w)
	AM_RANGE(0x0280, 0x029f) AM_MIRROR(0x0d60) AM_DEVREADWRITE_LEGACY(M6532_1_TAG, riot6532_r, riot6532_w)
	AM_RANGE(0x1000, 0x13ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share1")
	AM_RANGE(0x2000, 0x23ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share2")
	AM_RANGE(0x3000, 0x33ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share3")
	AM_RANGE(0x4000, 0x43ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share4")
	AM_RANGE(0xc000, 0xffff) AM_ROM AM_REGION(M6502_TAG, 0)
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( c9060_hdc_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( c9060_hdc_mem, AS_PROGRAM, 8, base_c9060_device )
	ADDRESS_MAP_GLOBAL_MASK(0x1fff)
	AM_RANGE(0x0000, 0x003f) AM_MIRROR(0x0300) AM_RAM // 6530
	AM_RANGE(0x0040, 0x004f) AM_MIRROR(0x0330) AM_DEVREADWRITE(M6522_TAG, via6522_device, read, write)
	AM_RANGE(0x0400, 0x07ff) AM_RAM AM_SHARE("share1")
	AM_RANGE(0x0800, 0x0bff) AM_RAM AM_SHARE("share2")
	AM_RANGE(0x0c00, 0x0fff) AM_RAM AM_SHARE("share3")
	AM_RANGE(0x1000, 0x13ff) AM_RAM AM_SHARE("share4")
	AM_RANGE(0x1800, 0x1fff) AM_ROM AM_REGION(M6504_TAG, 0)
ADDRESS_MAP_END


//-------------------------------------------------
//  SCSIBus_interface sasi_intf
//-------------------------------------------------

static const SCSIConfigTable sasi_dev_table =
{
	1,
	{
		{ SCSI_ID_0, "harddisk0", SCSI_DEVICE_HARDDISK }
	}
};

static const SCSIBus_interface sasi_intf =
{
    &sasi_dev_table,
    NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


//-------------------------------------------------
//  riot6532_interface riot0_intf
//-------------------------------------------------

READ8_MEMBER( base_c9060_device::dio_r )
{
	/*

        bit     description

        PA0     DI0
        PA1     DI1
        PA2     DI2
        PA3     DI3
        PA4     DI4
        PA5     DI5
        PA6     DI6
        PA7     DI7

    */

	return m_bus->dio_r();
}


WRITE8_MEMBER( base_c9060_device::dio_w )
{
	/*

        bit     description

        PB0     DO0
        PB1     DO1
        PB2     DO2
        PB3     DO3
        PB4     DO4
        PB5     DO5
        PB6     DO6
        PB7     DO7

    */

	m_bus->dio_w(this, data);
}


static const riot6532_interface riot0_intf =
{
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, base_c9060_device, dio_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, base_c9060_device, dio_w),
	DEVCB_NULL
};


//-------------------------------------------------
//  riot6532_interface riot1_intf
//-------------------------------------------------

READ8_MEMBER( base_c9060_device::riot1_pa_r )
{
	/*

        bit     description

        PA0     ATNA
        PA1     DACO
        PA2     RFDO
        PA3     EOIO
        PA4     DAVO
        PA5     EOII
        PA6     DAVI
        PA7     _ATN

    */

	UINT8 data = 0;

	/* end or identify in */
	data |= m_bus->eoi_r() << 5;

	/* data valid in */
	data |= m_bus->dav_r() << 6;

	/* attention */
	data |= !m_bus->atn_r() << 7;

	return data;
}


WRITE8_MEMBER( base_c9060_device::riot1_pa_w )
{
	/*

        bit     description

        PA0     ATNA
        PA1     DACO
        PA2     RFDO
        PA3     EOIO
        PA4     DAVO
        PA5     EOII
        PA6     DAVI
        PA7     _ATN

    */

	/* attention acknowledge */
	m_atna = BIT(data, 0);

	/* data accepted out */
	m_daco = BIT(data, 1);

	/* not ready for data out */
	m_rfdo = BIT(data, 2);

	/* end or identify out */
	m_bus->eoi_w(this, BIT(data, 3));

	/* data valid out */
	m_bus->dav_w(this, BIT(data, 4));

	update_ieee_signals();
}


READ8_MEMBER( base_c9060_device::riot1_pb_r )
{
	/*

        bit     description

        PB0
        PB1
        PB2
        PB3
        PB4     DRIVE RDY
        PB5     PWR ON AND NO ERRORS
        PB6     DACI
        PB7     RFDI

    */

	UINT8 data = 0;

	/* data accepted in */
	data |= m_bus->ndac_r() << 6;

	/* ready for data in */
	data |= m_bus->nrfd_r() << 7;

	return data;
}


WRITE8_MEMBER( base_c9060_device::riot1_pb_w )
{
	/*

        bit     description

        PB0
        PB1
        PB2
        PB3
        PB4     DRIVE RDY
        PB5     PWR ON AND NO ERRORS
        PB6     DACI
        PB7     RFDI

    */
}


static const riot6532_interface riot1_intf =
{
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, base_c9060_device, riot1_pa_r),
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, base_c9060_device, riot1_pb_r),
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, base_c9060_device, riot1_pa_w),
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, base_c9060_device, riot1_pb_w),
	DEVCB_CPU_INPUT_LINE(M6502_TAG, INPUT_LINE_IRQ0)
};


//-------------------------------------------------
//  via6522_interface via_intf
//-------------------------------------------------

READ8_MEMBER( base_c9060_device::via_pa_r )
{
	/*

        bit     description

        PA0     DB0
        PA1     DB1
        PA2     DB2
        PA3     DB3
        PA4     DB4
        PA5     DB5
        PA6     DB6
        PA7     DB7

    */

	return 0;
}

WRITE8_MEMBER( base_c9060_device::via_pa_w )
{
	/*

        bit     description

        PA0     DB0
        PA1     DB1
        PA2     DB2
        PA3     DB3
        PA4     DB4
        PA5     DB5
        PA6     DB6
        PA7     DB7

    */
}

READ8_MEMBER( base_c9060_device::via_pb_r )
{
	/*

        bit     description

        PB0     
        PB1     
        PB2     C/D
        PB3     BUSY
        PB4     J14
        PB5     J14
        PB6     I/O
        PB7     MSG

    */

	UINT8 data = 0;
	
	data |= scsi_cd_r(m_sasibus) << 2;
	data |= scsi_bsy_r(m_sasibus) << 3;
	data |= scsi_io_r(m_sasibus) << 6;
	data |= scsi_msg_r(m_sasibus) << 7;
	
	return data;
}

WRITE8_MEMBER( base_c9060_device::via_pb_w )
{
	/*

        bit     description

        PB0     SEL
        PB1     RST
        PB2     
        PB3     
        PB4     
        PB5     
        PB6     
        PB7     

    */
	
	scsi_sel_w(m_sasibus, BIT(data, 0));
	scsi_rst_w(m_sasibus, BIT(data, 1));
}

static const via6522_interface via_intf =
{
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, base_c9060_device, via_pa_r),
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, base_c9060_device, via_pb_r),
	DEVCB_NULL, // ACK
	DEVCB_NULL,
	DEVCB_NULL, // MSG
	DEVCB_NULL, // ?

	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, base_c9060_device, via_pa_w),
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, base_c9060_device, via_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_NULL
};


//-------------------------------------------------
//  MACHINE_CONFIG_FRAGMENT( c9060 )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( c9060 )
	// DOS
	MCFG_CPU_ADD(M6502_TAG, M6502, XTAL_16MHz/16)
	MCFG_CPU_PROGRAM_MAP(c9060_main_mem)

	MCFG_RIOT6532_ADD(M6532_0_TAG, XTAL_16MHz/16, riot0_intf)
	MCFG_RIOT6532_ADD(M6532_1_TAG, XTAL_16MHz/16, riot1_intf)

	// controller
	MCFG_CPU_ADD(M6504_TAG, M6504, XTAL_16MHz/16)
	MCFG_CPU_PROGRAM_MAP(c9060_hdc_mem)

	MCFG_VIA6522_ADD(M6522_TAG, XTAL_16MHz/16, via_intf)

	MCFG_SCSIBUS_ADD(SASIBUS_TAG, sasi_intf)
	MCFG_HARDDISK_ADD("harddisk0")
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor base_c9060_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( c9060 );
}



//**************************************************************************
//  INLINE HELPERS
//**************************************************************************

//-------------------------------------------------
//  update_ieee_signals -
//-------------------------------------------------

inline void base_c9060_device::update_ieee_signals()
{
	int atn = m_bus->atn_r();
	int nrfd = !(!(!(atn & m_atna) & m_rfdo) | !(atn | m_atna));
	int ndac = !(m_daco | !(atn | m_atna));

	m_bus->nrfd_w(this, nrfd);
	m_bus->ndac_w(this, ndac);
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  base_c9060_device - constructor
//-------------------------------------------------

base_c9060_device::base_c9060_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock, UINT32 variant)
    : device_t(mconfig, type, name, tag, owner, clock),
	  device_ieee488_interface(mconfig, *this),
	  m_maincpu(*this, M6502_TAG),
	  m_hdccpu(*this, M6504_TAG),
	  m_riot0(*this, M6532_0_TAG),
	  m_riot1(*this, M6532_1_TAG),
	  m_via(*this, M6522_TAG),
	  m_sasibus(*this, SASIBUS_TAG),
	  m_variant(variant)
{
}


//-------------------------------------------------
//  c9060_device - constructor
//-------------------------------------------------

c9060_device::c9060_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: base_c9060_device(mconfig, C9060, "C9060", tag, owner, clock, TYPE_9060) { }


//-------------------------------------------------
//  c9090_device - constructor
//-------------------------------------------------

c9090_device::c9090_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: base_c9060_device(mconfig, C9090, "C9090", tag, owner, clock, TYPE_9090) { }


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void base_c9060_device::device_start()
{
	// state saving
	save_item(NAME(m_rfdo));
	save_item(NAME(m_daco));
	save_item(NAME(m_atna));
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void base_c9060_device::device_reset()
{
	m_maincpu->set_input_line(M6502_SET_OVERFLOW, ASSERT_LINE);
	m_maincpu->set_input_line(M6502_SET_OVERFLOW, CLEAR_LINE);
}


//-------------------------------------------------
//  device_timer - handler timer events
//-------------------------------------------------

void base_c9060_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
}


//-------------------------------------------------
//  m_bus->atn -
//-------------------------------------------------

void base_c9060_device::ieee488_atn(int state)
{
	update_ieee_signals();

	// set RIOT PA7
	riot6532_porta_in_set(m_riot1, !state << 7, 0x80);
}


//-------------------------------------------------
//  m_bus->ifc -
//-------------------------------------------------

void base_c9060_device::ieee488_ifc(int state)
{
	if (!state)
	{
		device_reset();
	}
}
