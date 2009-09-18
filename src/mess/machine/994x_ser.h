#ifndef __994X_SER_H__
#define __994X_SER_H__

#define TI99_4_PIO	DEVICE_GET_INFO_NAME(ti99_4_pio)

#define MDRV_TI99_4_PIO_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, TI99_4_PIO, 0)

/* device interface */
DEVICE_GET_INFO( ti99_4_pio );

/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( ti99_4_rs232_card );

/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define TI99_4_RS232_CARD	DEVICE_GET_INFO_NAME(ti99_4_rs232_card)

#define MDRV_TI99_4_RS232_CARD_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, TI99_4_RS232_CARD, 0)

#endif /* __994X_SER_H__ */

