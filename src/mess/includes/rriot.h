#ifndef RIOT_H
#define RIOT_H

/* mos rom ram io timer 6530 */

/* ram and rom areas are not handled in this rriot compoment */

/* 
internal 64 byte ram
internal 1024 byte rom
inputs and output
timer
interrupt logic
logic to connect up to 7 6530 without additional address decoding logic

1kb rom selection 
a0..a9, 
rs0, cs1, cs2 mask programmed!

64byte ram selection
a0..a5
rs0, cs1, cs2, a6, a7, a8, a9 mask programmed!

io, timer selection
a0..a3 (a4,a5 don't care)
rs0, cs1, cs2, a6, a7, a8, a9 mask programmed!
(a2=0 io, a3 don't care)
 a2=1 timer, a3 interrupt enable)

standard configurations! CS2(a12) CS1(a11) rs0(a10)
chip subtype 1: rom 0x0400, ram 0x000, io 0x200
chip subtype 2: rom 0x0800, ram 0x040, io 0x240
chip subtype 3: rom 0x0c00, ram 0x080, io 0x280
chip subtype 4: rom 0x1000, ram 0x0c0, io 0x2c0
chip subtype 5: rom 0x1400, ram 0x100, io 0x300
chip subtype 6: rom 0x1800, ram 0x140, io 0x340
chip subtype 7: rom 0x1c00, ram 0x180, io 0x380

5V    1 40 PA1
PA0   2 39 PA2
CLK   3 38 PA3
RS0   4 37 PA4
A9    5 36 PA5
A8    6 35 PA6
A7    7 34 PA7
A6    8 33 D0
R /W  9 32 D1
A5   10 31 D2
A4   11 30 D3
A3   12 29 D4
A2   13 28 D5
A1   14 27 D6
A0   15 26 D7
/RES 16 25 PB0
PB7  17 24 PB1
PB6  18 23 PB2
PB5  19 22 PB3
GND  20 21 PB4

D0..7 data bus
A0..9 address bus
R /W  read or /write
RS0 rom select


/RES reset
CLK (phi2) clock for timer
PA0..7, PB0..8 input or output (software selectable)

PB7 could be configured as /IRQ line
PB6,PB5 could be configured as CS1,CS2 (mask programmed with the rom!)

dumping suggestions
wire an adapter for an 2764 eprom, so it is readable with many eprom burners
read the complete 8kbyte data
extract the positions from the dump

*/


/* 
mos 6532
chip also contains 128 bytes ram (own chip select line)
no rom
separate irq output
some significant differences, so unique component
*/
 
#ifdef __cplusplus
extern "C" {
#endif

#define MAX_RRIOTS   4

typedef struct {
	int baseclock;
	struct {
		int (*input)(int chip);
		void (*output)(int chip, int value);
	} port_a, port_b;
	void (*irq_callback)(int chip, int level);
} RRIOT_CONFIG;

/* This has to be called from a driver at startup */
void rriot_init(int nr, RRIOT_CONFIG *riot);
	
int rriot_r(int chip, int offs);
void rriot_w(int chip, int offs, int data);

void rriot_reset(int nr);

 READ8_HANDLER  ( rriot_0_r );
WRITE8_HANDLER ( rriot_0_w );
 READ8_HANDLER  ( rriot_1_r );
WRITE8_HANDLER ( rriot_1_w );
 READ8_HANDLER  ( rriot_2_r );
WRITE8_HANDLER ( rriot_2_w );
 READ8_HANDLER  ( rriot_3_r );
WRITE8_HANDLER ( rriot_3_w );

 READ8_HANDLER( rriot_0_a_r );
 READ8_HANDLER( rriot_1_a_r );
 READ8_HANDLER( rriot_2_a_r );
 READ8_HANDLER( rriot_3_a_r );

WRITE8_HANDLER( rriot_0_a_w );
WRITE8_HANDLER( rriot_1_a_w );
WRITE8_HANDLER( rriot_2_a_w );
WRITE8_HANDLER( rriot_3_a_w );

 READ8_HANDLER( rriot_0_b_r );
 READ8_HANDLER( rriot_1_b_r );
 READ8_HANDLER( rriot_2_b_r );
 READ8_HANDLER( rriot_3_b_r );

WRITE8_HANDLER( rriot_0_b_w );
WRITE8_HANDLER( rriot_1_b_w );
WRITE8_HANDLER( rriot_2_b_w );
WRITE8_HANDLER( rriot_3_b_w );

#ifdef __cplusplus
}
#endif

#endif
