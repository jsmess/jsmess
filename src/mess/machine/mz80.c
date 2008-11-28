/***************************************************************************

        MZ80 driver by Miodrag Milanovic

        22/11/2008 Preliminary driver.

****************************************************************************/

#include "driver.h"
#include "machine/8255ppi.h"
#include "machine/pit8253.h"
#include "devices/cassette.h"
#include "sound/speaker.h"
#include "includes/mz80.h"
 
UINT8 mz80k_tempo_strobe = 0;
UINT8 mz80k_8255_portc = 0;
UINT8 mz80k_vertical = 0;
UINT8 mz80k_cursor_cnt = 0;
UINT8 mz80k_keyboard_line = 0;


/* Driver initialization */
DRIVER_INIT(mz80k)
{
}


MACHINE_RESET( mz80k )
{
	mz80k_tempo_strobe = 0;
	mz80k_8255_portc = 0;
	mz80k_vertical = 0;
	mz80k_cursor_cnt = 0;
	mz80k_keyboard_line = 0;
}


READ8_DEVICE_HANDLER(mz80k_8255_portb_r)
{
	static const char *const keynames[] = { "LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7", "LINE8", "LINE9" };
	if (mz80k_keyboard_line > 9) {
		return 0xff;
	} else {
		return input_port_read(device->machine, keynames[mz80k_keyboard_line]);
	}
}

READ8_DEVICE_HANDLER(mz80k_8255_portc_r)
{
	UINT8 val = 0;
	val |= mz80k_vertical ? 0x80 : 0x00;
	val |= (mz80k_cursor_cnt > 31) ? 0x40 : 0x00;
    val |= (cassette_get_state(device_list_find_by_tag( device->machine->config->devicelist, CASSETTE, "cassette" )) & CASSETTE_MASK_UISTATE)== CASSETTE_PLAY ? 0x10 : 0x00;

    if (cassette_input(device_list_find_by_tag( device->machine->config->devicelist, CASSETTE, "cassette" )) > 0.00)
        val |= 0x20;

	return val;
}

WRITE8_DEVICE_HANDLER(mz80k_8255_porta_w) 
{	
	mz80k_keyboard_line = data & 0x0f;	
}

WRITE8_DEVICE_HANDLER(mz80k_8255_portc_w) 
{	
//	logerror("mz80k_8255_portc_w %02x\n",data);
}

static PIT8253_OUTPUT_CHANGED( pit_out0_changed )
{
	speaker_level_w( 0, state ? 1 : 0 );
}


static PIT8253_OUTPUT_CHANGED( pit_out1_changed )
{
	pit8253_set_clock_signal( device, 2, state );
}


static PIT8253_OUTPUT_CHANGED( pit_out2_changed )
{
	cpu_set_input_line(device->machine->cpu[0], 0, HOLD_LINE);
}

const ppi8255_interface mz80k_8255_int =
{
	NULL,
	mz80k_8255_portb_r,
	mz80k_8255_portc_r,
	mz80k_8255_porta_w,
	NULL,
	mz80k_8255_portc_w,
};

const struct pit8253_config mz80k_pit8253_config =
{
	{
		/* clockin	  irq callback	  */
		{ 1108800.0,  pit_out0_changed },		
		{	15611.0,  pit_out1_changed },
		{		  0,  pit_out2_changed },
	}
};

READ8_HANDLER(mz80k_strobe_r)
{
    mz80k_tempo_strobe^=1;
	return(0x7e | mz80k_tempo_strobe);
}
WRITE8_HANDLER(mz80k_strobe_w)
{
}

