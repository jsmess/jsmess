/***************************************************************************
    TI-99/4(A) and TI-99/8 main board

    Michael Zapf, October 2010
    TODO: Find out whether device can be determined more easily in CRU functions
***************************************************************************/

#include "emu.h"
#include "tiboard.h"
#include "gromport.h"
#include "video/tms9928a.h"
#include "handset.h"
#include "mecmouse.h"
#include "imagedev/cassette.h"
#include "mapper8.h"

typedef struct _tiboard_state
{
	/* Mode: Which console do we have? */
	int				mode;

	/* Keyboard support */
	int				keyCol;
	int				alphaLockLine;

	/* Devices */
	device_t	*cpu;
	device_t	*video;
	device_t  *tms9901;
	device_t	*peribox;
	device_t	*sound;

	device_t	*handset;
	device_t	*mecmouse;
	device_t	*gromport;

} tiboard_state;

static const char *const keynames[] = { "KEY0", "KEY1", "KEY2", "KEY3" };
static const char *const keynames8[] = {
		"KEY0", "KEY1", "KEY2", "KEY3", "KEY4", "KEY5", "KEY6", "KEY7",
		"KEY8", "KEY9", "KEY10", "KEY11", "KEY12", "KEY13", "KEY14", "KEY15"
	};

INLINE tiboard_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == TIBOARD);
	return (tiboard_state *)downcast<legacy_device_base *>(device)->token();
}

INLINE const tiboard_config *get_config(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == TIBOARD);

	return (const tiboard_config *) downcast<const legacy_device_config_base &>(device->baseconfig()).inline_config();
}

/****************************************************************************
    TMS9901 and attached devices handling
****************************************************************************/
/*
    VBL interrupt  (mmm...  actually, it happens when the beam enters the lower
    border, so it is not a genuine VBI, but who cares ?)
*/
INTERRUPT_GEN( ti99_vblank_interrupt )
{
	device_t *dev = device->machine().device("ti_board");
	tiboard_state *board = get_safe_token(dev);

	TMS9928A_interrupt(device->machine());

	if (board->handset != NULL)
		ti99_handset_task(board->handset);
	if (board->mecmouse != NULL)
		mecmouse_poll(board->mecmouse);
}

/*
    TI99/4x-specific tms9901 I/O handlers

    See mess/machine/tms9901.c for generic tms9901 CRU handlers.

    BTW, although TMS9900 is generally big-endian, it is little endian as far as CRU is
    concerned. (i.e. bit 0 is the least significant)

KNOWN PROBLEMS:
    * a read or write to bits 16-31 causes TMS9901 to quit timer mode.  The problem is:
      on a TI99/4A, any memory access causes a dummy CRU read.  Therefore, TMS9901 can quit
      timer mode even though the program did not explicitely ask... and THIS is impossible
      to emulate efficiently (we would have to check every memory operation).
*/
/*
    TMS9901 interrupt handling on a TI99/4(a).

    TI99/4(a) uses the following interrupts:
    INT1: external interrupt (used by RS232 controller, for instance)
    INT2: VDP interrupt
    TMS9901 timer interrupt (overrides INT3)
    INT12: handset interrupt (only on a TI-99/4 with the handset prototypes)

    Three (occasionally four) interrupts are used by the system (INT1, INT2,
    timer, and INT12 on a TI-99/4 with remote handset prototypes), out of 15/16
    possible interrupts.  Keyboard pins can be used as interrupt pins, too, but
    this is not emulated (it's a trick, anyway, and I don't know any program
    which uses it).

    When an interrupt line is set (and the corresponding bit in the interrupt mask is set),
    a level 1 interrupt is requested from the TMS9900.  This interrupt request lasts as long as
    the interrupt pin and the revelant bit in the interrupt mask are set.

    TIMER interrupts are kind of an exception, since they are not associated with an external
    interrupt pin, and I guess it takes a write to the 9901 CRU bit 3 ("SBO 3") to clear
    the pending interrupt (or am I wrong once again ?).

nota:
    All interrupt routines notify (by software) the TMS9901 of interrupt
    recognition (with a "SBO n").  However, unless I am missing something, this
    has absolutely no consequence on the TMS9901 (except for the TIMER
    interrupt routine), and interrupt routines would work fine without this
    SBO instruction.  This is quite weird.  Maybe the interrupt recognition
    notification is needed on TMS9985, or any other weird variant of TMS9900
    (how about the TI-99 development system connected to a TI990/10?).
*/

