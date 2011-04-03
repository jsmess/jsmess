/*
    Geneve "Memex" memory expansion
    Michael Zapf, February 2011
*/

#include "emu.h"
#include "peribox.h"
#include "memex.h"

typedef ti99_pebcard_config gen_memex_config;

typedef struct _gen_memex_state
{
	UINT8	*memory;
	UINT8	dip_switch[8];
	int		genmod;
} gen_memex_state;

INLINE gen_memex_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == GENMEMEX);

	return (gen_memex_state *)downcast<legacy_device_base *>(device)->token();
}

static int access_enabled(gen_memex_state *card, offs_t offset)
{
	// 1 0111 .... .... .... .... p-box address block 0xxx ... fxxx
	// first two bits are AME, AMD bits available on Genmod only
	// if AMD, AME are not available we assume AMD=0, AME=1
	// must be set on the Geneve board
	// Some traditional cards will not decode the AMx lines, so
	// we may have to lock out those areas
	int page = (offset >> 13)&0xff;
	int index = 0;

	// SW2: "off" locks
	//      10xxx010
	//      10111010   also locked when "on"
	if (page == 0xba) return FALSE;
	if ((page & 0xc7)==0x82 && card->dip_switch[1]==FALSE)
		return FALSE;

	// SW3: 111010xx    0=enabled 1=locked out
	// SW4: 111011xx
	// SW5: 111100xx
	// SW6: 111101xx
	// SW7: 111110xx
	// SW8: 111111xx

	index = ((page >> 2)&0x3f);
	if (index >= 0x3a && index <= 0x3f)
	{
		if (card->dip_switch[index - 0x38]==0) return TRUE;
		else
		{
			return FALSE;
		}
	}
	return TRUE;
}

/*
    Memory read. The memory is at locations 0x000000-0x1fffff. Some of these
    regions are hidden by onboard devices of the Geneve. We must block some
    areas which would otherwise interfere with peripheral cards.

    Note that the incomplete decoding of the standard Geneve must be
    considered.
*/
static READ8Z_DEVICE_HANDLER( memex_rz )
{
	gen_memex_state *card = get_safe_token(device);

	/* If not Genmod, add the upper two address bits 10 */
	if (!card->genmod) offset |= 0x100000;

	// The card is accessed for all addresses in the address space
	if (access_enabled(card, offset))
	{
		*value = card->memory[offset];
	}
}

/*
    Memory write
*/
static WRITE8_DEVICE_HANDLER( memex_w )
{
	gen_memex_state *card = get_safe_token(device);

	/* If not Genmod, add the upper two address bits 10 */
	if (!card->genmod) offset |= 0x100000;

	// The card is accessed for all addresses in the address space
	if (access_enabled(card, offset))
	{
		card->memory[offset] = data;
	}
}

/**************************************************************************/

static const ti99_peb_card memex_card =
{
	memex_rz, memex_w,				// memory access read/write
	NULL, NULL,						// CRU access (none here)
	NULL, NULL,						// SENILA/B access (none here)
	NULL, NULL						// 16 bit access (none here)
};

static DEVICE_START( gen_memex )
{
	gen_memex_state *card = get_safe_token(device);
	card->memory=NULL;
}

static DEVICE_STOP( gen_memex )
{
	gen_memex_state *card = get_safe_token(device);
	if (card->memory) free(card->memory);
}

static DEVICE_RESET( gen_memex )
{
	gen_memex_state *card = get_safe_token(device);
	/* Register the card */
	device_t *peb = device->owner();

	if (input_port_read(device->machine(), "EXTRAM")==1)
	{
		int success = mount_card(peb, device, &memex_card, get_pebcard_config(device)->slot);
		if (!success) return;

		if (card->memory==NULL)
		{
			// Allocate 2 MiB
			card->memory = (UINT8*)malloc(0x200000);
		}

		UINT8 dips = input_port_read(device->machine(), "MEMEXDIPS");
		for (int i=0; i < 8; i++)
		{
			card->dip_switch[i] = ((dips & 0x01)!=0x00);
			dips = dips >> 1;
		}

		card->genmod = input_port_read(device->machine(), "MODE");
	}
}

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)             p##gen_memex##s
#define DEVTEMPLATE_FEATURES            DT_HAS_START | DT_HAS_STOP | DT_HAS_RESET | DT_HAS_INLINE_CONFIG
#define DEVTEMPLATE_NAME                "Geneve Memex Memory Expansion Card"
#define DEVTEMPLATE_FAMILY              "Peripheral expansion"
#include "devtempl.h"

DEFINE_LEGACY_DEVICE( GENMEMEX, gen_memex );

