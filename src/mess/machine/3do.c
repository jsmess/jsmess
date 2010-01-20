
#include "emu.h"
#include "includes/3do.h"

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
} CLIO;


//static UINT32 unk_318_data_0 = 0;
static MADAM	madam;
static CLIO		clio;
static UINT32	svf_color;


READ32_HANDLER( _3do_nvarea_r ) {
	logerror( "%08X: NVRAM read offset = %08X\n", cpu_get_pc(devtag_get_device(space->machine, "maincpu")), offset );
	return 0;
}

WRITE32_HANDLER( _3do_nvarea_w ) {
	logerror( "%08X: NVRAM write offset = %08X, data = %08X, mask = %08X\n", cpu_get_pc(devtag_get_device(space->machine, "maincpu")), offset, data, mem_mask );
}



/*
    I have no idea what piece of hardware this is. Possibly some kind of communication hardware.

    During booting the following things get written/read:
    write 03180000 to 03180000
    4 read actions from 03180000, if lower bit is 1 0 1 0 then
    write 02000000 to 03180000
    write 04000000 to 03180000
    write 08000000 to 03180000
    write 10000000 to 03180000
    write 20000000 to 03180000
    write 40000000 to 03180000
    write 80000000 to 03180000
    write 00000001 to 03180000
    write 00000002 to 03180000
    write 00000004 to 03180000
    write 00000008 to 03180000
    write 00000010 to 03180000
    write 00000020 to 03180000
    write 00000040 to 03180000
    write 00000080 to 03180000
    write 00000100 to 03180000
    read from 03180000
    write 04000000 to 03180000
    write 08000000 to 03180000
    write 10000000 to 03180000
    write 20000000 to 03180000
    write 40000000 to 03180000
    write 80000000 to 03180000
    write 00000001 to 03180000
    write 00000002 to 03180000
    write 00000004 to 03180000
    write 00000008 to 03180000
    write 00000010 to 03180000
    write 00000020 to 03180000
    write 00000040 to 03180000
    write 00000080 to 03180000
    write 00000100 to 03180000
    write 00002000 to 03180000
    several groups of 16 write actions or 16 read actions
*/
READ32_HANDLER( _3do_unk_318_r ) {
	logerror( "%08X: UNK_318 read offset = %08X\n", cpu_get_pc(devtag_get_device(space->machine, "maincpu")), offset );
#if 0
	switch( offset ) {
	case 0:		/* Boot ROM checks here and expects to read 1, 0, 1, 0 in the lowest bit */
		unk_318_data_0 ^= 1;
		return unk_318_data_0;
	}
#endif
	return 0;
}

WRITE32_HANDLER( _3do_unk_318_w )
{
	logerror( "%08X: UNK_318 write offset = %08X, data = %08X, mask = %08X\n", cpu_get_pc(devtag_get_device(space->machine, "maincpu")), offset, data, mem_mask );

	switch( offset )
	{
		case 0:		/* Boot ROM writes 03180000 here and then starts reading some things */
		{
			/* disable ROM overlay */
			memory_set_bank(space->machine, "bank1", 0);
		}
		break;
	}
}


READ32_HANDLER( _3do_svf_r )
{
	logerror( "%08X: SVF read offset = %08X\n", cpu_get_pc(devtag_get_device(space->machine, "maincpu")), offset*4 );
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
	logerror( "%08X: SVF write offset = %08X, data = %08X, mask = %08X\n", cpu_get_pc(devtag_get_device(space->machine, "maincpu")), offset*4, data, mem_mask );
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
	logerror( "%08X: MADAM read offset = %08X\n", cpu_get_pc(devtag_get_device(space->machine, "maincpu")), offset );
	switch( offset ) {
	case 0x0000/4:		/* 03300000 - Revision */
		return madam.revision;
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
	}
	return 0;
}


WRITE32_HANDLER( _3do_madam_w ) {
	logerror( "%08X: MADAM write offset = %08X, data = %08X, mask = %08X\n", cpu_get_pc(devtag_get_device(space->machine, "maincpu")), offset, data, mem_mask );
	switch( offset ) {
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
	}
}


void _3do_madam_init( void )
{
	memset( &madam, 0, sizeof(MADAM) );
	madam.revision = 0x01020000;
}


READ32_HANDLER( _3do_clio_r )
{
	logerror( "%08X: CLIO read offset = %08X\n", cpu_get_pc(devtag_get_device(space->machine, "maincpu")), offset );

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
	}
	return 0;
}

WRITE32_HANDLER( _3do_clio_w )
{
	logerror( "%08X: CLIO write offset = %08X, data = %08X, mask = %08X\n", cpu_get_pc(devtag_get_device(space->machine, "maincpu")), offset, data, mem_mask );

	switch( offset )
	{
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
	case 0x0028/4:	/* 03400028 - bits 0,1, and 6 are tested (irq sources?) */
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

	case 0x88:	/* set timer frequency */
		break;
	}
}

void _3do_clio_init( void )
{
	memset( &clio, 0, sizeof(CLIO) );
	clio.revision = 0x02022000;
	clio.cstatbits = 0x40;
}

