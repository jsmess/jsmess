/***************************************************************************

	NEC uPD1990AC CMOS Digital Integrated Circuit
	Serial I/O Calendar & Clock
	CMOS LSI

***************************************************************************/

#ifndef __UPD1990A_H__
#define __UPD1990A_H__


#define UPD1990A		DEVICE_GET_INFO_NAME(upd1990a)

#define MDRV_UPD1990A_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, UPD1990A, 0)

#define MDRV_UPD1990A_REMOVE(_tag) \
	MDRV_DEVICE_REMOVE(_tag)


/* device interface */
DEVICE_GET_INFO( upd1990a );

READ8_DEVICE_HANDLER( upd1990a_data_out_r );
WRITE8_DEVICE_HANDLER( upd1990a_w );
WRITE8_DEVICE_HANDLER( upd1990a_clk_w );


#endif /* __UPD1990A_H__ */
