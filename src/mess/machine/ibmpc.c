#include "driver.h"
#include "mscommon.h"
#include "machine/pcshare.h"
#include "includes/ibmpc.h"

#define VERBOSE_PIO 0		/* PIO (keyboard controller) */

#define VERBOSE_DBG 0       /* general debug messages */

#if VERBOSE_DBG
#define DBG_LOG(N,M,A) \
	if(VERBOSE_DBG>=N){ if( M )logerror("%11.6f: %-24s",timer_get_time(),(char*)M ); logerror A; }
#else
#define DBG_LOG(n,m,a)
#endif

#if VERBOSE_PIO
#define PIO_LOG(N,M,A) \
	if(VERBOSE_PIO>=N){ if( M )logerror("%11.6f: %-24s",timer_get_time(),(char*)M ); logerror A; }
#else
#define PIO_LOG(n,m,a)
#endif

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


/*************************************************************************
 *
 *		PIO
 *		parallel input output
 *
 *************************************************************************/
static  READ8_HANDLER  ( pc_ppi_porta_r );
static  READ8_HANDLER  ( pc_ppi_portb_r );
static  READ8_HANDLER  ( pc_ppi_portc_r );
static WRITE8_HANDLER ( pc_ppi_porta_w );
static WRITE8_HANDLER ( pc_ppi_portb_w );
static WRITE8_HANDLER ( pc_ppi_portc_w );

/* PC-XT has a 8255 which is connected to keyboard and other
status information */
ppi8255_interface pc_ppi8255_interface =
{
	1,
	{pc_ppi_porta_r},
	{pc_ppi_portb_r},
	{pc_ppi_portc_r},
	{pc_ppi_porta_w},
	{pc_ppi_portb_w},
	{pc_ppi_portc_w}
};

static struct {
	int portc_switch_high;
	int speaker;
	int keyboard_disabled;
} pc_ppi={ 0 };

 READ8_HANDLER (pc_ppi_porta_r)
{
	int data;

	/* KB port A */
	if (pc_ppi.keyboard_disabled)
	{
		/*   0  0 - no floppy drives  
		 *   1  Not used  
		 * 2-3  The number of memory banks on the system board  
		 * 4-5  Display mode
		 *	    11 = monochrome
		 *      10 - color 80x25
		 *      01 - color 40x25  
		 * 6-7  The number of floppy disk drives  
		 */
		data = readinputport(1);
	}
	else
	{
		data = pc_keyb_read();
	}
    PIO_LOG(1,"PIO_A_r",("$%02x\n", data));
    return data;
}

 READ8_HANDLER (pc_ppi_portb_r )
{
	int data;

	data = 0xff;
	PIO_LOG(1,"PIO_B_r",("$%02x\n", data));
	return data;
}

 READ8_HANDLER ( pc_ppi_portc_r )
{
	int data=0xff;

	data&=~0x80; // no parity error
	data&=~0x40; // no error on expansion board
	/* KB port C: equipment flags */
//	if (pc_port[0x61] & 0x08)
	if (pc_ppi.portc_switch_high)
	{
		/* read hi nibble of S2 */
		data = (data&0xf0)|((input_port_1_r(0) >> 4) & 0x0f);
		PIO_LOG(1,"PIO_C_r (hi)",("$%02x\n", data));
	}
	else
	{
		/* read lo nibble of S2 */
		data = (data&0xf0)|(input_port_1_r(0) & 0x0f);
		PIO_LOG(1,"PIO_C_r (lo)",("$%02x\n", data));
	}

	return data;
}

WRITE8_HANDLER ( pc_ppi_porta_w )
{
	/* KB controller port A */
	PIO_LOG(1,"PIO_A_w",("$%02x\n", data));
}

WRITE8_HANDLER ( pc_ppi_portb_w )
{
	/* KB controller port B */
	pc_ppi.portc_switch_high = data & 0x08;
	pc_ppi.keyboard_disabled = data & 0x80;
	pc_sh_speaker(data & 0x03);
	pc_keyb_set_clock(data & 0x40);

	if (data & 0x80)
		pc_keyb_clear();
}

WRITE8_HANDLER ( pc_ppi_portc_w )
{
	/* KB controller port C */
	PIO_LOG(1,"PIO_C_w",("$%02x\n", data));
}

// damned old checkit doesn't test at standard adresses
// will do more when I have a program supporting it
static struct {
	int data[0x18];
	mame_timer *timer;
} pc_rtc;

static void pc_rtc_timer(int param)
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

void pc_rtc_init(void)
{
	memset(&pc_rtc,0,sizeof(pc_rtc));
	pc_rtc.timer = timer_alloc(pc_rtc_timer);
	timer_adjust(pc_rtc.timer, 0, 0, 1.0);
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

/*************************************************************************
 *
 *		EXP
 *		expansion port
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

