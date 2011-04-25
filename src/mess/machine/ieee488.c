/**********************************************************************

    IEEE-488.1 General Purpose Interface Bus emulation
    (aka HP-IB, GPIB, CBM IEEE)

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "ieee488.h"
#include "machine/devhelpr.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define LOG 0


static const char *const SIGNAL_NAME[] = { "EOI", "DAV", "NRFD", "NDAC", "IFC", "SRQ", "ATN", "REN" };



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type IEEE488 = ieee488_device_config::static_alloc_device_config;



//**************************************************************************
//  DEVICE CONFIGURATION INTERFACE
//**************************************************************************

//-------------------------------------------------
//  device_config_ieee488_interface - constructor
//-------------------------------------------------

device_config_ieee488_interface::device_config_ieee488_interface(const machine_config &mconfig, device_config &devconfig)
	: device_config_interface(mconfig, devconfig)
{
}


//-------------------------------------------------
//  ~device_config_ieee488_interface - destructor
//-------------------------------------------------

device_config_ieee488_interface::~device_config_ieee488_interface()
{
}



//**************************************************************************
//  DEVICE INTERFACE
//**************************************************************************

//-------------------------------------------------
//  device_ieee488_interface - constructor
//-------------------------------------------------

device_ieee488_interface::device_ieee488_interface(running_machine &machine, const device_config &config, device_t &device)
	: device_interface(machine, config, device),
	  m_ieee488_config(dynamic_cast<const device_config_ieee488_interface &>(config))
{
}


//-------------------------------------------------
//  ~device_ieee488_interface - destructor
//-------------------------------------------------

device_ieee488_interface::~device_ieee488_interface()
{
}



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

GENERIC_DEVICE_CONFIG_SETUP(ieee488, "IEEE488")


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void ieee488_device_config::device_config_complete()
{
	// inherit a copy of the static data
	const ieee488_config *intf = reinterpret_cast<const ieee488_config *>(static_config());
	if (intf != NULL)
		*static_cast<ieee488_config *>(this) = *intf;

	// or initialize to defaults if none provided
	else
	{
		fatalerror("Daisy chain not provided!");
	}

	m_daisy = intf;
}



//**************************************************************************
//  INLINE HELPERS
//**************************************************************************

//-------------------------------------------------
//  set_signal -
//-------------------------------------------------

inline void ieee488_device::set_signal(device_t *device, int signal, int state)
{
	bool changed = false;

	for (daisy_entry *daisy = m_daisy_list; daisy != NULL; daisy = daisy->m_next)
	{
		if (!strcmp(daisy->m_device->tag(), device->tag()))
		{
			if (daisy->m_line[signal] != state)
			{
				if (LOG) logerror("IEEE488: '%s' %s %u\n", device->tag(), SIGNAL_NAME[signal], state);
				daisy->m_line[signal] = state;
				changed = true;
			}
		}
	}

	if (changed)
	{
		for (daisy_entry *daisy = m_daisy_list; daisy != NULL; daisy = daisy->m_next)
		{
			switch (signal)
			{
			case EOI:
				daisy->m_interface->ieee488_eoi(state);
				break;

			case DAV:
				daisy->m_interface->ieee488_dav(state);
				break;

			case NRFD:
				daisy->m_interface->ieee488_nrfd(state);
				break;

			case NDAC:
				daisy->m_interface->ieee488_ndac(state);
				break;

			case IFC:
				daisy->m_interface->ieee488_ifc(state);
				break;

			case SRQ:
				daisy->m_interface->ieee488_srq(state);
				break;

			case ATN:
				daisy->m_interface->ieee488_atn(state);
				break;

			case REN:
				daisy->m_interface->ieee488_ren(state);
				break;
			}
		}

		if (LOG) logerror("IEEE-488: EOI %u DAV %u NRFD %u NDAC %u IFC %u SRQ %u ATN %u REN %u DIO %02x\n",
			get_signal(EOI), get_signal(DAV), get_signal(NRFD), get_signal(NDAC),
			get_signal(IFC), get_signal(SRQ), get_signal(ATN), get_signal(REN), get_data());
	}
}


//-------------------------------------------------
//  get_signal -
//-------------------------------------------------

inline int ieee488_device::get_signal(int signal)
{
	int state = 1;

	for (daisy_entry *daisy = m_daisy_list; daisy != NULL; daisy = daisy->m_next)
	{
		if (!daisy->m_line[signal])
		{
			state = 0;
			break;
		}
	}

	return state;
}


//-------------------------------------------------
//  set_data -
//-------------------------------------------------

inline void ieee488_device::set_data(device_t *device, UINT8 data)
{
	for (daisy_entry *daisy = m_daisy_list; daisy != NULL; daisy = daisy->m_next)
	{
		if (!strcmp(daisy->m_device->tag(), device->tag()))
		{
			if (daisy->m_dio != data)
			{
				if (LOG) logerror("IEEE488: '%s' DIO %02x\n", device->tag(), data);
				daisy->m_dio = data;
			}
		}
	}
}


//-------------------------------------------------
//  get_data -
//-------------------------------------------------

inline UINT8 ieee488_device::get_data()
{
	UINT8 data = 0xff;

	for (daisy_entry *daisy = m_daisy_list; daisy != NULL; daisy = daisy->m_next)
	{
		data &= daisy->m_dio;
	}

	return data;
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  ieee488_device - constructor
//-------------------------------------------------

ieee488_device::ieee488_device(running_machine &_machine, const ieee488_device_config &_config)
    : device_t(_machine, _config),
	  m_stub(*this->owner(), IEEE488_STUB_TAG),
      m_config(_config)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void ieee488_device::device_start()
{
	const ieee488_config *daisy = m_config.m_daisy;

	// create a linked list of devices
	daisy_entry **tailptr = &m_daisy_list;
	for ( ; daisy->m_tag != NULL; daisy++)
	{
		// find the device
		device_t *target = machine().device(daisy->m_tag);
		if (target == NULL)
			fatalerror("Unable to locate device '%s'", daisy->m_tag);

		// make sure it has an interface
		device_ieee488_interface *intf;
		if (!target->interface(intf))
			fatalerror("Device '%s' does not implement the CBM IEC interface!", daisy->m_tag);

		// append to the end
		*tailptr = auto_alloc(machine(), daisy_entry(target));
		tailptr = &(*tailptr)->m_next;
	}

	// append driver stub to the end
	device_t *target = machine().device(IEEE488_STUB_TAG);
	*tailptr = auto_alloc(machine(), daisy_entry(target));
	tailptr = &(*tailptr)->m_next;
}


//-------------------------------------------------
//  daisy_entry - constructor
//-------------------------------------------------

ieee488_device::daisy_entry::daisy_entry(device_t *device)
	: m_next(NULL),
	  m_device(device),
	  m_interface(NULL),
	  m_dio(0xff)
{
	for (int i = 0; i < SIGNAL_COUNT; i++)
	{
		m_line[i] = 1;
	}

	device->interface(m_interface);
}


//-------------------------------------------------
//  dio_r -
//-------------------------------------------------

UINT8 ieee488_device::dio_r()
{
	return get_data();
}


//-------------------------------------------------
//  dio_r -
//-------------------------------------------------

READ8_MEMBER( ieee488_device::dio_r )
{
	return get_data();
}


//-------------------------------------------------
//  eoi_r -
//-------------------------------------------------

READ_LINE_MEMBER( ieee488_device::eoi_r )
{
	return get_signal(EOI);
}


//-------------------------------------------------
//  dav_r -
//-------------------------------------------------

READ_LINE_MEMBER( ieee488_device::dav_r )
{
	return get_signal(DAV);
}


//-------------------------------------------------
//  nrfd_r -
//-------------------------------------------------

READ_LINE_MEMBER( ieee488_device::nrfd_r )
{
	return get_signal(NRFD);
}


//-------------------------------------------------
//  ndac_r -
//-------------------------------------------------

READ_LINE_MEMBER( ieee488_device::ndac_r )
{
	return get_signal(NDAC);
}


//-------------------------------------------------
//  ifc_r -
//-------------------------------------------------

READ_LINE_MEMBER( ieee488_device::ifc_r )
{
	return get_signal(IFC);
}


//-------------------------------------------------
//  srq_r -
//-------------------------------------------------

READ_LINE_MEMBER( ieee488_device::srq_r )
{
	return get_signal(SRQ);
}


//-------------------------------------------------
//  atn_r -
//-------------------------------------------------

READ_LINE_MEMBER( ieee488_device::atn_r )
{
	return get_signal(ATN);
}


//-------------------------------------------------
//  ren_r -
//-------------------------------------------------

READ_LINE_MEMBER( ieee488_device::ren_r )
{
	return get_signal(REN);
}


//-------------------------------------------------
//  dio_w -
//-------------------------------------------------

void ieee488_device::dio_w(UINT8 data)
{
	return set_data(m_stub, data);
}


//-------------------------------------------------
//  dio_w -
//-------------------------------------------------

WRITE8_MEMBER( ieee488_device::dio_w )
{
	return set_data(m_stub, data);
}


//-------------------------------------------------
//  eoi_w -
//-------------------------------------------------

WRITE_LINE_MEMBER( ieee488_device::eoi_w )
{
	set_signal(m_stub, EOI, state);
}


//-------------------------------------------------
//  dav_w -
//-------------------------------------------------

WRITE_LINE_MEMBER( ieee488_device::dav_w )
{
	set_signal(m_stub, DAV, state);
}


//-------------------------------------------------
//  nrfd_w -
//-------------------------------------------------

WRITE_LINE_MEMBER( ieee488_device::nrfd_w )
{
	set_signal(m_stub, NRFD, state);
}


//-------------------------------------------------
//  ndac_w -
//-------------------------------------------------

WRITE_LINE_MEMBER( ieee488_device::ndac_w )
{
	set_signal(m_stub, NDAC, state);
}


//-------------------------------------------------
//  ifc_w -
//-------------------------------------------------

WRITE_LINE_MEMBER( ieee488_device::ifc_w )
{
	set_signal(m_stub, IFC, state);
}


//-------------------------------------------------
//  srq_w -
//-------------------------------------------------

WRITE_LINE_MEMBER( ieee488_device::srq_w )
{
	set_signal(m_stub, SRQ, state);
}


//-------------------------------------------------
//  atn_w -
//-------------------------------------------------

WRITE_LINE_MEMBER( ieee488_device::atn_w )
{
	set_signal(m_stub, ATN, state);
}


//-------------------------------------------------
//  ren_w -
//-------------------------------------------------

WRITE_LINE_MEMBER( ieee488_device::ren_w )
{
	set_signal(m_stub, REN, state);
}


//-------------------------------------------------
//  dio_w -
//-------------------------------------------------

void ieee488_device::dio_w(device_t *device, UINT8 data)
{
	return set_data(device, data);
}


//-------------------------------------------------
//  eoi_w -
//-------------------------------------------------

void ieee488_device::eoi_w(device_t *device, int state)
{
	set_signal(device, EOI, state);
}


//-------------------------------------------------
//  dav_w -
//-------------------------------------------------

void ieee488_device::dav_w(device_t *device, int state)
{
	set_signal(device, DAV, state);
}


//-------------------------------------------------
//  nrfd_w -
//-------------------------------------------------

void ieee488_device::nrfd_w(device_t *device, int state)
{
	set_signal(device, NRFD, state);
}


//-------------------------------------------------
//  ndac_w -
//-------------------------------------------------

void ieee488_device::ndac_w(device_t *device, int state)
{
	set_signal(device, NDAC, state);
}


//-------------------------------------------------
//  ifc_w -
//-------------------------------------------------

void ieee488_device::ifc_w(device_t *device, int state)
{
	set_signal(device, IFC, state);
}


//-------------------------------------------------
//  srq_w -
//-------------------------------------------------

void ieee488_device::srq_w(device_t *device, int state)
{
	set_signal(device, SRQ, state);
}


//-------------------------------------------------
//  atn_w -
//-------------------------------------------------

void ieee488_device::atn_w(device_t *device, int state)
{
	set_signal(device, ATN, state);
}


//-------------------------------------------------
//  ren_w -
//-------------------------------------------------

void ieee488_device::ren_w(device_t *device, int state)
{
	set_signal(device, REN, state);
}
