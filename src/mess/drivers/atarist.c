/*******************************************************************************



	Emulation by Bryan McPhail, mish@tendril.co.uk

	Todo:
		Blitter bug
		Other IKBD commands
		Floppy Drive B
		Hard disk emu
		Load complete disk image at once to avoid frameskip problems


	Keyboard Notes, due to differences between ST & PC keyboards:
		Caps Lock is mapped to Right ALT
		ISO is mapped to Right Control
		Undo is mapped to Page Up
		Help is mapped to Page Down
		Keypad ( is mapped to F11
		Keypad ) is mapped to F12
		@ is mapped to tilde (~)

	Issues:
		gau_658 (or 668?) Dungeon Master - keyboard doesn't work on menu screen
		pp_35 - Castle Master hangs (timer A related?)
		PP57 hangs before menu
		PP60 - Lotus Turbo - slight flicker in last line (probably changing screenbase)
		PP15 - Rtype hangs writing keyboard resets?
		PP16 - Altered Beast hangs - keyboard codes again
		PP17 - Strider hangs when starting a game
		PP18 - Toobin, random raster streaks (screenbase)
		PP20 - Hard Driving, random raster streaks (screenbase)
		PP33 - Top border menus?  Timer A used
		Medway73 - Trace mode decryption routine!
		Venus The Flytrap - in game raster effects don't work.


Machine:
	MK68681 MFP (timers, interrupts, serial I/O)
	'Fake' IKBD (intelligent keyboard) emulation
	WD1772 disk controller + DMA system
	'Fake' Hard disk emulation

Vidhrdw:
	Shifter chip emulation
	Blitter chip emulation


*******************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "machine/wd17xx.h"
#include "cpu/m68000/m68k.h"
#include "cpu/m68000/m68000.h"
#include "devices/basicdsk.h"

#define MASTER_CLOCK	8000000
#define	MK68901_CLOCK	2457600
//#define MASTER_CLOCK	(8000000*1.2)
//#define	MK68901_CLOCK	(2457600*1.2)

//static int disk_changed[2];

UINT16 *atarist_ram,*sub_ram,*atarist_bank_ram,*atarist_edfs_ram,*atarist_blitter_ram;
unsigned int pal_lookup[16];

int atarist_current_drive;

UINT16 *atarist_fakehdc_ram;

static int atarist_options;

static int atarist_vidram_base;

static int wd1772_active=0;

// move to struct
static int current_line,current_pixel,shifter_sync,start_line,end_line,resolution,atarist_shifter_offset;


static int acia_status=0xe;
static int acia_buffer[64],acia_bufpos=0,acia_bufptr=0;

struct MFP {
	UINT8 genr,vr,ddr,aer;
	UINT8 iea,ipa,ima,isa;
	UINT8 ieb,ipb,imb,isb;
	UINT8 tacr,tbcr,tcdcr;
	UINT8 taty,tbty;
	UINT8 tarl,tbrl,tcrl,tdrl;
	int tadr,tbdr,tcdr,tddr;
	int timer_c_cycles;
	void *timer_a,*timer_b,*timer_c,*timer_d;
} mfp;

static void mfp_init(void)
{
	mfp.genr=mfp.vr=mfp.aer=mfp.ddr=0;
	mfp.iea=mfp.ipa=mfp.ima=mfp.isa=0;
	mfp.ieb=mfp.ipb=mfp.imb=mfp.isb=0;
	mfp.tadr=mfp.tbdr=mfp.tcdr=mfp.tddr=256;
	mfp.tacr=mfp.tbcr=mfp.tcdcr=0;
	mfp.tarl=mfp.tbrl=mfp.tcrl=mfp.tdrl=0; /* reload values */
	mfp.taty=mfp.tbty=0; /* Timer types */
	mfp.timer_a=mfp.timer_b=mfp.timer_c=mfp.timer_d=NULL;
	mfp.timer_c_cycles=0;
}

static VIDEO_START( atarist )
{
	int i;
	for (i=0; i<512; i++)
		palette_set_color_rgb(machine, i,((i>>6)&7)*0x24,((i>>3)&7)*0x24,(i&7)*0x24);
}

void atarist_drawborder(int line)
{
	unsigned short *bm=(unsigned short *)Machine->scrbitmap->line[line]+96*2;
	int x,i;

	if (line<60) return;
	if (line>304) return;

//	for (x=0; x<640; x+=16)
	for (x=0; x<640; x+=16)
		for (i=0; i<16; i++)
			bm[x+i]=Machine->pens[pal_lookup[0]];
}

void atarist_drawline(int line, int start, int end)
{
	int x;
	unsigned short *bm;
	int d=atarist_vidram_base+atarist_shifter_offset;
	int p1,p2,p3,p4;

	if (line<60) return;
	if (line>304) return;

//	if (resolution==0) line=line*2; /* Pixel doubling in low-res */

if (current_pixel!=96) return; //hmm

	bm = (unsigned short *)Machine->scrbitmap->line[line]+start*2;
//	for (x=0; x<640; x+=32) {
	for (x=0; x<(end-start)*2; x+=16) {
		p1 = READ_WORD(&atarist_ram[d+0]);
		p2 = READ_WORD(&atarist_ram[d+2]);
		p3 = READ_WORD(&atarist_ram[d+4]);
		p4 = READ_WORD(&atarist_ram[d+6]);

		if (resolution==0) {
#if 1
		bm[30]=bm[31]=Machine->pens[pal_lookup[((p1>> 0)&0x1) | ((p2<< 1)&0x2) | ((p3<< 2)&0x4) | ((p4<< 3)&0x8)]];
		bm[28]=bm[29]=Machine->pens[pal_lookup[((p1>> 1)&0x1) | ((p2>> 0)&0x2) | ((p3<< 1)&0x4) | ((p4<< 2)&0x8)]];
		bm[26]=bm[27]=Machine->pens[pal_lookup[((p1>> 2)&0x1) | ((p2>> 1)&0x2) | ((p3<< 0)&0x4) | ((p4<< 1)&0x8)]];
		bm[24]=bm[25]=Machine->pens[pal_lookup[((p1>> 3)&0x1) | ((p2>> 2)&0x2) | ((p3>> 1)&0x4) | ((p4>> 0)&0x8)]];
		bm[22]=bm[23]=Machine->pens[pal_lookup[((p1>> 4)&0x1) | ((p2>> 3)&0x2) | ((p3>> 2)&0x4) | ((p4>> 1)&0x8)]];
		bm[20]=bm[21]=Machine->pens[pal_lookup[((p1>> 5)&0x1) | ((p2>> 4)&0x2) | ((p3>> 3)&0x4) | ((p4>> 2)&0x8)]];
		bm[18]=bm[19]=Machine->pens[pal_lookup[((p1>> 6)&0x1) | ((p2>> 5)&0x2) | ((p3>> 4)&0x4) | ((p4>> 3)&0x8)]];
		bm[16]=bm[17]=Machine->pens[pal_lookup[((p1>> 7)&0x1) | ((p2>> 6)&0x2) | ((p3>> 5)&0x4) | ((p4>> 4)&0x8)]];
		bm[14]=bm[15]=Machine->pens[pal_lookup[((p1>> 8)&0x1) | ((p2>> 7)&0x2) | ((p3>> 6)&0x4) | ((p4>> 5)&0x8)]];
		bm[12]=bm[13]=Machine->pens[pal_lookup[((p1>> 9)&0x1) | ((p2>> 8)&0x2) | ((p3>> 7)&0x4) | ((p4>> 6)&0x8)]];
		bm[10]=bm[11]=Machine->pens[pal_lookup[((p1>>10)&0x1) | ((p2>> 9)&0x2) | ((p3>> 8)&0x4) | ((p4>> 7)&0x8)]];
		bm[ 8]=bm[ 9]=Machine->pens[pal_lookup[((p1>>11)&0x1) | ((p2>>10)&0x2) | ((p3>> 9)&0x4) | ((p4>> 8)&0x8)]];
		bm[ 6]=bm[ 7]=Machine->pens[pal_lookup[((p1>>12)&0x1) | ((p2>>11)&0x2) | ((p3>>10)&0x4) | ((p4>> 9)&0x8)]];
		bm[ 4]=bm[ 5]=Machine->pens[pal_lookup[((p1>>13)&0x1) | ((p2>>12)&0x2) | ((p3>>11)&0x4) | ((p4>>10)&0x8)]];
		bm[ 2]=bm[ 3]=Machine->pens[pal_lookup[((p1>>14)&0x1) | ((p2>>13)&0x2) | ((p3>>12)&0x4) | ((p4>>11)&0x8)]];
		bm[ 0]=bm[ 1]=Machine->pens[pal_lookup[((p1>>15)&0x1) | ((p2>>14)&0x2) | ((p3>>13)&0x4) | ((p4>>12)&0x8)]];
#endif
#if 0
		bm[15]=Machine->pens[pal_lookup[((p1>> 0)&0x1) | ((p2<< 1)&0x2) | ((p3<< 2)&0x4) | ((p4<< 3)&0x8)]];
		bm[14]=Machine->pens[pal_lookup[((p1>> 1)&0x1) | ((p2>> 0)&0x2) | ((p3<< 1)&0x4) | ((p4<< 2)&0x8)]];
		bm[13]=Machine->pens[pal_lookup[((p1>> 2)&0x1) | ((p2>> 1)&0x2) | ((p3<< 0)&0x4) | ((p4<< 1)&0x8)]];
		bm[12]=Machine->pens[pal_lookup[((p1>> 3)&0x1) | ((p2>> 2)&0x2) | ((p3>> 1)&0x4) | ((p4>> 0)&0x8)]];
		bm[11]=Machine->pens[pal_lookup[((p1>> 4)&0x1) | ((p2>> 3)&0x2) | ((p3>> 2)&0x4) | ((p4>> 1)&0x8)]];
		bm[10]=Machine->pens[pal_lookup[((p1>> 5)&0x1) | ((p2>> 4)&0x2) | ((p3>> 3)&0x4) | ((p4>> 2)&0x8)]];
		bm[ 9]=Machine->pens[pal_lookup[((p1>> 6)&0x1) | ((p2>> 5)&0x2) | ((p3>> 4)&0x4) | ((p4>> 3)&0x8)]];
		bm[ 8]=Machine->pens[pal_lookup[((p1>> 7)&0x1) | ((p2>> 6)&0x2) | ((p3>> 5)&0x4) | ((p4>> 4)&0x8)]];
		bm[ 7]=Machine->pens[pal_lookup[((p1>> 8)&0x1) | ((p2>> 7)&0x2) | ((p3>> 6)&0x4) | ((p4>> 5)&0x8)]];
		bm[ 6]=Machine->pens[pal_lookup[((p1>> 9)&0x1) | ((p2>> 8)&0x2) | ((p3>> 7)&0x4) | ((p4>> 6)&0x8)]];
		bm[ 5]=Machine->pens[pal_lookup[((p1>>10)&0x1) | ((p2>> 9)&0x2) | ((p3>> 8)&0x4) | ((p4>> 7)&0x8)]];
		bm[ 4]=Machine->pens[pal_lookup[((p1>>11)&0x1) | ((p2>>10)&0x2) | ((p3>> 9)&0x4) | ((p4>> 8)&0x8)]];
		bm[ 3]=Machine->pens[pal_lookup[((p1>>12)&0x1) | ((p2>>11)&0x2) | ((p3>>10)&0x4) | ((p4>> 9)&0x8)]];
		bm[ 2]=Machine->pens[pal_lookup[((p1>>13)&0x1) | ((p2>>12)&0x2) | ((p3>>11)&0x4) | ((p4>>10)&0x8)]];
		bm[ 1]=Machine->pens[pal_lookup[((p1>>14)&0x1) | ((p2>>13)&0x2) | ((p3>>12)&0x4) | ((p4>>11)&0x8)]];
		bm[ 0]=Machine->pens[pal_lookup[((p1>>15)&0x1) | ((p2>>14)&0x2) | ((p3>>13)&0x4) | ((p4>>12)&0x8)]];
#endif
		} else if (resolution==1) {
		bm[31]=Machine->pens[pal_lookup[((p3>> 0)&0x1) | ((p4<< 1)&0x2)]];
		bm[30]=Machine->pens[pal_lookup[((p3>> 1)&0x1) | ((p4>> 0)&0x2)]];
		bm[29]=Machine->pens[pal_lookup[((p3>> 2)&0x1) | ((p4>> 1)&0x2)]];
		bm[28]=Machine->pens[pal_lookup[((p3>> 3)&0x1) | ((p4>> 2)&0x2)]];
		bm[27]=Machine->pens[pal_lookup[((p3>> 4)&0x1) | ((p4>> 3)&0x2)]];
		bm[26]=Machine->pens[pal_lookup[((p3>> 5)&0x1) | ((p4>> 4)&0x2)]];
		bm[25]=Machine->pens[pal_lookup[((p3>> 6)&0x1) | ((p4>> 5)&0x2)]];
		bm[24]=Machine->pens[pal_lookup[((p3>> 7)&0x1) | ((p4>> 6)&0x2)]];
		bm[23]=Machine->pens[pal_lookup[((p3>> 8)&0x1) | ((p4>> 7)&0x2)]];
		bm[22]=Machine->pens[pal_lookup[((p3>> 9)&0x1) | ((p4>> 8)&0x2)]];
		bm[21]=Machine->pens[pal_lookup[((p3>>10)&0x1) | ((p4>> 9)&0x2)]];
		bm[20]=Machine->pens[pal_lookup[((p3>>11)&0x1) | ((p4>>10)&0x2)]];
		bm[19]=Machine->pens[pal_lookup[((p3>>12)&0x1) | ((p4>>11)&0x2)]];
		bm[18]=Machine->pens[pal_lookup[((p3>>13)&0x1) | ((p4>>12)&0x2)]];
		bm[17]=Machine->pens[pal_lookup[((p3>>14)&0x1) | ((p4>>13)&0x2)]];
		bm[16]=Machine->pens[pal_lookup[((p3>>15)&0x1) | ((p4>>14)&0x2)]];

		bm[15]=Machine->pens[pal_lookup[((p1>> 0)&0x1) | ((p2<< 1)&0x2)]];
		bm[14]=Machine->pens[pal_lookup[((p1>> 1)&0x1) | ((p2>> 0)&0x2)]];
		bm[13]=Machine->pens[pal_lookup[((p1>> 2)&0x1) | ((p2>> 1)&0x2)]];
 		bm[12]=Machine->pens[pal_lookup[((p1>> 3)&0x1) | ((p2>> 2)&0x2)]];
		bm[11]=Machine->pens[pal_lookup[((p1>> 4)&0x1) | ((p2>> 3)&0x2)]];
		bm[10]=Machine->pens[pal_lookup[((p1>> 5)&0x1) | ((p2>> 4)&0x2)]];
		bm[ 9]=Machine->pens[pal_lookup[((p1>> 6)&0x1) | ((p2>> 5)&0x2)]];
		bm[ 8]=Machine->pens[pal_lookup[((p1>> 7)&0x1) | ((p2>> 6)&0x2)]];
		bm[ 7]=Machine->pens[pal_lookup[((p1>> 8)&0x1) | ((p2>> 7)&0x2)]];
		bm[ 6]=Machine->pens[pal_lookup[((p1>> 9)&0x1) | ((p2>> 8)&0x2)]];
		bm[ 5]=Machine->pens[pal_lookup[((p1>>10)&0x1) | ((p2>> 9)&0x2)]];
 		bm[ 4]=Machine->pens[pal_lookup[((p1>>11)&0x1) | ((p2>>10)&0x2)]];
		bm[ 3]=Machine->pens[pal_lookup[((p1>>12)&0x1) | ((p2>>11)&0x2)]];
		bm[ 2]=Machine->pens[pal_lookup[((p1>>13)&0x1) | ((p2>>12)&0x2)]];
		bm[ 1]=Machine->pens[pal_lookup[((p1>>14)&0x1) | ((p2>>13)&0x2)]];
		bm[ 0]=Machine->pens[pal_lookup[((p1>>15)&0x1) | ((p2>>14)&0x2)]];

		}

		d+=8;
		bm+=32;
//		bm+=16;
	}

		current_pixel=end;

//	if (resolution==0) {
//		bm = Machine->scrbitmap->line[line+1];
//		bn = Machine->scrbitmap->line[line];
//		for (x=0; x<640; x++)
//			bm[x]=bn[x];
//	}

}

