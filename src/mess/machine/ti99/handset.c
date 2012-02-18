/****************************************************************************

    TI-99/4 Handset support (TI99/4 only)

    The ti99/4 was intended to support some so-called "IR remote handsets".
    This feature was canceled at the last minute (reportedly ten minutes before
    the introductory press conference in June 1979), but the first thousands of
    99/4 units had the required port, and the support code was seemingly not
    deleted from ROMs until the introduction of the ti99/4a in 1981.  You could
    connect up to 4 20-key keypads, and up to 4 joysticks with a maximum
    resolution of 15 levels on each axis.

    The keyboard DSR was able to couple two 20-key keypads together to emulate
    a full 40-key keyboard.  Keyboard modes 0, 1 and 2 would select either the
    console keyboard with its two wired remote controllers (i.e. joysticks), or
    remote handsets 1 and 2 with their associated IR remote controllers (i.e.
    joysticks), according to which was currently active.

    Originally written by R. Nabet

    Michael Zapf
    2010-10-24 Rewriten as device

    January 2012: Rewritten as class

*****************************************************************************/

#include "handset.h"
#include "machine/tms9901.h"

#define LOG logerror
#define VERBOSE 1

static const char *const joynames[2][4] =
{
	{ "JOY0", "JOY2", "JOY4", "JOY6" },		// X axis
	{ "JOY1", "JOY3", "JOY5", "JOY7" }		// Y axis
};

static const char *const keynames[] = { "KP0", "KP1", "KP2", "KP3", "KP4" };

ti99_handset_device::ti99_handset_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
: device_t(mconfig, HANDSET, "TI-99/4 IR handset", tag, owner, clock)
{
}

#define POLL_TIMER 1
#define DELAY_TIMER 2

/*****************************************************************************/

/*
    Handle data acknowledge sent by the ti-99/4 handset ISR (through tms9901
    line P0).  This function is called by a delayed timer 30us after the state
    of P0 is changed, because, in one occasion, the ISR asserts the line before
    it reads the data, so we need to delay the acknowledge process.
*/
void ti99_handset_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	if (id==DELAY_TIMER)
	{
		m_clock = !m_clock;
		m_buf >>= 4;
		m_buflen--;

		// Clear the INT12 line
		m_interrupt(CLEAR_LINE);

		if (m_buflen == 1)
		{
			// Unless I am missing something, the third and last nibble of the
			// message is not acknowledged by the DSR in any way, and the first nibble
			// of next message is not requested for either, so we need to decide on
			// our own when we can post a new event.  Currently, we wait for 1000us
			// after the DSR acknowledges the second nybble.
			m_delay_timer->adjust(attotime::from_usec(1000));
		}

		if (m_buflen == 0)
			/* See if we need to post a new event */
			do_task();
	}
	else
	{
		// Poll timer
		do_task();
	}
}

/*
    Handler for tms9901 P0 pin (handset data acknowledge)
*/
void ti99_handset_device::set_acknowledge(int data)
{
	if ((m_buflen !=0) && (data != m_ack))
	{
		m_ack = data;
		if (data == m_clock)
			/* I don't know what the real delay is, but 30us apears to be enough */
		m_delay_timer->adjust(attotime::from_usec(30));
	}
}

/*
    post_message()

    Post a 12-bit message: trigger an interrupt on the tms9901, and store the
    message in the I/R receiver buffer so that the handset ISR will read this
    message.

    message: 12-bit message to post (only the 12 LSBits are meaningful)
*/
void ti99_handset_device::post_message(int message)
{
	/* post message and assert interrupt */
	m_clock = 1;
	m_buf = ~message;
	m_buflen = 3;
	m_interrupt(ASSERT_LINE);
}

