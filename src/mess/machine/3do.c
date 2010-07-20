/*

Reset boot sequence:
- jump to ROM address space 03000028)
- determine source of reset from cstatbits (03000068)
- write cstatbits to safe madam location (03000078)
- clear cstatbits (0300007c)
- disable interrupts
- set up audout register
- set up brooktree part (03001020)
- wait 100ms
- set msysbits register for max memory config
- set sltime to 178906
- write 0 to 0 to unmap ram address space
- do 8 reads to 4 banks memory tyest
- do cbr to both banks of vram
- determine ram & vram sizes
- do initial diagnostics, including test of initial 64kb of ram (0300021c)
- copy remainder of code to ram at address 0 (030000224)
- jump to address 0 and continue boot sequence
- do some more memory checks (00000298)
- init stack pointer to 64k
- set hdelay to c6
- do remaining diagnostics
- set up vdl for 3do logo screen
- set up frame buffer 3do logo screen
- transfer sherry from rom to 10000
- transfer operator from rom to 20000
- transfer dipir from rom to 200
- transfer filesystem from rom to 28000
- init stack pointer to 64k
- store sherry address to 28 for use by dipir
- delay for at least 600ms to allow expansion bus to start
- wait for vcount = 10 and enable clut transfer
- wait for vcount = 10 and enable video
- set adbio to disable software controlled muting
- init registers for entry to sherry
- jump to sherry

*/

#include "emu.h"
#include "includes/3do.h"


typedef struct {
	/* 03180000 - 0318003f - configuration group */
	/* 03180040 - 0318007f - diagnostic UART */

	UINT8	cg_r_count;
	UINT8	cg_w_count;
	UINT32	cg_input;
	UINT32	cg_output;
} SLOW2;


typedef struct {
	UINT32	revision;		/* 03300000 */
	UINT32	msysbits;		/* 03300004 */
	UINT32	mctl;			/* 03300008 */
	UINT32	sltime;			/* 0330000c */
	UINT32	abortbits;		/* 03300020 */
	UINT32	privbits;		/* 03300024 */
	UINT32	statbits;		/* 03300028 */
	UINT32	diag;			/* 03300040 */

	UINT32	ccobctl0;		/* 03300110 */
	UINT32	ppmpc;			/* 03300120 */

	UINT32	regctl0;		/* 03300130 */
	UINT32	regctl1;		/* 03300134 */
	UINT32	regctl2;		/* 03300138 */
	UINT32	regctl3;		/* 0330013c */
	UINT32	xyposh;			/* 03300140 */
	UINT32	xyposl;			/* 03300144 */
	UINT32	linedxyh;		/* 03300148 */
	UINT32	linedxyl;		/* 0330014c */
	UINT32	dxyh;			/* 03300150 */
	UINT32	dxyl;			/* 03300154 */
	UINT32	ddxyh;			/* 03300158 */
	UINT32	ddxyl;			/* 0330015c */

	UINT32	pip[16];		/* 03300180-033001bc (W); 03300180-033001fc (R) */
	UINT32	fence[16];		/* 03300200-0330023c (W); 03300200-0330027c (R) */
	UINT32	mmu[64];		/* 03300300-033003fc */
	UINT32	dma[32][4];		/* 03300400-033005fc */
	UINT32	mult[40];		/* 03300600-0330069c */
	UINT32	mult_control;	/* 033007f0-033007f4 */
	UINT32	mult_status;	/* 033007f8 */
} MADAM;


