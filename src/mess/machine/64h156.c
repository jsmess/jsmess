/**********************************************************************

    Commodore 64H156 Gate Array emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

	TODO:

	- serial IEC protocol handling
	- write to disk image

*/

#include "emu.h"
#include "64h156.h"
#include "formats/g64_dsk.h"
#include "machine/devhelpr.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define SYNC \
	!(m_oe && ((m_data & G64_SYNC_MARK) == G64_SYNC_MARK))

#define BYTE \
	!(m_soe & m_byte)



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type C64H156 = c64h156_device_config::static_alloc_device_config;



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

GENERIC_DEVICE_CONFIG_SETUP(c64h156, "64H156");


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void c64h156_device_config::device_config_complete()
{
	// inherit a copy of the static data
	const c64h156_interface *intf = reinterpret_cast<const c64h156_interface *>(static_config());
	if (intf != NULL)
		*static_cast<c64h156_interface *>(this) = *intf;

	// or initialize to defaults if none provided
	else
	{
		memset(&m_out_atn_func, 0, sizeof(m_out_atn_func));
		memset(&m_out_sync_func, 0, sizeof(m_out_sync_func));
		memset(&m_out_byte_func, 0, sizeof(m_out_byte_func));
	}
}



//**************************************************************************
//  INLINE HELPERS
//**************************************************************************

//-------------------------------------------------
//  set_atn_line - 
//-------------------------------------------------

inline void c64h156_device::set_atn_line()
{
	m_atn = !m_atni; // TODO m_atna

	devcb_call_write_line(&m_out_atn_func, m_atn);
}


//-------------------------------------------------
//  set_sync_line - 
//-------------------------------------------------

inline void c64h156_device::set_sync_line(int state)
{
	m_sync = state;

	devcb_call_write_line(&m_out_sync_func, m_sync);
}


//-------------------------------------------------
//  set_byte_line - 
//-------------------------------------------------

inline void c64h156_device::set_byte_line(int state)
{
	m_byte = state;

	devcb_call_write_line(&m_out_byte_func, BYTE);
}


//-------------------------------------------------
//  read_current_track - 
//-------------------------------------------------

inline void c64h156_device::read_current_track()
{
	int track_length = G64_BUFFER_SIZE;

	// read track data
	floppy_drive_read_track_data_info_buffer(m_image, 0, m_track_buffer, &track_length);

	// extract track length
	m_track_len = floppy_drive_get_current_track_size(m_image, 0);

	// set bit pointer to track start
	m_buffer_pos = 0;
	m_bit_pos = 7;
	m_bit_count = 0;
}


//-------------------------------------------------
//  receive_bit - 
//-------------------------------------------------

inline void c64h156_device::receive_bit()
{
	int byte = 0;

	// shift in data from the read head
	m_data <<= 1;
	m_data |= BIT(m_track_buffer[m_buffer_pos], m_bit_pos);
	m_bit_pos--;
	m_bit_count++;

	if (m_bit_pos < 0)
	{
		m_bit_pos = 7;
		m_buffer_pos++;

		if (m_buffer_pos >= m_track_len)
		{
			// loop to the start of the track
			m_buffer_pos = 0;
		}
	}

	set_sync_line(SYNC);

	if (!m_sync)
	{
		// SYNC detected
		m_bit_count = 0;
	}

	if (m_bit_count > 7)
	{
		// byte ready
		m_bit_count = 0;
		byte = 1;

		m_yb = m_data & 0xff;

		if (!m_yb)
		{
			// simulate weak bits with randomness
			m_yb = machine().rand() & 0xff;
		}

		set_byte_line(byte);
	}
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  c64h156_device - constructor
//-------------------------------------------------

c64h156_device::c64h156_device(running_machine &_machine, const c64h156_device_config &_config)
    : device_t(_machine, _config),
	  m_image(*this->owner(), FLOPPY_0),
	  m_stp(-1),
	  m_mtr(0),
	  m_accl(1),
	  m_ds(-1),
	  m_soe(0),
	  m_byte(0),
	  m_oe(0),
	  m_sync(1),
	  m_atn(1),
	  m_atni(1),
	  m_atna(1),
      m_config(_config)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void c64h156_device::device_start()
{
	// resolve callbacks
	devcb_resolve_write_line(&m_out_atn_func, &m_config.m_out_atn_func, this);
	devcb_resolve_write_line(&m_out_sync_func, &m_config.m_out_sync_func, this);
	devcb_resolve_write_line(&m_out_byte_func, &m_config.m_out_byte_func, this);

	// allocate timers
	m_bit_timer = timer_alloc();

	// register for state saving
	save_item(NAME(m_stp));
	save_item(NAME(m_mtr));
	save_item(NAME(m_track_len));
	save_item(NAME(m_buffer_pos));
	save_item(NAME(m_bit_pos));
	save_item(NAME(m_bit_count));
	save_item(NAME(m_data));
	save_item(NAME(m_yb));
	save_item(NAME(m_accl));
	save_item(NAME(m_ds));
	save_item(NAME(m_soe));
	save_item(NAME(m_byte));
	save_item(NAME(m_oe));
	save_item(NAME(m_sync));
	save_item(NAME(m_atn));
	save_item(NAME(m_atni));
	save_item(NAME(m_atna));
}


//-------------------------------------------------
//  device_timer - handler timer events
//-------------------------------------------------

void c64h156_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	receive_bit();
}


