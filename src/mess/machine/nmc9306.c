/**********************************************************************

    National Semiconductor NMC9306 256-Bit Serial EEPROM emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "emu.h"
#include "nmc9306.h"
#include "machine/devhelpr.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define RAM_SIZE 32


// instructions
enum
{
	EWDS = 0,		// erase/write disable
	WRAL,			// write all registers
	ERAL,			// erase all registers
	EWEN,			// erase/write enable
	WRITE = 7,		// write register A3A2A1A0
	READ,			// read register A3A2A1A0
	ERASE			// erase register A3A2A1A0
};



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type NMC9306 = nmc9306_device_config::static_alloc_device_config;



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  nmc9306_device_config - constructor
//-------------------------------------------------

nmc9306_device_config::nmc9306_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
	: device_config(mconfig, static_alloc_device_config, "NMC9306", tag, owner, clock),
	  device_config_nvram_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  static_alloc_device_config - allocate a new
//  configuration object
//-------------------------------------------------

device_config *nmc9306_device_config::static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
{
	return global_alloc(nmc9306_device_config(mconfig, tag, owner, clock));
}


//-------------------------------------------------
//  alloc_device - allocate a new device object
//-------------------------------------------------

device_t *nmc9306_device_config::alloc_device(running_machine &machine) const
{
	return auto_alloc(&machine, nmc9306_device(machine, *this));
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  nmc9306_device - constructor
//-------------------------------------------------

nmc9306_device::nmc9306_device(running_machine &_machine, const nmc9306_device_config &config)
    : device_t(_machine, config),
	  device_nvram_interface(_machine, config, *this),
      m_config(config)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void nmc9306_device::device_start()
{
}


//-------------------------------------------------
//  nvram_default - called to initialize NVRAM to
//  its default state
//-------------------------------------------------

void nmc9306_device::nvram_default()
{
}


//-------------------------------------------------
//  nvram_read - called to read NVRAM from the
//  .nv file
//-------------------------------------------------

void nmc9306_device::nvram_read(emu_file &file)
{
	file.read(m_register, RAM_SIZE);
}


//-------------------------------------------------
//  nvram_write - called to write NVRAM to the
//  .nv file
//-------------------------------------------------

void nmc9306_device::nvram_write(emu_file &file)
{
	file.write(m_register, RAM_SIZE);
}


//-------------------------------------------------
//  cs_w - chip select input
//-------------------------------------------------

WRITE_LINE_MEMBER( nmc9306_device::cs_w )
{
	m_cs = state;
}


//-------------------------------------------------
//  ck_w - serial clock input
//-------------------------------------------------

WRITE_LINE_MEMBER( nmc9306_device::sk_w )
{
	m_sk = state;
}


//-------------------------------------------------
//  di_w - serial data input
//-------------------------------------------------

WRITE_LINE_MEMBER( nmc9306_device::di_w )
{
	m_di = state;
}


//-------------------------------------------------
//  do_r - serial data output
//-------------------------------------------------

READ_LINE_MEMBER( nmc9306_device::do_r )
{
	return m_do;
}
