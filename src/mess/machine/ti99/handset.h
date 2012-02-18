/****************************************************************************

    TI-99/4 handset
    See handset.c for documentation.

    Michael Zapf, October 2010
    February 2012: Rewritten as class

*****************************************************************************/

#ifndef __HANDSET__
#define __HANDSET__

#include "emu.h"

#define MAX_HANDSETS 4

extern const device_type HANDSET;

typedef struct _ti99_handset_intf
{
	devcb_write_line	interrupt;
} ti99_handset_intf;

class ti99_handset_device : public device_t
{
public:
	ti99_handset_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	void set_acknowledge(int data);

	// Poll the current state of the 4-bit data bus that goes from the I/R
	// receiver to the tms9901.
	int poll_bus()	{ return m_buf & 0xf; }
	int get_clock() { return m_clock; }
	int get_int()	{ return m_buflen==3; }

protected:
	void device_start(void);
	void device_reset(void);
	void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);
	void do_task();
	void post_message(int message);
	bool poll_keyboard(int num);
	bool poll_joystick(int num);

	int		m_ack;
	int		m_clock;
	int		m_buf;
	int		m_buflen;
	UINT8	previous_joy[MAX_HANDSETS];
	UINT8	previous_key[MAX_HANDSETS];

	devcb_resolved_write_line	m_interrupt;

	emu_timer *m_delay_timer;
	emu_timer *m_poll_timer;
};

#define MCFG_HANDSET_ADD(_tag, _intf, _clock )	\
	MCFG_DEVICE_ADD(_tag, HANDSET, _clock)	\
	MCFG_DEVICE_CONFIG(_intf)

#define TI99_HANDSET_INTERFACE(name)	\
	const ti99_handset_intf(name) =

#endif