typedef struct {
	UINT32	revision;		/* 03400000 */
	UINT32	csysbits;		/* 03400004 */
	UINT32	vint0;			/* 03400008 */
	UINT32	vint1;			/* 0340000c */
	UINT32	audin;			/* 03400020 */
	UINT32	audout;			/* 03400024 */
	UINT32	cstatbits;		/* 03400028 */
	UINT32	wdog;			/* 0340002c */
	UINT32	hcnt;			/* 03400030 */
	UINT32	vcnt;			/* 03400034 */
	UINT32	seed;			/* 03400038 */
	UINT32	random;			/* 0340004c */
	UINT32	irq0;			/* 03400040 / 03400044 */
	UINT32	irq0_enable;	/* 03400048 / 0340004c */
	UINT32	mode;			/* 03400050 / 03400054 */
	UINT32	badbits;		/* 03400058 */
	UINT32	spare;			/* 0340005c */
	UINT32	irq1;			/* 03400060 / 03400064 */
	UINT32	irq1_enable;	/* 03400068 / 0340006c */
	UINT32	hdelay;			/* 03400080 */
	UINT32	adbio;			/* 03400084 */
	UINT32	adbctl;			/* 03400088 */
							/* Timers */
	UINT32	timer0;			/* 03400100 */
	UINT32	timerback0;		/* 03400104 */
	UINT32	timer1;			/* 03400108 */
	UINT32	timerback1;		/* 0340010c */
	UINT32	timer2;			/* 03400110 */
	UINT32	timerback2;		/* 03400114 */
	UINT32	timer3;			/* 03400118 */
	UINT32	timerback3;		/* 0340011c */
	UINT32	timer4;			/* 03400120 */
	UINT32	timerback4;		/* 03400124 */
	UINT32	timer5;			/* 03400128 */
	UINT32	timerback5;		/* 0340012c */
	UINT32	timer6;			/* 03400130 */
	UINT32	timerback6;		/* 03400134 */
	UINT32	timer7;			/* 03400138 */
	UINT32	timerback7;		/* 0340013c */
	UINT32	timer8;			/* 03400140 */
	UINT32	timerback8;		/* 03400144 */
	UINT32	timer9;			/* 03400148 */
	UINT32	timerback9;		/* 0340014c */
	UINT32	timer10;		/* 03400150 */
	UINT32	timerback10;	/* 03400154 */
	UINT32	timer11;		/* 03400158 */
	UINT32	timerback11;	/* 0340015c */
	UINT32	timer12;		/* 03400160 */
	UINT32	timerback12;	/* 03400164 */
	UINT32	timer13;		/* 03400168 */
	UINT32	timerback13;	/* 0340016c */
	UINT32	timer14;		/* 03400170 */
	UINT32	timerback14;	/* 03400174 */
	UINT32	timer15;		/* 03400178 */
	UINT32	timerback15;	/* 0340017c */
	UINT32	settm0;			/* 03400200 */
	UINT32	clrtm0;			/* 03400204 */
	UINT32	settm1;			/* 03400208 */
	UINT32	clrtm1;			/* 0340020c */
	UINT32	slack;			/* 03400220 */
							/* DMA */
	UINT32	dmareqdis;		/* 03400308 */
							/* Expansion bus */
	UINT32	type0_4;		/* 03400400 */
	UINT32	dipir1;			/* 03400410 */
	UINT32	dipir2;			/* 03400414 */
							/* DSPP */
	UINT32	semaphore;		/* 034017d0 */
	UINT32	semaack;		/* 034017d4 */
	UINT32	dsppdma;		/* 034017e0 */
	UINT32	dspprst0;		/* 034017e4 */
	UINT32	dspprst1;		/* 034017e8 */
	UINT32	dspppc;			/* 034017f4 */
	UINT32	dsppnr;			/* 034017f8 */
	UINT32	dsppgw;			/* 034017fc */
	UINT32	dsppn[0x400];	/* 03401800 - 03401bff DSPP N stack (32bit writes) */
							/* 03402000 - 034027ff DSPP N stack (16bit writes) */
	UINT32	dsppei[0x100];	/* 03403000 - 034030ff DSPP EI stack (32bit writes) */
							/* 03403400 - 034035ff DSPP EI stack (16bit writes) */
	UINT32	dsppeo[0x1f];	/* 03403800 - 0340381f DSPP EO stack (32bit reads) */
							/* 03403c00 - 03403c3f DSPP EO stack (32bit reads) */
	UINT32	dsppclkreload;	/* 034039dc / 03403fbc */
							/* UNCLE */
	UINT32	unclerev;		/* 0340c000 */
	UINT32	uncle_soft_rev;	/* 0340c004 */
	UINT32	uncle_addr;		/* 0340c008 */
	UINT32	uncle_rom;		/* 0340c00c */
} CLIO;


static SLOW2	slow2;
static MADAM	madam;
static CLIO		clio;
static UINT32	svf_color;


READ32_HANDLER( _3do_nvarea_r ) {
	logerror( "%08X: NVRAM read offset = %08X\n", cpu_get_pc(space->machine->device("maincpu")), offset );
	return 0;
}

WRITE32_HANDLER( _3do_nvarea_w ) {
	logerror( "%08X: NVRAM write offset = %08X, data = %08X, mask = %08X\n", cpu_get_pc(space->machine->device("maincpu")), offset, data, mem_mask );
}



/*
    I have no idea what piece of hardware this is. Possibly some kind of communication hardware using shift registers.

    During booting the following things get written/read:
    - write 03180000 to 03180000 (init reading sequence)
    - 4 read actions from 03180000 bit#0, if lower bit is 1 0 1 0 then   (expect to read 0x0a)
	- read from 03180000 (init writing sequence)
	- write 0x0100 to shift register bit#0
	- reset PON bit in CSTATBITS
    - wait a little while
    - read from 03180000
	- write 0x0200 to shift register bit#0
	- write 0x0002 to shift register bit#1
	- dummy write to 03180000
	- read from shift register bits #0 and #1.
	- check if data read equals 0x44696167 (=Diag)   If so, jump 302196c in default bios
	- dummy read from shift register
	- write 0x0300 to shift register bit #0
	- dummy write to shift register
    - read data from shift register bit #0.
    - xor that data with 0x07ff
	- write that data & 0x00ff | 0x4000 to the shift register
3022630
*/

