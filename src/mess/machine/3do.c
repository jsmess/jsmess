
#include "driver.h"
#include "includes/3do.h"

typedef struct {
	UINT32	revision;
	UINT32	memory_configuration;
} MADAM;


typedef struct {
	UINT32	dummy;
} CLIO;


//static UINT32 unk_318_data_0 = 0;
static MADAM	madam;
static CLIO		clio;


READ32_HANDLER( nvarea_r ) {
	logerror( "%08X: NVRAM read offset = %08X\n", cpu_get_pc(cputag_get_cpu(space->machine, "maincpu")), offset );
	return 0;
}

WRITE32_HANDLER( nvarea_w ) {
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
READ32_HANDLER( unk_318_r ) {
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

WRITE32_HANDLER( unk_318_w )
{
	logerror( "%08X: UNK_318 write offset = %08X, data = %08X, mask = %08X\n", cpu_get_pc(cputag_get_cpu(space->machine, "maincpu")), offset, data, mem_mask );

	switch( offset )
	{
		case 0:		/* Boot ROM writes 03180000 here and then starts reading some things */
		{
			/* disable ROM overlay */
			memory_set_bank(space->machine, 1, 0);
		}
		break;
	}
}



READ32_HANDLER( vram_sport_r ) {
	logerror( "%08X: VRAM SPORT read offset = %08X\n", cpu_get_pc(cputag_get_cpu(space->machine, "maincpu")), offset );
	switch( offset ) {
	case 0x1840:	/* 03206100 - ?? */
		break;
	case 0x1A40:	/* 03206900 - ?? */
		break;
	}
	return 0;
}

WRITE32_HANDLER( vram_sport_w ) {
	logerror( "%08X: VRAM SPORT write offset = %08X, data = %08X, mask = %08X\n", cpu_get_pc(cputag_get_cpu(space->machine, "maincpu")), offset, data, mem_mask );
	switch( offset ) {
	case 0x0800:	/* 03202000 - ?? during boot 00190019 gets written */
		break;
	case 0x1180:	/* 03204600-0320472c - ?? during boot FFFFFFFF gets written to this block */
		break;
	}
}



READ32_HANDLER( madam_r ) {
	logerror( "%08X: MADAM read offset = %08X\n", cpu_get_pc(cputag_get_cpu(space->machine, "maincpu")), offset );
	switch( offset ) {
	case 0:		/* 03300000 - Revision */
		return madam.revision;
	case 1:		/* 03300004 - Memory configuration */
		return madam.memory_configuration;
	}
	return 0;
}


WRITE32_HANDLER( madam_w ) {
	logerror( "%08X: MADAM write offset = %08X, data = %08X, mask = %08X\n", cpu_get_pc(cputag_get_cpu(space->machine, "maincpu")), offset, data, mem_mask );
	switch( offset ) {
	case 0x01:	/* 03300004 - Memory configuration 29 = 2MB DRAM, 1MB VRAM */
		madam.memory_configuration = data;
		break;
	case 0x03:	/* 0330000C - ?? during boot first value written is 00178906 */
		break;
	case 0x08:	/* 03300020 - ?? during boot 00000000 is written here */
		break;
	}
}


void madam_init( void )
{
	memset( &madam, 0, sizeof(MADAM) );
	madam.revision = 0x00000001;
}


READ32_HANDLER( clio_r )
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

WRITE32_HANDLER( clio_w )
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

void clio_init( void )
{
	memset( &clio, 0, sizeof(CLIO) );
}

