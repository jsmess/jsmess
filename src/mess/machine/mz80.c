/***************************************************************************

        MZ80 driver by Miodrag Milanovic

        22/11/2008 Preliminary driver.

****************************************************************************/

#include "emu.h"
#include "machine/i8255a.h"
#include "machine/pit8253.h"
#include "devices/cassette.h"
#include "sound/speaker.h"
#include "includes/mz80.h"

UINT8 mz80k_tempo_strobe = 0;
static UINT8 mz80k_8255_portc = 0;
UINT8 mz80k_vertical = 0;
UINT8 mz80k_cursor_cnt = 0;
static UINT8 mz80k_keyboard_line = 0;


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


static READ8_DEVICE_HANDLER(mz80k_8255_portb_r)
{
	static const char *const keynames[] = { "LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7", "LINE8", "LINE9" };
	if (mz80k_keyboard_line > 9) {
		return 0xff;
	} else {
		return input_port_read(device->machine, keynames[mz80k_keyboard_line]);
	}
}

static READ8_DEVICE_HANDLER(mz80k_8255_portc_r)
{
	UINT8 val = 0;
	val |= mz80k_vertical ? 0x80 : 0x00;
	val |= (mz80k_cursor_cnt > 31) ? 0x40 : 0x00;
    val |= (cassette_get_state(devtag_get_device(device->machine, "cassette")) & CASSETTE_MASK_UISTATE)== CASSETTE_PLAY ? 0x10 : 0x00;

    if (cassette_input(devtag_get_device(device->machine, "cassette")) > 0.00)
        val |= 0x20;

	return val;
}

static WRITE8_DEVICE_HANDLER(mz80k_8255_porta_w)
{
	mz80k_keyboard_line = data & 0x0f;
}

static WRITE8_DEVICE_HANDLER(mz80k_8255_portc_w)
{
//  logerror("mz80k_8255_portc_w %02x\n",data);
}

static UINT8 speaker_level = 0;
static UINT8 prev_state = 0;

static WRITE_LINE_DEVICE_HANDLER( pit_out0_changed )
{
	running_device *speaker = devtag_get_device(device->machine, "speaker");
	if((prev_state==0) && (state==1)) {
		speaker_level ^= 1;
	}
	prev_state = state;
	speaker_level_w( speaker, speaker_level);
}

static WRITE_LINE_DEVICE_HANDLER( pit_out2_changed )
{
	cputag_set_input_line(device->machine, "maincpu", 0, HOLD_LINE);
}

I8255A_INTERFACE( mz80k_8255_int )
{
	DEVCB_NULL,
	DEVCB_HANDLER(mz80k_8255_portb_r),
	DEVCB_HANDLER(mz80k_8255_portc_r),
	DEVCB_HANDLER(mz80k_8255_porta_w),
	DEVCB_NULL,
	DEVCB_HANDLER(mz80k_8255_portc_w),
};

const struct pit8253_config mz80k_pit8253_config =
{
	{
		/* clockin        gate        callback    */
		{ XTAL_8MHz/  4,  DEVCB_NULL, DEVCB_LINE(pit_out0_changed) },
		{ XTAL_8MHz/256,  DEVCB_NULL, DEVCB_LINE(pit8253_clk2_w)   },
		{		      0,  DEVCB_NULL, DEVCB_LINE(pit_out2_changed) },
	}
};

READ8_HANDLER(mz80k_strobe_r)
{
	return(0x7e | mz80k_tempo_strobe);
}
WRITE8_HANDLER(mz80k_strobe_w)
{
	running_device *pit = devtag_get_device(space->machine, "pit8253");
	pit8253_gate0_w(pit, BIT(data, 0));
}