READ32_HANDLER( _3do_slow2_r ) {
	UINT32 data = 0;

	logerror( "%08X: UNK_318 read offset = %08X\n", cpu_get_pc(space->machine->device("maincpu")), offset );

	switch( offset ) {
	case 0:		/* Boot ROM checks here and expects to read 1, 0, 1, 0 in the lowest bit */
		data = slow2.cg_output & 0x00000001;
		slow2.cg_output = slow2.cg_output >> 1;
		slow2.cg_w_count = 0;
	}
	return data;
}


WRITE32_HANDLER( _3do_slow2_w )
{
	logerror( "%08X: UNK_318 write offset = %08X, data = %08X, mask = %08X\n", cpu_get_pc(space->machine->device("maincpu")), offset, data, mem_mask );

	switch( offset )
	{
		case 0:		/* Boot ROM writes 03180000 here and then starts reading some things */
		{
			/* disable ROM overlay */
			memory_set_bank(space->machine, "bank1", 0);
		}
		slow2.cg_input = slow2.cg_input << 1 | ( data & 0x00000001 );
		slow2.cg_w_count ++;
		if ( slow2.cg_w_count == 16 )
		{
		}
		break;
	}
}


void _3do_slow2_init( running_machine *machine )
{
	slow2.cg_input = 0;
	slow2.cg_output = 0x00000005 - 1;
}


READ32_HANDLER( _3do_svf_r )
{
	logerror( "%08X: SVF read offset = %08X\n", cpu_get_pc(space->machine->device("maincpu")), offset*4 );
	switch( offset & ( 0xE000 / 4 ) )
	{
	case 0x0000/4:		/* SPORT transfer */
		break;
	case 0x2000/4:		/* Write to color register */
		return svf_color;
	case 0x4000/4:		/* Flash write */
		break;
	case 0x6000/4:		/* CAS before RAS refresh/reset (CBR). Used to initialize VRAM mode during boot. */
		break;
	}
	return 0;
}

WRITE32_HANDLER( _3do_svf_w )
{
	logerror( "%08X: SVF write offset = %08X, data = %08X, mask = %08X\n", cpu_get_pc(space->machine->device("maincpu")), offset*4, data, mem_mask );
	switch( offset & ( 0xe000 / 4 ) )
	{
	case 0x0000/4:		/* SPORT transfer */
		break;
	case 0x2000/4:		/* Write to color register */
		svf_color = data;
		break;
	case 0x4000/4:		/* Flash write */
		break;
	case 0x6000/4:		/* CAS before RAS refresh/reset (CBR). Used to initialize VRAM mode during boot. */
		break;
	}
}



