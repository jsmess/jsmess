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

    September 2010: Rewritten as device
*/
#include "emu.h"
#include "peribox.h"
#include "p_code.h"
#include "grom.h"

#define GROMBASE 0x5bfc
#define PCODE_CRU_BASE 0x1f00

#define pcode_region "pcode_region"
#define grom_offset 0x3000

typedef ti99_pebcard_config ti99_pcoden_config;

typedef struct _ti99_pcoden_state
{
	int selected;

	int bank_select; // 0 or 1

	// Pointer to the pcode ROM data. We have three sections of the ROM code:
	// 4K from one chip, 2 banks with 4K each from the second chip.
	UINT8 *rom0;
	UINT8 *rom1;
	UINT8 *rom2;
	UINT8 *grom;

	device_t *gromdev[8];

	ti99_peb_connect		lines;

} ti99_pcoden_state;

INLINE ti99_pcoden_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == PCODEN);

	return (ti99_pcoden_state *)downcast<legacy_device_base *>(device)->token();
}

/*************************************************************************/

/*
    The CRU write handler. The CRU is a serial interface in the console.
    Using the CRU we can switch the banks in the p-code card
    Bit 0 = activate card
    Bit 4 = select second bank of high ROM.
    Note that the CRU address is created from address lines A8, A13, and A14
    so bit 0 is at 0x1f00, but bit 4 is at 0x1f80 (bit 7 would be 0x1f86
    but it is not used)
*/
static WRITE8_DEVICE_HANDLER( cru_w )
{
	ti99_pcoden_state *card = get_safe_token(device);

	if ((offset & 0xff00)==PCODE_CRU_BASE)
	{
		int addr = offset & 0x00ff;

		if (addr==0)
			card->selected = data;

		if (addr==0x80) // Bit 4 is on address line 8
			card->bank_select = data;
	}
}

/*
    Read a byte in pcode DSR space
*/
static READ8Z_DEVICE_HANDLER( data_r )
{
	ti99_pcoden_state *card = get_safe_token(device);

	if (card->selected)
	{
		if ((offset & 0x7e000)==0x74000)
		{
			if ((offset & 0xfffd)==GROMBASE)
			{
				for (int i=0; i < 8; i++)
					ti99grom_rz(card->gromdev[i], offset, value);
				return;
			}

			if ((offset & 0x1000) == 0x0000)
			{
				/* Accesses ROM 4732 (4K) */
				*value = card->rom0[offset & 0x0fff];
			}
			else
				// Accesses ROM 4764 (2*4K)
			// We have two banks here which are activated according
			// to the setting of CRU bit 4
			*value = (card->bank_select==0)? card->rom1[offset & 0x0fff] : card->rom2[offset & 0x0fff];
		}
	}
}

/*
    Write a byte in P-Code ROM space. This is only useful for setting the
    GROM address.
*/
static WRITE8_DEVICE_HANDLER( data_w )
{
	ti99_pcoden_state *card = get_safe_token(device);
	if (card->selected)
	{
		if ((offset & 0x7fffd)==(GROMBASE|0x70400))
		{
			for (int i=0; i < 8; i++)
				ti99grom_w(card->gromdev[i], offset, data);
		}
	}
}

static const ti99_peb_card pcode_ncard =
{
	data_r,
	data_w,
	NULL,
	cru_w,

	NULL, NULL,	NULL, NULL
};

static DEVICE_START( ti99_pcoden )
{
	ti99_pcoden_state *pcode = get_safe_token(device);

	/* Resolve the callbacks to the PEB */
	peb_callback_if *topeb = (peb_callback_if *)device->baseconfig().static_config();
	devcb_resolve_write_line(&pcode->lines.ready, &topeb->ready, device);
}

static DEVICE_STOP( ti99_pcoden )
{
	logerror("ti99_pcode: stop\n");
}

