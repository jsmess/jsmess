/***************************************************************************

	coco_pak.c

	Code for emulating standard CoCo cartridges

***************************************************************************/

#include "driver.h"
#include "device.h"
#include "cococart.h"


/*-------------------------------------------------
    pak_init - initializer for the PAKs
-------------------------------------------------*/

static void pak_init(coco_cartridge *cartridge)
{
	cococart_set_line(cartridge, COCOCART_LINE_CART, COCOCART_LINE_VALUE_Q);
}



/*-------------------------------------------------
    cococart_pak - get info function for standard
	CoCo cartridges
-------------------------------------------------*/

void cococart_pak(UINT32 state, cococartinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as pointers to data or functions --- */
		case COCOCARTINFO_PTR_INIT:							info->init = pak_init;	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case COCOCARTINFO_STR_NAME:							strcpy(info->s, "pak"); break;
	}
}



/***************************************************************************
	BANKED CARTRIDGES
***************************************************************************/

/*-------------------------------------------------
    banked_pak_set_bank - function to set the bank
-------------------------------------------------*/

static void banked_pak_set_bank(coco_cartridge *cartridge, UINT32 bank)
{
	cococart_map_memory(cartridge, bank * 0x4000, 0x3FFF);
}



/*-------------------------------------------------
    banked_pak_init
-------------------------------------------------*/

static void banked_pak_init(coco_cartridge *cartridge)
{
	pak_init(cartridge);
	banked_pak_set_bank(cartridge, 0);
}



/*-------------------------------------------------
    banked_pak_w - function to write to $FF40
-------------------------------------------------*/

static void banked_pak_w(coco_cartridge *cartridge, UINT16 addr, UINT8 data)
{
	switch(addr)
	{
		case 0:
			/* set the bank */
			banked_pak_set_bank(cartridge, data);
			break;
	}
}



/*-------------------------------------------------
    cococart_pak_banked16k - get info function for
	banked CoCo cartridges
-------------------------------------------------*/

void cococart_pak_banked16k(UINT32 state, cococartinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case COCOCARTINFO_PTR_INIT:							info->init = banked_pak_init;	break;
		case COCOCARTINFO_PTR_FF40_W:						info->wh = banked_pak_w;	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case COCOCARTINFO_STR_NAME:							strcpy(info->s, "pak_banked16k"); break;

		default: cococart_pak(state, info); break;
	}
}
