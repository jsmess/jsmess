/**********************************************************************

    SMC CRT9007 CRT Video Processor and Controller (VPAC) emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

    TODO:

    - interrupts
    - status register
    - reset
	- non-DMA mode
    - DMA mode
    - cursor/blank skew
    - sequential breaks
    - interlaced mode
    - smooth scroll
    - page blank
    - double height cursor
    - row attributes
    - pin configuration
    - operation modes 0,4,7
    - address modes 1,2,3
    - light pen
    - state saving

*/

#include "emu.h"
#include "crt9007.h"



//**************************************************************************
//	MACROS / CONSTANTS
//**************************************************************************

#define LOG 1

#define HAS_VALID_PARAMETERS \
	(m_reg[0x00] && m_reg[0x01] && m_reg[0x07] && m_reg[0x08] && m_reg[0x09])

#define CHARACTERS_PER_HORIZONTAL_PERIOD \
	m_reg[0x00]

#define CHARACTERS_PER_DATA_ROW \
	(m_reg[0x01] + 1)

#define HORIZONTAL_DELAY \
	m_reg[0x02]

#define HORIZONTAL_SYNC_WIDTH \
	m_reg[0x03]

#define VERTICAL_SYNC_WIDTH \
	m_reg[0x04]

#define VERTICAL_DELAY \
	(m_reg[0x05] - 1)

#define PIN_CONFIGURATION \
	(m_reg[0x06] >> 6)

#define CURSOR_SKEW \
	((m_reg[0x06] >> 3) & 0x07)

#define BLANK_SKEW \
	(m_reg[0x06] & 0x07)

#define VISIBLE_DATA_ROWS_PER_FRAME \
	(m_reg[0x07] + 1)

#define SCAN_LINES_PER_DATA_ROW \
	((m_reg[0x08] & 0x1f) + 1)

#define SCAN_LINES_PER_FRAME \
	(((m_reg[0x08] << 3) & 0x0700) | m_reg[0x09])

#define DMA_BURST_COUNT \
	(m_reg[0x0a] & 0x0f)

#define DMA_BURST_DELAY \
	((m_reg[0x0a] >> 4) & 0x07)

#define DMA_DISABLE \
	BIT(m_reg[0x0a], 7)

#define SINGLE_HEIGHT_CURSOR \
	BIT(m_reg[0x0b], 0)

#define OPERATION_MODE \
	((m_reg[0x0b] >> 1) & 0x07)

#define INTERLACE_MODE \
	((m_reg[0x0b] >> 4) & 0x03)

#define PAGE_BLANK \
	BIT(m_reg[0x0b], 6)

#define TABLE_START \
	(((m_reg[0x0d] << 8) & 0x3f00) | m_reg[0x0c])

#define ADDRESS_MODE \
	((m_reg[0x0d] >> 6) & 0x03)

#define AUXILIARY_ADDRESS_1 \
	(((m_reg[0x0f] << 8) & 0x3f00) | m_reg[0x0e])

#define ROW_ATTRIBUTES_1 \
	((m_reg[0x0f] >> 6) & 0x03)

#define SEQUENTIAL_BREAK_1 \
	m_reg[0x10]

#define SEQUENTIAL_BREAK_2 \
	m_reg[0x12]

#define DATA_ROW_START \
	m_reg[0x11]

#define DATA_ROW_END \
	m_reg[0x12]

#define AUXILIARY_ADDRESS_2 \
	(((m_reg[0x14] << 8) & 0x3f00) | m_reg[0x13])

#define ROW_ATTRIBUTES_2 \
	((m_reg[0x14] >> 6) & 0x03)

#define SMOOTH_SCROLL_OFFSET \
	((m_reg[0x17] >> 1) & 0x3f)

#define SMOOTH_SCROLL_OFFSET_OVERFLOW \
	BIT(m_reg[0x17], 7)

#define VERTICAL_CURSOR \
	m_reg[0x18]

#define HORIZONTAL_CURSOR \
	m_reg[0x19]

#define FRAME_TIMER \
	BIT(m_reg[0x1a], 0)

