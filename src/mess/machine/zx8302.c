/**********************************************************************

    Sinclair ZX8302 emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

	TODO:

	- set time
	- read from microdrive
	- write to microdrive
	- DTR/CTS handling
	- network

*/

#include "emu.h"
#include <time.h>
#include "devices/microdrv.h"
#include "machine/devhelpr.h"
#include "zx8302.h"



//**************************************************************************
//	MACROS / CONSTANTS
//**************************************************************************

#define LOG 0


// IPC serial state
enum
{
	IPC_START,
	IPC_DATA,
	IPC_STOP
};


// baud rate
static const int BAUD_19200				= 0x00;
static const int BAUD_9600				= 0x01;
static const int BAUD_4800				= 0x02;
static const int BAUD_2400				= 0x03;
static const int BAUD_1200				= 0x04;
static const int BAUD_600				= 0x05;
static const int BAUD_300				= 0x06;
static const int BAUD_75				= 0x07;
static const int BAUD_MASK				= 0x07;


// transmit mode
static const int MODE_SER1				= 0x00;
static const int MODE_SER2				= 0x08;
static const int MODE_MDV				= 0x10;
static const int MODE_NET				= 0x18;
static const int MODE_MASK				= 0x18;


// interrupts
static const int INT_GAP				= 0x01;
static const int INT_INTERFACE			= 0x02;
static const int INT_TRANSMIT			= 0x04;
static const int INT_FRAME				= 0x08;
static const int INT_EXTERNAL			= 0x10;


// status register
static const int STATUS_NETWORK_PORT	= 0x01;
static const int STATUS_TX_BUFFER_FULL	= 0x02;
static const int STATUS_RX_BUFFER_FULL	= 0x04;
static const int STATUS_MICRODRIVE_GAP	= 0x08;


// transmit bits
static const int TXD_START				= 0;
static const int TXD_STOP				= 9;
static const int TXD_STOP2				= 10;


// Monday 1st January 1979 00:00:00 UTC
static const int RTC_BASE_ADJUST		= 283996800;



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type ZX8302 = zx8302_device_config::static_alloc_device_config;



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

GENERIC_DEVICE_CONFIG_SETUP(zx8302, "Sinclair ZX8302")


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void zx8302_device_config::device_config_complete()
{
	// inherit a copy of the static data
	const zx8302_interface *intf = reinterpret_cast<const zx8302_interface *>(static_config());
	if (intf != NULL)
		*static_cast<zx8302_interface *>(this) = *intf;

	// or initialize to defaults if none provided
	else
	{
		memset(&out_ipl1l_func, 0, sizeof(out_ipl1l_func));
		memset(&out_baudx4_func, 0, sizeof(out_baudx4_func));
		memset(&out_comdata_func, 0, sizeof(out_comdata_func));
		memset(&out_txd1_func, 0, sizeof(out_txd1_func));
		memset(&out_txd2_func, 0, sizeof(out_txd2_func));
		memset(&in_dtr1_func, 0, sizeof(in_dtr1_func));
		memset(&in_cts2_func, 0, sizeof(in_cts2_func));
		memset(&out_netout_func, 0, sizeof(out_netout_func));
		memset(&in_netin_func, 0, sizeof(in_netin_func));
	}
}



//**************************************************************************
//  INLINE HELPERS
//**************************************************************************

//-------------------------------------------------
//  trigger_interrupt - 
//-------------------------------------------------

inline void zx8302_device::trigger_interrupt(UINT8 line)
{
	m_irq |= line;

	devcb_call_write_line(&m_out_ipl1l_func, ASSERT_LINE);
}


//-------------------------------------------------
//  transmit_ipc_data - transmit IPC data
//-------------------------------------------------

