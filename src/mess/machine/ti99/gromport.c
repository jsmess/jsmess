/**********************************************************************

    Gromport: the cartridge port of the TI-99/4, TI-99/4A, and
    TI-99/8 console. The name refers to the main intended application
    scenario, that is, to host cartridges with GROMs. (The other port
    of the console is called I/O port.)

    Over the years, many different kinds of cartridges appeared; the
    usual cartridge layout contains GROMs and possibly a ROM circuit
    for time-critical code (since the GROMs were utterly slow).

    Texas Instruments kept control over all software development by
    reserving GROMs and their programming to them and licensees.

    Interestingly, the OS designers planned for an alternative -
    cartridges which can be run with EPROMs only - which provided
    a means for third-party authors to create cartridges without
    TI license. The last action of TI before their retreat from the
    home computer market was to disable this alternative.

    Nevertheless, the majority of consoles runs with different kinds of
    cartridges; most recent developments by third party include
    multi-bank cartridges with storage space up to 128 KiB.

    The implementation as shown here goes beyond the simple port
    by realizing a "multi-cartridge extender". This is a somewhat
    mythical device which was never available for the normal
    customer, but there are reports of the existance of such a device
    in development labs or demonstrations.

    The interesting thing about this is that the OS of the console
    fully supports this multi-cartridge extender, providing a selection
    option on the screen to switch between different plugged-in
    cartridges.

    The switching is possible by decoding address lines that are reserved
    for GROM access. GROMs are accessed via four separate addresses
    9800, 9802, 9C00, 9C02. The addressing scheme looks like this:

    1001 1Wxx xxxx xxM0        W = write(1), read(0), M = address(1), data(0)

    This leaves 8 bits (256 options) which are not decoded inside the
    console. As the complete address is routed to the port, some circuit
    just needs to decode the xxx lines and turn on the respective slot.

    One catch must be considered: Some cartridges contain ROMs which are
    directly accessed and not via ports. This means that the ROMs must
    be activated according to the slot that is selected.

    Another issue: Each GROM contains an own address counter and an ID.
    According to the ID the GROM only delivers data if the address counter
    is within the ID area (0 = 0000-1fff, 1=2000-3fff ... 7=e000-ffff).
    Thus it is essential that all GROMs stay in sync with their address
    counters. We have to route all address settings to all slots and their
    GROMs, even when the slot has not been selected before. The selected
    just shows its effect when data is read. In this case, only the
    data from the selected slot will be delivered.

    This may be considered as a design flaw within the complete cartridge system
    which eventually led to TI not manufacturing that device for the broad
    market.

    --------

    This implementation also contains the GRAMKracker, a popular device
    with persistent RAM and a cartridge slot. With it, cartridge contents
    can be read, stored in its RAM, and modified. Moreover, the
    console GROMs can be loaded into the device and overrun the data
    delivered by the console GROMs. (Take care when changing something
    in this emulation - this overrun is emulated by the sequence in which
    the devices on the datamux are executed.)

    --------

    The second part of this file contains the implementation of the
    cartridge image format (RPK).

    Michael Zapf, 2012

*********************************************************************/

#include "gromport.h"
#include "ti99defs.h"

#define VERBOSE 1
#define LOG logerror

#define GROM_AREA 0x9800
#define GROM_MASK 0xf800
#define AUTO -1

#define CARTGROM_TAG "grom_contents"
#define CARTROM_TAG "rom_contents"
#define CARTROM2_TAG "rom2_contents"

#define GROM3_TAG "grom3"
#define GROM4_TAG "grom4"
#define GROM5_TAG "grom5"
#define GROM6_TAG "grom6"
#define GROM7_TAG "grom7"

enum
{
	PCB_STANDARD=1,
	PCB_PAGED,
	PCB_MINIMEM,
	PCB_SUPER,
	PCB_MBX,
	PCB_PAGED379I,
	PCB_PAGEDCRU,
	PCB_GKRACKER
};

#define GKSWITCH1_TAG "GKSWITCH1"
#define GKSWITCH2_TAG "GKSWITCH2"
#define GKSWITCH3_TAG "GKSWITCH3"
#define GKSWITCH4_TAG "GKSWITCH4"
#define GKSWITCH5_TAG "GKSWITCH5"

/*******************************************
    Cartridge PCB types
*******************************************/

cartridge_pcb_device::cartridge_pcb_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock)
: bus8z_device(mconfig, type, name, tag, owner, clock)
{
	if (VERBOSE>7) LOG("cartridge_pcb_device: common initialization\n");
	for (int i=0; i < 5; i++) m_grom[i] = 0;
	m_rom_ptr = 0;
	m_rom2_ptr = 0;
	m_ram_ptr = 0;
	m_grom_size = 0;
	m_rom_size = 0;
	m_ram_size = 0;
}

void cartridge_pcb_device::device_start(void)
{
}

cartridge_pcb_standard_device::cartridge_pcb_standard_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
: cartridge_pcb_device(mconfig, TICARTPCBSTD, "Standard cartridge PCB", tag, owner, clock)
{
}

cartridge_pcb_paged_device::cartridge_pcb_paged_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
: cartridge_pcb_device(mconfig, TICARTPCBPAG, "Paged cartridge PCB", tag, owner, clock)
{
}

cartridge_pcb_minimem_device::cartridge_pcb_minimem_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
: cartridge_pcb_device(mconfig, TICARTPCBMIN, "MiniMemory cartridge PCB", tag, owner, clock)
{
}

cartridge_pcb_superspace_device::cartridge_pcb_superspace_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
: cartridge_pcb_device(mconfig, TICARTPCBMIN, "SuperSpace II cartridge PCB", tag, owner, clock)
{
}

cartridge_pcb_mbx_device::cartridge_pcb_mbx_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
: cartridge_pcb_device(mconfig, TICARTPCBMIN, "MBX cartridge PCB", tag, owner, clock)
{
}

cartridge_pcb_paged379i_device::cartridge_pcb_paged379i_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
: cartridge_pcb_device(mconfig, TICARTPCBPGI, "Paged379i cartridge PCB", tag, owner, clock)
{
	m_rom_page = get_paged379i_bank(15);
}

cartridge_pcb_pagedcru_device::cartridge_pcb_pagedcru_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
: cartridge_pcb_device(mconfig, TICARTPCBPGC, "PagedCRU cartridge PCB", tag, owner, clock)
{
}

cartridge_pcb_gramkracker_device::cartridge_pcb_gramkracker_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
: cartridge_pcb_device(mconfig, TICARTPCBGRK, "GRAM Kracker cartridge PCB", tag, owner, clock)
{
	m_grom_address = 0;	// for the GROM emulation
	for (int i=1; i < 6; i++) m_gk_switch[i] = 0;
}

/*****************************************************************************
    Common methods.
******************************************************************************/

/* Read function for this cartridge, CRU. */
void cartridge_pcb_device::crureadz(offs_t offset, UINT8 *value) {  };

/* Write function for this cartridge, CRU. */
void cartridge_pcb_device::cruwrite(offs_t offset, UINT8 value) {  };

READ8Z_MEMBER( cartridge_pcb_device::gromreadz )
{
	for (int i=0; i < 5; i++)
	{
		if (m_grom[i] != NULL)
		{
			m_grom[i]->readz(space, offset, value, mem_mask);
		}
	}
}

WRITE8_MEMBER( cartridge_pcb_device::gromwrite )
{
	for (int i=0; i < 5; i++)
	{
		if (m_grom[i] != NULL)
		{
			m_grom[i]->write(space, offset, data, mem_mask);
		}
	}
}

/*****************************************************************************
  Cartridge type: Standard
    Most cartridges are built in this type. Every cartridge may contain
    GROM between 0 and 40 KiB, and ROM with 8 KiB length, no banking.
******************************************************************************/

/* Read function for the standard cartridge. */
READ8Z_MEMBER(cartridge_pcb_standard_device::readz)
{
	if ((offset & GROM_MASK)==GROM_AREA)
		gromreadz(space, offset, value, mem_mask);
	else
	{
		if (m_rom_ptr!=NULL)
		{
			// For TI-99/8 we should plan for 16K cartridges. However, none was ever produced.
			// Well, forget about that.
			*value = m_rom_ptr[offset & 0x1fff];
			//      LOG("read cartridge rom space %04x = %02x\n", offset, *value);
		}
	}
}

/* Write function for the standard cartridge. */
WRITE8_MEMBER(cartridge_pcb_standard_device::write)
{
	// LOG("write standard\n");
	if ((offset & GROM_MASK)==GROM_AREA)
		gromwrite(space, offset, data, mem_mask);
	else
	{
		if (VERBOSE>5) LOG("cartridge_pcb_device: Cannot write to ROM space at %04x\n", offset);
	}
}

/*****************************************************************************
  Cartridge type: Paged (Extended Basic)
    This cartridge consists of GROM memory and 2 pages of standard ROM.
    The page is set by writing any value to a location in
    the address area, where an even word offset sets the page to 0 and an
    odd word offset sets the page to 1 (e.g. 6000 = bank 0, and
    6002 = bank 1).
******************************************************************************/