#define LIGHT_PEN_INTERRUPT \
	BIT(m_reg[0x1a], 5)

#define VERTICAL_RETRACE_INTERRUPT \
	BIT(m_reg[0x1a], 6)

#define VERTICAL_LIGHT_PEN \
	m_reg[0x3b]

#define HORIZONTAL_LIGHT_PEN \
	m_reg[0x3c]


// interlace
enum
{
	NON_INTERLACED = 0,
	ENHANCED_VIDEO_INTERFACE,
	NORMAL_VIDEO_INTERFACE,
};


// operation modes
enum
{
	OPERATION_MODE_REPETITIVE_MEMORY_ADDRESSING = 0,
	OPERATION_MODE_DOUBLE_ROW_BUFFER,
	OPERATION_MODE_SINGLE_ROW_BUFFER = 4,
	OPERATION_MODE_ATTRIBUTE_ASSEMBLE = 7
};


// addressing modes
enum
{
	ADDRESS_MODE_SEQUENTIAL_ADDRESSING = 0,
	ADDRESS_MODE_SEQUENTIAL_ROLL_ADDRESSING,
	ADDRESS_MODE_CONTIGUOUS_ROW_TABLE,
	ADDRESS_MODE_LINKED_LIST_ROW_TABLE
};


// status register bits
const int STATUS_INTERRUPT_PENDING		= 0x80;
const int STATUS_VERTICAL_RETRACE		= 0x40;
const int STATUS_LIGHT_PEN_UPDATE		= 0x20;
const int STATUS_ODD_EVEN				= 0x04;
const int STATUS_FRAME_TIMER_OCCURRED	= 0x01;



//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

// devices
const device_type CRT9007 = crt9007_device_config::static_alloc_device_config;


