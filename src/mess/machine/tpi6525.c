/***************************************************************************
    mos tri port interface 6525
	mos triple interface adapter 6523

    peter.trauner@jk.uni-linz.ac.at

	used in commodore b series
	used in commodore c1551 floppy disk drive
***************************************************************************/

/*
 mos tpi 6525
 40 pin package
 3 8 bit ports (pa, pb, pc)
 8 registers to pc
 0 port a 0 in low
 1 port a data direction register 1 output
 2 port b
 3 port b ddr
 4 port c
  handshaking, interrupt mode
  0 interrupt 0 input, 1 interrupt enabled
   interrupt set on falling edge
  1 interrupt 1 input
  2 interrupt 2 input
  3 interrupt 3 input
  4 interrupt 4 input
  5 irq output
  6 ca handshake line (read handshake answer on I3 prefered)
  7 cb handshake line (write handshake clear on I4 prefered)
 5 port c ddr

 6 cr configuration register
  0 mc
    0 port c normal input output mode like port a and b
    1 port c used for handshaking and interrupt input
  1 1 interrupt priorized
  2 i3 configure edge
    1 interrupt set on positive edge
  3 i4 configure edge
  5,4 ca handshake
   00 on read
      rising edge of i3 sets ca high
      read a data from computers sets ca low
   01 pulse output
      1 microsecond low after read a operation
   10 manual output low
   11 manual output high
  7,6 cb handshake
   00 handshake on write
      write b data from computer sets cb 0
      rising edge of i4 sets cb high
   01 pulse output
      1 microsecond low after write b operation
   10 manual output low
   11 manual output high
 7 air active interrupt register
   0 I0 occured
   1 I1 occured
   2 I2 occured
   3 I3 occured
   4 I4 occured
   read clears interrupt

 non priorized interrupts
  interrupt is set when occured
  read clears all interrupts

 priorized interrupts
  I4>I3>I2>I1>I0
  highest interrupt can be found in air register
  read clears highest interrupt
*/
#include <ctype.h>
#include "driver.h"

#define VERBOSE_DBG 1
#include "includes/cbm.h"

#include "includes/tpi6525.h"

TPI6525 tpi6525[4]={
	{ 0 },
	{ 1 },
	{ 2 },
	{ 3 }
};

#define INTERRUPT_MODE (This->cr&1)
#define PRIORIZED_INTERRUPTS (This->cr&2)
#define INTERRUPT3_RISING_EDGE (This->cr&4)
#define INTERRUPT4_RISING_EDGE (This->cr&8)
#define CA_MANUAL_OUT (This->cr&0x20)
#define CA_MANUAL_LEVEL ((This->cr&0x10)?1:0)
#define CB_MANUAL_OUT (This->cr&0x80)
#define CB_MANUAL_LEVEL ((This->cr&0x40)?1:0)

static void tpi6525_reset(TPI6525 *This)
{
	This->cr=0;This->air=0;
	This->a.ddr=0;This->a.port=0;
	This->b.ddr=0;This->b.port=0;
	This->c.ddr=0;This->c.port=0;
	This->a.in=0xff;
	This->b.in=0xff;
	This->c.in=0xff;
	This->interrupt.level=
		This->irq_level[0]=
		This->irq_level[1]=
		This->irq_level[2]=
		This->irq_level[3]=
		This->irq_level[4]=0;
}

static void tpi6525_set_interrupt(TPI6525 *This)
{
	if (!This->interrupt.level && (This->air!=0)) {
		This->interrupt.level=1;
		DBG_LOG (3, "tpi6525",("%d set interrupt\n",This->number));
		if (This->interrupt.output!=NULL)
			This->interrupt.output(This->interrupt.level);
	}
}

static void tpi6525_clear_interrupt(TPI6525 *This)
{
	if (This->interrupt.level && (This->air==0)) {
		This->interrupt.level=0;
		DBG_LOG (3, "tpi6525",("%d clear interrupt\n",This->number));
		if (This->interrupt.output!=NULL)
			This->interrupt.output(This->interrupt.level);
	}
}

static void tpi6525_irq0_level(TPI6525 *This, int level)
{
	if (INTERRUPT_MODE && (level!=This->irq_level[0]) ) {
		This->irq_level[0]=level;
		if ((level==0)&&!(This->air&1)&&(This->c.ddr&1)) {
			This->air|=1;
			tpi6525_set_interrupt(This);
		}
	}
}

