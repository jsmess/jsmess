/**********************************************************************

    8279

2012-01-08 First draft [Robbbert]

Notes:
- All keys MUST be ACTIVE_LOW
- The data sheet doesn't say everything, so a lot of assumptions have been made.

ToDo:
- Command 5 (Nibble masking and blanking)
- Command 7 (Error Mode)
- Interrupts
- BD pin
- Sensor ram stuff
- save state
- Keyboard isn't working properly

**********************************************************************/

#include "i8279.h"
#include "machine/devhelpr.h"


//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

// device type definition
const device_type I8279 = &device_creator<i8279_device>;

//-------------------------------------------------
//  i8279_device - constructor
//-------------------------------------------------

i8279_device::i8279_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, I8279, "Intel 8279", tag, owner, clock)
{
}

//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void i8279_device::device_config_complete()
{
	// inherit a copy of the static data
	const i8279_interface *intf = reinterpret_cast<const i8279_interface *>(static_config());
	if (intf != NULL)
	{
		*static_cast<i8279_interface *>(this) = *intf;
	}

	// or initialize to defaults if none provided
	else
	{
    	memset(&m_out_irq_cb, 0, sizeof(m_out_irq_cb));
    	memset(&m_out_sl_cb, 0, sizeof(m_out_sl_cb));
    	memset(&m_out_disp_cb, 0, sizeof(m_out_disp_cb));
    	memset(&m_out_bd_cb, 0, sizeof(m_out_bd_cb));
    	memset(&m_in_rl_cb, 0, sizeof(m_in_rl_cb));
    	memset(&m_in_shift_cb, 0, sizeof(m_in_shift_cb));
    	memset(&m_in_ctrl_cb, 0, sizeof(m_in_ctrl_cb));
	}
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void i8279_device::device_start()
{
	/* resolve callbacks */
	m_out_irq_func.resolve(m_out_irq_cb, *this);
	m_out_sl_func.resolve(m_out_sl_cb, *this);
	m_out_disp_func.resolve(m_out_disp_cb, *this);
	m_out_bd_func.resolve(m_out_bd_cb, *this);
	m_in_rl_func.resolve(m_in_rl_cb, *this);
	m_in_shift_func.resolve(m_in_shift_cb, *this);
	m_in_ctrl_func.resolve(m_in_ctrl_cb, *this);
	m_clock = clock();
//  m_p_ram = machine().region("i8279")->base();
//  m_ctrls = m_p_ram + 0x10;
//  m_d_ram = m_p_ram + 0x20;
//  m_s_ram = m_p_ram + 0x40;
//  m_fifo = m_p_ram + 0x60;
	m_timer = machine().scheduler().timer_alloc(FUNC(timerproc_callback), (void *)this);
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void i8279_device::device_reset()
{
	UINT8 i;

	// startup values are unknown: setting to 0
	for (i = 2; i < 8; i++) m_ctrls[i] = 0;
	for (i = 0; i < 8; i++) m_fifo[i] = 0;
	for (i = 0; i < 8; i++) m_s_ram[i] = 0;
	for (i = 0; i < 16; i++) m_d_ram[i] = 0;
	m_status = 0;
	m_autoinc = 1;
	m_d_ram_ptr = 0;
	m_s_ram_ptr = 0;
	m_read_flag = 0;
	m_scanner = 0;
	m_ctrl_key = 1;
	m_key_down = 0xff;

	// from here is confirmed
	m_ctrls[0] = 8;
	m_ctrls[1] = 31;
	timer_adjust();
}


void i8279_device::timer_adjust()
{
	UINT8 divider = (m_ctrls[1]) ? m_ctrls[1] : 1;
	m_clock = clock() / divider;
	m_timer->adjust(attotime::from_hz(m_clock), 0, attotime::from_hz(m_clock));
}


void i8279_device::clear_display()
{
	// clear all digits
	UINT8 i,patterns[4] = { 0, 0, 0x20, 0xff };
	UINT8 data = patterns[(m_ctrls[6] & 12) >> 2];

	// The CD high bit (also done by CA)
	if (m_ctrls[6] & 11)
		for (i = 0; i < 16; i++)
			m_d_ram[i] = data;

	m_status &= 0x7f; // bit 7 not emulated, but do it anyway
	m_d_ram_ptr = 0; // not in the datasheet, but needed

	// The CF bit (also done by CA)
	if (m_ctrls[6] & 3)
	{
		m_status &= 0xc0; // blow away fifo
		m_s_ram_ptr = 0; // reset sensor pointer
		set_irq(0); // reset irq
	}
}


void i8279_device::set_irq(bool state)
{
	if ( !m_out_irq_func.isnull() )
		m_out_irq_func( state );
}


void i8279_device::new_key(UINT8 data, bool skey, bool ckey)
{
	UINT8 i, rl, sl;
	for (i = 0; BIT(data, i); i++);
	rl = i;
	if (BIT(m_ctrls[0], 0))
	{
		for (i = 0; !BIT(data, i); i++);
		sl = i;
	}
	else
		sl = m_scanner;

	new_fifo( (ckey << 7) | (skey << 6) | (sl << 3) | rl);
}


void i8279_device::new_fifo(UINT8 data)
{
	// see if key still down from last time
	UINT16 key_down = (m_scanner << 8) | data;
	if (key_down == m_key_down)
		return;
	m_key_down = key_down;

	// see if already overrun
	if (BIT(m_status, 5))
		return;

	// set overrun flag if full
	if (BIT(m_status, 3))
	{
		m_status |= 0x20;
		return;
	}

	m_fifo[m_status & 7] = data;

	// bump fifo size & turn off underrun
	UINT8 fifo_size = m_status & 7;
	if ((fifo_size)==7)
		m_status |= 8; // full
	else
		m_status = (m_status & 0xe8) + fifo_size + 1;

	if (!fifo_size)
		set_irq(1); // something just went into fifo, so int
}


TIMER_CALLBACK( i8279_device::timerproc_callback )
{
	reinterpret_cast<i8279_device*>(ptr)->timer_mainloop();
}


void i8279_device::timer_mainloop()
{
	// control byte 0
	// bit 0 - encoded or decoded keyboard scan
	// bits 1,2 - keyboard type
	// bit 3 - number of digits to display
	// bit 4 - left or right entry

	UINT8 scanner_mask = 15; //BIT(m_ctrls[0], 0) ? 15 : BIT(m_ctrls[0], 3) ? 15 : 7;
	bool decoded = BIT(m_ctrls[0], 0);
	UINT8 kbd_type = (m_ctrls[0] & 6) >> 1;
	bool shift_key = 1;
	bool ctrl_key = 1;
	bool strobe_pulse = 0;

	// keyboard
	// type 0 = kbd, 2-key lockout
	// type 1 = kdb, n-key
	// type 2 = sensor
	// type 3 = strobed

	// Get shift keys
	if ( !m_in_shift_func.isnull() )
		shift_key = m_in_shift_func();

	if ( !m_in_ctrl_func.isnull() )
		ctrl_key = m_in_ctrl_func();

	if (ctrl_key && !m_ctrl_key)
		strobe_pulse = 1; // low-to-high is a strobe

	m_ctrl_key = ctrl_key;

	// Read a row of keys

	if ( !m_in_rl_func.isnull() )
	{
		UINT8 rl = m_in_rl_func(0);
		if (rl < 0xff)
		{
			switch (kbd_type)
			{
				case 0:
				case 1:
					new_key(rl, shift_key, ctrl_key);
					break;
				case 2:
					new_fifo(rl);
					break;
				case 3:
					if (strobe_pulse) new_fifo(rl);
					break;
			}
		}
		else
			m_key_down = 0xff;
	}

	// Increment scanline

	if (decoded)
	{
		m_scanner<<= 1;
		if ((m_scanner & 15)==0)
			m_scanner = 1;
	}
	else
		m_scanner++;

	m_scanner &= scanner_mask; // 4-bit port

	if ( !m_out_sl_func.isnull() )
		m_out_sl_func(0, m_scanner);

	// output a digit

	if ( !m_out_disp_func.isnull() )
		m_out_disp_func(0, m_d_ram[m_scanner] );
}


UINT8 i8279_device::status_r()
{
	return m_status;
}


UINT8 i8279_device::data_r()
{
	UINT8 i;
	bool sensor_mode = ((m_ctrls[0] & 6)==4);
	UINT8 data;
	if (m_read_flag)
	{
	// read the display ram
		data = m_d_ram[m_d_ram_ptr];
		if (m_autoinc)
			m_d_ram_ptr++;
	}
	else
	if (sensor_mode)
	{
	// read sensor ram
		data = m_s_ram[m_s_ram_ptr];
		if (m_autoinc)
			m_s_ram_ptr++;
	}
	else
	{
	// read a key from fifo
		data = m_fifo[0];
		UINT8 fifo_size = m_status & 7;
		switch (m_status & 0x38)
		{
			case 0x00: // no errors
				if (!fifo_size)
					m_status |= 0x10; // underrun
				else
				{
					for (i = 1; i < 8; i++)
						m_fifo[i-1] = m_fifo[i];
					fifo_size--;
					if (!fifo_size)
						set_irq(0);
				}
				break;
			case 0x28: // overrun
			case 0x08: // fifo full
				for (i = 1; i < 8; i++)
					m_fifo[i-1] = m_fifo[i];
				break;
			case 0x10: // underrun
				if (!fifo_size)
					break;
			default:
				printf("Invalid status: %X\n", m_status);
		}
		m_status = (m_status & 0xd0) | fifo_size; // turn off overrun & full
	}

	m_d_ram_ptr &= 15;
	m_s_ram_ptr &= 7;
	return data;
}


void i8279_device::ctrl_w( UINT8 data )
{//printf("Command: %X=%X ",data>>5,data&31);
	UINT8 cmd = data >> 5;
	data &= 0x1f;
	m_ctrls[cmd] = data;
	switch (cmd)
	{
		case 1:
			if (data > 1)
				timer_adjust();
			break;
		case 2:
			m_read_flag = 0;
			if ((m_ctrls[0] & 6)==4) // sensor mode only
			{
				m_autoinc = BIT(data, 4);
				m_s_ram_ptr = data & 7;
			}
			break;
		case 3:
			m_read_flag = 1;
			m_d_ram_ptr = data & 15;
			m_autoinc = BIT(data, 4);
			break;
		case 4:
			m_d_ram_ptr = data & 15;
			m_autoinc = BIT(data, 4);
			break;
		case 6:
			clear_display();
			break;
	}
}


void i8279_device::data_w( UINT8 data )
{//printf("Data: %X ",data);
	if (BIT(m_ctrls[0], 4))
	{
	}
	else
	{
		m_d_ram[m_d_ram_ptr] = data;
		if (m_autoinc)
			m_d_ram_ptr++;
	}
	m_d_ram_ptr &= 15;
}


// These 2 are only to be used if the port numbers are sequential
READ8_MEMBER( i8279_device::read )
{
	return BIT(offset, 0) ? data_r() : status_r();
}


WRITE8_MEMBER( i8279_device::write)
{
	BIT(offset, 0) ? data_w(data) : ctrl_w(data);
}


/***************************************************************************
    TRAMPOLINES
***************************************************************************/

WRITE8_MEMBER( i8279_device::i8279_ctrl_w ) { ctrl_w(data); }
WRITE8_MEMBER( i8279_device::i8279_data_w ) { data_w(data); }
READ8_MEMBER( i8279_device::i8279_status_r ) { return status_r(); }
READ8_MEMBER( i8279_device::i8279_data_r ) { return data_r(); }
