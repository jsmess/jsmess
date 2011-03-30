/*
    TI-99 Myarc memory expansion

    Note that the DSR part is still missing
    TODO: Test this device with some emulated program!
*/
#include "emu.h"
#include "peribox.h"
#include "myarcmem.h"

typedef ti99_pebcard_config ti99_myarcmem_config;

typedef struct _ti99_myarcmem_state
{
	int 	bank;
	UINT8	*memory;

} ti99_myarcmem_state;

INLINE ti99_myarcmem_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == MYARCMEM);

	return (ti99_myarcmem_state *)downcast<legacy_device_base *>(device)->token();
}

#define MYARCMEM_CRU_BASE1 0x1000
#define MYARCMEM_CRU_BASE2 0x1900

/*
    CRU write (is there a read?)
*/
static WRITE8_DEVICE_HANDLER( myarcmem_cru_w )
{
	ti99_myarcmem_state *card = get_safe_token(device);
	if (((offset & 0xff00)==MYARCMEM_CRU_BASE1)||((offset & 0xff00)==MYARCMEM_CRU_BASE2))
	{
		if ((offset & 0x001e)==0)
		{
			logerror("Activate Myarc memory expansion DSR ROM. Not available.\n");
		}
		else
		{
			// offset / 2 = cru bit
			// bit 1 = 1; bit 2 = 2; bit 3 = 4; bit 4 = 8
			UINT8 mask = 1<<((offset>>1)-1);
			if (data==0)
				card->bank &= ~mask;
			else
			{
				card->bank |= mask;
			}
		}
	}
}

/*
    Memory read. The memory is at locations 0x2000-0x3fff and 0xa000-0xffff
*/
static READ8Z_DEVICE_HANDLER( myarcmem_rz )
{
	ti99_myarcmem_state *card = get_safe_token(device);
	if (((offset & 0xe000)==0x2000) || ((offset & 0xe000)==0xa000) || ((offset & 0xc000)==0xc000))
	{
		if (offset < 0xa000)
			*value = card->memory[offset-0x2000 + card->bank*32768];
		else
			*value = card->memory[offset-0x8000 + card->bank*32768];
	}
}

/*
    Memory write
*/
static WRITE8_DEVICE_HANDLER( myarcmem_w )
{
	ti99_myarcmem_state *card = get_safe_token(device);
	if (((offset & 0xe000)==0x2000) || ((offset & 0xe000)==0xa000) || ((offset & 0xc000)==0xc000))
	{
		if (offset < 0xa000)
			card->memory[offset-0x2000 + card->bank*32768] = data;
		else
			card->memory[offset-0x8000 + card->bank*32768] = data;
	}
}

/**************************************************************************/

static const ti99_peb_card myarcmem_card =
{
	myarcmem_rz, myarcmem_w,			// memory access read/write
	NULL, myarcmem_cru_w,			// CRU access (no read here)
	NULL, NULL,						// SENILA/B access (none here)
	NULL, NULL						// 16 bit access (none here)
};

static DEVICE_START( ti99_myarcmem )
{
	ti99_myarcmem_state *card = get_safe_token(device);
	card->memory=NULL;
}

static DEVICE_STOP( ti99_myarcmem )
{
	ti99_myarcmem_state *card = get_safe_token(device);
	if (card->memory) free(card->memory);
}

static DEVICE_RESET( ti99_myarcmem )
{
	ti99_myarcmem_state *card = get_safe_token(device);
	/* Register the card */
	device_t *peb = device->owner();

	if (input_port_read(device->machine(), "RAM")==RAM_MYARC512)
	{
		int success = mount_card(peb, device, &myarcmem_card, get_pebcard_config(device)->slot);
		if (!success) return;

		if (card->memory==NULL)
		{
			// Allocate 512 KiB
			card->memory = (UINT8*)malloc(0x080000);
		}
		card->bank = 0;
	}
}

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)             p##ti99_myarcmem##s
#define DEVTEMPLATE_FEATURES            DT_HAS_START | DT_HAS_STOP | DT_HAS_RESET | DT_HAS_INLINE_CONFIG
#define DEVTEMPLATE_NAME                "TI99 Myarc/Foundation Memory Expansion Card"
#define DEVTEMPLATE_FAMILY              "Peripheral expansion"
#include "devtempl.h"

DEFINE_LEGACY_DEVICE( MYARCMEM, ti99_myarcmem );

