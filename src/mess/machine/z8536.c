/**********************************************************************

    Zilog Z8536 Counter/Timer and Parallel I/O emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "emu.h"
#include "z8536.h"
#include "machine/devhelpr.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

// states
enum
{
	STATE_RESET = -1,
	STATE_0,
	STATE_1
};


// ports
enum
{
	PORT_C = 0,
	PORT_B,
	PORT_A,
	CONTROL
};


// registers
enum
{
	MASTER_INTERRUPT_CONTROL = 0,
	MASTER_CONFIGURATION_CONTROL,
	PORT_A_INTERRUPT_VECTOR,
	PORT_B_INTERRUPT_VECTOR,
	COUNTER_TIMER_INTERRUPT_VECTOR,
	PORT_C_DATA_PATH_POLARITY,
	PORT_C_DATA_DIRECTION,
	PORT_C_SPECIAL_IO_CONTROL,
	PORT_A_COMMAND_AND_STATUS,
	PORT_B_COMMAND_AND_STATUS,
	COUNTER_TIMER_1_COMMAND_AND_STATUS,
	COUNTER_TIMER_2_COMMAND_AND_STATUS,
	COUNTER_TIMER_3_COMMAND_AND_STATUS,
	PORT_A_DATA,
	PORT_B_DATA,
	PORT_C_DATA,
	COUNTER_TIMER_1_CURRENT_COUNT_MS_BYTE,
	COUNTER_TIMER_1_CURRENT_COUNT_LS_BYTE,
	COUNTER_TIMER_2_CURRENT_COUNT_MS_BYTE,
	COUNTER_TIMER_2_CURRENT_COUNT_LS_BYTE,
	COUNTER_TIMER_3_CURRENT_COUNT_MS_BYTE,
	COUNTER_TIMER_3_CURRENT_COUNT_LS_BYTE,
	COUNTER_TIMER_1_TIME_CONSTANT_MS_BYTE,
	COUNTER_TIMER_1_TIME_CONSTANT_LS_BYTE,
	COUNTER_TIMER_2_TIME_CONSTANT_MS_BYTE,
	COUNTER_TIMER_2_TIME_CONSTANT_LS_BYTE,
	COUNTER_TIMER_3_TIME_CONSTANT_MS_BYTE,
	COUNTER_TIMER_3_TIME_CONSTANT_LS_BYTE,
	COUNTER_TIMER_1_MODE_SPECIFICATION,
	COUNTER_TIMER_2_MODE_SPECIFICATION,
	COUNTER_TIMER_3_MODE_SPECIFICATION,
	CURRENT_VECTOR,
	PORT_A_MODE_SPECIFICATION,
	PORT_A_HANDSHAKE_SPECIFICATION,
	PORT_A_DATA_PATH_POLARITY,
	PORT_A_DATA_DIRECTION,
	PORT_A_SPECIAL_IO_CONTROL,
	PORT_A_PATTERN_POLARITY,
	PORT_A_PATTERN_TRANSITION,
	PORT_A_PATTERN_MASK,
	PORT_B_MODE_SPECIFICATION,
	PORT_B_HANDSHAKE_SPECIFICATION,
	PORT_B_DATA_PATH_POLARITY,
	PORT_B_DATA_DIRECTION,
	PORT_B_SPECIAL_IO_CONTROL,
	PORT_B_PATTERN_POLARITY,
	PORT_B_PATTERN_TRANSITION,
	PORT_B_PATTERN_MASK
};


// master interrupt control register
#define MICR_RESET		0x01
#define MICR_RJA 		0x02
#define MICR_CT_VIS		0x04
#define MICR_PB_VIS		0x08
#define MICR_PA_VIS		0x10
#define MICR_NV 		0x20
#define MICR_DLC 		0x40
#define MICR_MIE 		0x80



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type Z8536 = z8536_device_config::static_alloc_device_config;



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

GENERIC_DEVICE_CONFIG_SETUP(z8536, "Zilog Z8536")


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void z8536_device_config::device_config_complete()
{
	// inherit a copy of the static data
	const z8536_interface *intf = reinterpret_cast<const z8536_interface *>(static_config());
	if (intf != NULL)
		*static_cast<z8536_interface *>(this) = *intf;

	// or initialize to defaults if none provided
	else
	{
		memset(&m_out_int_func, 0, sizeof(m_out_int_func));
		memset(&m_in_pa_func, 0, sizeof(m_in_pa_func));
		memset(&m_out_pa_func, 0, sizeof(m_out_pa_func));
		memset(&m_in_pb_func, 0, sizeof(m_in_pb_func));
		memset(&m_out_pb_func, 0, sizeof(m_out_pb_func));
		memset(&m_in_pc_func, 0, sizeof(m_in_pc_func));
		memset(&m_out_pc_func, 0, sizeof(m_out_pc_func));
	}
}



//**************************************************************************
//  INLINE HELPERS
//**************************************************************************

//-------------------------------------------------
//  read_register - read from register
//-------------------------------------------------

inline UINT8 z8536_device::read_register(offs_t offset)
{
	UINT8 data = 0;
	
	data = m_register[offset]; // HACK

	switch (offset)
	{
	case PORT_A_DATA:
		data = devcb_call_read8(&m_in_pa_func, 0);
		break;
		
	case PORT_B_DATA:
		data = devcb_call_read8(&m_in_pb_func, 0);
		break;
		
	case PORT_C_DATA:
		data = devcb_call_read8(&m_in_pc_func, 0) & 0x0f;
		break;
		
	default:
		logerror("Z8536 '%s' Unimplemented read from register %u\n", tag(), offset);
	}
	
	return data;
}


//-------------------------------------------------
//  read_register - masked read from register
//-------------------------------------------------

inline UINT8 z8536_device::read_register(offs_t offset, UINT8 mask)
{
	return read_register(offset) & mask;
}


//-------------------------------------------------
//  write_register - write to register
//-------------------------------------------------

inline void z8536_device::write_register(offs_t offset, UINT8 data)
{
	switch (offset)
	{
	case MASTER_INTERRUPT_CONTROL:
		if (data & MICR_RESET)
		{
			device_reset();
		}
		else if (m_state == STATE_RESET)
		{
			m_state = STATE_0;
		}
		break;
		
	case PORT_A_DATA:
		devcb_call_write8(&m_out_pa_func, 0, data);
		break;
		
	case PORT_B_DATA:
		devcb_call_write8(&m_out_pb_func, 0, data);
		break;
		
	case PORT_C_DATA:
		devcb_call_write8(&m_out_pc_func, 0, data & 0x0f);
		break;
		
	default:
		logerror("Z8536 '%s' Unimplemented write %02x to register %u\n", tag(), data, offset);
	}
	
	m_register[offset] = data; // HACK
}


//-------------------------------------------------
//  write_register - masked write to register
//-------------------------------------------------

inline void z8536_device::write_register(offs_t offset, UINT8 data, UINT8 mask)
{
	UINT8 combined_data = (data & mask) | (m_register[offset] & (mask ^ 0xff));
	
	write_register(offset, combined_data);
}


//-------------------------------------------------
//   count - 
//-------------------------------------------------

inline void z8536_device::count(device_timer_id id)
{
}


//-------------------------------------------------
//  trigger - 
//-------------------------------------------------

inline void z8536_device::trigger(device_timer_id id)
{
}


//-------------------------------------------------
//  gate - 
//-------------------------------------------------

inline void z8536_device::gate(device_timer_id id)
{
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  z8536_device - constructor
//-------------------------------------------------

z8536_device::z8536_device(running_machine &_machine, const z8536_device_config &config)
    : device_t(_machine, config),
      m_config(config)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void z8536_device::device_start()
{
	// allocate timers
	m_timer[TIMER_1] = timer_alloc(TIMER_1);
	m_timer[TIMER_2] = timer_alloc(TIMER_2);
	m_timer[TIMER_3] = timer_alloc(TIMER_3);
		
	// resolve callbacks
	devcb_resolve_write_line(&m_out_int_func, &m_config.m_out_int_func, this);
	devcb_resolve_read8(&m_in_pa_func, &m_config.m_in_pa_func, this);
	devcb_resolve_write8(&m_out_pa_func, &m_config.m_out_pa_func, this);
	devcb_resolve_read8(&m_in_pb_func, &m_config.m_in_pb_func, this);
	devcb_resolve_write8(&m_out_pb_func, &m_config.m_out_pb_func, this);
	devcb_resolve_read8(&m_in_pc_func, &m_config.m_in_pc_func, this);
	devcb_resolve_write8(&m_out_pc_func, &m_config.m_out_pc_func, this);
}


//-------------------------------------------------
//  device_start - device-specific reset
//-------------------------------------------------

void z8536_device::device_reset()
{
	m_state = STATE_RESET;
	
	for (int i = 0; i < 48; i++)
	{
		m_register[i] = 0;
	}
	
	m_register[MASTER_INTERRUPT_CONTROL] = MICR_RESET;
	m_pointer = 0;
}


//-------------------------------------------------
//  device_timer - handler timer events
//-------------------------------------------------

void z8536_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	switch (id)
	{
	case TIMER_1:
		count(TIMER_1);
		break;
		
	case TIMER_2:
		count(TIMER_2);
		break;
		
	case TIMER_3:
		count(TIMER_3);
		break;
	}
}


//-------------------------------------------------
//  read - register read
//-------------------------------------------------

READ8_MEMBER( z8536_device::read )
{
	UINT8 data = 0;
	
	switch (offset & 0x03)
	{
	case PORT_C:
		data = read_register(PORT_C_DATA);
		break;

	case PORT_B:
		data = read_register(PORT_B_DATA);
		break;

	case PORT_A:
		data = read_register(PORT_A_DATA);
		break;

	case CONTROL:
		switch (m_state)
		{
		case STATE_RESET:
			// read RESET bit
			data = read_register(m_pointer, 0x01);
			break;

		case STATE_1:
			m_state = STATE_0;
			// fallthru
		case STATE_0:
			data = read_register(m_pointer);
			break;
		}
		break;
	}
	
	return data;
}


//-------------------------------------------------
//  write - register write
//-------------------------------------------------

WRITE8_MEMBER( z8536_device::write )
{
	switch (offset & 0x03)
	{
	case PORT_C:
		write_register(PORT_C_DATA, data);
		break;

	case PORT_B:
		write_register(PORT_B_DATA, data);
		break;

	case PORT_A:
		write_register(PORT_A_DATA, data);
		break;

	case CONTROL:
		switch (m_state)
		{
		case STATE_RESET:
			// write RESET bit
			write_register(m_pointer, data, 0x01);
			break;
			
		case STATE_0:
			m_pointer = data;
			m_state = STATE_1;
			break;

		case STATE_1:
			write_register(m_pointer, data);
			m_state = STATE_0;
		}
		break;
	}
}


//-------------------------------------------------
//  intack_w - interrupt acknowledge
//-------------------------------------------------

WRITE_LINE_MEMBER( z8536_device::intack_w )
{
}


//-------------------------------------------------
//  pb*_w - port B bits 0-7 write
//-------------------------------------------------

WRITE_LINE_MEMBER( z8536_device::pb0_w ) { }
WRITE_LINE_MEMBER( z8536_device::pb1_w ) { }
WRITE_LINE_MEMBER( z8536_device::pb2_w ) { }
WRITE_LINE_MEMBER( z8536_device::pb3_w ) { }
WRITE_LINE_MEMBER( z8536_device::pb4_w ) { }
WRITE_LINE_MEMBER( z8536_device::pb5_w ) { }
WRITE_LINE_MEMBER( z8536_device::pb6_w ) { }
WRITE_LINE_MEMBER( z8536_device::pb7_w ) { }


//-------------------------------------------------
//  pc*_w - port C bits 0-3 write
//-------------------------------------------------

WRITE_LINE_MEMBER( z8536_device::pc0_w ) { }
WRITE_LINE_MEMBER( z8536_device::pc1_w ) { }
WRITE_LINE_MEMBER( z8536_device::pc2_w ) { }
WRITE_LINE_MEMBER( z8536_device::pc3_w ) { }
