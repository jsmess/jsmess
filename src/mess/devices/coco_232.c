/***************************************************************************

    coco_232.c

    Code for emulating the CoCo RS-232 PAK

***************************************************************************/

#include "emu.h"
#include "cococart.h"
#include "machine/6551.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define UART_TAG		"uart"


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _coco_rs232_pcb_t coco_rs232_pcb_t;
struct _coco_rs232_pcb_t
{
	device_t *cococart;
	device_t *cart;
	device_t *uart;
};


/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE coco_rs232_pcb_t *get_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == COCO_CARTRIDGE_PCB_RS232);
	return (coco_rs232_pcb_t *) downcast<legacy_device_base *>(device)->token();
}


/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    DEVICE_START(coco_rs232)
-------------------------------------------------*/

static DEVICE_START(coco_rs232)
{
	coco_rs232_pcb_t *pak_pcb = get_token(device);

	memset(pak_pcb, 0, sizeof(*pak_pcb));
	pak_pcb->cococart = device->owner()->owner();
	pak_pcb->cart = device->owner();
	pak_pcb->uart = device->subdevice(UART_TAG);
}


/*-------------------------------------------------
    READ8_DEVICE_HANDLER(coco_rs232_ff40_r)
-------------------------------------------------*/

static READ8_DEVICE_HANDLER(coco_rs232_ff40_r)
{
	UINT8 result = 0x00;
	coco_rs232_pcb_t *pak_pcb = get_token(device);

	if ((offset >= 0x28) && (offset <= 0x2F))
		result = acia_6551_r(pak_pcb->uart, offset - 0x28);

	return result;
}


/*-------------------------------------------------
    WRITE8_DEVICE_HANDLER(coco_rs232_ff40_r)
-------------------------------------------------*/

static WRITE8_DEVICE_HANDLER(coco_rs232_ff40_w)
{
	coco_rs232_pcb_t *pak_pcb = get_token(device);

	if ((offset >= 0x28) && (offset <= 0x2F))
		acia_6551_w(pak_pcb->uart, offset - 0x28, data);
}


/*-------------------------------------------------
    DEVICE_GET_INFO(coco_cartridge_pcb_rs232)
-------------------------------------------------*/
static MACHINE_CONFIG_FRAGMENT(coco_rs232)
	MCFG_ACIA6551_ADD(UART_TAG)
MACHINE_CONFIG_END

DEVICE_GET_INFO(coco_cartridge_pcb_rs232)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(coco_rs232_pcb_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;

		/* --- the following bits of info are returned as pointers to data --- */
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(coco_rs232);	break;

		/* --- the following bits of info are returned as pointers to functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(coco_rs232);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							/* Nothing */								break;
		case COCOCARTINFO_FCT_FF40_R:					info->f = (genf *) coco_rs232_ff40_r;		break;
		case COCOCARTINFO_FCT_FF40_W:					info->f = (genf *) coco_rs232_ff40_w;		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "CoCo RS-232 PAK");			break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "CoCo RS-232 PAK");			break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						/* Nothing */								break;
	}
}

DEFINE_LEGACY_CART_SLOT_DEVICE(COCO_CARTRIDGE_PCB_RS232, coco_cartridge_pcb_rs232);
