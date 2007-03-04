/**********************************************************************
 * RIOT emulation
 *
 * MOS ROM RAM IO Timer 6530
 *
 * TODO:
 * - test if it works ;)
 * - change kim1 driver to use it
 *
 * used in chess champion mk2
 * rockwell aim65 ?
 **********************************************************************/
#include "driver.h"
#include "includes/rriot.h"

#if 0
#define LOG(a) logerror(a)
#else
#define LOG(a) ((void) 0)
#endif

/*
  riot 6530

  a0,a1 divide select (1,8,64,1024)
  a3 set means irq on

 io, timer selected
 xx000 Port A data
 xx001 Port A direction
 xx010 Port B data
 xx010 Port B direction

 xx1xx timer selected, irq disable
 x11xx t

    6530
    offset  R/W short   purpose
    0       X   DRA     Data register A
    1       X   DDRA    Data direction register A
    2       X   DRB     Data register B
    3       X   DDRB    Data direction register B
    4       W   CNT1T   Count down from value, divide by 1, disable IRQ
    5       W   CNT8T   Count down from value, divide by 8, disable IRQ
    6       W   CNT64T  Count down from value, divide by 64, disable IRQ
            R   LATCH   Read current counter value, disable IRQ
    7       W   CNT1KT  Count down from value, divide by 1024, disable IRQ
            R   STATUS  Read counter statzs, bit 7 = 1 means counter overflow
    8       X   DRA     Data register A
    9       X   DDRA    Data direction register A
    A       X   DRB     Data register B
    B       X   DDRB    Data direction register B
    C       W   CNT1I   Count down from value, divide by 1, enable IRQ
    D       W   CNT8I   Count down from value, divide by 8, enable IRQ
    E       W   CNT64I  Count down from value, divide by 64, enable IRQ
            R   LATCH   Read current counter value, enable IRQ
    F       W   CNT1KI  Count down from value, divide by 1024, enable IRQ
            R   STATUS  Read counter statzs, bit 7 = 1 means counter overflow
*/

typedef struct {
	RRIOT_CONFIG *config;
	struct {
		UINT8 in, out, ddr;
	} port_a, port_b;

	UINT8	irqen;		/* IRQ enabled ? */
	enum { Delay1, Delay8, Delay64, Delay1024, TimerShot } state;
	double shottime;
	int	irq;
	void	*timer; 	/* timer callback */
} RRIOT;

static RRIOT rriot[MAX_RRIOTS]= { {0} };

static void rriot_timer_cb(int chip);

void rriot_init(int nr, RRIOT_CONFIG *config)
{
	memset(&rriot[nr], 0, sizeof(rriot[nr]));
	rriot[nr].config = config;
	rriot[nr].timer = timer_alloc(rriot_timer_cb);

    LOG("RRIOT - successfully initialised\n");
}

void rriot_reset(int nr)
{
	rriot[nr].port_a.ddr=0;
	if (rriot[nr].config->port_a.output)
		rriot[nr].config->port_a.output(nr,0xff);
	rriot[nr].port_b.ddr=0;
	if (rriot[nr].config->port_b.output)
		rriot[nr].config->port_b.output(nr,0xff);
	
	rriot[nr].irq=0;rriot[nr].irqen=0;
	if (rriot[nr].config->irq_callback) 
		rriot[nr].config->irq_callback(nr, 0); 
}