/*
    set the state of int2 (called by the tms9928 core)
*/
void tms9901_set_int2(running_machine &machine, int state)
{
	tms9901_set_single_int(machine.device("tms9901"), 2, state);
}

/*
    Called by the 9901 core whenever the state of INTREQ and IC0-3 changes
*/
static TMS9901_INT_CALLBACK( tms9901_interrupt_callback )
{
	if (intreq)
	{
		// On TI99, TMS9900 IC0-3 lines are not connected to TMS9901,
		// but hard-wired to force level 1 interrupts
		cputag_set_input_line_and_vector(device->machine(), "maincpu", 0, ASSERT_LINE, 1);	/* interrupt it, baby */
	}
	else
	{
		cputag_set_input_line(device->machine(), "maincpu", 0, CLEAR_LINE);
	}
}

/*
    ti99_handset_set_ack()
    Handler for tms9901 P0 pin (handset data acknowledge)
*/
static WRITE8_DEVICE_HANDLER( ti99_handset_set_ack )
{
	device_t *dev = device->machine().device("ti_board");
	tiboard_state *board = get_safe_token(dev);

	if (board->handset != NULL)
		ti99_handset_set_acknowledge(board->handset, data);
}

/*
    Read pins INT3*-INT7* of TI99's 9901.

    signification:
     (bit 1: INT1 status)
     (bit 2: INT2 status)
     bit 3-7: keyboard status bits 0 to 4
*/
static READ8_DEVICE_HANDLER( ti99_R9901_0 )
{
	int answer = 0;
	device_t *dev = device->machine().device("ti_board");
	tiboard_state *board = get_safe_token(dev);

	if (board->keyCol == 7)
	{
		if (board->handset != NULL) answer = (ti99_handset_poll_bus(board->handset) << 3);
		answer |= 0x80;
	}
	else
	{
		if ((board->mecmouse != NULL) && (board->keyCol == 6))
		{
			answer = mecmouse_get_values(board->mecmouse);
		}
		else
			answer = ((input_port_read(device->machine(), keynames[board->keyCol >> 1]) >> ((board->keyCol & 1) * 8)) << 3) & 0xF8;
	}
	return answer;
}

/*
    Read pins INT3*-INT7* of TI99's 9901.

    signification:
     (bit 1: INT1 status)
     (bit 2: INT2 status)
     bit 3-7: keyboard status bits 0 to 4
*/
static READ8_DEVICE_HANDLER( ti99_R9901_0a )
{
	int answer;
	device_t *dev = device->machine().device("ti_board");
	tiboard_state *board = get_safe_token(dev);

	if ((board->mecmouse != NULL) && (board->keyCol == 7))
		answer = mecmouse_get_values(board->mecmouse);
	else
		answer = ((input_port_read(device->machine(), keynames[board->keyCol >> 1]) >> ((board->keyCol & 1) * 8)) << 3) & 0xF8;

	if (board->alphaLockLine == FALSE)
		answer &= ~(input_port_read(device->machine(), "ALPHA") << 3);

	return answer;
}


/*
    Read pins INT8*-INT15* of TI99's 9901.

    signification:
     bit 0-2: keyboard status bits 5 to 7
     bit 3: tape input mirror
     (bit 4: IR remote handset interrupt)
     bit 5-7: weird, not emulated
*/
static READ8_DEVICE_HANDLER( ti99_R9901_1 )
{
	int answer;
	device_t *dev = device->machine().device("ti_board");
	tiboard_state *board = get_safe_token(dev);

	if (board->keyCol == 7)
		answer = 0x07;
	else
	{
		answer = ((input_port_read(device->machine(), keynames[board->keyCol >> 1]) >> ((board->keyCol & 1) * 8)) >> 5) & 0x07;
	}

	/* we don't take CS2 into account, as CS2 is a write-only unit */
	//  if (cassette_input(device->machine().device("cassette1")) > 0)
	//      answer |= 8;

	return answer;
}

