
#include "driver.h"
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

	UINT32	pip[16];		/* 03300180-033001bc (W); 03300180-033001fc (R)*/
} MADAM;


typedef struct {
	UINT32	dummy;
} CLIO;


//static UINT32 unk_318_data_0 = 0;
static MADAM	madam;
static CLIO		clio;


READ32_HANDLER( _3do_nvarea_r ) {
	logerror( "%08X: NVRAM read offset = %08X\n", cpu_get_pc(cputag_get_cpu(space->machine, "maincpu")), offset );
	return 0;
}

WRITE32_HANDLER( _3do_nvarea_w ) {
	logerror( "%08X: NVRAM write offset = %08X, data = %08X, mask = %08X\n", cpu_get_pc(cputag_get_cpu(space->machine, "maincpu")), offset, data, mem_mask );
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
	logerror( "%08X: UNK_318 read offset = %08X\n", cpu_get_pc(cputag_get_cpu(space->machine, "maincpu")), offset );
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
	logerror( "%08X: UNK_318 write offset = %08X, data = %08X, mask = %08X\n", cpu_get_pc(cputag_get_cpu(space->machine, "maincpu")), offset, data, mem_mask );

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



READ32_HANDLER( _3do_vram_sport_r ) {
	logerror( "%08X: VRAM SPORT read offset = %08X\n", cpu_get_pc(cputag_get_cpu(space->machine, "maincpu")), offset );
	switch( offset ) {
	case 0x1840:	/* 03206100 - ?? */
		break;
	case 0x1A40:	/* 03206900 - ?? */
		break;
	}
	return 0;
}

WRITE32_HANDLER( _3do_vram_sport_w ) {
	logerror( "%08X: VRAM SPORT write offset = %08X, data = %08X, mask = %08X\n", cpu_get_pc(cputag_get_cpu(space->machine, "maincpu")), offset, data, mem_mask );
	switch( offset ) {
	case 0x0800:	/* 03202000 - ?? during boot 00190019 gets written */
		break;
	case 0x1180:	/* 03204600-0320472c - ?? during boot FFFFFFFF gets written to this block */
		break;
	}
}



READ32_HANDLER( _3do_madam_r ) {
	logerror( "%08X: MADAM read offset = %08X\n", cpu_get_pc(cputag_get_cpu(space->machine, "maincpu")), offset );
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
	case 0x0180/4:
	case 0x0188/4:
	case 0x0190/4:
	case 0x0198/4:
	case 0x01a0/4:
	case 0x01a8/4:
	case 0x01b0/4:
	case 0x01b8/4:
	case 0x01c0/4:
	case 0x01c8/4:
	case 0x01d0/4:
	case 0x01d8/4:
	case 0x01e0/4:
	case 0x01e8/4:
	case 0x01f0/4:
	case 0x01f8/4:
		return madam.pip[(offset/2) & 0x0f] & 0xffff;
	case 0x0184/4:
	case 0x018c/4:
	case 0x0194/4:
	case 0x019c/4:
	case 0x01a4/4:
	case 0x01ac/4:
	case 0x01b4/4:
	case 0x01bc/4:
	case 0x01c4/4:
	case 0x01cc/4:
	case 0x01d4/4:
	case 0x01dc/4:
	case 0x01e4/4:
	case 0x01ec/4:
	case 0x01f4/4:
	case 0x01fc/4:
		return madam.pip[(offset/2) & 0x0f] >> 16;
	}
	return 0;
}


WRITE32_HANDLER( _3do_madam_w ) {
	logerror( "%08X: MADAM write offset = %08X, data = %08X, mask = %08X\n", cpu_get_pc(cputag_get_cpu(space->machine, "maincpu")), offset, data, mem_mask );
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
	case 0x0180/4:
	case 0x0184/4:
	case 0x0188/4:
	case 0x018c/4:
	case 0x0190/4:
	case 0x0194/4:
	case 0x0198/4:
	case 0x019c/4:
	case 0x01a0/4:
	case 0x01a4/4:
	case 0x01a8/4:
	case 0x01ac/4:
	case 0x01b0/4:
	case 0x01b4/4:
	case 0x01b8/4:
	case 0x01bc/4:
		madam.pip[offset & 0x0f] = data;
	}
}


void _3do_madam_init( void )
{
	memset( &madam, 0, sizeof(MADAM) );
	madam.revision = 0x01020000;
}


READ32_HANDLER( _3do_clio_r )
{
	logerror( "%08X: CLIO read offset = %08X\n", cpu_get_pc(cputag_get_cpu(space->machine, "maincpu")), offset );

	switch( offset )
	{
		case 0x0a: return 0x40;
		case 0x0d:
		{
			static const UINT32 irq_sequence[3] = { 0, 4, 12 };
			static int counter = 0;

			return irq_sequence[(counter++)%3];
		}
		break;
	}
	return 0;
}

WRITE32_HANDLER( _3do_clio_w )
{
	logerror( "%08X: CLIO write offset = %08X, data = %08X, mask = %08X\n", cpu_get_pc(cputag_get_cpu(space->machine, "maincpu")), offset, data, mem_mask );

	switch( offset )
	{
		case 0x09:	/* 03400024 - VDL display control word ? c0020f0f is written here during boot */
			break;

		case 0x0A:	/* 03400028 - bits 0,1, and 6 are tested (irq sources?) */
			break;

		case 0x0B:	/* 0340002C - ?? during boot 0000000B is written here counter reload related?? */
			break;

		case 0x88:	/* set timer frequency */
			break;
	}
}

void _3do_clio_init( void )
{
	memset( &clio, 0, sizeof(CLIO) );
}

