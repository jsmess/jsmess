/******************************************************************************
 Synertek Systems Corp. SYM-1

 Early driver by PeT mess@utanet.at May 2000
 Rewritten by Dirk Best October 2007

******************************************************************************/


#include "driver.h"
#include "includes/sym1.h"

/* M6502 CPU */
#include "cpu/m6502/m6502.h"

/* Peripheral chips */
#include "machine/6522via.h"
#include "machine/6532riot.h"


static UINT8 riot_port_a, riot_port_b;
static UINT8 sym1_led[6];



/******************************************************************************
 6532 RIOT
******************************************************************************/


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


static void sym1_led_w(void)
{
	if ((riot_port_b & 0x0f) < 6)
	{
		sym1_led[riot_port_b] |= riot_port_a;
		output_set_digit_value(riot_port_b & 0x07, sym1_led[riot_port_b]);
	}
}


static WRITE8_HANDLER( sym1_riot_a_w )
{
	riot_port_a = data;
	logerror("riot_a_w 0x%02x\n", data);
	sym1_led_w();
}


static WRITE8_HANDLER( sym1_riot_b_w )
{
	riot_port_b = data;
	logerror("riot_b_w 0x%02x\n", data);
	sym1_led_w();
}


static const struct riot6532_interface r6532_interface =
{
	sym1_riot_a_r,
	sym1_riot_b_r,
	sym1_riot_a_w,
	sym1_riot_b_w
};



/******************************************************************************
 6522 VIA
******************************************************************************/


static void sym1_irq(int level)
{
	cpunum_set_input_line(0, M6502_IRQ_LINE, level);
}


static struct via6522_interface via0 =
{
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	sym1_irq
};


static struct via6522_interface via1 =
{
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	sym1_irq
};


static struct via6522_interface via2 =
{
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	sym1_irq
};



/******************************************************************************
 Driver init and reset
******************************************************************************/


DRIVER_INIT( sym1 )
{
	/* install memory */
	memory_install_readwrite8_handler(0, ADDRESS_SPACE_PROGRAM,
			0x0400, mess_ram_size - 0x0400 - 1, 0, 0, MRA8_RAM, MWA8_RAM);

	via_config(0, &via0);
	via_config(1, &via1);
	via_config(2, &via2);
	r6532_config(0, &r6532_interface);
	r6532_set_clock(0, OSC_Y1);
	r6532_reset(0);
}


MACHINE_RESET( sym1 )
{
	via_reset();
	r6532_reset(0);

	/* Make 0xf800 to 0xffff point to the last half of the monitor ROM */
	/* so that the CPU can find its reset vectors */
	memory_install_readwrite8_handler(0, ADDRESS_SPACE_PROGRAM,
			0xf800, 0xffff, 0, 0, MRA8_BANK1, MWA8_ROM);
	memory_configure_bank(1, 0, 1, sym1_monitor + 0x800, 0);
	memory_set_bank(1, 0);
}
