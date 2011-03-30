/*
    SNUG Enhanced Video Processor Card (evpc)
    based on v9938 (may also be equipped with v9958)
    Can be used with TI-99/4A as an add-on card; internal VDP must be removed

    The SGCPU ("TI-99/4P") only runs with EVPC.
    Michael Zapf
    Rewritten as device
    October 2010
*/
#include "emu.h"
#include "peribox.h"
#include "evpc.h"
#include "emuopts.h"

#define evpc_region "evpc_region"

#define EVPC_CRU_BASE 0x1400

typedef ti99_pebcard_config ti99_evpc_config;

typedef struct _evpc_palette
{
	UINT8		read_index, write_index, mask;
	int 		read;
	int 		state;
	struct { UINT8 red, green, blue; } color[0x100];
	//int dirty;
} evpc_palette;

typedef struct _ti99_evpc_state
{
	int 					selected;
	UINT8					*dsrrom;
	int 					RAMEN;
	int 					dsr_page;
	UINT8					*novram;	/* NOVRAM area */
	evpc_palette			palette;
	ti99_peb_connect		lines;

} ti99_evpc_state;

INLINE ti99_evpc_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == EVPC);

	return (ti99_evpc_state *)downcast<legacy_device_base *>(device)->token();
}

static int get_evpc_switch(device_t *device, int number)
{
	switch (number)
	{
	case 0:
		return input_port_read(device->machine(), "EVPC-SW1");
	case 1:
		return input_port_read(device->machine(), "EVPC-SW3");
	case 2:
		return input_port_read(device->machine(), "EVPC-SW4");
	case 3:
		return input_port_read(device->machine(), "EVPC-SW8");
	default:
		logerror("evpc: Invalid switch index %02x\n", number);
		return 0;
	}
}

/*************************************************************************/

/*
    The CRU read handler. The CRU is a serial interface in the console.

    Read EVPC CRU interface (dip switches)
    0: Video timing (PAL/NTSC)
    1: -
    2: charset
    3: RAM shift
    4: -
    5: -
    6: -
    7: DIP or NOVRAM
    Logic is inverted
*/
static READ8Z_DEVICE_HANDLER( cru_rz )
{
	if ((offset & 0xff00)==EVPC_CRU_BASE)
	{
		int bit = (offset >> 1) & 0x0f;
		if (bit == 0)
			*value = ~(get_evpc_switch(device, 0) | (get_evpc_switch(device, 1)<<2) | (get_evpc_switch(device, 2)<<3) | (get_evpc_switch(device, 3)<<7));
	}
}

/*
    The CRU write handler. The CRU is a serial interface in the console.
*/
static WRITE8_DEVICE_HANDLER( cru_w )
{
	ti99_evpc_state *card = get_safe_token(device);

	if ((offset & 0xff00)==EVPC_CRU_BASE)
	{
		int bit = (offset >> 1) & 0x0f;
		switch (bit)
		{
		case 0:
			card->selected = data;
			break;

		case 1:
			if (data)
				card->dsr_page |= 1;
			else
				card->dsr_page &= ~1;
			break;

		case 2:
			break;

		case 3:
			card->RAMEN = data;
			break;

		case 4:
			if (data)
				card->dsr_page |= 4;
			else
				card->dsr_page &= ~4;
			break;

		case 5:
			if (data)
				card->dsr_page |= 2;
			else
				card->dsr_page &= ~2;
			break;

		case 6:
			break;

		case 7:
			break;
		}
	}
}

/*
    Read a byte in evpc DSR space
    0x4000 - 0x5eff   DSR (paged)
    0x5f00 - 0x5fef   NOVRAM
    0x5ff0 - 0x5fff   Palette
*/
static READ8Z_DEVICE_HANDLER( data_rz )
{
	ti99_evpc_state *card = get_safe_token(device);

	if (card->selected)
	{
		if (card->dsrrom==NULL)
		{
			logerror("evpc: no dsrrom\n");
			return;
		}

		if ((offset & 0x7e000)==0x74000)
		{
			if ((offset & 0x1ff0)==0x1ff0)
			{
				/* PALETTE */
				switch (offset & 0x000f)
				{
				case 0:
					/* Palette Read Address Register */
					*value = card->palette.write_index;
					break;

				case 2:
					/* Palette Read Color Value */
					if (card->palette.read)
					{
						switch (card->palette.state)
						{
						case 0:
							*value = card->palette.color[card->palette.read_index].red;
							break;
						case 1:
							*value = card->palette.color[card->palette.read_index].green;
							break;
						case 2:
							*value = card->palette.color[card->palette.read_index].blue;
							break;
						}
						card->palette.state++;
						if (card->palette.state == 3)
						{
							card->palette.state = 0;
							card->palette.read_index++;
						}
					}
					break;

				case 4:
					/* Palette Read Pixel Mask */
					*value = card->palette.mask;
					break;
				case 6:
					/* Palette Read Address Register for Color Value */
					if (card->palette.read)
						*value = 0;
					else
						*value = 3;
					break;
				}
			}
			else
			{
				if ((offset & 0x1f00)==0x1f00)
				{
					/* NOVRAM */
					if (card->RAMEN)
					{
						*value = card->novram[offset & 0x00ff];
					}
					else
					{
						*value = card->dsrrom[(offset&0x1fff) + card->dsr_page*0x2000];
					}
				}
				else
				{
					*value = card->dsrrom[(offset&0x1fff) + card->dsr_page*0x2000];
				}
			}
		}
	}
}