int rriot_r(int chip, int offset)
{
	int data = 0xff;

	offset&=0xf;
	if (!(offset&4)) { // io part of chip
		switch( offset&3 )
		{
		case 0: /* Data register A */
			if( rriot[chip].config->port_a.input )
				data = (*rriot[chip].config->port_a.input)(chip);
			/* mask input bits and combine with output bits */
			data = (data & ~rriot[chip].port_a.ddr) | (rriot[chip].port_a.out & rriot[chip].port_a.ddr);
			LOG(("rriot(%d) DRA   read : $%02x\n", chip, data));
			break;
		case 1: /* Data direction register A */
			data = rriot[chip].port_a.ddr;
			LOG(("rriot(%d) DDRA  read : $%02x\n", chip, data));
			break;
		case 2: /* Data register B */
			if( rriot[chip].config->port_b.input )
				data = (*rriot[chip].config->port_b.input)(chip);
			/* mask input bits and combine with output bits */
			data = (data & ~rriot[chip].port_b.ddr) | (rriot[chip].port_b.out & rriot[chip].port_b.ddr);
//			LOG(("rriot(%d) DRB   read : $%02x\n", chip, data));
			break;
		case 3: /* Data direction register B */
			data = rriot[chip].port_b.ddr;
			LOG(("rriot(%d) DDRB  read : $%02x\n", chip, data));
			break;
		}
	} else { // timer part of chip
		switch (offset&1) {
		case 0: /* Timer count read */
			switch (rriot[chip].state) {
			case Delay1:
				data = (int)(timer_timeleft(rriot[chip].timer)*rriot[chip].config->baseclock);
				break;
			case Delay8:
				data = (int)(timer_timeleft(rriot[chip].timer)*rriot[chip].config->baseclock)>>3;
				break;
			case Delay64:
				data = (int)(timer_timeleft(rriot[chip].timer)*rriot[chip].config->baseclock)>>6;
				break;
			case Delay1024:
				data = (int)(timer_timeleft(rriot[chip].timer)*rriot[chip].config->baseclock)>>10;
				break;
			case TimerShot:
				data=255-(int)((timer_get_time()-rriot[chip].shottime)*rriot[chip].config->baseclock);
				if (data <0 ) data=0;
				break;
			}
			if( rriot[chip].irqen&&rriot[chip].irq ) /* with IRQ? */
			{
				if( rriot[chip].config->irq_callback )
					(*rriot[chip].config->irq_callback)(chip, 0);
			}
			rriot[chip].irq=0;
			break;
		case 1: /* Timer status read */
			data = rriot[chip].irq?0x80:0;
			LOG(("riot(%d) STAT  read : $%02x%s\n", chip, data, (char*)((offset & 8) ? " (IRQ)":"    ")));
			break;
		}
		// problem when restarting with read the timer!?
		rriot[chip].irqen = (offset & 8) ? 1 : 0;
    }
	return data;
}

static void rriot_timer_cb(int chip)
{
	LOG(("rriot(%d) timer expired\n", chip));
	rriot[chip].irq=1;
	rriot[chip].state=TimerShot;
	rriot[chip].shottime=timer_get_time();
	if( rriot[chip].irqen ) /* with IRQ? */
	{
		if( rriot[chip].config->irq_callback )
			(*rriot[chip].config->irq_callback)(chip, 1);
	}
}

void rriot_w(int chip, int offset, int data)
{
	offset&=0xf;
	if (!(offset&4)) {
		switch( offset&3 )
		{
		case 0: /* Data register A */
			LOG(("rriot(%d) DRA  write: $%02x\n", chip, data));
			rriot[chip].port_a.out = data;
			if( rriot[chip].config->port_a.output )
				(*rriot[chip].config->port_a.output)(chip,data);
			break;
		case 1: /* Data direction register A */
			LOG(("rriot(%d) DDRA  write: $%02x\n", chip, data));
			rriot[chip].port_a.ddr = data;
			break;
		case 2: /* Data register B */
//			LOG(("rriot(%d) DRB   write: $%02x\n", chip, data));
			rriot[chip].port_b.out = data;
			if( rriot[chip].config->port_b.output )
				(*rriot[chip].config->port_b.output)(chip,data);
			break;
		case 3: /* Data direction register B */
			LOG(("rriot(%d) DDRB  write: $%02x\n", chip, data));
			rriot[chip].port_b.ddr = data;
			break;
		}
	} else {
		if( rriot[chip].irqen&&rriot[chip].irq ) /* with IRQ? */
		{
			if( rriot[chip].config->irq_callback )
				(*rriot[chip].config->irq_callback)(chip, 0);
		}
		rriot[chip].irqen = (offset & 8) ? 1 : 0;
		rriot[chip].irq =0;
		
		switch (offset&3) {
		case 0: /* Timer 1 start */
			LOG(("rriot(%d) TMR1  write: $%02x%s\n", chip, data, (char*)((offset & 8) ? " (IRQ)":" ")));
			timer_adjust(rriot[chip].timer, TIME_IN_HZ( (data+1) / rriot[chip].config->baseclock), 
										  chip, 0);
			rriot[chip].state=Delay1;
			break;
		case 1: /* Timer 8 start */
			LOG(("rriot(%d) TMR8  write: $%02x%s\n", chip, data, (char*)((offset & 8) ? " (IRQ)":" ")));
			timer_adjust(rriot[chip].timer, TIME_IN_HZ( (data+1) * 8 / rriot[chip].config->baseclock), 
										  chip, 0);
			rriot[chip].state=Delay8;
			break;
		case 2: /* Timer 64 start */
			LOG(("rriot(%d) TMR64 write: $%02x%s\n", chip, data, (char*)((offset & 8) ? " (IRQ)":" ")));
//			LOG(("rriot(%d) TMR64 write: time is $%f\n", chip, (double)(64.0 * (data + 1) / rriot[chip].clock)));
			timer_adjust(rriot[chip].timer, TIME_IN_HZ( (data+1) * 64 / rriot[chip].config->baseclock), 
										  chip, 0);
			rriot[chip].state=Delay64;
			break;
		case 3: /* Timer 1024 start */
			LOG(("rriot(%d) TMR1K write: $%02x%s\n", chip, data, (char*)((offset & 8) ? " (IRQ)":" ")));
			timer_adjust(rriot[chip].timer, TIME_IN_HZ( (data+1) * 1024 / rriot[chip].config->baseclock), 
										  chip, 0);
			rriot[chip].state=Delay1024;
			break;
		}
    }
}

