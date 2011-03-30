/*
    TI-99 SuperAMS Memory Expansion Card. Uses a 74LS612 memory mapper.

    TODO: Test this device with some emulated program!
*/
#include "emu.h"
#include "peribox.h"
#include "samsmem.h"

typedef ti99_pebcard_config ti99_samsmem_config;

typedef struct _ti99_samsmem_state
{
	UINT8	mapper[16];
	int 	map_mode;
	int		access_mapper;
	UINT8	*memory;

} ti99_samsmem_state;

INLINE ti99_samsmem_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == SAMSMEM);

	return (ti99_samsmem_state *)downcast<legacy_device_base *>(device)->token();
}

#define SAMS_CRU_BASE 0x1e00

/*
    CRU write (there is no read here)
*/
static WRITE8_DEVICE_HANDLER( samsmem_cru_w )
{
	ti99_samsmem_state *card = get_safe_token(device);
	if ((offset & 0xff00)==SAMS_CRU_BASE)
	{
		if ((offset & 0x000e)==0) card->access_mapper = data;
		if ((offset & 0x000e)==2) card->map_mode = data;
	}
}

/*
    Memory read. The SAMS card has two address areas: The memory is at locations
    0x2000-0x3fff and 0xa000-0xffff, and the mapper area is at 0x4000-0x401e
    (only even addresses).
*/
static READ8Z_DEVICE_HANDLER( samsmem_rz )
{
	ti99_samsmem_state *card = get_safe_token(device);
	UINT32 address = 0;

	if (card->access_mapper && ((offset & 0xe000)==0x4000))
	{
		// select the mapper circuit
		*value = card->mapper[(offset>>1)&0x000f];
	}

	if (((offset & 0xe000)==0x2000) || ((offset & 0xe000)==0xa000) || ((offset & 0xc000)==0xc000))
	{
		// select memory expansion
		if (card->map_mode)
			address = (card->mapper[offset>>12] << 12) + (offset & 0x0fff);
		else // transparent mode
			address = offset;
		*value = card->memory[address];
	}
}

/*
    Memory write
*/
static WRITE8_DEVICE_HANDLER( samsmem_w )
{
	ti99_samsmem_state *card = get_safe_token(device);
	UINT32 address = 0;

	if (card->access_mapper && ((offset & 0xe000)==0x4000))
	{
		// select the mapper circuit
		card->mapper[(offset>>1)&0x000f] = data;
	}
	if (((offset & 0xe000)==0x2000) || ((offset & 0xe000)==0xa000) || ((offset & 0xc000)==0xc000))
	{
		// select memory expansion
		if (card->map_mode)
			address = (card->mapper[offset>>12] << 12) + (offset & 0x0fff);
		else // transparent mode
			address = offset;
		card->memory[address] = data;
	}
}

/**************************************************************************/

static const ti99_peb_card samsmem_card =
{
	samsmem_rz, samsmem_w,			// memory access read/write
	NULL, samsmem_cru_w,			// CRU access (no read here)
	NULL, NULL,						// SENILA/B access (none here)
	NULL, NULL						// 16 bit access (none here)
};

static DEVICE_START( ti99_samsmem )
{
	ti99_samsmem_state *card = get_safe_token(device);
	card->memory=NULL;
}

static DEVICE_STOP( ti99_samsmem )
{
	ti99_samsmem_state *card = get_safe_token(device);
	if (card->memory) free(card->memory);
}

static DEVICE_RESET( ti99_samsmem )
{
	ti99_samsmem_state *card = get_safe_token(device);
	/* Register the card */
	device_t *peb = device->owner();

	if (input_port_read(device->machine(), "RAM")==RAM_SUPERAMS1024)
	{
		int success = mount_card(peb, device, &samsmem_card, get_pebcard_config(device)->slot);
		if (!success) return;

		if (card->memory==NULL)
		{
			// Allocate 1 MiB (check whether we want to allocate this on demand)
			card->memory = (UINT8*)malloc(0x100000);
		}
		card->map_mode = FALSE;
		card->access_mapper = FALSE;
		for (int i=0; i < 16; i++) card->mapper[i]=0;
	}
}

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)             p##ti99_samsmem##s
#define DEVTEMPLATE_FEATURES            DT_HAS_START | DT_HAS_STOP | DT_HAS_RESET | DT_HAS_INLINE_CONFIG
#define DEVTEMPLATE_NAME                "TI99 SuperAMS Memory Expansion Card"
#define DEVTEMPLATE_FAMILY              "Peripheral expansion"
#include "devtempl.h"

DEFINE_LEGACY_DEVICE( SAMSMEM, ti99_samsmem );

