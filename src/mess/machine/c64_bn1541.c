/**********************************************************************

    Burst Nibbler 1541 Parallel Cable emulation

    http://sta.c64.org/cbmparc2.html

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "c64_bn1541.h"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type C64_BN1541 = &device_creator<c64_bn1541_device>;



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  c64_bn1541_device - constructor
//-------------------------------------------------

c64_bn1541_device::c64_bn1541_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, C64_BN1541, "C64 Burst Nibbler 1541 Parallel Cable", tag, owner, clock),
	device_c64_user_port_interface(mconfig, *this),
	m_drive(NULL)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void c64_bn1541_device::device_start()
{
	device_iterator iter(machine().root_device());

	for (device_t *device = iter.first(); device != NULL; device = iter.next())
	{
		if (device->subdevice(C1541_TAG) != NULL)
		{
			// grab the first 1541 and run to the hills
			m_drive = device->subdevice<c1541_device>(C1541_TAG);
			break;
		}
	}
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void c64_bn1541_device::device_reset()
{
	if (m_drive != NULL)
	{
		m_drive->parallel_connect(downcast<c64_bn1541_device *>(this));
	}
}


//-------------------------------------------------
//  parallel_data_w -
//-------------------------------------------------

void c64_bn1541_device::parallel_data_w(UINT8 data)
{
    //logerror("1541 parallel data %02x\n", data);
	m_data = data;
}


//-------------------------------------------------
//  parallel_strobe_w -
//-------------------------------------------------

void c64_bn1541_device::parallel_strobe_w(int state)
{
    //logerror("1541 parallel strobe %u\n", state);
	m_slot->flag2_w(state);
}


//-------------------------------------------------
//  c64_pb_r - port B read
//-------------------------------------------------

UINT8 c64_bn1541_device::c64_pb_r(address_space &space, offs_t offset)
{
	return m_data;
}


//-------------------------------------------------
//  c64_pb_w - port B write
//-------------------------------------------------

void c64_bn1541_device::c64_pb_w(address_space &space, offs_t offset, UINT8 data)
{
	if (m_drive != NULL)
	{
		m_drive->parallel_data_w(data);
	}
}


//-------------------------------------------------
//  c64_pc2_w - CIA2 PC write
//-------------------------------------------------

void c64_bn1541_device::c64_pc2_w(int state)
{
	if (m_drive != NULL)
	{
		m_drive->parallel_strobe_w(state);
	}
}
