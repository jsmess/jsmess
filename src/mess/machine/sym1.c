/******************************************************************************
    SYM-1

    PeT mess@utanet.at May 2000

******************************************************************************/
#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "video/generic.h"

#define VERBOSE_DBG 0
#include "includes/cbm.h"
#include "machine/6522via.h"
#include "machine/6532riot.h"
#include "includes/sym1.h"

static UINT8 riot_port_a, riot_port_b;

/*
  8903 7segment display output, key pressed
  892c keyboard input only ????
*/

static READ8_HANDLER( sym1_riot_a_r )
{
	int data = 0xff;

	if (!(riot_port_a & 0x80)) {
		data &= input_port_0_r(0);
	}

	if (!(riot_port_b & 0x01)) {
		data &= input_port_1_r(1);
	}

	if (!(riot_port_b & 0x02)) {
		data &= input_port_2_r(1);
	}

	if (!(riot_port_b & 0x04)) {
		data &= input_port_3_r(1);
	}

	if ( ((riot_port_a ^ 0xff) & (input_port_0_r(0) ^ 0xff)) & 0x3f )
		data &= ~0x80;

	return data;
}


static READ8_HANDLER( sym1_riot_b_r )
{
	int data = 0xff;

	if ( ((riot_port_a ^ 0xff) & (input_port_1_r(0) ^ 0xff)) & 0x3f )
		data &= ~1;

	if ( ((riot_port_a ^ 0xff) & (input_port_2_r(0) ^ 0xff)) & 0x3f )
		data &= ~2;

	if ( ((riot_port_a ^ 0xff) & (input_port_3_r(0) ^ 0xff)) & 0x3f )
		data &= ~4;

	data &= ~0x80; // else hangs 8b02

	return data;
}

static void sym1_led_w(int chip, int data)
{

	if ((riot_port_b &0xf)<6) {
		logerror("write 7seg(%d): %c%c%c%c%c%c%c\n",
				 data&7,
				 (riot_port_a & 0x01) ? 'a' : '.',
				 (riot_port_a & 0x02) ? 'b' : '.',
				 (riot_port_a & 0x04) ? 'c' : '.',
				 (riot_port_a & 0x08) ? 'd' : '.',
				 (riot_port_a & 0x10) ? 'e' : '.',
				 (riot_port_a & 0x20) ? 'f' : '.',
				 (riot_port_a & 0x40) ? 'g' : '.');
		sym1_led[riot_port_b] |= riot_port_a;
	}
}


static WRITE8_HANDLER( sym1_riot_a_w )
{
	riot_port_a = data;
	sym1_led_w(0, data);
}


static WRITE8_HANDLER( sym1_riot_b_w )
{
	riot_port_b = data;
	sym1_led_w(0, data);
}


static const struct R6532interface r6532_interface =
{
	1000000,
	0,
	sym1_riot_a_r,
	sym1_riot_b_r,
	sym1_riot_a_w,
	sym1_riot_b_w
};


static void sym1_irq(int level)
{
	cpunum_set_input_line(0, M6502_IRQ_LINE, level);
}

static struct via6522_interface via0={
#if 0
	int (*in_a_func)(int offset);
	int (*in_b_func)(int offset);
	int (*in_ca1_func)(int offset);
	int (*in_cb1_func)(int offset);
	int (*in_ca2_func)(int offset);
	int (*in_cb2_func)(int offset);
	void (*out_a_func)(int offset, int val);
	void (*out_b_func)(int offset, int val);
	void (*out_ca2_func)(int offset, int val);
	void (*out_cb2_func)(int offset, int val);
	void (*irq_func)(int state);

    /* kludges for the Vectrex */
	void (*out_shift_func)(int val);
	void (*t2_callback)(double time);
#endif
	0,
	0,
	0,
	0,
	0,
	0,

	0,
	0,
	0,
	0,
	0,
	0,
	sym1_irq
},
via1 = { 0 },
via2 = { 0 };


DRIVER_INIT( sym1 )
{
	via_config(0, &via0);
	via_config(1, &via1);
	via_config(2, &via2);
	r6532_init(0, &r6532_interface);
}

MACHINE_RESET( sym1 )
{
	via_reset();
	r6532_reset(0);
}

INTERRUPT_GEN( sym1_interrupt )
{
	int i;

	/* decrease the brightness of the six 7segment LEDs */
	for (i = 0; i < 6; i++)
	{
		if (videoram[i * 2 + 1] > 0)
			videoram[i * 2 + 1] -= 1;
	}
}

