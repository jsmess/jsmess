/****************************************************************************

    TI-99/4(A) and /8 Video subsystem
    This device actually wraps the naked video chip implementation

    EVPC (Enhanced Video Processor Card) from SNUG
    based on v9938 (may also be equipped with v9958)
    Can be used with TI-99/4A as an add-on card; internal VDP must be removed

    The SGCPU ("TI-99/4P") only runs with EVPC

    We also include a class wrapper for the sound chip here.

    Michael Zapf

    October 2010
    February 2012: Rewritten as class

*****************************************************************************/

#include "emu.h"
#include "videowrp.h"
#include "sound/sn76496.h"

/*
    Constructors
*/
ti_video_device::ti_video_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock)
: bus8z_device(mconfig, type, name, tag, owner, clock)
{
}

ti_std_video_device::ti_std_video_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: ti_video_device(mconfig, TI994AVIDEO, "Video subsystem", tag, owner, clock)
{
}

ti_std8_video_device::ti_std8_video_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: ti_video_device(mconfig, TI998VIDEO, "Video subsystem", tag, owner, clock)
{
}

ti_exp_video_device::ti_exp_video_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: ti_video_device(mconfig, V9938VIDEO, "Video subsystem", tag, owner, clock)
{
}

/*****************************************************************************/
/*
    Illegal accesses; just take a wait state. Used by TI-99/4A standard or with EVPC
*/

READ16_MEMBER( ti_video_device::noread )
{
	device_adjust_icount(m_cpu, -4);
	return 0;
}

WRITE16_MEMBER( ti_video_device::nowrite )
{
	device_adjust_icount(m_cpu, -4);
}

/*****************************************************************************/

// TODO: device_adjust_icount(video->cpu, -4);

/*
    Memory access (TI-99/4(A))
*/
READ16_MEMBER( ti_std_video_device::read16 )
{
	if (offset & 1)
	{	/* read VDP status */
		return ((int) m_tms9928a->register_read(*(this->m_space), 0)) << 8;
	}
	else
	{	/* read VDP RAM */
		return ((int) m_tms9928a->vram_read(*(this->m_space), 0)) << 8;
	}
}

WRITE16_MEMBER( ti_std_video_device::write16 )
{
	if (offset & 1)
	{	/* write VDP address */
		m_tms9928a->register_write(*(this->m_space), 0, (data >> 8) & 0xff);
	}
	else
	{	/* write VDP data */
		m_tms9928a->vram_write(*(this->m_space), 0, (data >> 8) & 0xff);
	}
}

/******************************************************************************/

/*
    Memory access (TI-99/8)
    Makes use of the Z memory handler.
*/
READ8Z_MEMBER( ti_std8_video_device::readz )
{
	if (offset & 2)
	{       /* read VDP status */
		*value = m_tms9928a->register_read(*(this->m_space), 0);
	}
	else
	{       /* read VDP RAM */
		*value = m_tms9928a->vram_read(*(this->m_space), 0);
	}
}

WRITE8_MEMBER( ti_std8_video_device::write )
{
	if (offset & 2)
	{	/* write VDP address */
		m_tms9928a->register_write(*(this->m_space), 0, data);
	}
	else
	{	/* write VDP data */
		m_tms9928a->vram_write(*(this->m_space), 0, data);
	}
}

/*****************************************************************************/

/*
    Memory access (EVPC) via 16 bit bus
*/
READ16_MEMBER( ti_exp_video_device::read16 )
{
	if (offset & 1)
	{	/* read VDP status */
		return ((int) m_v9938->status_r()) << 8;
	}
	else
	{	/* read VDP RAM */
		return ((int) m_v9938->vram_r()) << 8;
	}
}

WRITE16_MEMBER( ti_exp_video_device::write16 )
{
	switch (offset & 3)
	{
	case 0:
		/* write VDP data */
		m_v9938->vram_w((data >> 8) & 0xff);
		break;
	case 1:
		/* write VDP address */
		m_v9938->command_w((data >> 8) & 0xff);
		break;
	case 2:
		/* write VDP palette */
		m_v9938->palette_w((data >> 8) & 0xff);
		break;
	case 3:
		/* write VDP register pointer (indirect access) */
		m_v9938->register_w((data >> 8) & 0xff);
		break;
	}
}

/******************************************************************************/

/*
    Video read (Geneve) via 8 bit bus
*/
READ8Z_MEMBER( ti_exp_video_device::readz )
{
	if (offset & 2)
	{	/* read VDP status */
		*value = m_v9938->status_r();
	}
	else
	{	/* read VDP RAM */
		*value = m_v9938->vram_r();
	}
}

/*
    Video write (Geneve)
*/
WRITE8_MEMBER( ti_exp_video_device::write )
{
	switch (offset & 6)
	{
	case 0:
		/* write VDP data */
		m_v9938->vram_w(data);
		break;
	case 2:
		/* write VDP address */
		m_v9938->command_w(data);
		break;
	case 4:
		/* write VDP palette */
		m_v9938->palette_w(data);
		break;
	case 6:
		/* write VDP register pointer (indirect access) */
		m_v9938->register_w(data);
		break;
	}
}

/**************************************************************************/
// Interfacing to mouse attached to v9938

void ti_exp_video_device::video_update_mouse(int delta_x, int delta_y, int buttons)
{
	m_v9938->update_mouse_state(delta_x, delta_y, buttons & 3);
}

/**************************************************************************/

void ti_video_device::device_start(void)
{
	m_cpu = machine().device("maincpu");
	m_space = m_cpu->memory().space(AS_PROGRAM);
	m_tms9928a = static_cast<tms9928a_device*>(machine().device(TMS9928A_TAG));
}

void ti_exp_video_device::device_start(void)
{
	m_cpu = machine().device("maincpu");
	m_space = m_cpu->memory().space(AS_PROGRAM);
	m_v9938 = static_cast<v9938_device*>(machine().device(V9938_TAG));
}

void ti_video_device::device_reset(void)
{
}

/**************************************************************************/

ti_sound_system_device::ti_sound_system_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
: bus8z_device(mconfig, TISOUND, "TI sound chip wrapper", tag, owner, clock)
{
}

WRITE8_MEMBER( ti_sound_system_device::write )
{
	sn76496_w(m_sound_chip, offset, data);
}

void ti_sound_system_device::device_start(void)
{
	m_sound_chip = machine().device(TISOUNDCHIP_TAG);
}

/**************************************************************************/

const device_type TI994AVIDEO = &device_creator<ti_std_video_device>;
const device_type TI998VIDEO = &device_creator<ti_std8_video_device>;
const device_type V9938VIDEO = &device_creator<ti_exp_video_device>;
const device_type TISOUND = &device_creator<ti_sound_system_device>;