/*
    Read pins P0-P7 of TI99's 9901.

     bit 1: handset data clock pin
*/
static READ8_DEVICE_HANDLER( ti99_R9901_2 )
{
	device_t *dev = device->machine().device("ti_board");
	tiboard_state *board = get_safe_token(dev);

	if (board->handset != NULL)
		if (ti99_handset_get_clock(board->handset)) return 2;
	return 0;
}

/*
    Read pins P8-P15 of TI99's 9901. (99/4)

     bit 26: IR handset interrupt
     bit 27: tape input
*/
static READ8_DEVICE_HANDLER( ti99_R9901_3 )
{
	int answer = 4;
	device_t *dev = device->machine().device("ti_board");
	tiboard_state *board = get_safe_token(dev);

	/* on systems without handset, the pin is pulled up to avoid spurious interrupts */
	if (board->handset != NULL)
		if (ti99_handset_get_int(board->handset)) answer = 0;

	/* we don't take CS2 into account, as CS2 is a write-only unit */
	if (cassette_input(device->machine().device("cassette1")) > 0)
		answer |= 8;

	return answer;
}

/*
    Read pins P8-P15 of TI99's 9901. (TI-99/8)

     bit 26: high
     bit 27: tape input
*/
static READ8_DEVICE_HANDLER( ti99_8_R9901_3 )
{
	int answer = 4;	/* on systems without handset, the pin is pulled up to avoid spurious interrupts */
	if (cassette_input(device->machine().device("cassette")) > 0)
		answer |= 8;
	return answer;
}

/*
    WRITE key column select (P2-P4), TI-99/4
*/
static WRITE8_DEVICE_HANDLER( ti99_KeyC )
{
	device_t *dev = device->machine().device("ti_board");
	tiboard_state *board = get_safe_token(dev);

	int index=5;

	if (board->mode==TI994A)
		index=6;

	if (data)
		board->keyCol |= 1 << (offset-2);
	else
		board->keyCol &= ~ (1 << (offset-2));

	if (board->mecmouse != NULL)
		mecmouse_select(board->mecmouse, board->keyCol, index , index+1);
}

/*
    WRITE alpha lock line - TI99/4a only (P5)
*/
static WRITE8_DEVICE_HANDLER( ti99_AlphaW )
{
	device_t *dev = device->machine().device("ti_board");
	tiboard_state *board = get_safe_token(dev);
	board->alphaLockLine = data;
}

/*
    command CS1 tape unit motor (P6)
*/
static WRITE8_DEVICE_HANDLER( ti99_CS1_motor )
{
	device_t *img = device->machine().device("cassette1");
	cassette_change_state(img, data ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);
}

/*
    command CS2 tape unit motor (P7)
*/
static WRITE8_DEVICE_HANDLER( ti99_CS2_motor )
{
	device_t *img = device->machine().device("cassette2");
	cassette_change_state(img, data ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);
}

/*
    command CS1 (only) tape unit motor (P6)
*/
static WRITE8_DEVICE_HANDLER( ti99_CS_motor )
{
	device_t *img = device->machine().device("cassette");
	cassette_change_state(img, data ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);
}

/*
    audio gate (P8)

    Set to 1 before using tape: this enables the mixing of tape input sound
    with computer sound.

    We do not really need to emulate this as the tape recorder generates sound
    on its own.
*/
static WRITE8_DEVICE_HANDLER( ti99_audio_gate )
{
}

/*
    tape output (P9)
    I think polarity is correct, but don't take my word for it.
*/
static WRITE8_DEVICE_HANDLER( ti99_CS12_output )
{
	cassette_output(device->machine().device("cassette1"), data ? +1 : -1);
	cassette_output(device->machine().device("cassette2"), data ? +1 : -1);
}

