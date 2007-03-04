/**********************************************************************
 * RIOT emulation
 *
 * MOS RAM IO Timer 6532
 *
 * TODO:
 * - test if it works ;)
 * - change a2600 drivers to use it
 *
 * synertek sym1
 * rockwell aim65 ?
 **********************************************************************/
#include "driver.h"
#include "includes/riot6532.h"


#if 0
#define LOG(a) logerror(a)
#else
#define LOG(a) ((void) 0)
#endif


/* 
aim65 hardware documentation contains readable 6532 info

Reg.    Name    Description
---------------------------
 0      PA      Port A data
 1      DDRA    Port A Data Direction Register
 2      PB      Port B data
 3      DDRB    Port B Data Direction Register
 4      Timer   Timer read register

0c   T1      1 clocks per decrement
0d   T8      8
0e   T64     64
0f   T1024   1024
*/

typedef struct {
	RIOT_CONFIG *config;
	struct {
		UINT8 in, out, ddr;
	} port_a, port_b;

	UINT8	irqen;		/* IRQ enabled ? */
	enum { Delay1, Delay8, Delay64, Delay1024, TimerShot } state;
	double shottime;
	int	irq;
	void	*timer; 	/* timer callback */
} RIOT;

static RIOT riot[MAX_RIOTS]= { {0} };

static void riot_timer_cb(int chip);

void riot_init(int nr, RIOT_CONFIG *config)
{
	memset(&riot[nr], 0, sizeof(riot[nr]));
	riot[nr].timer = timer_alloc(riot_timer_cb);
	riot[nr].config = config;

    LOG("RIOT - successfully initialised\n");
}

void riot_reset(int nr)
{
	riot[nr].port_a.ddr=0;
	if (riot[nr].config->port_a.output)
		riot[nr].config->port_a.output(nr,0xff);
	riot[nr].port_b.ddr=0;
	if (riot[nr].config->port_b.output)
		riot[nr].config->port_b.output(nr,0xff);
	
	riot[nr].irq=0;riot[nr].irqen=0;
	if (riot[nr].config->irq_callback) 
		riot[nr].config->irq_callback(nr, 0); 
}

int riot_r(int chip, int offset)
{
	int data = 0xff;

	offset&=0xf;
	if (!(offset&4)) { // io part of chip
		switch( offset&3 )
		{
		case 0: /* Data register A */
			if( riot[chip].config->port_a.input )
				data = (*riot[chip].config->port_a.input)(chip);
			/* mask input bits and combine with output bits */
			data = (data & ~riot[chip].port_a.ddr) | (riot[chip].port_a.out & riot[chip].port_a.ddr);
			LOG(("riot(%d) DRA   read : $%02x\n", chip, data));
			break;
		case 1: /* Data direction register A */
			data = riot[chip].port_a.ddr;
			LOG(("riot(%d) DDRA  read : $%02x\n", chip, data));
			break;
		case 2: /* Data register B */
			if( riot[chip].config->port_b.input )
				data = (*riot[chip].config->port_b.input)(chip);
			/* mask input bits and combine with output bits */
			data = (data & ~riot[chip].port_b.ddr) | (riot[chip].port_b.out & riot[chip].port_b.ddr);
//			LOG(("riot(%d) DRB   read : $%02x\n", chip, data));
			break;
		case 3: /* Data direction register B */
			data = riot[chip].port_b.ddr;
			LOG(("riot(%d) DDRB  read : $%02x\n", chip, data));
			break;
		}
	} else { // timer part of chip
		switch (offset&1) {
		case 0: /* Timer count read */
			switch (riot[chip].state) {
			case Delay1:
				if (riot[chip].timer)
					data = (int)(timer_timeleft(riot[chip].timer)*riot[chip].config->baseclock);
				break;
			case Delay8:
				if (riot[chip].timer)
					data = (int)(timer_timeleft(riot[chip].timer)*riot[chip].config->baseclock)>>3;
				break;
			case Delay64:
				if (riot[chip].timer)
					data = (int)(timer_timeleft(riot[chip].timer)*riot[chip].config->baseclock)>>6;
				break;
			case Delay1024:
				if (riot[chip].timer)
					data = (int)(timer_timeleft(riot[chip].timer)*riot[chip].config->baseclock)>>10;
				break;
			case TimerShot:
				data=255-(int)((timer_get_time()-riot[chip].shottime)*riot[chip].config->baseclock);
				if (data <0 ) data=0;
				break;
			}
			if( riot[chip].irqen&&riot[chip].irq ) /* with IRQ? */
			{
				if( riot[chip].config->irq_callback )
					(*riot[chip].config->irq_callback)(chip, 0);
			}
			riot[chip].irq=0;
			break;
		case 1: /* Timer status read */
			data = riot[chip].irq?0x80:0;
			LOG(("riot(%d) STAT  read : $%02x%s\n", chip, data, (char*)((offset & 8) ? " (IRQ)":"    ")));
			break;
		}
		// problem when restarting with read the timer!?
		riot[chip].irqen = (offset & 8) ? 1 : 0;
    }
	return data;
}

static void riot_timer_cb(int chip)
{
	LOG(("riot(%d) timer expired\n", chip));
	riot[chip].irq=1;
	riot[chip].state=TimerShot;
	riot[chip].shottime=timer_get_time();
	if( riot[chip].irqen ) /* with IRQ? */
	{
		if( riot[chip].config->irq_callback )
			(*riot[chip].config->irq_callback)(chip, 1);
	}
}

