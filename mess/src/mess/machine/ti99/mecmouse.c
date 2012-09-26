/*
    TI-99 Mechatronic mouse with adapter
    Michael Zapf, October 2010

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
*/

#include "emu.h"
#include "mecmouse.h"

typedef struct _ti99_mecmouse_state
{
	int select;
	int read_y;
	int x;
	int y;
	int x_buf;
	int y_buf;
	int last_mx;
	int last_my;

} ti99_mecmouse_state;

INLINE ti99_mecmouse_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == MECMOUSE);

	return (ti99_mecmouse_state *)downcast<legacy_device_base *>(device)->token();
}

void mecmouse_select(device_t *device, int selnow, int stick1, int stick2)
{
	ti99_mecmouse_state *mouse = get_safe_token(device);

	if (selnow == stick2) {
		if (mouse->select == stick1) {
			if (!mouse->read_y)
			{
				/* Sample x motion. */
				if (mouse->x < -4)
					mouse->x_buf = -4;
				else if (mouse->x > 3)
					mouse->x_buf = 3;
				else
					mouse->x_buf = mouse->x;
				mouse->x -= mouse->x_buf;
				mouse->x_buf = (mouse->x_buf-1) & 7;
			}
			else
			{
				/* Sample y motion. */
				if (mouse->y < -4)
					mouse->y_buf = -4;
				else if (mouse->y > 3)
					mouse->y_buf = 3;
				else
					mouse->y_buf = mouse->y;
				mouse->y -= mouse->y_buf;
				mouse->y_buf = (mouse->y_buf-1) & 7;
			}
		}
		mouse->select = selnow;
	}
	else if (selnow == stick1)
	{
		if (mouse->select == stick2)
		{
			/* Swap the axes. */
			mouse->read_y = !mouse->read_y;
		}
		mouse->select = selnow;
	}
	else
	{
		/* Reset the axis toggle when the mouse is deselected */
		mouse->read_y = 0;
	}
}

void mecmouse_poll(device_t *device)
{
	ti99_mecmouse_state *mouse = get_safe_token(device);

	int new_mx, new_my;
	int delta_x, delta_y;

	new_mx = input_port_read(device->machine(), "MOUSEX");
	new_my = input_port_read(device->machine(), "MOUSEY");

	/* compute x delta */
	delta_x = new_mx - mouse->last_mx;

	/* check for wrap */
	if (delta_x > 0x80)
		delta_x = -0x100+delta_x;
	if  (delta_x < -0x80)
		delta_x = 0x100+delta_x;

	/* Prevent unplausible values at startup. */
	if (delta_x > 100 || delta_x<-100) delta_x = 0;

	mouse->last_mx = new_mx;

	/* compute y delta */
	delta_y = new_my - mouse->last_my;

	/* check for wrap */
	if (delta_y > 0x80)
		delta_y = -0x100+delta_y;
	if  (delta_y < -0x80)
		delta_y = 0x100+delta_y;

	if (delta_y > 100 || delta_y<-100) delta_y = 0;

	mouse->last_my = new_my;

	/* update state */
	mouse->x += delta_x;
	mouse->y += delta_y;
}

int mecmouse_get_values(device_t *device)
{
	ti99_mecmouse_state *mouse = get_safe_token(device);
	int answer;
	int buttons = input_port_read(device->machine(), "MOUSE0") & 3;
	answer = (mouse->read_y ? mouse->y_buf : mouse->x_buf) << 4;

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
int mecmouse_get_values8(device_t *device, int mode)
{
	ti99_mecmouse_state *mouse = get_safe_token(device);
	int answer;
	int buttons = input_port_read(device->machine(), "MOUSE0") & 3;

	if (mode == 0)
	{
		answer = ((mouse->read_y ? mouse->y_buf : mouse->x_buf) << 7) & 0x80;
		if ((buttons & 1)==0)
			/* action button */
			answer |= 0x40;
	}
	else
	{
		answer = ((mouse->read_y ? mouse->y_buf : mouse->x_buf) << 1) & 0x03;
		if ((buttons & 2)==0)
			/* home button */
			answer |= 0x04;
	}
	return answer;
}

/**************************************************************************/

static DEVICE_START( ti99_mecmouse )
{
}

static DEVICE_RESET( ti99_mecmouse )
{
	ti99_mecmouse_state *mouse = get_safe_token(device);
	mouse->select = 0;
	mouse->read_y = 0;
	mouse->x = 0;
	mouse->y = 0;
	mouse->last_mx = 0;
	mouse->last_my = 0;
}

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)             p##ti99_mecmouse##s
#define DEVTEMPLATE_FEATURES            DT_HAS_START | DT_HAS_RESET
#define DEVTEMPLATE_NAME                "Mechatronics mouse"
#define DEVTEMPLATE_FAMILY              "Human-computer interface"
#include "devtempl.h"

DEFINE_LEGACY_DEVICE( MECMOUSE, ti99_mecmouse );

