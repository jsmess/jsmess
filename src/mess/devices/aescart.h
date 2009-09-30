/*********************************************************************

    aescart.h

    AES cartridge management

*********************************************************************/

#ifndef __AESCART_H__
#define __AESCART_H__

#include "device.h"
#include "image.h"

#define AES_MULTICART			DEVICE_GET_INFO_NAME(aes_multicart)

#define AES_CARTRIDGE_PCB_NONE		DEVICE_GET_INFO_NAME(aes_cartridge_pcb_none)
#define AES_CARTRIDGE_PCB_STD		DEVICE_GET_INFO_NAME(aes_cartridge_pcb_std)	/* standard normal cart */

/* There's only 1 slot in an AES */
#define AES_NUMBER_OF_CARTRIDGE_SLOTS 1

/* Generic AES cartridge structure. */
struct _aescartridge_t
{
	/* PCB device associated to this cartridge. If NULL, the slot is empty. */
	const device_config *pcb;
};
typedef struct _aescartridge_t aescartridge_t;

DEVICE_GET_INFO(aes_multicart);

/* cartridge PCB types */
DEVICE_GET_INFO(aes_cartridge_pcb_std);

#define MDRV_AES_CARTRIDGE_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, AES_MULTICART, 0)

#endif /* __AESCART_H__ */
