/*
    SNUG HSGPL card emulation.
    Raphael Nabet, 2003.

    Rewritten as device
    Michael Zapf, October 2010
*/

#include "emu.h"
#include "emuopts.h"
#include "peribox.h"
#include "hsgpl.h"
#include "machine/at29040a.h"
#include "ti99defs.h"

/*
    Supports 16 banks of 8 GROMs (8kbytes each) with 16 associated banks of
    32kbytes (8kbytes*4) of module ROM, 2 banks of 8 GRAMs with 2 associated
    banks of 32 kbytes of RAM, and 512kbytes of DSR.  Roms are implemented with
    512kbyte EEPROMs (1 for DSR, 2 for GROMs, 1 for cartridge ROM).  RAM is
    implemented with 128kbyte SRAMs (1 for GRAM, 1 for cartridge RAM - only the
    first 64kbytes of the cartridge RAM chip is used).

    CRU bits:
       Name    Equates Meaning
    >0 DEN     DSRENA  DSR Enable
    >1 GRMENA  GRMENA  Enable GRAM instead of GROM in banks 0 and 1
    >2 BNKINH* BNKENA  Disable banking
    >3 PG0     PG0
    >4 PG1     PG1
    >5 PG2     PG2     Paging-Bits for DSR-area
    >6 PG3     PG3
    >7 PG4     PG4
    >8 PG5     PG5
    >9 CRDENA  CRDENA  Activate memory areas of HSGPL (i.e. enable HSGPL GROM and ROM6 ports)
    >A WRIENA  WRIENA  write enable for RAM and GRAM (and flash GROM!)
    >B SCENA   SCARTE  Activate SuperCart-banking
    >C LEDENA  LEDENA  turn LED on
    >D -       -       free
    >E MBXENA  MBXENA  Activate MBX-Banking
    >F RAMENA  RAMENA  Enable RAM6000 instead of ROM6000 in banks 0 and 1


    Direct access ports for all memory areas (the original manual gives
    >9880->989C and >9C80->9C9C for ROM6000, but this is incorrect):

    Module bank Read    Write   Read ROM6000        Write ROM6000
                GROM    GROM
    0           >9800   >9C00   >9860 Offset >0000  >9C60 Offset >0000
    1           >9804   >9C04   >9860 Offset >8000  >9C60 Offset >8000
    2           >9808   >9C08   >9864 Offset >0000  >9C64 Offset >0000
    3           >980C   >9C0C   >9864 Offset >8000  >9C64 Offset >8000
    4           >9810   >9C10   >9868 Offset >0000  >9C68 Offset >0000
    5           >9814   >9C14   >9868 Offset >8000  >9C68 Offset >8000
    6           >9818   >9C18   >986C Offset >0000  >9C6C Offset >0000
    7           >981C   >9C1C   >986C Offset >8000  >9C6C Offset >8000
    8           >9820   >9C20   >9870 Offset >0000  >9C70 Offset >0000
    9           >9824   >9C24   >9870 Offset >8000  >9C70 Offset >8000
    10          >9828   >9C28   >9874 Offset >0000  >9C74 Offset >0000
    11          >982C   >9C2C   >9874 Offset >8000  >9C74 Offset >8000
    12          >9830   >9C30   >9878 Offset >0000  >9C78 Offset >0000
    13          >9834   >9C34   >9878 Offset >8000  >9C78 Offset >8000
    14          >9838   >9C38   >987C Offset >0000  >9C7C Offset >0000
    15          >983C   >9C3C   >987C Offset >8000  >9C7C Offset >8000

    Module bank Read    Write   Read RAM6000        Write RAM6000
                GRAM    GRAM
    16 (Ram)    >9880   >9C80   >98C0 Offset >0000  >9CC0 Offset >0000
    17 (Ram)    >9884   >9C84   >98C0 Offset >8000  >9CC0 Offset >8000

    DSR bank    Read    Write
    0 - 7       >9840   >9C40
    8 - 15      >9844   >9C44
    16 - 23     >9848   >9C48
    24 - 31     >984C   >9C4C
    32 - 39     >9850   >9C50
    40 - 47     >9854   >9C54
    48 - 55     >9858   >9C58
    56 - 63     >985C   >9C5C

    Note: Writing only works for areas set up as RAM.  To write to the
        FEEPROMs, you must used the algorithm specified by their respective
        manufacturer.

    FIXME: Crashes when a cartridge is plugged in
*/

#define CRU_BASE 0x1B00