/* Read function for the paged cartridge. */
READ8Z_MEMBER(cartridge_pcb_paged_device::readz)
{
	if ((offset & GROM_MASK)==GROM_AREA)
		gromreadz(space, offset, value, mem_mask);
	else
	{
		if (m_rom_page==0)
		{
			*value = m_rom_ptr[offset & 0x1fff];
		}
		else
		{
			*value = m_rom2_ptr[offset & 0x1fff];
		}
	}
}

/* Write function for the paged cartridge. */
WRITE8_MEMBER(cartridge_pcb_paged_device::write)
{
	// LOG("write standard\n");
	if ((offset & GROM_MASK)==GROM_AREA)
		gromwrite(space, offset, data, mem_mask);

	else
		m_rom_page = (offset >> 1) & 1;
}


/*****************************************************************************
  Cartridge type: Mini Memory
    GROM: 6 KiB (occupies G>6000 to G>7800)
    ROM: 4 KiB (romfile is actually 8 K long, second half with zeros, 0x6000-0x6fff)
    persistent RAM: 4 KiB (0x7000-0x7fff)
******************************************************************************/

/* Read function for the minimem cartridge. */
READ8Z_MEMBER(cartridge_pcb_minimem_device::readz)
{
	if ((offset & GROM_MASK)==GROM_AREA)
		gromreadz(space, offset, value, mem_mask);

	else
	{
		if ((offset & 0x1000)==0x0000)
		{
			*value = m_rom_ptr[offset & 0x0fff];
		}
		else
		{
			*value = m_ram_ptr[offset & 0x0fff];
		}
	}
}

/* Write function for the minimem cartridge. */
WRITE8_MEMBER(cartridge_pcb_minimem_device::write)
{
	// LOG("write standard\n");
	if ((offset & GROM_MASK)==GROM_AREA)
		gromwrite(space, offset, data, mem_mask);

	else
	{
		if ((offset & 0x1000)==0x0000)
		{
			if (VERBOSE>1) LOG("ti99: gromport: Write access to cartridge ROM at address %04x ignored", offset);
		}
		else
		{
			m_ram_ptr[offset & 0x0fff] = data;
		}
	}
}

/*****************************************************************************
  Cartridge type: SuperSpace II
    SuperSpace cartridge type. SuperSpace is intended as a user-definable
    blank cartridge containing buffered RAM
    It has an Editor/Assembler GROM which helps the user to load
    the user program into the cartridge. If the user program has a suitable
    header, the console recognizes the cartridge as runnable, and
    assigns a number in the selection screen.
    Switching the RAM banks in this cartridge is achieved by setting
    CRU bits (the system serial interface).

    GROM: Editor/Assembler GROM
    ROM: none
    persistent RAM: 32 KiB (0x6000-0x7fff, 4 banks)
    Switching the bank is done via CRU write
******************************************************************************/

/* Read function for the super cartridge. */
READ8Z_MEMBER(cartridge_pcb_superspace_device::readz)
{
	if ((offset & GROM_MASK)==GROM_AREA)
		gromreadz(space, offset, value, mem_mask);
	else
	{
		if (m_ram_ptr==NULL)
		{
			*value = 0;
		}
		else
		{
			offs_t boffset = (m_ram_page << 13) | (offset & 0x1fff);
			*value = m_ram_ptr[boffset];
		}
	}
}

/* Write function for the super cartridge. */
WRITE8_MEMBER(cartridge_pcb_superspace_device::write)
{
	if ((offset & GROM_MASK)==GROM_AREA)
		gromwrite(space, offset, data, mem_mask);
	else
	{
		if ((offset & 0x1000)==0x0000)
		{
			if (VERBOSE>1) LOG("ti99: gromport: Write access to cartridge ROM at address %04x ignored", offset);
		}
		else
		{
			offs_t boffset = (m_ram_page << 13) | (offset & 0x1fff);
			m_ram_ptr[boffset] = data;
		}
	}
}

void cartridge_pcb_superspace_device::crureadz(offs_t offset, UINT8 *value)
{
	// offset is the bit number. The CRU base address is already divided  by 2.

	// ram_page contains the bank number. We have a maximum of
	// 4 banks; the Super Space II manual says:
	//
	// Banks are selected by writing a bit pattern to CRU address >0800:
	//
	// Bank #   Value
	// 0        >02  = 0000 0010
	// 1        >08  = 0000 1000
	// 2        >20  = 0010 0000
	// 3        >80  = 1000 0000
	//
	// With the bank number (0, 1, 2, or 3) in R0:
	//
	// BNKSW   LI    R12,>0800   Set CRU address
	//         LI    R1,2        Load Shift Bit
	//         SLA   R0,1        Align Bank Number
	//         JEQ   BNKS1       Skip shift if Bank 0
	//         SLA   R1,0        Align Shift Bit
	// BNKS1   LDCR  R1,0        Switch Banks
	//         SRL   R0,1        Restore Bank Number (optional)
	//         RT

	// Our implementation in MESS always gets 8 bits in one go. Also, the address
	// is twice the bit number. That is, the offset value is always a multiple
	// of 0x10.

	if (VERBOSE>2) LOG("ti99: gromport: Superspace: CRU accessed at %04x\n", offset);
	if ((offset & 0xfff0) != 0x0800)
		*value = 0;

	// CRU addresses are only 1 bit wide. Bytes are transferred from LSB
	// to MSB. That is, 8 bit are eight consecutive addresses. */
	*value = (m_ram_page == (offset-1)/2);
}

void cartridge_pcb_superspace_device::cruwrite(offs_t offset, UINT8 data)
{
	// data is bit
	// offset is address
	if (VERBOSE>2) LOG("ti99: gromport: Superspace: CRU accessed at %04x\n", offset);

	if (offset < 16)
	{
		if (data != 0)
			m_ram_page = (offset-2)/4;
	}
}

/*****************************************************************************
  Cartridge type: MBX
    GROM: up to 40 KiB
    ROM: up to 16 KiB (in up to 2 banks of 8KiB each)
    RAM: 1022 B (0x6c00-0x6ffd, overrides ROM in that area)
    ROM mapper: 6ffe

    Note that some MBX cartridges assume the presence of the MBX system
    (additional console) and will not run without it. The MBX hardware is
    not emulated yet.
******************************************************************************/

/* Read function for the mbx cartridge. */
READ8Z_MEMBER(cartridge_pcb_mbx_device::readz)
{
	if ((offset & GROM_MASK)==GROM_AREA)
		gromreadz(space, offset, value, mem_mask);
	else
	{
		if ((offset & 0x1c00)==0x0c00)
		{
			// This is the RAM area which overrides any ROM. There is no
			// known banking behavior for the RAM, so we must assume that
			// there is only one bank.
			if (m_ram_ptr != NULL)
				*value = m_ram_ptr[offset & 0x03ff];
		}
		else
		{
			*value = m_rom_ptr[(offset & 0x1fff) | (m_rom_page<<13)];
		}
	}
}

/* Write function for the super cartridge. */
WRITE8_MEMBER(cartridge_pcb_mbx_device::write)
{
	if ((offset & GROM_MASK)==GROM_AREA)
		gromwrite(space, offset, data, mem_mask);
	else
	{
		if (offset == 0x6ffe)
		{
			m_rom_page = data & 1;
			return;
		}

		if ((offset & 0x1c00)==0x0c00)
		{
			if (m_ram_ptr == NULL) return;
			m_ram_ptr[offset & 0x03ff] = data;
		}
	}
}

/*****************************************************************************
  Cartridge type: paged379i
    This cartridge consists of one 16 KiB, 32 KiB, 64 KiB, or 128 KiB EEPROM
    which is organised in 2, 4, 8, or 16 pages of 8 KiB each. The complete
    memory contents must be stored in one dump file.
    The pages are selected by writing a value to some memory locations. Due to
    using the inverted outputs of the LS379 latch, setting the inputs of the
    latch to all 0 selects the highest bank, while setting to all 1 selects the
    lowest. There are some cartridges (16 KiB) which are using this scheme, and
    there are new hardware developments mainly relying on this scheme.

    Writing to       selects page (16K/32K/64K/128K)
    >6000            1 / 3 / 7 / 15
    >6002            0 / 2 / 6 / 14
    >6004            1 / 1 / 5 / 13
    >6006            0 / 0 / 4 / 12
    >6008            1 / 3 / 3 / 11
    >600A            0 / 2 / 2 / 10
    >600C            1 / 1 / 1 / 9
    >600E            0 / 0 / 0 / 8
    >6010            1 / 3 / 7 / 7
    >6012            0 / 2 / 6 / 6
    >6014            1 / 1 / 5 / 5
    >6016            0 / 0 / 4 / 4
    >6018            1 / 3 / 3 / 3
    >601A            0 / 2 / 2 / 2
    >601C            1 / 1 / 1 / 1
    >601E            0 / 0 / 0 / 0

    The paged379i cartrige does not have any GROMs.
******************************************************************************/

