/***************************************************************************
    mos tri port interface 6525
	mos triple interface adapter 6523

    peter.trauner@jk.uni-linz.ac.at

	used in commodore b series
	used in commodore c1551 floppy disk drive
***************************************************************************/
#ifndef __TPI6525_H_
#define __TPI6525_H_

/* tia6523 is a tpi6525 without control register!? */

/*
 * tia6523
 *
 * only some lines of port b and c are in the pinout !
 *
 * connector to floppy c1551 (delivered with c1551 as c16 expansion)
 * port a for data read/write
 * port b
 * 0 status 0
 * 1 status 1
 * port c
 * 6 dav output edge data on port a available
 * 7 ack input edge ready for next datum
 */

/* fill in the callback functions */
typedef struct {
	int number;
	struct {
		int (*read)(void);
		void (*output)(int data);
		int port, ddr, in;
	} a,b,c;

	struct {
		void (*output)(int level);
		int level;
	} ca, cb, interrupt;

	int cr;
	int air;

	int irq_level[5];
} TPI6525;

extern TPI6525 tpi6525[4];

void tpi6525_0_reset(void);
void tpi6525_1_reset(void);
void tpi6525_2_reset(void);
void tpi6525_3_reset(void);

void tpi6525_0_irq0_level(int level);
void tpi6525_0_irq1_level(int level);
void tpi6525_0_irq2_level(int level);
void tpi6525_0_irq3_level(int level);
void tpi6525_0_irq4_level(int level);

void tpi6525_1_irq0_level(int level);
void tpi6525_1_irq1_level(int level);
void tpi6525_1_irq2_level(int level);
void tpi6525_1_irq3_level(int level);
void tpi6525_1_irq4_level(int level);

 READ8_HANDLER  ( tpi6525_0_port_r );
 READ8_HANDLER  ( tpi6525_1_port_r );
 READ8_HANDLER  ( tpi6525_2_port_r );
 READ8_HANDLER  ( tpi6525_3_port_r );

WRITE8_HANDLER ( tpi6525_0_port_w );
WRITE8_HANDLER ( tpi6525_1_port_w );
WRITE8_HANDLER ( tpi6525_2_port_w );
WRITE8_HANDLER ( tpi6525_3_port_w );

 READ8_HANDLER  ( tpi6525_0_port_a_r );
 READ8_HANDLER  ( tpi6525_1_port_a_r );
 READ8_HANDLER  ( tpi6525_2_port_a_r );
 READ8_HANDLER  ( tpi6525_3_port_a_r );

WRITE8_HANDLER ( tpi6525_0_port_a_w );
WRITE8_HANDLER ( tpi6525_1_port_a_w );
WRITE8_HANDLER ( tpi6525_2_port_a_w );
WRITE8_HANDLER ( tpi6525_3_port_a_w );

 READ8_HANDLER  ( tpi6525_0_port_b_r );
 READ8_HANDLER  ( tpi6525_1_port_b_r );
 READ8_HANDLER  ( tpi6525_2_port_b_r );
 READ8_HANDLER  ( tpi6525_3_port_b_r );

WRITE8_HANDLER ( tpi6525_0_port_b_w );
WRITE8_HANDLER ( tpi6525_1_port_b_w );
WRITE8_HANDLER ( tpi6525_2_port_b_w );
WRITE8_HANDLER ( tpi6525_3_port_b_w );

 READ8_HANDLER  ( tpi6525_0_port_c_r );
 READ8_HANDLER  ( tpi6525_1_port_c_r );
 READ8_HANDLER  ( tpi6525_2_port_c_r );
 READ8_HANDLER  ( tpi6525_3_port_c_r );

WRITE8_HANDLER ( tpi6525_0_port_c_w );
WRITE8_HANDLER ( tpi6525_1_port_c_w );
WRITE8_HANDLER ( tpi6525_2_port_c_w );
WRITE8_HANDLER ( tpi6525_3_port_c_w );

#endif
