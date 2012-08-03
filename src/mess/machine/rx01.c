/**********************************************************************

    DEC RX01 floppy drive controller

**********************************************************************/

/*

    TODO:
    - Create also unibus and qbus devices that contain this controller on them
*/

#include "emu.h"
#include "rx01.h"

//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type RX01 = &device_creator<rx01_device>;

//-------------------------------------------------
//  rx01_device - constructor
//-------------------------------------------------

rx01_device::rx01_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, RX01, "RX01", tag, owner, clock)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void rx01_device::device_start()
{
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void rx01_device::device_reset()
{
}

//-------------------------------------------------
//  read
//-------------------------------------------------

READ16_MEMBER( rx01_device::read )
{
	return 040;
}


//-------------------------------------------------
//  write
//-------------------------------------------------

WRITE16_MEMBER( rx01_device::write )
{
}