/*
    Determines which bank to set, depending on the size of the ROM. This is
    some magic code that actually represents different PCB versions.
*/
int cartridge_pcb_paged379i_device::get_paged379i_bank(int rompage)
{
	int mask = 0;
	if (m_rom_size > 16384)
	{
		if (m_rom_size > 32768)
		{
			if (m_rom_size > 65536)
				mask = 15;
			else
				mask = 7;
		}
		else
			mask = 3;
	}
	else
		mask = 1;

	return rompage & mask;
}


/* Read function for the paged379i cartridge. */
READ8Z_MEMBER(cartridge_pcb_paged379i_device::readz)
{
	if ((offset & 0xe000)==0x6000)
		*value = m_rom_ptr[(m_rom_page<<13) | (offset & 0x1fff)];
}

/* Write function for the paged379i cartridge. Only used to set the bank. */
WRITE8_MEMBER(cartridge_pcb_paged379i_device::write)
{
	// Bits: 0110 0000 000b bbbx
	// x = don't care, bbbb = bank
	if ((offset & 0xffe0)==0x6000)
	{
		// Set bank
		m_rom_page = get_paged379i_bank(15 - ((offset>>1) & 15));
	}
}

/*****************************************************************************
  Cartridge type: pagedcru
    This cartridge consists of one 16 KiB, 32 KiB, or 64 KiB EEPROM which is
    organised in 2, 4, or 8 pages of 8 KiB each. We assume there is only one
    dump file of the respective size.
    The pages are selected by writing a value to the CRU. This scheme is
    similar to the one used for the SuperSpace cartridge, with the exception
    that we are using ROM only, and we can have up to 8 pages.

    Bank     Value written to CRU>0800
    0      >0002  = 0000 0000 0000 0010
    1      >0008  = 0000 0000 0000 1000
    2      >0020  = 0000 0000 0010 0000
    3      >0080  = 0000 0000 1000 0000
    4      >0200  = 0000 0010 0000 0000
    5      >0800  = 0000 1000 0000 0000
    6      >2000  = 0010 0000 0000 0000
    7      >8000  = 1000 0000 0000 0000

    No GROMs used in this type.
******************************************************************************/

/* Read function for the pagedcru cartridge. */
READ8Z_MEMBER(cartridge_pcb_pagedcru_device::readz)
{
	if ((offset & 0xe000)==0x6000)
		*value = m_rom_ptr[(m_rom_page<<13) | (offset & 0x1fff)];
}

/* Write function for the pagedcru cartridge. No effect. */
WRITE8_MEMBER(cartridge_pcb_pagedcru_device::write)
{
	return;
}

void cartridge_pcb_pagedcru_device::crureadz(offs_t offset, UINT8 *value)
{
	int page = m_rom_page;
	if ((offset & 0xf800)==0x0800)
	{
		int bit = (offset & 0x001e)>>1;
		if (bit != 0)
		{
			page = page-(bit/2);  // 4 page flags per 8 bits
		}
		*value = 1 << (page*2+1);
	}
}

void cartridge_pcb_pagedcru_device::cruwrite(offs_t offset, UINT8 data)
{
	if ((offset & 0xf800)==0x0800)
	{
		int bit = (offset & 0x001e)>>1;
		if (data != 0 && bit > 0)
		{
			m_rom_page = (bit-1)/2;
		}
	}
}

/*****************************************************************************
    Cartridge type: GRAMKracker

    The GK is a big cartridge that fits into the same slot as any other
    cartridge but which offers a set of switches, an own cartridge slot,
    and buffered RAM inside. Also, it has a built-in ROM called "LOADER".

    Its primary use is to allow the user to copy cartridge contents into the
    buffered RAM and to modify it there. That way, ROM cartridges can be
    customized.

    The 80 KiB of the GK are allocated as follows
    00000 - 01fff: GRAM 0
    02000 - 03fff: GRAM 1  (alternatively, the loader GROM appears here)
    04000 - 05fff: GRAM 2
    06000 - 0ffff: GRAM 3-7 (alternatively, the guest GROMs appear here)
    10000 - 11fff: RAM 1
    12000 - 13fff: RAM 2

    The GK emulates GROMs, but without a readable address counter. Thus it
    relies on the console GROMs being still in place.
    Also, the GK does not implement a GROM buffer.

    We need to implement a complete second path of handling as it differs
    from the multi-cart extender. Eventually, the multi-cart extender and
    the GK should be implemented as separate slot devices.

    The GK cartridge type is in fact the internal LOADER ROM. In order to use
    the GK, the cartridge must be plugged into one slot, and the "guest"
    cartridge of the GK must be plugged into any other slot. Also, the DIP
    switch of the multi-cart extender must be set to "GRAM Kracker".
*****************************************************************************/

enum
{
	CART_AUTO = 0,
	CART_1,
	CART_2,
	CART_3,
	CART_4,
	CART_GK = 15
};

enum
{
	GK_OFF = 0,
	GK_NORMAL = 1,
	GK_GRAM0 = 0,
	GK_OPSYS = 1,
	GK_GRAM12 = 0,
	GK_TIBASIC = 1,
	GK_BANK1 = 0,
	GK_WP = 1,
	GK_BANK2 = 2,
	GK_LDON = 0,
	GK_LDOFF = 1
};

void cartridge_pcb_gramkracker_device::set_gk_guest(cartridge_device* guest)
{
	m_gk_guest = guest;
}

/*
    Read function for the GRAMKracker cartridge.
*/
READ8Z_MEMBER(cartridge_pcb_gramkracker_device::readz)
{
	if ((offset & GROM_MASK) == GROM_AREA)
	{
		// Reads from the GRAM space of the GRAM Kracker.
		// m_gk_slot is the slot where the GK module is plugged in
		// m_gk_guest_slot is the slot where the cartridge is plugged in

		// The GK does not have a readable address counter, but the console
		// GROMs will keep our address counter up to date. That is
		// exactly what happens in the real machine.
		// (The console GROMs are not accessed here but directly via the datamux
		// so we can just return without doing anything)
		if ((offset & 0x0002)!=0) return;

		int id = ((m_grom_address & 0xe000)>>13)&0x07;
		switch (id)
		{
		case 0:
			// GRAM 0. Only return a value if switch 2 is in GRAM0 position.
			if (m_gk_switch[2]==GK_GRAM0)
				*value = m_ram_ptr[m_grom_address];
			break;
		case 1:
			// If the loader is turned on, return loader contents.
			if (m_gk_switch[5]==GK_LDON)
			{
				// The only ROM contained in the GK box is the loader
				// Adjust the address
				*value = m_grom_ptr[m_grom_address & 0x1fff];
			}
			else
			{
				// Loader off
				// GRAM 1. Only return a value if switch 3 is in GRAM12 position.
				// Otherwise, the console GROM 1 will respond (not here; it is the grom_device
				// whose output would then not be overwritten)
				if (m_gk_switch[3]==GK_GRAM12)
					*value = m_ram_ptr[m_grom_address];
			}
			break;
		case 2:
			// GRAM 2. Only return a value if switch 3 is in GRAM12 position.
			if (m_gk_switch[3]==GK_GRAM12)
				*value = m_ram_ptr[m_grom_address];
			break;
		default:
			// Cartridge space (0x6000 - 0xffff)
			// When a cartridge is installed, it overrides the GK contents
			// but only if it has GROMs
			bool guest_has_grom = false;

			if (m_gk_guest != NULL)
			{
				guest_has_grom = (m_gk_guest->has_grom());
				// Note that we only have ONE real cartridge and the GK;
				// we need not access all slots.
				if (guest_has_grom)
				{
					m_gk_guest->readz(space, offset, value, mem_mask);	// read from guest
				}
			}
			if (!guest_has_grom && (m_gk_switch[1]==GK_NORMAL))
				*value = m_ram_ptr[m_grom_address];	// use the GK memory
		}

		// The GK GROM emulation does not wrap at 8K boundaries.
		m_grom_address = (m_grom_address + 1) & 0xffff;

		// Reset the write address flipflop.
		m_waddr_LSB = false;
	}
	else
	{
		if (m_gk_guest != NULL)
		{
			// Read from the guest cartridge.
			m_gk_guest->readz(space, offset, value, mem_mask);
		}
		else
		{
			// Reads from the RAM space of the GRAM Kracker.
			if (m_gk_switch[1] == GK_OFF) return; // just don't do anything
			switch (m_gk_switch[4])
			{
			// RAM is stored behind the GRAM area
			case GK_BANK1:
				*value = m_ram_ptr[offset+0x10000 - 0x6000];

			case GK_BANK2:
				*value = m_ram_ptr[offset+0x12000 - 0x6000];

			default:
				// Switch in middle position (WP, implies auto-select according to the page flag)
				if (m_ram_page==0)
					*value = m_ram_ptr[offset+0x10000 - 0x6000];
				else
					*value = m_ram_ptr[offset+0x12000 - 0x6000];
			}
		}
	}
}

