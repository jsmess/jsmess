/****************************************************************************

    TI-99 Mechatronic mouse with adapter

    The Mechatronic mouse is connected to the joystick port and occupies
    both joystick select lines and the switch lines. From these five
    lines, left/right/down are used for the motion (i.e. 3 motion steps
    for positive and four for negative motion and one for rest),
    the fire line is used for the secondary mouse button, and the up
    line is used for the primary button.
    The mouse motion is delivered by the same lines for both directions;
    this requires swapping the axes. According to the source code of
    the accompanying mouse driver, the readout of the current axis is
    done by selecting joystick 1, then joystick 2. The axis swapping is
    achieved by selecting stick 1 again. When selecting stick 2, the
    second axis is seen on the input lines.
    Interrupting this sequence will lead to swapped axes. This is
    prevented by resetting the toggle when the mouse is deselected
    (neither 1 nor 2 are selected).

    The joystick lines are selected as follows:
    TI-99/4:  Stick 1: P4=1, P3=0, P2=1 (5)
    Stick 2: P4=1, P3=1, P2=0 (6)

    TI-99/4A: Stick 1: P4=1, P3=1, P2=0 (6)
    Stick 2: P4=1, P3=1, P2=1 (7)

    TI-99/8:  Stick 1: P3=1, P2=1, P1=1, P0=0 (14)
    Stick 2: P3=1, P2=1, P1=1, P0=1 (15)

    Geneve: n/a, has own mouse handling via v9938

    As we can only deliver at max 3 steps positive and 4 steps negative,
    we split the delta so that subsequent queries add up to the actual
    delta. That is, one delta of +10 yields a 3+3+3+1.

    mecmouse_x holds the current delta to be counted down for x
    (y accordingly)

    mecmouse_x_buf is the current step count reported to CRU

    Michael Zapf, 2008-01-22

    2010-10-22 Rewriten as device
    February 2012: Rewritten as class

*****************************************************************************/

#include "mecmouse.h"

#define POLL_TIMER 1

mecmouse_device::mecmouse_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
: device_t(mconfig, MECMOUSE, "Mechatronics mouse", tag, owner, clock)
{
}

void mecmouse_device::select(int selnow, int stick1, int stick2)
{
	if (selnow == stick2) {
		if (m_select == stick1) {
			if (!m_read_y)
			{
				/* Sample x motion. */
				if (m_x < -4)
					m_x_buf = -4;
				else if (m_x > 3)
					m_x_buf = 3;
				else
					m_x_buf = m_x;
				m_x -= m_x_buf;
				m_x_buf = (m_x_buf-1) & 7;
			}
			else
			{
				/* Sample y motion. */
				if (m_y < -4)
					m_y_buf = -4;
				else if (m_y > 3)
					m_y_buf = 3;
				else
					m_y_buf = m_y;
				m_y -= m_y_buf;
				m_y_buf = (m_y_buf-1) & 7;
			}
		}
		m_select = selnow;
	}
	else if (selnow == stick1)
	{
		if (m_select == stick2)
		{
			/* Swap the axes. */
			m_read_y = !m_read_y;
		}
		m_select = selnow;
	}
	else
	{
		/* Reset the axis toggle when the mouse is deselected */
		m_read_y = 0;
	}
}

void mecmouse_device::poll()
{
	int new_mx, new_my;
	int delta_x, delta_y;

	new_mx = input_port_read(machine(), "MOUSEX");
	new_my = input_port_read(machine(), "MOUSEY");

	/* compute x delta */
	delta_x = new_mx - m_last_mx;

	/* check for wrap */
	if (delta_x > 0x80)
		delta_x = -0x100+delta_x;
	if  (delta_x < -0x80)
		delta_x = 0x100+delta_x;

	/* Prevent unplausible values at startup. */
	if (delta_x > 100 || delta_x<-100) delta_x = 0;

	m_last_mx = new_mx;

	/* compute y delta */
	delta_y = new_my - m_last_my;

	/* check for wrap */
	if (delta_y > 0x80)
		delta_y = -0x100+delta_y;
	if  (delta_y < -0x80)
		delta_y = 0x100+delta_y;

	if (delta_y > 100 || delta_y<-100) delta_y = 0;

	m_last_my = new_my;

	/* update state */
	m_x += delta_x;
	m_y += delta_y;
}

int mecmouse_device::get_values()
{
	int answer;
	int buttons = input_port_read(machine(), "MOUSE0") & 3;
	answer = (m_read_y ? m_y_buf : m_x_buf) << 4;

	if ((buttons & 1)==0)
		/* action button */
		answer |= 0x08;
	if ((buttons & 2)==0)
		/* home button */
		answer |= 0x80;
	return answer;
}

/*
    Variation for TI-99/8
*/
int mecmouse_device::get_values8(int mode)
{
	int answer;
	int buttons = input_port_read(machine(), "MOUSE0") & 3;

	if (mode == 0)
	{
		answer = ((m_read_y ? m_y_buf : m_x_buf) << 7) & 0x80;
		if ((buttons & 1)==0)
			/* action button */
			answer |= 0x40;
	}
	else
	{
		answer = ((m_read_y ? m_y_buf : m_x_buf) << 1) & 0x03;
		if ((buttons & 2)==0)
			/* home button */
			answer |= 0x04;
	}
	return answer;
}

void mecmouse_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	// only one timer here
	poll();
}

void mecmouse_device::device_start(void)
{
	m_poll_timer = timer_alloc(POLL_TIMER);
	m_poll_timer->adjust(attotime::from_hz(clock()), 0, attotime::from_hz(clock()));
}

void mecmouse_device::device_reset(void)
{
	m_poll_timer->enable(true);
	m_select = 0;
	m_read_y = 0;
	m_x = 0;
	m_y = 0;
	m_last_mx = 0;
	m_last_my = 0;
}

const device_type MECMOUSE = &device_creator<mecmouse_device>;
