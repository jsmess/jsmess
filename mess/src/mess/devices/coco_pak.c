/***************************************************************************

    coco_pak.c

    Code for emulating standard CoCo cartridges

***************************************************************************/

#include "emu.h"
#include "cococart.h"


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _coco_pak_pcb_t coco_pak_pcb_t;
struct _coco_pak_pcb_t
{
	device_t *cococart;
	device_image_interface *cart;
};


/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE coco_pak_pcb_t *get_token(device_t *device)
{
	assert(device != NULL);
	assert((device->type() == COCO_CARTRIDGE_PCB_PAK) || (device->type() == COCO_CARTRIDGE_PCB_PAK_BANKED16K));
	return (coco_pak_pcb_t *) downcast<legacy_device_base *>(device)->token();
}


/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    DEVICE_START(coco_pak) - initializer for the PAKs
-------------------------------------------------*/

static DEVICE_START(coco_pak)
{
	coco_pak_pcb_t *pak_pcb = get_token(device);

	memset(pak_pcb, 0, sizeof(*pak_pcb));
	pak_pcb->cococart = device->owner()->owner();
	pak_pcb->cart = dynamic_cast<device_image_interface *>(device->owner());
}


/*-------------------------------------------------
    DEVICE_RESET(coco_pak) - initializer for the PAKs
-------------------------------------------------*/

static DEVICE_RESET(coco_pak)
{
	coco_pak_pcb_t *pak_pcb = get_token(device);
	cococart_line_value cart_line;

	cart_line = input_port_read_safe(device->machine(), "CARTAUTO", 0x01)
		? COCOCART_LINE_VALUE_Q
		: COCOCART_LINE_VALUE_CLEAR;

	/* normal CoCo PAKs tie their CART line to Q - the system clock */
	coco_cartridge_set_line(
		pak_pcb->cococart,
		COCOCART_LINE_CART,
		cart_line);
}


/*-------------------------------------------------
    DEVICE_GET_INFO(coco_cartridge_pcb_pak) - get
    info function for standard CoCo cartridges
-------------------------------------------------*/

DEVICE_GET_INFO(coco_cartridge_pcb_pak)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(coco_pak_pcb_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;

		/* --- the following bits of info are returned as pointers to functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(coco_pak);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(coco_pak);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "CoCo Program PAK");		break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "CoCo Program PAK");		break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						/* Nothing */								break;
	}
}



/***************************************************************************
    BANKED CARTRIDGES
***************************************************************************/

/*-------------------------------------------------
    banked_pak_set_bank - function to set the bank
-------------------------------------------------*/

static void banked_pak_set_bank(device_t *device, UINT32 bank)
{
	coco_pak_pcb_t *pak_pcb = get_token(device);

	UINT64 pos;
	UINT32 i;
	UINT8 *rom = device->machine().region("cart")->base();
	UINT32 rom_length = device->machine().region("cart")->bytes();

	pos = (bank * 0x4000) % pak_pcb->cart->length();

	for (i = 0; i < rom_length / 0x4000; i++)
	{
		pak_pcb->cart->fseek(pos, SEEK_SET);
		pak_pcb->cart->fread(&rom[i * 0x4000], 0x4000);
	}

}



/*-------------------------------------------------
    DEVICE_RESET(banked_pak)
-------------------------------------------------*/

static DEVICE_RESET(banked_pak)
{
	DEVICE_RESET_CALL(coco_pak);

	banked_pak_set_bank(device, 0);
}



/*-------------------------------------------------
    banked_pak_w - function to write to $FF40
-------------------------------------------------*/

static WRITE8_DEVICE_HANDLER( banked_pak_w )
{
	switch(offset)
	{
		case 0:
			/* set the bank */
			banked_pak_set_bank(device, data);
			break;
	}
}



/*-------------------------------------------------
    DEVICE_GET_INFO(coco_cartridge_pcb_pak_banked16k) -
    get info function for banked CoCo cartridges
-------------------------------------------------*/

DEVICE_GET_INFO(coco_cartridge_pcb_pak_banked16k)
{
	switch(state)
	{
		/* --- the following bits of info are returned as pointers to functions --- */
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(banked_pak);	break;
		case COCOCARTINFO_FCT_FF40_W:					info->f = (genf *) banked_pak_w; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "CoCo Program PAK (Banked)");		break;

		default: DEVICE_GET_INFO_CALL(coco_cartridge_pcb_pak); break;
	}
}

DEFINE_LEGACY_CART_SLOT_DEVICE(COCO_CARTRIDGE_PCB_PAK, coco_cartridge_pcb_pak);
DEFINE_LEGACY_CART_SLOT_DEVICE(COCO_CARTRIDGE_PCB_PAK_BANKED16K, coco_cartridge_pcb_pak_banked16k);