READ32_HANDLER( _3do_madam_r ) {
	//logerror( "%08X: MADAM read offset = %08X\n", cpu_get_pc(space->machine->device("maincpu")), offset*4 );

	switch( offset ) {
	case 0x0000/4:		/* 03300000 - Revision */
		return madam.revision;
	case 0x0004/4:
		return madam.msysbits;
	case 0x0008/4:
		return madam.mctl;
	case 0x000c/4:
		return madam.sltime;
	case 0x0020/4:
		return madam.abortbits;
	case 0x0024/4:
		return madam.privbits;
	case 0x0028/4:
		return madam.statbits;
	case 0x0040/4:
		return madam.diag;
	case 0x0110/4:
		return madam.ccobctl0;
	case 0x0120/4:
		return madam.ppmpc;
	case 0x0130/4:
		return madam.regctl0;
	case 0x0134/4:
		return madam.regctl1;
	case 0x0138/4:
		return madam.regctl2;
	case 0x013c/4:
		return madam.regctl3;
	case 0x0140/4:
		return madam.xyposh;
	case 0x0144/4:
		return madam.xyposl;
	case 0x0148/4:
		return madam.linedxyh;
	case 0x014c/4:
		return madam.linedxyl;
	case 0x0150/4:
		return madam.dxyh;
	case 0x0154/4:
		return madam.dxyl;
	case 0x0158/4:
		return madam.ddxyh;
	case 0x015c/4:
		return madam.ddxyl;
	case 0x0180/4: case 0x0188/4:
	case 0x0190/4: case 0x0198/4:
	case 0x01a0/4: case 0x01a8/4:
	case 0x01b0/4: case 0x01b8/4:
	case 0x01c0/4: case 0x01c8/4:
	case 0x01d0/4: case 0x01d8/4:
	case 0x01e0/4: case 0x01e8/4:
	case 0x01f0/4: case 0x01f8/4:
		return madam.pip[(offset/2) & 0x0f] & 0xffff;
	case 0x0184/4: case 0x018c/4:
	case 0x0194/4: case 0x019c/4:
	case 0x01a4/4: case 0x01ac/4:
	case 0x01b4/4: case 0x01bc/4:
	case 0x01c4/4: case 0x01cc/4:
	case 0x01d4/4: case 0x01dc/4:
	case 0x01e4/4: case 0x01ec/4:
	case 0x01f4/4: case 0x01fc/4:
		return madam.pip[(offset/2) & 0x0f] >> 16;
	case 0x0200/4: case 0x0208/4:
	case 0x0210/4: case 0x0218/4:
	case 0x0220/4: case 0x0228/4:
	case 0x0230/4: case 0x0238/4:
	case 0x0240/4: case 0x0248/4:
	case 0x0250/4: case 0x0258/4:
	case 0x0260/4: case 0x0268/4:
	case 0x0270/4: case 0x0278/4:
		return madam.fence[(offset/2) & 0x0f] & 0xffff;
	case 0x0204/4: case 0x020c/4:
	case 0x0214/4: case 0x021c/4:
	case 0x0224/4: case 0x022c/4:
	case 0x0234/4: case 0x023c/4:
	case 0x0244/4: case 0x024c/4:
	case 0x0254/4: case 0x025c/4:
	case 0x0264/4: case 0x026c/4:
	case 0x0274/4: case 0x027c/4:
		return madam.fence[(offset/2) & 0x0f] >> 16;
	case 0x0300/4: case 0x0304/4: case 0x0308/4: case 0x030c/4:
	case 0x0310/4: case 0x0314/4: case 0x0318/4: case 0x031c/4:
	case 0x0320/4: case 0x0324/4: case 0x0328/4: case 0x032c/4:
	case 0x0330/4: case 0x0334/4: case 0x0338/4: case 0x033c/4:
	case 0x0340/4: case 0x0344/4: case 0x0348/4: case 0x034c/4:
	case 0x0350/4: case 0x0354/4: case 0x0358/4: case 0x035c/4:
	case 0x0360/4: case 0x0364/4: case 0x0368/4: case 0x036c/4:
	case 0x0370/4: case 0x0374/4: case 0x0378/4: case 0x037c/4:
	case 0x0380/4: case 0x0384/4: case 0x0388/4: case 0x038c/4:
	case 0x0390/4: case 0x0394/4: case 0x0398/4: case 0x039c/4:
	case 0x03a0/4: case 0x03a4/4: case 0x03a8/4: case 0x03ac/4:
	case 0x03b0/4: case 0x03b4/4: case 0x03b8/4: case 0x03bc/4:
	case 0x03c0/4: case 0x03c4/4: case 0x03c8/4: case 0x03cc/4:
	case 0x03d0/4: case 0x03d4/4: case 0x03d8/4: case 0x03dc/4:
	case 0x03e0/4: case 0x03e4/4: case 0x03e8/4: case 0x03ec/4:
	case 0x03f0/4: case 0x03f4/4: case 0x03f8/4: case 0x03fc/4:
		return madam.mmu[offset&0x3f];
	case 0x0400/4: case 0x0404/4: case 0x0408/4: case 0x040c/4:
	case 0x0410/4: case 0x0414/4: case 0x0418/4: case 0x041c/4:
	case 0x0420/4: case 0x0424/4: case 0x0428/4: case 0x042c/4:
	case 0x0430/4: case 0x0434/4: case 0x0438/4: case 0x043c/4:
	case 0x0440/4: case 0x0444/4: case 0x0448/4: case 0x044c/4:
	case 0x0450/4: case 0x0454/4: case 0x0458/4: case 0x045c/4:
	case 0x0460/4: case 0x0464/4: case 0x0468/4: case 0x046c/4:
	case 0x0470/4: case 0x0474/4: case 0x0478/4: case 0x047c/4:
	case 0x0480/4: case 0x0484/4: case 0x0488/4: case 0x048c/4:
	case 0x0490/4: case 0x0494/4: case 0x0498/4: case 0x049c/4:
	case 0x04a0/4: case 0x04a4/4: case 0x04a8/4: case 0x04ac/4:
	case 0x04b0/4: case 0x04b4/4: case 0x04b8/4: case 0x04bc/4:
	case 0x04c0/4: case 0x04c4/4: case 0x04c8/4: case 0x04cc/4:
	case 0x04d0/4: case 0x04d4/4: case 0x04d8/4: case 0x04dc/4:
	case 0x04e0/4: case 0x04e4/4: case 0x04e8/4: case 0x04ec/4:
	case 0x04f0/4: case 0x04f4/4: case 0x04f8/4: case 0x04fc/4:
	case 0x0500/4: case 0x0504/4: case 0x0508/4: case 0x050c/4:
	case 0x0510/4: case 0x0514/4: case 0x0518/4: case 0x051c/4:
	case 0x0520/4: case 0x0524/4: case 0x0528/4: case 0x052c/4:
	case 0x0530/4: case 0x0534/4: case 0x0538/4: case 0x053c/4:
	case 0x0540/4: case 0x0544/4: case 0x0548/4: case 0x054c/4:
	case 0x0550/4: case 0x0554/4: case 0x0558/4: case 0x055c/4:
	case 0x0560/4: case 0x0564/4: case 0x0568/4: case 0x056c/4:
	case 0x0570/4: case 0x0574/4: case 0x0578/4: case 0x057c/4:
	case 0x0580/4: case 0x0584/4: case 0x0588/4: case 0x058c/4:
	case 0x0590/4: case 0x0594/4: case 0x0598/4: case 0x059c/4:
	case 0x05a0/4: case 0x05a4/4: case 0x05a8/4: case 0x05ac/4:
	case 0x05b0/4: case 0x05b4/4: case 0x05b8/4: case 0x05bc/4:
	case 0x05c0/4: case 0x05c4/4: case 0x05c8/4: case 0x05cc/4:
	case 0x05d0/4: case 0x05d4/4: case 0x05d8/4: case 0x05dc/4:
	case 0x05e0/4: case 0x05e4/4: case 0x05e8/4: case 0x05ec/4:
	case 0x05f0/4: case 0x05f4/4: case 0x05f8/4: case 0x05fc/4:
		return madam.dma[(offset/4) & 0x1f][offset & 0x03];

	/* Hardware multiplier */
	case 0x0600/4: case 0x0604/4: case 0x0608/4: case 0x060c/4:
	case 0x0610/4: case 0x0614/4: case 0x0618/4: case 0x061c/4:
	case 0x0620/4: case 0x0624/4: case 0x0628/4: case 0x062c/4:
	case 0x0630/4: case 0x0634/4: case 0x0638/4: case 0x063c/4:
	case 0x0640/4: case 0x0644/4: case 0x0648/4: case 0x064c/4:
	case 0x0650/4: case 0x0654/4: case 0x0658/4: case 0x065c/4:
	case 0x0660/4: case 0x0664/4: case 0x0668/4: case 0x066c/4:
	case 0x0670/4: case 0x0674/4: case 0x0678/4: case 0x067c/4:
	case 0x0680/4: case 0x0684/4: case 0x0688/4: case 0x068c/4:
	case 0x0690/4: case 0x0694/4: case 0x0698/4: case 0x069c/4:
		return madam.mult[offset & 0xff];
	case 0x07f0/4:
		return madam.mult_control;
	case 0x07f8/4:
		return madam.mult_status;
	default:
		logerror( "%08X: unhandled MADAM read offset = %08X\n", cpu_get_pc(space->machine->device("maincpu")), offset*4 );
		break;
	}
	return 0;
}


