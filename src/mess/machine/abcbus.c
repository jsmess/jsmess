/**********************************************************************

    Luxor ABC-bus emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "abcbus.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define LOG 1



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type ABCBUS = &device_creator<abcbus_device>;


//**************************************************************************
//  DEVICE INTERFACE
//**************************************************************************

//-------------------------------------------------
//  device_abcbus_interface - constructor
//-------------------------------------------------

device_abcbus_interface::device_abcbus_interface(const machine_config &mconfig, device_t &device)
	: device_interface(device)
{
}


//-------------------------------------------------
//  ~device_abcbus_interface - destructor
//-------------------------------------------------

device_abcbus_interface::~device_abcbus_interface()
{
}


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void abcbus_device::device_config_complete()
{
	// inherit a copy of the static data
	const abcbus_config *intf = reinterpret_cast<const abcbus_config *>(static_config());
	if (intf != NULL)
		*static_cast<abcbus_config *>(this) = *intf;

	// or initialize to defaults if none provided
	else
	{
		fatalerror("Daisy chain not provided!");
	}

	m_daisy = intf;
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  abcbus_device - constructor
//-------------------------------------------------

abcbus_device::abcbus_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, ABCBUS, "Luxor ABC bus", tag, owner, clock)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void abcbus_device::device_start()
{
	const abcbus_config *daisy = m_daisy;

	// create a linked list of devices
	daisy_entry **tailptr = &m_daisy_list;
	for ( ; daisy->devname != NULL; daisy++)
	{
		// find the device
		device_t *target = machine().device(daisy->devname);
		if (target == NULL)
			fatalerror("Unable to locate device '%s'", daisy->devname);

		// make sure it has an interface
		device_abcbus_interface *intf;
		if (!target->interface(intf))
			fatalerror("Device '%s' does not implement the abcbus interface!", daisy->devname);

		// append to the end
		*tailptr = auto_alloc(machine(), daisy_entry(target));
		tailptr = &(*tailptr)->m_next;
	}
}


//-------------------------------------------------
//  daisy_entry - constructor
//-------------------------------------------------

abcbus_device::daisy_entry::daisy_entry(device_t *device)
	: m_next(NULL),
	  m_device(device),
	  m_interface(NULL)
{
	device->interface(m_interface);
}


//-------------------------------------------------
//  cs_w -
//-------------------------------------------------

WRITE8_MEMBER( abcbus_device::cs_w )
{
	for (daisy_entry *daisy = m_daisy_list; daisy != NULL; daisy = daisy->m_next)
	{
		daisy->m_interface->abcbus_cs(data);
	}
}


//-------------------------------------------------
//  rst_r -
//-------------------------------------------------

READ8_MEMBER( abcbus_device::rst_r )
{
	for (daisy_entry *daisy = m_daisy_list; daisy != NULL; daisy = daisy->m_next)
	{
		daisy->m_interface->abcbus_rst(0);
		daisy->m_interface->abcbus_rst(1);
	}

	return 0xff;
}


//-------------------------------------------------
//  inp_r -
//-------------------------------------------------

READ8_MEMBER( abcbus_device::inp_r )
{
	UINT8 data = 0xff;

	for (daisy_entry *daisy = m_daisy_list; daisy != NULL; daisy = daisy->m_next)
	{
		data &= daisy->m_interface->abcbus_inp();
	}

	return data;
}


//-------------------------------------------------
//  utp_w -
//-------------------------------------------------

WRITE8_MEMBER( abcbus_device::utp_w )
{
	for (daisy_entry *daisy = m_daisy_list; daisy != NULL; daisy = daisy->m_next)
	{
		daisy->m_interface->abcbus_utp(data);
	}
}


//-------------------------------------------------
//  stat_r -
//-------------------------------------------------

READ8_MEMBER( abcbus_device::stat_r )
{
	UINT8 data = 0xff;

	for (daisy_entry *daisy = m_daisy_list; daisy != NULL; daisy = daisy->m_next)
	{
		data &= daisy->m_interface->abcbus_stat();
	}

	return data;
}


//-------------------------------------------------
//  c1_w -
//-------------------------------------------------

WRITE8_MEMBER( abcbus_device::c1_w )
{
	for (daisy_entry *daisy = m_daisy_list; daisy != NULL; daisy = daisy->m_next)
	{
		daisy->m_interface->abcbus_c1(data);
	}
}


//-------------------------------------------------
//  c2_w -
//-------------------------------------------------

WRITE8_MEMBER( abcbus_device::c2_w )
{
	for (daisy_entry *daisy = m_daisy_list; daisy != NULL; daisy = daisy->m_next)
	{
		daisy->m_interface->abcbus_c2(data);
	}
}


//-------------------------------------------------
//  c3_w -
//-------------------------------------------------

WRITE8_MEMBER( abcbus_device::c3_w )
{
	for (daisy_entry *daisy = m_daisy_list; daisy != NULL; daisy = daisy->m_next)
	{
		daisy->m_interface->abcbus_c3(data);
	}
}


//-------------------------------------------------
//  c4_w -
//-------------------------------------------------

WRITE8_MEMBER( abcbus_device::c4_w )
{
	for (daisy_entry *daisy = m_daisy_list; daisy != NULL; daisy = daisy->m_next)
	{
		daisy->m_interface->abcbus_c4(data);
	}
}


//-------------------------------------------------
//  resin_w -
//-------------------------------------------------

WRITE_LINE_MEMBER( abcbus_device::resin_w )
{
}


//-------------------------------------------------
//  int_w -
//-------------------------------------------------

WRITE_LINE_MEMBER( abcbus_device::int_w )
{
}


//-------------------------------------------------
//  nmi_w -
//-------------------------------------------------

WRITE_LINE_MEMBER( abcbus_device::nmi_w )
{
}


//-------------------------------------------------
//  rdy_w -
//-------------------------------------------------

WRITE_LINE_MEMBER( abcbus_device::rdy_w )
{
}
