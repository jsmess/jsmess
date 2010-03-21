/*********************************************************************

    ti99cart.h

    TI99 family cartridge management

*********************************************************************/

#ifndef __TI99CART_H__
#define __TI99CART_H__

#define TI99_MULTICART	DEVICE_GET_INFO_NAME(ti99_multicart)

/* We set the number of slots to 8, although we may have up to 16. From a
   logical point of view we could have 256, but the operating system only checks
   the first 16 banks. */
#define NUMBER_OF_CARTRIDGE_SLOTS 8

READ8_DEVICE_HANDLER(ti99_cartridge_grom_r);
WRITE8_DEVICE_HANDLER(ti99_cartridge_grom_w);

DEVICE_GET_INFO(ti99_multicart);

READ16_DEVICE_HANDLER(ti99_multicart_r);
WRITE16_DEVICE_HANDLER(ti99_multicart_w);

/* Support for TI-99/8 */
READ8_DEVICE_HANDLER(ti99_multicart8_r);
WRITE8_DEVICE_HANDLER(ti99_multicart8_w);

/* CRU handlers for SuperSpace and Pagedcru cartridge */
READ8_DEVICE_HANDLER( ti99_multicart_cru_r );
WRITE8_DEVICE_HANDLER( ti99_multicart_cru_w );

#define MDRV_TI99_CARTRIDGE_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, TI99_MULTICART, 0)

#endif /* __TI99CART_H__ */
