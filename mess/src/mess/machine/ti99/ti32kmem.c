/*
    TI-99 32 KiB Memory Expansion Card
*/
#include "emu.h"
#include "peribox.h"
#include "ti32kmem.h"

typedef ti99_pebcard_config ti99_mem32k_config;

typedef struct _ti99_mem32k_state
{
	UINT8 *memory;

} ti99_mem32k_state;

INLINE ti99_mem32k_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == TI32KMEM);

	return (ti99_mem32k_state *)downcast<legacy_device_base *>(device)->token();
}

/*
    Memory read
*/
static READ8Z_DEVICE_HANDLER( mem32k_rz )
{
	ti99_mem32k_state *card = get_safe_token(device);
	if (((offset & 0x7e000)==0x72000) || ((offset & 0x7e000)==0x7a000) || ((offset & 0x7c000)==0x7c000))
	{
		if ((offset & 0xffff) < 0xa000)
			*value = card->memory[offset & 0x1fff];
		else
			*value = card->memory[offset & 0x7fff];
	}
}

/*
    Memory write
*/
static WRITE8_DEVICE_HANDLER( mem32k_w )
{
	ti99_mem32k_state *card = get_safe_token(device);
	if (((offset & 0x7e000)==0x72000) || ((offset & 0x7e000)==0x7a000) || ((offset & 0x7c000)==0x7c000))
	{
		if ((offset & 0xffff) < 0xa000)
			card->memory[offset & 0x1fff] = data;
		else
			card->memory[offset & 0x7fff] = data;
	}
}

/**************************************************************************/

static const ti99_peb_card ti32k_card =
{
	mem32k_rz, mem32k_w,			// memory access read/write
	NULL, NULL,						// CRU access (none here)
	NULL, NULL,						// SENILA/B access (none here)
	NULL, NULL						// 16 bit access (none here)
};

static DEVICE_START( ti99_mem32k )
{
	ti99_mem32k_state *card = get_safe_token(device);
	card->memory = NULL;
}

static DEVICE_STOP( ti99_mem32k )
{
	ti99_mem32k_state *card = get_safe_token(device);
	if (card->memory) free(card->memory);
}

static DEVICE_RESET( ti99_mem32k )
{
	ti99_mem32k_state *card = get_safe_token(device);
	/* Register the card */
	device_t *peb = device->owner();

	if (input_port_read(device->machine(), "RAM")==RAM_TI32_EXT)
	{
		int success = mount_card(peb, device, &ti32k_card, get_pebcard_config(device)->slot);
		if (!success) return;

		if (card->memory==NULL) card->memory = (UINT8*)malloc(32768);
	}
}

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)             p##ti99_mem32k##s
#define DEVTEMPLATE_FEATURES            DT_HAS_START | DT_HAS_STOP | DT_HAS_RESET | DT_HAS_INLINE_CONFIG
#define DEVTEMPLATE_NAME                "TI99 32KiB Memory Expansion Card"
#define DEVTEMPLATE_FAMILY              "Peripheral expansion"
#include "devtempl.h"

DEFINE_LEGACY_DEVICE( TI32KMEM, ti99_mem32k );

