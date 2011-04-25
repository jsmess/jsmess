/**********************************************************************

    RCA VP550 - VIP Super Sound System emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*********************************************************************/

#include "vp550.h"
#include "machine/devhelpr.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define LOG 0


#define CDP1863_A_TAG	"u1"
#define CDP1863_B_TAG	"u2"
#define CDP1863_C_TAG	"cdp1863c"
#define CDP1863_D_TAG	"cdp1863d"


enum
{
	CHANNEL_A = 0,
	CHANNEL_B,
	CHANNEL_C,
	CHANNEL_D
};



//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

// devices
const device_type VP550 = vp550_device_config::static_alloc_device_config;
const device_type VP551 = vp550_device_config::static_alloc_device_config;



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

GENERIC_DEVICE_CONFIG_SETUP(vp550, "VP550")


//-------------------------------------------------
//  static_set_config - configuration helper
//-------------------------------------------------

void vp550_device_config::static_set_config(device_config *device, int variant)
{
	vp550_device_config *vp550 = downcast<vp550_device_config *>(device);

	vp550->m_variant = variant;
}


//-------------------------------------------------
//  MACHINE_CONFIG_FRAGMENT( vp550 )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( vp550 )
	MCFG_CDP1863_ADD(CDP1863_A_TAG, 0, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MCFG_CDP1863_ADD(CDP1863_B_TAG, 0, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG_FRAGMENT( vp551 )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( vp551 )
	MCFG_CDP1863_ADD(CDP1863_A_TAG, 0, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MCFG_CDP1863_ADD(CDP1863_B_TAG, 0, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MCFG_CDP1863_ADD(CDP1863_C_TAG, 0, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MCFG_CDP1863_ADD(CDP1863_D_TAG, 0, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor vp550_device_config::device_mconfig_additions() const
{
	switch (m_variant)
	{
	default:
	case TYPE_VP550:
		return MACHINE_CONFIG_NAME( vp550 );

	case TYPE_VP551:
		return MACHINE_CONFIG_NAME( vp551 );
	}
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  vp550_device - constructor
//-------------------------------------------------

vp550_device::vp550_device(running_machine &_machine, const vp550_device_config &config)
    : device_t(_machine, config),
	  m_pfg_a(*this, CDP1863_A_TAG),
	  m_pfg_b(*this, CDP1863_B_TAG),
	  m_pfg_c(*this, CDP1863_C_TAG),
	  m_pfg_d(*this, CDP1863_D_TAG),
      m_config(config)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void vp550_device::device_start()
{
	// allocate timers
	m_sync_timer = timer_alloc();
	m_sync_timer->adjust(attotime::from_hz(50), 0, attotime::from_hz(50));
	m_sync_timer->enable(0);
}


//-------------------------------------------------
//  device_timer - handler timer events
//-------------------------------------------------

void vp550_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	if (LOG) logerror("VP550 '%s' Interrupt\n", tag());

	device_set_input_line(machine().firstcpu, COSMAC_INPUT_LINE_INT, ASSERT_LINE);
}


//-------------------------------------------------
//  octave_w - octave select write
//-------------------------------------------------

WRITE8_MEMBER( vp550_device::octave_w )
{
	int channel = (data >> 2) & 0x03;
	int clock2 = 0;

	if (data & 0x10)
	{
		switch (data & 0x03)
		{
		case 0: clock2 = clock() / 8; break;
		case 1: clock2 = clock() / 4; break;
		case 2: clock2 = clock() / 2; break;
		case 3: clock2 = clock();	 break;
		}
	}

	switch (channel)
	{
	case CHANNEL_A: m_pfg_a->set_clk2(clock2); break;
	case CHANNEL_B: m_pfg_b->set_clk2(clock2); break;
	case CHANNEL_C: if (m_pfg_c) m_pfg_c->set_clk2(clock2); break;
	case CHANNEL_D: if (m_pfg_d) m_pfg_d->set_clk2(clock2); break;
	}

	if (LOG) logerror("VP550 '%s' Clock %c: %u Hz\n", tag(), 'A' + channel, clock2);
}


//-------------------------------------------------
//  vlmna_w - channel A amplitude write
//-------------------------------------------------

WRITE8_MEMBER( vp550_device::vlmna_w )
{
	if (LOG) logerror("VP550 '%s' A Volume: %u\n", tag(), data & 0x0f);
}


//-------------------------------------------------
//  vlmnb_w - channel B amplitude write
//-------------------------------------------------

WRITE8_MEMBER( vp550_device::vlmnb_w )
{
	if (LOG) logerror("VP550 '%s' B Volume: %u\n", tag(), data & 0x0f);
}


//-------------------------------------------------
//  sync_w - interrupt enable write
//-------------------------------------------------

WRITE8_MEMBER( vp550_device::sync_w )
{
	if (LOG) logerror("VP550 '%s' Interrupt Enable: %u\n", tag(), BIT(data, 0));

	m_sync_timer->enable(BIT(data, 0));
}


//-------------------------------------------------
//  q_w -
//-------------------------------------------------

WRITE_LINE_MEMBER( vp550_device::q_w )
{
	m_pfg_a->oe_w(state);
	m_pfg_b->oe_w(state);
	if (m_pfg_c) m_pfg_c->oe_w(state);
	if (m_pfg_d) m_pfg_d->oe_w(state);
}


//-------------------------------------------------
//  sc1_w -
//-------------------------------------------------

WRITE_LINE_MEMBER( vp550_device::sc1_w )
{
	if (state)
	{
		if (LOG) logerror("VP550 '%s' Clear Interrupt\n", tag());

		device_set_input_line(machine().firstcpu, COSMAC_INPUT_LINE_INT, CLEAR_LINE);
	}
}


//-------------------------------------------------
//  install_write_handlers -
//-------------------------------------------------

void vp550_device::install_write_handlers(address_space *space, bool enabled)
{
	if (enabled)
	{
		space->install_write_handler(0x8001, 0x8001, write8_delegate(FUNC(cdp1863_device::str_w), (cdp1863_device*)m_pfg_a));
		space->install_write_handler(0x8002, 0x8002, write8_delegate(FUNC(cdp1863_device::str_w), (cdp1863_device*)m_pfg_b));
		space->install_write_handler(0x8003, 0x8003, write8_delegate(FUNC(vp550_device::octave_w), this));
		space->install_write_handler(0x8010, 0x8010, write8_delegate(FUNC(vp550_device::vlmna_w), this));
		space->install_write_handler(0x8020, 0x8020, write8_delegate(FUNC(vp550_device::vlmnb_w), this));
		space->install_write_handler(0x8030, 0x8030, write8_delegate(FUNC(vp550_device::sync_w), this));
	}
	else
	{
		space->unmap_write(0x8001, 0x8001);
		space->unmap_write(0x8002, 0x8002);
		space->unmap_write(0x8003, 0x8003);
		space->unmap_write(0x8010, 0x8010);
		space->unmap_write(0x8020, 0x8020);
		space->unmap_write(0x8030, 0x8030);
	}
}
