/*****************************************************************************
 *
 * machine/74145.h
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
	UINT16 number;
};



/*****************************************************************************
 Global variables
*****************************************************************************/


static ttl74145_state ttl74145[MAX_74145];



/*****************************************************************************
 Implementation
*****************************************************************************/


/* Reset */
void ttl74145_reset(int which)
{
	ttl74145[which].number = 0;
}


/* Data Write */
void ttl74145_write(int which, offs_t offset, UINT8 data)
{
	ttl74145[which].number = bcd_2_dec(data & 0x0f);
}


int ttl74145_output_0(int which) { return ttl74145[which].number == 0; }
int ttl74145_output_1(int which) { return ttl74145[which].number == 1; }
int ttl74145_output_2(int which) { return ttl74145[which].number == 2; }
int ttl74145_output_3(int which) { return ttl74145[which].number == 3; }
int ttl74145_output_4(int which) { return ttl74145[which].number == 4; }
int ttl74145_output_5(int which) { return ttl74145[which].number == 5; }
int ttl74145_output_6(int which) { return ttl74145[which].number == 6; }
int ttl74145_output_7(int which) { return ttl74145[which].number == 7; }
int ttl74145_output_8(int which) { return ttl74145[which].number == 8; }
int ttl74145_output_9(int which) { return ttl74145[which].number == 9; }


WRITE8_HANDLER( ttl74145_0_w ) { ttl74145_write(0, offset, data); }
