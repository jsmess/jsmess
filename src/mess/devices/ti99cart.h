/*********************************************************************

	ti99cart.h

	TI99 family cartridge management

*********************************************************************/

#ifndef __TI99CART_H__
#define __TI99CART_H__

#include "device.h"
#include "image.h"

#define TI99_MULTICART			DEVICE_GET_INFO_NAME(ti99_multicart)

#define TI99_CARTRIDGE_PCB_NONE		DEVICE_GET_INFO_NAME(ti99_cartridge_pcb_none)
#define TI99_CARTRIDGE_PCB_STD		DEVICE_GET_INFO_NAME(ti99_cartridge_pcb_std)
#define TI99_CARTRIDGE_PCB_PAGED	DEVICE_GET_INFO_NAME(ti99_cartridge_pcb_paged)
#define TI99_CARTRIDGE_PCB_MINIMEM	DEVICE_GET_INFO_NAME(ti99_cartridge_pcb_minimem)
#define TI99_CARTRIDGE_PCB_SUPER	DEVICE_GET_INFO_NAME(ti99_cartridge_pcb_super)
#define TI99_CARTRIDGE_PCB_MBX		DEVICE_GET_INFO_NAME(ti99_cartridge_pcb_mbx)

/* We set the number of slots to 8, although we may have up to 16. From a 
   logical point of view we could have 256, but the operating system only checks
   the first 16 banks. */
#define NUMBER_OF_CARTRIDGE_SLOTS 8

/* GROM_port_t: descriptor for a port of 8 GROMs */
struct _GROM_port_t
{
	/* pointer to GROM data */
	UINT8 *data_ptr;
	/* current address pointer for the active GROM in port (16 bits) */
	unsigned int addr;
	/* GROM data buffer */
	UINT8 buf;
	/* internal flip-flops that are set after the first access to the GROM
	address so that next access is mapped to the LSB, and cleared after each
	data access */
	char raddr_LSB, waddr_LSB;
};
typedef struct _GROM_port_t GROM_port_t;

/* Generic TI99 cartridge structure. */
struct _cartridge_t
{
	/* PCB device associated to this cartridge. If NULL, the slot is empty. */
	const device_config *pcb;
	
        /* GROM buffer size. */
        int grom_size;

        /* ROM page. */
        int rom_page;
        /* RAM page. */
        int ram_page;

        /* ROM buffer size. All banks have equal sizes. */
        int rom_size;
        /* RAM buffer size. All banks have equal sizes. */
        int ram_size;

        /* pointer to GROM data. */
        UINT8 *grom_ptr;

        /* GROM buffered data output. */
        UINT8 grom_buffer;

        /* ROM buffer. We are using this for both 16 bit (99/4a) and 8 bit 
	   (99/8) access. */
        void *rom_ptr;
	
        /* ROM buffer for the second bank of paged cartridges. Other cartridges 
	   usually store their ROM in one large file. */
        void *rom2_ptr;

        /* RAM buffer. The persistence service is done by the cartridge system.
	   The RAM space is a consecutive space; all banks are in one buffer. */
        void *ram_ptr;
};
typedef struct _cartridge_t cartridge_t;

/* Functions to be called from the console. */
void   cartridge_slot_set(const device_config *cartsys, int slotnumber);
void   lock_cartridge_slot(const device_config *cartsys, int slotnumber);
UINT8  cartridge_grom_read(const device_config *cartsys, int offset);

DEVICE_GET_INFO(ti99_multicart);

/* cartridge PCB types */
DEVICE_GET_INFO(ti99_cartridge_pcb_std);
DEVICE_GET_INFO(ti99_cartridge_pcb_minimem);
DEVICE_GET_INFO(ti99_cartridge_pcb_mbx);

READ16_DEVICE_HANDLER(ti99_multicart_r);
WRITE16_DEVICE_HANDLER(ti99_multicart_w);

/* Support for TI-99/8 */
READ8_DEVICE_HANDLER(ti99_multicart8_r);
WRITE8_DEVICE_HANDLER(ti99_multicart8_w);

/* CRU handlers for SuperSpace cartridge */
READ8_DEVICE_HANDLER( ti99_multicart_cru_r );
WRITE8_DEVICE_HANDLER( ti99_multicart_cru_w );

#define MDRV_TI99_CARTRIDGE_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, TI99_MULTICART, 0)

#endif /* __TI99CART_H__ */
