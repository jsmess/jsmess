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
#include "machine/74145.h"
#include "sound/speaker.h"


#define LED_REFRESH_DELAY  ATTOTIME_IN_USEC(70)


static UINT8 riot_port_a, riot_port_b;
static emu_timer *led_update;



/******************************************************************************
 6532 RIOT
******************************************************************************/


static void sym1_74145_output_0_w(int state)
{
	if (state) timer_adjust(led_update, LED_REFRESH_DELAY, 0, attotime_never);
}


static void sym1_74145_output_1_w(int state)
{
	if (state) timer_adjust(led_update, LED_REFRESH_DELAY, 1, attotime_never);
}


static void sym1_74145_output_2_w(int state)
{
	if (state) timer_adjust(led_update, LED_REFRESH_DELAY, 2, attotime_never);
}


static void sym1_74145_output_3_w(int state)
{
	if (state) timer_adjust(led_update, LED_REFRESH_DELAY, 3, attotime_never);
}


static void sym1_74145_output_4_w(int state)
{
	if (state) timer_adjust(led_update, LED_REFRESH_DELAY, 4, attotime_never);
}


static void sym1_74145_output_5_w(int state)
{
	if (state) timer_adjust(led_update, LED_REFRESH_DELAY, 5, attotime_never);
}


static TIMER_CALLBACK( led_refresh )
{
	output_set_digit_value(param, riot_port_a);
}


/* The speaker is connected to output 6 of the 74145 */
static void sym1_74145_output_6_w(int state)
{
	speaker_level_w(0, state);	
}


static READ8_HANDLER( sym1_riot_a_r )
{
	int data = 0x7f;

	/* scan keypad rows */
	if (!(riot_port_a & 0x80)) data &= readinputportbytag("ROW-0");
	if (!(riot_port_b & 0x01)) data &= readinputportbytag("ROW-1");
	if (!(riot_port_b & 0x02)) data &= readinputportbytag("ROW-2");
	if (!(riot_port_b & 0x04)) data &= readinputportbytag("ROW-3");

	/* determine column */
	if ( ((riot_port_a ^ 0xff) & (readinputportbytag("ROW-0") ^ 0xff)) & 0x7f )
		data &= ~0x80;

	return data;
}


static READ8_HANDLER( sym1_riot_b_r )
{
	int data = 0xff;

	/* determine column */
	if ( ((riot_port_a ^ 0xff) & (readinputportbytag("ROW-1") ^ 0xff)) & 0x7f )
		data &= ~1;

	if ( ((riot_port_a ^ 0xff) & (readinputportbytag("ROW-2") ^ 0xff)) & 0x3f )
		data &= ~2;

	if ( ((riot_port_a ^ 0xff) & (readinputportbytag("ROW-3") ^ 0xff)) & 0x1f )
		data &= ~4;

	data &= ~0x80; // else hangs 8b02

	return data;
}


static WRITE8_HANDLER( sym1_riot_a_w )
{
	logerror("%x: riot_a_w 0x%02x\n", activecpu_get_pc(), data);

	/* save for later use */
	riot_port_a = data;
}


static WRITE8_HANDLER( sym1_riot_b_w )
{
	logerror("%x: riot_b_w 0x%02x\n", activecpu_get_pc(), data);

	/* save for later use */
	riot_port_b = data;

	/* first 4 pins are connected to the 74145 */
	ttl74145_0_w(0, data & 0x0f);
}


static const struct riot6532_interface r6532_interface =
{
	sym1_riot_a_r,
	sym1_riot_b_r,
	sym1_riot_a_w,
	sym1_riot_b_w
};


static const ttl74145_interface ttl74145_intf =
{
	sym1_74145_output_0_w,  /* connected to DS0 */
	sym1_74145_output_1_w,  /* connected to DS1 */
	sym1_74145_output_2_w,  /* connected to DS2 */
	sym1_74145_output_3_w,  /* connected to DS3 */
	sym1_74145_output_4_w,  /* connected to DS4 */
	sym1_74145_output_5_w,  /* connected to DS5 */
	sym1_74145_output_6_w,  /* connected to the speaker */
	NULL,                   /* not connected */
	NULL,                   /* not connected */
	NULL                    /* not connected */
};


/******************************************************************************
 6522 VIA
******************************************************************************/


static void sym1_irq(int level)
{
	cpunum_set_input_line(0, M6502_IRQ_LINE, level);
}


static READ8_HANDLER( sym1_via0_b_r )
{
	return 0xff;
}


static WRITE8_HANDLER( sym1_via0_b_w )
{
	logerror("%x: via0_b_w 0x%02x\n", activecpu_get_pc(), data);
}


