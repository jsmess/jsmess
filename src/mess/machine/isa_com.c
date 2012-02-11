/***************************************************************************

  ISA 8 bit Generic Communication Card

***************************************************************************/

#include "emu.h"
#include "isa_com.h"
#include "machine/ins8250.h"
#include "machine/ser_mouse.h"

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

static void pc_com_refresh_connected_common(device_t *uart, int n, int data)
{
	isa8_com_device	*com  = downcast<isa8_com_device *>(uart->owner());
	rs232_port_device *port = com->get_port(n);
	
	port->dtr_w(data & 1);
	port->rts_w((data & 2) && 1);
	ins8250_handshake_in(uart, (port->cts_r() << 4)|(port->dsr_r() << 5)|(port->ri_r() << 6)|(port->dcd_r() << 7));
}

static INS8250_HANDSHAKE_OUT( pc_com_handshake_out_0 ) { pc_com_refresh_connected_common( device, 0, data ); }
static INS8250_HANDSHAKE_OUT( pc_com_handshake_out_1 ) { pc_com_refresh_connected_common( device, 1, data ); }
static INS8250_HANDSHAKE_OUT( pc_com_handshake_out_2 ) { pc_com_refresh_connected_common( device, 2, data ); }
static INS8250_HANDSHAKE_OUT( pc_com_handshake_out_3 ) { pc_com_refresh_connected_common( device, 3, data ); }

static void pc_com_tx(device_t *uart, UINT8 data, int n)
{
	isa8_com_device	*com  = downcast<isa8_com_device *>(uart->owner());
	com->get_port(n)->tx8(*memory_nonspecific_space(uart->machine()), 0, data);
}

static INS8250_TRANSMIT( pc_com_tx_0 ) { pc_com_tx(device, data, 0); }
static INS8250_TRANSMIT( pc_com_tx_1 ) { pc_com_tx(device, data, 1); }
static INS8250_TRANSMIT( pc_com_tx_2 ) { pc_com_tx(device, data, 2); }
static INS8250_TRANSMIT( pc_com_tx_3 ) { pc_com_tx(device, data, 3); }

WRITE8_DEVICE_HANDLER( pc_com_rx_0 ) { ins8250_receive(downcast<isa8_com_device *>(device->owner())->get_uart(0), data); }
WRITE8_DEVICE_HANDLER( pc_com_rx_1 ) { ins8250_receive(downcast<isa8_com_device *>(device->owner())->get_uart(1), data); }
WRITE8_DEVICE_HANDLER( pc_com_rx_2 ) { ins8250_receive(downcast<isa8_com_device *>(device->owner())->get_uart(2), data); }
WRITE8_DEVICE_HANDLER( pc_com_rx_3 ) { ins8250_receive(downcast<isa8_com_device *>(device->owner())->get_uart(3), data); }

static const ins8250_interface genpc_com_interface[4]=
{
	{
		1843200,
		DEVCB_LINE(pc_com_interrupt_1),
		pc_com_tx_0,
		pc_com_handshake_out_0,
		NULL
	},
	{
		1843200,
		DEVCB_LINE(pc_com_interrupt_2),
		pc_com_tx_1,
		pc_com_handshake_out_1,
		NULL
	},
	{
		1843200,
		DEVCB_LINE(pc_com_interrupt_1),
		pc_com_tx_2,
		pc_com_handshake_out_2,
		NULL
	},
	{
		1843200,
		DEVCB_LINE(pc_com_interrupt_2),
		pc_com_tx_3,
		pc_com_handshake_out_3,
		NULL
	}
};