WRITE32_HANDLER( _3do_madam_w ) {
	//logerror( "%08X: MADAM write offset = %08X, data = %08X, mask = %08X\n", cpu_get_pc(space->machine->device("maincpu")), offset*4, data, mem_mask );

	switch( offset ) {
	case 0x0000/4:
		break;
	case 0x0004/4:	/* 03300004 - Memory configuration 29 = 2MB DRAM, 1MB VRAM */
		madam.msysbits = data;
		break;
	case 0x0008/4:
		madam.mctl = data;
		break;
	case 0x000c/4:
		madam.sltime = data;
		break;
	case 0x0020/4:
		madam.abortbits = data;
		break;
	case 0x0024/4:
		madam.privbits = data;
		break;
	case 0x0028/4:
		madam.statbits = data;
	case 0x0040/4:
		madam.diag = 1;
		break;

	/* CEL */
	case 0x0100/4:	/* 03300100 - SPRSTRT - Start the CEL engine (W) */
	case 0x0104/4:	/* 03300104 - SPRSTOP - Stop the CEL engine (W) */
	case 0x0108/4:	/* 03300108 - SPRCNTU - Continue the CEL engine (W) */
	case 0x010c/4:	/* 0330010c - SPRPAUS - Pause the the CEL engine (W) */
		break;
	case 0x0110/4:	/* 03300110 - CCOBCTL0 - CCoB control (RW) */
		madam.ccobctl0 = data;
		break;
	case 0x0129/4:	/* 03300120 - PPMPC (RW) */
		madam.ppmpc = data;
		break;

	/* Regis */
	case 0x0130/4:
		madam.regctl0 = data;
		break;
	case 0x0134/4:
		madam.regctl1 = data;
		break;
	case 0x0138/4:
		madam.regctl2 = data;
		break;
	case 0x013c/4:
		madam.regctl3 = data;
		break;
	case 0x0140/4:
		madam.xyposh = data;
		break;
	case 0x0144/4:
		madam.xyposl = data;
		break;
	case 0x0148/4:
		madam.linedxyh = data;
		break;
	case 0x014c/4:
		madam.linedxyl = data;
		break;
	case 0x0150/4:
		madam.dxyh = data;
		break;
	case 0x0154/4:
		madam.dxyl = data;
		break;
	case 0x0158/4:
		madam.ddxyh = data;
		break;
	case 0x015c/4:
		madam.ddxyl = data;
		break;

	/* Pip */
	case 0x0180/4: case 0x0184/4: case 0x0188/4: case 0x018c/4:
	case 0x0190/4: case 0x0194/4: case 0x0198/4: case 0x019c/4:
	case 0x01a0/4: case 0x01a4/4: case 0x01a8/4: case 0x01ac/4:
	case 0x01b0/4: case 0x01b4/4: case 0x01b8/4: case 0x01bc/4:
		madam.pip[offset & 0x0f] = data;
		break;

	/* Fence */
	case 0x0200/4: case 0x0204/4: case 0x0208/4: case 0x020c/4:
	case 0x0210/4: case 0x0214/4: case 0x0218/4: case 0x021c/4:
	case 0x0220/4: case 0x0224/4: case 0x0228/4: case 0x022c/4:
	case 0x0230/4: case 0x0234/4: case 0x0238/4: case 0x023c/4:
		madam.fence[offset & 0x0f] = data;
		break;

	/* MMU */
	case 0x0300/4: case 0x0304/4: case 0x0308/4: case 0x030c/4:
	case 0x0310/4: case 0x0314/4: case 0x0318/4: case 0x031c/4:
	case 0x0320/4: case 0x0324/4: case 0x0328/4: case 0x032c/4:
	case 0x0330/4: case 0x0334/4: case 0x0338/4: case 0x033c/4:
	case 0x0340/4: case 0x0344/4: case 0x0348/4: case 0x034c/4:
	case 0x0350/4: case 0x0354/4: case 0x0358/4: case 0x035c/4:
	case 0x0360/4: case 0x0364/4: case 0x0368/4: case 0x036c/4:
	case 0x0370/4: case 0x0374/4: case 0x0378/4: case 0x037c/4:
	case 0x0380/4: case 0x0384/4: case 0x0388/4: case 0x038c/4:
	case 0x0390/4: case 0x0394/4: case 0x0398/4: case 0x039c/4:
	case 0x03a0/4: case 0x03a4/4: case 0x03a8/4: case 0x03ac/4:
	case 0x03b0/4: case 0x03b4/4: case 0x03b8/4: case 0x03bc/4:
	case 0x03c0/4: case 0x03c4/4: case 0x03c8/4: case 0x03cc/4:
	case 0x03d0/4: case 0x03d4/4: case 0x03d8/4: case 0x03dc/4:
	case 0x03e0/4: case 0x03e4/4: case 0x03e8/4: case 0x03ec/4:
	case 0x03f0/4: case 0x03f4/4: case 0x03f8/4: case 0x03fc/4:
		madam.mmu[offset&0x3f] = data;
		break;

	/* DMA */
	case 0x0400/4: case 0x0404/4: case 0x0408/4: case 0x040c/4:
	case 0x0410/4: case 0x0414/4: case 0x0418/4: case 0x041c/4:
	case 0x0420/4: case 0x0424/4: case 0x0428/4: case 0x042c/4:
	case 0x0430/4: case 0x0434/4: case 0x0438/4: case 0x043c/4:
	case 0x0440/4: case 0x0444/4: case 0x0448/4: case 0x044c/4:
	case 0x0450/4: case 0x0454/4: case 0x0458/4: case 0x045c/4:
	case 0x0460/4: case 0x0464/4: case 0x0468/4: case 0x046c/4:
	case 0x0470/4: case 0x0474/4: case 0x0478/4: case 0x047c/4:
	case 0x0480/4: case 0x0484/4: case 0x0488/4: case 0x048c/4:
	case 0x0490/4: case 0x0494/4: case 0x0498/4: case 0x049c/4:
	case 0x04a0/4: case 0x04a4/4: case 0x04a8/4: case 0x04ac/4:
	case 0x04b0/4: case 0x04b4/4: case 0x04b8/4: case 0x04bc/4:
	case 0x04c0/4: case 0x04c4/4: case 0x04c8/4: case 0x04cc/4:
	case 0x04d0/4: case 0x04d4/4: case 0x04d8/4: case 0x04dc/4:
	case 0x04e0/4: case 0x04e4/4: case 0x04e8/4: case 0x04ec/4:
	case 0x04f0/4: case 0x04f4/4: case 0x04f8/4: case 0x04fc/4:
	case 0x0500/4: case 0x0504/4: case 0x0508/4: case 0x050c/4:
	case 0x0510/4: case 0x0514/4: case 0x0518/4: case 0x051c/4:
	case 0x0520/4: case 0x0524/4: case 0x0528/4: case 0x052c/4:
	case 0x0530/4: case 0x0534/4: case 0x0538/4: case 0x053c/4:
	case 0x0540/4: case 0x0544/4: case 0x0548/4: case 0x054c/4:
	case 0x0550/4: case 0x0554/4: case 0x0558/4: case 0x055c/4:
	case 0x0560/4: case 0x0564/4: case 0x0568/4: case 0x056c/4:
	case 0x0570/4: case 0x0574/4: case 0x0578/4: case 0x057c/4:
	case 0x0580/4: case 0x0584/4: case 0x0588/4: case 0x058c/4:
	case 0x0590/4: case 0x0594/4: case 0x0598/4: case 0x059c/4:
	case 0x05a0/4: case 0x05a4/4: case 0x05a8/4: case 0x05ac/4:
	case 0x05b0/4: case 0x05b4/4: case 0x05b8/4: case 0x05bc/4:
	case 0x05c0/4: case 0x05c4/4: case 0x05c8/4: case 0x05cc/4:
	case 0x05d0/4: case 0x05d4/4: case 0x05d8/4: case 0x05dc/4:
	case 0x05e0/4: case 0x05e4/4: case 0x05e8/4: case 0x05ec/4:
	case 0x05f0/4: case 0x05f4/4: case 0x05f8/4: case 0x05fc/4:
		madam.dma[(offset/4) & 0x1f][offset & 0x03] = data;
		return;

	/* Hardware multiplier */
	case 0x0600/4: case 0x0604/4: case 0x0608/4: case 0x060c/4:
	case 0x0610/4: case 0x0614/4: case 0x0618/4: case 0x061c/4:
	case 0x0620/4: case 0x0624/4: case 0x0628/4: case 0x062c/4:
	case 0x0630/4: case 0x0634/4: case 0x0638/4: case 0x063c/4:
	case 0x0640/4: case 0x0644/4: case 0x0648/4: case 0x064c/4:
	case 0x0650/4: case 0x0654/4: case 0x0658/4: case 0x065c/4:
	case 0x0660/4: case 0x0664/4: case 0x0668/4: case 0x066c/4:
	case 0x0670/4: case 0x0674/4: case 0x0678/4: case 0x067c/4:
	case 0x0680/4: case 0x0684/4: case 0x0688/4: case 0x068c/4:
	case 0x0690/4: case 0x0694/4: case 0x0698/4: case 0x069c/4:
		madam.mult[offset & 0xff] = data;
	case 0x07f0/4:
		madam.mult_control |= data;
		break;
	case 0x07f4/4:
		madam.mult_control &= ~data;
		break;
	case 0x07fc/4:	/* Start process */
		break;

	default:
		logerror( "%08X: unhandled MADAM write offset = %08X, data = %08X, mask = %08X\n", cpu_get_pc(space->machine->device("maincpu")), offset*4, data, mem_mask );
		break;
	}
}