/* PA0: Write protect R6532 RAM
 * PA1: Write protect RAM 0x400-0x7ff
 * PA2: Write protect RAM 0x800-0xbff
 * PA3: Write protect RAM 0xc00-0xfff
 */
static WRITE8_HANDLER( sym1_via2_a_w )
{
	logerror("SYM1 VIA2 W 0x%02x\n", data);

	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa600, 0xa67f, 0, 0,  
		((readinputportbytag("WP") & 0x01) && !(data & 0x01)) ? MWA8_NOP : MWA8_BANK5);

	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM,	0x0400, 0x07ff, 0, 0, 
		((readinputportbytag("WP") & 0x02) && !(data & 0x02)) ? MWA8_NOP : MWA8_BANK2);

	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM,	0x0800, 0x0bff, 0, 0, 
		((readinputportbytag("WP") & 0x04) && !(data & 0x04)) ? MWA8_NOP : MWA8_BANK3);

	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM,	0x0c00, 0x0fff, 0, 0, 
		((readinputportbytag("WP") & 0x08) && !(data & 0x08)) ? MWA8_NOP : MWA8_BANK4);
}


static const struct via6522_interface via0 =
{
	NULL,           /* VIA Port A Input */
	sym1_via0_b_r,  /* VIA Port B Input */
	NULL,           /* VIA Port CA1 Input */
	NULL,           /* VIA Port CB1 Input */
	NULL,           /* VIA Port CA2 Input */
	NULL,           /* VIA Port CB2 Input */
	NULL,           /* VIA Port A Output */
	sym1_via0_b_w,  /* VIA Port B Output */
	NULL,           /* VIA Port CA1 Output */
	NULL,           /* VIA Port CB1 Output */
	NULL,           /* VIA Port CA2 Output */
	NULL,           /* VIA Port CB2 Output */
	sym1_irq        /* VIA IRQ Callback */
};


static const struct via6522_interface via1 =
{
	NULL,           /* VIA Port A Input */
	NULL,           /* VIA Port B Input */
	NULL,           /* VIA Port CA1 Input */
	NULL,           /* VIA Port CB1 Input */
	NULL,           /* VIA Port CA2 Input */
	NULL,           /* VIA Port CB2 Input */
	NULL,           /* VIA Port A Output */
	NULL,           /* VIA Port B Output */
	NULL,           /* VIA Port CA1 Output */
	NULL,           /* VIA Port CB1 Output */
	NULL,           /* VIA Port CA2 Output */
	NULL,           /* VIA Port CB2 Output */
	sym1_irq        /* VIA IRQ Callback */
};


static const struct via6522_interface via2 =
{
	NULL,           /* VIA Port A Input */
	NULL,           /* VIA Port B Input */
	NULL,           /* VIA Port CA1 Input */
	NULL,           /* VIA Port CB1 Input */
	NULL,           /* VIA Port CA2 Input */
	NULL,           /* VIA Port CB2 Input */
	sym1_via2_a_w,  /* VIA Port A Output */
	NULL,           /* VIA Port B Output */
	NULL,           /* VIA Port CA1 Output */
	NULL,           /* VIA Port CB1 Output */
	NULL,           /* VIA Port CA2 Output */
	NULL,           /* VIA Port CB2 Output */
	sym1_irq        /* VIA IRQ Callback */
};



/******************************************************************************
 Driver init and reset
******************************************************************************/


DRIVER_INIT( sym1 )
{
	/* wipe expansion memory banks that are not installed */
	if (mess_ram_size < 4*1024)
	{
		memory_install_readwrite8_handler(0, ADDRESS_SPACE_PROGRAM,
			mess_ram_size, 0x0fff, 0, 0, MRA8_NOP, MWA8_NOP);
	}

	/* configure vias and riot */
	via_config(0, &via0);
	via_config(1, &via1);
	via_config(2, &via2);
	r6532_config(0, &r6532_interface);
	r6532_set_clock(0, OSC_Y1);
	r6532_reset(0);
	
	/* configure 74145 */
	ttl74145_config(0, &ttl74145_intf);
	
	/* allocate a timer to refresh the led display */
	led_update = timer_alloc(led_refresh, NULL);
}


MACHINE_RESET( sym1 )
{
	via_reset();
	r6532_reset(0);
	ttl74145_reset(0);

	/* make 0xf800 to 0xffff point to the last half of the monitor ROM
	   so that the CPU can find its reset vectors */
	memory_install_readwrite8_handler(0, ADDRESS_SPACE_PROGRAM,
			0xf800, 0xffff, 0, 0, MRA8_BANK1, MWA8_ROM);
	memory_set_bankptr(1, sym1_monitor + 0x800);
}
