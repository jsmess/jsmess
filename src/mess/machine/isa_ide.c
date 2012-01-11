/***************************************************************************

  ISA 16 bit IDE controller

***************************************************************************/

#include "emu.h"
#include "isa_ide.h"
#include "machine/idectrl.h"
#include "imagedev/harddriv.h"

static READ16_DEVICE_HANDLER( ide16_r )
{
	return ide_controller16_r(device, 0x1f0/2 + offset, mem_mask);
}

static WRITE16_DEVICE_HANDLER( ide16_w )
{
	ide_controller16_w(device, 0x1f0/2 + offset, data, mem_mask);
}

static void ide_interrupt(device_t *device, int state)
{
	isa16_ide_device *ide  = downcast<isa16_ide_device *>(device->owner());
	ide->m_isa->irq14_w(state);
}


static MACHINE_CONFIG_FRAGMENT( ide )
	MCFG_HARDDISK_ADD("harddisk1")
	MCFG_HARDDISK_ADD("harddisk2")

	MCFG_IDE_CONTROLLER_ADD("ide", ide_interrupt)
	MCFG_IDE_CONTROLLER_REGIONS("harddisk1", "harddisk2")
MACHINE_CONFIG_END

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type ISA16_IDE = &device_creator<isa16_ide_device>;

//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor isa16_ide_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( ide );
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  isa16_ide_device - constructor
//-------------------------------------------------

isa16_ide_device::isa16_ide_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
      : device_t(mconfig, ISA16_IDE, "ISA16_IDE", tag, owner, clock),
		device_isa16_card_interface( mconfig, *this )
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void isa16_ide_device::device_start()
{
	set_isa_device();
	m_isa->install16_device(subdevice("ide"), 0x01f0, 0x01f7, 0, 0, FUNC(ide16_r), FUNC(ide16_w) );
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void isa16_ide_device::device_reset()
{
}