static void tpi6525_irq1_level(TPI6525 *This, int level)
{
	if (INTERRUPT_MODE && (level!=This->irq_level[1]) ) {
		This->irq_level[1]=level;
		if ((level==0)&&!(This->air&2)&&(This->c.ddr&2)) {
			This->air|=2;
			tpi6525_set_interrupt(This);
		}
	}
}

static void tpi6525_irq2_level(TPI6525 *This, int level)
{
	if (INTERRUPT_MODE && (level!=This->irq_level[2]) ) {
		This->irq_level[2]=level;
		if ((level==0)&&!(This->air&4)&&(This->c.ddr&4)) {
			This->air|=4;
			tpi6525_set_interrupt(This);
		}
	}
}

static void tpi6525_irq3_level(TPI6525 *This, int level)
{
	if (INTERRUPT_MODE && (level!=This->irq_level[3]) ) {
		This->irq_level[3]=level;
		if ( ((INTERRUPT3_RISING_EDGE&&(level==1))
			  ||(!INTERRUPT3_RISING_EDGE&&(level==0)))
			 &&!(This->air&8)&&(This->c.ddr&8)) {
			This->air|=8;
			tpi6525_set_interrupt(This);
		}
	}
}

static void tpi6525_irq4_level(TPI6525 *This, int level)
{
	if (INTERRUPT_MODE &&(level!=This->irq_level[4]) ) {
		This->irq_level[4]=level;
		if ( ((INTERRUPT4_RISING_EDGE&&(level==1))
			  ||(!INTERRUPT4_RISING_EDGE&&(level==0)))
			  &&!(This->air&0x10)&&(This->c.ddr&0x10)) {
			This->air|=0x10;
			tpi6525_set_interrupt(This);
		}
	}
}

static int tpi6525_port_a_r(TPI6525 *This, int offset)
{
	int data=This->a.in;

	if (This->a.read) data=This->a.read();
	data=(data&~This->a.ddr)|(This->a.ddr&This->a.port);

	return data;
}

static void tpi6525_port_a_w(TPI6525 *This, int offset, int data)
{
	This->a.in=data;
}

static int tpi6525_port_b_r(TPI6525 *This, int offset)
{
	int data=This->b.in;

	if (This->b.read) data=This->b.read();
	data=(data&~This->b.ddr)|(This->b.ddr&This->b.port);

	return data;
}

static void tpi6525_port_b_w(TPI6525 *This, int offset, int data)
{
	This->b.in=data;
}

static int tpi6525_port_c_r(TPI6525 *This, int offset)
{
	int data=This->c.in;

	if (This->c.read) data&=This->c.read();
	data=(data&~This->c.ddr)|(This->c.ddr&This->c.port);

	return data;
}

static void tpi6525_port_c_w(TPI6525 *This, int offset, int data)
{
	This->c.in=data;
}

static int tpi6525_port_r(TPI6525 *This, int offset)
{
	int data=0xff;
	switch (offset&7) {
	case 0:
		data=This->a.in;
		if (This->a.read) data&=This->a.read();
		data=(data&~This->a.ddr)|(This->a.ddr&This->a.port);
		break;
	case 1:
		data=This->b.in;
		if (This->b.read) data&=This->b.read();
		data=(data&~This->b.ddr)|(This->b.ddr&This->b.port);
		break;
	case 2:
		if (INTERRUPT_MODE) {
			data=0;
			if (This->irq_level[0]) data|=1;
			if (This->irq_level[1]) data|=2;
			if (This->irq_level[2]) data|=4;
			if (This->irq_level[3]) data|=8;
			if (This->irq_level[4]) data|=0x10;
			if (!This->interrupt.level) data|=0x20;
			if (This->ca.level) data|=0x40;
			if (This->cb.level) data|=0x80;
		} else {
			data=This->c.in;
			if (This->c.read) data&=This->c.read();
			data=(data&~This->c.ddr)|(This->c.ddr&This->c.port);
		}
		DBG_LOG (2, "tpi6525",
				 ("%d read %.2x %.2x\n",This->number, offset,data));
		break;
	case 3:
		data=This->a.ddr;
		break;
	case 4:
		data=This->b.ddr;
		break;
	case 5:
		data=This->c.ddr;
		break;
	case 6: /* cr */
		data=This->cr;
		break;
	case 7: /* air */
		if (PRIORIZED_INTERRUPTS) {
			if (This->air&0x10) {
				data=0x10;
				This->air&=~0x10;
			} else if (This->air&8) {
				data=8;
				This->air&=~8;
			} else if (This->air&4) {
				data=4;
				This->air&=~4;
			} else if (This->air&2) {
				data=2;
				This->air&=~2;
			} else if (This->air&1) {
				data=1;
				This->air&=~1;
			}
		} else {
			data=This->air;
			This->air=0;
		}
		tpi6525_clear_interrupt(This);
		break;
	}
	DBG_LOG (3, "tpi6525",
			 ("%d read %.2x %.2x\n",This->number, offset,data));
	return data;
}

