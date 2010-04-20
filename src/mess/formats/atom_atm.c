/*********************************************************************

    formats/atom_atm.c

    Quickload code for Acorn Atom atm files

*********************************************************************/

#include "emu.h"
#include "formats/atom_atm.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 1

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    image_fread_memory - read image to memory
-------------------------------------------------*/

static void image_fread_memory(running_device *image, UINT16 addr, UINT32 count)
{
	void *ptr = memory_get_write_ptr(cpu_get_address_space(image->machine->firstcpu, ADDRESS_SPACE_PROGRAM), addr);

	image_fread(image, ptr, count);
}

/*-------------------------------------------------
    QUICKLOAD_LOAD( atom_atm )
-------------------------------------------------*/

QUICKLOAD_LOAD( atom_atm )
{
	/*

		The format for the .ATM files is as follows:

		Offset Size     Description
		------ -------- -----------------------------------------------------------
		0000h  16 BYTEs ATOM filename (if less than 16 BYTEs, rest is 00h bytes)
		0010h  WORD     Start address for load
		0012h  WORD     Execution address
		0014h  WORD     Size of data in BYTEs
		0016h  Size     Data

	*/
	
	UINT8 header[0x16] = { 0 };

	image_fread(image, header, 0x16);

	UINT16 start_address = header[0x10] << 8 | header[0x11];
	UINT16 run_address = header[0x12] << 8 | header[0x13];
	UINT16 size = header[0x14] << 8 | header[0x15];

	if (LOG)
	{
		header[16] = 0;
		logerror("ATM filename: %s\n", header);
		logerror("ATM start address: %04x\n", start_address);
		logerror("ATM run address: %04x\n", run_address);
		logerror("ATM size: %04x\n", size);
	}

	image_fread_memory(image, start_address, size);

	cpu_set_reg(image->machine->firstcpu, REG_GENPC, run_address);

	return INIT_PASS;
}
