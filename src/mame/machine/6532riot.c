/***************************************************************************

  RIOT 6532 emulation

TODO:
- irq callback for timer overflow

***************************************************************************/

#include "driver.h"
#include "machine/6532riot.h"


struct R6532
{
	struct R6532interface intf;

	UINT8 DRA;
	UINT8 DRB;
	UINT8 DDRA;
	UINT8 DDRB;

	int shift;
	int cleared;

	UINT32 target;

	int pa7_enable;
	int pa7_direction;	/* 0 = high-to-low, 1 = low-to-high */
	int pa7_flag;
	UINT8 pa7_last;
};


static struct R6532* r6532[2];


static UINT8 r6532_combineA(int n, UINT8 val)
{
	return (r6532[n]->DDRA & r6532[n]->DRA) | (~r6532[n]->DDRA & val);
}


static UINT8 r6532_combineB(int n, UINT8 val)
{
	return (r6532[n]->DDRB & r6532[n]->DRB) | (~r6532[n]->DDRB & val);
}


static UINT8 r6532_read_portA(int n)
{
	if (r6532[n]->intf.portA_r != NULL)
	{
		return r6532_combineA(n, r6532[n]->intf.portA_r(0));
	}

	logerror("Read from unhandled 6532 #%d port A\n", n);

	return 0;
}


static UINT8 r6532_read_portB(int n)
{
	if (r6532[n]->intf.portB_r != NULL)
	{
		return r6532_combineB(n, r6532[n]->intf.portB_r(2));
	}

	logerror("Read from unhandled 6532 #%d port B\n", n);

	return 0;
}


static void r6532_write_portA(int n, UINT8 data)
{
	r6532[n]->DRA = data;

	if (r6532[n]->intf.portA_w != NULL)
	{
		r6532[n]->intf.portA_w(0, r6532_combineA(n, 0xFF));
	}
	else
		logerror("Write %02x to unhandled 6532 #%d port A\n", data, n);
}


static void r6532_write_portB(int n, UINT8 data)
{
	r6532[n]->DRB = data;

	if (r6532[n]->intf.portB_w != NULL)
	{
		r6532[n]->intf.portB_w(0, r6532_combineB(n, 0xFF));
	}
	else
		logerror("Write %02x to unhandled 6532 #%d port B\n", data, n);
}


static void r6532_write(int n, offs_t offset, UINT8 data)
{
	if (offset & 4)
	{
		if (offset & 0x10)
		{
			r6532[n]->cleared = 0;

			switch (offset & 3)
			{
			case 0:
				r6532[n]->shift = 0;
				break;
			case 1:
				r6532[n]->shift = 3;
				break;
			case 2:
				r6532[n]->shift = 6;
				break;
			case 3:
				r6532[n]->shift = 10;
				break;
			}

			r6532[n]->target = activecpu_gettotalcycles() + (data << r6532[n]->shift);
		}
		else
		{
			r6532[n]->pa7_enable = (offset & 2) >> 1;
			r6532[n]->pa7_direction = offset & 1;
		}
	}
	else
	{
		offset &= 3;

		switch (offset)
		{
		case 0:
			r6532_write_portA(n, data);
			break;
		case 1:
			r6532[n]->DDRA = data;
			break;
		case 2:
			r6532_write_portB(n, data);
			break;
		case 3:
			r6532[n]->DDRB = data;
			break;
		}
	}
}


static UINT8 r6532_read_timer(int n, int enable)
{
	int count = ( r6532[n]->target ? r6532[n]->target : 5 ) - activecpu_gettotalcycles();

	if (count >= 0)
	{
		return count >> r6532[n]->shift;
	}
	else
	{
		if (count != -1)
		{
			r6532[n]->cleared = 1;
		}

		return count & 0xFF;
	}
}


static void r6532_PA7_write(int n, offs_t offset, UINT8 data)
{
	data &= 0x80;

	if ((r6532[n]->pa7_last ^ data) &&
			((r6532[n]->pa7_direction == 0 && !data) ||
			 (r6532[n]->pa7_direction == 1 &&  data)))
	{
		r6532[n]->pa7_flag = 1;
		if (r6532[n]->pa7_enable)
		{
			if (r6532[n]->intf.irq_func != NULL)
				(*r6532[n]->intf.irq_func)(ASSERT_LINE);
			else
				logerror("No irq handler for 6532 #%d PA7 edge detect\n", n);
		}
	}

	r6532[n]->pa7_last = data;
}



static UINT8 r6532_read_irq_flags(int n)
{
	int count = r6532[n]->target - activecpu_gettotalcycles();
	int res = 0;

	if (count < 0 && !r6532[n]->cleared)
		res |= 0x80;

	if (r6532[n]->pa7_flag)
	{
		res |= 0x40;
		r6532[n]->pa7_flag = 0;

		if (r6532[n]->intf.irq_func != NULL)
		{
			/* TODO: shouldn't clear line if timer irq pending */
			(*r6532[n]->intf.irq_func)(CLEAR_LINE);
		}
	}

	return res;
}


static UINT8 r6532_read(int n, offs_t offset)
{
	UINT8 val = 0;

	switch (offset & 7)
	{
	case 0:
		val = r6532_read_portA(n);
		break;
	case 1:
		val = r6532[n]->DDRA;
		break;
	case 2:
		val = r6532_read_portB(n);
		break;
	case 3:
		val = r6532[n]->DDRB;
		break;
	case 4:
		val = r6532_read_timer(n, 0);
		break;
	case 5:
		val = r6532_read_irq_flags(n);
		break;
	case 6:
		val = r6532_read_timer(n, 1);
		break;
	case 7:
		val = r6532_read_irq_flags(n);
		break;
	}

	return val;
}


WRITE8_HANDLER( r6532_0_w ) { r6532_write(0, offset, data); }
WRITE8_HANDLER( r6532_1_w ) { r6532_write(1, offset, data); }

READ8_HANDLER( r6532_0_r ) { return r6532_read(0, offset); }
READ8_HANDLER( r6532_1_r ) { return r6532_read(1, offset); }

WRITE8_HANDLER( r6532_0_PA7_w ) { r6532_PA7_write(0, offset, data); }
WRITE8_HANDLER( r6532_1_PA7_w ) { r6532_PA7_write(1, offset, data); }


void r6532_init(int n, const struct R6532interface* intf)
{
	assert_always(mame_get_phase(Machine) == MAME_PHASE_INIT, "Can only call r6532_init at init time!");

	r6532[n] = auto_malloc(sizeof(struct R6532));

	r6532[n]->intf = *intf;

	r6532[n]->DRA = 0;
	r6532[n]->DRB = 0;
	r6532[n]->DDRA = 0;
	r6532[n]->DDRB = 0;

	r6532[n]->shift = 0;
	r6532[n]->cleared = 0;

	r6532[n]->target = 0;

	r6532[n]->pa7_enable = 0;
	r6532[n]->pa7_direction = 0;
	r6532[n]->pa7_flag = 0;
	r6532[n]->pa7_last = 0;

	if (r6532[n]->intf.irq_func != NULL)
		(*r6532[n]->intf.irq_func)(CLEAR_LINE);
}