static int rriot_a_r(int chip)
{
	int data;
	if (rriot[chip].config->port_a.input) 
		data = rriot[chip].config->port_a.input(chip);
	else
		data = 0xff;
	data &= ~rriot[chip].port_a.ddr;
	data |= (rriot[chip].port_a.ddr&rriot[chip].port_a.out);
	return data;
}

static int rriot_b_r(int chip)
{
	int data;
	if (rriot[chip].config->port_b.input) 
		data = rriot[chip].config->port_b.input(chip);
	else
		data = 0xff;
	data &= ~rriot[chip].port_b.ddr;
	data |= (rriot[chip].port_b.ddr&rriot[chip].port_b.out);
	return data;
}

static void rriot_a_w(int chip, int data)
{
#if 0
	rriot[chip].port_a.in = data;
#endif
}

static void rriot_b_w(int chip, int data)
{
	
}

 READ8_HANDLER ( rriot_0_r ) { return rriot_r(0,offset); }
WRITE8_HANDLER ( rriot_0_w ) { rriot_w(0,offset,data); }
 READ8_HANDLER ( rriot_1_r ) { return rriot_r(1,offset); }
WRITE8_HANDLER ( rriot_1_w ) { rriot_w(1,offset,data); }
 READ8_HANDLER ( rriot_2_r ) { return rriot_r(2,offset); }
WRITE8_HANDLER ( rriot_2_w ) { rriot_w(2,offset,data); }
 READ8_HANDLER ( rriot_3_r ) { return rriot_r(3,offset); }
WRITE8_HANDLER ( rriot_3_w ) { rriot_w(3,offset,data); }

 READ8_HANDLER( rriot_0_a_r ) { return rriot_a_r(0); }
 READ8_HANDLER( rriot_1_a_r ) { return rriot_a_r(1); }
 READ8_HANDLER( rriot_2_a_r ) { return rriot_a_r(2); }
 READ8_HANDLER( rriot_3_a_r ) { return rriot_a_r(3); }

 READ8_HANDLER( rriot_0_b_r ) { return rriot_b_r(0); }
 READ8_HANDLER( rriot_1_b_r ) { return rriot_b_r(1); }
 READ8_HANDLER( rriot_2_b_r ) { return rriot_b_r(2); }
 READ8_HANDLER( rriot_3_b_r ) { return rriot_b_r(3); }

WRITE8_HANDLER( rriot_0_a_w ) { rriot_a_w(0,data); }
WRITE8_HANDLER( rriot_1_a_w ) { rriot_a_w(1,data); }
WRITE8_HANDLER( rriot_2_a_w ) { rriot_a_w(2,data); }
WRITE8_HANDLER( rriot_3_a_w ) { rriot_a_w(3,data); }

WRITE8_HANDLER( rriot_0_b_w ) { rriot_b_w(0,data); }
WRITE8_HANDLER( rriot_1_b_w ) { rriot_b_w(1,data); }
WRITE8_HANDLER( rriot_2_b_w ) { rriot_b_w(2,data); }
WRITE8_HANDLER( rriot_3_b_w ) { rriot_b_w(3,data); }


