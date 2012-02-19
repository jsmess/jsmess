/***************************************************************************

  ISA 8 bit Generic Communication Card

***************************************************************************/

#include "isa_com.h"
#include "machine/ser_mouse.h"
#include "machine/terminal.h"
#include "machine/null_modem.h"

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

static void pc_com_tx(device_t *uart, UINT8 state, int n)
{
	isa8_com_device	*com  = downcast<isa8_com_device *>(uart->owner());
	com->get_port(n)->tx(state);
}

static void pc_com_dtr(device_t *uart, UINT8 state, int n)
{
	isa8_com_device	*com  = downcast<isa8_com_device *>(uart->owner());
	com->get_port(n)->dtr_w(state);
}

static void pc_com_rts(device_t *uart, UINT8 state, int n)
{
	isa8_com_device	*com  = downcast<isa8_com_device *>(uart->owner());
	com->get_port(n)->rts_w(state);
}

static WRITE_LINE_DEVICE_HANDLER( pc_com_tx_0 ) { pc_com_tx(device, state, 0); }
static WRITE_LINE_DEVICE_HANDLER( pc_com_dtr_0 ) { pc_com_dtr(device, state, 0); }
static WRITE_LINE_DEVICE_HANDLER( pc_com_rts_0 ) { pc_com_rts(device, state, 0); }

static WRITE_LINE_DEVICE_HANDLER( pc_com_tx_1 ) { pc_com_tx(device, state, 1); }
static WRITE_LINE_DEVICE_HANDLER( pc_com_dtr_1 ) { pc_com_dtr(device, state, 1); }
static WRITE_LINE_DEVICE_HANDLER( pc_com_rts_1 ) { pc_com_rts(device, state, 1); }

static WRITE_LINE_DEVICE_HANDLER( pc_com_tx_2 ) { pc_com_tx(device, state, 2); }
static WRITE_LINE_DEVICE_HANDLER( pc_com_dtr_2 ) { pc_com_dtr(device, state, 2); }
static WRITE_LINE_DEVICE_HANDLER( pc_com_rts_2 ) { pc_com_rts(device, state, 2); }

static WRITE_LINE_DEVICE_HANDLER( pc_com_tx_3 ) { pc_com_tx(device, state, 3); }
static WRITE_LINE_DEVICE_HANDLER( pc_com_dtr_3 ) { pc_com_dtr(device, state, 3); }
static WRITE_LINE_DEVICE_HANDLER( pc_com_rts_3 ) { pc_com_rts(device, state, 3); }

static void pc_com_rx(device_t *port, UINT8 state, int n)
{
	isa8_com_device *com  = downcast<isa8_com_device *>(port->owner());
	com->get_uart(n)->rx_w(state);
}

static void pc_com_dcd(device_t *port, UINT8 state, int n)
{
	isa8_com_device *com  = downcast<isa8_com_device *>(port->owner());
	com->get_uart(n)->dcd_w(state);
}

static void pc_com_dsr(device_t *port, UINT8 state, int n)
{
	isa8_com_device *com  = downcast<isa8_com_device *>(port->owner());
	com->get_uart(n)->dsr_w(state);
}

static void pc_com_ri(device_t *port, UINT8 state, int n)
{
	isa8_com_device *com  = downcast<isa8_com_device *>(port->owner());
	com->get_uart(n)->ri_w(state);
}

static void pc_com_cts(device_t *port, UINT8 state, int n)
{
	isa8_com_device *com  = downcast<isa8_com_device *>(port->owner());
	com->get_uart(n)->cts_w(state);
}

static WRITE_LINE_DEVICE_HANDLER( pc_com_rx_0 ) { pc_com_rx(device, state, 0); }
static WRITE_LINE_DEVICE_HANDLER( pc_com_dcd_0 ) { pc_com_dcd(device, state, 0); }
static WRITE_LINE_DEVICE_HANDLER( pc_com_dsr_0 ) { pc_com_dsr(device, state, 0); }
static WRITE_LINE_DEVICE_HANDLER( pc_com_ri_0 ) { pc_com_ri(device, state, 0); }
static WRITE_LINE_DEVICE_HANDLER( pc_com_cts_0 ) { pc_com_cts(device, state, 0); }