static void tpi6525_port_w(TPI6525 *This, int offset, int data)
{
	DBG_LOG (2, "tpi6525",
			 ("%d write %.2x %.2x\n",This->number, offset,data));

	switch (offset&7) {
	case 0:
		This->a.port=data;
		if (This->a.output) {
			This->a.output(This->a.port&This->a.ddr);
		}
		break;
	case 1:
		This->b.port=data;
		if (This->b.output) {
			This->b.output(This->b.port&This->b.ddr);
		}
		break;
	case 2:
		This->c.port=data;
		if (!INTERRUPT_MODE&&This->c.output) {
			This->c.output(This->c.port&This->c.ddr);
		}
		break;
	case 3:
		This->a.ddr=data;
		if (This->a.output) {
			This->a.output(This->a.port&This->a.ddr);
		}
		break;
	case 4:
		This->b.ddr=data;
		if (This->b.output) {
			This->b.output(This->b.port&This->b.ddr);
		}
		break;
	case 5:
		This->c.ddr=data;
		if (!INTERRUPT_MODE&&This->c.output) {
			This->c.output(This->c.port&This->c.ddr);
		}
		break;
	case 6:
		This->cr=data;
		if (INTERRUPT_MODE) {
			if (CA_MANUAL_OUT) {
				if (This->ca.level!=CA_MANUAL_LEVEL) {
					This->ca.level=CA_MANUAL_LEVEL;
					if (This->ca.output) This->ca.output(This->ca.level);
				}
			}
			if (CB_MANUAL_OUT) {
				if (This->cb.level!=CB_MANUAL_LEVEL) {
					This->cb.level=CB_MANUAL_LEVEL;
					if (This->cb.output) This->cb.output(This->cb.level);
				}
			}
		}
		break;
	case 7:
		/*This->air=data; */
		break;
	}
}


void tpi6525_0_reset(void)
{
	tpi6525_reset(tpi6525);
}

void tpi6525_1_reset(void)
{
	tpi6525_reset(tpi6525+1);
}

void tpi6525_2_reset(void)
{
	tpi6525_reset(tpi6525+2);
}

void tpi6525_3_reset(void)
{
	tpi6525_reset(tpi6525+3);
}

void tpi6525_0_irq0_level(int level)
{
	tpi6525_irq0_level(tpi6525, level);
}

void tpi6525_0_irq1_level(int level)
{
	tpi6525_irq1_level(tpi6525, level);
}

void tpi6525_0_irq2_level(int level)
{
	tpi6525_irq2_level(tpi6525, level);
}

void tpi6525_0_irq3_level(int level)
{
	tpi6525_irq3_level(tpi6525, level);
}

void tpi6525_0_irq4_level(int level)
{
	tpi6525_irq4_level(tpi6525, level);
}

void tpi6525_1_irq0_level(int level)
{
	tpi6525_irq0_level(tpi6525+1, level);
}

void tpi6525_1_irq1_level(int level)
{
	tpi6525_irq1_level(tpi6525+1, level);
}

void tpi6525_1_irq2_level(int level)
{
	tpi6525_irq2_level(tpi6525+1, level);
}

void tpi6525_1_irq3_level(int level)
{
	tpi6525_irq3_level(tpi6525+1, level);
}

void tpi6525_1_irq4_level(int level)
{
	tpi6525_irq4_level(tpi6525+1, level);
}

 READ8_HANDLER ( tpi6525_0_port_r )
{
	return tpi6525_port_r(tpi6525, offset);
}

 READ8_HANDLER ( tpi6525_1_port_r )
{
	return tpi6525_port_r(tpi6525+1, offset);
}

 READ8_HANDLER ( tpi6525_2_port_r )
{
	return tpi6525_port_r(tpi6525+2, offset);
}

 READ8_HANDLER ( tpi6525_3_port_r )
{
	return tpi6525_port_r(tpi6525+3, offset);
}