/* This handles any 'mid-line' graphics changes (start position, palette, etc) */
void atarist_pixel_update(void)
{
	int c=512-cpu_geticount(); /* 512 cycles per line */

	/* Are in left border? If so we don't need to update this line */
	if (c<96) return;

	/* Are we in right border?  If so, we can draw entire line */
//	if (c>415) {
		atarist_drawline(current_line,96/*current_pixel*/,(320+96)*2);
//		return;
//	}

//	/* Else midline update */
//	if ( c > current_pixel + 8) {
//		atarist_drawline(current_line,current_pixel,c);
	//	return;
//	}
}

static VIDEO_UPDATE( atarist )
{
	return 0;
}

/***************************************************************************/

static READ16_HANDLER( atarist_psg_r )
{
	return AY8910_read_port_0_r(0)<<8;
}

static WRITE16_HANDLER( atarist_psg_w )
{
	offset&=3; /* Mirror addresses */
	if (offset)
		AY8910_write_port_0_w(0,(data>>8)&0xff);
	else
		AY8910_control_port_0_w(0,(data>>8)&0xff);
}

/***************************************************************************/

static int precounter[]={0,4,10,16,50,64,100,200};
enum MODES { NONE=0, DELAY, EVENT, WIDTH };

static void atarist_mfp_interrupt(int level)
{
	/* The highest level should take priority over lower levels, but does
	  it really matter?  There will be very few interrupt conflicts anyway */
	cpunum_set_input_line_vector(0, 6, 0x40 + level);
	cpunum_set_input_line(0, 6, ASSERT_LINE);
}

static void timer_a_callback(int param)
{
	if (mfp.iea&0x20) { /* Interrupt enabled */
		mfp.ipa=mfp.ipa|0x20; /* Set pending interrupt bit */
		logerror("Timer A IRQ fired! (line %d)\n",current_line);
		if (mfp.ima&0x20)
			atarist_mfp_interrupt(13);
	}
}

static void timer_b_callback(int param)
{
	if (mfp.iea&0x01) { /* Interrupt enabled */
		mfp.ipa=mfp.ipa|0x01; /* Set pending interrupt bit */
//		logerror("Timer B IRQ fired! (line %d)\n",current_line);
		if (mfp.ima&0x01)
			atarist_mfp_interrupt(8);
	}
}

static void timer_c_callback(int param)
{
	if (mfp.ieb&0x20) { /* Interrupt enabled */
		mfp.ipb=mfp.ipb|0x20; /* Set pending interrupt bit */
		if (mfp.imb&0x20)
			atarist_mfp_interrupt(5);
	}
}

static void timer_d_callback(int param)
{
	if (mfp.ieb&0x10) { /* Interrupt enabled */
		mfp.ipb=mfp.ipb|0x10; /* Set pending interrupt bit */
		logerror("Timer D IRQ fired!\n");
		if (mfp.imb&0x10)
			atarist_mfp_interrupt(4);
	}
}


/* MFP Master Clock is 2,457,600 cycles/second */
static READ16_HANDLER( atarist_mfp_r )
{
	switch (offset) {
	case 0x00: /* General purpose IO */
//		logerror("%06x: MFP 00 read\n", activecpu_get_pc());
		return 0x80 | mfp.genr | wd1772_active; //wd1772_active=0x20 for BUSY
	case 0x02: /* Active Edge Register */
//		logerror("%06x: MFP 02 read\n", activecpu_get_pc());
		return mfp.aer;
	case 0x04: /* Data direction */
//		logerror("%06x: MFP 04 read\n", activecpu_get_pc());
		return mfp.ddr;


	case 0x06: /* Interrupt enable A */
		return mfp.iea;
	case 0x0a: /* Interrupt Pending A */
//		logerror("%06x: MFP 0xa read (Timer A poll)\n", activecpu_get_pc());
		return mfp.ipa;
	case 0x0e: /* Interrupt In Service A */
		return mfp.isa;
	case 0x12: /* Interrupt Mask A */
		return mfp.ima;

	case 0x08: /* Interrupt enable B */
		return mfp.ieb;
	case 0x0c: /* Interrupt Pending B */
//		logerror("%06x: MFP 0xC read (Timer B poll)\n", activecpu_get_pc());
		return mfp.ipb;
	case 0x10: /* Interrupt In Service B */
		return mfp.isb;
	case 0x14: /* Interrupt Mask B */
		return mfp.imb;

	case 0x16: /* Vector register */
		return mfp.vr;

	case 0x18: /* Timer A control */
		return mfp.tacr;
	case 0x1a: /* Timer B control */
		return mfp.tbcr;
	case 0x1c: /* Timer C & D control */
		return mfp.tcdcr;

	case 0x1e: /* Timer A data register (TADR) */
		if (mfp.taty==DELAY) {
			logerror("Timer A unsupported delay mode read - (%02x)\n",mfp.tadr);
			return 1;
		}
		return mfp.tadr&0xff;
	case 0x20: /* Timer B data register (TBDR) */
		if (mfp.tbty==DELAY) {
			logerror("Timer B unsupported delay mode read - (%02x)\n",mfp.tbdr);
			return 1;
		}
		return mfp.tbdr&0xff;
	case 0x22: /* Timer C data register (TCDR) */
		//if delay timer - update DR on the fly here

if (mfp.timer_c)
		logerror("Timer C read - unsupported, timer c cycles is %d, reload is %02x, cyc values is %d, data value is %04x\n",mfp.timer_c_cycles,mfp.tcrl,TIME_IN_CYCLES(timer_timeelapsed(mfp.timer_c),0),(TIME_IN_CYCLES(timer_timeelapsed(mfp.timer_c),0)/mfp.timer_c_cycles/mfp.tcrl));


		//logerror("Timer C read, value is %02x\n",mfp.tcdr-((TIME_IN_CYCLES(MASTER_CLOCK/MK68901_CLOCK,0)-TIME_IN_CYCLES(timer_timeleft,0)/precounter[(mfp.tcdr>>4)&7]));
		if (mfp.timer_c && mfp.timer_c_cycles && mfp.tcrl)
			return (TIME_IN_CYCLES(timer_timeleft(mfp.timer_c),0)/(mfp.timer_c_cycles/mfp.tcrl));   //(mfp.tcdr&0xff)-((TIME_IN_CYCLES(MASTER_CLOCK/MK68901_CLOCK,0)-  TIME_IN_CYCLES(timer_timeleft,0)/precounter[(mfp.tcdr>>4)&7]);
		return mfp.tcdr&0xff;
	case 0x24: /* Timer D data register (TDDR) */
		logerror("Timer D read - unsupported\n");
		return mfp.tddr&0xff;
	}

	logerror("%06x:  Unmapped mfp read %02x\n",activecpu_get_pc(),offset);

	return 0;
}