/*
    tape output (P9)
    I think polarity is correct, but don't take my word for it.
*/
static WRITE8_DEVICE_HANDLER( ti99_CS_output )
{
	cassette_output(device->machine().device("cassette"), data ? +1 : -1);
}

/*
    WRITE key column select (P0-P3)
*/
static WRITE8_DEVICE_HANDLER( ti99_8_KeyC )
{
	device_t *dev = device->machine().device("ti_board");
	tiboard_state *board = get_safe_token(dev);

	if (data)
		board->keyCol |= 1 << offset;
	else
		board->keyCol &= ~ (1 << offset);

	if (board->mecmouse != NULL)
		mecmouse_select(board->mecmouse, board->keyCol, 14, 15);
}

static WRITE8_DEVICE_HANDLER( ti99_8_CRUS )
{
	device_t *mapper = device->machine().device("mapper");
	mapper8_CRUS_w(mapper, 0, data);
}

static WRITE8_DEVICE_HANDLER( ti99_8_PTGEN )
{
	device_t *mapper = device->machine().device("mapper");
	mapper8_PTGEN_w(mapper, 0, data);
}

/*
    Read pins INT3*-INT7* of TI99's 9901.

    signification:
     (bit 1: INT1 status)
     (bit 2: INT2 status)
     bits 3-4: unused?
     bit 5: ???
     bit 6-7: keyboard status bits 0 through 1
*/
static READ8_DEVICE_HANDLER( ti99_8_R9901_0 )
{
	int answer;
	device_t *dev = device->machine().device("ti_board");
	tiboard_state *board = get_safe_token(dev);

	if (board->mecmouse != NULL && (board->keyCol == 15))
	{
		answer = mecmouse_get_values8(board->mecmouse, 0);
	}
	else
	{
		answer = (input_port_read(device->machine(), keynames8[board->keyCol]) << 6) & 0xC0;
	}

	return answer;
}


/*
    Read pins INT8*-INT15* of TI99's 9901.

    signification:
     bit 0-2: keyboard status bits 2 to 4
     bit 3: tape input mirror
     (bit 4: IR remote handset interrupt)
     bit 5-7: weird, not emulated
*/
static READ8_DEVICE_HANDLER( ti99_8_R9901_1 )
{
	device_t *dev = device->machine().device("ti_board");
	tiboard_state *board = get_safe_token(dev);
	int answer;

	if (board->mecmouse != NULL && (board->keyCol == 15))
	{
		answer = mecmouse_get_values8(board->mecmouse, 1);
	}
	else
	{
		answer = (input_port_read(device->machine(), keynames8[board->keyCol]) >> 2) & 0x07;
	}

	return answer;
}

/*
    Read pins P0-P7 of TI99's 9901.

     bit 1: handset data clock pin
*/
static READ8_DEVICE_HANDLER( ti99_8_R9901_2 )
{
	return 0;
}


/* TMS9901 setup. The callback functions pass a reference to the TMS9901 as device. */
const tms9901_interface tms9901_wiring_ti99_4 =
{
	TMS9901_INT1 | TMS9901_INT2 | TMS9901_INTC,	/* only input pins whose state is always known */

	{	/* read handlers */
		ti99_R9901_0,
		ti99_R9901_1,
		ti99_R9901_2,
		ti99_R9901_3
	},

	{	/* write handlers */
		ti99_handset_set_ack,
		NULL,
		ti99_KeyC,
		ti99_KeyC,
		ti99_KeyC,
		NULL,
		ti99_CS1_motor,
		ti99_CS2_motor,
		ti99_audio_gate,
		ti99_CS12_output,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	},

	/* interrupt handler */
	tms9901_interrupt_callback,

	/* clock rate = 3MHz */
	3000000.
};

/* TMS9901 setup. The callback functions pass a reference to the TMS9901 as device. */
const tms9901_interface tms9901_wiring_ti99_4a =
{
	TMS9901_INT1 | TMS9901_INT2 | TMS9901_INTC,	/* only input pins whose state is always known */

	{	/* read handlers */
		ti99_R9901_0a,
		ti99_R9901_1,
		ti99_R9901_2,
		ti99_R9901_3
	},

	{	/* write handlers */
		NULL,
		NULL,
		ti99_KeyC,
		ti99_KeyC,
		ti99_KeyC,
		ti99_AlphaW,
		ti99_CS1_motor,
		ti99_CS2_motor,
		ti99_audio_gate,
		ti99_CS12_output,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	},

	/* interrupt handler */
	tms9901_interrupt_callback,

	/* clock rate = 3MHz */
	3000000.
};