/*
    See above.
*/
WRITE8_MEMBER(cartridge_pcb_gramkracker_device::write)
{
	// write to the guest cartridge if present
	if (m_gk_guest != NULL)
	{
		m_gk_guest->write(space, offset, data, mem_mask);
	}

	if ((offset & GROM_MASK) == GROM_AREA)
	{
		// Writes to the GRAM space of the GRAM Kracker.
		if ((offset & 0x0002)==0x0002)
		{
			// Set address
			if (m_waddr_LSB == true)
			{
				// Accept low address byte (second write)
				m_grom_address = (m_grom_address & 0xff00) | data;
				m_waddr_LSB = false;
			}
			else
			{
				// Accept high address byte (first write)
				m_grom_address = (m_grom_address & 0x00ff) | (data << 8);
				m_waddr_LSB = true;
			}
		}
		else
		{
			// Write data byte to GRAM area.

			// According to manual:
			// Writing to GRAM 0: switch 2 set to GRAM 0 + Write protect switch (4) in 1 or 2 position
			// Writing to GRAM 1: switch 3 set to GRAM 1-2 + Loader off (5); write prot has no effect
			// Writing to GRAM 2: switch 3 set to GRAM 1-2 (write prot has no effect)
			// Writing to GRAM 3-7: switch 1 set to GK_NORMAL, no cartridge inserted
			// GK_NORMAL switch has no effect on GRAM 0-2
			int id = ((m_grom_address & 0xe000)>>13)&0x07;
			switch (id)
			{
			case 0:
				if (m_gk_switch[2]==GK_GRAM0 && m_gk_switch[4]!=GK_WP)
					m_ram_ptr[m_grom_address] = data;
				break;
			case 1:
				if (m_gk_switch[3]==GK_GRAM12 && m_gk_switch[5]==GK_LDOFF)
					m_ram_ptr[m_grom_address] = data;
				break;
			case 2:
				if (m_gk_switch[3]==GK_GRAM12)
					m_ram_ptr[m_grom_address] = data;
				break;
			default:
				if (m_gk_switch[1]==GK_NORMAL && m_gk_guest != NULL)
					m_ram_ptr[m_grom_address] = data;
				break;
			}
			// The GK GROM emulation does not wrap at 8K boundaries.
			m_grom_address = (m_grom_address + 1) & 0xffff;

			// Reset the write address flipflop.
			m_waddr_LSB = false;
		}
	}
	else
	{
		// Write to the RAM space of the GRAM Kracker.
		if (m_gk_guest == NULL)
		{
			if (m_gk_switch[1] == GK_OFF) return; // just don't do anything
			switch (m_gk_switch[4])
			{
			// RAM is stored behind the GRAM area
			case GK_BANK1:
				m_ram_ptr[offset+0x10000 - 0x6000] = data;
				break;

			case GK_BANK2:
				m_ram_ptr[offset+0x12000 - 0x6000] = data;
				break;

			default:
				// Switch in middle position (WP, implies auto-select according to the page flag)
				// This is handled like in Extended Basic (using addresses)
				m_ram_page = (offset >> 1) & 1;
				break;
			}
		}
	}
}

/*
    Read cartridge mapper CRU interface (Guest in GRAM Kracker)
*/
void cartridge_pcb_gramkracker_device::crureadz(offs_t offset, UINT8 *value)
{
	if (m_gk_guest != NULL)	m_gk_guest->crureadz(offset, value);
}

/*
    Write cartridge mapper CRU interface (Guest in GRAM Kracker)
*/
void cartridge_pcb_gramkracker_device::cruwrite(offs_t offset, UINT8 data)
{
	if (m_gk_guest != NULL) m_gk_guest->cruwrite(offset, data);
}

/*****************************************************************************
    A cartridge. In fact, just the empty cartridge, as the image to be loaded
    will be included as the PCB.
*****************************************************************************/

static const pcb_type pcbdefs[] =
{
	{ PCB_STANDARD, "standard" },
	{ PCB_PAGED, "paged" },
	{ PCB_MINIMEM, "minimem" },
	{ PCB_SUPER, "super" },
	{ PCB_MBX, "mbx" },
	{ PCB_PAGED379I, "paged379i" },
	{ PCB_PAGEDCRU, "pagedcru" },
	{ PCB_GKRACKER, "gramkracker" },
	{ 0, NULL}
};

cartridge_device::cartridge_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
: bus8z_device(mconfig, TICARTRIDGE, "TI-99 cartridge", tag, owner, clock),
  device_image_interface(mconfig, *this),
  m_pcb(NULL)
{
	m_shortname = "cartridge";
}

void cartridge_device::device_start()
{
	if (VERBOSE>7) LOG("cartridge_device: device_start\n");
}

void cartridge_device::device_stop()
{
	if (VERBOSE>7) LOG("cartridge_device: device_stop\n");
}

void cartridge_device::device_reset()
{
	if (m_type==PCB_GKRACKER)
	{
		cartridge_pcb_gramkracker_device* pcb = static_cast<cartridge_pcb_gramkracker_device*>(m_pcb);
		pcb->m_gk_switch[1] = input_port_read(*owner(), GKSWITCH1_TAG);
		pcb->m_gk_switch[2] = input_port_read(*owner(), GKSWITCH2_TAG);
		pcb->m_gk_switch[3] = input_port_read(*owner(), GKSWITCH3_TAG);
		pcb->m_gk_switch[4] = input_port_read(*owner(), GKSWITCH4_TAG);
		pcb->m_gk_switch[5] = input_port_read(*owner(), GKSWITCH5_TAG);
	}
}

void cartridge_device::device_config_complete()
{
	if (VERBOSE>8) LOG("cartridge_device: device_config_complete\n");
	update_names();
}

void cartridge_device::prepare_cartridge()
{
	int rom2_length;

	UINT8* grom_ptr;

	const memory_region *regg;
	const memory_region *regr;
	const memory_region *regr2;

	m_pcb->m_grom_size = m_rpk->get_resource_length("grom_socket");

	if (m_pcb->m_grom_size > 0)
	{
		regg = subregion("grom_contents");
		grom_ptr = (UINT8*)m_rpk->get_contents_of_socket("grom_socket");
		memcpy(regg->base(), grom_ptr, m_pcb->m_grom_size);

		if (m_type==PCB_GKRACKER)
		{
			cartridge_pcb_gramkracker_device* pcb = static_cast<cartridge_pcb_gramkracker_device*>(m_pcb);
			pcb->m_grom_ptr = grom_ptr;
		}

		// Find the GROMs and keep their pointers
		m_pcb->m_grom[0] = static_cast<ti99_grom_device*>(subdevice(GROM3_TAG));
		if (m_pcb->m_grom_size > 0x2000) m_pcb->m_grom[1] = static_cast<ti99_grom_device*>(subdevice(GROM4_TAG));
		if (m_pcb->m_grom_size > 0x4000) m_pcb->m_grom[2] = static_cast<ti99_grom_device*>(subdevice(GROM5_TAG));
		if (m_pcb->m_grom_size > 0x6000) m_pcb->m_grom[3] = static_cast<ti99_grom_device*>(subdevice(GROM6_TAG));
		if (m_pcb->m_grom_size > 0x8000) m_pcb->m_grom[4] = static_cast<ti99_grom_device*>(subdevice(GROM7_TAG));
	}

	m_pcb->m_rom_size = m_rpk->get_resource_length("rom_socket");
	if (m_pcb->m_rom_size > 0)
	{
		regr = subregion("rom_contents");
		m_pcb->m_rom_ptr = (UINT8*)m_rpk->get_contents_of_socket("rom_socket");
		memcpy(regr->base(), m_pcb->m_rom_ptr, m_pcb->m_rom_size);
	}

	rom2_length = m_rpk->get_resource_length("rom2_socket");
	if (rom2_length > 0)
	{
		// sizes do not differ between rom and rom2
		regr2 = subregion("rom2_contents");
		m_pcb->m_rom2_ptr = (UINT8*)m_rpk->get_contents_of_socket("rom2_socket");
		memcpy(regr2->base(), m_pcb->m_rom2_ptr, rom2_length);
	}

	m_pcb->m_ram_size = m_rpk->get_resource_length("ram_socket");
	if (m_pcb->m_ram_size > 0)
	{
		// TODO: Consider to use a region as well. If so, do not forget to memcpy.
		m_pcb->m_ram_ptr = (UINT8*)m_rpk->get_contents_of_socket("ram_socket");
	}
}

bool cartridge_device::has_grom()
{
	assert (m_pcb != NULL);
	return (m_pcb->m_grom_size>0);
}

void cartridge_device::set_gk_guest(cartridge_device* guest)
{
	assert (m_pcb != NULL);
	assert (m_rpk->get_type()==PCB_GKRACKER);

	cartridge_pcb_gramkracker_device* pcb = static_cast<cartridge_pcb_gramkracker_device*>(m_pcb);
	pcb->set_gk_guest(guest);
}

void cartridge_device::set_switch(int swtch, int val)
{
	assert (m_pcb != NULL);
	assert (m_rpk->get_type()==PCB_GKRACKER);
	cartridge_pcb_gramkracker_device* pcb = static_cast<cartridge_pcb_gramkracker_device*>(m_pcb);
	pcb->m_gk_switch[swtch] = val;
}


