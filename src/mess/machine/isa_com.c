/***************************************************************************

  ISA 8 bit Generic Communication Card

***************************************************************************/

#include "emu.h"
#include "isa_com.h"
#include "machine/ins8250.h"


/* called when a interrupt is set/cleared from com hardware */
static WRITE_LINE_DEVICE_HANDLER( pc_com_interrupt_1 )
{
	isa8_com_device	*com  = downcast<isa8_com_device *>(device->owner());
	com->m_isa->irq4_w(state);
}

static WRITE_LINE_DEVICE_HANDLER( pc_com_interrupt_2 )
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
		DEVCB_LINE(pc_com_interrupt_1),
		NULL,
		pc_com_handshake_out_0,
		NULL
	},
	{
		1843200,
		DEVCB_LINE(pc_com_interrupt_2),
		NULL,
		pc_com_handshake_out_1,
		NULL
	},
	{
		1843200,
		DEVCB_LINE(pc_com_interrupt_1),
		NULL,
		pc_com_handshake_out_2,
		NULL
	},
	{
		1843200,
		DEVCB_LINE(pc_com_interrupt_2),
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

const device_type ISA8_COM = &device_creator<isa8_com_device>;

//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor isa8_com_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( com_config );
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  isa8_com_device - constructor
//-------------------------------------------------

isa8_com_device::isa8_com_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, ISA8_COM, "ISA8_COM", tag, owner, clock),
		device_isa8_card_interface(mconfig, *this),
		device_slot_card_interface(mconfig, *this)
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void isa8_com_device::device_start()
{
	set_isa_device();
	m_isa->install_device(subdevice("ins8250_0"), 0x03f8, 0x03ff, 0, 0, FUNC(ins8250_r), FUNC(ins8250_w) );
	m_isa->install_device(subdevice("ins8250_1"), 0x02f8, 0x02ff, 0, 0, FUNC(ins8250_r), FUNC(ins8250_w) );
	m_isa->install_device(subdevice("ins8250_2"), 0x03e8, 0x03ef, 0, 0, FUNC(ins8250_r), FUNC(ins8250_w) );
	m_isa->install_device(subdevice("ins8250_3"), 0x02e8, 0x02ef, 0, 0, FUNC(ins8250_r), FUNC(ins8250_w) );
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void isa8_com_device::device_reset()
{
}