void _3do_madam_init( running_machine *machine )
{
	memset( &madam, 0, sizeof(MADAM) );
	madam.revision = 0x01020000;
}


READ32_HANDLER( _3do_clio_r )
{
	//logerror( "%08X: CLIO read offset = %08X\n", cpu_get_pc(space->machine->device("maincpu")), offset * 4 );

	switch( offset )
	{
	case 0x0000/4:
		return clio.revision;
	case 0x0020/4:
		return clio.audin;
	case 0x0024/4:
		return clio.audout;
	case 0x0028/4:
		return clio.cstatbits;
	case 0x0030/4:
		return clio.hcnt;
	case 0x0034/4:
		{
			static const UINT32 irq_sequence[3] = { 0, 4, 12 };
			static int counter = 0;

			return irq_sequence[(counter++)%3];
		}
		return clio.vcnt;
	case 0x0038/4:
		return clio.seed;
	case 0x003c/4:
		return clio.random;
	case 0x0080/4:
		return clio.hdelay;
	case 0x0084/4:
		return clio.adbio;
	case 0x0088/4:
		return clio.adbctl;

	case 0x0200/4:
		return clio.settm0;
	case 0x0204/4:
		return clio.clrtm0;
	case 0x0208/4:
		return clio.settm1;
	case 0x020c/4:
		return clio.clrtm1;

	case 0x0220/4:
		return clio.slack;

	case 0x0410/4:
		return clio.dipir1;
	case 0x0414/4:
		return clio.dipir2;

	case 0xc000/4:
		return clio.unclerev;
	case 0xc004/4:
		return clio.uncle_soft_rev;
	case 0xc008/4:
		return clio.uncle_addr;
	case 0xc00c/4:
		return clio.uncle_rom;

	default:
		logerror( "%08X: unhandled CLIO read offset = %08X\n", cpu_get_pc(space->machine->device("maincpu")), offset * 4 );
		break;
	}
	return 0;
}