const tms9901_interface tms9901_wiring_ti99_8 =
{
	TMS9901_INT1 | TMS9901_INT2 | TMS9901_INTC,	/* only input pins whose state is always known */

	{	/* read handlers */
		ti99_8_R9901_0,
		ti99_8_R9901_1,
		ti99_8_R9901_2,
		ti99_8_R9901_3
	},

	{	/* write handlers */
		ti99_8_KeyC,
		ti99_8_KeyC,
		ti99_8_KeyC,
		ti99_8_KeyC,
		ti99_8_CRUS,
		ti99_8_PTGEN,
		ti99_CS_motor,
		NULL,
		ti99_audio_gate,
		ti99_CS_output,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	},

	/* interrupt handler */
	tms9901_interrupt_callback,

	/* clock rate = 3.58MHz??? 2.68MHz??? */
	/*3579545.*/2684658.75
};

/*
    Handles the EXTINT line. This one is directly connected to the 9901.
*/
WRITE_LINE_DEVICE_HANDLER( console_extint )
{
	tms9901_set_single_int(device->machine().device("tms9901"), 1, state);
}

WRITE_LINE_DEVICE_HANDLER( console_notconnected )
{
	logerror("Not connected line set ... sorry\n");
}

WRITE_LINE_DEVICE_HANDLER( console_ready )
{
	logerror("READY line set ... not yet connected, level=%02x\n", state);
}

/***************************************************************************
    DEVICE LIFECYCLE FUNCTIONS
***************************************************************************/

static DEVICE_START( tiboard )
{
	tiboard_state *board = get_safe_token(device);
	logerror("Starting TI-99 board\n");
	board->tms9901 = device->siblingdevice("tms9901");
	board->gromport = device->siblingdevice("gromport");
}

static DEVICE_STOP( tiboard )
{
	logerror("Stopping TI-99 board\n");
}

static DEVICE_RESET( tiboard )
{
	tiboard_state *board = get_safe_token(device);
	const tiboard_config* conf = (const tiboard_config*)get_config(device);
	board->mode = conf->mode;

	/* clear keyboard interface state (probably overkill, but can't harm) */
	board->keyCol = 0;
	board->alphaLockLine = 0;
	board->handset = NULL;
	board->mecmouse = NULL;

	if (board->mode == TI994)
	{
		/* reset handset */
		tms9901_set_single_int(board->tms9901, 12, 0);

		if (input_port_read(device->machine(), "HCI") & HCI_IR)
			board->handset = device->siblingdevice("handset");
	}

	if (input_port_read(device->machine(), "HCI") & HCI_MECMOUSE)
		board->mecmouse = device->siblingdevice("mecmouse");

	if (board->mode != TI998)
	{
		set_gk_switches(board->gromport, 1, input_port_read(device->machine(), "GKSWITCH1"));
		set_gk_switches(board->gromport, 2, input_port_read(device->machine(), "GKSWITCH2"));
		set_gk_switches(board->gromport, 3, input_port_read(device->machine(), "GKSWITCH3"));
		set_gk_switches(board->gromport, 4, input_port_read(device->machine(), "GKSWITCH4"));
		set_gk_switches(board->gromport, 5, input_port_read(device->machine(), "GKSWITCH5"));
	}
}

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)             p##tiboard##s
#define DEVTEMPLATE_FEATURES            DT_HAS_START | DT_HAS_STOP | DT_HAS_RESET | DT_HAS_INLINE_CONFIG
#define DEVTEMPLATE_NAME                "TI-99 system board"
#define DEVTEMPLATE_FAMILY              "Internal component"
#include "devtempl.h"

DEFINE_LEGACY_DEVICE( TIBOARD, tiboard );