bool cartridge_device::call_load()
{
	// File name is in m_basename
	// return true = error
	if (VERBOSE>8) LOG("cartridge_device: loading\n");
	rpk_reader *reader = new rpk_reader(pcbdefs);

	astring ntag(tag(), ":PCB");
	const char* mytag = ntag.cstr();

	gromport_device *gp = static_cast<gromport_device*>(owner());
	int slot = get_index_from_tagname();

	try
	{
		m_rpk = reader->open(machine().options(), m_basename, machine().system().name);
		m_type = m_rpk->get_type();
		switch (m_type)
		{
		case PCB_STANDARD:
			if (VERBOSE>6) LOG("cartridge_device: standard PCB\n");
			m_pcb = static_cast<cartridge_pcb_standard_device*>(&machine().add_dynamic_device(*this, TICARTPCBSTD, mytag, 0));
			break;
		case PCB_PAGED:
			if (VERBOSE>6) LOG("cartridge_device: paged PCB\n");
			m_pcb = static_cast<cartridge_pcb_paged_device*>(&machine().add_dynamic_device(*this, TICARTPCBPAG, mytag, 0));
			break;
		case PCB_MINIMEM:
			if (VERBOSE>6) LOG("cartridge_device: minimem PCB\n");
			m_pcb = static_cast<cartridge_pcb_minimem_device*>(&machine().add_dynamic_device(*this, TICARTPCBMIN, mytag, 0));
			break;
		case PCB_SUPER:
			if (VERBOSE>6) LOG("cartridge_device: superspace PCB\n");
			m_pcb = static_cast<cartridge_pcb_superspace_device*>(&machine().add_dynamic_device(*this, TICARTPCBSUP, mytag, 0));
			break;
		case PCB_MBX:
			if (VERBOSE>6) LOG("cartridge_device: MBX PCB\n");
			m_pcb = static_cast<cartridge_pcb_mbx_device*>(&machine().add_dynamic_device(*this, TICARTPCBMBX, mytag, 0));
			break;
		case PCB_PAGED379I:
			if (VERBOSE>6) LOG("cartridge_device: Paged379i PCB\n");
			m_pcb = static_cast<cartridge_pcb_paged379i_device*>(&machine().add_dynamic_device(*this, TICARTPCBPGI, mytag, 0));
			break;
		case PCB_PAGEDCRU:
			if (VERBOSE>6) LOG("cartridge_device: PagedCRU PCB\n");
			m_pcb = static_cast<cartridge_pcb_pagedcru_device*>(&machine().add_dynamic_device(*this, TICARTPCBPGC, mytag, 0));
			break;
		case PCB_GKRACKER:
			if (VERBOSE>6) LOG("cartridge_device: GRAMKracker PCB\n");
			m_pcb = static_cast<cartridge_pcb_gramkracker_device*>(&machine().add_dynamic_device(*this, TICARTPCBGRK, mytag, 0));
			gp->set_gk_slot(slot);
			break;
		}

		prepare_cartridge();
		m_pcb->set_machine(machine()); // TODO: Feels like a hack.
		gp->change_slot(true, slot);

		machine().schedule_hard_reset();  // could be configurable by DIP switch, simulating taping the RESET line
	}
	catch (rpk_exception& err)
	{
		LOG("Failed to load cartridge '%s': %s\n", basename(), err.to_string());
		m_rpk = NULL;
		m_err = IMAGE_ERROR_INVALIDIMAGE;
		return true;
	}
	return false;
}

void cartridge_device::call_unload()
{
	if (VERBOSE>6) LOG("cartridge_device: unloading from %s\n", tag());

	gromport_device *gp = static_cast<gromport_device*>(owner());
	int slot = get_index_from_tagname();
	if (slot == gp->m_gk_slot)
	{
		gp->m_gk_slot = -1;		// Remove GRAMKracker slot index
		// seems to be no way to change the dip switch except by keyboard
	}

	if (m_rpk != NULL)
	{
		// Transfer RAM contents back into socket structure
/*      if (m_pcb->m_ram_size > 0)
        {
            UINT8* ram_ptr;
            ram_ptr = (UINT8*)m_rpk->get_contents_of_socket("ram_socket");
            memcpy(ram_ptr, m_pcb->m_ram_ptr, m_pcb->m_ram_size);
        } */
		m_rpk->close(); // will write NVRAM contents
		delete m_rpk;
	}

	// TODO: How can we remove the dynamically added device?
	m_pcb = NULL;
	gp->change_slot(false, get_index_from_tagname());
}

READ8Z_MEMBER(cartridge_device::readz)
{
	assert(m_pcb != NULL);
	m_pcb->readz(space, offset, value, mem_mask);
}

WRITE8_MEMBER(cartridge_device::write)
{
	assert(m_pcb != NULL);
	m_pcb->write(space, offset, data, mem_mask);
}

void cartridge_device::crureadz(offs_t offset, UINT8 *value)
{
	assert(m_pcb != NULL);
	m_pcb->crureadz(offset, value);
}

void cartridge_device::cruwrite(offs_t offset, UINT8 data)
{
	assert(m_pcb != NULL);
	m_pcb->cruwrite(offset, data);
}

/*
    Find the index of the cartridge name. We assume the format
    <name><number>, i.e. the number is the longest string from the right
    which can be interpreted as a number. Subtract 1.
*/
int cartridge_device::get_index_from_tagname()
{
	const char *mytag = tag();
	int maxlen = strlen(mytag);
	int i;
	for (i=maxlen-1; i >=0; i--)
		if (mytag[i] < 48 || mytag[i] > 57) break;

	return atoi(mytag+i+1)-1;
}

static GROM_CONFIG(grom3_config)
{
	false, 3, CARTGROM_TAG, 0x0000, 0x1800, false, DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, gromport_device, ready_line)
};
static GROM_CONFIG(grom4_config)
{
	false, 4, CARTGROM_TAG, 0x2000, 0x1800, false, DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, gromport_device, ready_line)
};
static GROM_CONFIG(grom5_config)
{
	false, 5, CARTGROM_TAG, 0x4000, 0x1800, false, DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, gromport_device, ready_line)
};
static GROM_CONFIG(grom6_config)
{
	false, 6, CARTGROM_TAG, 0x6000, 0x1800, false, DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, gromport_device, ready_line)
};
static GROM_CONFIG(grom7_config)
{
	false, 7, CARTGROM_TAG, 0x8000, 0x1800, false, DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, gromport_device, ready_line)
};

/*
    The GROM devices on a cartridge.
*/
static MACHINE_CONFIG_FRAGMENT( cartridge_groms )
	MCFG_GROM_ADD( GROM3_TAG, grom3_config )
	MCFG_GROM_ADD( GROM4_TAG, grom4_config )
	MCFG_GROM_ADD( GROM5_TAG, grom5_config )
	MCFG_GROM_ADD( GROM6_TAG, grom6_config )
	MCFG_GROM_ADD( GROM7_TAG, grom7_config )
MACHINE_CONFIG_END

machine_config_constructor cartridge_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( cartridge_groms );
}

/*
    Memory area for one cartridge.
*/
ROM_START( cartridge_memory )
	ROM_REGION(0xa000, CARTGROM_TAG, 0)
	ROM_FILL(0x0000, 0xa000, 0x00)
	ROM_REGION(0x2000, CARTROM_TAG, 0)
	ROM_FILL(0x0000, 0x2000, 0x00)
	ROM_REGION(0x2000, CARTROM2_TAG, 0)
	ROM_FILL(0x0000, 0x2000, 0x00)
ROM_END

const rom_entry *cartridge_device::device_rom_region() const
{
	return ROM_NAME( cartridge_memory );
}

void gromport_device::set_gk_slot(int slot)
{
	if (VERBOSE>5) LOG("gromport_device: Setting slot %d as GK slot.\n", slot);
	m_gk_slot = slot;
}

void gromport_device::find_gk_guest()
{
	// The first slot which contains a cartridge (not the GK) is the guest of the GK
	for (int i=0; i < m_numcart; i++)
	{
		if (i==m_gk_slot) continue;
		if (m_cartridge[i]->is_available())
		{
			m_cartridge[m_gk_slot]->set_gk_guest(m_cartridge[i]);
			if (VERBOSE>5) LOG("gromport_device: Setting slot %d as guest slot.\n", i);
			return;
		}
	}
	m_cartridge[m_gk_slot]->set_gk_guest(NULL);
}

/*****************************************************************************
    The complete multi-port cartridge system. Hosts up to four cartridges
    at this time.
*****************************************************************************/

gromport_device::gromport_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
: bus8z_device(mconfig, GROMPORT, "TI-99 multi-cartridge extender", tag, owner, clock)
{
	m_gk_slot = -1;
}

