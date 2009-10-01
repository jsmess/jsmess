#include "driver.h"
#include "memconv.h"
#include "includes/ibmpc.h"
#include "devices/cassette.h"
#include "machine/pit8253.h"

#include "machine/pcshare.h"

#define VERBOSE_PIO 0		/* PIO (keyboard controller) */

#define VERBOSE_DBG 0       /* general debug messages */

#define LOG(LEVEL,N,M,A) \
	do { \
		if(LEVEL>=N) \
		{ \
			if( M ) \
				logerror("%11.6f: %-24s",attotime_to_double(timer_get_time(machine)),(char*)M ); \
			logerror A; \
		} \
	} while (0)

#define DBG_LOG(N,M,A) LOG(VERBOSE_DBG,N,M,A)
#define PIO_LOG(N,M,A) LOG(VERBOSE_PIO,N,M,A)

/*
ibm xt bios
-----------

fe0ac
fe10c
fe12b: hangs after reset
fe15e
fe19c
fe2c6
 graphics adapter sync signals
fe332
 search roms
fe35d
 pic test
fe38f
fe3c6
fe3e6
 301 written
 expect 0xaa after reset, send when lines activated in short time
 (keyboard polling in frame interrupt is not quick enough now)
fe424
fe448
 i/o expansion test
fe49c
 memory test
fe500
 harddisk bios used
fe55c
 disk booting

 f0bef
  f0b85
   f096d
    disk related
    feca0
     prueft kanal 0 adress register (memory refresh!?)

ibm pc bios
-----------
fe104
fe165
fe1b4
fe205
fe23f
fe256
fe363
fe382 expansion test
fe3c4
 memory test
fe43b
fe443 call fe643 keyboard test
fe4a1 call ff979 tape!!! test
*/


// damned old checkit doesn't test at standard adresses
// will do more when I have a program supporting it
static struct {
	int data[0x18];
	emu_timer *timer;
} pc_rtc;

static TIMER_CALLBACK(pc_rtc_timer)
{
	int year;
	if (++pc_rtc.data[2]>=60) {
		pc_rtc.data[2]=0;
		if (++pc_rtc.data[3]>=60) {
			pc_rtc.data[3]=0;
			if (++pc_rtc.data[4]>=24) {
				pc_rtc.data[4]=0;
				pc_rtc.data[5]=(pc_rtc.data[5]%7)+1;
				year=pc_rtc.data[9]+2000;
				if (++pc_rtc.data[6]>=gregorian_days_in_month(pc_rtc.data[7], year)) {
					pc_rtc.data[6]=1;
					if (++pc_rtc.data[7]>12) {
						pc_rtc.data[7]=1;
						pc_rtc.data[9]=(pc_rtc.data[9]+1)%100;
					}
				}
			}
		}
	}
}

void pc_rtc_init(running_machine *machine)
{
	memset(&pc_rtc,0,sizeof(pc_rtc));
	pc_rtc.timer = timer_alloc(machine, pc_rtc_timer, NULL);
	timer_adjust_periodic(pc_rtc.timer, attotime_zero, 0, attotime_make(1, 0));
}

READ8_HANDLER( pc_rtc_r )
{
	int data;
	switch (offset) {
	default:
		data=pc_rtc.data[offset];
	}
	logerror( "rtc read %.2x %.2x\n", offset, data);
	return data;
}

WRITE8_HANDLER( pc_rtc_w )
{
	logerror( "rtc write %.2x %.2x\n", offset, data);
	switch(offset) {
	default:
		pc_rtc.data[offset]=data;
	}
}

READ16_HANDLER( pc16le_rtc_r ) { return read16le_with_read8_handler(pc_rtc_r, space, offset, mem_mask); }
WRITE16_HANDLER( pc16le_rtc_w ) { write16le_with_write8_handler(pc_rtc_w, space, offset, data, mem_mask); }

/*************************************************************************
 *
 *      EXP
 *      expansion port
 *
 *************************************************************************/

// I even don't know what it is!
static struct {
	/*
      reg 0 ram behaviour if in
      reg 3 write 1 to enable it
      reg 4 ram behaviour ???
      reg 5,6 (5 hi, 6 lowbyte) ???
    */
	/* selftest in ibmpc, ibmxt */
	UINT8 reg[8];
} pc_expansion={ { 0,0,0,0,0,0,1 } };

WRITE8_HANDLER ( pc_EXP_w )
{
	running_machine *machine = space->machine;

	DBG_LOG(1,"EXP_unit_w",("%.2x $%02x\n", offset, data));
	switch (offset) {
	case 4:
		pc_expansion.reg[4]=pc_expansion.reg[5]=pc_expansion.reg[6]=data;
		break;
	default:
		pc_expansion.reg[offset] = data;
	}
}

READ8_HANDLER ( pc_EXP_r )
{
    int data;
	UINT16 a;
	running_machine *machine = space->machine;

	switch (offset) {
	case 6:
		data = pc_expansion.reg[offset];
		a=(pc_expansion.reg[5]<<8)|pc_expansion.reg[6];
		a<<=1;
		pc_expansion.reg[5]=a>>8;
		pc_expansion.reg[6]=a&0xff;
		break;
	default:
		data = pc_expansion.reg[offset];
	}
    DBG_LOG(1,"EXP_unit_r",("%.2x $%02x\n", offset, data));
	return data;
}

