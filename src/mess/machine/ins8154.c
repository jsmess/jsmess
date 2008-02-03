/*****************************************************************************
 *
 * machine/ins8154.h
 * 
 * INS8154 N-Channel 128-by-8 Bit RAM Input/Output (RAM I/O)
 * 
 * Written by Dirk Best, January 2008
 *
 * TODO: Strobed modes
 * 
 ****************************************************************************/

#include "driver.h"
#include "deprecat.h"
#include "ins8154.h"

#define MAX_INS8154   2

#define VERBOSE 1
#define LOG(x)	do { if (VERBOSE) logerror x; } while (0)


/* Mode Definition Register */
enum
{
	MDR_BASIC                 = 0x00,
	MDR_STROBED_INPUT         = 0x20,
	MDR_STROBED_OUTPUT        = 0x60,
	MDR_STROBED_OUTPUT_3STATE = 0xe0
};


typedef struct _ins8154_state ins8154_state;
struct _ins8154_state
{
	/* Pointer to our interface */
	const ins8154_interface *intf;
	
	UINT8 in_a;  /* Input Latch Port A */
	UINT8 in_b;  /* Input Latch Port B */
	UINT8 out_a; /* Output Latch Port A */
	UINT8 out_b; /* Output Latch Port B */
	UINT8 mdr;   /* Mode Definition Register */
	UINT8 odra;  /* Output Definition Register Port A */
	UINT8 odrb;  /* Output Definition Register Port B */
};

static ins8154_state ins8154[MAX_INS8154];


/* Config */
void ins8154_config(int which, const ins8154_interface *intf)
{
	assert_always(mame_get_phase(Machine) == MAME_PHASE_INIT,
		"Can only call ins8154_config at init time!");
	assert_always(which < MAX_INS8154,
		"'which' exceeds maximum number of configured INS8154s!");

	/* Assign interface */
	ins8154[which].intf = intf;

	/* Initialize */
	ins8154_reset(which);
}


/* Reset */
void ins8154_reset(int which)
{
	ins8154[which].in_a = 0;
	ins8154[which].in_b = 0;
	ins8154[which].out_a = 0;
	ins8154[which].out_b = 0;
	ins8154[which].mdr = 0;
	ins8154[which].odra = 0;
	ins8154[which].odrb = 0;
}


static UINT8 ins8154_read(int which, offs_t offset)
{
	ins8154_state *i = &ins8154[which];
	UINT8 val = 0xff;
	
	if (offset > 0x24)
	{
		logerror("INS8154 chip #%d (%08x): Read from unknown offset %02x!\n",
			which, safe_activecpu_get_pc(), offset);
		return 0xff;
	}
	
	switch (offset)
	{
	case 0x20:
		if (i->intf->in_a_func)
			val = i->intf->in_a_func(0);
		i->in_a = val;
		break;
		
	case 0x21:
		if (i->intf->in_b_func)
			val = i->intf->in_b_func(0);
		i->in_b = val;
		break;
		
	default:
		if (offset < 0x08)
		{
			if (i->intf->in_a_func)
				val = (i->intf->in_a_func(0) << (8 - offset)) & 0x80;
			i->in_a = val;
		}
		else
		{
			if (i->intf->in_a_func)
				val = (i->intf->in_a_func(0) << (8 - (offset >> 4))) & 0x80;
			i->in_b = val;
		}
		break;
	}
	
	return val;
}


static void ins8154_write_port_a(int which, int data)
{
	ins8154_state *i = &ins8154[which];
	
	i->out_a = data;

	/* Test if any pins are set as outputs */
	if (i->odra)
	{
		if (i->intf->out_a_func)
			i->intf->out_a_func(0, (data & i->odra) | (i->odra ^ 0xff));
		else
			logerror("INS8154 chip #%d (%08x): Write to port A but no write handler defined!\n",
				which, safe_activecpu_get_pc());
	}
}


static void ins8154_write_port_b(int which, int data)
{
	ins8154_state *i = &ins8154[which];
	
	i->out_b = data;
	
	/* Test if any pins are set as outputs */
	if (i->odrb)
	{
		if (i->intf->out_b_func)
			i->intf->out_b_func(0, (data & i->odrb) | (i->odrb ^ 0xff));
		else
			logerror("INS8154 chip #%d (%08x): Write to port B but no write handler defined!\n",
				which, safe_activecpu_get_pc());
	}
}


static void ins8154_write(int which, offs_t offset, UINT8 data)
{
	ins8154_state *i = &ins8154[which];
	
	if (offset > 0x24)
	{
		logerror("INS8154 chip %d (%04x): Write %02x to invalid offset %02x!\n",
			which, safe_activecpu_get_pc(), data, offset);
		return;
	}
	
	switch (offset)
	{
	case 0x20:
		ins8154_write_port_a(which, data);
		break;
		
	case 0x21:
		ins8154_write_port_b(which, data);
		break;
		
	case 0x22:
		LOG(("INS8154 chip %d (%04x): ODR for port A set to %02x\n",
			which, safe_activecpu_get_pc(), data));
		i->odra = data;
		break;
		
	case 0x23:
		LOG(("INS8154 chip %d (%04x): ODR for port B set to %02x\n",
			which, safe_activecpu_get_pc(), data));
		i->odrb = data;
		break;
		
	case 0x24:
		LOG(("INS8154 chip %d (%04x): MDR set to %02x\n",
			which, safe_activecpu_get_pc(), data));
		i->mdr = data;
		break;
		
	default:
		if (offset & 0x10)
		{
			/* Set bit */
			if (offset < 0x08)
			{
				ins8154_write_port_a(which, i->out_a |= offset & 0x07);
			}
			else
			{
				ins8154_write_port_b(which, i->out_b |= (offset >> 4) & 0x07);
			}
		}
		else
		{
			/* Clear bit */
			if (offset < 0x08)
			{
				ins8154_write_port_a(which, i->out_a & ~(offset & 0x07));
			}
			else
			{
				ins8154_write_port_b(which, i->out_b & ~((offset >> 4) & 0x07));
			}			
		}
		
		break;
	}
}



/******************* Standard 8-bit CPU interfaces, D0-D7 *******************/

WRITE8_HANDLER( ins8154_0_w ) {	ins8154_write(0, offset, data); }
WRITE8_HANDLER( ins8154_1_w ) {	ins8154_write(1, offset, data); }

READ8_HANDLER( ins8154_0_r ) { return ins8154_read(0, offset); }
READ8_HANDLER( ins8154_1_r ) { return ins8154_read(1, offset); }