/*
    Write a byte in evpc DSR space
    0x4000 - 0x5eff   DSR (paged)
    0x5f00 - 0x5fef   NOVRAM
    0x5ff0 - 0x5fff   Palette
*/
static WRITE8_DEVICE_HANDLER( data_w )
{
	ti99_evpc_state *card = get_safe_token(device);
	if (card->selected)
	{
		if ((offset & 0x7e000)==0x74000)
		{
			if ((offset & 0x1ff0)==0x1ff0)
			{
				/* PALETTE */
				logerror("palette write, offset=%d\n, data=%d", offset-0x5ff0, data);
				switch (offset & 0x000f)
				{
				case 8:
					/* Palette Write Address Register */
					logerror("EVPC palette address write (for write access)\n");
					card->palette.write_index = data;
					card->palette.state = 0;
					card->palette.read = 0;
					break;

				case 10:
					/* Palette Write Color Value */
					logerror("EVPC palette color write\n");
					if (!card->palette.read)
					{
						switch (card->palette.state)
						{
						case 0:
							card->palette.color[card->palette.write_index].red = data;
							break;
						case 1:
							card->palette.color[card->palette.write_index].green = data;
							break;
						case 2:
							card->palette.color[card->palette.write_index].blue = data;
							break;
						}
						card->palette.state++;
						if (card->palette.state == 3)
						{
							card->palette.state = 0;
							card->palette.write_index++;
						}
						//evpc_palette.dirty = 1;
					}
					break;

				case 12:
					/* Palette Write Pixel Mask */
					logerror("EVPC palette mask write\n");
					card->palette.mask = data;
					break;

				case 14:
					/* Palette Write Address Register for Color Value */
					logerror("EVPC palette address write (for read access)\n");
					card->palette.read_index = data;
					card->palette.state = 0;
					card->palette.read = 1;
					break;
				}
			}
			else
			{
				if ((offset & 0x1f00)==0x1f00)
				{
					if (card->RAMEN)
					{
						// NOVRAM
						card->novram[offset & 0x00ff] = data;
					}
				}
			}
		}
	}
}

static const ti99_peb_card evpc_card =
{
	data_rz,
	data_w,
	cru_rz,
	cru_w,

	NULL, NULL,	NULL, NULL
};

static DEVICE_START( ti99_evpc )
{
	ti99_evpc_state *card = get_safe_token(device);
	card->novram = (UINT8*)malloc(256); // need that already now for NVRAM handling

	/* Resolve the callbacks to the PEB */
	peb_callback_if *topeb = (peb_callback_if *)device->baseconfig().static_config();
	devcb_resolve_write_line(&card->lines.ready, &topeb->ready, device);
}

static DEVICE_STOP( ti99_evpc )
{
	logerror("ti99_evpc: stop\n");
	ti99_evpc_state *card = get_safe_token(device);
	free(card->novram);
}

static DEVICE_RESET( ti99_evpc )
{
	logerror("ti99_evpc: reset\n");
	ti99_evpc_state *card = get_safe_token(device);
	astring *region = new astring();

	/* If the card is selected in the menu, register the card */
	device_t *peb = device->owner();
	int success = mount_card(peb, device, &evpc_card, get_pebcard_config(device)->slot);
	if (!success)
	{
		logerror("evpc: Could not mount card.\n");
		return;
	}
	card->RAMEN = 0;
	card->dsr_page = 0;

	astring_assemble_3(region, device->tag(), ":", evpc_region);

	card->dsrrom = device->machine().region(astring_c(region))->base();
}

static DEVICE_NVRAM( ti99_evpc )
{
	// Called between START and RESET
	ti99_evpc_state *card = get_safe_token(device);
	astring *hsname = astring_assemble_3(astring_alloc(), device->machine().system().name, PATH_SEPARATOR, "evpc.nv");
	file_error filerr;

	if (read_or_write==0)
	{
		logerror("evpc: device nvram load %s\n", astring_c(hsname));

		emu_file nvfile(device->machine().options().nvram_directory(), OPEN_FLAG_READ);
		filerr = nvfile.open(astring_c(hsname));
		if (filerr == FILERR_NONE)
		{
			if (nvfile.read(card->novram, 256) != 256)
				logerror("evpc: NOVRAM load error\n");
		}
	}
	else
	{
		logerror("evpc: device nvram save %s\n", astring_c(hsname));
		emu_file nvfile(device->machine().options().nvram_directory(), OPEN_FLAG_WRITE | OPEN_FLAG_CREATE | OPEN_FLAG_CREATE_PATHS);
		filerr = nvfile.open(astring_c(hsname));

		if (filerr == FILERR_NONE)
		{
			if (nvfile.write(card->novram, 256) != 256)
				logerror("evpc: NOVRAM save error\n");
		}
	}
}

MACHINE_CONFIG_FRAGMENT( ti99_evpc )
MACHINE_CONFIG_END

ROM_START( ti99_evpc )
	ROM_REGION(0x10000, evpc_region, 0)
	ROM_LOAD_OPTIONAL("evpcdsr.bin", 0, 0x10000, CRC(a062b75d) SHA1(6e8060f86e3bb9c36f244d88825e3fe237bfe9a9)) /* evpc DSR ROM */
ROM_END

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)             p##ti99_evpc##s
#define DEVTEMPLATE_FEATURES            DT_HAS_START | DT_HAS_STOP | DT_HAS_RESET | DT_HAS_ROM_REGION | DT_HAS_INLINE_CONFIG | DT_HAS_MACHINE_CONFIG | DT_HAS_NVRAM
#define DEVTEMPLATE_NAME                "SNUG Enhanced Video Processor Card"
#define DEVTEMPLATE_SHORTNAME           "snugvdc"
#define DEVTEMPLATE_FAMILY              "Peripheral expansion"
#include "devtempl.h"

DEFINE_LEGACY_NVRAM_DEVICE(EVPC, ti99_evpc);