static WRITE16_HANDLER( atarist_mfp_w )
{
	switch (offset) {

	case 0x02: /* Active edge register */
		mfp.aer=data&0xff;
		return;
	case 0x04: /* Data direction register */
		mfp.ddr=data&0xff;
		return;

	case 0x06: /* Interrupt Enable A */
		mfp.iea=data&0xff;
		return;
	case 0x0a: /* Interrupt Pending A */
		mfp.ipa=data&0xff;
		return;
	case 0x0e: /* Interrupt In Service A */
		mfp.isa=data&0xff;
		return;
	case 0x12: /* Interrupt Mask A */
		mfp.ima=data&0xff;
		return;

	case 0x08: /* Interrupt Enable B */
		mfp.ieb=data&0xff;
		return;
	case 0x0c: /* Interrupt Pending B */
		mfp.ipb=data&0xff;
		return;
	case 0x10: /* Interrupt In Service B */
		mfp.isb=data&0xff;
		return;
	case 0x14: /* Interrupt Mask B */
		mfp.imb=data&0xff;
		return;

	case 0x16: /* Vector register */
		mfp.vr=data&0xff;
		return;

	case 0x18: /* Timer A control register (TACR) */
		if (data!=mfp.tacr) { /* Timer A changed state - stop it */
			if (mfp.timer_a) logerror("Timer A stopped\n");
			if (mfp.timer_a) timer_remove(mfp.timer_a);
			mfp.timer_a=NULL;
			mfp.taty=NONE;
		}
		if ((data&0x7)!=0 && mfp.timer_a==NULL) { /* Timer A started, from stopped state */
            logerror("Timer A pulse enabled - reload %04x, divider is %d, set to fire every %d cycles\n",mfp.tarl,precounter[data&7],(MASTER_CLOCK/MK68901_CLOCK)*mfp.tarl*precounter[data&7]);
			if (((MASTER_CLOCK/MK68901_CLOCK)*mfp.tarl*precounter[data&7])>511) /* Don't bother with small timers - too slow */
 				mfp.timer_a=timer_pulse(TIME_IN_CYCLES((MASTER_CLOCK/MK68901_CLOCK)*mfp.tarl*precounter[data&7],0), 0, timer_a_callback);
			else logerror("Pulse period too high - timer not set\n");

			mfp.taty=DELAY;
		}
		if ((data&0x8)!=0 && mfp.timer_a==NULL) { /* Timer A started, from stopped state */
			logerror("Timer A event count enabled\n");
			mfp.taty=EVENT;
		}

		/* Not implemented : Pulse-width timer (nothing uses it anyway) */
		if ((data&0xf)>0x8) {
			logerror("Timer A set to pulse width mode...\n");
		}

		mfp.tacr=data&0xff;
		return;

	case 0x1a: /* Timer B control register (TBCR) */
		if (data!=mfp.tbcr) { /* Timer B changed state - stop it */
			if (mfp.timer_b) logerror("Timer B stopped\n");
			if (mfp.timer_b) timer_remove(mfp.timer_b);
			mfp.timer_b=NULL;
			mfp.tbty=NONE;
		}
		if ((data&0x7)!=0 && mfp.timer_b==NULL) { /* Timer B started, from stopped state */
			logerror("Timer B pulse enabled - reload %04x, divider is %d, set to fire every %d cycles\n",mfp.tbrl,precounter[data&7],(MASTER_CLOCK/MK68901_CLOCK)*mfp.tbrl*precounter[data&7]);
			if (((MASTER_CLOCK/MK68901_CLOCK)*mfp.tbrl*precounter[data&7])>511) /* Dont fire less than this */
				mfp.timer_b=timer_pulse(TIME_IN_CYCLES((MASTER_CLOCK/MK68901_CLOCK)*mfp.tbrl*precounter[data&7],0), 0, timer_b_callback);
			else logerror("Pulse period too high - timer not set\n");

			mfp.tbty=DELAY;
		}
		if ((data&0x8)!=0 && mfp.timer_b==NULL) { /* Timer B started, from stopped state */
//			logerror("Timer B event count enabled (%06x), dt is %02x (line %d)\n",activecpu_get_pc(),mfp.tbrl,current_line);
			mfp.tbty=EVENT;
		}

		/* Not implemented : Pulse-width timer (nothing uses it anyway) */
		if ((data&0xf)>0x8) {
			logerror("Timer B set to pulse width mode...\n");
		}

		mfp.tbcr=data&0xff;
		return;

	case 0x1c: /* Timer C & D control register (TCDCR) */
		if (((data&0x70)>>4)!=((mfp.tcdcr&0x70)>>4)) { /* Timer C changed state - stop it */
			if (mfp.timer_c) logerror("Timer C stopped\n");
			if (mfp.timer_c) timer_remove(mfp.timer_c);
			mfp.timer_c=NULL;
		}
		if (((data&0x70)>>4)!=0 && mfp.timer_c==NULL) { /* Timer C started, from stopped state */
			mfp.timer_c_cycles=(MASTER_CLOCK/MK68901_CLOCK)*mfp.tcrl*precounter[(data>>4)&7];
			mfp.timer_c=timer_pulse(TIME_IN_CYCLES(mfp.timer_c_cycles, 0), 0, timer_c_callback);
			logerror("Timer C pulse enabled - reload %04x, divider is %d, set to fire every %d cycles\n",mfp.tcrl,precounter[(data>>4)&7],mfp.timer_c_cycles);
		}

		if ((data&0x7)!=(mfp.tcdcr&0x7)) { /* Timer D changed state - stop it */
			if (mfp.timer_d) logerror("Timer D stopped\n");
			if (mfp.timer_d) timer_remove(mfp.timer_d);
			mfp.timer_d=NULL;
		}
		if ((data&0x7)!=0 && mfp.timer_d==NULL) { /* Timer D started, from stopped state */
			logerror("Timer D pulse enabled - reload %04x, divider is %d, set to fire every %d cycles\n",mfp.tdrl,precounter[data&7],(MASTER_CLOCK/MK68901_CLOCK)*mfp.tdrl*precounter[data&7]);
			if (((MASTER_CLOCK/MK68901_CLOCK)*mfp.tdrl*precounter[data&7])>511) /* Dont fire less than this */
				mfp.timer_d=timer_pulse(TIME_IN_CYCLES((MASTER_CLOCK/MK68901_CLOCK)*mfp.tdrl*precounter[data&7],0), 0, timer_d_callback);
			else logerror("Pulse period too high - timer not set\n");
		}

		mfp.tcdcr=data&0xff;
		return;

	case 0x1e: /* Timer A data register (TADR) */
		data&=0xff;
		if (!data) data=256;
		mfp.tadr=mfp.tarl=data;
		return;
	case 0x20: /* Timer B data register (TBDR) */
		data&=0xff;
		if (!data) data=256;
		mfp.tbdr=mfp.tbrl=data;
		return;
	case 0x22: /* Timer C data register (TCDR) */
		data&=0xff;
		if (!data) data=256;
		mfp.tcdr=mfp.tcrl=data;
		return;
	case 0x24: /* Timer D data register (TDDR) */
		data&=0xff;
		if (!data) data=256;
		mfp.tddr=mfp.tdrl=data;
		return;
	}

	logerror("%06x:  Unmapped mfp write %02x %04x\n",activecpu_get_pc(),offset,data);
}

/************************************************************************************/

int acia_joystick_event_mode,acia_mouse_event_mode;

static void ikbd_reset(void)
{
	acia_bufpos=acia_buffer[0]=acia_bufptr=0;
	acia_joystick_event_mode=1;
	acia_mouse_event_mode=1; /* Event reporting mode is default */
}

static void atarist_fake_ikbd_w(int data)
{
	static int input_buffer[32],bufpos=0,needed=0,current_command=0,last_mouse_fire=0;
	static int mouse_position[2],mouse_max[2];
	static int last_frame[2];
	int mouse_fire,mouse_return=0;
	int new_frame[2];

	new_frame[0]=readinputport(1);
	new_frame[1]=readinputport(2);

	acia_status=0xe;

	/* Multi-byte commands */
	if (needed) {
		input_buffer[(bufpos++)&0x1f]=data;
		needed--;
		if (needed==0) { /* Process command if all bytes received */
			bufpos=0;
			switch (current_command) {
				case 0x07: /* Set mouse button mode */
					logerror("IKBD: Mouse keycode button mode (%d) (default)\n",input_buffer[0]);
					break;
				case 0x09: /* Set mouse max */
					mouse_max[0]=(input_buffer[0]<<8)|input_buffer[1];
					mouse_max[1]=(input_buffer[2]<<8)|input_buffer[3];
					logerror("IKBD: Mouse maximum set to %d %d\n",mouse_max[0],mouse_max[1]);
					break;
				case 0x0a: /* Set mouse keycode mode */
					logerror("IKBD: Mouse keycode mode set (%d %d) (unimplemented)\n",input_buffer[0],input_buffer[1]);
					break;
				case 0x0b: /* Set mouse threshold */
					logerror("IKBD: Mouse threshold set to %d %d (unimplemented)\n",input_buffer[0],input_buffer[1]);
					break;
				case 0x0e: /* Set mouse position */
					mouse_position[0]=(input_buffer[1]<<8)|input_buffer[2];
					mouse_position[1]=(input_buffer[3]<<8)|input_buffer[4];
					logerror("IKBD: Mouse position set to %d %d\n",mouse_position[0],mouse_position[1]);
					break;
				case 0x80:
					logerror("IKBD: Reset\n");
					ikbd_reset();
					break;
				default:
					logerror("IKBD: Unknown multi-byte command %02x\n",current_command);
			}
		}
		return;
	}

	/* Single byte commands, and header bytes */
	switch (data) {
		case 0x07: /* Set mouse button mode - requires 1 byte */
			current_command=data;
			needed=1;
			break;
		case 0x08: /* Set mouse relative position reporting */
			acia_mouse_event_mode=1;
			break;
		case 0x09: /* Set mouse absolute position - requires 4 bytes */
			current_command=data;
			acia_mouse_event_mode=0;
			needed=4;
			break;
		case 0x0a: /* Set mouse keycode mode - requires 2 bytes */
			current_command=data;
			needed=2;
			break;
		case 0x0b: /* Set mouse threshold - requires 2 bytes */
			current_command=data;
			needed=2;
			break;
		case 0x0d: /* Interrogate mouse position - returns 5 bytes */
			acia_buffer[(acia_bufpos++)&0x3f]=0xf7;
			mouse_fire=readinputport(3);
			if ((mouse_fire&1) && !(last_mouse_fire&1)) mouse_return|=1;
			if (!(mouse_fire&1) && (last_mouse_fire&1)) mouse_return|=2;
			if ((mouse_fire&2) && !(last_mouse_fire&2)) mouse_return|=4;
			if (!(mouse_fire&2) && (last_mouse_fire&2)) mouse_return|=8;
			mouse_position[0]+=new_frame[0]-last_frame[0];
			mouse_position[1]+=new_frame[1]-last_frame[1];
			if (mouse_position[0]<0) mouse_position[0]=0;
			if (mouse_position[1]<0) mouse_position[1]=0;
			if (mouse_position[0]>mouse_max[0]) mouse_position[0]=mouse_max[0];
			if (mouse_position[1]>mouse_max[1]) mouse_position[1]=mouse_max[1];
			acia_buffer[(acia_bufpos++)&0x3f]=mouse_return;
			acia_buffer[(acia_bufpos++)&0x3f]=mouse_position[0]>>8;
			acia_buffer[(acia_bufpos++)&0x3f]=mouse_position[0]&0xff;
			acia_buffer[(acia_bufpos++)&0x3f]=mouse_position[1]>>8;
			acia_buffer[(acia_bufpos++)&0x3f]=mouse_position[1]&0xff;
			last_mouse_fire=mouse_fire;
			acia_bufptr+=6;
			break;
		case 0x0e: /* Load mouse position - requires 5 bytes */
			current_command=data;
			needed=5;
			break;
		case 0x0f:
			logerror("IKBD: Mouse set y = 0 as bottom (unsupported)\n");
			break;
		case 0x10:
			logerror("IKBD: Mouse set y = 0 as top (default)\n");
			break;
		case 0x11:
			logerror("IKBD: Resume (unsupported)\n");
			break;
		case 0x12:
			logerror("IKBD: Mouse disabled (unsupported)\n");
			break;
		case 0x13:
			logerror("IKBD: Output paused (unsupported)\n");
			break;
		case 0x14:
			acia_joystick_event_mode=1;
			logerror("IKBD: Joystick event mode enabled\n");
			break;
		case 0x15:
			acia_joystick_event_mode=0;
			logerror("IKBD: Joystick interrogation mode enabled\n");
			break;
		case 0x16: /* Joystick interrogation */
			acia_buffer[(acia_bufpos++)&0x3f]=0xfd;
			acia_buffer[(acia_bufpos++)&0x3f]=(readinputport(4)>>7) | (readinputport(5)>>6);
			acia_buffer[(acia_bufpos++)&0x3f]=(readinputport(4)&0xf) | ((readinputport(5)<<4)&0xf0);
			acia_bufptr+=3;
			break;
		case 0x80: /* Reset */
			current_command=data;
			needed=1;
			break;
		default:
			logerror("IKBD: Unknown command %02x\n",data);
	}

	last_frame[0]=new_frame[0];
	last_frame[1]=new_frame[1];
}

static READ16_HANDLER( atarist_acia_r )
{
	int a;

//	logerror("%06x: ikbd read %02x\n", activecpu_get_pc(),offset);

	switch (offset) {
	case 0: return (acia_status)<<8;
	case 2: /* ACIA data */
		if (acia_bufptr==0) return acia_buffer[(acia_bufpos-1)&0x3f]<<8;
		a=acia_buffer[(acia_bufpos-acia_bufptr)&0x3f];
		acia_bufptr-=1;
		acia_bufpos&=0xff;
//		if (acia_bufptr<0) acia_bufptr=0;
		if (acia_bufptr==0) { acia_status=0xe; mfp.genr=0x10; }
		return a<<8;
	case 4: return (0xe)<<8;
	case 6: return 0;

	}
	return 0;
}

static WRITE16_HANDLER( atarist_acia_w )
{
logerror("%06x: ikbd write %02x %04x\n", activecpu_get_pc(),offset,data);

	if (offset==2) atarist_fake_ikbd_w((data>>8)&0xff);
}

