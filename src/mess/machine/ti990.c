/*
	machine/ti990.c

	Emulation for a few generic aspects of TI990
*/

#include "driver.h"
#include "ti990.h"

/*
	Interrupt priority encoder.  Actually part of the CPU board.
*/
static UINT16 intlines;

void ti990_reset_int(void)
{
	intlines = 0;
}

void ti990_set_int_line(int line, int state)
{
	int level;


	if (state)
		intlines |= (1 << line);
	else
		intlines &= ~ (1 << line);

	if (intlines)
	{
		for (level = 0; ! (intlines & (1 << level)); level++)
			;
		cpunum_set_input_line_and_vector(0, 0, ASSERT_LINE, level);	/* interrupt it, baby */
	}
	else
		cpunum_set_input_line(0, 0, CLEAR_LINE);
}

void ti990_set_int2(int state)
{
	ti990_set_int_line(2, state);
}

void ti990_set_int3(int state)
{
	ti990_set_int_line(3, state);
}

void ti990_set_int6(int state)
{
	ti990_set_int_line(6, state);
}

void ti990_set_int7(int state)
{
	ti990_set_int_line(7, state);
}

void ti990_set_int9(int state)
{
	ti990_set_int_line(9, state);
}

void ti990_set_int10(int state)
{
	ti990_set_int_line(10, state);
}

void ti990_set_int13(int state)
{
	ti990_set_int_line(13, state);
}

/*
	hold and debounce load line (emulation is inaccurate)
*/

static void clear_load(int dummy)
{
	cpunum_set_input_line(0, INPUT_LINE_NMI, CLEAR_LINE);
}

void ti990_hold_load(void)
{
	cpunum_set_input_line(0, INPUT_LINE_NMI, ASSERT_LINE);
	timer_set(TIME_IN_MSEC(100), 0, clear_load);
}

/*
	line interrupt
*/

/* ckon_state: 1 if line clock active (RTCLR flip-flop on TI990/10 schematics -
SMI sheet 4) */
static char ckon_state;

void ti990_line_interrupt(void)
{
	if (ckon_state)
		ti990_set_int_line(5, 1);
}

void ti990_ckon_ckof_callback(int state)
{
	ckon_state = state;
	if (! ckon_state)
		ti990_set_int_line(5, 0);
}



/*
	Control panel emulation

	three panel types
	* operator panel
	* programmer panel
	* MDU (external unit connected instead of the control panel, as seen in
	  945401-9701 p. 2-5 though 2-15)

	Operator panel:
	* Power led
	* Fault led
	* Off/On/Load switch

	Programmer panel:
	* 16 status light, 32 switches, IDLE, RUN leds
	* interface to a low-level debugger in ROMs

	* MDU:
	* includes a programmer panel, a tape unit, and a few parts
	  (diagnostic tape, diagnostic ROMs, etc.)

	CRU output:
	0-7: lights 0-7
	8: increment scan
	9: clear scan (according to 990 handbook)
	A: run light (additionally sets all data LEDs to 1s, the scan count to 0b10 and enables the HALT/SIE switch)
	B: fault light
	C: Memory Error Interrupt clear
	D: Start panel timer
	E: Set SIE function (interrupt after 2 instructions are executed)
	F: flag (according to 990 handbook)

	input :
	0-7: switches 0-7 (or data from MDU tape)
	8: scan count bit 1
	9: scan count bit 0
	A: timer active
	B: programmer panel not present or locked
	C: char in MDU tape unit buffer?
	D: unused?
	E: if 0, MDU unit present
	F: flag (according to 990 handbook)
*/

 READ8_HANDLER ( ti990_panel_read )
{
	if (offset == 1)
		return 0x48;

	return 0;
}

WRITE8_HANDLER ( ti990_panel_write )
{
}


/*
	CPU board soft reset (RSET instruction)
*/
void ti990_cpuboard_reset(void)
{
	ckon_state = 0;
}