#define RAMSIZE 		0x020000
#define GRAMSIZE		0x020000
#define FEEPROM_SIZE	0x80000
#define FLROMSIZE	4*(FEEPROM_SIZE+2) + 1

#define NVVERSION 0

typedef ti99_pebcard_config hsgpl_config;

typedef struct _hsgpl_state
{
	device_t	*dsr, *rom6, *groma, *gromb;
	UINT8	*ram6;
	UINT8	*gram;
	UINT8	*flashrom;

	int 	current_grom_port;

	int		dsr_enabled;
	int		gram_enabled;
	int		bank_enabled;
	int		dsr_page;
	int		card_enabled;
	int		write_enabled;
	int		supercart_enabled;
	int		led_on;
	int		mbx_enabled;
	int		ram_enabled;

	int		flash_mode;

	int		current_bank;

	/* GROM emulation */
	int		raddr_LSB, waddr_LSB;
	int		grom_address;

} hsgpl_state;

INLINE hsgpl_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == HSGPL);

	return (hsgpl_state *)downcast<legacy_device_base *>(device)->token();
}

/*
    Write hsgpl CRU interface
*/
static WRITE8_DEVICE_HANDLER( cru_w )
{
	hsgpl_state *card = get_safe_token(device);

	if ((offset & 0xff00)==CRU_BASE)
	{
		int bit = (offset >> 1) & 0x0f;
		switch (bit)
		{
		case 0:
			card->dsr_enabled = data;
			logerror("hsgpl: Set dsr_enabled=%x\n", data);
			break;
		case 1:
			card->gram_enabled = data;
			logerror("hsgpl: Set gram_enabled=%x\n", data);
			break;
		case 2:
			card->bank_enabled = data;
			logerror("hsgpl: Set bank_enabled=%x\n", data);
			break;
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
			if (data)
				card->dsr_page |= (1 << (bit-3));
			else
				card->dsr_page &= ~(1 << (bit-3));
			logerror("hsgpl: Set dsr_page=%d\n", card->dsr_page);
			break;
		case 9:
			card->card_enabled = data;
			logerror("hsgpl: Set card_enabled=%x\n", data);
			break;
		case 10:
			card->write_enabled = data;
			logerror("hsgpl: Set write_enabled=%x\n", data);
			break;
		case 11:
			card->supercart_enabled = data;
			logerror("hsgpl: Set supercart_enabled=%x\n", data);
			break;
		case 12:
			card->led_on = data;
			logerror("hsgpl: Set led_on=%x\n", data);
			break;
		case 13:
			break;
		case 14:
			card->mbx_enabled = data;
			logerror("hsgpl: Set mbx_enabled=%x\n", data);
			break;
		case 15:
			card->ram_enabled = data;
			logerror("hsgpl: Set ram_enabled=%x\n", data);
			break;
		}
	}
}

/*
    GROM read. This is somewhat different to the original TI GROMs, so we
    partly reimplement it here. Anyway, the card does not use GROMs but makes
    use of a special decoder chip (MACH) which emulates the GROM functions.
*/
static READ8Z_DEVICE_HANDLER ( hsgpl_grom_rz )
{
	int port;
	hsgpl_state *card = get_safe_token(device);
	//activedevice_adjust_icount(-4);

	// 1001 10bb bbbb bba0
	port = card->current_grom_port = (offset & 0x3fc) >> 2;

	if (offset & 2)
	{	// Read GPL address. This must be available even when the rest
		// of the card is offline (card_enabled=0).
		card->waddr_LSB = FALSE;

		if (card->raddr_LSB)
		{
			*value = ((card->grom_address + 1) & 0xff);
			card->raddr_LSB = FALSE;
		}
		else
		{
			*value = ((card->grom_address + 1) >> 8) & 0xff;
			card->raddr_LSB = TRUE;
		}
	}
	else
	{	/* read GPL data */
		if (card->card_enabled)
		{
			if ((port < 2) && (card->gram_enabled))
			{
				*value = card->gram[card->grom_address + 0x10000*port];
			}
			else
			{
				if (port < 8)
				{
					if (!card->flash_mode)
					{
						*value = at29c040a_r(card->groma, card->grom_address + 0x10000*port);
					}
				}
				else
				{
					if (port < 16)
					{
						*value = at29c040a_r(card->gromb, card->grom_address + 0x10000*(port-8));
					}
					else
					{
						if (port < 24)
						{
							*value = at29c040a_r(card->dsr, card->grom_address + 0x10000*(port-16));
						}
						else
						{
							if (port < 32)
							{
								/* The HSGPL manual says 32-47, but this is incorrect */
								*value = at29c040a_r(card->rom6, card->grom_address + 0x10000*(port-24));
							}
							else
							{
								if (port==32 || port==33)
								{
									*value = card->gram[card->grom_address + 0x10000*(port-32)];
								}
								else
								{
									if (port==48 || port==49)
									{
										*value = card->ram6[card->grom_address];
									}
									else
									{
										logerror("hsgpl: Attempt to read from undefined port 0x%0x; ignored.\n", port);
									}
								}
							}
						}
					}
				}
			}
		}
		// The address auto-increment should be done even when the card is
		// offline
		card->grom_address++;
		card->raddr_LSB = card->waddr_LSB = FALSE;
	}
}

