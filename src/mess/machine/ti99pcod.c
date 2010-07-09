/*
    TI-99 P-Code Card emulation.
    Michael Zapf, July 2009

    The P-Code card is part of the UCSD p-System support for the TI-99
    computer family. This system is a comprehensive development system for
    creating, running, and debugging programs written in UCSD Pascal.

    The complete system consists of
    - P-Code card, plugged into the Peripheral Expansion Box (PEB)
    - Software on disk:
      + PHD5063: UCSD p-System Compiler
      + PHD5064: UCSD p-System Assembler/Linker
      + PHD5065: UCSD p-System Editor/Filer (2 disks)

    The card has a switch on the circuit board extending outside the PEB
    which allows to turn off the card without removing it. Unlike other
    expansion cards for the TI system, the P-Code card immediately takes
    over control after the system is turned on.

    When the p-System is booted, the screen turns cyan and remains empty.
    There are two beeps, a pause for about 15 seconds, another three beeps,
    and then a welcome text is displayed with a one-line menu at the screen
    top. (Delay times seem unrealistically short; the manual says
    30-60 seconds. To be checked.)
    Many of the functions require one of the disks be inserted in one
    of the disk drives. You can leave the p-System by waiting for the menu
    to appear, and typing H (halt). This returns you to the Master Title
    Screen, and the card is inactive until the system is reset.

    The P-Code card contains the P-Code interpreter which is somewhat
    comparable to today's Java virtual machine. Programs written for the
    p-System are interchangeable between different platforms.

    On the P-Code card we find 12 KiB of ROM, visible in the DSR memory area
    (>4000 to >5FFF). The first 4 KiB (>4000->4FFF) are from the 4732 ROM,
    the second and third 4 KiB (>5000->5FFF) are from a 4764 ROM, switched
    by setting the CRU bit 4 to 1 on the CRU base >1F00.

    CRU base >1F00
        Bit 0: Activate card
        Bit 4: Select bank 2 of the 4764 ROM (0 = bank 1)
        Bit 7: May be connected to an indicator LED which is by default
               wired to bit 0 (on the PCB)

    The lines are used in a slightly uncommon way: the three bits of the
    CRU bit address are A8, A13, and A14 (A15=LSB). Hence, bit 4 is at
    address >1F80, and bit 7 is at address >1F86. These bits are purely
    write-only.

    Moreover, the card contains 48 KiB of GROM memory, occupying the address
    space from G>0000 to G>FFFF in portions of 6KiB at every 8KiB boundary.

    Another specialty of the card is that the GROM contents are accessed via
    another GROM base address than what is used in the console:
    - >5BFC = read GROM data
    - >5BFE = read GROM address
    - >5FFC = write GROM data
    - >5FFE = write GROM address

    This makes the GROM memory "private" to the card; together with the
    rest of the ROM space the ports become invisible when the card is
    deactivated.

    Emulation information:

    The P-Code card switch is mapped to a DIP switch in the configuration
    menu. If the switch is turned on, the system assumes that ROM dumps are
    present called "pcode_r0.bin" (4 KiB), "pcode_r1.bin" (8KiB),
    "pcode_g0.bin" (64 KiB). The switch is turned off by default. Check the
    switch position if the Master Title Screen does not appear as expected.
*/
#include "emu.h"
#include "devices/ti99_peb.h"
#include "ti99_4x.h"
#include "ti99pcod.h"

static const int GROM_base = 0x1bfc;
static int bank_select; // 0 or 1
/*
    Pointer to the pcode ROM data. We have three sections of the ROM
    code: 4K from one chip, 2 banks with 4K each from the second chip.
*/
static UINT8 *ti99_pcode_rom0;
static UINT8 *ti99_pcode_rom1;
static UINT8 *ti99_pcode_rom2;
static UINT8 *ti99_pcode_grom;

static READ8_HANDLER(pcode_mem_r);
static WRITE8_HANDLER(pcode_mem_w);
static int pcode_cru_r(running_machine *machine, int offset);
static void pcode_cru_w(running_machine *machine, int offset, int data);

static running_device *expansion_box;

static const ti99_peb_card_handlers_t pcode_handlers =
{
	pcode_cru_r,
	pcode_cru_w,
	pcode_mem_r,
	pcode_mem_w
};

/*************************************************************************
   Private GROM support
   The P-Code card contains 8 GROMs, spanning the whole GROM address
   space. But since the card uses an own GROM base address, the console
   GROMs are not affected. The GROMs on the P-Code card can only be
   accessed when the card is selected.
   We take the routines from ti99_4x, slightly changing them. (At some later
   time we should possibly model the GROMs as own devices.)
**************************************************************************/

/* descriptor for pcode GROMs */
static GROM_port_t pcode_GROMs;

static UINT8 GROM_dataread(void)
{
	UINT8 reply;
	/* GROMs are buffered. Data is retrieved from a buffer,
    while the buffer is replaced with the next cell
    content. */
	reply = pcode_GROMs.buf;

	/* Get next value, put it in buffer. Note that the
    GROM wraps at 8K boundaries. */
	pcode_GROMs.buf = pcode_GROMs.data_ptr[pcode_GROMs.addr];

	/* The program counter wraps at each GROM chip size (8K),
    so 0x5fff + 1 = 0x4000. */
	pcode_GROMs.addr = ((pcode_GROMs.addr + 1) & 0x1FFF) | (pcode_GROMs.addr & 0xE000);

	/* Reset the read and write address flipflops. */
	pcode_GROMs.raddr_LSB = pcode_GROMs.waddr_LSB = FALSE;

	return reply;
}

