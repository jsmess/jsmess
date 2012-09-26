/***************************************************************************

    orch90.c

    Code for emulating the CoCo Orch-90 (Orchestra 90) sound cartridge

    The Orch-90 was a simple sound cartridge; it had two 8-bit DACs
    supporting stereo sound.  The left channel was at $FF7A, and the right
    channel was at $FF7B

***************************************************************************/

#include "emu.h"
#include "cococart.h"
#include "sound/dac.h"


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _orch90_t orch90_t;
struct _orch90_t
{
	device_t *left_dac;
	device_t *right_dac;
};



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE orch90_t *get_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == COCO_CARTRIDGE_PCB_ORCH90);
	return (orch90_t *) downcast<legacy_device_base *>(device)->token();
}


/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    DEVICE_START(orch90) - initializer for the Orch-90
-------------------------------------------------*/

static DEVICE_START(orch90)
{
	orch90_t *info = get_token(device);

	/* TODO - when we can instantiate DACs, we can do something better here */
	info->left_dac = device->machine().device("dac");
	info->right_dac = device->machine().device("dac");
}



/*-------------------------------------------------
    orch90_w - function to write to the Orch-90
-------------------------------------------------*/

static WRITE8_DEVICE_HANDLER( orch90_w )
{
	orch90_t *info = get_token(device);

	switch(offset)
	{
		case 0x3A:
			/* left channel write */
			dac_data_w(info->left_dac, data);
			break;

		case 0x3B:
			/* right channel write */
			dac_data_w(info->right_dac, data);
			break;
	}
}



/*-------------------------------------------------
    DEVICE_GET_INFO(coco_cartridge_pcb_orch90) -
    get info function for the Orch-90
-------------------------------------------------*/

DEVICE_GET_INFO(coco_cartridge_pcb_orch90)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(orch90_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;							break;

		/* --- the following bits of info are returned as pointers to functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(orch90);break;
		case DEVINFO_FCT_STOP:							/* Nothing */							break;
		case DEVINFO_FCT_RESET:							/* Nothing */							break;
		case COCOCARTINFO_FCT_FF40_W:					info->f = (genf *) orch90_w;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Orch-90");				break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Orch-90");				break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");					break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);				break;
	}
}

DEFINE_LEGACY_CART_SLOT_DEVICE(COCO_CARTRIDGE_PCB_ORCH90, coco_cartridge_pcb_orch90);