/*
    GROM write
*/
static WRITE8_DEVICE_HANDLER ( hsgpl_grom_w )
{
	int port;
	hsgpl_state *card = get_safe_token(device);

	//activedevice_adjust_icount(-4);

	// 1001 11bb bbbb bba0
	port = card->current_grom_port = (offset & 0x3fc) >> 2;

	if (offset & 2)
	{	// Write GPL address. This must be available even when the rest
		// of the card is offline (card_enabled=0).
		card->raddr_LSB = FALSE;

		if (card->waddr_LSB)
		{
			card->grom_address = (card->grom_address & 0xFF00) | data;
			card->waddr_LSB = FALSE;
		}
		else
		{
			card->grom_address = (data << 8) | (card->grom_address & 0xFF);
			card->waddr_LSB = TRUE;
		}
	}
	else
	{
		if (card->card_enabled)
		{
			/* write GPL data */
			if (card->write_enabled)
			{
				if ((port < 2) && (card->gram_enabled))
				{
					card->gram[card->grom_address + 0x10000*port] = data;
				}
				else
				{
					if (port < 8)
					{
						at29c040a_w(card->groma, card->grom_address + 0x10000*port, data);
					}
					else
					{
						if (port < 16)
						{
							at29c040a_w(card->gromb, card->grom_address + 0x10000*(port-8), data);
						}
						else
						{
							if (port < 24)
							{
								at29c040a_w(card->dsr, card->grom_address + 0x10000*(port-16), data);
							}
							else
							{
								if (port < 32)
								{
									/* The HSGPL manual says 32-47, but this is incorrect */
									at29c040a_w(card->rom6, card->grom_address + 0x10000*(port-24), data);
								}
								else
								{
									if (port==32 || port==33)
									{
										card->gram[card->grom_address + 0x10000*(port-32)] = data;
									}
									else
									{
										if (port==48 || port==49)
										{
											card->ram6[card->grom_address] = data;
										}
										else
										{
											logerror("hsgpl: Attempt to write to undefined port; ignored.\n");
										}
									}
								}
							}
						}
					}
				}
			}
		}
		// The address auto-increment should be done even when the card is
		// offline
		card->grom_address++;
		card->raddr_LSB = card->waddr_LSB = FALSE;
	}
}

static READ8Z_DEVICE_HANDLER ( hsgpl_dsrspace_rz )
{
	hsgpl_state *card = get_safe_token(device);
	if (card->dsr_enabled)
	{
		*value = at29c040a_r(card->dsr, (offset & 0x1fff) + 0x2000 * card->dsr_page);
//      logerror("hsgpl: read dsr %04x[%02x] -> %02x\n", offset, card->dsr_page, *value);
	}
}

static READ8Z_DEVICE_HANDLER ( hsgpl_cartspace_rz )
{
	hsgpl_state *card = get_safe_token(device);

	if (!card->card_enabled || card->flash_mode)
	{
//      logerror("hsgpl cart read ignored (enable=%02x)\n", card->card_enabled);
		return;
	}

	int port = card->current_grom_port;

	if ((port < 2) && (card->ram_enabled))
	{
		*value = card->ram6[(offset & 0x1fff) + 0x2000*card->current_bank + 0x8000*port];
//      logerror("hsgpl cart ram read %04x -> %02x\n", offset, *value);
		return;
	}

	if (port < 16)
	{
		*value = at29c040a_r(card->rom6, (offset & 0x1fff) + 0x2000*card->current_bank + 0x8000*port);
//      logerror("hsgpl cart read %04x -> %02x\n", offset, *value);
	}
	else
	{
		if (port==32 || port==33)
		{
			*value = card->ram6[(offset & 0x1fff) + 0x2000*card->current_bank + 0x8000*(port-32)];
		}
		else
		{
			logerror("unknown 0x6000 port\n");
		}
	}
}

