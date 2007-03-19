#ifndef RIOT_H
#define RIOT_H

/* ram area is not handled in this riot compoment */

/* 
mos ram io timer 6532
chip also contains 128 bytes ram (own chip select line)

significant differences to rriot6530
no rom
separate irq output
a7 acts as interrupt input
(level configurable through access addresses)
state of this interrupt input can be read as bit 6 in interrupt status
*/
 
#ifdef __cplusplus
extern "C" {
#endif

#define MAX_RIOTS   4

typedef struct {
	int baseclock;
	struct {
		int (*input)(int chip);
		void (*output)(int chip, int value);
	} port_a, port_b;
	void (*irq_callback)(int chip, int level);
} RIOT_CONFIG;

/* This has to be called from a driver at startup */
void riot_init(int nr, RIOT_CONFIG *riot);
	
int riot_r(int chip, int offs);
void riot_w(int chip, int offs, int data);

void riot_reset(int nr);

 READ8_HANDLER  ( riot_0_r );
WRITE8_HANDLER ( riot_0_w );
 READ8_HANDLER  ( riot_1_r );
WRITE8_HANDLER ( riot_1_w );
 READ8_HANDLER  ( riot_2_r );
WRITE8_HANDLER ( riot_2_w );
 READ8_HANDLER  ( riot_3_r );
WRITE8_HANDLER ( riot_3_w );

 READ8_HANDLER( riot_0_a_r );
 READ8_HANDLER( riot_1_a_r );
 READ8_HANDLER( riot_2_a_r );
 READ8_HANDLER( riot_3_a_r );

WRITE8_HANDLER( riot_0_a_w );
WRITE8_HANDLER( riot_1_a_w );
WRITE8_HANDLER( riot_2_a_w );
WRITE8_HANDLER( riot_3_a_w );

 READ8_HANDLER( riot_0_b_r );
 READ8_HANDLER( riot_1_b_r );
 READ8_HANDLER( riot_2_b_r );
 READ8_HANDLER( riot_3_b_r );

WRITE8_HANDLER( riot_0_b_w );
WRITE8_HANDLER( riot_1_b_w );
WRITE8_HANDLER( riot_2_b_w );
WRITE8_HANDLER( riot_3_b_w );

#ifdef __cplusplus
}
#endif

#endif
