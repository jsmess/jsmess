/***************************************************************************

        MZ80 driver by Miodrag Milanovic

        22/11/2008 Preliminary driver.

****************************************************************************/

#include "emu.h"
#include "machine/i8255a.h"
#include "machine/pit8253.h"
#include "imagedev/cassette.h"
#include "sound/speaker.h"
#include "includes/mz80.h"

UINT8 mz80k_tempo_strobe = 0;
UINT8 mz80k_vertical = 0;
UINT8 mz80k_cursor_cnt = 0;


/* Driver initialization */
DRIVER_INIT(mz80k)
{
}

MACHINE_RESET( mz80k )
{
	mz80_state *state = machine.driver_data<mz80_state>();
	state->m_mz80k_tempo_strobe = 0;
	state->m_mz80k_8255_portc = 0;
	state->m_mz80k_vertical = 0;
	state->m_mz80k_cursor_cnt = 0;
	state->m_mz80k_keyboard_line = 0;
}


static READ8_DEVICE_HANDLER(mz80k_8255_portb_r)
{
	mz80_state *state = device->machine().driver_data<mz80_state>();
	static const char *const keynames[] = { "LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7", "LINE8", "LINE9" };
	if (state->m_mz80k_keyboard_line > 9) {
		return 0xff;
	} else {
		return input_port_read(device->machine(), keynames[state->m_mz80k_keyboard_line]);
	}
}

static READ8_DEVICE_HANDLER(mz80k_8255_portc_r)
{
	mz80_state *state = device->machine().driver_data<mz80_state>();
	UINT8 val = 0;
	val |= state->m_mz80k_vertical ? 0x80 : 0x00;
	val |= (state->m_mz80k_cursor_cnt > 31) ? 0x40 : 0x00;
    val |= (cassette_get_state(device->machine().device("cassette")) & CASSETTE_MASK_UISTATE)== CASSETTE_PLAY ? 0x10 : 0x00;

    if (cassette_input(device->machine().device("cassette")) > 0.00)
        val |= 0x20;

	return val;
}

static WRITE8_DEVICE_HANDLER(mz80k_8255_porta_w)
{
	mz80_state *state = device->machine().driver_data<mz80_state>();
	state->m_mz80k_keyboard_line = data & 0x0f;
}

static WRITE8_DEVICE_HANDLER(mz80k_8255_portc_w)
{
//  logerror("mz80k_8255_portc_w %02x\n",data);
}


static WRITE_LINE_DEVICE_HANDLER( pit_out0_changed )
{
	mz80_state *drvstate = device->machine().driver_data<mz80_state>();
	device_t *speaker = device->machine().device("speaker");
	if((drvstate->m_prev_state==0) && (state==1)) {
		drvstate->m_speaker_level ^= 1;
	}
	drvstate->m_prev_state = state;
	speaker_level_w( speaker, drvstate->m_speaker_level);
}

static WRITE_LINE_DEVICE_HANDLER( pit_out2_changed )
{
	cputag_set_input_line(device->machine(), "maincpu", 0, HOLD_LINE);
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
	mz80_state *state = space->machine().driver_data<mz80_state>();
	return(0x7e | state->m_mz80k_tempo_strobe);
}
WRITE8_HANDLER(mz80k_strobe_w)
{
	device_t *pit = space->machine().device("pit8253");
	pit8253_gate0_w(pit, BIT(data, 0));
}
