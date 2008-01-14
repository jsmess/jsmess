/***************************************************************************

	orch90.c

	Code for emulating the CoCo Orch-90 (Orchestra 90) sound cartridge

	The Orch-90 was a simple sound cartridge; it had two 8-bit DACs
	supporting stereo sound.  The left channel was at $FF7A, and the right
	channel was at $FF7B

***************************************************************************/

#include "driver.h"
#include "cococart.h"
#include "sound/dac.h"


typedef struct _orch90_info orch90_info;
struct _orch90_info
{
	int left_dac;
	int right_dac;
};



/*-------------------------------------------------
    orch90_get_info
-------------------------------------------------*/

static orch90_info *orch90_get_info(coco_cartridge *cartridge)
{
	return cococart_get_extra_data(cartridge);
}



/*-------------------------------------------------
    orch90_init - initializer for the Orch-90
-------------------------------------------------*/

static void orch90_init(coco_cartridge *cartridge)
{
	orch90_info *info = orch90_get_info(cartridge);

	/* TODO - when we can instantiate DACs, we can do something better here */
	info->left_dac = 0;
	info->right_dac = 0;
}



/*-------------------------------------------------
    orch90_w - function to write to the Orch-90
-------------------------------------------------*/

static void orch90_w(coco_cartridge *cartridge, UINT16 addr, UINT8 data)
{
	orch90_info *info = orch90_get_info(cartridge);

	switch(addr)
	{
		case 0x3A:
			/* left channel write */
			DAC_data_w(info->left_dac, data);
			break;

		case 0x3B:
			/* right channel write */
			DAC_data_w(info->right_dac, data);
			break;
	}
}



/*-------------------------------------------------
    cococart_orch90 - get info function for the
	Orch-90
-------------------------------------------------*/

void cococart_orch90(UINT32 state, cococartinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case COCOCARTINFO_INT_DATASIZE:						info->i = sizeof(orch90_info);	break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case COCOCARTINFO_PTR_INIT:							info->init = orch90_init;	break;
		case COCOCARTINFO_PTR_FF40_W:						info->wh = orch90_w;	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case COCOCARTINFO_STR_NAME:							strcpy(info->s, "orch90"); break;
	}
}