/*
    GROM read for P-Code card
    For comments, see the handler for TI-99/4A
*/
static READ8_HANDLER ( ti99_pcode_grom_r )
{
	UINT8 reply;

	cpu_adjust_icount(space->machine->firstcpu,-4);

	if (offset & 2)
	{
		pcode_GROMs.waddr_LSB = FALSE;

		if (pcode_GROMs.raddr_LSB)
		{
			reply = pcode_GROMs.addr & 0x00ff;
			pcode_GROMs.raddr_LSB = FALSE;
		}
		else
		{
			reply = (pcode_GROMs.addr>>8) & 0x00ff;
			pcode_GROMs.raddr_LSB = TRUE;
		}
	}
	else
		reply = GROM_dataread();

	return reply;
}

/*
    GROM write for P-Code card.
    For comments, see the handler for TI-99/4A
*/
static WRITE8_HANDLER ( ti99_pcode_grom_w )
{
	cpu_adjust_icount(space->machine->firstcpu,-4);

	if (offset & 2)
	{
		pcode_GROMs.raddr_LSB = FALSE;
		if (pcode_GROMs.waddr_LSB)
		{
			pcode_GROMs.addr = (pcode_GROMs.addr & 0xFF00) | (data & 0xFF);

			GROM_dataread();
			pcode_GROMs.waddr_LSB = FALSE;
		}
		else
		{
			pcode_GROMs.addr = ((data<<8) & 0xFF00) | (pcode_GROMs.addr & 0xFF);
			pcode_GROMs.waddr_LSB = TRUE;
		}

	}
	else
	{
		GROM_dataread();
		pcode_GROMs.raddr_LSB = pcode_GROMs.waddr_LSB = FALSE;
	}
}

/**************************************************************************/

void ti99_pcode_reset(running_machine *machine)
{
	ti99_pcode_rom0 = memory_region(machine, region_dsr) + offset_pcode_rom;
	ti99_pcode_rom1 = ti99_pcode_rom0 + 0x1000;
	ti99_pcode_rom2 = ti99_pcode_rom1 + 0x1000;
	ti99_pcode_grom = memory_region(machine, region_dsr) + offset_pcode_grom;
	bank_select = 0;
	pcode_GROMs.addr = 0;
	pcode_GROMs.data_ptr = ti99_pcode_grom;

	expansion_box = machine->device("per_exp_box");

	ti99_peb_set_card_handlers(expansion_box, 0x1f00, &pcode_handlers);
}

/*
    The CRU write handler. The CRU is a serial interface in the console.
    Using the CRU we can switch the banks in the p-code card
    Bit 0 = activate card
    Bit 4 = select second bank of high ROM.
    Note that the CRU address is created from address lines A8, A13, and A14
    so bit 0 is at 0x1f00, but bit 4 is at 0x1f80 (bit 7 would be 0x1f86
    but it is not used)
*/
static void pcode_cru_w(running_machine *machine, int offset, int data)
{
	/* offset is the bit number. The CRU base address is already divided
           by 2. */
	if (offset == 0)
	{
//      printf("pcode card state = %d\n", data);
	}
	if (offset == 0x40)  // Bit 4 is on A8
	{
		if (data==0)
			bank_select = 0;
		else
			bank_select = 1;
//      printf("pcode bank select = %d\n", data);
	}
}

/*
    The CRU read handler. The p-code card does not provide a CRU read
    functionality, so we assume that it only returns 0.
*/
static int pcode_cru_r(running_machine *machine, int offset)
{
	return 0;
}

/*
    Read a byte in pcode DSR space
*/
static  READ8_HANDLER( pcode_mem_r )
{
	UINT8 reply;
	if (offset < 0x1000)
	{
		/* Accesses ROM 4732 (4K) */
		reply = ti99_pcode_rom0[offset];
	}
	else
	{
		/* Check if we access the GROM port */
		switch (offset)
		{
			/* GROMs only answer at even addresses. The odd addresses are
            undefined, so we can as well just ignore these cases. */
		case 0x1bfc:
		case 0x1bfe:
			reply = ti99_pcode_grom_r(space, offset);
			break;
		case 0x1ffc:
		case 0x1ffe:
			// Cannot read from this port
			reply = 0;
			break;
		default:
			/* Accesses ROM 4764 (2*4K)
            We have two banks here which are activated according
            to the setting of CRU bit 4
            */
			reply = (bank_select==0)? ti99_pcode_rom1[offset-0x1000] : ti99_pcode_rom2[offset-0x1000];
		}
	}
	return reply;
}

/*
    Write a byte in P-Code ROM space. This is only useful for setting the
    GROM address.
*/
static WRITE8_HANDLER( pcode_mem_w )
{
	/* Check if we access the GROM "set address" port */
	if (offset == 0x1ffe)
	{
		ti99_pcode_grom_w(space, offset, data);
	}
}

