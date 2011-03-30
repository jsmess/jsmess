/*
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
*/

#include "emu.h"
#include "handset.h"
#include "machine/tms9901.h"

#define MAX_HANDSETS 4

typedef struct _ti99_handset_state
{
	int 		buf;
	int 		buflen;
	int 		clock;
	int 		ack;
	UINT8		previous_joy[MAX_HANDSETS];
	UINT8		previous_key[MAX_HANDSETS];

	emu_timer		*timer;
	device_t	*console9901;

} ti99_handset_state;

static const char *const joynames[2][4] =
{
	{ "JOY0", "JOY2", "JOY4", "JOY6" },		// X axis
	{ "JOY1", "JOY3", "JOY5", "JOY7" }		// Y axis
};

static const char *const keynames[] = { "KP0", "KP1", "KP2", "KP3", "KP4" };

INLINE ti99_handset_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == HANDSET);

	return (ti99_handset_state *)downcast<legacy_device_base *>(device)->token();
}

INLINE const ti99_handset_config *get_config(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == HANDSET);

	return (const ti99_handset_config *) downcast<const legacy_device_config_base &>(device->baseconfig()).inline_config();
}

/*****************************************************************************/

/*
    ti99_handset_poll_bus()

    Poll the current state of the 4-bit data bus that goes from the I/R
    receiver to the tms9901.
*/
int ti99_handset_poll_bus(device_t *device)
{
	ti99_handset_state *handset = get_safe_token(device);
	return (handset->buf & 0xf);
}

int ti99_handset_get_clock(device_t *device)
{
	ti99_handset_state *handset = get_safe_token(device);
	return handset->clock;
}

int ti99_handset_get_int(device_t *device)
{
	ti99_handset_state *handset = get_safe_token(device);
	return handset->buflen==3;
}

/*
    ti99_handset_ack_callback()

    Handle data acknowledge sent by the ti-99/4 handset ISR (through tms9901
    line P0).  This function is called by a delayed timer 30us after the state
    of P0 is changed, because, in one occasion, the ISR asserts the line before
    it reads the data, so we need to delay the acknowledge process.
*/
static TIMER_CALLBACK(ti99_handset_ack_callback)
{
	device_t *device = (device_t *)ptr;
	ti99_handset_state *handset = get_safe_token(device);

	handset->clock = !handset->clock;
	handset->buf >>= 4;
	handset->buflen--;

	tms9901_set_single_int(handset->console9901, 12, 0);

	if (handset->buflen == 1)
	{
		// Unless I am missing something, the third and last nybble of the
		// message is not acknowledged by the DSR in any way, and the first nybble
		// of next message is not requested for either, so we need to decide on
		// our own when we can post a new event.  Currently, we wait for 1000us
		// after the DSR acknowledges the second nybble.
		handset->timer->adjust(attotime::from_usec(1000));
	}

	if (handset->buflen == 0)
		/* See if we need to post a new event */
		ti99_handset_task(device);
}

/*
    ti99_handset_set_ack()
    Handler for tms9901 P0 pin (handset data acknowledge)
*/
void ti99_handset_set_acknowledge(device_t *device, UINT8 data)
{
	ti99_handset_state *handset = get_safe_token(device);
	if (handset->buflen && (data != handset->ack))
	{
		handset->ack = data;
		if (data == handset->clock)
			/* I don't know what the real delay is, but 30us apears to be enough */
			handset->timer->adjust(attotime::from_usec(30));
	}
}

/*
    ti99_handset_post_message()

    Post a 12-bit message: trigger an interrupt on the tms9901, and store the
    message in the I/R receiver buffer so that the handset ISR will read this
    message.

    message: 12-bit message to post (only the 12 LSBits are meaningful)
*/
static void ti99_handset_post_message(device_t *device, int message)
{
	ti99_handset_state *handset = get_safe_token(device);
	/* post message and assert interrupt */
	handset->clock = 1;
	handset->buf = ~message;
	handset->buflen = 3;

	tms9901_set_single_int(handset->console9901, 12, 1);
}