static WRITE_LINE_DEVICE_HANDLER( pc_com_rx_1 ) { pc_com_rx(device, state, 1); }
static WRITE_LINE_DEVICE_HANDLER( pc_com_dcd_1 ) { pc_com_dcd(device, state, 1); }
static WRITE_LINE_DEVICE_HANDLER( pc_com_dsr_1 ) { pc_com_dsr(device, state, 1); }
static WRITE_LINE_DEVICE_HANDLER( pc_com_ri_1 ) { pc_com_ri(device, state, 1); }
static WRITE_LINE_DEVICE_HANDLER( pc_com_cts_1 ) { pc_com_cts(device, state, 1); }

static WRITE_LINE_DEVICE_HANDLER( pc_com_rx_2 ) { pc_com_rx(device, state, 2); }
static WRITE_LINE_DEVICE_HANDLER( pc_com_dcd_2 ) { pc_com_dcd(device, state, 2); }
static WRITE_LINE_DEVICE_HANDLER( pc_com_dsr_2 ) { pc_com_dsr(device, state, 2); }
static WRITE_LINE_DEVICE_HANDLER( pc_com_ri_2 ) { pc_com_ri(device, state, 2); }
static WRITE_LINE_DEVICE_HANDLER( pc_com_cts_2 ) { pc_com_cts(device, state, 2); }

static WRITE_LINE_DEVICE_HANDLER( pc_com_rx_3 ) { pc_com_rx(device, state, 3); }
static WRITE_LINE_DEVICE_HANDLER( pc_com_dcd_3 ) { pc_com_dcd(device, state, 3); }
static WRITE_LINE_DEVICE_HANDLER( pc_com_dsr_3 ) { pc_com_dsr(device, state, 3); }
static WRITE_LINE_DEVICE_HANDLER( pc_com_ri_3 ) { pc_com_ri(device, state, 3); }
static WRITE_LINE_DEVICE_HANDLER( pc_com_cts_3 ) { pc_com_cts(device, state, 3); }

static const ins8250_interface genpc_com_interface[4]=
{
	{
		DEVCB_LINE(pc_com_tx_0),
		DEVCB_LINE(pc_com_dtr_0),
		DEVCB_LINE(pc_com_rts_0),
		DEVCB_LINE(pc_com_interrupt_1),
		DEVCB_NULL,
		DEVCB_NULL
	},
	{
		DEVCB_LINE(pc_com_tx_1),
		DEVCB_LINE(pc_com_dtr_1),
		DEVCB_LINE(pc_com_rts_1),
		DEVCB_LINE(pc_com_interrupt_2),
		DEVCB_NULL,
		DEVCB_NULL
	},
	{
		DEVCB_LINE(pc_com_tx_2),
		DEVCB_LINE(pc_com_dtr_2),
		DEVCB_LINE(pc_com_rts_2),
		DEVCB_LINE(pc_com_interrupt_1),
		DEVCB_NULL,
		DEVCB_NULL
	},
	{
		DEVCB_LINE(pc_com_tx_3),
		DEVCB_LINE(pc_com_dtr_3),
		DEVCB_LINE(pc_com_rts_3),
		DEVCB_LINE(pc_com_interrupt_2),
		DEVCB_NULL,
		DEVCB_NULL
	}
};

static const rs232_port_interface serport_config[4] =
{
	{
		DEVCB_LINE( pc_com_rx_0 ),
		DEVCB_LINE( pc_com_dcd_0 ),
		DEVCB_LINE( pc_com_dsr_0 ),
		DEVCB_LINE( pc_com_ri_0 ),
		DEVCB_LINE( pc_com_cts_0 )
	},
	{
		DEVCB_LINE( pc_com_rx_1 ),
		DEVCB_LINE( pc_com_dcd_1 ),
		DEVCB_LINE( pc_com_dsr_1 ),
		DEVCB_LINE( pc_com_ri_1 ),
		DEVCB_LINE( pc_com_cts_1 )
	},
	{
		DEVCB_LINE( pc_com_rx_2 ),
		DEVCB_LINE( pc_com_dcd_2 ),
		DEVCB_LINE( pc_com_dsr_2 ),
		DEVCB_LINE( pc_com_ri_2 ),
		DEVCB_LINE( pc_com_cts_2 )
	},
	{
		DEVCB_LINE( pc_com_rx_3 ),
		DEVCB_LINE( pc_com_dcd_3 ),
		DEVCB_LINE( pc_com_dsr_3 ),
		DEVCB_LINE( pc_com_ri_3 ),
		DEVCB_LINE( pc_com_cts_3 )
	}
};