/*
    poll_keyboard()
    Poll the current state of one given handset keypad.
    num: number of the keypad to poll (0-3)
    Returns TRUE if the handset state has changed and a message was posted.
*/
bool ti99_handset_device::poll_keyboard(int num)
{
	UINT32 key_buf;
	UINT8 current_key;
	int i;

	/* read current key state */
	key_buf = (input_port_read(machine(), keynames[num]) | (input_port_read(machine(), keynames[num + 1]) << 16) ) >> (4*num);

	// If a key was previously pressed, this key was not shift, and this key is
	// still down, then don't change the current key press.
	if (previous_key[num] !=0 && (previous_key[num] != 0x24)
		&& (key_buf & (1 << (previous_key[num] & 0x1f))))
	{
		/* check the shift modifier state */
		if (((previous_key[num] & 0x20) != 0) == ((key_buf & 0x0008) != 0))
			/* the shift modifier state has not changed */
			return false;
		else
		{
			// The shift modifier state has changed: we need to update the
			// keyboard state
			if (key_buf & 0x0008)
			{	/* shift has been pressed down */
				previous_key[num] = current_key = previous_key[num] | 0x20;
			}
			else
			{
				previous_key[num] = current_key = previous_key[num] & ~0x20;
			}
			/* post message */
			post_message((((unsigned)current_key) << 4) | (num << 1));
			return true;
		}

	}

	current_key = 0;	/* default value if no key is down */
	for (i=0; i<20; i++)
	{
		if (key_buf & (1 << i))
		{
			current_key = i + 1;
			if (key_buf & 0x0008)
				current_key |= 0x20;	/* set shift flag */

			if (current_key != 0x24)
				// If this is the shift key, any other key we may find will
				// have higher priority; otherwise, we may exit the loop and keep
				// the key we have just found.
				break;
		}
	}

	if (current_key != previous_key[num])
	{
		previous_key[num] = current_key;

		/* post message */
		post_message((((unsigned) current_key) << 4) | (num << 1));
		return true;
	}
	return false;
}

/*
    poll_joystick()

    Poll the current state of one given handset joystick.
    num: number of the joystick to poll (0-3)
    Returns TRUE if the handset state has changed and a message was posted.
*/
bool ti99_handset_device::poll_joystick(int num)
{
	UINT8 current_joy;
	int current_joy_x, current_joy_y;
	int message;
	/* read joystick position */
	current_joy_x = input_port_read(machine(), joynames[0][num]);
	current_joy_y = input_port_read(machine(), joynames[1][num]);

	/* compare with last saved position */
	current_joy = current_joy_x | (current_joy_y << 4);
	if (current_joy != previous_joy[num])
	{
		/* save position */
		previous_joy[num] = current_joy;

		/* transform position to signed quantity */
		current_joy_x -= 7;
		current_joy_y -= 7;

		message = 0;

		/* set sign */
		// note that we set the sign if the joystick position is 0 to work
		// around a bug in the ti99/4 ROMs
		if (current_joy_x <= 0)
		{
			message |= 0x040;
			current_joy_x = - current_joy_x;
		}

		if (current_joy_y <= 0)
		{
			message |= 0x400;
			current_joy_y = - current_joy_y;
		}

		/* convert absolute values to Gray code and insert in message */
		if (current_joy_x & 4)
			current_joy_x ^= 3;
		if (current_joy_x & 2)
			current_joy_x ^= 1;
		message |= current_joy_x << 3;

		if (current_joy_y & 4)
			current_joy_y ^= 3;
		if (current_joy_y & 2)
			current_joy_y ^= 1;
		message |= current_joy_y << 7;

		/* set joystick address */
		message |= (num << 1) | 0x1;

		/* post message */
		post_message(message);
		return true;
	}
	return false;
}

/*
    ti99_handset_task()
    Manage handsets, posting an event if the state of any handset has changed.
*/
void ti99_handset_device::do_task()
{
	int i;

	if (m_buflen == 0)
	{
		/* poll every handset */
		for (i=0; i < MAX_HANDSETS; i++)
			if (poll_joystick(i)==true) return;
		for (i=0; i < MAX_HANDSETS; i++)
			if (poll_keyboard(i)==true) return;
	}
	else if (m_buflen == 3)
	{	/* update messages after they have been posted */
		if (m_buf & 1)
		{	/* keyboard */
			poll_keyboard((~(m_buf >> 1)) & 0x3);
		}
		else
		{	/* joystick */
			poll_joystick((~(m_buf >> 1)) & 0x3);
		}
	}
}

void ti99_handset_device::device_start(void)
{
	const ti99_handset_intf *intf = reinterpret_cast<const ti99_handset_intf *>(static_config());

	m_interrupt.resolve(intf->interrupt, *this);
	m_delay_timer = timer_alloc(DELAY_TIMER);
	m_poll_timer = timer_alloc(POLL_TIMER);
	m_poll_timer->adjust(attotime::from_hz(clock()), 0, attotime::from_hz(clock()));
}

void ti99_handset_device::device_reset(void)
{
	if (VERBOSE>5) LOG("ti99_handset_device reset\n");
	m_delay_timer->enable(true);
	m_poll_timer->enable(true);
	m_buf = 0;
	m_buflen = 0;
	m_clock = 0;
	m_ack = 0;
}

const device_type HANDSET = &device_creator<ti99_handset_device>;