static DEVICE_RESET( ti99_pcoden )
{
	logerror("ti99_pcode: reset\n");
	ti99_pcoden_state *pcode = get_safe_token(device);

	/* If the card is selected in the menu, register the card */
	if (input_port_read(device->machine(), "EXTCARD") & EXT_PCODE)
	{
		device_t *peb = device->owner();
		int success = mount_card(peb, device, &pcode_ncard, get_pebcard_config(device)->slot);
		if (!success) return;

		astring *region = new astring();
		astring_assemble_3(region, device->tag(), ":", pcode_region);

		pcode->rom0 = device->machine().region(astring_c(region))->base();
		pcode->rom1 = pcode->rom0 + 0x1000;
		pcode->rom2 = pcode->rom0 + 0x2000;
		pcode->grom = pcode->rom0 + 0x3000;
		pcode->bank_select = 0;
		pcode->selected = 0;

		astring *gromname = new astring();
		for (int i=0; i < 8; i++)
		{
			astring_printf(gromname, "grom_%d", i);
			pcode->gromdev[i] = device->subdevice(astring_c(gromname));
		}
	}
}

static WRITE_LINE_DEVICE_HANDLER( pcode_ready )
{
	// Caution: The device pointer passed to this function is the calling
	// device. That is, if we want *this* device, we need to take the owner.
	ti99_pcoden_state *pcode = get_safe_token(device->owner());
	devcb_call_write_line( &pcode->lines.ready, state );
}

/*
    Get the pointer to the GROM data from the P-Code card. Called by the GROM.
*/
static UINT8 *get_grom_ptr(device_t *device)
{
	ti99_pcoden_state *pcode = get_safe_token(device);
	return pcode->grom;
}

MACHINE_CONFIG_FRAGMENT( ti99_pcoden )
	MCFG_GROM_ADD_P( "grom_0", 0, get_grom_ptr, 0x0000, 0x1800, pcode_ready )
	MCFG_GROM_ADD_P( "grom_1", 1, get_grom_ptr, 0x2000, 0x1800, pcode_ready )
	MCFG_GROM_ADD_P( "grom_2", 2, get_grom_ptr, 0x4000, 0x1800, pcode_ready )
	MCFG_GROM_ADD_P( "grom_3", 3, get_grom_ptr, 0x6000, 0x1800, pcode_ready )
	MCFG_GROM_ADD_P( "grom_4", 4, get_grom_ptr, 0x8000, 0x1800, pcode_ready )
	MCFG_GROM_ADD_P( "grom_5", 5, get_grom_ptr, 0xa000, 0x1800, pcode_ready )
	MCFG_GROM_ADD_P( "grom_6", 6, get_grom_ptr, 0xc000, 0x1800, pcode_ready )
	MCFG_GROM_ADD_P( "grom_7", 7, get_grom_ptr, 0xe000, 0x1800, pcode_ready )
MACHINE_CONFIG_END

ROM_START( ti99_pcoden )
	ROM_REGION(0x13000, pcode_region, 0)
	ROM_LOAD_OPTIONAL("pcode_r0.bin", 0x0000, 0x1000, CRC(3881d5b0) SHA1(a60e0468bb15ff72f97cf6e80979ca8c11ed0426)) /* TI P-Code card rom4732 */
	ROM_LOAD_OPTIONAL("pcode_r1.bin", 0x1000, 0x2000, CRC(46a06b8b) SHA1(24e2608179921aef312cdee6f455e3f46deb30d0)) /* TI P-Code card rom4764 */
	ROM_LOAD_OPTIONAL("pcode_g0.bin", 0x3000, 0x10000, CRC(541b3860) SHA1(7be77c216737334ae997753a6a85136f117affb7)) /* TI P-Code card groms */
ROM_END

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)             p##ti99_pcoden##s
#define DEVTEMPLATE_FEATURES            DT_HAS_START | DT_HAS_STOP | DT_HAS_RESET | DT_HAS_ROM_REGION | DT_HAS_INLINE_CONFIG | DT_HAS_MACHINE_CONFIG
#define DEVTEMPLATE_NAME                "TI99 P-Code Card"
#define DEVTEMPLATE_SHORTNAME           "ti99pcode"
#define DEVTEMPLATE_FAMILY              "Peripheral expansion"
#include "devtempl.h"

DEFINE_LEGACY_DEVICE(PCODEN, ti99_pcoden);