static SLOT_INTERFACE_START(isa_com)
	SLOT_INTERFACE("microsoft_mouse", MSFT_SERIAL_MOUSE)
	SLOT_INTERFACE("msystems_mouse", MSYSTEM_SERIAL_MOUSE)
	SLOT_INTERFACE("serial_terminal", SERIAL_TERMINAL)
	SLOT_INTERFACE("null_modem", NULL_MODEM)
SLOT_INTERFACE_END

static MACHINE_CONFIG_FRAGMENT( com_config )
	MCFG_INS8250_ADD( "uart_0", genpc_com_interface[0], XTAL_1_8432MHz )
	MCFG_INS8250_ADD( "uart_1", genpc_com_interface[1], XTAL_1_8432MHz )
	MCFG_INS8250_ADD( "uart_2", genpc_com_interface[2], XTAL_1_8432MHz )
	MCFG_INS8250_ADD( "uart_3", genpc_com_interface[3], XTAL_1_8432MHz )
	MCFG_RS232_PORT_ADD( "serport0", serport_config[0], isa_com, "microsoft_mouse", NULL )
	MCFG_RS232_PORT_ADD( "serport1", serport_config[1], isa_com, NULL, NULL )
	MCFG_RS232_PORT_ADD( "serport2", serport_config[2], isa_com, NULL, NULL )
	MCFG_RS232_PORT_ADD( "serport3", serport_config[3], isa_com, NULL, NULL )
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
	m_uart[0] = subdevice<ins8250_uart_device>("uart_0");
	m_uart[1] = subdevice<ins8250_uart_device>("uart_1");
	m_uart[2] = subdevice<ins8250_uart_device>("uart_2");
	m_uart[3] = subdevice<ins8250_uart_device>("uart_3");
	m_isa->install_device(0x03f8, 0x03ff, 0, 0, read8_delegate(FUNC(ins8250_device::ins8250_r), m_uart[0]), write8_delegate(FUNC(ins8250_device::ins8250_w), m_uart[0]) );
	m_isa->install_device(0x02f8, 0x02ff, 0, 0, read8_delegate(FUNC(ins8250_device::ins8250_r), m_uart[1]), write8_delegate(FUNC(ins8250_device::ins8250_w), m_uart[1]) );
	m_isa->install_device(0x03e8, 0x03ef, 0, 0, read8_delegate(FUNC(ins8250_device::ins8250_r), m_uart[2]), write8_delegate(FUNC(ins8250_device::ins8250_w), m_uart[2]) );
	m_isa->install_device(0x02e8, 0x02ef, 0, 0, read8_delegate(FUNC(ins8250_device::ins8250_r), m_uart[3]), write8_delegate(FUNC(ins8250_device::ins8250_w), m_uart[3]) );

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
	MCFG_NS16450_ADD( "uart_0", genpc_com_interface[0], XTAL_1_8432MHz ) /* Verified: IBM P/N 6320947 Serial/Parallel card uses an NS16450N */
	MCFG_NS16450_ADD( "uart_1", genpc_com_interface[1], XTAL_1_8432MHz )
	MCFG_NS16450_ADD( "uart_2", genpc_com_interface[2], XTAL_1_8432MHz )
	MCFG_NS16450_ADD( "uart_3", genpc_com_interface[3], XTAL_1_8432MHz )
	MCFG_RS232_PORT_ADD( "serport0", serport_config[0], isa_com, "microsoft_mouse", NULL )
	MCFG_RS232_PORT_ADD( "serport1", serport_config[1], isa_com, NULL, NULL )
	MCFG_RS232_PORT_ADD( "serport2", serport_config[2], isa_com, NULL, NULL )
	MCFG_RS232_PORT_ADD( "serport3", serport_config[3], isa_com, NULL, NULL )
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