inline void zx8302_device::transmit_ipc_data()
{
	/*

        IPC <-> ZX8302 serial link protocol
        ***********************************

        Send bit to IPC
        ---------------

        1. ZX start bit (COMDATA = 0)
        2. IPC clock (COMCTL = 0, COMTL = 1)
        3. ZX data bit (COMDATA = 0/1)
        4. IPC clock (COMCTL = 0, COMTL = 1)
        5. ZX stop bit (COMDATA = 1)

        Receive bit from IPC
        --------------------

        1. ZX start bit (COMDATA = 0)
        2. IPC clock (COMCTL = 0, COMTL = 1)
        3. IPC data bit (COMDATA = 0/1)
        4. IPC clock (COMCTL = 0, COMTL = 1)
        5. IPC stop bit (COMDATA = 1)

    */

	switch (m_ipc_state)
	{
	case IPC_START:
		if (LOG) logerror("ZX8302 '%s' COMDATA Start Bit\n", tag());

		devcb_call_write_line(&m_out_comdata_func, 0);
		m_ipc_busy = 1;
		m_ipc_state = IPC_DATA;
		break;

	case IPC_DATA:
		if (LOG) logerror("ZX8302 '%s' COMDATA Data Bit: %x\n", tag(), BIT(m_idr, 1));

		m_comdata = BIT(m_idr, 1);
		devcb_call_write_line(&m_out_comdata_func, m_comdata);
		m_ipc_state = IPC_STOP;
		break;

	case IPC_STOP:
		if (!m_ipc_rx)
		{
			if (LOG) logerror("ZX8302 '%s' COMDATA Stop Bit\n", tag());

			devcb_call_write_line(&m_out_comdata_func, 1);
			m_ipc_busy = 0;
			m_ipc_state = IPC_START;
		}
		break;
	}
}


//-------------------------------------------------
//  transmit_bit - transmit serial bit
//-------------------------------------------------

inline void zx8302_device::transmit_bit(int state)
{
	switch (m_tcr & MODE_MASK)
	{
	case MODE_SER1:
		devcb_call_write_line(&m_out_txd1_func, state);
		break;

	case MODE_SER2:
		devcb_call_write_line(&m_out_txd2_func, state);
		break;

	case MODE_MDV:
		// TODO
		break;

	case MODE_NET:
		devcb_call_write_line(&m_out_netout_func, state);
		break;
	}
}


//-------------------------------------------------
//  transmit_data - transmit serial data
//-------------------------------------------------

inline void zx8302_device::transmit_serial_data()
{
	switch (m_tx_bits)
	{
	case TXD_START:
		if (!(m_irq & INT_TRANSMIT))
		{
			transmit_bit(0);
			m_tx_bits++;
		}
		break;

	default:
		transmit_bit(BIT(m_tdr, 0));
		m_tdr >>= 1;
		m_tx_bits++;
		break;

	case TXD_STOP:
		transmit_bit(1);
		m_tx_bits++;
		break;

	case TXD_STOP2:
		transmit_bit(1);
		m_tx_bits = TXD_START;
		m_status &= ~STATUS_TX_BUFFER_FULL;
		trigger_interrupt(INT_TRANSMIT);
		break;
	}
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  zx8302_device - constructor
//-------------------------------------------------

zx8302_device::zx8302_device(running_machine &_machine, const zx8302_device_config &config)
    : device_t(_machine, config),
	  m_idr(1),
	  m_irq(0),
	  m_ctr(time(NULL) + RTC_BASE_ADJUST),
	  m_status(0),
	  m_comdata(1),
	  m_comctl(1),
	  m_ipc_state(0),
	  m_ipc_rx(0),
	  m_ipc_busy(0),
	  m_track(0),
      m_config(config)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void zx8302_device::device_start()
{
	// resolve callbacks
	devcb_resolve_write_line(&m_out_ipl1l_func, &m_config.out_ipl1l_func, this);
	devcb_resolve_write_line(&m_out_baudx4_func, &m_config.out_baudx4_func, this);
	devcb_resolve_write_line(&m_out_comdata_func, &m_config.out_comdata_func, this);
	devcb_resolve_write_line(&m_out_txd1_func, &m_config.out_txd1_func, this);
	devcb_resolve_write_line(&m_out_txd2_func, &m_config.out_txd2_func, this);
	devcb_resolve_read_line(&m_in_dtr1_func, &m_config.in_dtr1_func, this);
	devcb_resolve_read_line(&m_in_cts2_func, &m_config.in_cts2_func, this);
	devcb_resolve_write_line(&m_out_netout_func, &m_config.out_netout_func, this);
	devcb_resolve_read_line(&m_in_netin_func, &m_config.in_netin_func, this);

	// allocate timers
	m_txd_timer = device_timer_alloc(*this, TIMER_TXD);
	m_baudx4_timer = device_timer_alloc(*this, TIMER_BAUDX4);
	m_rtc_timer = device_timer_alloc(*this, TIMER_RTC);
	m_gap_timer = device_timer_alloc(*this, TIMER_GAP);
	m_ipc_timer = device_timer_alloc(*this, TIMER_IPC);

	timer_adjust_periodic(m_rtc_timer, attotime_zero, 0, ATTOTIME_IN_HZ(m_config.rtc_clock / 32768));
	timer_adjust_periodic(m_gap_timer, attotime_zero, 0, ATTOTIME_IN_MSEC(31));

	// register for state saving
	state_save_register_device_item(this, 0, m_idr);
	state_save_register_device_item(this, 0, m_tcr);
	state_save_register_device_item(this, 0, m_tdr);
	state_save_register_device_item(this, 0, m_irq);
	state_save_register_device_item(this, 0, m_ctr);
	state_save_register_device_item(this, 0, m_status);
	state_save_register_device_item(this, 0, m_comdata);
	state_save_register_device_item(this, 0, m_comctl);
	state_save_register_device_item(this, 0, m_ipc_state);
	state_save_register_device_item(this, 0, m_ipc_rx);
	state_save_register_device_item(this, 0, m_ipc_busy);
	state_save_register_device_item(this, 0, m_baudx4);
	state_save_register_device_item(this, 0, m_tx_bits);
	state_save_register_device_item(this, 0, m_mdrdw);
	state_save_register_device_item(this, 0, m_mdselck);
	state_save_register_device_item(this, 0, m_mdseld);
	state_save_register_device_item(this, 0, m_erase);
	state_save_register_device_item(this, 0, m_mdv_motor);
	state_save_register_device_item_array(this, 0, m_mdv_data);
	state_save_register_device_item(this, 0, m_track);
}


//-------------------------------------------------
//  device_timer - handler timer events
//-------------------------------------------------

void zx8302_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	switch (id)
	{
	case TIMER_TXD:
		transmit_serial_data();
		break;

	case TIMER_BAUDX4:
		m_baudx4 = !m_baudx4;
		devcb_call_write_line(&m_out_baudx4_func, m_baudx4);
		break;

	case TIMER_RTC:
		m_ctr++;
		break;

	case TIMER_GAP:
		if (m_mdv_motor)
		{
			trigger_interrupt(INT_GAP);
		}
		break;

	case TIMER_IPC:
		m_idr = param;
		m_ipc_state = IPC_START;
		m_ipc_rx = 0;

		transmit_ipc_data();
		break;
	}
}