// default address map
static ADDRESS_MAP_START( crt9007, 0, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_RAM
ADDRESS_MAP_END



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  crt9007_device_config - constructor
//-------------------------------------------------

crt9007_device_config::crt9007_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
	: device_config(mconfig, static_alloc_device_config, "SMC CRT9007", tag, owner, clock),
	  device_config_memory_interface(mconfig, *this),
	  m_space_config("videoram", ENDIANNESS_LITTLE, 8, 14, 0, NULL, *ADDRESS_MAP_NAME(crt9007))
{
}


//-------------------------------------------------
//  static_alloc_device_config - allocate a new
//  configuration object
//-------------------------------------------------

device_config *crt9007_device_config::static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
{
	return global_alloc(crt9007_device_config(mconfig, tag, owner, clock));
}


//-------------------------------------------------
//  alloc_device - allocate a new device object
//-------------------------------------------------

device_t *crt9007_device_config::alloc_device(running_machine &machine) const
{
	return auto_alloc(&machine, crt9007_device(machine, *this));
}


//-------------------------------------------------
//  memory_space_config - return a description of
//  any address spaces owned by this device
//-------------------------------------------------

const address_space_config *crt9007_device_config::memory_space_config(int spacenum) const
{
	return (spacenum == 0) ? &m_space_config : NULL;
}


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void crt9007_device_config::device_config_complete()
{
	// inherit a copy of the static data
	const crt9007_interface *intf = reinterpret_cast<const crt9007_interface *>(static_config());
	if (intf != NULL)
		*static_cast<crt9007_interface *>(this) = *intf;

	// or initialize to defaults if none provided
	else
	{
		memset(&out_int_func, 0, sizeof(out_int_func));
		memset(&out_dmar_func, 0, sizeof(out_dmar_func));
		memset(&out_vs_func, 0, sizeof(out_vs_func));
		memset(&out_hs_func, 0, sizeof(out_hs_func));
		memset(&out_vlt_func, 0, sizeof(out_vlt_func));
		memset(&out_curs_func, 0, sizeof(out_curs_func));
		memset(&out_drb_func, 0, sizeof(out_drb_func));
		memset(&out_slg_func, 0, sizeof(out_slg_func));
		memset(&out_sld_func, 0, sizeof(out_sld_func));
	}
}



//**************************************************************************
//  INLINE HELPERS
//**************************************************************************

//-------------------------------------------------
//  readbyte - read a byte at the given address
//-------------------------------------------------

inline UINT8 crt9007_device::readbyte(offs_t address)
{
	return space()->read_byte(address);
}


//-------------------------------------------------
//  update_hsync_timer - 
//-------------------------------------------------

inline void crt9007_device::update_hsync_timer(int state)
{
	int y = m_screen->vpos();

	int next_x = state ? m_hsync_start : m_hsync_end;
	int next_y = (y + 1) % SCAN_LINES_PER_FRAME;

	attotime duration = m_screen->time_until_pos(next_y, next_x);

	timer_adjust_oneshot(m_hsync_timer, duration, !state);
}


//-------------------------------------------------
//  update_vsync_timer - 
//-------------------------------------------------

inline void crt9007_device::update_vsync_timer(int state)
{
	int next_y = state ? m_vsync_start : m_vsync_end;

	attotime duration = m_screen->time_until_pos(next_y, 0);

	timer_adjust_oneshot(m_vsync_timer, duration, !state);
}


//-------------------------------------------------
//  update_vlt_timer - 
//-------------------------------------------------

inline void crt9007_device::update_vlt_timer(int state)
{
	// TODO
}


//-------------------------------------------------
//  update_curs_timer - 
//-------------------------------------------------

inline void crt9007_device::update_curs_timer(int state)
{
	// TODO
}


//-------------------------------------------------
//  update_drb_timer - 
//-------------------------------------------------

inline void crt9007_device::update_drb_timer(int state)
{
	// TODO
}


//-------------------------------------------------
//  recompute_parameters - 
//-------------------------------------------------

inline void crt9007_device::recompute_parameters()
{
	// check that necessary registers have been loaded
	if (!HAS_VALID_PARAMETERS) return;

	int horiz_pix_total = CHARACTERS_PER_HORIZONTAL_PERIOD * m_hpixels_per_column;
	int vert_pix_total = SCAN_LINES_PER_FRAME;

	attoseconds_t refresh = HZ_TO_ATTOSECONDS(clock()) * horiz_pix_total * vert_pix_total;

	m_hsync_start = (CHARACTERS_PER_HORIZONTAL_PERIOD - HORIZONTAL_SYNC_WIDTH) * m_hpixels_per_column;
	m_hsync_end = 0;
	m_hfp = (HORIZONTAL_DELAY - HORIZONTAL_SYNC_WIDTH) * m_hpixels_per_column;

	m_vsync_start = SCAN_LINES_PER_FRAME - VERTICAL_SYNC_WIDTH;
	m_vsync_end = 0;
	m_vfp = VERTICAL_DELAY - VERTICAL_SYNC_WIDTH;

	rectangle visarea;

	visarea.min_x = m_hsync_end;
	visarea.max_x = m_hsync_start - 1;

	visarea.min_y = m_vsync_end;
	visarea.max_y = m_vsync_start - 1;

	if (LOG)
	{
		logerror("CRT9007 '%s' Screen: %u x %u @ %f Hz\n", tag(), horiz_pix_total, vert_pix_total, 1 / ATTOSECONDS_TO_DOUBLE(refresh));
		logerror("CRT9007 '%s' Visible Area: (%u, %u) - (%u, %u)\n", tag(), visarea.min_x, visarea.min_y, visarea.max_x, visarea.max_y);
	}

	m_screen->configure(horiz_pix_total, vert_pix_total, visarea, refresh);

	update_hsync_timer(1);
	update_vsync_timer(1);
	update_vlt_timer(1);
	update_curs_timer(1);
	update_drb_timer(1);
}


//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  crt9007_device - constructor
//-------------------------------------------------

crt9007_device::crt9007_device(running_machine &_machine, const crt9007_device_config &config)
    : device_t(_machine, config),
	  device_memory_interface(_machine, config, *this),
      m_config(config)
{
	for (int i = 0; i < 0x3d; i++)
		m_reg[i] = 0;
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void crt9007_device::device_start()
{
	// allocate timers
	m_hsync_timer = device_timer_alloc(*this, TIMER_HSYNC);
	m_vsync_timer = device_timer_alloc(*this, TIMER_VSYNC);
	m_vlt_timer = device_timer_alloc(*this, TIMER_VLT);
	m_curs_timer = device_timer_alloc(*this, TIMER_CURS);
	m_drb_timer = device_timer_alloc(*this, TIMER_DRB);

	// resolve callbacks
	devcb_resolve_write_line(&m_out_int_func, &m_config.out_int_func, this);
	devcb_resolve_write_line(&m_out_dmar_func, &m_config.out_dmar_func, this);
	devcb_resolve_write_line(&m_out_hs_func, &m_config.out_hs_func, this);
	devcb_resolve_write_line(&m_out_vs_func, &m_config.out_vs_func, this);
	devcb_resolve_write_line(&m_out_vlt_func, &m_config.out_vlt_func, this);
	devcb_resolve_write_line(&m_out_curs_func, &m_config.out_curs_func, this);
	devcb_resolve_write_line(&m_out_drb_func, &m_config.out_drb_func, this);

	// get the screen device
	m_screen = machine->device<screen_device>(m_config.screen_tag);
	assert(m_screen != NULL);

	// set horizontal pixels per column
	m_hpixels_per_column = m_config.hpixels_per_column;

	// register for state saving
//	state_save_register_device_item(this, 0, );
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void crt9007_device::device_reset()
{
	m_disp = 0;

	// HS = 1
	devcb_call_write_line(&m_out_hs_func, 1);

	// VS = 1
	devcb_call_write_line(&m_out_vs_func, 1);

	// CBLANK = 1
//	devcb_call_write_line(&m_out_cblank_func, 0);

	// CURS = 0
	devcb_call_write_line(&m_out_curs_func, 0);

	// VLT = 0
	devcb_call_write_line(&m_out_vlt_func, 0);

	// DRB = 1
	devcb_call_write_line(&m_out_drb_func, 1);

	// INT = 0
	devcb_call_write_line(&m_out_int_func, CLEAR_LINE);

	// 28 (DMAR) = 0
	devcb_call_write_line(&m_out_dmar_func, CLEAR_LINE);

	// 29 (WBEN) = 0

	// 30 (SLG) = 0
//	devcb_call_write_line(&m_out_slg_func, 0);

	// 31 (SLD) = 0
//	devcb_call_write_line(&m_out_sld_func, 0);

	// 32 (LPSTB) = 0
}


//-------------------------------------------------
//  device_clock_changed - handle clock change
//-------------------------------------------------

void crt9007_device::device_clock_changed()
{
	recompute_parameters();
}


//-------------------------------------------------
//  device_timer - handle timer events
//-------------------------------------------------

void crt9007_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	switch (id)
	{
	case TIMER_HSYNC:
		devcb_call_write_line(&m_out_hs_func, param);
		update_hsync_timer(param);
		break;

	case TIMER_VSYNC:
		devcb_call_write_line(&m_out_vs_func, param);
		update_vsync_timer(param);
		break;

	case TIMER_VLT:
		devcb_call_write_line(&m_out_vlt_func, param);
		update_vlt_timer(param);
		break;

	case TIMER_CURS:
		devcb_call_write_line(&m_out_curs_func, param);
		update_curs_timer(param);
		break;

	case TIMER_DRB:
		devcb_call_write_line(&m_out_drb_func, param);
		update_drb_timer(param);
		break;
	}
}


//-------------------------------------------------
//  read - register read
//-------------------------------------------------

READ8_MEMBER( crt9007_device::read )
{
	UINT8 data = 0;

	switch (offset)
	{
	case 0x15:
		if (LOG) logerror("CRT9007 '%s' Start\n", tag());
		m_disp = 1;
		break;

	case 0x16:
		if (LOG) logerror("CRT9007 '%s' Reset\n", tag());
		device_reset();
		break;

	case 0x38:
		data = VERTICAL_CURSOR;
		break;

	case 0x39:
		data = HORIZONTAL_CURSOR;
		break;

	case 0x3a:
		data = m_status;
		break;

	case 0x3b:
		data = VERTICAL_LIGHT_PEN;
		break;

	case 0x3c:
		data = HORIZONTAL_LIGHT_PEN;
		break;

	default:
		logerror("CRT9007 '%s' Read from Invalid Register: %02x!\n", tag(), offset);
	}

	return data;
}


//-------------------------------------------------
//  write - register write
//-------------------------------------------------

WRITE8_MEMBER( crt9007_device::write )
{
	m_reg[offset] = data;

	switch (offset)
	{
	case 0x00:
		recompute_parameters();
		if (LOG) logerror("CRT9007 '%s' Characters per Horizontal Period: %u\n", tag(), CHARACTERS_PER_HORIZONTAL_PERIOD);
		break;

	case 0x01:
		recompute_parameters();
		if (LOG) logerror("CRT9007 '%s' Characters per Data Row: %u\n", tag(), CHARACTERS_PER_DATA_ROW);
		break;

	case 0x02:
		recompute_parameters();
		if (LOG) logerror("CRT9007 '%s' Horizontal Delay: %u\n", tag(), HORIZONTAL_DELAY);
		break;

	case 0x03:
		recompute_parameters();
		if (LOG) logerror("CRT9007 '%s' Horizontal Sync Width: %u\n", tag(), HORIZONTAL_SYNC_WIDTH);
		break;

	case 0x04:
		recompute_parameters();
		if (LOG) logerror("CRT9007 '%s' Vertical Sync Width: %u\n", tag(), VERTICAL_SYNC_WIDTH);
		break;

	case 0x05:
		recompute_parameters();
		if (LOG) logerror("CRT9007 '%s' Vertical Delay: %u\n", tag(), VERTICAL_DELAY);
		break;

	case 0x06:
		recompute_parameters();
		if (LOG)
		{
			logerror("CRT9007 '%s' Pin Configuration: %u\n", tag(), PIN_CONFIGURATION);
			logerror("CRT9007 '%s' Cursor Skew: %u\n", tag(), CURSOR_SKEW);
			logerror("CRT9007 '%s' Blank Skew: %u\n", tag(), BLANK_SKEW);
		}
		break;

	case 0x07:
		recompute_parameters();
		if (LOG) logerror("CRT9007 '%s' Visible Data Rows per Frame: %u\n", tag(), VISIBLE_DATA_ROWS_PER_FRAME);
		break;

	case 0x08:
		recompute_parameters();
		if (LOG) logerror("CRT9007 '%s' Scan Lines per Data Row: %u\n", tag(), SCAN_LINES_PER_DATA_ROW);
		break;

	case 0x09:
		recompute_parameters();
		if (LOG) logerror("CRT9007 '%s' Scan Lines per Frame: %u\n", tag(), SCAN_LINES_PER_FRAME);
		break;

	case 0x0a:
		if (LOG)
		{
			logerror("CRT9007 '%s' DMA Burst Count: %u\n", tag(), DMA_BURST_COUNT);
			logerror("CRT9007 '%s' DMA Burst Delay: %u\n", tag(), DMA_BURST_DELAY);
			logerror("CRT9007 '%s' DMA Disable: %u\n", tag(), DMA_DISABLE);
		}
		break;

	case 0x0b:
		if (LOG)
		{
			logerror("CRT9007 '%s' %s Height Cursor\n", tag(), SINGLE_HEIGHT_CURSOR ? "Single" : "Double");
			logerror("CRT9007 '%s' Operation Mode: %u\n", tag(), OPERATION_MODE);
			logerror("CRT9007 '%s' Interlace Mode: %u\n", tag(), INTERLACE_MODE);
			logerror("CRT9007 '%s' %s Mechanism\n", tag(), PAGE_BLANK ? "Page Blank" : "Smooth Scroll");
		}
		break;

	case 0x0c:
		break;

	case 0x0d:
		if (LOG)
		{
			logerror("CRT9007 '%s' Table Start Register: %04x\n", tag(), TABLE_START);
			logerror("CRT9007 '%s' Address Mode: %u\n", tag(), ADDRESS_MODE);
		}
		break;

	case 0x0e:
		break;

	case 0x0f:
		if (LOG)
		{
			logerror("CRT9007 '%s' Auxialiary Address Register 1: %04x\n", tag(), AUXILIARY_ADDRESS_1);
			logerror("CRT9007 '%s' Row Attributes: %u\n", tag(), ROW_ATTRIBUTES_1);
		}
		break;

	case 0x10:
		if (LOG) logerror("CRT9007 '%s' Sequential Break Register 1: %u\n", tag(), SEQUENTIAL_BREAK_1);
		break;

	case 0x11:
		if (LOG) logerror("CRT9007 '%s' Data Row Start Register: %u\n", tag(), DATA_ROW_START);
		break;

	case 0x12:
		if (LOG) logerror("CRT9007 '%s' Data Row End/Sequential Break Register 2: %u\n", tag(), SEQUENTIAL_BREAK_2);
		break;

	case 0x13:
		break;

	case 0x14:
		if (LOG)
		{
			logerror("CRT9007 '%s' Auxiliary Address Register 2: %04x\n", tag(), AUXILIARY_ADDRESS_2);
			logerror("CRT9007 '%s' Row Attributes: %u\n", tag(), ROW_ATTRIBUTES_2);
		}
		break;

	case 0x15:
		if (LOG) logerror("CRT9007 '%s' Start\n", tag());
		m_disp = 1;
		break;

	case 0x16:
		if (LOG) logerror("CRT9007 '%s' Reset\n", tag());
		device_reset();
		break;

	case 0x17:
		if (LOG)
		{
			logerror("CRT9007 '%s' Smooth Scroll Offset: %u\n", tag(), SMOOTH_SCROLL_OFFSET);
			logerror("CRT9007 '%s' Smooth Scroll Offset Overflow: %u\n", tag(), SMOOTH_SCROLL_OFFSET_OVERFLOW);
		}
		break;

	case 0x18:
		if (LOG) logerror("CRT9007 '%s' Vertical Cursor Register: %u\n", tag(), VERTICAL_CURSOR);
		break;

	case 0x19:
		if (LOG) logerror("CRT9007 '%s' Horizontal Cursor Register: %u\n", tag(), HORIZONTAL_CURSOR);
		break;

	case 0x1a:
		if (LOG)
		{
			logerror("CRT9007 '%s' Frame Timer: %u\n", tag(), FRAME_TIMER);
			logerror("CRT9007 '%s' Light Pen Interrupt: %u\n", tag(), LIGHT_PEN_INTERRUPT);
			logerror("CRT9007 '%s' Vertical Retrace Interrupt: %u\n", tag(), VERTICAL_RETRACE_INTERRUPT);
		}
		break;

	default:
		logerror("CRT9007 '%s' Write to Invalid Register: %02x!\n", tag(), offset);
	}
}


//-------------------------------------------------
//  ack_w - DMA acknowledge
//-------------------------------------------------

WRITE_LINE_MEMBER( crt9007_device::ack_w )
{
	// TODO
}


//-------------------------------------------------
//  lpstb_w - light pen strobe
//-------------------------------------------------

WRITE_LINE_MEMBER( crt9007_device::lpstb_w )
{
	// TODO
}


//-------------------------------------------------
//  vlt_r - 
//-------------------------------------------------

READ_LINE_MEMBER( crt9007_device::vlt_r )
{
	return m_vlt;
}


//-------------------------------------------------
//  wben_r - 
//-------------------------------------------------

READ_LINE_MEMBER( crt9007_device::wben_r )
{
	return m_wben;
}


//-------------------------------------------------
//  set_hpixels_per_column - 
//-------------------------------------------------

void crt9007_device::set_hpixels_per_column(int hpixels_per_column)
{
	m_hpixels_per_column = hpixels_per_column;
	recompute_parameters();
}