/*
    Activates a slot in the multi-cartridge extender.
    Setting the slot is done by accessing the GROM ports using a
    specific address:

    slot 0:  0x9800 (0x9802, 0x9c00, 0x9c02)   : cartridge1
    slot 1:  0x9804 (0x9806, 0x9c04, 0x9c06)   : cartridge2
    ...
    slot 15: 0x983c (0x983e, 0x9c3c, 0x9c3e)   : cartridge16

    Scheme:

    1001 1Wxx xxxx xxM0 (M=mode; M=0: data, M=1: address; W=write)

    The following addresses are theoretically available, but the
    built-in OS does not use them; i.e. cartridges will not be
    included in the selection list, and their features will not be
    found by lookup, but they could be accessed directly by user
    programs.
    slot 16: 0x9840 (0x9842, 0x9c40, 0x9c42)
    ...
    slot 255:  0x9bfc (0x9bfe, 0x9ffc, 0x9ffe)

    Setting the GROM base should select one cartridge, but the ROMs in the
    CPU space must also be switched. As there is no known special mechanism
    we assume that by switching the GROM base, the ROM is automatically
    switched.

    Caution: This means that cartridges which do not have at least one
    GROM cannot be switched with this mechanism.

    We assume that the slot number is already calculated in the caller:
    slotnumber>=0 && slotnumber<=255

    NOTE: The OS will stop searching when it finds slots 1 and 2 empty.
    Interestingly, cartridge subroutines are found nevertheless, even when
    the cartridge is plugged into a higher slot.
*/
void gromport_device::set_slot(int slotnumber)
{
	if (VERBOSE>7)
		if (m_active_slot != slotnumber) LOG("ti99: gromport: Setting cartslot to %d\n", slotnumber);

	if (m_fixed_slot==AUTO)
		m_active_slot = slotnumber;
	else
		m_active_slot = m_fixed_slot;
}

int gromport_device::get_active_slot(bool changebase, offs_t offset)
{
	int slot;
	if (changebase)
	{
		if ((offset & GROM_MASK) == GROM_AREA)
		{
			set_slot((offset>>2) & 0x00ff);
		}
	}
	slot = m_active_slot;

	// Here we have a design flaw in the operating system of the TI.
	// The system iterates through all banks to check for multiple cartridges.
	// If we do not have a multi-cart system, each bank shows the same contents
	// since there is no decoding of the bank information.
	// This brings us into trouble here: If only slot 1 is occupied, slot 2
	// would deliver zeros only. This differs from slot 1, so we get a
	// meaningless selection option.
	//
	// If we have only one cartridge in slot 1, we pretend that we have a
	// single-slot system. The slot variable is forced to the first slot.

	if (m_fixed_slot==AUTO && m_next_free_slot==1)
		slot=0;

	return slot;
}

void gromport_device::change_slot(bool inserted, int index)
{
	if (inserted)
	{
		if (VERBOSE>6) LOG("gromport_device: Inserted into slot %d\n", index);
		if (m_next_free_slot <= index)
			m_next_free_slot = index+1;
	}
	else
	{
		if (VERBOSE>6) LOG("gromport_device: Removed from slot %d\n", index);
		for (int i=NUMBER_OF_CARTRIDGE_SLOTS-1; i>=0; i--)
		{
			if (m_cartridge[i]->is_available())
			{
				m_next_free_slot = index+1;
				break;
			}
		}
	}
	if (VERBOSE>6) LOG("Next free slot: %d\n", m_next_free_slot);

	if (m_gk_slot != -1) find_gk_guest();
}

/****************************************************
    Memory space (ROM, GROM)
****************************************************/
READ8Z_MEMBER(gromport_device::readz)
{
	// GRAMKracker support
	// We *could* check the DIP switch, but this would only introduce another
	// source of confusion when the GRAMKracker is plugged in, but the switch
	// is not set.
	// The check for the presence of the GK using the index is safer.
	if ((m_gk_slot != -1) /*&& (input_port_read(*this, "CARTSLOT")==CART_GK) */)
	{
		m_cartridge[m_gk_slot]->readz(space, offset, value, mem_mask);
		return;
	}

	int slot = get_active_slot(true, offset);

	// If we have a GROM access, we need to send the read request to all
	// attached cartridges so the slot is irrelevant here. Each GROM
	// contains an internal address counter, and we must make sure they all stay in sync.
	if ((offset & GROM_MASK) == GROM_AREA)
	{
		for (int i=0; i < m_numcart; i++)
		{
			if (m_cartridge[i]->is_available())
			{
				UINT8 newval = *value;
				m_cartridge[i]->readz(space, offset, &newval, mem_mask);
				if (i==slot)
				{
					*value = newval;
				}
			}
		}
	}
	else
	{
		if (slot < m_numcart && m_cartridge[slot]->is_available())
		{
			m_cartridge[slot]->readz(space, offset, value, mem_mask);
		}
	}
}

WRITE8_MEMBER(gromport_device::write)
{
	// GRAMKracker support
	if ((m_gk_slot != -1) /*&& (input_port_read(*this, "CARTSLOT")==CART_GK)*/)
	{
		m_cartridge[m_gk_slot]->write(space, offset, data, mem_mask);
		return;
	}

	int slot = get_active_slot(true, offset);

	// Same issue as above (read)
	// We don't have GRAM cartridges, anyway, so it's just used for setting the address.
	if ((offset & GROM_MASK) == GROM_AREA)
	{
		for (int i=0; i < m_numcart; i++)
		{
			if (m_cartridge[i]->is_available())
			{
				m_cartridge[i]->write(space, offset, data, mem_mask);
			}
		}
	}
	else
	{
		if (slot < m_numcart && m_cartridge[slot]->is_available())
		{
			//      LOG("try it on slot %d\n", slot);
			m_cartridge[slot]->write(space, offset, data, mem_mask);
		}
	}
}

/*
    Called from the cartriges.
*/
WRITE_LINE_MEMBER(gromport_device::ready_line)
{
	m_ready(state);
}

/****************************************************
    CRU interface
****************************************************/
void gromport_device::crureadz(offs_t offset, UINT8 *value)
{
	// GRAMKracker support
	if ((m_gk_slot != -1) /* && (input_port_read(*this, "CARTSLOT")==CART_GK)*/)
	{
		m_cartridge[m_gk_slot]->crureadz(offset, value);
		return;
	}

	int slot = get_active_slot(false, offset);

	/* Sanity check. Higher slots are always empty. */
	if (slot >= m_numcart)
		return;

	if (m_cartridge[slot]->is_available())
	{
		m_cartridge[slot]->crureadz(offset, value);
	}
}

void gromport_device::cruwrite(offs_t offset, UINT8 data)
{
	// GRAMKracker support
	if ((m_gk_slot != -1) /*&& (input_port_read(*this, "CARTSLOT")==CART_GK)*/)
	{
		m_cartridge[m_gk_slot]->cruwrite(offset, data);
		return;
	}

	int slot = get_active_slot(true, offset);

	/* Sanity check. Higher slots are always empty. */
	if (slot >= m_numcart)
		return;

	if (m_cartridge[slot]->is_available())
	{
		m_cartridge[slot]->cruwrite(offset, data);
	}
}

/****************************************************
    Device management
****************************************************/
void gromport_device::device_start(void)
{
	// Resolve the callback line
	const gromport_config *intf = reinterpret_cast<const gromport_config *>(static_config());
	m_ready.resolve(intf->ready, *this);

	// Add the cartridges to the list
	device_iterator *iter = new device_iterator(*this, 1);

	int i = 0;
	cartridge_device *cart = static_cast<cartridge_device*>(iter->first());

	// Skip the first node (it's the gromport)
	cart = static_cast<cartridge_device*>(iter->next());
	while (cart != NULL)
	{
		m_cartridge[i++] = cart;
		// LOG("tag = %s\n", cart->tag());
		cart = static_cast<cartridge_device*>(iter->next());
	}
	m_numcart = i;
	m_next_free_slot = 0;
	m_active_slot = 0;
}

void gromport_device::device_stop(void)
{
}

void gromport_device::device_reset(void)
{
	m_active_slot = 0;
	m_fixed_slot = input_port_read(*this, "CARTSLOT") - 1;
}

INPUT_CHANGED_MEMBER( gromport_device::gk_changed )
{
	if (VERBOSE>7) LOG("gromport: input changed %d - %d\n", (int)((UINT64)param & 0x07), newval);
	if (m_gk_slot != -1)
	{
		m_cartridge[m_gk_slot]->set_switch((UINT64)param & 0x07, newval); // param is a pointer type
	}
}