static void atarist_keyboard_update(void)
{
	static int last_frame[16];
	int new_frame[16],diff[16],key_ptr=1,i,j;

	atarist_options=readinputport(0);
	for (i=1; i<14; i++) {
		new_frame[i]=readinputport(i);
		if (new_frame[i]!=last_frame[i])
			diff[i]=1; else diff[i]=0;
	}

	/* Port 0 : Mouse or joystick */
	if (atarist_options&0x10) {
		if (acia_joystick_event_mode && diff[5]) {
			acia_buffer[(acia_bufpos++)&0x3f]=0xfe;
			acia_buffer[(acia_bufpos++)&0x3f]=new_frame[5];
			acia_bufptr+=2;
		}
	} else { /* Return values only if event mode is turned on */
		if (acia_mouse_event_mode && (diff[1] || diff[2] || diff[3])) {
			acia_buffer[(acia_bufpos++)&0x3f]=0xf8 | new_frame[3];
			acia_buffer[(acia_bufpos++)&0x3f]=(new_frame[1]-last_frame[1])&0xff;
			acia_buffer[(acia_bufpos++)&0x3f]=(new_frame[2]-last_frame[2])&0xff;
			acia_bufptr+=3;
		}
	}

	/* Port 1 : Joystick only */
	if (acia_joystick_event_mode && diff[4] && atarist_options&0x20) {
		acia_buffer[(acia_bufpos++)&0x3f]=0xff;
		acia_buffer[(acia_bufpos++)&0x3f]=new_frame[4];
		acia_bufptr+=2;
	}

	/* Keyboard inputs */
	for (j=6; j<14; j++) { /* Walk over the input structs */
		for (i=0; i<16; i++) { /* Loop over 16 bit inputs */
			int a=(new_frame[j]>>i)&1;
			int b=(last_frame[j]>>i)&1;

			if (a==1 && b==0) { /* Key pressed */
				acia_buffer[(acia_bufpos++)&0x3f]=key_ptr;
				acia_bufptr++;
			}
			if (a==0 && b==1) { /* Key released */
				acia_buffer[(acia_bufpos++)&0x3f]=key_ptr | 0x80;
				acia_bufptr++;
			}

			key_ptr++; /* Step through scancodes */
		}
	}

	/* Check for any new ikbd activity this frame */
	if (acia_bufptr) {
		acia_status|=0x81;
		mfp.genr=0x00;
		if (mfp.ieb&0x40) { /* Channel enabled */
			mfp.ipb|=0x40; /* Set pending interrupt bit */
			if (mfp.imb&0x40) /* Interrupt enabled */
				atarist_mfp_interrupt(6);
		}
	}

	for (i=1; i<14; i++)
		last_frame[i]=new_frame[i];
}

/************************************************************************************/

/*void dma_transfer(unsigned char *mem, int length); */

struct FDC {
	UINT8 dma_status;
	UINT8 reg_select;
	UINT8 dma_sector_count;
	UINT8 dma_select;
	UINT8 dma_direction;
	int dma_base;
	int dma_bytes_remaining;
} fdc;

void atarist_dma_transfer(void)
{
	unsigned char *addr;

	addr = memory_region(REGION_CPU1) + fdc.dma_base;

	if (fdc.dma_direction)
	{
		/* write the data */
		wd17xx_data_w(0,addr[0]);
	}
	else
	{
		/* read the data */
		addr[0] = wd17xx_data_r(0);

	}

	fdc.dma_base++;
	fdc.dma_bytes_remaining--;

	/* if no more bytes are remaining, set int */
	if (fdc.dma_bytes_remaining<=0)
	{
		logerror("Done DMA to %06x at %06x (512*%d)\n",fdc.dma_base,activecpu_get_pc(),fdc.dma_sector_count);

		mfp.ipb=mfp.ipb|0x80; /* Set pending interrupt bit */
		if (mfp.ieb&0x80) /* Interrupt enabled */
			atarist_mfp_interrupt(7);
	}
}


void atarist_fdc_callback(int event, void *param)
{
	switch (event)
	{
		case WD17XX_IRQ_CLR:
		case WD17XX_DRQ_CLR:
			break;
		case WD17XX_IRQ_SET:
			logerror("WD1792:  Warning: IRQ set!\n");
			break;
		case WD17XX_DRQ_SET:
			if (fdc.dma_select==0) { /* If DMA is enabled */
				atarist_dma_transfer();
			}

			break;
	}
}

static READ16_HANDLER( atarist_fdc_r )
{
	switch (offset) {
		case 0: /* Reserved (unused) */
		case 2:
			break;
		case 4: /* Data register */
			wd1772_active=0;//todo?
			switch (fdc.reg_select&0xf) {
				/* A0/A1 pins on wd179x controller */
				case 0: fdc.dma_status=1; return wd17xx_status_r(0);
				case 1: fdc.dma_status=1; return wd17xx_track_r(0);
				case 2: fdc.dma_status=1; return wd17xx_sector_r(0);
				case 3: fdc.dma_status=1; return wd17xx_data_r(0);

				/* HDC register select - Unimplemented */
				case 4: case 5: case 6: case 7:
					fdc.dma_status=0; /* Force DMA error on hard-disk accesses */
					break;

				/* DMA sector count register */
				default: /* cases 8-15 */
					logerror("Sector count register read %02x\n",fdc.dma_sector_count);
					fdc.dma_status=1;
					return fdc.dma_sector_count;
			}
			break;
		case 6: /* Status register */
			//logerror("%06x:  DMA fdc read (%02x)\n",activecpu_get_pc(),0xfe | fdc.dma_status);
			return 0xfe | fdc.dma_status;
	}

	return 0xff;
}

static WRITE16_HANDLER( atarist_fdc_w )
{
	switch (offset) {
		case 0: /* Unused */
		case 2:
			break;
		case 4: /* Data register */
			switch (fdc.reg_select&0xf) {
				/* A0/A1 pins on wd179x controller */
				case 0: wd17xx_command_w(0, data&0xff); break;
				case 1: wd17xx_track_w(0, data&0xff); break;
				case 2: wd17xx_sector_w(0, data&0xff); break;
				case 3: wd17xx_data_w(0, data&0xff); break;

				/* HDC register select - Unimplemented */
				case 4: case 5: case 6: case 7:
					break;

				/* DMA sector count register */
				default: /* cases 8-15 */
					if ((data&0xff)!=1) logerror("Sector count register write %02x\n",data);
					fdc.dma_sector_count=data&0xff;
					break;
			}
			break;
		case 6: /* Select/Status register */
			fdc.reg_select=(data>>1)&0xf;
			fdc.dma_select=data&0x40;
			fdc.dma_direction=data&0x100;
			break;
		case 8:
			fdc.dma_base=(fdc.dma_base&0x00ffff) | ((data&0xff)<<16);
			/* KT - added */
			fdc.dma_bytes_remaining = 512;
			break;
		case 10:
			fdc.dma_base=(fdc.dma_base&0xff00ff) | ((data&0xff)<<8);
			/* KT - added */
			fdc.dma_bytes_remaining = 512;
			break;
		case 12:
			fdc.dma_base=(fdc.dma_base&0xffff00) | (data&0xff);

			/* KT - added */
			fdc.dma_bytes_remaining = 512;
			break;
	}
}

/************************************************************************************/

/* Communication registers with the 68000 code */
#define	HDC_COMMAND		0
#define	HDC_TRAP		2
#define	HDC_RETURN		4
#define HDC_STATUS		8
#define	HDC_PARAM0		10
#define	HDC_PARAM1		12
#define	HDC_PARAM2		14
#define	HDC_PARAM3		16
#define	HDC_PARAM4		18
#define	HDC_PARAM5		20
#define	HDC_PARAM6		22

/* commented out because I commented some other hdc code */
#if 0
/* Grabs the string pointed in ST memory by the parameter registers */
static char *get_hdc_string(int param)
{
	static char filebuf[256];
	unsigned char *ram_ptr=atarist_ram;
	int i,addr;

	addr=((READ_WORD(&atarist_fakehdc_ram[param])<<16)|READ_WORD(&atarist_fakehdc_ram[param+2]))&0xfffffe;
	if (addr>=0xfc0000) { ram_ptr=memory_region(REGION_USER1); addr-=0xfc0000; }

	/* Check for odd source addresses */
	if (READ_WORD(&atarist_fakehdc_ram[param+2])&1) {
		filebuf[0]=READ_WORD(&ram_ptr[addr&0x3fffff])&0xff;
		for (i=0; i<253; i+=2) {
			filebuf[i+1]=READ_WORD(&ram_ptr[(addr+i+2)&0x3fffff])>>8;
			filebuf[i+2]=READ_WORD(&ram_ptr[(addr+i+2)&0x3fffff])&0xff;
			if (!filebuf[i] || !filebuf[i+1])
				break;
		}
	} else {
		for (i=0; i<254; i+=2) {
			filebuf[i]=READ_WORD(&ram_ptr[(addr+i)&0x3fffff])>>8;
			filebuf[i+1]=READ_WORD(&ram_ptr[(addr+i)&0x3fffff])&0xff;
			if (!filebuf[i] || !filebuf[i+1])
				break;
		}
	}

	return filebuf;
}
#endif

static READ16_HANDLER(atarist_fakehdc_r)
{
	return READ_WORD(&atarist_fakehdc_ram[offset]);
}