static WRITE8_DEVICE_HANDLER ( hsgpl_cartspace_w )
{
	hsgpl_state *card = get_safe_token(device);

	if (!card->card_enabled || card->flash_mode)
	{
		logerror("hsgpl cart write ignored: card_enabled=%02x\n", card->card_enabled);
		return;
	}

	int port = card->current_grom_port;
//  logerror("hsgpl cart write %04x -> %02x\n", offset, data);

	if (card->bank_enabled)
	{
		card->current_bank = (offset>>1) & 3;
//      logerror("hsgpl cart select bank %02x\n", card->current_bank);
		return;		/* right??? */
	}

	if ((card->mbx_enabled) && (offset==0x6ffe))
	{	/* MBX: mapper at 0x6ffe */
		card->current_bank = data & 0x03;
		return;
	}

	// MBX: RAM in 0x6c00-0x6ffd (it is unclear whether the MBX RAM area is
	// enabled/disabled by the wriena bit).  I guess RAM is unpaged, but it is
	// not implemented
	if ((card->write_enabled) || ((card->mbx_enabled) && ((offset & 0xfc00)==0x6c00)))
	{
		if ((port < 2) && (card->ram_enabled))
		{
			card->ram6[(offset & 0x1fff) + 0x2000*card->current_bank + 0x8000*port ] = data;
		}
		else
		{	// keep in mind that these lines are also reached for port < 2
			// and !ram_enabled
			if (port < 16)
			{
			// feeprom is normally written to using GPL ports, and I don't know
			// writing through >6000 page is enabled
/*
            at29c040a_w(feeprom_rom6, 1 + 2*offset + 0x2000*hsgpl.cur_bank + 0x8000*port, data);
            at29c040a_w(feeprom_rom6, 2*offset + 0x2000*hsgpl.cur_bank + 0x8000*port, data >> 8);
*/
			}
			else
			{
				if (port==32 || port==33)
				{
					card->ram6[(offset & 0x1fff) + 0x2000*card->current_bank + 0x8000*(port-32)] = data;
				}
				else
				{
					logerror("unknown 0x6000 port\n");
				}
			}
		}
	}
}

/*
    Memory read
*/
static READ8Z_DEVICE_HANDLER ( data_rz )
{
	if ((offset & 0x7e000)==0x74000)
	{
		hsgpl_dsrspace_rz(device, offset & 0xffff, value);
	}

	if ((offset & 0x7e000)==0x76000)
	{
		hsgpl_cartspace_rz(device, offset & 0xffff, value);
	}

	// 1001 1wbb bbbb bba0
	if ((offset & 0x7fc01)==0x79800)
	{
		hsgpl_grom_rz(device, offset & 0xffff, value);
	}
}

/*
    Memory write
*/
static WRITE8_DEVICE_HANDLER ( data_w )
{
	if ((offset & 0x7e000)==0x76000)
	{
		hsgpl_cartspace_w(device, offset & 0xffff, data);
	}

	// 1001 1wbb bbbb bba0
	if ((offset & 0x7fc01)==0x79c00)
	{
		hsgpl_grom_w(device, offset & 0xffff, data);
	}
}

static const ti99_peb_card hsgpl_card =
{
	data_rz,
	data_w,
	NULL,
	cru_w,
	NULL, NULL,	NULL, NULL
};

static DEVICE_START( hsgpl )
{
	logerror("HSGPL start\n");
	hsgpl_state *card = get_safe_token(device);
	card->dsr = device->subdevice("u9_dsr");
	card->rom6 = device->subdevice("u6_rom6");
	card->groma = device->subdevice("u4_grom");
	card->gromb = device->subdevice("u1_grom");

	/* We need this for the NVRAM; at this point we cannot query the settings yet. */
	card->flashrom = (UINT8*)malloc(FLROMSIZE);
}

static DEVICE_STOP( hsgpl )
{
	hsgpl_state *card = get_safe_token(device);
	logerror("hsgpl: stop\n");
	free(card->flashrom);
	if (card->ram6)
	{
		free(card->ram6);
		free(card->gram);
	}
}

