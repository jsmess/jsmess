/**********************************************************************

    LH5810/LH5811 Input/Output Port Controller

    TODO:
    - serial data transfer
    - data transfer to the cassette tape

**********************************************************************/

#include "emu.h"
#include "machine/devhelpr.h"
#include "lh5810.h"

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type LH5810 = lh5810_device_config::static_alloc_device_config;


//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

GENERIC_DEVICE_CONFIG_SETUP(lh5810, "LH5810")


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void lh5810_device_config::device_config_complete()
{
	// inherit a copy of the static data
	const lh5810_interface *intf = reinterpret_cast<const lh5810_interface *>(static_config());
	if (intf != NULL)
		*static_cast<lh5810_interface *>(this) = *intf;

	// or initialize to defaults if none provided
	else
	{
		memset(&m_porta_r_func, 0, sizeof(m_porta_r_func));
		memset(&m_porta_w_func, 0, sizeof(m_porta_w_func));
		memset(&m_portb_r_func, 0, sizeof(m_portb_r_func));
		memset(&m_portb_w_func, 0, sizeof(m_portb_w_func));
		memset(&m_portc_w_func, 0, sizeof(m_portc_w_func));
		memset(&m_out_int_func, 0, sizeof(m_out_int_func));
	}
}


//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  lh5810_device - constructor
//-------------------------------------------------

lh5810_device::lh5810_device(running_machine &_machine, const lh5810_device_config &config)
    : device_t(_machine, config),
      m_config(config)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void lh5810_device::device_start()
{
	// resolve callbacks
	devcb_resolve_read8(&m_porta_r_func, &m_config.m_porta_r_func, this);
	devcb_resolve_write8(&m_porta_w_func, &m_config.m_porta_w_func, this);
	devcb_resolve_read8(&m_portb_r_func, &m_config.m_portb_r_func, this);
	devcb_resolve_write8(&m_portb_w_func, &m_config.m_portb_w_func, this);
	devcb_resolve_write8(&m_portc_w_func, &m_config.m_portc_w_func, this);
	devcb_resolve_write_line(&m_out_int_func, &m_config.m_out_int_func, this);

	// register for state saving
	save_item(NAME(m_irq));
	save_item(NAME(m_reg));
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void lh5810_device::device_reset()
{
	memset(m_reg, 0, sizeof(m_reg));
	m_irq = 0;
}


//-------------------------------------------------
//  data_r - data read
//-------------------------------------------------

READ8_MEMBER( lh5810_device::data_r )
{
	switch (offset)
	{
		case LH5810_U:
		case LH5810_L:
		case LH5810_G:
		case LH5810_DDA:
		case LH5810_DDB:
		case LH5810_OPC:
		case LH5820_F:
			return m_reg[offset];

		case LH5810_IF:
			if (BIT(devcb_call_read8(&m_portb_r_func, 0) & ~m_reg[LH5810_DDB], 7))
				m_reg[offset] |= 2;
			else
				m_reg[offset] &= 0xfd;

			return m_reg[offset];

		case LH5810_MSK:
			return (m_reg[offset]&0x0f) | (m_irq<<4) | (BIT(m_reg[LH5810_OPB],7)<<5);

		case LH5810_OPA:
			m_reg[offset] = (m_reg[offset] & m_reg[LH5810_DDA]) | (devcb_call_read8(&m_porta_r_func, 0) & ~m_reg[LH5810_DDA]);
			return m_reg[offset];

		case LH5810_OPB:
			m_reg[offset] = (m_reg[offset] & m_reg[LH5810_DDB]) | (devcb_call_read8(&m_portb_r_func, 0) & ~m_reg[LH5810_DDB]);
			devcb_call_write_line(&m_out_int_func, (m_reg[offset] & 0x80 && m_reg[LH5810_MSK] & 0x02) ? ASSERT_LINE : CLEAR_LINE);
			return m_reg[offset];

		default:
			return 0x00;
	}
}


//-------------------------------------------------
//  data_w - data write
//-------------------------------------------------

WRITE8_MEMBER( lh5810_device::data_w )
{
	switch (offset)
	{
		case LH5810_RESET:
			break;

		case LH5810_G:
		case LH5820_F:
		case LH5810_DDA:
		case LH5810_DDB:
			m_reg[offset] = data;
			break;

		case LH5810_U:
			//writing on U register clear the RD flag of IF register
			m_reg[LH5810_IF] &= 0xfb;
			m_reg[offset] = data;
			break;

		case LH5810_L:
			//writing on L register clear the TD flag of IF register
			m_reg[LH5810_IF] &= 0xf7;
			m_reg[offset] = data;
			break;

		case LH5810_MSK:
			m_reg[offset] = data & 0x0f;
			break;

		case LH5810_IF:
			//only bit 0 and 1 are writable
			m_reg[offset] = (m_reg[offset] & 0xfc) | (data & 0x03);
			break;

		case LH5810_OPA:
			m_reg[offset] = (data & m_reg[LH5810_DDA]) | (m_reg[offset] & ~m_reg[LH5810_DDA]);
			devcb_call_write8(&m_porta_w_func, 0, m_reg[offset]);
			break;

		case LH5810_OPB:
			m_reg[offset] = (data & m_reg[LH5810_DDB]) | (m_reg[offset] & ~m_reg[LH5810_DDB]);
			devcb_call_write8(&m_portb_w_func, 0, m_reg[offset]);
			devcb_call_write_line(&m_out_int_func, (m_reg[offset] & 0x80 && m_reg[LH5810_MSK] & 0x02) ? ASSERT_LINE : CLEAR_LINE);
			break;

		case LH5810_OPC:
			m_reg[offset] = data;
			devcb_call_write8(&m_portc_w_func, 0, m_reg[offset]);
			break;
	}
}