static WRITE16_HANDLER(atarist_fakehdc_w)
{
// KT - commented this lot out because loads of osd functions could not be found
#if 0
	static int dta_high,dta_low,ok_to_go=0;
	static char *str_ptr;
	static mame_file *file0;

	COMBINE_WORD_MEM(&atarist_fakehdc_ram[offset],data);
//	logerror("%06x: Write mish_fs (%d) %04x\n",activecpu_get_pc(),offset,data);
	if (offset==HDC_COMMAND) {
		switch (data&0xff) {
			case 3: /* Return BPB */
				memset(atarist_fakehdc_ram,0,512);
				WRITE_WORD(&atarist_fakehdc_ram[0x10c],0x0202);
				WRITE_WORD(&atarist_fakehdc_ram[0x10e],0x1000);
				WRITE_WORD(&atarist_fakehdc_ram[0x110],0x0270);
				WRITE_WORD(&atarist_fakehdc_ram[0x112],0x0068);
				WRITE_WORD(&atarist_fakehdc_ram[0x114],0x06f9);
				WRITE_WORD(&atarist_fakehdc_ram[0x116],0x0500);
				WRITE_WORD(&atarist_fakehdc_ram[0x118],0x0a00);
				WRITE_WORD(&atarist_fakehdc_ram[0x11a],0x0100);
				break;
		}
	}

	if (offset==HDC_TRAP) {	/* A trap #1 function call, any unused commands default to TOS */
		WRITE_WORD(&atarist_fakehdc_ram[HDC_STATUS],0);
		switch (data&0xff) {
			case 0x0e: /* Set default drive, we want to hook all Drive C commands */
				atarist_current_drive=READ_WORD(&atarist_fakehdc_ram[HDC_PARAM0]);
				logerror("Default drive is %04x\n",atarist_current_drive);
				if (atarist_current_drive==0x0002)
					ok_to_go=1; /* Drive C selected - we are ok to process commands */
				else
					ok_to_go=0; /* Not Drive C - we want to ignore any commands until Drive C is selected again */
				break;

			case 0x4e: /* SFirst - Find files from a given filemask */
				str_ptr=get_hdc_string(HDC_PARAM0);
				logerror("Filemask %s requested\n",str_ptr);

				/* Always allow drive C queries */
				if ((str_ptr[0]=='C' || str_ptr[0]=='c')
				&& (str_ptr[1]==':' && str_ptr[2]=='\\')) {
				 	ok_to_go=1;
				 	str_ptr+=3;
				}

				/* Strip leading slashes */
				if (str_ptr[0]=='\\' && ok_to_go) {
					str_ptr++;
				}

				if (ok_to_go) {
					if (osd_findfirst(str_ptr,0)==0) {
						char *ptr=osd_getfindfilename();
						int s=osd_getfindsize();
						int d=osd_getfinddate();
						int t=osd_getfindtime();
						int a=osd_getfindattributes();

						if (ptr) {
							WRITE_WORD(&atarist_ram[(dta_low|(dta_high<<16))+30],((ptr[0]<<8)|ptr[1])); /* Filename */
							WRITE_WORD(&atarist_ram[(dta_low|(dta_high<<16))+32],((ptr[2]<<8)|ptr[3])); /* Filename */
							WRITE_WORD(&atarist_ram[(dta_low|(dta_high<<16))+34],((ptr[4]<<8)|ptr[5])); /* Filename */
							WRITE_WORD(&atarist_ram[(dta_low|(dta_high<<16))+36],((ptr[6]<<8)|ptr[7])); /* Filename */
							WRITE_WORD(&atarist_ram[(dta_low|(dta_high<<16))+38],((ptr[8]<<8)|ptr[9])); /* Filename */
							WRITE_WORD(&atarist_ram[(dta_low|(dta_high<<16))+40],((ptr[10]<<8)|ptr[11])); /* Filename */
						}
						WRITE_WORD(&atarist_ram[(dta_low|(dta_high<<16))+20],  a&0xff); /* Attributes */
						WRITE_WORD(&atarist_ram[(dta_low|(dta_high<<16))+22],t&0xffff); /* Time */
						WRITE_WORD(&atarist_ram[(dta_low|(dta_high<<16))+24],d&0xffff); /* Date */
						WRITE_WORD(&atarist_ram[(dta_low|(dta_high<<16))+26],   s>>16); /* Filesize */
						WRITE_WORD(&atarist_ram[(dta_low|(dta_high<<16))+28],s&0xffff); /* Filesize */

						WRITE_WORD(&atarist_fakehdc_ram[HDC_RETURN],0); /* F_SNEXT is ok to run */
						WRITE_WORD(&atarist_fakehdc_ram[HDC_RETURN+2],0);

						logerror("Found mask %s ok. (cpu %06x)\n",str_ptr,activecpu_get_pc());

						ok_to_go=1;
						return;
					}
					else {
						logerror("Found mask %s failed.\n",str_ptr);
						WRITE_WORD(&atarist_fakehdc_ram[HDC_RETURN],0);
						WRITE_WORD(&atarist_fakehdc_ram[HDC_RETURN+2],1); /* Flag F_SNEXT to fail also */
					}
				}
				break;

			case 0x4f: /* F_SNEXT - Find next file after a Ffirst */
				if (ok_to_go) {
					if (osd_findnext()==0) {
						char *ptr=osd_getfindfilename();
						int s=osd_getfindsize();
						int d=osd_getfinddate();
						int t=osd_getfindtime();
						int a=osd_getfindattributes();

						if (ptr) {
							WRITE_WORD(&atarist_ram[(dta_low|(dta_high<<16))+30],((ptr[0]<<8)|ptr[1])); /* Filename */
							WRITE_WORD(&atarist_ram[(dta_low|(dta_high<<16))+32],((ptr[2]<<8)|ptr[3])); /* Filename */
							WRITE_WORD(&atarist_ram[(dta_low|(dta_high<<16))+34],((ptr[4]<<8)|ptr[5])); /* Filename */
							WRITE_WORD(&atarist_ram[(dta_low|(dta_high<<16))+36],((ptr[6]<<8)|ptr[7])); /* Filename */
							WRITE_WORD(&atarist_ram[(dta_low|(dta_high<<16))+38],((ptr[8]<<8)|ptr[9])); /* Filename */
							WRITE_WORD(&atarist_ram[(dta_low|(dta_high<<16))+40],((ptr[10]<<8)|ptr[11])); /* Filename */
						}
						WRITE_WORD(&atarist_ram[(dta_low|(dta_high<<16))+20],  a&0xff); /* Attributes */
						WRITE_WORD(&atarist_ram[(dta_low|(dta_high<<16))+22],t&0xffff); /* Time */
						WRITE_WORD(&atarist_ram[(dta_low|(dta_high<<16))+24],d&0xffff); /* Date */
						WRITE_WORD(&atarist_ram[(dta_low|(dta_high<<16))+26],   s>>16); /* Filesize */
						WRITE_WORD(&atarist_ram[(dta_low|(dta_high<<16))+28],s&0xffff); /* Filesize */

						WRITE_WORD(&atarist_fakehdc_ram[HDC_RETURN],0); /* F_SNEXT is ok to run */
						WRITE_WORD(&atarist_fakehdc_ram[HDC_RETURN+2],0);
					} else {
						WRITE_WORD(&atarist_fakehdc_ram[HDC_RETURN],0);
						WRITE_WORD(&atarist_fakehdc_ram[HDC_RETURN+2],1);
 					}
 					return;
				}
				break;

			case 0x1a: /* Set Disk transfer address */
				dta_high=READ_WORD(&atarist_fakehdc_ram[HDC_PARAM0]);
				dta_low =READ_WORD(&atarist_fakehdc_ram[HDC_PARAM1]);
				break;

			case 0x36: /* Free disk space */
				logerror("Free disk space\n");
				break;

			case 0x39: /* Create Directory */
				logerror("Create Directory\n");
				break;

			case 0x3a: /* Delete Directory */
				logerror("Delete Directory\n");
				break;

			case 0x3b: /* Set current directory */
/* defined but not used */
/*				str_ptr=get_hdc_string(HDC_PARAM0); */
				logerror("Change directory to %s requested\n",str_ptr);
				break;

			case 0x3d: /* Open file */
				str_ptr=get_hdc_string(HDC_PARAM0);
				logerror("Asked to open %s (mode %04x)\n",str_ptr,READ_WORD(&atarist_fakehdc_ram[HDC_PARAM2]));
				if (ok_to_go) {
					file0=mame_fopen_native(str_ptr,0);
					if (file0) {
						logerror("Opened OK\n");
						WRITE_WORD(&atarist_fakehdc_ram[HDC_RETURN],0);
						WRITE_WORD(&atarist_fakehdc_ram[HDC_RETURN+2],1); /* File handle 1 */
					}
					else {
						logerror("Opened failed\n");
						WRITE_WORD(&atarist_fakehdc_ram[HDC_RETURN],0xffff); /* Error (Negative) */
						WRITE_WORD(&atarist_fakehdc_ram[HDC_RETURN+2],0xffff);
					}
					return;
				} else
					logerror("Open ignored - not meant for C drive\n");
				break;

			case 0x3f: /* Read file */
				if (ok_to_go) {
					if (file0) {
						int bufsize=(READ_WORD(&atarist_fakehdc_ram[HDC_PARAM1])<<16)|READ_WORD(&atarist_fakehdc_ram[HDC_PARAM2]);
						unsigned char *buf=malloc(16384),*buf2;
						int count=0,i,len,addr,todo=bufsize;

						if (!buf) logerror("Malloc for %d bytes failed!!!!\n",bufsize);
						if (!buf) break;

						addr=((READ_WORD(&atarist_fakehdc_ram[HDC_PARAM3])<<16)|READ_WORD(&atarist_fakehdc_ram[HDC_PARAM4]))&0xffffff;
						buf2=atarist_ram+(addr&0x3fffff);

						if (addr&1) logerror("WARNING - LOAD TO ODD ADDRESS!!!\n");
						if (bufsize&1) logerror("WARNING - LOAD WITH ODD SIZE!!!\n");

						do {
							if (todo<16384)
								len=mame_fread(file0,buf,todo);
							else
								len=mame_fread(file0,buf,16384);

							for (i=0; i<len; i+=2)
								WRITE_WORD(&buf2[i+count],(buf[i]<<8)|buf[i+1]);
				//TODO!! Odd address loading..
							count+=len;

						} while (count<bufsize && len);

						logerror("Read file (handle %04x) to addr %06x, size %d bytes, read %d ok\n",READ_WORD(&atarist_fakehdc_ram[HDC_PARAM0]),addr,bufsize,count);

						WRITE_WORD(&atarist_fakehdc_ram[HDC_RETURN],count>>16); /* Number of bytes read */
						WRITE_WORD(&atarist_fakehdc_ram[HDC_RETURN+2],count&0xffff);
						free(buf);
					} else {
						WRITE_WORD(&atarist_fakehdc_ram[HDC_RETURN],0xffff); /* Error (Negative) */
						WRITE_WORD(&atarist_fakehdc_ram[HDC_RETURN+2],0xffff);
					}
					return;
				}
				break;

			case 0x3e: /* Close file */
				logerror("Close file (handle %04x)\n",READ_WORD(&atarist_fakehdc_ram[HDC_PARAM0]));
				if (ok_to_go) {
					if (file0) {
						WRITE_WORD(&atarist_fakehdc_ram[HDC_RETURN],0);
						WRITE_WORD(&atarist_fakehdc_ram[HDC_RETURN+2],0);
						mame_fclose(file0);
					} else {
						WRITE_WORD(&atarist_fakehdc_ram[HDC_RETURN],0xffff); /* Error (Negative) */
						WRITE_WORD(&atarist_fakehdc_ram[HDC_RETURN+2],0xffff);
					}
					return;
				}
				break;

			case 0x40: /* Write file */
				logerror("Write file\n");
				break;

			case 0x42: /* Seek file position */
				logerror("Seek file\n");
				break;

			case 0x4b: /* Load/Execute a process */
				str_ptr=get_hdc_string(HDC_PARAM1);
				if (*str_ptr) {
					logerror("Asked to exec %s\n",str_ptr);
					if (ok_to_go) {
						logerror("Executing\n");
						/* Execute needs extra code on the 68000 side - signal it */
						WRITE_WORD(&atarist_fakehdc_ram[HDC_TRAP],0xffff);
						ok_to_go=0;
						if (file0) /* TODO */
							mame_fclose(file0);
						file0=mame_fopen_native(str_ptr,0);
						return;
					}
				}
				break;

			case 0xff: /* Execute a process (part 2) */
				ok_to_go=1;

//check stack on $4b

				if (file0) {
					unsigned char *buf=malloc(65536),*buf2;
					int count=0,i,len,addr;

					addr=((READ_WORD(&atarist_fakehdc_ram[HDC_PARAM0])<<16)|READ_WORD(&atarist_fakehdc_ram[HDC_PARAM1]))&0xffffff;
					logerror("Exec address is %06x\n",addr);

					/* Load the header */
					len=mame_fread(file0,buf,0x1c);
					buf2=atarist_ram+(addr&0x3fffff);
					for (i=0; i<len; i+=2)
						WRITE_WORD(&buf2[i+228],(buf[i]<<8)|buf[i+1]);

					/* Load the main program */
					do {
						len=mame_fread(file0,buf,65536);
						for (i=0; i<len; i+=2)
							WRITE_WORD(&buf2[i+256+count],(buf[i]<<8)|buf[i+1]);
						count+=len;
					} while (len);

					free(buf);
				}
				mame_fclose(file0);

				return;
			}

		/* If we didn't handle this Trap command, let TOS handle it by flagging the status */
		WRITE_WORD(&atarist_fakehdc_ram[HDC_STATUS],1);
	}
#endif
}

/************************************************************************************/

struct BLITTER {
	UINT8 hop;
	UINT8 op;
	UINT8 skew;
	UINT8 line;
	UINT8 nfsr, fxsr;
} blitter;

#define BLITTER_DO_SOURCE \
		if (src_x_inc<0) { \
			source_buffer >>=16;\
			if (blitter.hop&2) { \
				/*if (source_addr&1)*/ /* Odd address */ \
				/*	source_buffer |= ( ( (unsigned int)( READ_WORD(&atarist_ram[source_addr-1]) << 24) )&0xff000000)| \
									 ( ( (unsigned int)( READ_WORD(&atarist_ram[source_addr+1]) <<  8) )&0x00ff0000); \
				else */	\
					source_buffer |= (unsigned int)( READ_WORD(&atarist_ram[source_addr]) << 16);		 	\
			} \
		} else {\
			source_buffer <<=16;\
			if (blitter.hop&2) { \
			/*	if (source_addr&1)*/ /* Odd address */ \
			/*		source_buffer |= (((unsigned int)( READ_WORD(&atarist_ram[source_addr-1]) <<  8))&0xff00)| \
									 (((unsigned int)( READ_WORD(&atarist_ram[source_addr+1]) >>  8))&0x00ff); \
			else*/ \
				source_buffer |= READ_WORD(&atarist_ram[source_addr]);\
			} \
		}

#define BLITTER_DO_HOP \
		switch (blitter.hop) { \
			case 0: src_data=0xffff; break; \
			case 1: src_data=READ_WORD(&atarist_blitter_ram[halftone_offset]); break; \
			case 2: src_data=(source_buffer>>blitter.skew); break; \
			case 3: src_data=(source_buffer>>blitter.skew)&READ_WORD(&atarist_blitter_ram[halftone_offset]); break; \
		}