static DEVICE_RESET( hsgpl )
{
	logerror("hsgpl: reset\n");
	hsgpl_state *card = get_safe_token(device);

	/* If the card is selected in the menu, register the card */
	if (input_port_read(device->machine(), "EXTCARD") & (EXT_HSGPL_ON | EXT_HSGPL_FLASH))
	{
		device_t *peb = device->owner();
		int success = mount_card(peb, device, &hsgpl_card, get_pebcard_config(device)->slot);
		if (!success)
		{
			logerror("hsgpl: Failed to mount.\n");
			return;
		}

		card->grom_address = 0;
		card->raddr_LSB = 0;
		card->waddr_LSB = 0;
		card->current_grom_port = 0;
		card->current_bank = 0;

		card->dsr_enabled = FALSE;
		card->gram_enabled = FALSE;
		card->bank_enabled = TRUE;		// important, assumed to be enabled by default
		card->dsr_page = 0;
		card->card_enabled = TRUE;		// important, assumed to be enabled by default
		card->write_enabled = FALSE;
		card->supercart_enabled = FALSE;
		card->led_on = FALSE;
		card->mbx_enabled = FALSE;
		card->ram_enabled = FALSE;

		card->flash_mode = (input_port_read(device->machine(), "EXTCARD") & EXT_HSGPL_FLASH);
		if (card->flash_mode) logerror("hsgpl: flash mode\n");
		else logerror("hsgpl: full mode\n");

		if (card->ram6==NULL)
		{
			card->ram6 = (UINT8*)malloc(RAMSIZE);
			card->gram = (UINT8*)malloc(GRAMSIZE);
		}
	}
}

static DEVICE_NVRAM( hsgpl )
{
	// Called between START and RESET
	hsgpl_state *card = get_safe_token(device);
	astring *hsname = astring_assemble_3(astring_alloc(), device->machine().system().name, PATH_SEPARATOR, "hsgpl.nv");
	file_error filerr;

	if (read_or_write==0)
	{
		logerror("hsgpl: device nvram load %s\n", astring_c(hsname));

		emu_file nvfile(device->machine().options().nvram_directory(), OPEN_FLAG_READ);
		filerr = nvfile.open(astring_c(hsname));
		if (filerr != FILERR_NONE)
		{
			logerror("hsgpl: Could not restore NVRAM\n");
			return;
		}

		if (nvfile.read(card->flashrom, FLROMSIZE) != FLROMSIZE)
		{
			logerror("hsgpl: Error loading NVRAM; unexpected EOF\n");
			return;
		}

		if (card->flashrom[0] != NVVERSION)
		{
			logerror("hsgpl: Wrong NVRAM image version: %d\n", card->flashrom[0]);
			return;
		}
	}
	else
	{
		logerror("hsgpl: device nvram save %s\n", astring_c(hsname));
		// Check dirty flags
		if (at29c040a_is_dirty(card->dsr) ||
			at29c040a_is_dirty(card->rom6) ||
			at29c040a_is_dirty(card->groma) ||
			at29c040a_is_dirty(card->gromb))
		{
			card->flashrom[0] = NVVERSION;
			
			emu_file nvfile(device->machine().options().nvram_directory(), OPEN_FLAG_WRITE | OPEN_FLAG_CREATE | OPEN_FLAG_CREATE_PATHS);
			filerr = nvfile.open(astring_c(hsname));
			if (filerr != FILERR_NONE)
			{
				logerror("hsgpl: Could not save NVRAM\n");
			}
			else
			{
				if (nvfile.write(card->flashrom, FLROMSIZE) != FLROMSIZE)
				{
					logerror("hsgpl: Error while saving contents of NVRAM.\n");
				}
			}
			return;
		}
		else
		{
			logerror("hsgpl: No changes on card, leaving nvram file unchanged.\n");
		}
	}
}

/*
    Get the pointer to the memory data from the HSGPL card. Called by the FEEPROM.
*/
static UINT8 *get_mem_ptr(device_t *device)
{
	hsgpl_state *card = get_safe_token(device);
	return card->flashrom;
}

MACHINE_CONFIG_FRAGMENT( hsgpl )
	MCFG_AT29C040_ADD_P( "u9_dsr",  get_mem_ptr, 0x000001)
	MCFG_AT29C040_ADD_P( "u4_grom", get_mem_ptr, 0x080003)
	MCFG_AT29C040_ADD_P( "u1_grom", get_mem_ptr, 0x100005)
	MCFG_AT29C040_ADD_P( "u6_rom6", get_mem_ptr, 0x180007)
MACHINE_CONFIG_END

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)             p##hsgpl##s
#define DEVTEMPLATE_FEATURES            DT_HAS_START | DT_HAS_STOP | DT_HAS_RESET | DT_HAS_INLINE_CONFIG | DT_HAS_MACHINE_CONFIG | DT_HAS_NVRAM
#define DEVTEMPLATE_NAME                "SNUG High-Speed GPL card"
#define DEVTEMPLATE_FAMILY              "Peripheral expansion"
#include "devtempl.h"

DEFINE_LEGACY_NVRAM_DEVICE( HSGPL, hsgpl );
