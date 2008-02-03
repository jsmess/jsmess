/*****************************************************************************
 *
 * machine/74145.c
 *
 * BCD-to-Decimal decoder
 *
 *        __ __
 *     0-|  v  |-VCC
 *     1-|     |-A
 *     2-|     |-B
 *     3-|     |-C
 *     4-|     |-D
 *     5-|     |-9
 *     6-|     |-8
 *   GND-|_____|-7
 *
 *
 * Truth table
 *  _______________________________
 * | Inputs  | Outputs             |
 * | D C B A | 0 1 2 3 4 5 6 7 8 9 |
 * |-------------------------------|
 * | L L L L | L H H H H H H H H H |
 * | L L L H | H L H H H H H H H H |
 * | L L H L | H H L H H H H H H H |
 * | L L H H | H H H L H H H H H H |
 * | L H L L | H H H H L H H H H H |
 * |-------------------------------|
 * | L H L H | H H H H H L H H H H |
 * | L H H L | H H H H H H L H H H |
 * | L H H H | H H H H H H H L H H |
 * | H L L L | H H H H H H H H L H |
 * | H L L H | H H H H H H H H H L |
 * |-------------------------------|
 * | H L H L | H H H H H H H H H H |
 * | H L H H | H H H H H H H H H H |
 * | H H L L | H H H H H H H H H H |
 * | H H L H | H H H H H H H H H H |
 * | H H H L | H H H H H H H H H H |
 * | H H H H | H H H H H H H H H H |
 *  -------------------------------
 *
 ****************************************************************************/

#include "driver.h"
#include "deprecat.h"
#include "74145.h"



/*****************************************************************************
 Macros
*****************************************************************************/


#define MAX_74145   (1)



/*****************************************************************************
 Type definitions
*****************************************************************************/


typedef struct _ttl74145_state ttl74145_state;
struct _ttl74145_state
{
	/* Pointer to our interface */
	const ttl74145_interface *intf;

	/* Decoded number */
	UINT16 number;
};



/*****************************************************************************
 Global variables
*****************************************************************************/


static ttl74145_state ttl74145[MAX_74145];



/*****************************************************************************
 Implementation
*****************************************************************************/


/* Config */
void ttl74145_config(int which, const ttl74145_interface *intf)
{
	assert_always(mame_get_phase(Machine) == MAME_PHASE_INIT,
		"Can only call ttl74145_config at init time!");
	assert_always(which < MAX_74145,
		"'which' exceeds maximum number of configured 74145s!");

	/* Assign interface */
	ttl74145[which].intf = intf;

	/* Initialize */
	ttl74145_reset(which);
}


/* Reset */
void ttl74145_reset(int which)
{
	ttl74145[which].number = 0;
}


/* Data Write */
static void ttl74145_write(int which, offs_t offset, UINT8 data)
{
	/* Decode number */
	UINT16 new_number = bcd_2_dec(data & 0x0f);

	/* Call output callbacks if the number changed */
	if (new_number != ttl74145[which].number)
	{
		const ttl74145_interface *i = ttl74145[which].intf;

		if (i->output_line_0) i->output_line_0(new_number == 0);
		if (i->output_line_1) i->output_line_1(new_number == 1);
		if (i->output_line_2) i->output_line_2(new_number == 2);
		if (i->output_line_3) i->output_line_3(new_number == 3);
		if (i->output_line_4) i->output_line_4(new_number == 4);
		if (i->output_line_5) i->output_line_5(new_number == 5);
		if (i->output_line_6) i->output_line_6(new_number == 6);
		if (i->output_line_7) i->output_line_7(new_number == 7);
		if (i->output_line_8) i->output_line_8(new_number == 8);
		if (i->output_line_9) i->output_line_9(new_number == 9);
	}

	/* Update state */
	ttl74145[which].number = new_number;
}


WRITE8_HANDLER( ttl74145_0_w ) { ttl74145_write(0, offset, data); }
READ16_HANDLER( ttl74145_0_r ) { return ttl74145[0].number; }