INPUT_PORTS_START(gromport_device)
	PORT_START( "CARTSLOT" )
	PORT_DIPNAME( 0x0f, 0x00, "Multi-cartridge slot" )
		PORT_DIPSETTING(    0x00, "Auto" )
		PORT_DIPSETTING(    0x01, "Slot 1" )
		PORT_DIPSETTING(    0x02, "Slot 2" )
		PORT_DIPSETTING(    0x03, "Slot 3" )
		PORT_DIPSETTING(    0x04, "Slot 4" )
		PORT_DIPSETTING(    0x0f, "GRAM Kracker" )

	PORT_START( GKSWITCH1_TAG )
	PORT_DIPNAME( 0x01, 0x01, "GK switch 1" ) PORT_CONDITION( "CARTSLOT", 0x0f, PORTCOND_EQUALS, 0x0f ) PORT_CHANGED_MEMBER(DEVICE_SELF, gromport_device, gk_changed, 1)
		PORT_DIPSETTING(    0x00, "GK Off" )
		PORT_DIPSETTING(    0x01, DEF_STR( Normal ) )

	PORT_START( GKSWITCH2_TAG )
	PORT_DIPNAME( 0x01, 0x01, "GK switch 2" ) PORT_CONDITION( "CARTSLOT", 0x0f, PORTCOND_EQUALS, 0x0f ) PORT_CHANGED_MEMBER(DEVICE_SELF, gromport_device, gk_changed, 2)
		PORT_DIPSETTING(    0x00, "GRAM 0" )
		PORT_DIPSETTING(    0x01, "Op Sys" )

	PORT_START( GKSWITCH3_TAG )
	PORT_DIPNAME( 0x01, 0x01, "GK switch 3" ) PORT_CONDITION( "CARTSLOT", 0x0f, PORTCOND_EQUALS, 0x0f ) PORT_CHANGED_MEMBER(DEVICE_SELF, gromport_device, gk_changed, 3)
		PORT_DIPSETTING(    0x00, "GRAM 1-2" )
		PORT_DIPSETTING(    0x01, "TI BASIC" )

	PORT_START( GKSWITCH4_TAG )
	PORT_DIPNAME( 0x03, 0x01, "GK switch 4" ) PORT_CONDITION( "CARTSLOT", 0x0f, PORTCOND_EQUALS, 0x0f ) PORT_CHANGED_MEMBER(DEVICE_SELF, gromport_device, gk_changed, 4)
		PORT_DIPSETTING(    0x00, "Bank 1" )
		PORT_DIPSETTING(    0x01, "W/P" )
		PORT_DIPSETTING(    0x02, "Bank 2" )

	PORT_START( GKSWITCH5_TAG )
	PORT_DIPNAME( 0x01, 0x00, "GK switch 5" ) PORT_CONDITION( "CARTSLOT", 0x0f, PORTCOND_EQUALS, 0x0f ) PORT_CHANGED_MEMBER(DEVICE_SELF, gromport_device, gk_changed, 5)
		PORT_DIPSETTING(    0x00, "Loader On" )
		PORT_DIPSETTING(    0x01, "Loader Off" )
INPUT_PORTS_END

static MACHINE_CONFIG_FRAGMENT(gromport_device)
	MCFG_DEVICE_ADD("cartridge1", TICARTRIDGE, 0)
	MCFG_DEVICE_ADD("cartridge2", TICARTRIDGE, 0)
	MCFG_DEVICE_ADD("cartridge3", TICARTRIDGE, 0)
	MCFG_DEVICE_ADD("cartridge4", TICARTRIDGE, 0)
MACHINE_CONFIG_END

machine_config_constructor gromport_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( gromport_device );
}

ioport_constructor gromport_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(gromport_device);
}

const device_type GROMPORT = &device_creator<gromport_device>;
const device_type TICARTRIDGE = &device_creator<cartridge_device>;

const device_type TICARTPCBSTD = &device_creator<cartridge_pcb_standard_device>;
const device_type TICARTPCBPAG = &device_creator<cartridge_pcb_paged_device>;
const device_type TICARTPCBMIN = &device_creator<cartridge_pcb_minimem_device>;
const device_type TICARTPCBSUP = &device_creator<cartridge_pcb_superspace_device>;
const device_type TICARTPCBMBX = &device_creator<cartridge_pcb_mbx_device>;
const device_type TICARTPCBPGI = &device_creator<cartridge_pcb_paged379i_device>;
const device_type TICARTPCBPGC = &device_creator<cartridge_pcb_pagedcru_device>;
const device_type TICARTPCBGRK = &device_creator<cartridge_pcb_gramkracker_device>;

/*****************************************************************************************************************
******************************************************************************************************************
RPK format support

    A RPK file ("rompack") contains a collection of dump files and a layout
    file that defines the kind of circuit board (PCB) used in the cartridge
    and the mapping of dumps to sockets on the board.

Example:
    <?xml version="1.0" encoding="utf-8"?>
    <romset>
        <resources>
            <rom id="gromimage" file="ed-assmg.bin" />
        </resources>
        <configuration>
            <pcb type="standard">
                <socket id="grom_socket" uses="gromimage"/>
            </pcb>
        </configuration>
    </romset>

DTD:
    <!ELEMENT romset (resources, configuration)>
    <!ELEMENT resources (rom|ram)+>
    <!ELEMENT rom EMPTY>
    <!ELEMENT ram EMPTY>
    <!ELEMENT configuration (pcb)>
    <!ELEMENT pcb (socket)+>
    <!ELEMENT socket EMPTY>
    <!ATTLIST romset version CDATA #IMPLIED>
    <!ATTLIST rom id ID #REQUIRED
    <!ATTLIST rom file CDATA #REQUIRED>
    <!ATTLIST rom crc CDATA #IMPLIED>
    <!ATTLIST rom sha1 CDATA #IMPLIED>
    <!ATTLIST ram id ID #REQUIRED>
    <!ATTLIST ram type (volatile|persistent) #IMPLIED>
    <!ATTLIST ram store (internal|external) #IMPLIED>
    <!ATTLIST ram file CDATA #IMPLIED>
    <!ATTLIST ram length CDATA #REQUIRED>
    <!ATTLIST pcb type CDATA #REQUIRED>
    <!ATTLIST socket id ID #REQUIRED>
    <!ATTLIST socket uses IDREF #REQUIRED>

****************************************************************************/

#include "unzip.h"
#include "xmlfile.h"

/****************************************
    RPK class
****************************************/
/*
    Constructor.
*/
rpk::rpk(emu_options& options, const char* sysname)
	:m_options(options),
	m_system_name(sysname)
{
	m_sockets.reset();
};

rpk::~rpk()
{
	if (VERBOSE>6) LOG("Destroy RPK\n");
}

/*
    Deliver the contents of the socket by name of the socket.
*/
void* rpk::get_contents_of_socket(const char *socket_name)
{
	rpk_socket *socket = m_sockets.find(socket_name);
	if (socket==NULL) return NULL;
	return socket->get_contents();
}

/*
    Deliver the length of the contents of the socket by name of the socket.
*/
int rpk::get_resource_length(const char *socket_name)
{
	rpk_socket *socket = m_sockets.find(socket_name);
	if (socket==NULL) return 0;
	return socket->get_content_length();
}

void rpk::add_socket(const char* id, rpk_socket *newsock)
{
	m_sockets.append(id, *newsock);
}

/*-------------------------------------------------
    rpk_close - closes a rpk
    Saves the contents of the NVRAMs and frees all memory.
-------------------------------------------------*/

void rpk::close()
{
	// Save the NVRAM contents
	rpk_socket *socket = m_sockets.first();
	while (socket != NULL)
	{
		if (socket->persistent_ram())
		{
			image_battery_save_by_name(m_options, socket->get_pathname(), socket->get_contents(), socket->get_content_length());
		}
		socket->cleanup();
		socket = socket->m_next;
	}
}

/**************************************************************
    RPK socket (location in the PCB where a chip is plugged in;
    not a network socket)
***************************************************************/

rpk_socket::rpk_socket(const char* id, int length, void* contents, const char *pathname)
: m_id(id), m_length(length), m_next(NULL), m_contents(contents), m_pathname(pathname)
{
};

rpk_socket::rpk_socket(const char* id, int length, void* contents)
: m_id(id), m_length(length), m_next(NULL), m_contents(contents), m_pathname(NULL)
{
};

/*
    Locate a file in the ZIP container
*/
const zip_file_header* rpk_reader::find_file(zip_file *zip, const char *filename, UINT32 crc)
{
	const zip_file_header *header;
	for (header = zip_file_first_file(zip); header != NULL; header = zip_file_next_file(zip))
	{
		// We don't check for CRC == 0.
		if (crc != 0)
		{
			// if the CRC and name both match, we're good
			// if the CRC matches and the name doesn't, we're still good
			if (header->crc == crc)
				return header;
		}
		else
		{
			if (core_stricmp(header->filename, filename)==0)
			{
				return header;
			}
		}
	}
	return NULL;
}

/*
    Load a rom resource and put it in a pcb socket instance.
*/
rpk_socket* rpk_reader::load_rom_resource(zip_file* zip, xml_data_node* rom_resource_node, const char* socketname)
{
	const char* file;
	const char* crcstr;
	const char* sha1;
	zip_error ziperr;
	UINT32 crc;
	int length;
	void* contents;
	const zip_file_header *header;

	// find the file attribute (required)
	file = xml_get_attribute_string(rom_resource_node, "file", NULL);
	if (file == NULL) throw rpk_exception(RPK_INVALID_LAYOUT, "<rom> must have a 'file' attribute");

	if (VERBOSE>6) LOG("gromport/RPK: Loading ROM contents for socket '%s' from file %s\n", socketname, file);

	// check for crc
	crcstr = xml_get_attribute_string(rom_resource_node, "crc", NULL);
	if (crcstr==NULL)
	{
		// no CRC, just find the file in the RPK
		header = find_file(zip, file, 0);
	}
	else
	{
		crc = strtoul(crcstr, NULL, 16);
		header = find_file(zip, file, crc);
	}
	if (header == NULL)	throw rpk_exception(RPK_INVALID_FILE_REF, "File not found or CRC check failed");

	length = header->uncompressed_length;

	// Allocate storage
	contents = malloc(length);
	if (contents==NULL) throw rpk_exception(RPK_OUT_OF_MEMORY);

	// and unzip file from the zip file
	ziperr = zip_file_decompress(zip, contents, length);
	if (ziperr != ZIPERR_NONE) throw rpk_exception(RPK_ZIP_ERROR);

	// check for sha1
	sha1 = xml_get_attribute_string(rom_resource_node, "sha1", NULL);
	if (sha1 != NULL)
	{
		hash_collection actual_hashes;
		actual_hashes.compute((const UINT8 *)contents, length, hash_collection::HASH_TYPES_CRC_SHA1);

		hash_collection expected_hashes;
		expected_hashes.add_from_string(hash_collection::HASH_SHA1, sha1, strlen(sha1));

		if (actual_hashes != expected_hashes) throw rpk_exception(RPK_INVALID_FILE_REF, "SHA1 check failed");
	}

	// Create a socket instance
	return new rpk_socket(socketname, length, contents);
}

