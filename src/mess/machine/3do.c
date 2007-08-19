
#include "driver.h"
#include "includes/3do.h"

/*
registers to be located:
- SCOBCTL0 - Spryte engine control word
- REGCTL0  - Modulo for reading source frame buffer data and writing spryte image data
	bit0	G1 = 32 for CFBD read buffer
	bit1	Undefined, set to 0.
	bit2	G1 = 256 for CFBD read buffer
	bit3	G1 = 1024 for CFBD read buffer
	bit4	G2 = 64 for CFBD read buffer
	bit5	G2 = 128 for CFBD read buffer
	bit6	G2 = 256 for CFBD read buffer
	bit7	Undefined, set to 0.
	bit8	G1 = 32 for destination buffer
	bit9	Undefined, set to 0.
	bit10	G1 = 256 for destination buffer
	bit11	G1 = 1024 for destination buffer
	bit12	G2 = 64 for destination buffer
	bit13	G2 = 128 for destination buffer
	bit14	G2 = 256 for destination buffer
	bit15	Undefined, set to 0
- REGCTL1	- X and Y clip values
	bit26-16	- max Y
	bit10-0		- max x
- REGCTL2	- Read base address. Address of the upper left corner pixel of the source frame buffer.
- REGCTL3	- Write base address. Address of the upper left corner pixel of the destination buffer.
- XYPOSH
- XYPOSL
- DXYH
- DXYL
- DDXYH
- DDXYL
- PPMPC
- pixel count register

Spryte engine uses 8 DMA registers:
- 0 = current scob address
- 1 = next scob address
- 2 = pip address
- 3 = spryte base address
- 4 = engine a fetch address
- 5 = engine a length
- 6 = engine b fetch address
- 7 = engine b length

Sprite engine registers:
- SPRSTRT	- Start spryte engine (sets Spryte-ON flipflop)
- SPRSTOP	- Stop spryte engine, everything gets reset.
- SPRPAUS	- Pause spryte engine when it's done with the current spryte (sets pause flipflop).
- SPRCNTU	- Continue after pause of spryte engine.
When interrupt is present, pause flipflop is set
*/

typedef struct {
	UINT32	revision;
	UINT32	memory_configuration;
} MADAM;

typedef struct {
	UINT32	dummy;
} CLIO;

static UINT32	unk_318_data_0 = 0;
static MADAM	madam;
static CLIO		clio;


READ32_HANDLER( nvarea_r ) {
	logerror( "%08X: NVRAM read offset = %08X\n", activecpu_get_pc(), offset );
	return 0;
}

WRITE32_HANDLER( nvarea_w ) {
	logerror( "%08X: NVRAM write offset = %08X, data = %08X, mask = %08X\n", activecpu_get_pc(), offset, data, mem_mask );
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
	logerror( "%08X: UNK_318 read offset = %08X\n", activecpu_get_pc(), offset );
	switch( offset ) {
	case 0:		/* Boot ROM checks here and expects to read 1, 0, 1, 0 in the lowest bit */
		unk_318_data_0 ^= 1;
		return unk_318_data_0;
	}
	return 0;
}

WRITE32_HANDLER( unk_318_w ) {
	logerror( "%08X: UNK_318 write offset = %08X, data = %08X, mask = %08X\n", activecpu_get_pc(), offset, data, mem_mask );
	switch( offset ) {
	case 0:		/* Boot ROM writes 03180000 here and then starts reading some things */
		break;
	}
}


/*
  I think this is the spryte engine - WP
 */
READ32_HANDLER( vram_sport_r ) {
	logerror( "%08X: VRAM SPORT read offset = %08X\n", activecpu_get_pc(), offset );
	switch( offset ) {
	case 0x1840:	/* 03206100 - ?? */
		break;
	case 0x1A40:	/* 03206900 - ?? */
		break;
	}
	return 0;
}

WRITE32_HANDLER( vram_sport_w ) {
	logerror( "%08X: VRAM SPORT write offset = %08X, data = %08X, mask = %08X\n", activecpu_get_pc(), offset, data, mem_mask );
	switch( offset ) {
	case 0x0800:	/* 03202000 - ?? during boot 00190019 gets written */
		break;
	case 0x1180:	/* 03204600-0320472c - ?? during boot FFFFFFFF gets written to this block */
		break;
	}
}



READ32_HANDLER( madam_r ) {
	logerror( "%08X: MADAM read offset = %08X\n", activecpu_get_pc(), offset );
	switch( offset ) {
	case 0:		/* 03300000 - Revision */
		return madam.revision;
	case 1:		/* 03300004 - Memory configuration */
		return madam.memory_configuration;
	}
	return 0;
}

WRITE32_HANDLER( madam_w ) {
	logerror( "%08X: MADAM write offset = %08X, data = %08X, mask = %08X\n", activecpu_get_pc(), offset, data, mem_mask );
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

void madam_init( void ) {
	memset( &madam, 0, sizeof(MADAM) );
	madam.revision = 0x00000001;
}


READ32_HANDLER( clio_r ) {
	logerror( "%08X: CLIO read offset = %08X\n", activecpu_get_pc(), offset );
	return 0;
}

WRITE32_HANDLER( clio_w ) {
	logerror( "%08X: CLIO write offset = %08X, data = %08X, mask = %08X\n", activecpu_get_pc(), offset, data, mem_mask );
	switch( offset ) {
	case 0x0A:	/* 03400028 - bits 0,1, and 6 are tested (irq sources?) */
		break;
	case 0x0B:	/* 0340002C - ?? during boot 0000000B is written here counter reload related?? */
		break;
	}
}

void clio_init( void ) {
	memset( &clio, 0, sizeof(CLIO) );
}

