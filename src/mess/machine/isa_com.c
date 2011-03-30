/***************************************************************************

  ISA 8 bit Generic Communication Card

***************************************************************************/

#include "emu.h"
#include "isa_com.h"
#include "machine/ins8250.h"


/* called when a interrupt is set/cleared from com hardware */
static INS8250_INTERRUPT( pc_com_interrupt_1 )
{
	isa8_com_device	*com  = downcast<isa8_com_device *>(device->owner());
	com->m_isa->irq4_w(state);
}

static INS8250_INTERRUPT( pc_com_interrupt_2 )
{
	isa8_com_device	*com  = downcast<isa8_com_device *>(device->owner());
	com->m_isa->irq3_w(state);
}

/* called when com registers read/written - used to update peripherals that
are connected */
static void pc_com_refresh_connected_common(device_t *device, int n, int data)
{
	/* mouse connected to this port? */
	//if (input_port_read(device->machine(), "DSW2") & (0x80>>n))
		//pc_mouse_handshake_in(device,data);
}

static INS8250_HANDSHAKE_OUT( pc_com_handshake_out_0 ) { pc_com_refresh_connected_common( device, 0, data ); }
static INS8250_HANDSHAKE_OUT( pc_com_handshake_out_1 ) { pc_com_refresh_connected_common( device, 1, data ); }
static INS8250_HANDSHAKE_OUT( pc_com_handshake_out_2 ) { pc_com_refresh_connected_common( device, 2, data ); }
static INS8250_HANDSHAKE_OUT( pc_com_handshake_out_3 ) { pc_com_refresh_connected_common( device, 3, data ); }

static const ins8250_interface genpc_com_interface[4]=
{
	{
		1843200,
		pc_com_interrupt_1,
		NULL,
		pc_com_handshake_out_0,
		NULL
	},
	{
		1843200,
		pc_com_interrupt_2,
		NULL,
		pc_com_handshake_out_1,
		NULL
	},
	{
		1843200,
		pc_com_interrupt_1,
		NULL,
		pc_com_handshake_out_2,
		NULL
	},
	{
		1843200,
		pc_com_interrupt_2,
		NULL,
		pc_com_handshake_out_3,
		NULL
	}
};

static MACHINE_CONFIG_FRAGMENT( com_config )
	MCFG_INS8250_ADD( "ins8250_0", genpc_com_interface[0] )
	MCFG_INS8250_ADD( "ins8250_1", genpc_com_interface[1] )
	MCFG_INS8250_ADD( "ins8250_2", genpc_com_interface[2] )
	MCFG_INS8250_ADD( "ins8250_3", genpc_com_interface[3] )
MACHINE_CONFIG_END

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************
 
const device_type ISA8_COM = isa8_com_device_config::static_alloc_device_config;
 
//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************
 
//-------------------------------------------------
//  isa8_com_device_config - constructor
//-------------------------------------------------
 
isa8_com_device_config::isa8_com_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
        : device_config(mconfig, static_alloc_device_config, "ISA8_COM", tag, owner, clock),
			device_config_isa8_card_interface(mconfig, *this)
{
}
 
//-------------------------------------------------
//  static_alloc_device_config - allocate a new
//  configuration object
//-------------------------------------------------
 
device_config *isa8_com_device_config::static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
{
        return global_alloc(isa8_com_device_config(mconfig, tag, owner, clock));
}
 
//-------------------------------------------------
//  alloc_device - allocate a new device object
//-------------------------------------------------
 
device_t *isa8_com_device_config::alloc_device(running_machine &machine) const
{
        return auto_alloc(machine, isa8_com_device(machine, *this));
}
 
//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor isa8_com_device_config::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( com_config );
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************
 
//-------------------------------------------------
//  isa8_com_device - constructor
//-------------------------------------------------
 
isa8_com_device::isa8_com_device(running_machine &_machine, const isa8_com_device_config &config) :
        device_t(_machine, config),
		device_isa8_card_interface( _machine, config, *this ),
        m_config(config),
		m_isa(*owner(),config.m_isa_tag)
{
}
 
//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------
 
void isa8_com_device::device_start()
{        
	m_isa->add_isa_card(this, m_config.m_isa_num);
	m_isa->install_device(subdevice("ins8250_0"), 0x03f8, 0x03ff, 0, 0, ins8250_r, ins8250_w );
	m_isa->install_device(subdevice("ins8250_1"), 0x02f8, 0x02ff, 0, 0, ins8250_r, ins8250_w );
	m_isa->install_device(subdevice("ins8250_2"), 0x03e8, 0x03ef, 0, 0, ins8250_r, ins8250_w );
	m_isa->install_device(subdevice("ins8250_3"), 0x02e8, 0x02ef, 0, 0, ins8250_r, ins8250_w );
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------
 
void isa8_com_device::device_reset()
{
}