static void blitter_op(int op, int dest_addr, int src_data, int dst_data, int mask)
{
	src_data&=0xffff;
	switch (op) {
		case 0:	 WRITE_WORD(&atarist_ram[dest_addr], (dst_data & ~mask)|((               0x0000)&mask)); break;
		case 1:  WRITE_WORD(&atarist_ram[dest_addr], (dst_data & ~mask)|(( src_data &  dst_data)&mask)); break;
		case 2:	 WRITE_WORD(&atarist_ram[dest_addr], (dst_data & ~mask)|(( src_data & ~dst_data)&mask)); break;
		case 3:	 WRITE_WORD(&atarist_ram[dest_addr], (dst_data & ~mask)|(( src_data            )&mask)); break;
		case 4:	 WRITE_WORD(&atarist_ram[dest_addr], (dst_data & ~mask)|((~src_data &  dst_data)&mask)); break;
		case 5:	 WRITE_WORD(&atarist_ram[dest_addr], (dst_data & ~mask)|((             dst_data)&mask)); break;
		case 6:	 WRITE_WORD(&atarist_ram[dest_addr], (dst_data & ~mask)|(( src_data ^  dst_data)&mask)); break;
		case 7:	 WRITE_WORD(&atarist_ram[dest_addr], (dst_data & ~mask)|(( src_data |  dst_data)&mask)); break;
		case 8:	 WRITE_WORD(&atarist_ram[dest_addr], (dst_data & ~mask)|((~src_data & ~dst_data)&mask)); break;
		case 9:	 WRITE_WORD(&atarist_ram[dest_addr], (dst_data & ~mask)|((~src_data ^  dst_data)&mask)); break;
		case 10: WRITE_WORD(&atarist_ram[dest_addr], (dst_data & ~mask)|((            ~dst_data)&mask)); break;
		case 11: WRITE_WORD(&atarist_ram[dest_addr], (dst_data & ~mask)|(( src_data | ~dst_data)&mask)); break;
		case 12: WRITE_WORD(&atarist_ram[dest_addr], (dst_data & ~mask)|((~src_data            )&mask)); break;
		case 13: WRITE_WORD(&atarist_ram[dest_addr], (dst_data & ~mask)|((~src_data |  dst_data)&mask)); break;
		case 14: WRITE_WORD(&atarist_ram[dest_addr], (dst_data & ~mask)|((~src_data | ~dst_data)&mask)); break;
		case 15: WRITE_WORD(&atarist_ram[dest_addr], (dst_data & ~mask)|((               0xffff)&mask)); break;
	}
}

static void do_blit(void)
{
	int source_addr=((READ_WORD(&atarist_blitter_ram[0x24])<<16) | READ_WORD(&atarist_blitter_ram[0x26]))&0x3ffffe;
	int dest_addr=((READ_WORD(&atarist_blitter_ram[0x32])<<16) | READ_WORD(&atarist_blitter_ram[0x34]))&0x3ffffe;
	int x_count=READ_WORD(&atarist_blitter_ram[0x36]);
	int y_count=READ_WORD(&atarist_blitter_ram[0x38]);
	int end_mask1=READ_WORD(&atarist_blitter_ram[0x28]);
	int end_mask2=READ_WORD(&atarist_blitter_ram[0x2a]);
	int end_mask3=READ_WORD(&atarist_blitter_ram[0x2c]);
	int src_x_inc=READ_WORD(&atarist_blitter_ram[0x20])&0xfffe;
	int src_y_inc=READ_WORD(&atarist_blitter_ram[0x22])&0xfffe;
	int dst_x_inc=READ_WORD(&atarist_blitter_ram[0x2e])&0xfffe;
	int dst_y_inc=READ_WORD(&atarist_blitter_ram[0x30])&0xfffe;
	int dst_data,src_data=0,x,halftone_offset=0,halftone_inc;
	unsigned int source_buffer=0; /* change */
logerror("Doing blit\n");
	if (dst_y_inc&0x8000) dst_y_inc=dst_y_inc-0x10000;
	if (dst_x_inc&0x8000) dst_x_inc=dst_x_inc-0x10000;
	if (src_y_inc&0x8000) src_y_inc=src_y_inc-0x10000;
	if (src_x_inc&0x8000) src_x_inc=src_x_inc-0x10000;

	if (dst_y_inc<0) halftone_inc=-2; else halftone_inc=2;
	if (READ_WORD(&atarist_blitter_ram[0x3c])&0x2000) halftone_offset=blitter.skew<<1; else halftone_offset=((READ_WORD(&atarist_blitter_ram[0x3c])>>8)&0xf)<<1;
	blitter.nfsr=READ_WORD(&atarist_blitter_ram[0x3c])&0x40;
	blitter.fxsr=READ_WORD(&atarist_blitter_ram[0x3c])&0x80;
	blitter.hop=(READ_WORD(&atarist_blitter_ram[0x3a])>>8)&3;
	blitter.op=READ_WORD(&atarist_blitter_ram[0x3a])&0xf;
	blitter.skew=READ_WORD(&atarist_blitter_ram[0x3c])&0xf;

	/* Each blitter line is in 3 parts, according to the 3 masks (start, middle, end) */
	while (y_count>0) {
		if (blitter.fxsr) {
			BLITTER_DO_SOURCE;
			source_addr+=src_x_inc;
		}

		/* Start */
		BLITTER_DO_SOURCE;
		BLITTER_DO_HOP;
		dst_data=READ_WORD(&atarist_ram[dest_addr]);
		blitter_op(blitter.op,dest_addr,src_data,dst_data,end_mask1);

		/* Middle */
		for(x=0 ; x<x_count-2; x++) {
			source_addr+=src_x_inc;
			dest_addr+=dst_x_inc;

			BLITTER_DO_SOURCE;
			BLITTER_DO_HOP;
			dst_data=READ_WORD(&atarist_ram[dest_addr]);
			blitter_op(blitter.op,dest_addr,src_data,dst_data,end_mask2);
		}

		/* End */
		if (x_count>2) {
			dest_addr+=dst_x_inc;
//			BLITTER_DO_SOURCE;
			if ((!blitter.nfsr) || ((~(0xffff>>blitter.skew))>end_mask3)) {
				source_addr += src_x_inc;
				BLITTER_DO_SOURCE;
			} else if (src_x_inc<0) source_buffer >>=16; else source_buffer <<=16;

			BLITTER_DO_HOP;
			dst_data=READ_WORD(&atarist_ram[dest_addr]);
			blitter_op(blitter.op,dest_addr,src_data,dst_data,end_mask3);
		}

	//	logerror("Blitted line to %08x dest (x incs is %d, x incd is %d),(y incs is %d, y incd is %d)\n",dest_addr,src_x_inc,dst_x_inc,src_y_inc,dst_y_inc);

		source_addr+=src_y_inc;
		dest_addr+=dst_y_inc;
		if (blitter.hop&1) halftone_offset=(halftone_offset+halftone_inc)&0x1e;
		y_count--;
	}
	WRITE_WORD(&atarist_blitter_ram[0x24],source_addr>>16);
	WRITE_WORD(&atarist_blitter_ram[0x26],source_addr&0xffff);
	WRITE_WORD(&atarist_blitter_ram[0x32],dest_addr>>16);
	WRITE_WORD(&atarist_blitter_ram[0x34],dest_addr&0xffff);
	WRITE_WORD(&atarist_blitter_ram[0x38],0); /* Y count */
}

static READ16_HANDLER( atarist_blitter_r )
{
//	logerror("%06x:  blitter read %02x\n",activecpu_get_pc(),offset);
	if (offset==0x3c) return READ_WORD(&atarist_blitter_ram[0x3c])&0x3fff;
	return READ_WORD(&atarist_blitter_ram[offset]);
}

static WRITE16_HANDLER( atarist_blitter_w )
{
if (offset==0x3c) logerror("%06x:  blitter write %02x %04x\n",activecpu_get_pc(),offset,data);
	COMBINE_WORD_MEM(&atarist_blitter_ram[offset],data);

//if (offset==0x20) logerror("%06x:  blitter write xinc %02x %04x\n",activecpu_get_pc(),offset,data);
//if (offset==0x22) logerror("%06x:  blitter write yinc %02x %04x\n",activecpu_get_pc(),offset,data);

	if (offset==0x3c) {
		if ((data&0xffff8000)==0x8000) do_blit();
	}
}

static READ16_HANDLER( atarist_shifter_r )
{
	/* Video ram base: $ffff8200 and $ffff8202 */
	if (offset==0x0)
		return (atarist_vidram_base>>16)&0xff;
	if (offset==0x2)
		return (atarist_vidram_base>>8)&0xff;

	if (offset==0x4) /* Current screen address being rendered by shifter (top byte) */
		return ((atarist_vidram_base+atarist_shifter_offset)>>16)&0xff;
	if (offset==0x6) { /* Current screen address being rendered by shifter (mid byte) */
		int c=512-cpu_geticount(); /* 512 cycles per line */

		/* Are in left border? If so we haven't yet started drawing this line */
		if (c<96) return ((atarist_vidram_base+atarist_shifter_offset)>>8)&0xff;

		/* Are we in right border?  If so, we have drawn 320 pixels (160 bytes) */
		if (c>415) return ((atarist_vidram_base+atarist_shifter_offset+160)>>8)&0xff;

		return ((atarist_vidram_base+atarist_shifter_offset+((c-96)/2))>>8)&0xff;
	}
	if (offset==0x8) { /* Current screen address being rendered by shifter (low byte) */
		int c=512-cpu_geticount(); /* 512 cycles per line */

	logerror("%06x: render read low byte\n",activecpu_get_pc());

		/* Are in left border? If so we haven't yet started drawing this line */
		if (c<96) return (atarist_vidram_base+atarist_shifter_offset)&0xfe;

		/* Are we in right border?  If so, we have drawn 320 pixels (160 bytes) */
		if (c>415) return (atarist_vidram_base+atarist_shifter_offset+160)&0xfe;

		/* Else mid-line, also I'm not sure if the real ST masks to 0xfe, 0xfc or 0xf8 */
		return ((atarist_vidram_base+atarist_shifter_offset+(c-96)/2))&0xfe;
	}

	if (offset==0xa)
		return shifter_sync;

	/* Palette data: $ffff8240 to $ffff825f*/
	if ((offset&0xe0)==0x40)
		return READ_WORD(&paletteram[offset&0x1e]);

	/* Shifter resolution */
	if (offset==0x60)
		return resolution<<8;


	logerror("%06x:  Unmapped video read %02x\n",activecpu_get_pc(),offset);


	return 0;
}

static WRITE16_HANDLER( atarist_shifter_w )
{
	if (offset==0) { /* Video ram base: $ffff8200 and $ffff8202 */
		logerror("Switched screen base at line %d\n",current_line);
		atarist_pixel_update(); /* Update line for all pixels already drawn */
		atarist_vidram_base=(atarist_vidram_base&0xff00)|((data&0xff)<<16);
		return;
	}
	if (offset==2) {
		atarist_pixel_update(); /* Update line for all pixels already drawn */
		atarist_vidram_base=(atarist_vidram_base&0xff0000)|((data&0xff)<<8);
		return;
	}

	if (offset==0xa) { /* Sync */
		char *t[2]={"60hz","50hz"};

		/* Check for change, toggle resets lines left to draw (lower border overscan) */
		if (shifter_sync!=(data&0x200)) {
			start_line=current_line;
			end_line=start_line+200;
			logerror("%06x: Sync changed to %s (line %d)\n",activecpu_get_pc(),t[(data>>9)&1],current_line);
		}

		shifter_sync=data&0x200;
		return;
	}

	/* Palette data: $ffff8240 to $ffff825f */
	if ((offset&0xe0)==0x40) {
		int d;

//		atarist_pixel_update(); /* Update line for all pixels already drawn */

		COMBINE_WORD_MEM(&paletteram[offset&0x1f],data);
		d=READ_WORD(&paletteram[offset&0x1f]);
		pal_lookup[(offset&0x1f)/2]=(d&7) | ((d&0x70)>>1) | ((d&0x700)>>2);
		return;
	}

	if (offset==0x60) {
static int a;

		resolution=(data>>8)&3;

//		if (a!=resolution)
//			osd_set_display(640,480,Machine->drv->video_attributes);

		a=resolution;

		return;
	}

	logerror("%06x:  Unmapped video write %02x %04x (line %d)\n",activecpu_get_pc(),offset,data,current_line);
}

static int mmu;

static READ16_HANDLER( atarist_mmu_r )
{
	return mmu;
}

static WRITE16_HANDLER( atarist_mmu_w )
{
	mmu=data&0xff;
}

/***************************************************************************/

static ADDRESS_MAP_START (atarist_readmem, ADDRESS_SPACE_PROGRAM, 16)
	AM_RANGE( 0x000000, 0x000007) AM_READ( MRA16_ROM )
	AM_RANGE( 0x000008, 0x3fffff) AM_READ( MRA16_RAM )	/* User RAM */
	AM_RANGE( 0xfa0000, 0xfbffff) AM_READ( MRA16_BANK1 )	/* User ROM */
	AM_RANGE( 0xfc0000, 0xfeffff) AM_READ( MRA16_BANK2 )	/* System ROM */
	AM_RANGE( 0xef0000, 0xef01ff) AM_READ( atarist_fakehdc_r )
	AM_RANGE( 0xff8000, 0xff8007) AM_READ( atarist_mmu_r )
	AM_RANGE( 0xff8200, 0xff82ff) AM_READ( atarist_shifter_r )
	AM_RANGE( 0xff8600, 0xff860f) AM_READ( atarist_fdc_r )
	AM_RANGE( 0xff8800, 0xff88ff) AM_READ( atarist_psg_r )
	AM_RANGE( 0xff8a00, 0xff8a3f) AM_READ( atarist_blitter_r )
	AM_RANGE( 0xfffa00, 0xfffa2f) AM_READ( atarist_mfp_r )
	AM_RANGE( 0xfffc00, 0xfffc07) AM_READ( atarist_acia_r )