//-------------------------------------------------
//  rtc_r - real time clock read
//-------------------------------------------------

READ8_MEMBER( zx8302_device::rtc_r )
{
	UINT8 data = 0;

	switch (offset)
	{
	case 0:
		data = (m_ctr >> 24) & 0xff;
	case 1:
		data = (m_ctr >> 16) & 0xff;
	case 2:
		data = (m_ctr >> 8) & 0xff;
	case 3:
		data = m_ctr & 0xff;
	}

	return data;
}


//-------------------------------------------------
//  rtc_w - real time clock write
//-------------------------------------------------

WRITE8_MEMBER( zx8302_device::rtc_w )
{
	if (LOG) logerror("ZX8302 '%s' Set Real Time Clock: %02x\n", tag(), data);
}


//-------------------------------------------------
//  control_w - serial transmit clock
//-------------------------------------------------

WRITE8_MEMBER( zx8302_device::control_w )
{
	if (LOG) logerror("ZX8302 '%s' Transmit Control: %02x\n", tag(), data);

	int baud = (19200 >> (data & BAUD_MASK));
	int baudx4 = baud * 4;

	m_tcr = data;

	timer_adjust_periodic(m_txd_timer, attotime_zero, 0, ATTOTIME_IN_HZ(baud));
	timer_adjust_periodic(m_baudx4_timer, attotime_zero, 0, ATTOTIME_IN_HZ(baudx4));
}


//-------------------------------------------------
//  mdv_track_r - microdrive track data
//-------------------------------------------------

READ8_MEMBER( zx8302_device::mdv_track_r )
{
	if (LOG) logerror("ZX8302 '%s' Microdrive Track %u: %02x\n", tag(), m_track, m_mdv_data[m_track]);

	UINT8 data = m_mdv_data[m_track];

	m_track = !m_track;

	return data;
}


//-------------------------------------------------
//  status_r - status register
//-------------------------------------------------

READ8_MEMBER( zx8302_device::status_r )
{
	/*

        bit     description

        0       Network port
        1       Transmit buffer full
        2       Receive buffer full
        3       Microdrive GAP
        4       SER1 DTR
        5       SER2 CTS
        6       IPC busy
        7       COMDATA

    */

	UINT8 data = 0;

	// TODO network port

	// serial status
	data |= m_status;

	// TODO microdrive GAP

	// data terminal ready
	data |= devcb_call_read_line(&m_in_dtr1_func) << 4;

	// clear to send
	data |= devcb_call_read_line(&m_in_cts2_func) << 5;

	// IPC busy
	data |= m_ipc_busy << 6;

	// COMDATA
	data |= m_comdata << 7;

	if (LOG) logerror("ZX8302 '%s' Status: %02x\n", tag(), data);

	return data;
}


