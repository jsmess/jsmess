#ifndef __994X_SER_H__
#define __994X_SER_H__

DECLARE_LEGACY_IMAGE_DEVICE(TI99_4_PIO, ti99_4_pio);

#define MDRV_TI99_4_PIO_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, TI99_4_PIO, 0)

/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

DECLARE_LEGACY_DEVICE(TI99_4_RS232_CARD, ti99_4_rs232_card);

#define MDRV_TI99_4_RS232_CARD_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, TI99_4_RS232_CARD, 0)

#endif /* __994X_SER_H__ */