/*
    ti99_handset_poll_keyboard()
    Poll the current state of one given handset keypad.
    num: number of the keypad to poll (0-3)
    Returns TRUE if the handset state has changed and a message was posted.
*/
static int ti99_handset_poll_keyboard(device_t *device, int num)
{
	ti99_handset_state *handset = get_safe_token(device);
	UINT32 key_buf;
	UINT8 current_key;
	int i;

	/* read current key state */
	key_buf = ( input_port_read(device->machine(), keynames[num]) | (input_port_read(device->machine(), keynames[num + 1]) << 16) ) >> (4*num);

	// If a key was previously pressed, this key was not shift, and this key is
	// still down, then don't change the current key press.
	if (handset->previous_key[num] && (handset->previous_key[num] != 0x24)
		&& (key_buf & (1 << (handset->previous_key[num] & 0x1f))))
	{
		/* check the shift modifier state */
		if (((handset->previous_key[num] & 0x20) != 0) == ((key_buf & 0x0008) != 0))
			/* the shift modifier state has not changed */
		return FALSE;
		else
		{
			// The shift modifier state has changed: we need to update the
			// keyboard state
			if (key_buf & 0x0008)
			{	/* shift has been pressed down */
				handset->previous_key[num] = current_key = handset->previous_key[num] | 0x20;
			}
			else
			{	/* shift has been pressed down */
				handset->previous_key[num] = current_key = handset->previous_key[num] & ~0x20;
			}
			/* post message */
			ti99_handset_post_message(device, (((unsigned) current_key) << 4) | (num << 1));

			return TRUE;
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

	if (current_key != handset->previous_key[num])
	{
		handset->previous_key[num] = current_key;

		/* post message */
		ti99_handset_post_message(device, (((unsigned) current_key) << 4) | (num << 1));

		return TRUE;
	}
	return FALSE;
}

/*
    ti99_handset_poll_joystick()

    Poll the current state of one given handset joystick.
    num: number of the joystick to poll (0-3)
    Returns TRUE if the handset state has changed and a message was posted.
*/
static int ti99_handset_poll_joystick(device_t *device, int num)
{
	ti99_handset_state *handset = get_safe_token(device);

	UINT8 current_joy;
	int current_joy_x, current_joy_y;
	int message;

	/* read joystick position */
	current_joy_x = input_port_read(device->machine(), joynames[0][num]);
	current_joy_y = input_port_read(device->machine(), joynames[1][num]);

	/* compare with last saved position */
	current_joy = current_joy_x | (current_joy_y << 4);
	if (current_joy != handset->previous_joy[num])
	{
		/* save position */
		handset->previous_joy[num] = current_joy;

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
		ti99_handset_post_message(device, message);
		return TRUE;
	}
	return FALSE;
}

/*
    ti99_handset_task()
    Manage handsets, posting an event if the state of any handset has changed.
*/
void ti99_handset_task(device_t *device)
{
	int i;
	ti99_handset_state *handset = get_safe_token(device);

	if (handset->buflen == 0)
	{
		/* poll every handset */
		for (i=0; i < MAX_HANDSETS; i++)
			if (ti99_handset_poll_joystick(device, i)) return;
		for (i=0; i < MAX_HANDSETS; i++)
			if (ti99_handset_poll_keyboard(device, i)) return;
	}
	else if (handset->buflen == 3)
	{	/* update messages after they have been posted */
		if (handset->buf & 1)
		{	/* keyboard */
			ti99_handset_poll_keyboard(device, (~(handset->buf >> 1)) & 0x3);
		}
		else
		{	/* joystick */
			ti99_handset_poll_joystick(device, (~(handset->buf >> 1)) & 0x3);
		}
	}
}


/**************************************************************************/

static DEVICE_START( ti99_handset )
{
	ti99_handset_state *handset = get_safe_token(device);
	handset->timer = device->machine().scheduler().timer_alloc(FUNC(ti99_handset_ack_callback), (void *)device);
}

static DEVICE_RESET( ti99_handset )
{
	const ti99_handset_config* conf = (const ti99_handset_config*)get_config(device);
	ti99_handset_state *handset = get_safe_token(device);
	handset->console9901 = device->machine().device(conf->sysintf);
	handset->buf = 0;
	handset->buflen = 0;
	handset->clock = 0;
	handset->ack = 0;
}

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)             p##ti99_handset##s
#define DEVTEMPLATE_FEATURES            DT_HAS_START | DT_HAS_RESET | DT_HAS_INLINE_CONFIG
#define DEVTEMPLATE_NAME                "TI-99/4 Handset"
#define DEVTEMPLATE_FAMILY              "Human-Computer Interface"
#include "devtempl.h"

DEFINE_LEGACY_DEVICE( HANDSET, ti99_handset );