WRITE8_HANDLER ( tpi6525_0_port_w )
{
	tpi6525_port_w(tpi6525, offset, data);
}

WRITE8_HANDLER ( tpi6525_1_port_w )
{
	tpi6525_port_w(tpi6525+1, offset, data);
}

WRITE8_HANDLER ( tpi6525_2_port_w )
{
	tpi6525_port_w(tpi6525+2, offset, data);
}

WRITE8_HANDLER ( tpi6525_3_port_w )
{
	tpi6525_port_w(tpi6525+3, offset, data);
}

 READ8_HANDLER ( tpi6525_0_port_a_r )
{
	return tpi6525_port_a_r(tpi6525, offset);
}

 READ8_HANDLER ( tpi6525_1_port_a_r )
{
	return tpi6525_port_a_r(tpi6525+1, offset);
}

 READ8_HANDLER ( tpi6525_2_port_a_r )
{
	return tpi6525_port_a_r(tpi6525+2, offset);
}

 READ8_HANDLER ( tpi6525_3_port_a_r )
{
	return tpi6525_port_a_r(tpi6525+3, offset);
}

WRITE8_HANDLER ( tpi6525_0_port_a_w )
{
	tpi6525_port_a_w(tpi6525, offset, data);
}

WRITE8_HANDLER ( tpi6525_1_port_a_w )
{
	tpi6525_port_a_w(tpi6525+1, offset, data);
}

WRITE8_HANDLER ( tpi6525_2_port_a_w )
{
	tpi6525_port_a_w(tpi6525+2, offset, data);
}

WRITE8_HANDLER ( tpi6525_3_port_a_w )
{
	tpi6525_port_a_w(tpi6525+3, offset, data);
}

 READ8_HANDLER ( tpi6525_0_port_b_r )
{
	return tpi6525_port_b_r(tpi6525, offset);
}

 READ8_HANDLER ( tpi6525_1_port_b_r )
{
	return tpi6525_port_b_r(tpi6525+1, offset);
}

 READ8_HANDLER ( tpi6525_2_port_b_r )
{
	return tpi6525_port_b_r(tpi6525+2, offset);
}

 READ8_HANDLER ( tpi6525_3_port_b_r )
{
	return tpi6525_port_b_r(tpi6525+3, offset);
}

WRITE8_HANDLER ( tpi6525_0_port_b_w )
{
	tpi6525_port_b_w(tpi6525, offset, data);
}

WRITE8_HANDLER ( tpi6525_1_port_b_w )
{
	tpi6525_port_b_w(tpi6525+1, offset, data);
}

WRITE8_HANDLER ( tpi6525_2_port_b_w )
{
	tpi6525_port_b_w(tpi6525+2, offset, data);
}

WRITE8_HANDLER ( tpi6525_3_port_b_w )
{
	tpi6525_port_b_w(tpi6525+3, offset, data);
}

 READ8_HANDLER ( tpi6525_0_port_c_r )
{
	return tpi6525_port_c_r(tpi6525, offset);
}

 READ8_HANDLER ( tpi6525_1_port_c_r )
{
	return tpi6525_port_c_r(tpi6525+1, offset);
}

 READ8_HANDLER ( tpi6525_2_port_c_r )
{
	return tpi6525_port_c_r(tpi6525+2, offset);
}

 READ8_HANDLER ( tpi6525_3_port_c_r )
{
	return tpi6525_port_c_r(tpi6525+3, offset);
}

WRITE8_HANDLER ( tpi6525_0_port_c_w )
{
	tpi6525_port_c_w(tpi6525, offset, data);
}

WRITE8_HANDLER ( tpi6525_1_port_c_w )
{
	tpi6525_port_c_w(tpi6525+1, offset, data);
}

WRITE8_HANDLER ( tpi6525_2_port_c_w )
{
	tpi6525_port_c_w(tpi6525+2, offset, data);
}

WRITE8_HANDLER ( tpi6525_3_port_c_w )
{
	tpi6525_port_c_w(tpi6525+3, offset, data);
}