ADDRESS_MAP_END

static WRITE16_HANDLER(log_mem)
{
	COMBINE_WORD_MEM(&atarist_ram[offset+0x122],data);
	logerror("%06x: Write LOGMEM %04x (line %d)\n",activecpu_get_pc(),data,current_line);
}

static ADDRESS_MAP_START (atarist_writemem, ADDRESS_SPACE_PROGRAM, 16)
	AM_RANGE( 0x000000, 0x000007) AM_WRITE( MWA16_ROM) AM_BASE( &atarist_ram )	/* Mirror of first 8 bytes of ROM */
//	{ 0x000122, 0x000123, log_mem },
	AM_RANGE( 0x000008, 0x3fffff) AM_WRITE( MWA16_RAM )	/* User RAM */
	AM_RANGE( 0xfa0000, 0xfbffff) AM_WRITE( MWA16_BANK1 )	/* User ROM */ //todo!
	AM_RANGE( 0xfc0000, 0xfeffff) AM_WRITE( MWA16_ROM )	/* System ROM */

	AM_RANGE( 0xef0000, 0xef01ff) AM_WRITE( atarist_fakehdc_w) AM_BASE( &atarist_fakehdc_ram )
	AM_RANGE( 0xff8000, 0xff8001) AM_WRITE( atarist_mmu_w )
	AM_RANGE( 0xff8200, 0xff82ff) AM_WRITE( atarist_shifter_w) AM_BASE( &paletteram )
	AM_RANGE( 0xff8600, 0xff860f) AM_WRITE( atarist_fdc_w )
	AM_RANGE( 0xff8800, 0xff88ff) AM_WRITE( atarist_psg_w )
	AM_RANGE( 0xff8a00, 0xff8a3f) AM_WRITE( atarist_blitter_w) AM_BASE( &atarist_blitter_ram )
	AM_RANGE( 0xfffa00, 0xfffa2f) AM_WRITE( atarist_mfp_w )
	AM_RANGE( 0xfffc00, 0xfffc07) AM_WRITE( atarist_acia_w )
ADDRESS_MAP_END

/***************************************************************************/

INPUT_PORTS_START( atarist )
	PORT_START /* Config Options */
	PORT_DIPNAME( 0x01, 0x00, "Number of floppy drives" )
	PORT_DIPSETTING(	0x00, "1" )
	PORT_DIPSETTING(	0x01, "2" )
	PORT_BIT( 0x06, 0x04, IPT_DIPSWITCH_NAME) PORT_NAME("Memory") PORT_TOGGLE
	PORT_DIPSETTING(	0x02, "512k" )
	PORT_DIPSETTING(	0x04, "1 Meg" )
	PORT_DIPSETTING(	0x06, "2 Meg" )
	PORT_DIPSETTING(	0x00, "4 Meg" )
//	PORT_DIPNAME( 0x08, 0x00, "Skip bootsector" )
//	PORT_DIPSETTING(	0x00, "No" ) //change to drive C emulation
//	PORT_DIPSETTING(	0x08, "Yes" )
	PORT_DIPNAME( 0x10, 0x00, "Port 0" )
	PORT_DIPSETTING(	0x00, "Mouse" )
	PORT_DIPSETTING(	0x10, "Joystick" )
	PORT_DIPNAME( 0x20, 0x20, "Port 1" )
	PORT_DIPSETTING(	0x00, "Nothing" )
	PORT_DIPSETTING(	0x20, "Joystick" )

	//blitter?
	//parallel port joysticks
	//mono/colour
	//write protect on disk 1/2

//CHANGE to PLAYER2 later!!  core fix for default..

	PORT_START /* Mouse */
	PORT_BIT( 0xffff, 0x0000, IPT_TRACKBALL_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)

	PORT_START /* Mouse */
	PORT_BIT( 0xffff, 0x0000, IPT_TRACKBALL_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)

	PORT_START /* Mouse - Buttons */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_PLAYER(1)

	PORT_START /* Joystick port 1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_PLAYER(2)

	PORT_START /* Joystick port 0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_PLAYER(1)

	PORT_START /* Keyboard */
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ESCAPE") PORT_CODE(KEYCODE_ESC)
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8)
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS)
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("=") PORT_CODE(KEYCODE_MINUS)
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Backspace") PORT_CODE(KEYCODE_BACKSPACE)
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Tab") PORT_CODE(KEYCODE_TAB)
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)

	PORT_START
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("[") PORT_CODE(KEYCODE_OPENBRACE)
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("]") PORT_CODE(KEYCODE_CLOSEBRACE)
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Return") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Control") PORT_CODE(KEYCODE_LCONTROL)
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)

	PORT_START
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(";") PORT_CODE(KEYCODE_COLON)
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("'") PORT_CODE(KEYCODE_QUOTE)
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("@") PORT_CODE(KEYCODE_TILDE)
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Left Shift") PORT_CODE(KEYCODE_LSHIFT)
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("\\") PORT_CODE(KEYCODE_BACKSLASH)
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)

	PORT_START
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(",") PORT_CODE(KEYCODE_COMMA)
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(".") PORT_CODE(KEYCODE_STOP)
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("/") PORT_CODE(KEYCODE_SLASH)
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Right Shift") PORT_CODE(KEYCODE_RSHIFT)
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Alternate") PORT_CODE(KEYCODE_LALT)
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE)
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Caps Lock") PORT_CODE(KEYCODE_RALT)
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F1") PORT_CODE(KEYCODE_F1)
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F2") PORT_CODE(KEYCODE_F2)
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F3") PORT_CODE(KEYCODE_F3)
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F4") PORT_CODE(KEYCODE_F4)
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F5") PORT_CODE(KEYCODE_F5)
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F6") PORT_CODE(KEYCODE_F6)

	PORT_START
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F7") PORT_CODE(KEYCODE_F7)
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F8") PORT_CODE(KEYCODE_F8)
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F9") PORT_CODE(KEYCODE_F9)
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F10") PORT_CODE(KEYCODE_F10)
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("HOME") PORT_CODE(KEYCODE_HOME)
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("UP") PORT_CODE(KEYCODE_UP)
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("- (Keypad)") PORT_CODE(KEYCODE_MINUS_PAD)
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT)
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT)
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("+ (Keypad)") PORT_CODE(KEYCODE_PLUS_PAD)
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN)

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Insert") PORT_CODE(KEYCODE_INSERT)
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Delete") PORT_CODE(KEYCODE_DEL)
	PORT_BIT( 0x7ff8, IP_ACTIVE_HIGH, IPT_UNUSED ) /* Scancodes 0x54 to 0x5f are unused */
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ISO (Atari Key)") PORT_CODE(KEYCODE_RCONTROL)

	PORT_START
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Undo") PORT_CODE(KEYCODE_PGUP)
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Help") PORT_CODE(KEYCODE_PGDN)
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("( (Keypad)") PORT_CODE(KEYCODE_F11)
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(") (Keypad)") PORT_CODE(KEYCODE_F12)
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("/ (Keypad)") PORT_CODE(KEYCODE_SLASH_PAD)
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("* (Keypad)") PORT_CODE(KEYCODE_ASTERISK)
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("7 (Keypad)") PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("8 (Keypad)") PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9 (Keypad)") PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("4 (Keypad)") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("5 (Keypad)") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("6 (Keypad)") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("1 (Keypad)") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2 (Keypad)") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3 (Keypad)") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0 (Keypad)") PORT_CODE(KEYCODE_0_PAD)

	PORT_START
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(". (Keypad)") PORT_CODE(KEYCODE_DEL_PAD)
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Enter (Keypad)") PORT_CODE(KEYCODE_ENTER_PAD)
	PORT_BIT( 0xfffc, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

/***************************************************************************/

static gfx_decode gfxdecodeinfo[] =
{
	{ -1 } /* end of array */
};

/***************************************************************************/

enum
{
	ATARI_ST_IMAGE_TYPE_NONE,
	ATARI_ST_IMAGE_TYPE_MSA,
	ATARI_ST_IMAGE_TYPE_RAW
};

static int atari_st_image_type[2] = {ATARI_ST_IMAGE_TYPE_NONE, ATARI_ST_IMAGE_TYPE_NONE};


/* basic format like .ST */
static int atarist_basic_floppy_init(int id, mame_file *file, int open_mode)
{
	if (basicdsk_floppy_load(id)==INIT_PASS)
	{
		/* Figure out correct disk format, try standard formats first */
		if (file) {
			int s=mame_fsize(file),i,f=0;
			int table[][4]={
				{ 808960, 10, 2, 79 }, /* 79 tracks, 2 sides, 10 sectors */
				{ 819200, 10, 2, 80 }, /* 80 tracks, 2 sides, 10 sectors */
				{ 829440, 10, 2, 81 },
				{ 839680, 10, 2, 82 },
				{ 849920, 10, 2, 83 },

				{ 728064, 9, 2, 79 }, /* 79 tracks, 2 sides, 9 sectors */
				{ 737280, 9, 2, 80 },
				{ 746496, 9, 2, 81 },
				{ 755712, 9, 2, 82 },
				{ 764928, 9, 2, 83 },

				};

			for (i=0; i<10; i++)
				if (s==table[i][0]) {
					f=1;
					basicdsk_set_geometry(id, table[i][3], table[i][2], table[i][1], 512, 1 );
				}

			if (f==0) { /* No standard format found - take data from the bootsector */
				char bootsector[512];

				mame_fseek(file, 0, SEEK_SET);

				if (mame_fread(file, bootsector, 512))
				{
					int sectors, head, tracks;

					sectors=bootsector[0x18];
					head=bootsector[0x1a];
					tracks=(bootsector[0x13]|(bootsector[0x14]<<8))/sectors/head;

					/* set geometry so disk image can be read */
					basicdsk_set_geometry(id, tracks, head, sectors, 512, 1);

					logerror("Drive %c: Using %d tracks, %d sectors, %d sides for disk image (values from bootsector).\n",'A'+id, tracks, sectors, head );
				}
			}

			atari_st_image_type[id] = ATARI_ST_IMAGE_TYPE_RAW;
			return INIT_PASS;
		}
		else logerror("Disk open failed\n");
	}

	return INIT_FAIL;
}



/* load image */
int atarist_load(mame_file *file, unsigned char **ptr)
{
	if (file)
	{
		int datasize;
		unsigned char *data;

		/* get file size */
		datasize = mame_fsize(file);

		if (datasize!=0)
		{
			/* malloc memory for this data */
			data = malloc(datasize);

			if (data!=NULL)
			{
				/* read whole file */
				mame_fread(file, data, datasize);

				*ptr = data;

				logerror("File loaded!\r\n");

				/* ok! */
				return 1;
			}
		}
	}

	return 0;
}

/***************************************************************************/

static VIDEO_EOF( atarist )
{
	static int last_mem=-1;
	int mid,new_mem=readinputport(0)&6;


	/* Check if the user has changed memory configuration */
	if (new_mem!=last_mem) {
		switch (new_mem) {
			case 0x02: logerror("Memory set to 512k\n"); mid=0x80000; break;
			case 0x04: logerror("Memory set to 1 Meg\n"); mid=0x100000; break;
			case 0x06: logerror("Memory set to 2 Meg\n"); mid=0x200000; break;
			case 0x00: logerror("Memory set to 4 Meg\n"); mid=0x400000; break;
			default: mid=0x100000; break;
		}

		/* Turn unavailable memory areas into a NOP region */
		install_mem_write16_handler(0, 0x8,mid-1, MWA16_RAM);
		install_mem_read16_handler (0, 0x8,mid-1, MRA16_RAM);
		install_mem_write16_handler(0, mid,0x3fffff, MWA16_NOP);
		install_mem_read16_handler (0, mid,0x3fffff, MRA16_NOP);
		last_mem=new_mem;
	}
}

static MACHINE_RESET( atarist )
{
	unsigned char *RAM = memory_region(REGION_USER1);
	unsigned char *RAM2 = memory_region(REGION_CPU1);
	unsigned char *RAM3 = memory_region(REGION_USER2);

	/* Setup ROM */
	memory_set_bankptr(1,&RAM3[0]);
	memory_set_bankptr(2,&RAM[0]);

	/* Setup first 8 bytes of memory */
	WRITE_WORD(&RAM2[0],READ_WORD(&RAM[0]));
	WRITE_WORD(&RAM2[2],READ_WORD(&RAM[2]));
	WRITE_WORD(&RAM2[4],READ_WORD(&RAM[4]));
	WRITE_WORD(&RAM2[6],READ_WORD(&RAM[6]));

	mfp_init();
	wd17xx_init(WD_TYPE_177X, atarist_fdc_callback, NULL);
	ikbd_reset();
	atarist_current_drive=-1;
}

static INTERRUPT_GEN( atarist_interrupt )
{
	static int keyboard_line;
	current_line = 311 - cpu_getiloops();

	/* 'Randomise' where the keyboard interrupts are triggered each frame */
	if (current_line==keyboard_line) {
		atarist_keyboard_update();
		keyboard_line=(keyboard_line+517)%311;
	}

	/* Start and end lines can change according to overscan effects */
	if (current_line>=start_line && current_line<end_line) {
		atarist_drawline(current_line,current_pixel,320+96);
		atarist_shifter_offset+=160; /* Offset to next line to draw */
		current_pixel=96;

		/* Timer B event counter pin is connected to shifter pin DE (toggles every bitmap line [NOT borders]) */
		if (mfp.tbty==EVENT) {
			mfp.tbdr-=1;
			if (mfp.tbdr<1) {
				mfp.tbdr=mfp.tbrl; /* Reload data register */
				timer_b_callback(0); /* Should perhaps fire 96 cycles later than this (after left border?) */
			}
		}
	}
	else
		atarist_drawborder(current_line);

	if (current_line==306) { /* Actual Vblank position unknown, this must be close though */
		start_line=63; /* Reset shifter values to draw 200 lines, no overscan */
		end_line=start_line+200;
		atarist_shifter_offset=0; /* Reset shifter start address every VBL */
		return 4; /* Hardware V-Blank */
	}

	return 2; /* Hardware H-Blank */
}

/***************************************************************************/

static int ym2149_port_a,ym2149_port_b;

static  READ8_HANDLER( ym2149_port_a_r )
{
	return ym2149_port_a;
}

static  READ8_HANDLER( ym2149_port_b_r )
{
	return ym2149_port_b;
}

static WRITE8_HANDLER( ym2149_port_a_w )
{
	static int strobe=-1;

	ym2149_port_a=data&0xff;

	/* info from "Atari ST Internals" by my friend James Boulton ;) */
	wd17xx_set_side(data & 0x01);

	/* not sure if these bits form drive index or, select drives independantly */
	/* for now select drive 0 */
	wd17xx_set_drive(0);

//	logerror("PSG port a %02x\n",data);
	/* BIT 8 RTS */
	/* BIT 10 DTR */

	/* BIT 20 Centronics Strobe */
	if (data&0x20 && strobe==0) { /* 0 -> 1 causes transition */
		logerror("Parallel port output byte %02x (%c)\n",ym2149_port_b,ym2149_port_b);
	}
	strobe=data&0x20;

	/* BIT 40 */
	/* BIT 80 */
}

static WRITE8_HANDLER( ym2149_port_b_w )
{
	/* Parallel port byte */
	ym2149_port_b=data&0xff;
}

static struct AY8910interface ay8910_interface =
{
	ym2149_port_a_r,
	ym2149_port_b_r,
	ym2149_port_a_w,
	ym2149_port_b_w
};

/***************************************************************************/

static MACHINE_DRIVER_START( ataris )
	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, MASTER_CLOCK)        /* 8 MHz */
	MDRV_CPU_PROGRAM_MAP(atarist_readmem,atarist_writemem)
	MDRV_CPU_VBLANK_INT(atarist_interrupt,312)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(TIME_IN_USEC(0))
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_RESET( atarist )

	/* video hardware, doubled to allow mode changes */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(512*2, 313*1)	/* 50Hz values (Note, 60Hz is 508 by 315) */
	/*MDRV_SCREEN_VISIBLE_AREA(0, 639, 0+63, 239+63)*/
	/*MDRV_SCREEN_VISIBLE_AREA(0, 639, 0+(63*2), (239+63)*2)*/
	MDRV_SCREEN_VISIBLE_AREA(96*2, (320+96)*2-1, 0+63, 239+63)
	/*MDRV_SCREEN_VISIBLE_AREA(0, 639, 0, 239+63)*/
	MDRV_GFXDECODE( gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(512)
	MDRV_COLORTABLE_LENGTH(512)

	MDRV_VIDEO_EOF( atarist )
	MDRV_VIDEO_START( atarist )
	MDRV_VIDEO_UPDATE( atarist )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910, 2000000)
	MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)	