/*
    Load a ram resource and put it in a pcb socket instance.
*/
rpk_socket* rpk_reader::load_ram_resource(emu_options &options, xml_data_node* ram_resource_node, const char* socketname, const char* system_name)
{
	const char* length_string;
	const char* ram_type;
	const char* ram_filename;
	const char* ram_pname;
	int length;
	void* contents;

	// find the length attribute
	length_string = xml_get_attribute_string(ram_resource_node, "length", NULL);
	if (length_string == NULL) throw rpk_exception(RPK_MISSING_RAM_LENGTH);

	// parse it
	char suffix = '\0';
	sscanf(length_string, "%u%c", &length, &suffix);
	switch(tolower(suffix))
	{
		case 'k': // kilobytes
			length *= 1024;
			break;

		case 'm':
			/* megabytes */
			length *= 1024*1024;
			break;

		case '\0':
			break;

		default:  // failed
			throw rpk_exception(RPK_INVALID_RAM_SPEC);
	}

	// Allocate memory for this resource
	contents = malloc(length);
	if (contents==NULL) throw rpk_exception(RPK_OUT_OF_MEMORY);

	if (VERBOSE>6) LOG("gromport/RPK: Allocating RAM buffer (%d bytes) for socket '%s'\n", length, socketname);

	ram_pname = NULL;

	// That's it for pure RAM. Now check whether the RAM is "persistent", i.e. NVRAM.
	// In that case we must load it from the NVRAM directory.
	// The file name is given in the RPK file; the subdirectory is the system name.
	ram_type = xml_get_attribute_string(ram_resource_node, "type", NULL);
	if (ram_type != NULL)
	{
		if (strcmp(ram_type, "persistent")==0)
		{
			// Get the file name (required if persistent)
			ram_filename = xml_get_attribute_string(ram_resource_node, "file", NULL);
			if (ram_filename==NULL)
			{
				free(contents);
				throw rpk_exception(RPK_INVALID_RAM_SPEC, "<ram type='persistent'> must have a 'file' attribute");
			}
			astring ram_pathname(system_name, PATH_SEPARATOR, ram_filename);
			ram_pname = core_strdup(ram_pathname.cstr());
			// load, and fill rest with 00
			if (VERBOSE>6) LOG("gromport/RPK: Loading NVRAM contents from '%s'\n", ram_pname);
			image_battery_load_by_name(options, ram_pname, contents, length, 0x00);
		}
	}

	// Create a socket instance
	return new rpk_socket(socketname, length, contents, ram_pname);
}

/*-------------------------------------------------
    rpk_open - open a RPK file
    options - parameters from the settings; we need it only for the NVRAM directory
    system_name - name of the driver (also just for NVRAM handling)
-------------------------------------------------*/

rpk* rpk_reader::open(emu_options &options, const char *filename, const char *system_name)
{
	zip_error ziperr;

	const zip_file_header *header;
	const char *pcb_type;
	const char *id;
	const char *uses_name;
	const char *resource_name;

	zip_file* zipfile;

	char *layout_text = NULL;
	xml_data_node *layout_xml;
	xml_data_node *romset_node;
	xml_data_node *configuration_node;
	xml_data_node *resources_node;
	xml_data_node *resource_node;
	xml_data_node *socket_node;
	xml_data_node *pcb_node;

	rpk_socket *newsock;

	int i;

	rpk *newrpk = new rpk(options, system_name);

	try
	{
		/* open the ZIP file */
		ziperr = zip_file_open(filename, &zipfile);
		if (ziperr != ZIPERR_NONE) throw rpk_exception(RPK_NOT_ZIP_FORMAT);

		/* find the layout.xml file */
		header = find_file(zipfile, "layout.xml", 0);
		if (header == NULL) throw rpk_exception(RPK_MISSING_LAYOUT);

		/* reserve space for the layout file contents (+1 for the termination) */
		layout_text = (char*)malloc(header->uncompressed_length + 1);
		if (layout_text == NULL) throw rpk_exception(RPK_OUT_OF_MEMORY);

		/* uncompress the layout text */
		ziperr = zip_file_decompress(zipfile, layout_text, header->uncompressed_length);
		if (ziperr != ZIPERR_NONE) throw rpk_exception(RPK_ZIP_ERROR);

		layout_text[header->uncompressed_length] = '\0';  // Null-terminate

		/* parse the layout text */
		layout_xml = xml_string_read(layout_text, NULL);
		if (layout_xml == NULL) throw rpk_exception(RPK_XML_ERROR);

		// Now we work within the XML tree

		// romset is the root node
		romset_node = xml_get_sibling(layout_xml->child, "romset");
		if (romset_node==NULL) throw rpk_exception(RPK_INVALID_LAYOUT, "document element must be <romset>");

		// resources is a child of romset
		resources_node = xml_get_sibling(romset_node->child, "resources");
		if (resources_node==NULL) throw rpk_exception(RPK_INVALID_LAYOUT, "<romset> must have a <resources> child");

		// configuration is a child of romset; we're actually interested in ...
		configuration_node = xml_get_sibling(romset_node->child, "configuration");
		if (configuration_node==NULL) throw rpk_exception(RPK_INVALID_LAYOUT, "<romset> must have a <configuration> child");

		// ... pcb, which is a child of configuration
		pcb_node = xml_get_sibling(configuration_node->child, "pcb");
		if (pcb_node==NULL) throw rpk_exception(RPK_INVALID_LAYOUT, "<configuration> must have a <pcb> child");

		// We'll try to find the PCB type on the provided type list.
		pcb_type = xml_get_attribute_string(pcb_node, "type", NULL);
		if (pcb_type==NULL) throw rpk_exception(RPK_INVALID_LAYOUT, "<pcb> must have a 'type' attribute");
		if (VERBOSE>6) LOG("gromport/RPK: Cartridge says it has PCB type '%s'\n", pcb_type);

		i=0;
		do
		{
			if (strcmp(pcb_type, m_types[i].name)==0)
			{
				newrpk->m_type = m_types[i].id;
				break;
			}
			i++;
		} while (m_types[i].id != 0);

		if (m_types[i].id==0) throw rpk_exception(RPK_UNKNOWN_PCB_TYPE);

		// Find the sockets and load their respective resource
		for (socket_node = pcb_node->child;  socket_node != NULL; socket_node = socket_node->next)
		{
			if (strcmp(socket_node->name, "socket")!=0) throw rpk_exception(RPK_INVALID_LAYOUT, "<pcb> element has only <socket> children");
			id = xml_get_attribute_string(socket_node, "id", NULL);
			if (id == NULL) throw rpk_exception(RPK_INVALID_LAYOUT, "<socket> must have an 'id' attribute");
			uses_name = xml_get_attribute_string(socket_node, "uses", NULL);
			if (uses_name == NULL) throw rpk_exception(RPK_INVALID_LAYOUT, "<socket> must have a 'uses' attribute");

			bool found = false;
			// Locate the resource node
			for (resource_node = resources_node->child; resource_node != NULL; resource_node = resource_node->next)
			{
				resource_name = xml_get_attribute_string(resource_node, "id", NULL);
				if (resource_name == NULL) throw rpk_exception(RPK_INVALID_LAYOUT, "resource node must have an 'id' attribute");

				if (strcmp(resource_name, uses_name)==0)
				{
					// found it
					if (strcmp(resource_node->name, "rom")==0)
					{
						newsock = load_rom_resource(zipfile, resource_node, id);
						newrpk->add_socket(id, newsock);
					}
					else
					{
						if (strcmp(resource_node->name, "ram")==0)
						{
							newsock = load_ram_resource(options, resource_node, id, system_name);
							newrpk->add_socket(id, newsock);
						}
						else throw rpk_exception(RPK_INVALID_LAYOUT, "resource node must be <rom> or <ram>");
					}
					found = true;
				}
			}
			if (!found) throw rpk_exception(RPK_INVALID_RESOURCE_REF, uses_name);
		}
	}
	catch (rpk_exception &exp)
	{
		newrpk->close();
		if (zipfile != NULL)		zip_file_close(zipfile);
		if (layout_text != NULL)	free(layout_text);

		// rethrow the exception
		throw exp;
	}

	if (zipfile != NULL)		zip_file_close(zipfile);
	if (layout_text != NULL)	free(layout_text);

	return newrpk;
}