//-------------------------------------------------
//  yb_r - 
//-------------------------------------------------

READ8_MEMBER( c64h156_device::yb_r )
{
	set_byte_line(0); // TODO remove?

	return m_yb;
}


//-------------------------------------------------
//  yb_w - 
//-------------------------------------------------

WRITE8_MEMBER( c64h156_device::yb_w )
{
	m_yb = data;
}


//-------------------------------------------------
//  test_w - 
//-------------------------------------------------

WRITE_LINE_MEMBER( c64h156_device::test_w )
{
}


//-------------------------------------------------
//  accl_w - 
//-------------------------------------------------

WRITE_LINE_MEMBER( c64h156_device::accl_w )
{
	m_accl = state;
}


//-------------------------------------------------
//  sync_r - 
//-------------------------------------------------

READ_LINE_MEMBER( c64h156_device::sync_r )
{
	return m_sync;
}


//-------------------------------------------------
//  byte_r - 
//-------------------------------------------------

READ_LINE_MEMBER( c64h156_device::byte_r )
{
	return BYTE;
}


//-------------------------------------------------
//  stp_w - 
//-------------------------------------------------

WRITE_LINE_MEMBER( c64h156_device::mtr_w )
{
	if (m_mtr != state)
	{
		if (state)
		{
			// read track data
			read_current_track();
		}

		floppy_mon_w(m_image, !state);
		m_bit_timer->enable(state);

		m_mtr = state;
	}
}


//-------------------------------------------------
//  oe_w - 
//-------------------------------------------------

WRITE_LINE_MEMBER( c64h156_device::oe_w )
{
	m_oe = state;
}


//-------------------------------------------------
//  soe_w - 
//-------------------------------------------------

WRITE_LINE_MEMBER( c64h156_device::soe_w )
{
	m_soe = state;
}


//-------------------------------------------------
//  atn_r - 
//-------------------------------------------------

READ_LINE_MEMBER( c64h156_device::atn_r )
{
	return m_atn;
}


//-------------------------------------------------
//  atni_w - 
//-------------------------------------------------

WRITE_LINE_MEMBER( c64h156_device::atni_w )
{
	m_atni = state;

	set_atn_line();
}


//-------------------------------------------------
//  atna_w - 
//-------------------------------------------------

WRITE_LINE_MEMBER( c64h156_device::atna_w )
{
	m_atna = state;

	set_atn_line();
}


//-------------------------------------------------
//  stp_w - 
//-------------------------------------------------

void c64h156_device::stp_w(int data)
{
	if (m_mtr && (m_stp != data))
	{
		int tracks = 0;

		switch (m_stp)
		{
		case 0:	if (data == 1) tracks++; else if (data == 3) tracks--; break;
		case 1:	if (data == 2) tracks++; else if (data == 0) tracks--; break;
		case 2: if (data == 3) tracks++; else if (data == 1) tracks--; break;
		case 3: if (data == 0) tracks++; else if (data == 2) tracks--; break;
		}

		if (tracks != 0)
		{
			// step read/write head
			floppy_drive_seek(m_image, tracks);

			// read new track data
			read_current_track();
		}

		m_stp = data;
	}
}


//-------------------------------------------------
//  ds_w - density select
//-------------------------------------------------

void c64h156_device::ds_w(int data)
{
	if (m_ds != data)
	{
		m_bit_timer->adjust(attotime::zero, 0, attotime::from_hz(C2040_BITRATE[data]/4));
		m_ds = data;
	}
}


//-------------------------------------------------
//  on_disk_changed - 
//-------------------------------------------------

void c64h156_device::on_disk_changed()
{
	read_current_track();
}