//-------------------------------------------------
//  ipc_command_w - IPC command
//-------------------------------------------------

WRITE8_MEMBER( zx8302_device::ipc_command_w )
{
	if (LOG) logerror("ZX8302 '%s' IPC Command: %02x\n", tag(), data);

	if (data != 0x01)
	{
		timer_adjust_oneshot(m_ipc_timer, ATTOTIME_IN_NSEC(480), data);
	}
}


//-------------------------------------------------
//  mdv_control_w - microdrive control
//-------------------------------------------------

WRITE8_MEMBER( zx8302_device::mdv_control_w )
{
	/*

        bit     description

        0       MDSELDH
        1       MDSELCKH
        2       MDRDWL
        3       ERASE
        4
        5
        6
        7

    */

	if (LOG) logerror("ZX8302 '%s' Microdrive Control: %02x\n", tag(), data);

	m_mdseld = BIT(data, 0);
	m_mdselck = BIT(data, 1);
	m_mdrdw = BIT(data, 2) ? 0 : 1;
	m_erase = BIT(data, 3);

	// Microdrive selection shift register
	if (m_mdselck)
	{
		m_mdv_motor >>= 1;
		m_mdv_motor |= (m_mdseld << 7);

		m_status &= ~STATUS_RX_BUFFER_FULL;
	}
}


//-------------------------------------------------
//  irq_status_r - interrupt status
//-------------------------------------------------

READ8_MEMBER( zx8302_device::irq_status_r )
{
	if (LOG) logerror("ZX8302 '%s' Interrupt Status: %02x\n", tag(), m_irq);

	return m_irq;
}


//-------------------------------------------------
//  irq_acknowledge_w - interrupt acknowledge
//-------------------------------------------------

WRITE8_MEMBER( zx8302_device::irq_acknowledge_w )
{
	if (LOG) logerror("ZX8302 '%s' Interrupt Acknowledge: %02x\n", tag(), data);

	m_irq &= ~data;

	if (!m_irq)
	{
		devcb_call_write_line(&m_out_ipl1l_func, CLEAR_LINE);
	}
}


//-------------------------------------------------
//  data_w - transmit buffer
//-------------------------------------------------

WRITE8_MEMBER( zx8302_device::data_w )
{
	if (LOG) logerror("ZX8302 '%s' Data Register: %02x\n", tag(), data);

	m_tdr = data;
	m_status |= STATUS_TX_BUFFER_FULL;
}


//-------------------------------------------------
//  vsync_w - vertical sync
//-------------------------------------------------

WRITE_LINE_MEMBER( zx8302_device::vsync_w )
{
	if (state)
	{
		if (LOG) logerror("ZX8302 '%s' Frame Interrupt\n", tag());

		trigger_interrupt(INT_FRAME);
	}
}


//-------------------------------------------------
//  comctl_w - IPC COMCTL
//-------------------------------------------------

WRITE_LINE_MEMBER( zx8302_device::comctl_w )
{
	if (LOG) logerror("ZX8302 '%s' COMCTL: %x\n", tag(), state);

	if (state)
	{
		transmit_ipc_data();
	}
}


//-------------------------------------------------
//  comdata_w - IPC COMDATA
//-------------------------------------------------

WRITE_LINE_MEMBER( zx8302_device::comdata_w )
{
	if (LOG) logerror("ZX8302 '%s' COMDATA: %x\n", tag(), state);

	if (m_ipc_state == IPC_DATA || m_ipc_state == IPC_STOP)
	{
		if (m_ipc_rx)
		{
			m_ipc_rx = 0;
			m_ipc_busy = 0;
			m_ipc_state = IPC_START;
		}
		else
		{
			m_ipc_rx = 1;
			m_comdata = state;
		}
	}
}


//-------------------------------------------------
//  extint_w - external interrupt
//-------------------------------------------------

WRITE_LINE_MEMBER( zx8302_device::extint_w )
{
	if (LOG) logerror("ZX8302 '%s' EXTINT: %x\n", tag(), state);

	if (state == ASSERT_LINE)
	{
		trigger_interrupt(INT_EXTERNAL);
	}
}