void riot_w(int chip, int offset, int data)
{
	offset&=0xf;
	if (!(offset&4)) {
		switch( offset&3 )
		{
		case 0: /* Data register A */
			LOG(("riot(%d) DRA  write: $%02x\n", chip, data));
			riot[chip].port_a.out = data;
			if( riot[chip].config->port_a.output )
				(*riot[chip].config->port_a.output)(chip,data);
			break;
		case 1: /* Data direction register A */
			LOG(("riot(%d) DDRA  write: $%02x\n", chip, data));
			riot[chip].port_a.ddr = data;
			break;
		case 2: /* Data register B */
//			LOG(("riot(%d) DRB   write: $%02x\n", chip, data));
			riot[chip].port_b.out = data;
			if( riot[chip].config->port_b.output )
				(*riot[chip].config->port_b.output)(chip,data);
			break;
		case 3: /* Data direction register B */
			LOG(("riot(%d) DDRB  write: $%02x\n", chip, data));
			riot[chip].port_b.ddr = data;
			break;
		}
	} else {
		if( riot[chip].irqen&&riot[chip].irq ) /* with IRQ? */
		{
			if( riot[chip].config->irq_callback )
				(*riot[chip].config->irq_callback)(chip, 0);
		}
		riot[chip].irqen = (offset & 8) ? 1 : 0;
		riot[chip].irq =0;
		
		switch (offset&3) {
		case 0: /* Timer 1 start */
			LOG(("rriot(%d) TMR1  write: $%02x%s\n", chip, data, (char*)((offset & 8) ? " (IRQ)":" ")));
			timer_adjust(riot[chip].timer, TIME_IN_HZ( (data+1) / riot[chip].config->baseclock), 
										  chip, 0);
			riot[chip].state=Delay1;
			break;
		case 1: /* Timer 8 start */
			LOG(("riot(%d) TMR8  write: $%02x%s\n", chip, data, (char*)((offset & 8) ? " (IRQ)":" ")));
			timer_adjust(riot[chip].timer, TIME_IN_HZ( (data+1) * 8 / riot[chip].config->baseclock), 
										  chip, 0);
			riot[chip].state=Delay8;
			break;
		case 2: /* Timer 64 start */
			LOG(("riot(%d) TMR64 write: $%02x%s\n", chip, data, (char*)((offset & 8) ? " (IRQ)":" ")));
//			LOG(("riot(%d) TMR64 write: time is $%f\n", chip, (double)(64.0 * (data + 1) / riot[chip].clock)));
			timer_adjust(riot[chip].timer, TIME_IN_HZ( (data+1) * 64 / riot[chip].config->baseclock), 
										  chip, 0);
			riot[chip].state=Delay64;
			break;
		case 3: /* Timer 1024 start */
			LOG(("riot(%d) TMR1K write: $%02x%s\n", chip, data, (char*)((offset & 8) ? " (IRQ)":" ")));
			timer_adjust(riot[chip].timer, TIME_IN_HZ( (data+1) * 1024 / riot[chip].config->baseclock), 
										  chip, 0);
			riot[chip].state=Delay1024;
			break;
		}
    }
}

static int riot_a_r(int chip)
{
	int data;
	data=(riot[chip].port_a.ddr&riot[chip].port_a.out);
	return data;
}

static int riot_b_r(int chip)
{
	int data;
	data=(riot[chip].port_b.ddr&riot[chip].port_b.out);
	return data;
}

static void riot_a_w(int chip, int data)
{
#if 0
	riot[chip].port_a.in = data;
#endif
}

static void riot_b_w(int chip, int data)
{
	
}

 READ8_HANDLER ( riot_0_r ) { return riot_r(0,offset); }
WRITE8_HANDLER ( riot_0_w ) { riot_w(0,offset,data); }
 READ8_HANDLER ( riot_1_r ) { return riot_r(1,offset); }
WRITE8_HANDLER ( riot_1_w ) { riot_w(1,offset,data); }
 READ8_HANDLER ( riot_2_r ) { return riot_r(2,offset); }
WRITE8_HANDLER ( riot_2_w ) { riot_w(2,offset,data); }
 READ8_HANDLER ( riot_3_r ) { return riot_r(3,offset); }
WRITE8_HANDLER ( riot_3_w ) { riot_w(3,offset,data); }

 READ8_HANDLER( riot_0_a_r ) { return riot_a_r(0); }
 READ8_HANDLER( riot_1_a_r ) { return riot_a_r(1); }
 READ8_HANDLER( riot_2_a_r ) { return riot_a_r(2); }
 READ8_HANDLER( riot_3_a_r ) { return riot_a_r(3); }

 READ8_HANDLER( riot_0_b_r ) { return riot_b_r(0); }
 READ8_HANDLER( riot_1_b_r ) { return riot_b_r(1); }
 READ8_HANDLER( riot_2_b_r ) { return riot_b_r(2); }
 READ8_HANDLER( riot_3_b_r ) { return riot_b_r(3); }

WRITE8_HANDLER( riot_0_a_w ) { riot_a_w(0,data); }
WRITE8_HANDLER( riot_1_a_w ) { riot_a_w(1,data); }
WRITE8_HANDLER( riot_2_a_w ) { riot_a_w(2,data); }
WRITE8_HANDLER( riot_3_a_w ) { riot_a_w(3,data); }

WRITE8_HANDLER( riot_0_b_w ) { riot_b_w(0,data); }
WRITE8_HANDLER( riot_1_b_w ) { riot_b_w(1,data); }
WRITE8_HANDLER( riot_2_b_w ) { riot_b_w(2,data); }
WRITE8_HANDLER( riot_3_b_w ) { riot_b_w(3,data); }


