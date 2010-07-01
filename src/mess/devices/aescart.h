/*********************************************************************

    aescart.h

    AES cartridge management

*********************************************************************/

#ifndef __AESCART_H__
#define __AESCART_H__

#include "image.h"
#include "cartslot.h"

DECLARE_LEGACY_CART_SLOT_DEVICE(AES_MULTICART, aes_multicart);
DECLARE_LEGACY_DEVICE(AES_CARTRIDGE_PCB_NONE, aes_cartridge_pcb_none);
DECLARE_LEGACY_DEVICE(AES_CARTRIDGE_PCB_STD, aes_cartridge_pcb_std);

/* There's only 1 slot in an AES */
#define AES_NUMBER_OF_CARTRIDGE_SLOTS 1

/* Generic AES cartridge structure. */
struct _aescartridge_t
{
	/* PCB device associated to this cartridge. If NULL, the slot is empty. */
	running_device *pcb;
};
typedef struct _aescartridge_t aescartridge_t;

#define MDRV_AES_CARTRIDGE_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, AES_MULTICART, 0)

#endif /* __AESCART_H__ */
