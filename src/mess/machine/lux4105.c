#include "lux4105.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define SASIBUS_TAG		"sasi"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type LUXOR_4105 = &device_creator<luxor_4105_device>;


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

static void sasi_linechange(device_t *device, UINT8 line, UINT8 state)
{
}

static const SCSIBus_interface sasi_intf =
{
    &sasi_dev_table,
    &sasi_linechange
};


//-------------------------------------------------
//  MACHINE_DRIVER( luxor_4105 )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( luxor_4105 )
    MCFG_SCSIBUS_ADD(SASIBUS_TAG, sasi_intf)
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor luxor_4105_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( luxor_4105 );
}


//-------------------------------------------------
//  INPUT_PORTS( luxor_4105 )
//-------------------------------------------------

INPUT_PORTS_START( luxor_4105 )
	PORT_START("DSW")
	// TODO
INPUT_PORTS_END


//-------------------------------------------------
//  input_ports - device-specific input ports
//-------------------------------------------------

ioport_constructor luxor_4105_device::device_input_ports() const
{
	return INPUT_PORTS_NAME( luxor_4105 );
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  luxor_4105_device - constructor
//-------------------------------------------------

luxor_4105_device::luxor_4105_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, LUXOR_4105, "Luxor 4105", tag, owner, clock),
	  device_abc1600bus_card_interface(mconfig, *this),
	  m_sasibus(*this, SASIBUS_TAG)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void luxor_4105_device::device_start()
{
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void luxor_4105_device::device_reset()
{
	init_scsibus(m_sasibus);
	
	m_cs = 0;
	m_data = 0;
	m_dma = 0;
	
	set_scsi_line(m_sasibus, SCSI_LINE_RESET, 0);
	set_scsi_line(m_sasibus, SCSI_LINE_RESET, 1);
}



//**************************************************************************
//  ABC 1600 BUS INTERFACE
//**************************************************************************

//-------------------------------------------------
//  abc1600bus_cs -
//-------------------------------------------------

void luxor_4105_device::abc1600bus_cs(UINT8 data)
{
	m_cs = (data == ADDRESS);
}


//-------------------------------------------------
//  abc1600bus_csb -
//-------------------------------------------------

int luxor_4105_device::abc1600bus_csb()
{
	return !m_cs;
}


//-------------------------------------------------
//  abc1600bus_rst -
//-------------------------------------------------

void luxor_4105_device::abc1600bus_brst()
{
	device_reset();
}


//-------------------------------------------------
//  abc1600bus_stat -
//-------------------------------------------------

UINT8 luxor_4105_device::abc1600bus_stat()
{
	UINT8 data = 0xff;

	if (m_cs)
	{
		/*
			
			bit		description
			
			0		?
			1		BSY
			2		?
			3		?
			4		
			5		
			6		
			7		
			
		*/
		
		data = !get_scsi_line(m_sasibus, SCSI_LINE_BSY) << 1;
		
		/*
		data = !get_scsi_line(m_sasibus, SCSI_LINE_CD);
		data |= !get_scsi_line(m_sasibus, SCSI_LINE_IO) << 1;
		data |= !get_scsi_line(m_sasibus, SCSI_LINE_MSG) << 2;
		data |= !get_scsi_line(m_sasibus, SCSI_LINE_REQ) << 3;
		*/
	}
	
	return data;
}


//-------------------------------------------------
//  abc1600bus_inp -
//-------------------------------------------------

UINT8 luxor_4105_device::abc1600bus_inp()
{
	UINT8 data = 0xff;
	
	if (m_cs)
	{
		if (get_scsi_line(m_sasibus, SCSI_LINE_BSY))
		{
			input_port_read(this, "DSW");
		}
		else
		{
			data = scsi_data_r(m_sasibus);
		}
	}
	
	return data;
}


//-------------------------------------------------
//  abc1600bus_utp -
//-------------------------------------------------

void luxor_4105_device::abc1600bus_out(UINT8 data)
{
	if (m_cs)
	{
		scsi_data_w(m_sasibus, data);
	}
}


//-------------------------------------------------
//  abc1600bus_c1 -
//-------------------------------------------------

void luxor_4105_device::abc1600bus_c1(UINT8 data)
{
	if (m_cs)
	{
		set_scsi_line(m_sasibus, SCSI_LINE_SEL, BIT(data, 0));
	}
}


//-------------------------------------------------
//  abc1600bus_c3 -
//-------------------------------------------------

void luxor_4105_device::abc1600bus_c3(UINT8 data)
{
	if (m_cs)
	{
		m_data = 0;
		m_dma = 0;
		
		set_scsi_line(m_sasibus, SCSI_LINE_RESET, 0);
		set_scsi_line(m_sasibus, SCSI_LINE_RESET, 1);
	}
}


//-------------------------------------------------
//  abc1600bus_c4 -
//-------------------------------------------------

void luxor_4105_device::abc1600bus_c4(UINT8 data)
{
	if (m_cs)
	{
		/*
			
			bit		description
			
			0
			1
			2
			3
			4
			5		interrupts?
			6		DMA/CPU mode
			7		interrupts?
			
		*/

		m_dma = data;
	}
}