static const rs232_port_interface serport_config[4] =
{
	{
		DEVCB_HANDLER( pc_com_rx_0 ),
		DEVCB_NULL,
		DEVCB_NULL,
		DEVCB_NULL,
		DEVCB_NULL,
		DEVCB_NULL
	},
	{
		DEVCB_HANDLER( pc_com_rx_1 ),
		DEVCB_NULL,
		DEVCB_NULL,
		DEVCB_NULL,
		DEVCB_NULL,
		DEVCB_NULL
	},
	{
		DEVCB_HANDLER( pc_com_rx_2 ),
		DEVCB_NULL,
		DEVCB_NULL,
		DEVCB_NULL,
		DEVCB_NULL,
		DEVCB_NULL
	},
	{
		DEVCB_HANDLER( pc_com_rx_3 ),
		DEVCB_NULL,
		DEVCB_NULL,
		DEVCB_NULL,
		DEVCB_NULL,
		DEVCB_NULL
	}
};

static SLOT_INTERFACE_START(isa_com)
	SLOT_INTERFACE("mouse", SERIAL_MOUSE)
SLOT_INTERFACE_END

static MACHINE_CONFIG_FRAGMENT( com_config )
	MCFG_INS8250_ADD( "uart_0", genpc_com_interface[0] )
	MCFG_INS8250_ADD( "uart_1", genpc_com_interface[1] )
	MCFG_INS8250_ADD( "uart_2", genpc_com_interface[2] )
	MCFG_INS8250_ADD( "uart_3", genpc_com_interface[3] )
	MCFG_RS232_PORT_ADD( "serport0", serport_config[0], isa_com, "mouse", NULL )
	MCFG_RS232_PORT_ADD( "serport1", serport_config[1], isa_com, NULL, NULL )
	MCFG_RS232_PORT_ADD( "serport2", serport_config[3], isa_com, NULL, NULL )
	MCFG_RS232_PORT_ADD( "serport3", serport_config[4], isa_com, NULL, NULL )
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
		device_isa8_card_interface(mconfig, *this)
{
}

isa8_com_device::isa8_com_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, type, name, tag, owner, clock),
		device_isa8_card_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void isa8_com_device::device_start()
{
	set_isa_device();
	m_uart[0] = subdevice("uart_0");
	m_uart[1] = subdevice("uart_1");
	m_uart[2] = subdevice("uart_2");
	m_uart[3] = subdevice("uart_3");
	m_isa->install_device(m_uart[0], 0x03f8, 0x03ff, 0, 0, FUNC(ins8250_r), FUNC(ins8250_w) );
	m_isa->install_device(m_uart[1], 0x02f8, 0x02ff, 0, 0, FUNC(ins8250_r), FUNC(ins8250_w) );
	m_isa->install_device(m_uart[2], 0x03e8, 0x03ef, 0, 0, FUNC(ins8250_r), FUNC(ins8250_w) );
	m_isa->install_device(m_uart[3], 0x02e8, 0x02ef, 0, 0, FUNC(ins8250_r), FUNC(ins8250_w) );

	m_serport[0] = subdevice<rs232_port_device>("serport0");
	m_serport[1] = subdevice<rs232_port_device>("serport1");
	m_serport[2] = subdevice<rs232_port_device>("serport2");
	m_serport[3] = subdevice<rs232_port_device>("serport3");
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void isa8_com_device::device_reset()
{
}

static MACHINE_CONFIG_FRAGMENT( com_at_config )
	MCFG_NS16450_ADD( "uart_0", genpc_com_interface[0] ) /* Verified: IBM P/N 6320947 Serial/Parallel card uses an NS16450N */
	MCFG_NS16450_ADD( "uart_1", genpc_com_interface[1] )
	MCFG_NS16450_ADD( "uart_2", genpc_com_interface[2] )
	MCFG_NS16450_ADD( "uart_3", genpc_com_interface[3] )
	MCFG_RS232_PORT_ADD( "serport0", serport_config[0], isa_com, "mouse", NULL )
	MCFG_RS232_PORT_ADD( "serport1", serport_config[1], isa_com, NULL, NULL )
	MCFG_RS232_PORT_ADD( "serport2", serport_config[3], isa_com, NULL, NULL )
	MCFG_RS232_PORT_ADD( "serport3", serport_config[4], isa_com, NULL, NULL )
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