WRITE32_HANDLER( _3do_clio_w )
{
	//logerror( "%08X: CLIO write offset = %08X, data = %08X, mask = %08X\n", cpu_get_pc(space->machine->device("maincpu")), offset*4, data, mem_mask );

	switch( offset )
	{
	case 0x0000/4:
		break;
	case 0x0004/4:
		clio.csysbits = data;
		break;
	case 0x0008/4:
		clio.vint0 = data;
		break;
	case 0x000c/4:
		clio.vint1 = data;
		break;
	case 0x0020/4:
		clio.audin = data;
		break;
	case 0x0024/4:	/* 03400024 - c0020f0f is written here during boot */
		clio.audout = data;
		break;
	case 0x0028/4:	/* 03400028 - bits 0,1, and 6 are tested (reset source) */
		clio.cstatbits = data;
		break;
	case 0x002c/4:	/* 0340002C - ?? during boot 0000000B is written here counter reload related?? */
		clio.wdog = data;
		break;
	case 0x0030/4:
		clio.hcnt = data;
		break;
	case 0x0034/4:
		clio.vcnt = data;
		break;
	case 0x0038/4:
		clio.seed = data;
		break;
	case 0x0040/4:
		clio.irq0 |= data;
		break;
	case 0x0044/4:
		clio.irq0 &= ~data;
		break;
	case 0x0048/4:
		clio.irq0_enable |= data;
		break;
	case 0x004c/4:
		clio.irq0_enable &= ~data;
		break;
	case 0x0050/4:
		clio.mode |= data;
		break;
	case 0x0054/4:
		clio.mode &= ~data;
		break;
	case 0x0058/4:
		clio.badbits = data;
		break;
	case 0x005c/4:
		clio.spare = data;
		break;
	case 0x0060/4:
		clio.irq1 |= data;
		break;
	case 0x0064/4:
		clio.irq1 &= ~data;
		break;
	case 0x0068/4:
		clio.irq1_enable |= data;
		break;
	case 0x006c/4:
		clio.irq1_enable &= ~data;
		break;
	case 0x0080/4:
		clio.hdelay = data;
		break;
	case 0x0084/4:
		clio.adbio = data;
		break;
	case 0x0088/4:
		clio.adbctl = data;
		break;

	case 0x0100/4:
		clio.timer0 = data & 0x0000ffff;
		break;
	case 0x0104/4:
		clio.timerback0 = data & 0x0000ffff;
		break;
	case 0x0108/4:
		clio.timer1 = data & 0x0000ffff;
		break;
	case 0x010c/4:
		clio.timerback1 = data & 0x0000ffff;
		break;
	case 0x0110/4:
		clio.timer2 = data & 0x0000ffff;
		break;
	case 0x0114/4:
		clio.timerback2 = data & 0x0000ffff;
		break;
	case 0x0118/4:
		clio.timer3 = data & 0x0000ffff;
		break;
	case 0x011c/4:
		clio.timerback3 = data & 0x0000ffff;
		break;
	case 0x0120/4:
		clio.timer4 = data & 0x0000ffff;
		break;
	case 0x0124/4:
		clio.timerback4 = data & 0x0000ffff;
		break;
	case 0x0128/4:
		clio.timer5 = data & 0x0000ffff;
		break;
	case 0x012c/4:
		clio.timerback5 = data & 0x0000ffff;
		break;
	case 0x0130/4:
		clio.timer6 = data & 0x0000ffff;
		break;
	case 0x0134/4:
		clio.timerback6 = data & 0x0000ffff;
		break;
	case 0x0138/4:
		clio.timer7 = data & 0x0000ffff;
		break;
	case 0x013c/4:
		clio.timerback7 = data & 0x0000ffff;
		break;
	case 0x0140/4:
		clio.timer8 = data & 0x0000ffff;
		break;
	case 0x0144/4:
		clio.timerback8 = data & 0x0000ffff;
		break;
	case 0x0148/4:
		clio.timer9 = data & 0x0000ffff;
		break;
	case 0x014c/4:
		clio.timerback9 = data & 0x0000ffff;
		break;
	case 0x0150/4:
		clio.timer10 = data & 0x0000ffff;
		break;
	case 0x0154/4:
		clio.timerback10 = data & 0x0000ffff;
		break;
	case 0x0158/4:
		clio.timer11 = data & 0x0000ffff;
		break;
	case 0x015c/4:
		clio.timerback11 = data & 0x0000ffff;
		break;
	case 0x0160/4:
		clio.timer12 = data & 0x0000ffff;
		break;
	case 0x0164/4:
		clio.timerback12 = data & 0x0000ffff;
		break;
	case 0x0168/4:
		clio.timer13 = data & 0x0000ffff;
		break;
	case 0x016c/4:
		clio.timerback13 = data & 0x0000ffff;
		break;
	case 0x0170/4:
		clio.timer14 = data & 0x0000ffff;
		break;
	case 0x0174/4:
		clio.timerback14 = data & 0x0000ffff;
		break;
	case 0x0178/4:
		clio.timer15 = data & 0x0000ffff;
		break;
	case 0x017c/4:
		clio.timerback15 = data & 0x0000ffff;
		break;

	case 0x0200/4:
		clio.settm0 = data;
		break;
	case 0x0204/4:
		clio.clrtm0 = data;
		break;
	case 0x0208/4:
		clio.settm0 = data;
		break;
	case 0x020c/4:
		break;

	case 0x0220/4:
		clio.slack = data & 0x000003ff;
		break;

	case 0x0308/4:
		clio.dmareqdis = data;
		break;

	case 0x0408/4:
		clio.type0_4 = data;
		break;

	case 0xc000/4:
	case 0xc004/4:
	case 0xc00c/4:
		break;
	case 0xc008/4:
		clio.uncle_addr = data;
		break;

	default:
		logerror( "%08X: unhandled CLIO write offset = %08X, data = %08X, mask = %08X\n", cpu_get_pc(space->machine->device("maincpu")), offset*4, data, mem_mask );
		break;
	}
}

void _3do_clio_init( running_machine *machine )
{
	memset( &clio, 0, sizeof(CLIO) );
	clio.revision = 0x02022000 /* 0x04000000 */;
	clio.cstatbits = 0x01;	/* bit 0 = reset of clio caused by power on */
	clio.unclerev = 0x03800000;
}

