/***************************************************************************

	MPU-401 MIDI device interface

	TODO:
	- skeleton, doesn't do anything

***************************************************************************/

#include "emu.h"
#include "isa_mpu401.h"



READ8_MEMBER( isa8_mpu401_device::mpu401_r )
{
	return 0xff;
}

WRITE8_MEMBER( isa8_mpu401_device::mpu401_w )
{
	// ...
}

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type ISA8_MPU401 = &device_creator<isa8_mpu401_device>;


//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  isa8_adlib_device - constructor
//-------------------------------------------------

isa8_mpu401_device::isa8_mpu401_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
      : device_t(mconfig, ISA8_MPU401, "mpu401", tag, owner, clock),
		device_isa8_card_interface( mconfig, *this )
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void isa8_mpu401_device::device_start()
{
	set_isa_device();
	m_isa->install_device(0x330, 0x0331, 0, 0, read8_delegate(FUNC(isa8_mpu401_device::mpu401_r), this), write8_delegate(FUNC(isa8_mpu401_device::mpu401_w), this));
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void isa8_mpu401_device::device_reset()
{
}
