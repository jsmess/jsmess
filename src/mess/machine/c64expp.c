/**********************************************************************

    Commodore 64 expansion port emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "machine/c64expp.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define LOG 1



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type C64_EXPANSION_PORT = &device_creator<c64_expansion_port_device>;



//**************************************************************************
//  DEVICE INTERFACE
//**************************************************************************

//-------------------------------------------------
//  device_c64_expansion_port_interface - constructor
//-------------------------------------------------

device_c64_expansion_port_interface::device_c64_expansion_port_interface(const machine_config &mconfig, device_t &device)
	: device_interface(device)
{
}


//-------------------------------------------------
//  ~device_c64_expansion_port_interface - destructor
//-------------------------------------------------

device_c64_expansion_port_interface::~device_c64_expansion_port_interface()
{
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  c64_expansion_port_device - constructor
//-------------------------------------------------

c64_expansion_port_device::c64_expansion_port_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, C64_EXPANSION_PORT, "C64 expansion port", tag, owner, clock),
	m_device(NULL),
	m_interface(NULL)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void c64_expansion_port_device::device_start()
{
	// resolve callbacks
	m_out_game_func.resolve(m_out_game_cb, *this);
	m_out_exrom_func.resolve(m_out_exrom_cb, *this);
	m_out_irq_func.resolve(m_out_irq_cb, *this);
	m_out_dma_func.resolve(m_out_dma_cb, *this);
	m_out_nmi_func.resolve(m_out_nmi_cb, *this);
	m_out_reset_func.resolve(m_out_reset_cb, *this);
}


//-------------------------------------------------
//  insert_cartridge - 
//-------------------------------------------------

void c64_expansion_port_device::insert_cartridge(device_t *device)
{
	device_c64_expansion_port_interface *intf;

	if (!device->interface(intf))
		fatalerror("Device '%s' does not implement the C64 expansion port interface!", device->tag());

	m_device = device;
	device->interface(m_interface);
}


//-------------------------------------------------
//  remove_cartridge - 
//-------------------------------------------------

void c64_expansion_port_device::remove_cartridge()
{
	m_device = NULL;
	m_interface = NULL;
}


//-------------------------------------------------
//  io1_r - I/O 1 read
//-------------------------------------------------

READ8_MEMBER( c64_expansion_port_device::io1_r )
{
	UINT8 data = 0;

	if (m_interface) 
		data = m_interface->c64_io1_r(offset);

	return data;
}


//-------------------------------------------------
//  io1_w - I/O 1 write
//-------------------------------------------------

WRITE8_MEMBER( c64_expansion_port_device::io1_w )
{
	if (m_interface) 
		m_interface->c64_io1_w(offset, data);
}


//-------------------------------------------------
//  io2_r - I/O 2 read
//-------------------------------------------------

READ8_MEMBER( c64_expansion_port_device::io2_r )
{
	UINT8 data = 0;

	if (m_interface) 
		data = m_interface->c64_io2_r(offset);

	return data;
}


//-------------------------------------------------
//  io2_w - I/O 2 write
//-------------------------------------------------

WRITE8_MEMBER( c64_expansion_port_device::io2_w )
{
	if (m_interface) 
		m_interface->c64_io2_w(offset, data);
}


//-------------------------------------------------
//  roml_r - ROM low read
//-------------------------------------------------

READ8_MEMBER( c64_expansion_port_device::roml_r )
{
	UINT8 data = 0;

	if (m_interface) 
		data = m_interface->c64_roml_r(offset);

	return data;
}


//-------------------------------------------------
//  roml_w - ROM low write
//-------------------------------------------------

WRITE8_MEMBER( c64_expansion_port_device::roml_w )
{
	if (m_interface) 
		m_interface->c64_roml_w(offset, data);
}


//-------------------------------------------------
//  romh_r - ROM high read
//-------------------------------------------------

READ8_MEMBER( c64_expansion_port_device::romh_r )
{
	UINT8 data = 0;

	if (m_interface) 
		data = m_interface->c64_romh_r(offset);

	return data;
}


//-------------------------------------------------
//  romh_w - ROM high write
//-------------------------------------------------

WRITE8_MEMBER( c64_expansion_port_device::romh_w )
{
	if (m_interface) 
		m_interface->c64_romh_w(offset, data);
}


//-------------------------------------------------
//  game_r - GAME read
//-------------------------------------------------

DECLARE_READ_LINE_MEMBER( c64_expansion_port_device::game_r )
{
	if (m_interface) 
		return m_interface->c64_game_r();
	else
		return 1;
}


//-------------------------------------------------
//  game_w - GAME write
//-------------------------------------------------

DECLARE_WRITE_LINE_MEMBER( c64_expansion_port_device::game_w )
{
	m_out_game_func(state);
}


//-------------------------------------------------
//  exrom_r - EXROM read
//-------------------------------------------------

DECLARE_READ_LINE_MEMBER( c64_expansion_port_device::exrom_r )
{
	if (m_interface) 
		return m_interface->c64_exrom_r();
	else
		return 1;
}


//-------------------------------------------------
//  exrom_w - EXROM write
//-------------------------------------------------

DECLARE_WRITE_LINE_MEMBER( c64_expansion_port_device::exrom_w )
{
	m_out_exrom_func(state);
}


//-------------------------------------------------
//  irq_w - interrupt request write
//-------------------------------------------------

WRITE_LINE_MEMBER( c64_expansion_port_device::irq_w )
{
	m_out_irq_func(state);
}


//-------------------------------------------------
//  nmi_w - non-maskable interrupt write
//-------------------------------------------------

WRITE_LINE_MEMBER( c64_expansion_port_device::nmi_w )
{
	m_out_nmi_func(state);
}


//-------------------------------------------------
//  dma_w - direct memory access write
//-------------------------------------------------

WRITE_LINE_MEMBER( c64_expansion_port_device::dma_w )
{
	m_out_dma_func(state);
}


//-------------------------------------------------
//  reset_w - reset write
//-------------------------------------------------

WRITE_LINE_MEMBER( c64_expansion_port_device::reset_w )
{
	m_out_reset_func(state);
}