MACHINE_DRIVER_END


#if 0
static struct MachineDriver machine_driver_stmono =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			MASTER_CLOCK, /* 8 MHz */
			atarist_readmem,atarist_writemem,0,0,
			atarist_interrupt,500
		}
	},
	71, 0,
	1,
	atarist_init_machine,	/* init machine */
	0,

	/* video hardware, doubled to allow mode changes */
  	640, 501,
 	{ 0, 639, 0, 399 },

	gfxdecodeinfo,
	2,2,
	0,

	VIDEO_TYPE_RASTER,
	atarist_keyboard_update,
	atarist_vh_start,
	atarist_vh_stop,
	atarist_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};
#endif

/***************************************************************************/

ROM_START( atarist )
	ROM_REGION(0x400000, REGION_CPU1,0)
	/* Up to 4 Meg Main RAM */

	ROM_REGION(0x40000, REGION_USER1,ROMREGION_16BIT) /* System rom */
//	ROM_LOAD_WIDE( "tos_206.img", 0x00000, 0x30000,CRC(3f2f840f) SHA1(ee58768bdfc602c9b14942ce5481e97dd24e7c83))

	ROM_LOAD ( "tos_104.img", 0x00000, 0x30000,CRC(90f4fbff) SHA1(2487f330b0895e5d88d580d4ecb24061125e88ad))
//	ROM_LOAD_WIDE( "tos102uk.rom", 0x00000, 0x30000,CRC(3b5cd0c5) SHA1(87900a40a890fdf03bd08be6c60cc645855cbce5))
//	ROM_LOAD_WIDE( "tos100.img", 0x00000, 0x30000,CRC(d331af30) SHA1(7bcc2311d122f451bd03c9763ade5a119b2f90da))

	ROM_REGION(0x40000, REGION_USER2,0) /* Cartridge rom */
	//ROM_LOAD_WIDE( "cart.exe", 0x00000, 0x30000,CRC(b997f9cb))
ROM_END

/***************************************************************************/
/* msa image handling */
/* hacked together quickly from basicdsk. No error checking and can be heavily improved! */
static void msa_seek_callback(int,int);
static int msa_get_sectors_per_track(int,int);
static void msa_get_id_callback(int, chrn_id *, int, int);
static void msa_read_sector_data_into_buffer(int drive, int side, int index1, char *ptr, int length);
static void msa_write_sector_data_from_buffer(int drive, int side, int index1, char *ptr, int length,int ddam);

floppy_interface msa_floppy_interface=
{
	msa_seek_callback,
	msa_get_sectors_per_track,             /* done */
	msa_get_id_callback,                   /* done */
	msa_read_sector_data_into_buffer,      /* done */
	msa_write_sector_data_from_buffer, /* done */
	NULL
};

struct msa_image
{
	int sectors_per_track;
	int current_track;
};

/* data for each image loaded into memory */
static unsigned char *msa_images_data[2] = {NULL, NULL};

/* data to handle each image */
static struct msa_image msa_images[2];

void    msa_get_id_callback(int drive, chrn_id *id, int id_index, int side)
{
	struct msa_image *w = &msa_images[drive];

	/* construct a id value */
	id->C = w->current_track;
	id->H = side;
	id->R = 1 + id_index;
    id->N = 2;

	id->data_id = 1 + id_index;
	id->flags = 0;
}

int  msa_get_sectors_per_track(int drive, int side)
{
	struct msa_image *w = &msa_images[drive];

	/* return number of sectors per track */
	return w->sectors_per_track;
}

void    msa_seek_callback(int drive, int physical_track)
{
	struct msa_image *image = &msa_images[drive];

	image->current_track = physical_track;
}

unsigned char *msa_get_data_ptr(int drive, int track, int side, int sector_index)
{
	struct msa_image *w = &msa_images[drive];

	return (unsigned char *)(
		(unsigned long)msa_images_data[drive] + (unsigned long)
		(
		/* header */
		12 +
		/* skip to track start */
		(((track*2)+side)*(w->sectors_per_track*512+2)) +
		/* skip to sector */
		((sector_index-1)*512)
		));
}

void msa_write_sector_data_from_buffer(int drive, int side, int index1, char *ptr, int length, int ddam)
{
	struct msa_image *w = &msa_images[drive];
	unsigned char *pDataPtr;

	pDataPtr = msa_get_data_ptr(drive, w->current_track, side, index1);

	memcpy(pDataPtr, ptr, length);
}

void msa_read_sector_data_into_buffer(int drive, int side, int index1, char *ptr, int length)
{
	struct msa_image *w = &msa_images[drive];
	unsigned char *pDataPtr;

	pDataPtr = msa_get_data_ptr(drive, w->current_track, side, index1);

	memcpy(ptr, pDataPtr, length);
}

int atarist_msa_floppy_init(mess_image *img, mame_file *file, int open_mode)
{
	/* load whole file into memory */
	if (atarist_load(file, &msa_images_data[id])!=NULL)
	{
		if (msa_images_data[id][0]!=0xe && msa_images_data[id][1]!=0xf)
			logerror("MSA Warning:  Header doesn't match\n");

		msa_images[id].sectors_per_track = msa_images_data[id][3];

		/* set type */
		atari_st_image_type[id] = ATARI_ST_IMAGE_TYPE_MSA;

		/* tell floppy drive code to use these functions for accessing disk image in this drive */
		floppy_drive_set_disk_image_interface(img,&msa_floppy_interface);

		return INIT_PASS;
	}

	return INIT_FAIL;
}

void atarist_msa_floppy_exit(int id)
{
	if (msa_images_data[id]!=NULL)
	{
		free(msa_images_data[id]);
	}
	msa_images_data[id] = NULL;
}

/***************************************************************************/

/* init for msa and raw image types */
int atarist_floppy_init(int id)
{
	const char *name;

	name = image_filename(IO_FLOPPY,id);
	if (!name)
		return INIT_PASS; /* Emulation can continue even with no floppy */

	/* msa file? */
	if (!strcmp(".msa",name+strlen(name)-4))
	{
		/* yes - setup msa access */
		return atarist_msa_floppy_init(id);
	}

	/* no, assume basic (e.g. ".st"), setup basic access */
	return atarist_basic_floppy_init(id);
}

/* exit for msa and raw image types */
void atarist_floppy_exit(int id)
{
	switch (atari_st_image_type[id])
	{
		case ATARI_ST_IMAGE_TYPE_RAW:
		{
		}
		break;

		case ATARI_ST_IMAGE_TYPE_MSA:
		{
			atarist_msa_floppy_exit(id);
		}
		break;

		default:
			break;
	}
}

/***************************************************************************/

SYSTEM_CONFIG_START(atarist)
	CONFIG_DEVICE_LEGACY(IO_FLOPPY, 1, "st,msa", DEVICE_LOAD_RESETS_NONE, OSD_FOPEN_RW_CREATE_OR_READ,
		atarist_floppy_init, atarist_floppy_exit, floppy_status)
SYSTEM_CONFIG_END

/*
References:

http://www.atari-history.com/computers/16bits/a1632bit.html
http://home.wanadoo.nl/jarod/museum/megaste.htm

*/

/* Atari 1040 ST - 1 Meg, OS on ROM, internal floppy drive */
/* 1990: Atari 1040 STE - */
/* Mega ST - Seperate keyboard, */

/* 1986 : Atari EST - Proposed 68020, 68881 FPU, 4 Meg ram, never released! */

/* 1985 : Atari 130 ST - Prototype seen at Winter Las Vegas Consumer Electronics Show, 128k RAM */
/* 1985 : Atari 260 ST - 256k RAM version of above */
/* 1985 : Atari 520 ST - 512k RAM version of above */

/* 1987 : Atari 520 STf (1st version) - 512k RAM, OS on disk (only 32k boot rom onboard), external floppy drive (Single sided) */
/* 1987 : Atari 520 STf (2nd version) - 512k RAM, OS in ROM (Tos 1.02), external floppy drive (Single or double sided) */

/* 1987 : Mega STf - Seperate keyboard, blitter, 1/2/4 Meg ram options */
/* 1989 : Mega STe - Seperate keyboard, 16MHz 68000 (switchable to 8MHz), memory cache, VME bus, TOS 2.05 & 2.06 */

/*     YEAR  NAME      PARENT    COMPAT	MACHINE   INPUT     INIT	CONFIG   COMPANY    FULLNAME */
COMP ( 1985, atarist,  0,        0,		atarist,  atarist,  0,     atarist, "atarist", "atarist" , 0)
