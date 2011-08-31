/***************************************************************************

  ISA 8 bit Generic Communication Card

***************************************************************************/

#include "emu.h"
#include "isa_com.h"
#include "machine/ins8250.h"
#include "machine/pc_mouse.h"

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
	if (input_port_read(device->owner(), "DSW") & (0x80>>n))
		pc_mouse_handshake_in(device,data);
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
	MCFG_INS8250_ADD( "uart_0", genpc_com_interface[0] )
	MCFG_INS8250_ADD( "uart_1", genpc_com_interface[1] )
	MCFG_INS8250_ADD( "uart_2", genpc_com_interface[2] )
	MCFG_INS8250_ADD( "uart_3", genpc_com_interface[3] )
MACHINE_CONFIG_END

static INPUT_PORTS_START( com )
	PORT_START("DSW")
	PORT_DIPNAME( 0xf0, 0x80, "Serial mouse")
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_BIT( 0x0f, 0x0f,	IPT_UNUSED )
INPUT_PORTS_END

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

//-------------------------------------------------
//  input_ports - device-specific input ports
//-------------------------------------------------

ioport_constructor isa8_com_device::device_input_ports() const
{
	return INPUT_PORTS_NAME( com );
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

isa8_com_device::isa8_com_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, type, name, tag, owner, clock),
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
	m_isa->install_device(subdevice("uart_0"), 0x03f8, 0x03ff, 0, 0, FUNC(ins8250_r), FUNC(ins8250_w) );
	m_isa->install_device(subdevice("uart_1"), 0x02f8, 0x02ff, 0, 0, FUNC(ins8250_r), FUNC(ins8250_w) );
	m_isa->install_device(subdevice("uart_2"), 0x03e8, 0x03ef, 0, 0, FUNC(ins8250_r), FUNC(ins8250_w) );
	m_isa->install_device(subdevice("uart_3"), 0x02e8, 0x02ef, 0, 0, FUNC(ins8250_r), FUNC(ins8250_w) );
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void isa8_com_device::device_reset()
{
	pc_mouse_initialise(machine());
	pc_mouse_set_serial_port(subdevice("uart_0"));
}

static MACHINE_CONFIG_FRAGMENT( com_at_config )
	MCFG_NS16450_ADD( "uart_0", genpc_com_interface[0] ) /* Verified: IBM P/N 6320947 Serial/Parallel card uses an NS16450N */
	MCFG_NS16450_ADD( "uart_1", genpc_com_interface[1] )
	MCFG_NS16450_ADD( "uart_2", genpc_com_interface[2] )
	MCFG_NS16450_ADD( "uart_3", genpc_com_interface[3] )
MACHINE_CONFIG_END

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type ISA8_COM_AT = &device_creator<isa8_com_at_device>;

//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor isa8_com_at_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( com_at_config );
}

//-------------------------------------------------
//  isa8_com_device - constructor
//-------------------------------------------------

isa8_com_at_device::isa8_com_at_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
        isa8_com_device(mconfig, ISA8_COM_AT, "ISA8_COM_AT", tag, owner, clock)
{
}